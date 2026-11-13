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
constexpr float kMinShift = 0.5f; // límites del parámetro "pitch" del DSP de FMOD
constexpr float kMaxShift = 2.f;
constexpr int kDspTypePitchShift = 13; // FMOD_DSP_TYPE_PITCHSHIFT en FMOD 2.x
constexpr int kDspPitchShiftPitch = 0; // FMOD_DSP_PITCHSHIFT_PITCH

// --- Tiempo del juego: cocos2d::CCScheduler::update(float dt) ---
using SchedulerUpdateFn = void (*)(void* self, float dt);
SchedulerUpdateFn g_origSchedulerUpdate = nullptr;
std::atomic<float> g_timeScale{1.f};
std::atomic<float> g_timeBoost{1.f};   // Fast chests
std::atomic<float> g_lockedDelta{0.f}; // Lock delta: segundos por frame; 0 = el tiempo real
bool g_timeHook = false;

void schedulerUpdateHook(void* self, float dt) {
    const float locked = g_lockedDelta;
    if (locked > 0.f) dt = locked;
    g_origSchedulerUpdate(self, dt * g_timeScale * g_timeBoost);
}

// --- Audio: tono del grupo maestro de FMOD ---
using FmodUpdateFn = int (*)(void* system);
using FmodGetMasterFn = int (*)(void* system, void** group);
using FmodSetPitchFn = int (*)(void* channelControl, float pitch);
using FmodSetPausedFn = int (*)(void* channelControl, bool paused);
FmodSetPausedFn g_fmodSetPaused = nullptr;
std::atomic<bool> g_audioPaused{false};
bool g_appliedPaused = false;
FmodUpdateFn g_origFmodUpdate = nullptr;
FmodGetMasterFn g_fmodGetMaster = nullptr;
FmodSetPitchFn g_fmodSetPitch = nullptr;
std::atomic<void*> g_fmodSystem{nullptr};
std::atomic<float> g_pitch{1.f};
std::atomic<float> g_appliedPitch{1.f};

// --- Pitch shift: DSP de FMOD en el grupo maestro ---
using FmodCreateDspFn = int (*)(void* system, int type, void** dsp);
using FmodAddDspFn = int (*)(void* channelControl, int index, void* dsp);
using FmodRemoveDspFn = int (*)(void* channelControl, void* dsp);
using FmodSetParamFn = int (*)(void* dsp, int index, float value);
using FmodGetVersionFn = int (*)(void* system, unsigned int* version);
FmodGetVersionFn g_fmodGetVersion = nullptr;
FmodCreateDspFn g_fmodCreateDsp = nullptr;
FmodAddDspFn g_fmodAddDsp = nullptr;
FmodRemoveDspFn g_fmodRemoveDsp = nullptr;
FmodSetParamFn g_fmodSetParam = nullptr;
std::atomic<float> g_shift{0.f}; // 0 = sin DSP
void* g_shiftDsp = nullptr;      // se crea la primera vez y se reutiliza
bool g_shiftAttached = false;
float g_shiftApplied = 0.f;

// Solo desde el hilo del juego (hook de FMOD::System::update o al desactivar)
void applyShift(void* system) {
    const float shift = g_shift;
    if (!system || (shift > 0.f) == g_shiftAttached && shift == g_shiftApplied) return;
    void* master = nullptr;
    if (g_fmodGetMaster(system, &master) != 0 || !master) return;
    if (shift > 0.f) {
        // El número del tipo de DSP es el de FMOD 2.x: con otra versión mayor no se crea
        unsigned int version = 0;
        if (!g_shiftDsp && (g_fmodGetVersion(system, &version) != 0 || (version >> 16) != 2)) {
            core::logMsg("Pitch shift: FMOD 0x%X no es 2.x, no se usa", version);
            g_shift = 0.f;
            return;
        }
        if (!g_shiftDsp && g_fmodCreateDsp(system, kDspTypePitchShift, &g_shiftDsp) != 0) {
            g_shiftDsp = nullptr;
            g_shift = 0.f; // no se pudo crear: no lo intentamos cada frame
            return;
        }
        if (!g_shiftAttached) g_shiftAttached = g_fmodAddDsp(master, 0, g_shiftDsp) == 0;
        if (g_shiftAttached && g_fmodSetParam(g_shiftDsp, kDspPitchShiftPitch, shift) == 0) g_shiftApplied = shift;
    } else if (g_shiftAttached) {
        g_fmodRemoveDsp(master, g_shiftDsp);
        g_shiftAttached = false;
        g_shiftApplied = 0.f;
    }
}

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
    applyShift(system);
    if (g_fmodSetPaused && g_audioPaused != g_appliedPaused) {
        void* master = nullptr;
        if (g_fmodGetMaster(system, &master) == 0 && master && g_fmodSetPaused(master, g_audioPaused) == 0)
            g_appliedPaused = g_audioPaused;
    }
    return g_origFmodUpdate(system);
}

bool hookExport(const wchar_t* module, const char* name, void* hook, void** original) {
    HMODULE mod = GetModuleHandleW(module);
    void* target = mod ? reinterpret_cast<void*>(GetProcAddress(mod, name)) : nullptr;
    return target && MH_CreateHook(target, hook, original) == MH_OK && MH_EnableHook(target) == MH_OK;
}

} // namespace

