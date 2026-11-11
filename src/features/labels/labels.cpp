#include "labels.hpp"

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <imgui.h>
#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/input.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

using PercentFn = float (*)(void* playLayer);
PercentFn g_currentPercent = nullptr; // PlayLayer::getCurrentPercent
int32_t g_attemptsOffset = 0;         // PlayLayer::m_attempts, leído de kUpdateAttempts

// Una línea con sombra; avanza `y` para la siguiente
void line(ImDrawList* draw, float& y, ImU32 color, const char* text) {
    const ImVec2 pos(8.f, y);
    draw->AddText({pos.x + 1.f, pos.y + 1.f}, IM_COL32(0, 0, 0, 200), text);
    draw->AddText(pos, color, text);
    y += ImGui::GetFontSize() + 2.f;
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
    changed |= gui::toggle("Percentage", m_levelInfo, m_showPercent);
    changed |= gui::toggle("Attempt", m_levelInfo, m_showAttempt);
    changed |= gui::toggle("Level time", m_time, m_showTime);
    changed |= gui::toggle("Clock", true, m_showClock);
    return changed;
}

void Labels::drawOverlay(bool cheatsActive) {
    // Solo dentro de un nivel
    void* playLayer = core::game::playLayer();
    if (!m_show || !playLayer) return;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    const ImU32 white = IM_COL32(255, 255, 255, 255);
    float y = 8.f;
    char text[64];

    if (m_showCheat) {
        // Punto verde = partida limpia, rojo = hay alguna trampa activa
        const float radius = ImGui::GetFontSize() * 0.3f;
        draw->AddCircleFilled({8.f + radius, y + ImGui::GetFontSize() * 0.5f}, radius,
                              cheatsActive ? IM_COL32(255, 60, 60, 255) : IM_COL32(60, 220, 60, 255));
        y += ImGui::GetFontSize() + 2.f;
    }
    if (m_showFps) {
        snprintf(text, sizeof(text), "%.0f FPS", ImGui::GetIO().Framerate);
        line(draw, y, white, text);
    }
    if (m_showCps) {
        snprintf(text, sizeof(text), "%d CPS", core::input::clicksPerSecond());
        line(draw, y, core::input::jumpHeld() ? IM_COL32(120, 255, 120, 255) : white, text);
    }
    if (m_showPercent && m_levelInfo) {
        snprintf(text, sizeof(text), "%.2f%%", g_currentPercent(playLayer));
        line(draw, y, white, text);
    }
    if (m_showAttempt && m_levelInfo) {
        int32_t attempts;
        memcpy(&attempts, static_cast<const uint8_t*>(playLayer) + g_attemptsOffset, 4);
        snprintf(text, sizeof(text), "Attempt %d", attempts);
        line(draw, y, white, text);
    }
    if (m_showTime && m_time) {
        const double seconds = core::game::attemptTime();
        snprintf(text, sizeof(text), "%02d:%05.2f", static_cast<int>(seconds) / 60, seconds - 60 * (static_cast<int>(seconds) / 60));
        line(draw, y, white, text);
    }
    if (m_showClock) {
        SYSTEMTIME now;
        GetLocalTime(&now);
        snprintf(text, sizeof(text), "%02d:%02d:%02d", now.wHour, now.wMinute, now.wSecond);
        line(draw, y, white, text);
    }
}

void Labels::load(const nlohmann::json& j) {
    m_show = j.value("show", m_show);
    m_showCheat = j.value("cheat_indicator", m_showCheat);
    m_showFps = j.value("fps", m_showFps);
    m_showCps = j.value("cps", m_showCps);
    m_showPercent = j.value("percentage", m_showPercent);
    m_showAttempt = j.value("attempt", m_showAttempt);
    m_showTime = j.value("level_time", m_showTime);
    m_showClock = j.value("clock", m_showClock);
}

void Labels::save(nlohmann::json& j) const {
    j = {{"show", m_show},           {"cheat_indicator", m_showCheat}, {"fps", m_showFps},
         {"cps", m_showCps},         {"percentage", m_showPercent},    {"attempt", m_showAttempt},
         {"level_time", m_showTime}, {"clock", m_showClock}};
}

} // namespace features
