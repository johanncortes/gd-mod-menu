#include "labels.hpp"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <imgui.h>
#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/input.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "features/player/noclip.hpp"
#include "gui/game_font.hpp"
#include "gui/menu.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;
namespace gf = gui::gamefont;

using PercentFn = float (*)(void* playLayer);
PercentFn g_currentPercent = nullptr; // PlayLayer::getCurrentPercent
int32_t g_attemptsOffset = 0;         // PlayLayer::m_attempts, leído de kUpdateAttempts

constexpr float kMinScale = 0.2f, kMaxScale = 1.5f;
constexpr float kMinOpacity = 0.1f;
constexpr float kDefaultX = 0.01f, kDefaultY = 0.015f;

const ImU32 kWhite = IM_COL32(255, 255, 255, 255);
const ImU32 kRed = IM_COL32(255, 90, 90, 255);
const ImU32 kGreen = IM_COL32(120, 255, 120, 255);

struct Line {
    std::string text; // vacío = punto del cheat indicator
    ImU32 color;
};

// Arrastre del bloque (con el panel abierto)
bool g_dragging = false;
ImVec2 g_dragTopLeft; // esquina superior izquierda mientras se arrastra, en píxeles

ImU32 withAlpha(ImU32 color, float alpha) {
    return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(255.f * std::clamp(alpha, 0.f, 1.f)) << IM_COL32_A_SHIFT);
}

} // namespace

void Labels::init(bool gameAvailable) {
    HMODULE exe = GetModuleHandleW(nullptr);
    g_currentPercent = reinterpret_cast<PercentFn>(core::memory::findPattern(exe, sig::kCurrentPercent));
    if (const uint8_t* updateAttempts = core::memory::findPattern(exe, sig::kUpdateAttempts))
        memcpy(&g_attemptsOffset, updateAttempts + sig::kUpdateAttemptsOffset, 4);
    m_levelInfo = gameAvailable && g_currentPercent && g_attemptsOffset > 0 && g_attemptsOffset < 0x10000;
    m_time = gameAvailable && core::game::levelHooksAvailable();
    core::logMsg("Labels: porcentaje e intentos %s, tiempo %s", m_levelInfo ? "listos" : "no disponibles",
                 m_time ? "listo" : "no disponible");
}

bool Labels::drawUI() {
    bool changed = gui::toggle("Show labels", true, m_show);
    changed |= gui::toggle("Cheat indicator", true, m_showCheat);
    changed |= gui::toggle("FPS", true, m_showFps);
    changed |= gui::toggle("CPS", true, m_showCps);
    changed |= gui::toggle("Noclip deaths", m_time, m_showNoclipDeaths);
    changed |= gui::toggle("Noclip accuracy", m_time, m_showNoclipAccuracy);
    changed |= gui::toggle("Percentage", m_levelInfo, m_showPercent);
    changed |= gui::toggle("Attempt", m_levelInfo, m_showAttempt);
    changed |= gui::toggle("Level time", m_time, m_showTime);
    changed |= gui::toggle("Clock", true, m_showClock);

    // Aspecto (también lo usa el aviso del StartPos switcher)
    ImGui::Separator();
    gf::Style& style = gf::style();
    ImGui::PushItemWidth(140.f * ImGui::GetIO().FontGlobalScale);
    int font = static_cast<int>(style.font);
    if (ImGui::Combo("Fuente", &font, gf::kFontNames, gf::kFontCount)) {
        style.font = static_cast<gf::Font>(font);
        changed = true;
    }
    if (ImGui::SliderFloat("Tamaño", &style.scale, kMinScale, kMaxScale, "x%.2f")) {
        style.scale = std::clamp(style.scale, kMinScale, kMaxScale);
        changed = true;
    }
    float opacity = style.opacity * 100.f;
    if (ImGui::SliderFloat("Opacidad", &opacity, kMinOpacity * 100.f, 100.f, "%.0f%%")) {
        style.opacity = std::clamp(opacity / 100.f, kMinOpacity, 1.f);
        changed = true;
    }
    ImGui::PopItemWidth();

    if (ImGui::Button("Restablecer posición")) {
        m_x = kDefaultX;
        m_y = kDefaultY;
        m_alignRight = false;
        changed = true;
    }
    ImGui::TextDisabled("Dentro de un nivel, con el panel\nabierto, arrastra los labels.");
    return changed;
}

