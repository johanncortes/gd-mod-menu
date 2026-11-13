#include "game_time.hpp"

#include <windows.h>

#include <algorithm>

#include <MinHook.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "features/global/speedhack.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

// --- Fast chests ---
constexpr float kBoost = 4.f;
constexpr ULONGLONG kMaxBoostMs = 8000; // por si showCloseButton no llega (se cerró la capa antes)

using LayerFn = void (*)(void* rewardLayer);
LayerFn g_origStep2 = nullptr;
LayerFn g_origShowClose = nullptr;
bool g_chestsEnabled = false;
ULONGLONG g_boostSince = 0; // 0 = sin acelerar

void endBoost() {
    if (!g_boostSince) return;
    g_boostSince = 0;
    setTimeBoost(1.f);
}

void step2Hook(void* rewardLayer) {
    if (g_chestsEnabled) {
        g_boostSince = GetTickCount64();
        setTimeBoost(kBoost);
    }
    g_origStep2(rewardLayer);
}

void showCloseHook(void* rewardLayer) {
    endBoost();
    g_origShowClose(rewardLayer);
}

bool hookAt(void* target, void* hook, void** original) {
    return target && MH_CreateHook(target, hook, original) == MH_OK && MH_EnableHook(target) == MH_OK;
}

// --- Lock delta ---
constexpr float kMinFps = 30.f, kMaxFps = 1000.f;

} // namespace

void FastChests::init() {
    HMODULE exe = GetModuleHandleW(nullptr);
    // Las dos firmas antes de instalar ningún hook
    void* step2 = core::memory::findPattern(exe, core::signatures::kRewardStep2);
    void* showClose = core::memory::findPattern(exe, core::signatures::kRewardShowClose);
    m_available = timeHookAvailable() && step2 && showClose &&
                  hookAt(showClose, &showCloseHook, reinterpret_cast<void**>(&g_origShowClose)) &&
                  hookAt(step2, &step2Hook, reinterpret_cast<void**>(&g_origStep2));
    core::logMsg("Fast chests: %s", m_available ? "listo" : "no disponible");
}

void FastChests::enable() {
    m_enabled = true;
    g_chestsEnabled = m_available;
}

void FastChests::disable() {
    m_enabled = false;
    g_chestsEnabled = false;
    endBoost();
}

void FastChests::update() {
    if (g_boostSince && GetTickCount64() - g_boostSince > kMaxBoostMs) endBoost();
}

bool FastChests::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Fast chests", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void FastChests::load(const nlohmann::json& j) {
    j.value("fast_chests", false) ? enable() : disable();
}

void FastChests::save(nlohmann::json& j) const {
    j["fast_chests"] = m_enabled;
}

void LockDelta::init() {
    m_available = timeHookAvailable();
}

void LockDelta::enable() {
    m_enabled = true;
    setLockedDelta(m_available ? 1.f / m_fps : 0.f);
}

void LockDelta::disable() {
    m_enabled = false;
    setLockedDelta(0.f);
}

bool LockDelta::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggleValue("Lock delta", m_available, enabled, m_fps, kMinFps, kMaxFps, "%.0f FPS");
    if (changed) enabled ? enable() : disable();
    return changed;
}

void LockDelta::load(const nlohmann::json& j) {
    m_fps = std::clamp(j.value("fps", m_fps), kMinFps, kMaxFps);
    j.value("enabled", false) ? enable() : disable();
}

void LockDelta::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"fps", m_fps}};
}

} // namespace features
