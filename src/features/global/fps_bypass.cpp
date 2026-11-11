#include "fps_bypass.hpp"

#include <windows.h>

#include <algorithm>
#include <atomic>

#include <MinHook.h>
#include <imgui.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

constexpr float kMinFps = 15.f;
constexpr float kMaxFps = 10000.f;

using SharedFn = void* (*)();
using SetIntervalFn = void (*)(void* app, double interval);
using GetIntervalFn = double (*)(void* director);
using GetVsyncFn = bool (*)(const void* app);

SharedFn g_sharedApplication = nullptr;
SharedFn g_sharedDirector = nullptr;
SetIntervalFn g_origSetInterval = nullptr;
GetIntervalFn g_getDirectorInterval = nullptr;
GetVsyncFn g_getVsync = nullptr;

// Intervalo forzado en segundos; 0 = no forzar (se respeta el del juego)
std::atomic<double> g_forcedInterval{0.0};

// El juego llama aquí al arrancar y al cambiar sus opciones de FPS
void setIntervalHook(void* app, double interval) {
    const double forced = g_forcedInterval;
    g_origSetInterval(app, forced > 0.0 ? forced : interval);
}

// Aplica ya el intervalo forzado o, si no hay, devuelve el que pidió el juego
void applyNow() {
    void* app = g_sharedApplication();
    if (!app) return; // todavía no existe: el hook lo aplicará cuando el juego lo fije
    double interval = g_forcedInterval;
    if (interval <= 0.0) {
        // CCDirector guarda el último intervalo que pidió el juego
        void* director = g_sharedDirector();
        interval = director ? g_getDirectorInterval(director) : 0.0;
        if (interval <= 0.0) interval = 1.0 / 60.0;
    }
    g_origSetInterval(app, interval);
}

} // namespace

void FpsBypass::init() {
    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    auto get = [cocos](const char* name) {
        return cocos ? reinterpret_cast<void*>(GetProcAddress(cocos, name)) : nullptr;
    };
    g_sharedApplication = reinterpret_cast<SharedFn>(get("?sharedApplication@CCApplication@cocos2d@@SAPEAV12@XZ"));
    g_sharedDirector = reinterpret_cast<SharedFn>(get("?sharedDirector@CCDirector@cocos2d@@SAPEAV12@XZ"));
    g_getDirectorInterval = reinterpret_cast<GetIntervalFn>(get("?getAnimationInterval@CCDirector@cocos2d@@QEAANXZ"));
    g_getVsync = reinterpret_cast<GetVsyncFn>(get("?getVerticalSyncEnabled@CCApplication@cocos2d@@QEBA_NXZ")); // opcional
    void* setInterval = get("?setAnimationInterval@CCApplication@cocos2d@@UEAAXN@Z");

    m_available = g_sharedApplication && g_sharedDirector && g_getDirectorInterval && setInterval &&
                  MH_CreateHook(setInterval, &setIntervalHook, reinterpret_cast<void**>(&g_origSetInterval)) == MH_OK &&
                  MH_EnableHook(setInterval) == MH_OK;
    core::logMsg("FPS bypass: %s", m_available ? "listo" : "no disponible (exports de libcocos2d)");
}

void FpsBypass::enable() {
    m_enabled = true;
    if (!m_available) return;
    g_forcedInterval = 1.0 / m_fps;
    applyNow();
}

void FpsBypass::disable() {
    m_enabled = false;
    if (!m_available) return;
    g_forcedInterval = 0.0;
    applyNow();
}

bool FpsBypass::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggleValue("FPS bypass", m_available, enabled, m_fps, kMinFps, kMaxFps, "%.0f");
    if (changed) enabled ? enable() : disable();

    if (m_available && m_enabled && g_getVsync) {
        const void* app = g_sharedApplication();
        if (app && g_getVsync(app))
            ImGui::TextDisabled("  VSync activo: apágalo en el juego");
    }
    return changed;
}

void FpsBypass::load(const nlohmann::json& j) {
    m_fps = std::clamp(j.value("fps", m_fps), kMinFps, kMaxFps);
    j.value("enabled", false) ? enable() : disable();
}

void FpsBypass::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"fps", m_fps}};
}

} // namespace features
