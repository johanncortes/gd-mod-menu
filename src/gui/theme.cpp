#include "theme.hpp"

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <string>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <nlohmann/json.hpp>

#include "config/config.hpp"
#include "core/log.hpp"

namespace gui::theme {

namespace {

const char* const kThemes[] = {"Oscuro", "Claro", "Clásico"};

// Fuentes: la de ImGui y algunas de Windows (archivo dentro de C:\Windows\Fonts)
struct FontChoice {
    const char* name;
    const char* file; // nullptr = fuente incluida en ImGui
};
const FontChoice kFonts[] = {
    {"ImGui (por defecto)", nullptr},
    {"Segoe UI", "segoeui.ttf"},
    {"Verdana", "verdana.ttf"},
    {"Tahoma", "tahoma.ttf"},
    {"Consolas", "consola.ttf"},
};

constexpr float kMinScale = 0.75f, kMaxScale = 2.f;
constexpr float kMinFontSize = 10.f, kMaxFontSize = 28.f;

int g_theme = 0;
float g_accent[3] = {0.26f, 0.59f, 0.98f}; // azul de ImGui
float g_scale = 1.f;
int g_font = 1; // 1 = Segoe UI (fuente moderna por defecto en Windows)
float g_fontSize = 16.f;
bool g_fontPending = true; // cargar la fuente al primer frame

std::string g_layoutPath; // ImGui guarda el puntero, tiene que seguir vivo

// Colores y escala: se pueden cambiar en cualquier momento
void applyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    switch (g_theme) {
    case 1: ImGui::StyleColorsLight(); break;
    case 2: ImGui::StyleColorsClassic(); break;
    default: {
        // Tema oscuro moderno estilo Mega Hack: grafito elegante con bordes y contraste limpios
        ImGui::StyleColorsDark();
        ImVec4* c = style.Colors;
        c[ImGuiCol_WindowBg]             = ImVec4(0.09f, 0.09f, 0.11f, 0.95f);
        c[ImGuiCol_ChildBg]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        c[ImGuiCol_PopupBg]              = ImVec4(0.10f, 0.11f, 0.14f, 0.96f);
        c[ImGuiCol_Border]               = ImVec4(0.20f, 0.22f, 0.28f, 0.65f);
        c[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        c[ImGuiCol_FrameBg]              = ImVec4(0.14f, 0.15f, 0.18f, 0.85f);
        c[ImGuiCol_TitleBg]              = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
        c[ImGuiCol_TitleBgActive]        = ImVec4(0.11f, 0.12f, 0.16f, 1.00f);
        c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.07f, 0.07f, 0.09f, 0.75f);
        c[ImGuiCol_MenuBarBg]            = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
        c[ImGuiCol_ScrollbarBg]          = ImVec4(0.07f, 0.07f, 0.09f, 0.50f);
        c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.22f, 0.24f, 0.30f, 0.80f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.33f, 0.40f, 0.90f);
        c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.38f, 0.42f, 0.50f, 1.00f);
        c[ImGuiCol_Separator]            = ImVec4(0.20f, 0.22f, 0.28f, 0.50f);
        break;
    }
    }

    // Geometría y bordes pulidos
    style.WindowRounding    = 6.f;
    style.ChildRounding     = 4.f;
    style.FrameRounding     = 4.f;
    style.PopupRounding     = 6.f;
    style.ScrollbarRounding = 9.f;
    style.GrabRounding      = 4.f;
    style.TabRounding       = 4.f;

    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.5f;
    style.PopupBorderSize   = 1.0f;

    style.WindowPadding     = ImVec2(10.f, 10.f);
    style.FramePadding      = ImVec2(6.f, 4.f);
    style.ItemSpacing       = ImVec2(8.f, 6.f);
    style.ItemInnerSpacing  = ImVec2(6.f, 4.f);
    style.ScrollbarSize     = 12.f;
    style.WindowTitleAlign  = ImVec2(0.5f, 0.5f);

    // El color de acento sustituye al azul de los elementos activos
    const auto accent = [](float alpha) { return ImVec4(g_accent[0], g_accent[1], g_accent[2], alpha); };
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_CheckMark] = accent(1.f);
    colors[ImGuiCol_SliderGrab] = accent(0.85f);
    colors[ImGuiCol_SliderGrabActive] = accent(1.f);
    colors[ImGuiCol_Button] = accent(0.40f);
    colors[ImGuiCol_ButtonHovered] = accent(0.75f);
    colors[ImGuiCol_ButtonActive] = accent(0.95f);
    colors[ImGuiCol_Header] = accent(0.30f);
    colors[ImGuiCol_HeaderHovered] = accent(0.65f);
    colors[ImGuiCol_HeaderActive] = accent(0.85f);
    colors[ImGuiCol_FrameBgHovered] = accent(0.35f);
    colors[ImGuiCol_FrameBgActive] = accent(0.60f);
    colors[ImGuiCol_TitleBgActive] = accent(0.70f);
    colors[ImGuiCol_SeparatorHovered] = accent(0.70f);
    colors[ImGuiCol_SeparatorActive] = accent(1.00f);
    colors[ImGuiCol_ResizeGrip] = accent(0.25f);
    colors[ImGuiCol_ResizeGripHovered] = accent(0.75f);
    colors[ImGuiCol_ResizeGripActive] = accent(1.f);
    colors[ImGuiCol_TextSelectedBg] = accent(0.35f);

    ImGui::GetIO().FontGlobalScale = g_scale;
}

} // namespace

