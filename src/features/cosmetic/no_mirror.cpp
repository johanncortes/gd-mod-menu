#include "no_mirror.hpp"

#include <windows.h>

#include <MinHook.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

using ToggleFlippedFn = void (*)(void* gameLayer, bool flip, bool noEffects);

ToggleFlippedFn g_origToggleFlipped = nullptr;
bool g_enabled = false;
bool g_instant = false;

void toggleFlippedHook(void* gameLayer, bool flip, bool noEffects) {
    g_origToggleFlipped(gameLayer, g_enabled ? false : flip, noEffects || g_instant);
}

} // namespace

void NoMirror::init() {
    uint8_t* toggleFlipped = core::memory::findPattern(GetModuleHandleW(nullptr), core::signatures::kToggleFlipped);
    if (!toggleFlipped) {
        core::logMsg("No mirror: no disponible (firma kToggleFlipped no encontrada)");
        return;
    }
    m_available = MH_CreateHook(toggleFlipped, &toggleFlippedHook, reinterpret_cast<void**>(&g_origToggleFlipped)) ==
                      MH_OK &&
                  MH_EnableHook(toggleFlipped) == MH_OK;
    core::logMsg("No mirror: %s", m_available ? "listo" : "no disponible (hook)");
}

void NoMirror::enable() {
    m_enabled = true;
    g_enabled = m_available;
}

void NoMirror::disable() {
    m_enabled = false;
    g_enabled = false;
}

bool NoMirror::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("No mirror", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    if (gui::toggle("Instant mirror", m_available, m_instant)) {
        g_instant = m_instant && m_available;
        return true;
    }
    return changed;
}

void NoMirror::load(const nlohmann::json& j) {
    j.value("no_mirror", false) ? enable() : disable();
    m_instant = j.value("instant_mirror", false);
    g_instant = m_instant && m_available;
}

void NoMirror::save(nlohmann::json& j) const {
    j["no_mirror"] = m_enabled;
    j["instant_mirror"] = m_instant;
}

} // namespace features
