#include "game_font.hpp"

#include <windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include "core/log.hpp"

namespace gui::gamefont {

namespace {

// --- Exports de libcocos2d.dll ---
using SharedFn = void* (*)();
using AddImageFn = void* (*)(void* cache, const char* path, bool);
using TextureUIntFn = unsigned (*)(void* texture);
using TextureBoolFn = bool (*)(void* texture);
using RefFn = void (*)(void* object);
using ScaleFactorFn = float (*)(void* director);

struct Cocos {
    bool tried = false;
    bool ok = false;
    SharedFn sharedTextureCache = nullptr;
    AddImageFn addImage = nullptr;
    TextureUIntFn getName = nullptr;
    TextureBoolFn premultiplied = nullptr;
    RefFn retain = nullptr;
    RefFn release = nullptr;
    SharedFn sharedDirector = nullptr;
    ScaleFactorFn contentScale = nullptr;
} g_cocos;

bool loadCocos() {
    if (g_cocos.tried) return g_cocos.ok;
    g_cocos.tried = true;
    HMODULE dll = GetModuleHandleW(L"libcocos2d.dll");
    if (!dll) return false;
    auto get = [dll](const char* name) { return reinterpret_cast<void*>(GetProcAddress(dll, name)); };
    g_cocos.sharedTextureCache = reinterpret_cast<SharedFn>(get("?sharedTextureCache@CCTextureCache@cocos2d@@SAPEAV12@XZ"));
    g_cocos.addImage = reinterpret_cast<AddImageFn>(get("?addImage@CCTextureCache@cocos2d@@QEAAPEAVCCTexture2D@2@PEBD_N@Z"));
    g_cocos.getName = reinterpret_cast<TextureUIntFn>(get("?getName@CCTexture2D@cocos2d@@UEAAIXZ"));
    g_cocos.premultiplied = reinterpret_cast<TextureBoolFn>(get("?hasPremultipliedAlpha@CCTexture2D@cocos2d@@QEAA_NXZ"));
    g_cocos.retain = reinterpret_cast<RefFn>(get("?retain@CCObject@cocos2d@@QEAAXXZ"));
    g_cocos.release = reinterpret_cast<RefFn>(get("?release@CCObject@cocos2d@@QEAAXXZ"));
    g_cocos.sharedDirector = reinterpret_cast<SharedFn>(get("?sharedDirector@CCDirector@cocos2d@@SAPEAV12@XZ"));
    g_cocos.contentScale = reinterpret_cast<ScaleFactorFn>(get("?getContentScaleFactor@CCDirector@cocos2d@@QEAAMXZ"));
    g_cocos.ok = g_cocos.sharedTextureCache && g_cocos.addImage && g_cocos.getName &&
                 g_cocos.premultiplied && g_cocos.retain && g_cocos.release &&
                 g_cocos.sharedDirector && g_cocos.contentScale;
    if (!g_cocos.ok) core::logMsg("Fuentes de GD: faltan exports de libcocos2d, se usa la fuente del panel");
    return g_cocos.ok;
}

// --- Fuente de bitmap (formato BMFont de texto) ---
struct Glyph {
    float x, y, w, h;      // en la textura, píxeles
    float xoff, yoff, adv; // píxeles de la fuente
};

struct BitmapFont {
    bool tried = false;
    bool ok = false;
    float loadedScale = 0.f; // contentScaleFactor con el que se cargó (-uhd, -hd o normal)
    std::unordered_map<int, Glyph> glyphs;
    std::unordered_map<uint64_t, float> kerning;
    float texW = 0.f, texH = 0.f;
    float top = 0.f, height = 0.f; // franja vertical que ocupan letras y números (sin el hueco del .fnt)
    std::filesystem::path pngPath;
    void* texture = nullptr; // CCTexture2D retenida
};

BitmapFont g_fonts[3]; // Big, Gold, Chat

const char* const kFiles[3] = {"bigFont", "goldFont", "chatFont"};

std::filesystem::path resourcesDir() {
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    return std::filesystem::path(exe).parent_path() / L"Resources";
}

// Valor entero de "clave=" dentro de una línea del .fnt
bool field(const std::string& line, const char* key, float& out) {
    const std::string pattern = std::string(" ") + key + "=";
    const size_t at = line.find(pattern);
    if (at == std::string::npos) return false;
    out = static_cast<float>(atof(line.c_str() + at + pattern.size()));
    return true;
}

void unload(BitmapFont& font) {
    if (font.texture && g_cocos.ok) g_cocos.release(font.texture);
    font = BitmapFont{};
}

// El juego elige -uhd, -hd o normal según la calidad de texturas (contentScaleFactor 4, 2 o 1)
bool load(BitmapFont& font, int index, float scaleFactor) {
    unload(font);
    font.tried = true;
    font.loadedScale = scaleFactor;
    const char* suffix = scaleFactor >= 3.f ? "-uhd" : scaleFactor >= 1.5f ? "-hd" : "";
    const std::filesystem::path dir = resourcesDir();
    const std::filesystem::path fnt = dir / (std::string(kFiles[index]) + suffix + ".fnt");
    std::ifstream file(fnt);
    if (!file) {
        core::logMsg("Fuentes de GD: no se pudo abrir %s", fnt.string().c_str());
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("common ", 0) == 0) {
            field(line, "scaleW", font.texW);
            field(line, "scaleH", font.texH);
        } else if (line.rfind("page ", 0) == 0) {
            const size_t at = line.find("file=\"");
            if (at != std::string::npos) {
                const size_t end = line.find('"', at + 6);
                font.pngPath = dir / line.substr(at + 6, end - at - 6);
            }
        } else if (line.rfind("char ", 0) == 0) {
            float id = 0.f;
            Glyph g{};
            if (field(line, "id", id) && field(line, "x", g.x) && field(line, "y", g.y) && field(line, "width", g.w) &&
                field(line, "height", g.h) && field(line, "xoffset", g.xoff) && field(line, "yoffset", g.yoff) &&
                field(line, "xadvance", g.adv))
                font.glyphs[static_cast<int>(id)] = g;
        } else if (line.rfind("kerning ", 0) == 0) {
            float first = 0.f, second = 0.f, amount = 0.f;
            if (field(line, "first", first) && field(line, "second", second) && field(line, "amount", amount))
                font.kerning[(static_cast<uint64_t>(first) << 32) | static_cast<uint32_t>(second)] = amount;
        }
    }
    if (font.glyphs.empty() || font.texW <= 0.f || font.texH <= 0.f || font.pngPath.empty()) {
        core::logMsg("Fuentes de GD: %s no tiene el formato esperado", fnt.string().c_str());
        return false;
    }

