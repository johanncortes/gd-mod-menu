#include "speedhack.hpp"

#include <windows.h>

#include <algorithm>
#include <atomic>

#include <MinHook.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

constexpr float kMinSpeed = 0.05f;
constexpr float kMaxSpeed = 20.f;

// --- Tiempo del juego: cocos2d::CCScheduler::update(float dt) ---
using SchedulerUpdateFn = void (*)(void* self, float dt);
SchedulerUpdateFn g_origSchedulerUpdate = nullptr;
std::atomic<float> g_timeScale{1.f};

void schedulerUpdateHook(void* self, float dt) {
    g_origSchedulerUpdate(self, dt * g_timeScale);
}

// --- Audio: tono del grupo maestro de FMOD ---
using FmodUpdateFn = int (*)(void* system);
using FmodGetMasterFn = int (*)(void* system, void** group);
using FmodSetPitchFn = int (*)(void* channelControl, float pitch);
FmodUpdateFn g_origFmodUpdate = nullptr;
FmodGetMasterFn g_fmodGetMaster = nullptr;
FmodSetPitchFn g_fmodSetPitch = nullptr;
std::atomic<void*> g_fmodSystem{nullptr};
std::atomic<float> g_pitch{1.f};
std::atomic<float> g_appliedPitch{1.f};

void applyPitch(void* system) {
    const float pitch = g_pitch;
    if (!system || pitch == g_appliedPitch) return;
    void* master = nullptr;
    if (g_fmodGetMaster(system, &master) == 0 && master && g_fmodSetPitch(master, pitch) == 0)
        g_appliedPitch = pitch;
}

// FMOD::System::update se llama cada frame: de aquí sale el puntero al sistema
int fmodUpdateHook(void* system) {
    g_fmodSystem = system;
    applyPitch(system);
    return g_origFmodUpdate(system);
}

bool hookExport(const wchar_t* module, const char* name, void* hook, void** original) {
    HMODULE mod = GetModuleHandleW(module);
    void* target = mod ? reinterpret_cast<void*>(GetProcAddress(mod, name)) : nullptr;
    return target && MH_CreateHook(target, hook, original) == MH_OK && MH_EnableHook(target) == MH_OK;
}

} // namespace

void Speedhack::init() {
    m_available = hookExport(L"libcocos2d.dll", "?update@CCScheduler@cocos2d@@UEAAXM@Z",
                             &schedulerUpdateHook, reinterpret_cast<void**>(&g_origSchedulerUpdate));
    core::logMsg("Speedhack: %s", m_available ? "listo" : "no disponible (CCScheduler::update)");

    if (HMODULE fmod = GetModuleHandleW(L"fmod.dll")) {
        g_fmodGetMaster = reinterpret_cast<FmodGetMasterFn>(GetProcAddress(
            fmod, "?getMasterChannelGroup@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEAPEAVChannelGroup@2@@Z"));
        g_fmodSetPitch = reinterpret_cast<FmodSetPitchFn>(
            GetProcAddress(fmod, "?setPitch@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@M@Z"));
    }
    m_audioAvailable = m_available && g_fmodGetMaster && g_fmodSetPitch &&
                       hookExport(L"fmod.dll", "?update@System@FMOD@@QEAA?AW4FMOD_RESULT@@XZ",
                                  &fmodUpdateHook, reinterpret_cast<void**>(&g_origFmodUpdate));
    if (!m_audioAvailable) core::logMsg("Speedhack: audio no disponible (FMOD)");
}

// Pasa el estado actual a los hooks
void Speedhack::apply() {
    const bool on = active();
    g_timeScale = on ? m_speed : 1.f;
    g_pitch = on && m_audio && m_audioAvailable ? m_speed : 1.f;
}

void Speedhack::enable() {
    m_enabled = true;
    apply();
}

void Speedhack::disable() {
    m_enabled = false;
    apply();
    // Por si el juego no vuelve a llamar a FMOD::System::update (p. ej. al descargar la DLL)
    if (m_audioAvailable) applyPitch(g_fmodSystem);
}

bool Speedhack::drawUI() {
    bool enabled = m_enabled;
    bool changed = gui::toggleValue("Speedhack", m_available, enabled, m_speed, kMinSpeed, kMaxSpeed, "%.2f");
    if (m_available)
        changed |= gui::toggle("Speedhack en el audio", m_audioAvailable, m_audio);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void Speedhack::load(const nlohmann::json& j) {
    m_speed = std::clamp(j.value("speed", m_speed), kMinSpeed, kMaxSpeed);
    m_audio = j.value("audio", m_audio);
    j.value("enabled", false) ? enable() : disable();
}

void Speedhack::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"speed", m_speed}, {"audio", m_audio}};
}

} // namespace features
