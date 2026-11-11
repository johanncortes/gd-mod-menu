#include "noclip.hpp"

#include <windows.h>

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
int32_t g_spikeOffset = 0; // PlayLayer::m_anticheatSpike, leído de la firma
int32_t g_idOffset = 0;    // GameObject::m_uniqueID, leído de la firma

// Estadísticas del intento actual
void* g_playLayer = nullptr; // nivel al que pertenecen
int g_deaths = 0;
int g_frames = 0;
int g_hitFrames = 0;
bool g_hitThisFrame = false;
bool g_hitLastFrame = false;

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
        return;
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
    m_statsAvailable = core::game::levelHooksAvailable();
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
    if (m_available) {
        changed |= gui::toggle("Contador de noclip en pantalla", m_statsAvailable, m_showStats);
        if (m_statsAvailable && m_enabled && g_frames > 0)
            ImGui::TextDisabled("  %d muertes, %.2f%%", g_deaths, 100.0 * (g_frames - g_hitFrames) / g_frames);
    }
    return changed;
}

void Noclip::drawOverlay() {
    // Solo mientras el nivel corre (ni en pausa ni fuera del nivel)
    if (!active() || !m_statsAvailable || !m_showStats || g_frames == 0) return;
    if (!core::game::levelRunning()) return;

    char text[64];
    snprintf(text, sizeof(text), "Noclip: %d muertes  %.2f%%", g_deaths,
             100.0 * (g_frames - g_hitFrames) / g_frames);
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    const ImVec2 pos(8.f, ImGui::GetIO().DisplaySize.y - ImGui::GetFontSize() - 8.f);
    draw->AddText({pos.x + 1.f, pos.y + 1.f}, IM_COL32(0, 0, 0, 200), text); // sombra
    draw->AddText(pos, g_hitLastFrame ? IM_COL32(255, 90, 90, 255) : IM_COL32(255, 255, 255, 255), text);
}

void Noclip::load(const nlohmann::json& j) {
    m_showStats = j.value("show_stats", m_showStats);
    j.value("enabled", false) ? enable() : disable();
}

void Noclip::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"show_stats", m_showStats}};
}

} // namespace features
