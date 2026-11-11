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
#include "gui/menu.hpp"
#include "gui/theme.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

using PercentFn = float (*)(void* playLayer);
PercentFn g_currentPercent = nullptr; // PlayLayer::getCurrentPercent
int32_t g_attemptsOffset = 0;         // PlayLayer::m_attempts, leído de kUpdateAttempts

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
    changed |= gui::toggle("Fondo semitransparente", true, m_showBackground);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Posición: (%.0f, %.0f)", m_posX, m_posY);
    ImGui::TextDisabled("Abre con Tab para arrastrar los labels.");
    if (gui::button("Restablecer posición", true)) {
        m_posX = 10.f;
        m_posY = 10.f;
        m_resetPos = true;
        m_posChanged = true;
        changed = true;
    }
    return changed;
}

void Labels::drawOverlay(bool cheatsActive) {
    // Solo dentro de un nivel
    void* playLayer = core::game::playLayer();
    if (!m_show || !playLayer) return;

    const bool menuOpen = gui::isMenuOpen();
    if (menuOpen) {
        if (m_resetPos) {
            ImGui::SetNextWindowPos(ImVec2(m_posX, m_posY), ImGuiCond_Always);
            m_resetPos = false;
        } else {
            ImGui::SetNextWindowPos(ImVec2(m_posX, m_posY), ImGuiCond_Appearing);
        }
    } else {
        ImGui::SetNextWindowPos(ImVec2(m_posX, m_posY), ImGuiCond_Always);
    }

    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav;

    if (!menuOpen) {
        flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration;
        if (!m_showBackground) {
            flags |= ImGuiWindowFlags_NoBackground;
        }
    } else {
        flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    }

    const float padX = (m_showBackground || menuOpen) ? 8.f : 2.f;
    const float padY = (m_showBackground || menuOpen) ? 6.f : 2.f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padX, padY));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, (menuOpen || m_showBackground) ? 1.0f : 0.0f);

    if (m_showBackground || menuOpen) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.07f, 0.09f, menuOpen ? 0.85f : 0.60f));
        ImGui::PushStyleColor(ImGuiCol_Border, menuOpen ? ImVec4(0.35f, 0.65f, 1.0f, 0.80f) : ImVec4(0.20f, 0.22f, 0.28f, 0.50f));
    }

    if (ImGui::Begin("##labels_hud", nullptr, flags)) {
        if (menuOpen) {
            const ImVec2 currentPos = ImGui::GetWindowPos();
            if (currentPos.x != m_posX || currentPos.y != m_posY) {
                m_posX = currentPos.x;
                m_posY = currentPos.y;
                m_posChanged = true;
            }
            ImGui::TextDisabled("Labels [Arrastra]");
            ImGui::Separator();
        }

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImU32 white = IM_COL32(255, 255, 255, 255);
        const float fontSize = ImGui::GetFontSize();
        const float lineStep = fontSize + 2.f;

        auto drawLabelLine = [&](const char* text, ImU32 color) {
            if (!m_showBackground && !menuOpen) {
                const ImVec2 p = ImGui::GetCursorScreenPos();
                const ImU32 shadow = IM_COL32(0, 0, 0, 230);
                draw->AddText({p.x - 1.f, p.y}, shadow, text);
                draw->AddText({p.x + 1.f, p.y}, shadow, text);
                draw->AddText({p.x, p.y - 1.f}, shadow, text);
                draw->AddText({p.x, p.y + 1.f}, shadow, text);
                draw->AddText(p, color, text);
                const ImVec2 sz = ImGui::CalcTextSize(text);
                ImGui::Dummy(ImVec2(sz.x, lineStep));
            } else {
                ImGui::TextColored(ImColor(color), "%s", text);
            }
        };

        if (m_showCheat) {
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float radius = fontSize * 0.35f;
            const ImVec2 center = {p.x + radius + 1.f, p.y + fontSize * 0.5f};
            draw->AddCircleFilled(center, radius + 1.2f, IM_COL32(0, 0, 0, 220));
            draw->AddCircleFilled(center, radius, cheatsActive ? IM_COL32(255, 65, 65, 255) : IM_COL32(60, 220, 60, 255));
            ImGui::Dummy(ImVec2(radius * 2.f + 4.f, lineStep));
        }

        char text[64];
        if (m_showFps) {
            snprintf(text, sizeof(text), "%.0f FPS", ImGui::GetIO().Framerate);
            drawLabelLine(text, white);
        }
        if (m_showCps) {
            snprintf(text, sizeof(text), "%d CPS", core::input::clicksPerSecond());
            drawLabelLine(text, core::input::jumpHeld() ? IM_COL32(120, 255, 120, 255) : white);
        }
        if (m_showPercent && m_levelInfo) {
            snprintf(text, sizeof(text), "%.2f%%", g_currentPercent(playLayer));
            drawLabelLine(text, white);
        }
        if (m_showAttempt && m_levelInfo) {
            int32_t attempts = 0;
            memcpy(&attempts, static_cast<const uint8_t*>(playLayer) + g_attemptsOffset, 4);
            snprintf(text, sizeof(text), "Attempt %d", attempts);
            drawLabelLine(text, white);
        }
        if (m_showTime && m_time) {
            const double seconds = core::game::attemptTime();
            snprintf(text, sizeof(text), "%02d:%05.2f", static_cast<int>(seconds) / 60, seconds - 60 * (static_cast<int>(seconds) / 60));
            drawLabelLine(text, white);
        }
        if (m_showClock) {
            SYSTEMTIME now;
            GetLocalTime(&now);
            snprintf(text, sizeof(text), "%02d:%02d:%02d", now.wHour, now.wMinute, now.wSecond);
            drawLabelLine(text, white);
        }
    }
    ImGui::End();

    if (m_showBackground || menuOpen) {
        ImGui::PopStyleColor(2);
    }
    ImGui::PopStyleVar(3);
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
    m_showBackground = j.value("background", m_showBackground);
    m_posX = j.value("pos_x", m_posX);
    m_posY = j.value("pos_y", m_posY);
}

void Labels::save(nlohmann::json& j) const {
    j = {{"show", m_show},           {"cheat_indicator", m_showCheat}, {"fps", m_showFps},
         {"cps", m_showCps},         {"percentage", m_showPercent},    {"attempt", m_showAttempt},
         {"level_time", m_showTime}, {"clock", m_showClock},           {"background", m_showBackground},
         {"pos_x", m_posX},          {"pos_y", m_posY}};
}

bool Labels::takePositionChanged() {
    const bool changed = m_posChanged;
    m_posChanged = false;
    return changed;
}

} // namespace features