    // Franja útil: de lo más alto a lo más bajo de letras y números. El lineHeight del .fnt
    // deja mucho aire arriba y abajo, y apilar líneas con él separa demasiado los labels.
    float top = 1e9f, bottom = -1e9f;
    for (const auto& [id, g] : font.glyphs) {
        if (!((id >= '0' && id <= '9') || (id >= 'A' && id <= 'Z') || (id >= 'a' && id <= 'z')) || g.h <= 0.f) continue;
        top = std::min(top, g.yoff);
        bottom = std::max(bottom, g.yoff + g.h);
    }
    if (bottom <= top) return false;
    font.top = top;
    font.height = bottom - top;

    // La textura: la del caché de texturas del juego (si ya la tiene, no se vuelve a cargar)
    void* cache = g_cocos.sharedTextureCache();
    font.texture = cache ? g_cocos.addImage(cache, font.pngPath.string().c_str(), false) : nullptr;
    if (!font.texture) {
        core::logMsg("Fuentes de GD: no se pudo cargar %s", font.pngPath.string().c_str());
        return false;
    }
    g_cocos.retain(font.texture); // que siga viva aunque el juego vacíe su caché
    font.ok = true;
    return true;
}

// Fuente lista para dibujar, o nullptr si hay que usar la del panel
BitmapFont* current() {
    const int index = static_cast<int>(style().font);
    if (index < 0 || index >= 3 || !loadCocos()) return nullptr;
    void* director = g_cocos.sharedDirector();
    const float scaleFactor = director ? g_cocos.contentScale(director) : 4.f;
    BitmapFont& font = g_fonts[index];
    // Si el usuario cambia la calidad de texturas en GD, se recarga con el sufijo nuevo
    if (!font.tried || font.loadedScale != scaleFactor) load(font, index, scaleFactor);
    return font.ok ? &font : nullptr;
}

// Píxeles de pantalla por píxel de la fuente. GD dibuja en una pantalla de 320 puntos de alto;
// la fuente -uhd tiene 4 píxeles por punto. scale = 1 es el tamaño normal de la fuente en GD.
float pixelScale(const BitmapFont& font) {
    return ImGui::GetIO().DisplaySize.y / 320.f / font.loadedScale * style().scale;
}

ImU32 withOpacity(ImU32 color) {
    const float alpha = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.f * std::clamp(style().opacity, 0.f, 1.f);
    return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(alpha * 255.f + 0.5f) << IM_COL32_A_SHIFT);
}

