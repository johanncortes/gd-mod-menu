#include "shortcuts.hpp"

#include <windows.h>

#include <cstdint>
#include <cstring>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

using PlayLayerFn = void (*)(void* playLayer);
using TogglePracticeFn = void (*)(void* playLayer, bool practice);
using UiHandlerFn = void (*)(void* uiLayer, void* sender);

int32_t g_resetLevelSlot = 0;   // posición de resetLevel() en el vtable de PlayLayer
int32_t g_practiceOffset = 0;   // PlayLayer::m_isPracticeMode
TogglePracticeFn g_togglePractice = nullptr;
// Los manejadores de los botones de checkpoint de la interfaz del nivel. No usan ninguno de
// sus dos argumentos (sacan el PlayLayer de GameManager) y solo actúan en modo práctica.
UiHandlerFn g_placeCheckpoint = nullptr;
UiHandlerFn g_removeCheckpoint = nullptr;

int32_t readInt(const uint8_t* at) {
    int32_t value;
    memcpy(&value, at, 4);
    return value;
}

bool plausibleOffset(int32_t value) {
    return value > 0 && value < 0x10000;
}

} // namespace

void Shortcuts::init(bool gameAvailable) {
    if (!gameAvailable) {
        core::logMsg("Shortcuts: no disponibles (GameManager)");
        return;
    }
    HMODULE exe = GetModuleHandleW(nullptr);
    if (const uint8_t* code = core::memory::findPattern(exe, sig::kSwitchStartPos)) {
        g_resetLevelSlot = readInt(code + sig::kSwitchResetLevelSlot);
        m_restart = plausibleOffset(g_resetLevelSlot);
    }
    if (uint8_t* toggle = core::memory::findPattern(exe, sig::kTogglePracticeMode)) {
        g_togglePractice = reinterpret_cast<TogglePracticeFn>(toggle);
        g_practiceOffset = readInt(toggle + sig::kTogglePracticeModeFlag);
        m_practice = plausibleOffset(g_practiceOffset);
    }
    g_placeCheckpoint = reinterpret_cast<UiHandlerFn>(core::memory::findPattern(exe, sig::kUiOnCheck));
    g_removeCheckpoint = reinterpret_cast<UiHandlerFn>(core::memory::findPattern(exe, sig::kUiOnDeleteCheck));
    m_checkpoints = g_placeCheckpoint && g_removeCheckpoint;
    core::logMsg("Shortcuts: reiniciar %s, práctica %s, checkpoints %s", m_restart ? "listo" : "no disponible",
                 m_practice ? "listo" : "no disponible", m_checkpoints ? "listos" : "no disponibles");
}

void Shortcuts::drawUI() {
    // Fuera de un nivel los botones (y sus teclas) no hacen nada
    void* playLayer = core::game::playLayer();
    auto* bytes = static_cast<uint8_t*>(playLayer);

    if (gui::button("Restart level", m_restart) && playLayer) {
        void** vtable = *static_cast<void***>(playLayer);
        PlayLayerFn resetLevel;
        memcpy(&resetLevel, reinterpret_cast<uint8_t*>(vtable) + g_resetLevelSlot, sizeof(resetLevel));
        resetLevel(playLayer);
    }
    if (gui::button("Toggle practice mode", m_practice) && playLayer)
        g_togglePractice(playLayer, bytes[g_practiceOffset] == 0);
    if (gui::button("Place checkpoint", m_checkpoints) && playLayer) g_placeCheckpoint(nullptr, nullptr);
    if (gui::button("Remove checkpoint", m_checkpoints) && playLayer) g_removeCheckpoint(nullptr, nullptr);
}

} // namespace features
