#include "level_tweaks.hpp"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

// --- Random seed ---
constexpr float kMinSeed = 1.f, kMaxSeed = 999999.f; // 0 no vale: para el juego es "sin semilla"
int32_t g_storedSeedOffset = 0; // PlayLayer: semilla guardada (uint64)
uint64_t g_seed = 0;            // 0 = opción apagada

void writeSeed(void* playLayer, uint64_t seed) {
    memcpy(static_cast<uint8_t*>(playLayer) + g_storedSeedOffset, &seed, sizeof(seed));
}

// Antes de cada intento
void seedOnReset(void* playLayer) {
    if (g_seed) writeSeed(playLayer, g_seed);
}

// --- Stop triggers on death ---
int32_t g_deadOffset = 0; // PlayLayer: jugador muerto (byte)
bool g_stopEnabled = false;

bool stopFilter(void* gameLayer, float&) {
    if (!g_stopEnabled || gameLayer != core::game::playLayer()) return true;
    return *(static_cast<const uint8_t*>(gameLayer) + g_deadOffset) == 0;
}

} // namespace

void RandomSeed::init(bool gameAvailable) {
    uint8_t* code = core::memory::findPattern(GetModuleHandleW(nullptr), sig::kRandomSeed);
    if (code) memcpy(&g_storedSeedOffset, code + sig::kRandomSeedStored, 4);
    m_available = gameAvailable && core::game::levelHooksAvailable() && code && g_storedSeedOffset > 0 &&
                  g_storedSeedOffset < 0x10000;
    if (m_available) {
        m_patch.set(code + sig::kRandomSeedJump, {0x90, 0x90});
        core::game::onResetLevel(&seedOnReset);
    }
    core::logMsg("Random seed: %s", m_available ? "listo" : "no disponible");
}

void RandomSeed::enable() {
    m_enabled = true;
    if (!m_available || !m_patch.apply()) return;
    g_seed = static_cast<uint64_t>(m_seed);
}

void RandomSeed::disable() {
    m_enabled = false;
    if (!m_available) return;
    g_seed = 0;
    m_patch.revert();
    // El nivel abierto se queda sin semilla guardada, como estaba
    if (void* playLayer = core::game::playLayer()) writeSeed(playLayer, 0);
}

bool RandomSeed::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggleValue("Random seed", m_available, enabled, m_seed, kMinSeed, kMaxSeed, "%.0f");
    if (changed) enabled ? enable() : disable();
    return changed;
}

void RandomSeed::load(const nlohmann::json& j) {
    m_seed = std::clamp(j.value("seed", m_seed), kMinSeed, kMaxSeed);
    j.value("enabled", false) ? enable() : disable();
}

void RandomSeed::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"seed", m_seed}};
}

void StopOnDeath::init(bool gameAvailable) {
    if (const uint8_t* hitboxes = core::memory::findPattern(GetModuleHandleW(nullptr), sig::kShowHitboxes))
        memcpy(&g_deadOffset, hitboxes + sig::kShowHitboxesDead, 4);
    m_available = gameAvailable && core::game::levelHooksAvailable() && g_deadOffset > 0 && g_deadOffset < 0x10000;
    if (m_available) core::game::addUpdateFilter(&stopFilter);
    core::logMsg("Stop triggers on death: %s", m_available ? "listo" : "no disponible");
}

void StopOnDeath::enable() {
    m_enabled = true;
    g_stopEnabled = m_available;
}

void StopOnDeath::disable() {
    m_enabled = false;
    g_stopEnabled = false;
}

bool StopOnDeath::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Stop triggers on death", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void StopOnDeath::load(const nlohmann::json& j) {
    j.value("stop_on_death", false) ? enable() : disable();
}

void StopOnDeath::save(nlohmann::json& j) const {
    j["stop_on_death"] = m_enabled;
}

} // namespace features
