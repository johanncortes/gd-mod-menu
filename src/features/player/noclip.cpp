#include "noclip.hpp"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <MinHook.h>
#include <imgui.h>
#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

// Todo esto se toca solo desde el hilo principal del juego (lógica y render van en el mismo)
using DestroyPlayerFn = void (*)(void* playLayer, void* player, void* object);

DestroyPlayerFn g_origDestroyPlayer = nullptr;

bool g_enabled = false;
bool g_statsAvailable = false; // hooks de nivel de core::game
int32_t g_spikeOffset = 0; // PlayLayer::m_anticheatSpike, leído de la firma
int32_t g_idOffset = 0;    // GameObject::m_uniqueID, leído de la firma

// Estadísticas del intento actual
void* g_playLayer = nullptr; // nivel al que pertenecen
int g_deaths = 0;
int g_frames = 0;
int g_hitFrames = 0;
bool g_hitThisFrame = false;
bool g_hitLastFrame = false;

// Límites: pasado uno, el golpe siguiente mata de verdad (0 = sin límite)
constexpr float kMinDeathLimit = 1.f, kMaxDeathLimit = 1000.f;
constexpr int kAccuracyGraceFrames = 60; // al empezar el intento la precisión aún no dice nada
int g_deathLimit = 0;
float g_minAccuracy = 0.f; // en %

bool limitReached() {
    if (g_deathLimit > 0 && g_deaths >= g_deathLimit) return true;
    return g_minAccuracy > 0.f && g_frames > kAccuracyGraceFrames &&
           100.0 * (g_frames - g_hitFrames) / g_frames < g_minAccuracy;
}

void resetStats() {
    g_deaths = g_frames = g_hitFrames = 0;
    g_hitThisFrame = g_hitLastFrame = false;
}

template <typename T>
T readAt(const void* base, int32_t offset) {
    T value;
    memcpy(&value, static_cast<const uint8_t*>(base) + offset, sizeof(T));
    return value;
}

// El juego pone un pincho invisible y espera que "mate" al jugador: destroyPlayer lo detecta
// comparando IDs y solo apunta que el nivel no está trucado. Esa llamada no se bloquea.
bool isAnticheatSpike(const void* playLayer, const void* object) {
    if (!object) return false;
    const void* spike = readAt<const void*>(playLayer, g_spikeOffset);
    return spike && readAt<int32_t>(spike, g_idOffset) == readAt<int32_t>(object, g_idOffset);
}

void destroyPlayerHook(void* playLayer, void* player, void* object) {
    if (g_enabled && !isAnticheatSpike(playLayer, object)) {
        if (playLayer != g_playLayer) {
            g_playLayer = playLayer;
            resetStats();
        }
        g_hitThisFrame = true;
        if (!limitReached()) return;
    }
    g_origDestroyPlayer(playLayer, player, object);
}

// Empieza un intento (aviso de core::game)
void onResetLevel(void* playLayer) {
    g_playLayer = playLayer;
    resetStats();
}

// Un frame de nivel (aviso de core::game)
void onUpdate(void* gameLayer, float) {
    if (!g_enabled || gameLayer != g_playLayer) return;
    ++g_frames;
    if (g_hitThisFrame) {
        ++g_hitFrames;
        if (!g_hitLastFrame) ++g_deaths; // empieza un contacto nuevo
    }
    g_hitLastFrame = g_hitThisFrame;
    g_hitThisFrame = false;
}

bool hookAt(void* target, void* hook, void** original) {
    return target && MH_CreateHook(target, hook, original) == MH_OK && MH_EnableHook(target) == MH_OK;
}

} // namespace

void Noclip::init(uint8_t* destroyPlayer) {
    if (!destroyPlayer) {
        core::logMsg("Noclip: no disponible (firma kDestroyPlayer no encontrada)");
        return;
    }
    g_spikeOffset = readAt<int32_t>(destroyPlayer, core::signatures::kDestroyPlayerSpikeOffset);
    g_idOffset = readAt<int32_t>(destroyPlayer, core::signatures::kDestroyPlayerIdOffset);
    if (g_spikeOffset <= 0 || g_spikeOffset > 0x10000 || g_idOffset <= 0 || g_idOffset > 0x10000) {
        core::logMsg("Noclip: no disponible (offsets inesperados: 0x%X y 0x%X)", g_spikeOffset, g_idOffset);
        return;
    }
    m_available = hookAt(destroyPlayer, &destroyPlayerHook, reinterpret_cast<void**>(&g_origDestroyPlayer));
    core::logMsg("Noclip: %s (destroyPlayer en %p)", m_available ? "listo" : "no disponible (hook)",
                 static_cast<void*>(destroyPlayer));
    if (!m_available) return;

    // Contador y precisión: opcionales, el noclip funciona sin ellos
    m_statsAvailable = g_statsAvailable = core::game::levelHooksAvailable();
    if (m_statsAvailable) {
        core::game::onResetLevel(&onResetLevel);
        core::game::onUpdate(&onUpdate);
    } else {
        core::logMsg("Noclip: contador no disponible (hooks de nivel)");
    }
}

void Noclip::enable() {
    m_enabled = true;
    if (!m_available) return;
    resetStats();
    g_enabled = true;
}

void Noclip::disable() {
    m_enabled = false;
    g_enabled = false;
}

bool Noclip::drawUI() {
    bool enabled = m_enabled;
    bool changed = gui::toggle("Noclip", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    // El contador en pantalla está en la ventana Labels (Noclip deaths / Noclip accuracy)
    if (m_available && m_statsAvailable && m_enabled && g_frames > 0)
        ImGui::TextDisabled("  %d muertes, %.2f%%", g_deaths, 100.0 * (g_frames - g_hitFrames) / g_frames);
    // Límites: necesitan el contador
    const bool limits = m_available && m_statsAvailable;
    bool limitChanged = gui::toggleValue("Noclip: límite de muertes", limits, m_deathLimitOn, m_deathLimit,
                                         kMinDeathLimit, kMaxDeathLimit, "%.0f");
    limitChanged |= gui::toggleValue("Noclip: precisión mínima", limits, m_accuracyOn, m_minAccuracy, 1.f, 100.f,
                                     "%.1f%%");
    if (limitChanged) applyLimits();
    return changed || limitChanged;
}

void Noclip::applyLimits() {
    g_deathLimit = m_deathLimitOn ? static_cast<int>(m_deathLimit) : 0;
    g_minAccuracy = m_accuracyOn ? m_minAccuracy : 0.f;
}

void Noclip::load(const nlohmann::json& j) {
    m_deathLimitOn = j.value("death_limit_on", false);
    m_deathLimit = std::clamp(j.value("death_limit", m_deathLimit), kMinDeathLimit, kMaxDeathLimit);
    m_accuracyOn = j.value("min_accuracy_on", false);
    m_minAccuracy = std::clamp(j.value("min_accuracy", m_minAccuracy), 1.f, 100.f);
    applyLimits();
    j.value("enabled", false) ? enable() : disable();
}

void Noclip::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled},
         {"death_limit_on", m_deathLimitOn},
         {"death_limit", m_deathLimit},
         {"min_accuracy_on", m_accuracyOn},
         {"min_accuracy", m_minAccuracy}};
}

bool noclipStats(NoclipStats& out) {
    if (!g_enabled || !g_statsAvailable) return false;
    out.deaths = g_deaths;
    out.accuracy = g_frames > 0 ? static_cast<float>(100.0 * (g_frames - g_hitFrames) / g_frames) : 100.f;
    out.hitting = g_hitLastFrame;
    return true;
}

} // namespace features
