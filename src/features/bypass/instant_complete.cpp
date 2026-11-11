#include "instant_complete.hpp"

#include <windows.h>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

using LevelCompleteFn = void (*)(void* playLayer);
LevelCompleteFn g_levelComplete = nullptr;

} // namespace

void InstantComplete::init(bool gameAvailable) {
    g_levelComplete = reinterpret_cast<LevelCompleteFn>(
        core::memory::findPattern(GetModuleHandleW(nullptr), core::signatures::kLevelComplete));
    m_available = gameAvailable && g_levelComplete;
    core::logMsg("Instant complete: %s", m_available ? "listo" : "no disponible (firma no encontrada)");
}

void InstantComplete::drawUI() {
    // Fuera de un nivel el botón (y su tecla) no hace nada
    void* playLayer = core::game::playLayer();
    if (gui::button("Instant complete", m_available) && playLayer) g_levelComplete(playLayer);
}

} // namespace features