// Las texturas de cocos tienen el alfa premultiplicado: se dibujan con glBlendFunc(ONE, 1-SRC_ALPHA)
void premultipliedBlend(const ImDrawList*, const ImDrawCmd*) {
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

// Respaldo: fuente del panel con sombra
float panelScale() {
    return std::max(0.3f, style().scale * 2.f);
}

} // namespace

Style& style() {
    static Style s;
    return s;
}

float lineHeight() {
    if (const BitmapFont* font = current()) return font->height * pixelScale(*font) * 1.15f;
    return ImGui::GetFontSize() * panelScale() + 2.f;
}

float textWidth(const char* text) {
    const BitmapFont* font = current();
    if (!font) return ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize() * panelScale(), FLT_MAX, 0.f, text).x;
    const float s = pixelScale(*font);
    float width = 0.f;
    int previous = 0;
    for (const unsigned char* c = reinterpret_cast<const unsigned char*>(text); *c; ++c) {
        const auto glyph = font->glyphs.find(*c);
        if (glyph == font->glyphs.end()) continue;
        const auto kern = font->kerning.find((static_cast<uint64_t>(previous) << 32) | *c);
        if (kern != font->kerning.end()) width += kern->second * s;
        width += glyph->second.adv * s;
        previous = *c;
    }
    return width;
}

void draw(ImDrawList* drawList, ImVec2 pos, ImU32 color, const char* text) {
    color = withOpacity(color);
    BitmapFont* font = current();
    if (!font) {
        const float size = ImGui::GetFontSize() * panelScale();
        const ImU32 shadow = withOpacity(IM_COL32(0, 0, 0, 220));
        drawList->AddText(ImGui::GetFont(), size, {pos.x + 1.f, pos.y + 1.f}, shadow, text);
        drawList->AddText(ImGui::GetFont(), size, pos, color, text);
        return;
    }

    const float s = pixelScale(*font);
    const bool premultiplied = g_cocos.premultiplied(font->texture);
    if (premultiplied) {
        // El color del vértice también tiene que ir premultiplicado
        const float a = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;
        auto channel = [&](int shift) { return static_cast<ImU32>(((color >> shift) & 0xFF) * a + 0.5f) << shift; };
        color = channel(IM_COL32_R_SHIFT) | channel(IM_COL32_G_SHIFT) | channel(IM_COL32_B_SHIFT) | (color & IM_COL32_A_MASK);
        drawList->AddCallback(&premultipliedBlend, nullptr);
    }

    const ImTextureID texture = static_cast<ImTextureID>(g_cocos.getName(font->texture));
    float x = pos.x;
    const float y = pos.y - font->top * s; // la franja útil empieza justo en pos.y
    int previous = 0;
    for (const unsigned char* c = reinterpret_cast<const unsigned char*>(text); *c; ++c) {
        const auto found = font->glyphs.find(*c);
        if (found == font->glyphs.end()) continue; // carácter que la fuente no tiene
        const Glyph& g = found->second;
        const auto kern = font->kerning.find((static_cast<uint64_t>(previous) << 32) | *c);
        if (kern != font->kerning.end()) x += kern->second * s;
        if (g.w > 0.f && g.h > 0.f) {
            const ImVec2 p0{x + g.xoff * s, y + g.yoff * s};
            const ImVec2 p1{p0.x + g.w * s, p0.y + g.h * s};
            drawList->AddImage(texture, p0, p1, {g.x / font->texW, g.y / font->texH},
                               {(g.x + g.w) / font->texW, (g.y + g.h) / font->texH}, color);
        }
        x += g.adv * s;
        previous = *c;
    }

    if (premultiplied) drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
}

void shutdown() {
    for (BitmapFont& font : g_fonts) unload(font);
}

} // namespace gui::gamefont
