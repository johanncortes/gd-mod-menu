#include "respawn_delay.hpp"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <intrin.h>

#include <MinHook.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

constexpr float kMinDelay = 0.f;
constexpr float kMaxDelay = 10.f;

using DelayCreateFn = void* (*)(float seconds);
DelayCreateFn g_origDelayCreate = nullptr;

uintptr_t g_destroyPlayer = 0;        // inicio de PlayLayer::destroyPlayer
std::atomic<float> g_delay{-1.f};     // < 0 = no cambiar

// true si `address` está dentro de la función que empieza en g_destroyPlayer
// (según las tablas de funciones del propio exe)
bool insideDestroyPlayer(void* address) {
    DWORD64 imageBase = 0;
    const RUNTIME_FUNCTION* fn = RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(address), &imageBase, nullptr);
    return fn && imageBase + fn->BeginAddress == g_destroyPlayer;
}

// cocos2d::CCDelayTime::create(float): lo usa todo el juego, así que se filtra por quién llama
void* delayCreateHook(float seconds) {
    const float delay = g_delay;
    if (delay >= 0.f && insideDestroyPlayer(_ReturnAddress()))
        seconds = delay;
    return g_origDelayCreate(seconds);
}

} // namespace

void RespawnDelay::init(uint8_t* destroyPlayer) {
    if (!destroyPlayer) {
        core::logMsg("Respawn delay: no disponible (firma kDestroyPlayer no encontrada)");
        return;
    }
    g_destroyPlayer = reinterpret_cast<uintptr_t>(destroyPlayer);

    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    void* create = cocos ? reinterpret_cast<void*>(GetProcAddress(cocos, "?create@CCDelayTime@cocos2d@@SAPEAV12@M@Z")) : nullptr;
    m_available = create &&
                  MH_CreateHook(create, &delayCreateHook, reinterpret_cast<void**>(&g_origDelayCreate)) == MH_OK &&
                  MH_EnableHook(create) == MH_OK;
    core::logMsg("Respawn delay: %s", m_available ? "listo" : "no disponible (CCDelayTime::create)");
}

void RespawnDelay::enable() {
    m_enabled = true;
    if (m_available) g_delay = m_delay;
}

void RespawnDelay::disable() {
    m_enabled = false;
    g_delay = -1.f;
}

bool RespawnDelay::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggleValue("Respawn delay (s)", m_available, enabled, m_delay, kMinDelay, kMaxDelay, "%.2f");
    if (changed) enabled ? enable() : disable();
    return changed;
}

void RespawnDelay::load(const nlohmann::json& j) {
    m_delay = std::clamp(j.value("seconds", m_delay), kMinDelay, kMaxDelay);
    j.value("enabled", false) ? enable() : disable();
}

void RespawnDelay::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"seconds", m_delay}};
}

} // namespace features
