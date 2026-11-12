#include "hitboxes.hpp"

#include <windows.h>

#include <cstring>

#include <MinHook.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

using ShowHitboxesFn = bool (*)(void* playLayer);
using UpdateVisibilityFn = void (*)(void* playLayer);

ShowHitboxesFn g_origShowHitboxes = nullptr;
UpdateVisibilityFn g_updateVisibility = nullptr;
int32_t g_deadOffset = 0; // PlayLayer: "el jugador ha muerto" (leído de la firma)

bool g_enabled = false;
bool g_onDeath = false;
bool g_refreshPending = false; // hay que volver a mostrar/ocultar el nodo de hitboxes
bool g_refreshing = false;

bool playerDead(void* playLayer) {
    return g_deadOffset > 0 && *(reinterpret_cast<const bool*>(playLayer) + g_deadOffset);
}

// El juego pregunta aquí cada frame si toca dibujar las hitboxes
bool showHitboxesHook(void* playLayer) {
    // El nodo de hitboxes solo cambia de visible a oculto en momentos concretos (al morir,
    // al cambiar opciones...). Al activar o desactivar la opción hay que pedírselo al juego,
    // y este es el único sitio donde tenemos el PlayLayer a mano.
    if (g_refreshPending && !g_refreshing) {
        g_refreshPending = false;
        g_refreshing = true; // esa función vuelve a llamar aquí
        g_updateVisibility(playLayer);
        g_refreshing = false;
    }
    return g_enabled || (g_onDeath && playerDead(playLayer)) || g_origShowHitboxes(playLayer);
}

} // namespace

void Hitboxes::init() {
    HMODULE exe = GetModuleHandleW(nullptr);
    uint8_t* showHitboxes = core::memory::findPattern(exe, core::signatures::kShowHitboxes);
    uint8_t* updateVisibility = core::memory::findPattern(exe, core::signatures::kUpdateHitboxVisibility);
    if (!showHitboxes || !updateVisibility) {
        core::logMsg("Hitboxes: no disponible (firma no encontrada: %s%s)", showHitboxes ? "" : "kShowHitboxes ",
                     updateVisibility ? "" : "kUpdateHitboxVisibility");
        return;
    }
    g_updateVisibility = reinterpret_cast<UpdateVisibilityFn>(updateVisibility);
    std::memcpy(&g_deadOffset, showHitboxes + core::signatures::kShowHitboxesDead, sizeof(g_deadOffset));
    m_onDeathAvailable = g_deadOffset > 0 && g_deadOffset < 0x10000;
    m_available = MH_CreateHook(showHitboxes, &showHitboxesHook, reinterpret_cast<void**>(&g_origShowHitboxes)) == MH_OK &&
                  MH_EnableHook(showHitboxes) == MH_OK;
    if (!m_available) m_onDeathAvailable = false;
    core::logMsg("Hitboxes: %s (firmas en %p y %p)", m_available ? "listo" : "no disponible (hook)",
                 static_cast<void*>(showHitboxes), static_cast<void*>(updateVisibility));
}

void Hitboxes::enable() {
    m_enabled = true;
    if (!m_available) return;
    g_enabled = true;
    g_refreshPending = true;
}

void Hitboxes::disable() {
    m_enabled = false;
    if (!m_available) return;
    g_enabled = false;
    g_refreshPending = true;
}

void Hitboxes::setOnDeath(bool on) {
    m_onDeath = on;
    g_onDeath = on && m_onDeathAvailable;
    if (m_available) g_refreshPending = true;
}

bool Hitboxes::drawUI() {
    bool enabled = m_enabled;
    bool changed = gui::toggle("Mostrar hitboxes", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    bool onDeath = m_onDeath;
    if (gui::toggle("Hitboxes al morir", m_onDeathAvailable, onDeath)) {
        setOnDeath(onDeath);
        changed = true;
    }
    return changed;
}

void Hitboxes::load(const nlohmann::json& j) {
    j.value("enabled", false) ? enable() : disable();
    setOnDeath(j.value("on_death", false));
}

void Hitboxes::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"on_death", m_onDeath}};
}

} // namespace features
