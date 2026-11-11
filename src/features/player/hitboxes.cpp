#include "hitboxes.hpp"

#include <windows.h>

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

bool g_enabled = false;
bool g_refreshPending = false; // hay que volver a mostrar/ocultar el nodo de hitboxes
bool g_refreshing = false;

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
    return g_enabled || g_origShowHitboxes(playLayer);
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
    m_available = MH_CreateHook(showHitboxes, &showHitboxesHook, reinterpret_cast<void**>(&g_origShowHitboxes)) == MH_OK &&
                  MH_EnableHook(showHitboxes) == MH_OK;
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

bool Hitboxes::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Mostrar hitboxes", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void Hitboxes::load(const nlohmann::json& j) {
    j.value("enabled", false) ? enable() : disable();
}

void Hitboxes::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}};
}

} // namespace features
