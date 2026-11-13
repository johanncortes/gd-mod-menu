#include "no_new_best.hpp"

#include <windows.h>

#include <MinHook.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

using ShowNewBestFn = void (*)(void* playLayer, bool newReward, int orbs, int diamonds, bool demonKey,
                               bool noRetry, bool noTitle);
ShowNewBestFn g_origShowNewBest = nullptr;
bool g_enabled = false;

void showNewBestHook(void* playLayer, bool newReward, int orbs, int diamonds, bool demonKey, bool noRetry,
                     bool noTitle) {
    if (g_enabled && orbs <= 0 && diamonds <= 0) return; // solo era el cartel
    g_origShowNewBest(playLayer, newReward, orbs, diamonds, demonKey, noRetry, noTitle);
}

} // namespace

void NoNewBest::init() {
    void* target = core::memory::findPattern(GetModuleHandleW(nullptr), core::signatures::kShowNewBest);
    m_available = target &&
                  MH_CreateHook(target, &showNewBestHook, reinterpret_cast<void**>(&g_origShowNewBest)) == MH_OK &&
                  MH_EnableHook(target) == MH_OK;
    core::logMsg("No new best popup: %s", m_available ? "listo" : "no disponible");
}

void NoNewBest::enable() {
    m_enabled = true;
    g_enabled = m_available;
}

void NoNewBest::disable() {
    m_enabled = false;
    g_enabled = false;
}

bool NoNewBest::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("No new best popup", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void NoNewBest::load(const nlohmann::json& j) {
    j.value("no_new_best", false) ? enable() : disable();
}

void NoNewBest::save(nlohmann::json& j) const {
    j["no_new_best"] = m_enabled;
}

} // namespace features
