#include "auto_practice.hpp"

#include <windows.h>

#include <MinHook.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

using SetupCompletedFn = void (*)(void* playLayer);
using TogglePracticeFn = void (*)(void* playLayer, bool practice);

SetupCompletedFn g_origSetupCompleted = nullptr;
TogglePracticeFn g_togglePractice = nullptr;
bool g_enabled = false;

// PlayLayer::setupHasCompleted: el nivel ya está cargado y la partida arrancada
void setupCompletedHook(void* playLayer) {
    g_origSetupCompleted(playLayer);
    if (g_enabled) g_togglePractice(playLayer, true);
}

} // namespace

void AutoPractice::init() {
    HMODULE exe = GetModuleHandleW(nullptr);
    uint8_t* setupCompleted = core::memory::findPattern(exe, core::signatures::kSetupHasCompleted);
    uint8_t* togglePractice = core::memory::findPattern(exe, core::signatures::kTogglePracticeMode);
    if (!setupCompleted || !togglePractice) {
        core::logMsg("Auto practice: no disponible (firma no encontrada: %s%s)",
                     setupCompleted ? "" : "kSetupHasCompleted ", togglePractice ? "" : "kTogglePracticeMode");
        return;
    }
    g_togglePractice = reinterpret_cast<TogglePracticeFn>(togglePractice);
    m_available = MH_CreateHook(setupCompleted, &setupCompletedHook, reinterpret_cast<void**>(&g_origSetupCompleted)) == MH_OK &&
                  MH_EnableHook(setupCompleted) == MH_OK;
    core::logMsg("Auto practice: %s", m_available ? "listo" : "no disponible (hook)");
}

void AutoPractice::enable() {
    m_enabled = true;
    g_enabled = m_available;
}

void AutoPractice::disable() {
    m_enabled = false;
    g_enabled = false;
}

bool AutoPractice::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Auto practice mode", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void AutoPractice::load(const nlohmann::json& j) {
    j.value("enabled", false) ? enable() : disable();
}

void AutoPractice::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}};
}

} // namespace features
