#include "frame_stepper.hpp"

#include <windows.h>

#include "core/game.hpp"
#include "core/log.hpp"
#include "features/global/speedhack.hpp"
#include "gui/keybinds.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

constexpr float kStepDt = 1.f / 240.f; // un tick de física a 240 TPS
constexpr const char* kStepLabel = "Next frame";
constexpr ULONGLONG kRepeatDelayMs = 300; // mantener la tecla: empieza a repetir tras esto

bool g_enabled = false;
int g_pendingSteps = 0;
ULONGLONG g_heldSince = 0;

// Antes de GJBaseGameLayer::update: false = este frame el nivel no avanza
bool updateFilter(void* gameLayer, float& dt) {
    if (!g_enabled || gameLayer != core::game::playLayer()) return true;
    if (g_pendingSteps <= 0) return false;
    --g_pendingSteps;
    dt = kStepDt;
    return true;
}

} // namespace

void FrameStepper::init(bool gameAvailable) {
    m_available = gameAvailable && core::game::levelHooksAvailable();
    if (m_available) core::game::setUpdateFilter(&updateFilter);
    core::logMsg("Frame stepper: %s", m_available ? "listo" : "no disponible (hooks de nivel)");
}

void FrameStepper::enable() {
    m_enabled = true;
    g_enabled = m_available;
    g_pendingSteps = 0;
    setGameAudioPaused(g_enabled);
}

void FrameStepper::disable() {
    m_enabled = false;
    g_enabled = false;
    g_pendingSteps = 0;
    setGameAudioPaused(false);
}

void FrameStepper::update() {
    // El audio solo se pausa dentro de un nivel (en los menús la música sigue)
    setGameAudioPaused(g_enabled && core::game::playLayer());
    if (!g_enabled) return;
    // Tecla de "Next frame" mantenida: tras un momento, un tick por frame
    if (gui::keybinds::held(kStepLabel)) {
        if (!g_heldSince) g_heldSince = GetTickCount64();
        else if (GetTickCount64() - g_heldSince > kRepeatDelayMs && g_pendingSteps == 0) g_pendingSteps = 1;
    } else {
        g_heldSince = 0;
    }
}

bool FrameStepper::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Frame stepper", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    if (gui::button(kStepLabel, m_available) && m_enabled) ++g_pendingSteps;
    return changed;
}

} // namespace features