void Labels::drawOverlay(bool cheatsActive) {
    // Solo dentro de un nivel
    void* playLayer = core::game::playLayer();
    if (!m_show || !playLayer) {
        g_dragging = false;
        return;
    }

    // --- Líneas activas ---
    std::vector<Line> lines;
    char text[64];
    if (m_showCheat) lines.push_back({"", cheatsActive ? IM_COL32(255, 65, 65, 255) : IM_COL32(60, 220, 60, 255)});
    if (m_showFps) {
        snprintf(text, sizeof(text), "%.0f FPS", ImGui::GetIO().Framerate);
        lines.push_back({text, kWhite});
    }
    if (m_showCps) {
        snprintf(text, sizeof(text), "%d CPS", core::input::clicksPerSecond());
        lines.push_back({text, core::input::jumpHeld() ? kGreen : kWhite});
    }
    NoclipStats noclip;
    if ((m_showNoclipDeaths || m_showNoclipAccuracy) && noclipStats(noclip)) {
        if (m_showNoclipDeaths) {
            snprintf(text, sizeof(text), "%d Deaths", noclip.deaths);
            lines.push_back({text, noclip.hitting ? kRed : kWhite});
        }
        if (m_showNoclipAccuracy) {
            snprintf(text, sizeof(text), "%.2f%%", noclip.accuracy);
            lines.push_back({text, noclip.hitting ? kRed : kWhite});
        }
    }
    if (m_showPercent && m_levelInfo) {
        snprintf(text, sizeof(text), "%.2f%%", g_currentPercent(playLayer));
        lines.push_back({text, kWhite});
    }
    if (m_showAttempt && m_levelInfo) {
        int32_t attempts = 0;
        memcpy(&attempts, static_cast<const uint8_t*>(playLayer) + g_attemptsOffset, 4);
        snprintf(text, sizeof(text), "Attempt %d", attempts);
        lines.push_back({text, kWhite});
    }
    if (m_showTime && m_time) {
        const double seconds = core::game::attemptTime();
        const int minutes = static_cast<int>(seconds) / 60;
        snprintf(text, sizeof(text), "%d:%05.2f", minutes, seconds - 60.0 * minutes);
        lines.push_back({text, kWhite});
    }
    if (m_showClock) {
        SYSTEMTIME now;
        GetLocalTime(&now);
        snprintf(text, sizeof(text), "%02d:%02d:%02d", now.wHour, now.wMinute, now.wSecond);
        lines.push_back({text, kWhite});
    }
    if (lines.empty()) {
        g_dragging = false;
        return;
    }

    // --- Tamaño y posición del bloque ---
    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    if (screen.x <= 0.f || screen.y <= 0.f) return;
    const float lineStep = gf::lineHeight();
    const float dot = lineStep * 0.7f; // diámetro del punto del cheat indicator
    std::vector<float> widths;
    float blockW = 0.f;
    for (const Line& line : lines) {
        widths.push_back(line.text.empty() ? dot : gf::textWidth(line.text.c_str()));
        blockW = std::max(blockW, widths.back());
    }
    const float blockH = lineStep * static_cast<float>(lines.size());

    ImVec2 topLeft;
    bool alignRight = m_alignRight;
    if (g_dragging) {
        topLeft = g_dragTopLeft;
        alignRight = topLeft.x + blockW * 0.5f > screen.x * 0.5f;
    } else {
        topLeft = {m_alignRight ? m_x * screen.x - blockW : m_x * screen.x, m_y * screen.y};
    }
    // Siempre dentro de la pantalla
    topLeft.x = std::clamp(topLeft.x, 0.f, std::max(0.f, screen.x - blockW));
    topLeft.y = std::clamp(topLeft.y, 0.f, std::max(0.f, screen.y - blockH));

    // --- Dibujo: detrás de las ventanas del panel, delante del juego ---
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    const float opacity = gf::style().opacity;
    for (size_t i = 0; i < lines.size(); ++i) {
        const float x = alignRight ? topLeft.x + blockW - widths[i] : topLeft.x;
        const float y = topLeft.y + lineStep * static_cast<float>(i);
        if (lines[i].text.empty()) {
            const ImVec2 center{x + dot * 0.5f, y + lineStep * 0.45f};
            draw->AddCircleFilled(center, dot * 0.5f, IM_COL32(0, 0, 0, static_cast<int>(200 * opacity)));
            draw->AddCircleFilled(center, dot * 0.5f - std::max(1.f, dot * 0.12f), withAlpha(lines[i].color, opacity));
        } else {
            gf::draw(draw, {x, y}, lines[i].color, lines[i].text.c_str());
        }
    }

    // --- Arrastre, solo con el panel abierto. Una ventana invisible encima del bloque recoge
    // el ratón (así el clic no llega al juego). ---
    if (!gui::isMenuOpen()) {
        g_dragging = false;
        return;
    }
    const float pad = 4.f;
    ImGui::SetNextWindowPos({topLeft.x - pad, topLeft.y - pad}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({blockW + 2 * pad, blockH + 2 * pad}, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, {1.f, 1.f});
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 40));
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyleColorVec4(ImGuiCol_CheckMark));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove;
    if (ImGui::Begin("##labels_drag", nullptr, flags)) {
        ImGui::InvisibleButton("##handle", {blockW + 2 * pad, blockH + 2 * pad});
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        if (ImGui::IsItemActivated()) {
            g_dragging = true;
            g_dragTopLeft = topLeft;
        } else if (g_dragging && ImGui::IsItemActive()) {
            const ImVec2 delta = ImGui::GetIO().MouseDelta;
            g_dragTopLeft = {topLeft.x + delta.x, topLeft.y + delta.y};
        } else if (g_dragging) {
            // Al soltar se guarda el borde de anclaje según el lado de la pantalla
            g_dragging = false;
            m_alignRight = alignRight;
            m_x = (alignRight ? topLeft.x + blockW : topLeft.x) / screen.x;
            m_y = topLeft.y / screen.y;
            m_positionChanged = true;
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
}

bool Labels::takePositionChanged() {
    const bool changed = m_positionChanged;
    m_positionChanged = false;
    return changed;
}

void Labels::load(const nlohmann::json& j) {
    m_show = j.value("show", m_show);
    m_showCheat = j.value("cheat_indicator", m_showCheat);
    m_showFps = j.value("fps", m_showFps);
    m_showCps = j.value("cps", m_showCps);
    m_showNoclipDeaths = j.value("noclip_deaths", m_showNoclipDeaths);
    m_showNoclipAccuracy = j.value("noclip_accuracy", m_showNoclipAccuracy);
    m_showPercent = j.value("percentage", m_showPercent);
    m_showAttempt = j.value("attempt", m_showAttempt);
    m_showTime = j.value("level_time", m_showTime);
    m_showClock = j.value("clock", m_showClock);
    m_x = std::clamp(j.value("x", m_x), 0.f, 1.f);
    m_y = std::clamp(j.value("y", m_y), 0.f, 1.f);
    m_alignRight = j.value("align_right", m_alignRight);

    gf::Style& style = gf::style();
    const std::string font = j.value("font", std::string(gf::kFontNames[0]));
    for (int i = 0; i < gf::kFontCount; ++i)
        if (font == gf::kFontNames[i]) style.font = static_cast<gf::Font>(i);
    style.scale = std::clamp(j.value("scale", style.scale), kMinScale, kMaxScale);
    style.opacity = std::clamp(j.value("opacity", style.opacity), kMinOpacity, 1.f);
}

void Labels::save(nlohmann::json& j) const {
    const gf::Style& style = gf::style();
    j = {{"show", m_show},
         {"cheat_indicator", m_showCheat},
         {"fps", m_showFps},
         {"cps", m_showCps},
         {"noclip_deaths", m_showNoclipDeaths},
         {"noclip_accuracy", m_showNoclipAccuracy},
         {"percentage", m_showPercent},
         {"attempt", m_showAttempt},
         {"level_time", m_showTime},
         {"clock", m_showClock},
         {"x", m_x},
         {"y", m_y},
         {"align_right", m_alignRight},
         {"font", gf::kFontNames[static_cast<int>(style.font)]},
         {"scale", style.scale},
         {"opacity", style.opacity}};
}

} // namespace features
