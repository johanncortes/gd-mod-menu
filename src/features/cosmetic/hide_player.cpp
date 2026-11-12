#include "hide_player.hpp"

#include <windows.h>

#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

using SetVisibleFn = void (*)(void* node, bool visible);
SetVisibleFn g_setVisible = nullptr; // cocos2d::CCNode::setVisible
bool g_enabled = false;

void setPlayersVisible(void* gameLayer, bool visible) {
    for (void* player : {core::game::player1(gameLayer), core::game::player2(gameLayer)})
        if (player) g_setVisible(player, visible);
}

// Tras cada frame del nivel, antes de que se dibuje
void onUpdate(void* gameLayer, float) {
    if (g_enabled && gameLayer == core::game::playLayer()) setPlayersVisible(gameLayer, false);
}

} // namespace

void HidePlayer::init(bool gameAvailable) {
    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    g_setVisible = cocos ? reinterpret_cast<SetVisibleFn>(GetProcAddress(cocos, "?setVisible@CCNode@cocos2d@@UEAAX_N@Z"))
                         : nullptr;
    m_available = gameAvailable && core::game::levelHooksAvailable() && g_setVisible;
    if (m_available) core::game::onUpdate(&onUpdate);
    core::logMsg("Hide player: %s", m_available ? "listo" : "no disponible");
}

void HidePlayer::enable() {
    m_enabled = true;
    g_enabled = m_available;
}

void HidePlayer::disable() {
    const bool wasHiding = g_enabled;
    m_enabled = false;
    g_enabled = false;
    // Se vuelven a mostrar ya (si no, seguirían ocultos hasta que el juego los toque)
    if (wasHiding)
        if (void* playLayer = core::game::playLayer()) setPlayersVisible(playLayer, true);
}

bool HidePlayer::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Hide player", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void HidePlayer::load(const nlohmann::json& j) {
    j.value("hide_player", false) ? enable() : disable();
}

void HidePlayer::save(nlohmann::json& j) const {
    j["hide_player"] = m_enabled;
}

} // namespace features
