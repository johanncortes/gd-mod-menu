#include "instant_complete.hpp"

#include <windows.h>

#include <cstring>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

struct CCPoint { float x; float y; };

using LevelCompleteFn = void (*)(void* playLayer);
using PlayEndAnimationFn = void (*)(void* playLayer, CCPoint pos);
using SetPositionFn = void (*)(void* node, const CCPoint& pos);

LevelCompleteFn g_levelComplete = nullptr;
PlayEndAnimationFn g_playEndAnimation = nullptr;
SetPositionFn g_setPosition = nullptr;

constexpr int32_t kPlayer1Offset = 0xDB8;
constexpr int32_t kPlayer2Offset = 0xDC0;
constexpr int32_t kEndPositionOffset = 0x3318;

} // namespace

void InstantComplete::init(bool gameAvailable) {
    HMODULE exe = GetModuleHandleW(nullptr);
    g_levelComplete = reinterpret_cast<LevelCompleteFn>(
        core::memory::findPattern(exe, core::signatures::kLevelComplete));
    g_playEndAnimation = reinterpret_cast<PlayEndAnimationFn>(
        core::memory::findPattern(exe, core::signatures::kPlayEndAnimation));

    if (HMODULE cocos = GetModuleHandleA("libcocos2d.dll")) {
        g_setPosition = reinterpret_cast<SetPositionFn>(
            GetProcAddress(cocos, "?setPosition@CCNode@cocos2d@@UEAAXAEBVCCPoint@2@@Z"));
    }

    m_available = gameAvailable && (g_playEndAnimation || g_levelComplete);
    core::logMsg("Instant complete: %s (animación: %s, teletransporte: %s)",
                 m_available ? "listo" : "no disponible",
                 g_playEndAnimation ? "sí" : "no",
                 g_setPosition ? "sí" : "no");
}

void InstantComplete::drawUI() {
    // Fuera de un nivel el botón (y su tecla) no hace nada
    void* playLayer = core::game::playLayer();
    if (!gui::button("Instant complete", m_available) || !playLayer) return;

    // Obtener la posición X del final del nivel
    float endX = 0.f;
    memcpy(&endX, static_cast<const uint8_t*>(playLayer) + kEndPositionOffset, sizeof(endX));
    if (endX <= 0.f) endX = 10000.f;

    const CCPoint endPos{endX, 105.f};

    // Teletransportar al jugador al final del nivel
    if (g_setPosition) {
        void* player1 = nullptr;
        memcpy(&player1, static_cast<const uint8_t*>(playLayer) + kPlayer1Offset, sizeof(player1));
        if (player1) g_setPosition(player1, endPos);

        void* player2 = nullptr;
        memcpy(&player2, static_cast<const uint8_t*>(playLayer) + kPlayer2Offset, sizeof(player2));
        if (player2) g_setPosition(player2, endPos);
    }

    // Iniciar la animación y el flujo de completado
    if (g_playEndAnimation) {
        g_playEndAnimation(playLayer, endPos);
    } else if (g_levelComplete) {
        g_levelComplete(playLayer);
    }
}

} // namespace features
