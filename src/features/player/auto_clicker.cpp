#include "auto_clicker.hpp"

#include <windows.h>

#include <algorithm>

#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

constexpr float kMinCps = 1.f;
constexpr float kMaxCps = 60.f;

using HandleButtonFn = void (*)(void* gameLayer, bool down, int button, bool isPlayer1);
HandleButtonFn g_handleButton = nullptr;

long long now() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
}

long long frequency() {
    LARGE_INTEGER value;
    QueryPerformanceFrequency(&value);
    return value.QuadPart;
}

} // namespace

void AutoClicker::init(bool gameAvailable) {
    g_handleButton = reinterpret_cast<HandleButtonFn>(
        core::memory::findPattern(GetModuleHandleW(nullptr), core::signatures::kHandleButton));
    m_available = gameAvailable && g_handleButton;
    core::logMsg("Auto clicker: %s", m_available ? "listo" : "no disponible (firma kHandleButton)");
}

void AutoClicker::release() {
    if (!m_down) return;
    m_down = false;
    if (void* playLayer = core::game::playLayer()) g_handleButton(playLayer, false, 1, true);
}

void AutoClicker::enable() {
    m_enabled = true;
    m_nextToggle = 0;
}

void AutoClicker::disable() {
    m_enabled = false;
    if (m_available) release();
}

void AutoClicker::update() {
    if (!active()) return;
    void* playLayer = core::game::playLayer();
    if (!playLayer || !core::game::levelRunning()) {
        // Fuera del nivel o en pausa no se pulsa nada (y lo pulsado se olvida: el nivel ya no está)
        if (!playLayer) m_down = false;
        m_nextToggle = 0;
        return;
    }
    const long long t = now();
    if (m_nextToggle != 0 && t < m_nextToggle) return;
    m_down = !m_down;
    g_handleButton(playLayer, m_down, 1, true);
    const long long halfPeriod = static_cast<long long>(frequency() / (2.0 * m_cps));
    m_nextToggle = (m_nextToggle == 0 ? t : m_nextToggle) + halfPeriod;
    if (m_nextToggle < t) m_nextToggle = t + halfPeriod; // si el juego fue lento, no recuperar de golpe
}

bool AutoClicker::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggleValue("Auto clicker", m_available, enabled, m_cps, kMinCps, kMaxCps, "%.0f CPS");
    if (changed) enabled ? enable() : disable();
    return changed;
}

void AutoClicker::load(const nlohmann::json& j) {
    m_cps = std::clamp(j.value("cps", m_cps), kMinCps, kMaxCps);
    j.value("enabled", false) ? enable() : disable();
}

void AutoClicker::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"cps", m_cps}};
}

} // namespace features