void setGameAudioPaused(bool paused) {
    g_audioPaused = paused;
}

bool timeHookAvailable() { return g_timeHook; }
void setTimeBoost(float factor) { g_timeBoost = factor; }
void setLockedDelta(float seconds) { g_lockedDelta = seconds; }

void Speedhack::init() {
    m_available = hookExport(L"libcocos2d.dll", "?update@CCScheduler@cocos2d@@UEAAXM@Z",
                             &schedulerUpdateHook, reinterpret_cast<void**>(&g_origSchedulerUpdate));
    g_timeHook = m_available;
    core::logMsg("Speedhack: %s", m_available ? "listo" : "no disponible (CCScheduler::update)");

    if (HMODULE fmod = GetModuleHandleW(L"fmod.dll")) {
        g_fmodGetMaster = reinterpret_cast<FmodGetMasterFn>(GetProcAddress(
            fmod, "?getMasterChannelGroup@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEAPEAVChannelGroup@2@@Z"));
        g_fmodSetPitch = reinterpret_cast<FmodSetPitchFn>(
            GetProcAddress(fmod, "?setPitch@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@M@Z"));
        g_fmodCreateDsp = reinterpret_cast<FmodCreateDspFn>(GetProcAddress(
            fmod, "?createDSPByType@System@FMOD@@QEAA?AW4FMOD_RESULT@@W4FMOD_DSP_TYPE@@PEAPEAVDSP@2@@Z"));
        g_fmodAddDsp = reinterpret_cast<FmodAddDspFn>(
            GetProcAddress(fmod, "?addDSP@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@HPEAVDSP@2@@Z"));
        g_fmodRemoveDsp = reinterpret_cast<FmodRemoveDspFn>(
            GetProcAddress(fmod, "?removeDSP@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@PEAVDSP@2@@Z"));
        g_fmodSetPaused = reinterpret_cast<FmodSetPausedFn>(
            GetProcAddress(fmod, "?setPaused@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@_N@Z"));
        g_fmodGetVersion = reinterpret_cast<FmodGetVersionFn>(
            GetProcAddress(fmod, "?getVersion@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEAI@Z"));
        g_fmodSetParam = reinterpret_cast<FmodSetParamFn>(
            GetProcAddress(fmod, "?setParameterFloat@DSP@FMOD@@QEAA?AW4FMOD_RESULT@@HM@Z"));
    }
    m_audioAvailable = m_available && g_fmodGetMaster && g_fmodSetPitch &&
                       hookExport(L"fmod.dll", "?update@System@FMOD@@QEAA?AW4FMOD_RESULT@@XZ",
                                  &fmodUpdateHook, reinterpret_cast<void**>(&g_origFmodUpdate));
    if (!m_audioAvailable) core::logMsg("Speedhack: audio no disponible (FMOD)");
    m_shiftAvailable = m_audioAvailable && g_fmodCreateDsp && g_fmodAddDsp && g_fmodRemoveDsp && g_fmodSetParam &&
                       g_fmodGetVersion;
    core::logMsg("Pitch shift: %s", m_shiftAvailable ? "listo" : "no disponible (exports de FMOD)");
}

// Pasa el estado actual a los hooks
void Speedhack::apply() {
    const bool on = active();
    g_timeScale = on ? m_speed : 1.f;
    g_pitch = on && m_audio && m_audioAvailable ? m_speed : 1.f;
    g_shift = m_shift && m_shiftAvailable ? m_shiftPitch : 0.f;
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

void Speedhack::shutdown() {
    disable();
    m_shift = false;
    apply();
    if (m_shiftAvailable) applyShift(g_fmodSystem); // quita el DSP antes de descargar la DLL
    // Por si el frame stepper dejó el audio en pausa
    void* master = nullptr;
    if (g_fmodSetPaused && g_appliedPaused && g_fmodSystem && g_fmodGetMaster(g_fmodSystem, &master) == 0 && master)
        g_fmodSetPaused(master, false);
}

bool Speedhack::drawUI() {
    bool enabled = m_enabled;
    bool changed = gui::toggleValue("Speedhack", m_available, enabled, m_speed, kMinSpeed, kMaxSpeed, "%.2f");
    if (m_available)
        changed |= gui::toggle("Speedhack en el audio", m_audioAvailable, m_audio);
    if (changed) enabled ? enable() : disable();
    if (gui::toggleValue("Pitch shift", m_shiftAvailable, m_shift, m_shiftPitch, kMinShift, kMaxShift, "x%.2f")) {
        apply();
        changed = true;
    }
    return changed;
}

void Speedhack::load(const nlohmann::json& j) {
    m_speed = std::clamp(j.value("speed", m_speed), kMinSpeed, kMaxSpeed);
    m_audio = j.value("audio", m_audio);
    m_shift = j.value("pitch_shift", false);
    m_shiftPitch = std::clamp(j.value("pitch_shift_value", m_shiftPitch), kMinShift, kMaxShift);
    j.value("enabled", false) ? enable() : disable();
}

void Speedhack::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"speed", m_speed}, {"audio", m_audio}, {"pitch_shift", m_shift},
         {"pitch_shift_value", m_shiftPitch}};
}

} // namespace features
