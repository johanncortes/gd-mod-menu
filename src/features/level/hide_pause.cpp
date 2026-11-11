#include "hide_pause.hpp"

#include <windows.h>

#include <MinHook.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

using CustomSetupFn = void (*)(void* pauseLayer);
using SetVisibleFn = void (*)(void* node, bool visible);

CustomSetupFn g_origCustomSetup = nullptr;
SetVisibleFn g_setVisible = nullptr; // cocos2d::CCNode::setVisible
bool g_enabled = false;

// PauseLayer::customSetup: aquí el juego rellena el menú de pausa recién creado
void customSetupHook(void* pauseLayer) {
    g_origCustomSetup(pauseLayer);
    if (g_enabled) g_setVisible(pauseLayer, false);
}

} // namespace

void HidePause::init() {
    uint8_t* customSetup = core::memory::findPattern(GetModuleHandleW(nullptr), core::signatures::kPauseLayerSetup);
    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    g_setVisible = cocos ? reinterpret_cast<SetVisibleFn>(GetProcAddress(cocos, "?setVisible@CCNode@cocos2d@@UEAAX_N@Z")) : nullptr;
    if (!customSetup || !g_setVisible) {
        core::logMsg("Hide pause menu: no disponible (%s)", customSetup ? "CCNode::setVisible" : "firma kPauseLayerSetup no encontrada");
        return;
    }
    m_available = MH_CreateHook(customSetup, &customSetupHook, reinterpret_cast<void**>(&g_origCustomSetup)) == MH_OK &&
                  MH_EnableHook(customSetup) == MH_OK;
    core::logMsg("Hide pause menu: %s", m_available ? "listo" : "no disponible (hook)");
}

void HidePause::enable() {
    m_enabled = true;
    g_enabled = m_available;
}

void HidePause::disable() {
    m_enabled = false;
    g_enabled = false;
}

bool HidePause::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Hide pause menu", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void HidePause::load(const nlohmann::json& j) {
    j.value("enabled", false) ? enable() : disable();
}

void HidePause::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}};
}

} // namespace features