void init() {
    g_layoutPath = config::siblingPath("gdmenu_layout.ini");
    ImGui::GetIO().IniFilename = g_layoutPath.c_str();
    applyStyle();
}

void applyPending() {
    if (!g_fontPending) return;
    g_fontPending = false;

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFont* font = nullptr;
    if (const char* file = kFonts[g_font].file) {
        wchar_t windows[MAX_PATH] = {};
        GetWindowsDirectoryW(windows, MAX_PATH);
        const std::filesystem::path path = std::filesystem::path(windows) / L"Fonts" / file;
        if (std::filesystem::exists(path))
            font = io.Fonts->AddFontFromFileTTF(path.string().c_str(), g_fontSize);
        if (!font) core::logMsg("Fuente %s no disponible: se usa la de ImGui", kFonts[g_font].name);
    }
    if (!font) {
        ImFontConfig config;
        config.SizePixels = g_font == 0 ? g_fontSize : 13.f;
        io.Fonts->AddFontDefault(&config);
    }
    io.Fonts->Build();
    // La textura de la fuente vive en el backend: hay que volver a subirla
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();
}

bool drawUI() {
    bool changed = false;
    ImGui::PushItemWidth(155.f * g_scale);

    changed |= ImGui::Combo("Tema", &g_theme, kThemes, IM_ARRAYSIZE(kThemes));
    changed |= ImGui::ColorEdit3("Color de acento", g_accent, ImGuiColorEditFlags_NoInputs);

    // La escala se aplica al soltar, para que el panel no cambie de tamaño mientras se arrastra
    static float scale = g_scale;
    if (!ImGui::IsAnyItemActive()) scale = g_scale;
    ImGui::SliderFloat("Escala", &scale, kMinScale, kMaxScale, "x%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        g_scale = std::clamp(scale, kMinScale, kMaxScale);
        changed = true;
    }

    if (ImGui::BeginCombo("Fuente", kFonts[g_font].name)) {
        for (int i = 0; i < IM_ARRAYSIZE(kFonts); ++i) {
            if (ImGui::Selectable(kFonts[i].name, i == g_font) && i != g_font) {
                g_font = i;
                g_fontPending = changed = true;
            }
        }
        ImGui::EndCombo();
    }
    static float fontSize = g_fontSize;
    if (!ImGui::IsAnyItemActive()) fontSize = g_fontSize;
    ImGui::SliderFloat("Tamaño de fuente", &fontSize, kMinFontSize, kMaxFontSize, "%.0f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        g_fontSize = std::clamp(fontSize, kMinFontSize, kMaxFontSize);
        g_fontPending = changed = true;
    }

    ImGui::PopItemWidth();
    if (changed) applyStyle();
    return changed;
}

void load(const nlohmann::json& j) {
    g_theme = std::clamp(j.value("theme", g_theme), 0, IM_ARRAYSIZE(kThemes) - 1);
    g_scale = std::clamp(j.value("scale", g_scale), kMinScale, kMaxScale);
    g_font = std::clamp(j.value("font", g_font), 0, IM_ARRAYSIZE(kFonts) - 1);
    g_fontSize = std::clamp(j.value("font_size", g_fontSize), kMinFontSize, kMaxFontSize);
    const auto accent = j.find("accent");
    if (accent != j.end() && accent->is_array() && accent->size() == 3)
        for (int i = 0; i < 3; ++i) g_accent[i] = std::clamp((*accent)[i].get<float>(), 0.f, 1.f);
    g_fontPending = true;
    applyStyle();
}

void save(nlohmann::json& j) {
    j = {{"theme", g_theme}, {"scale", g_scale}, {"font", g_font}, {"font_size", g_fontSize},
         {"accent", {g_accent[0], g_accent[1], g_accent[2]}}};
}

} // namespace gui::theme
