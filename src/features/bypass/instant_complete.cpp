#include "instant_complete.hpp"

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

struct Point {
    float x, y; // mismo formato que cocos2d::CCPoint
};

using PlayLayerFn = void (*)(void* playLayer);
using SpawnPosFn = Point* (*)(void* portal, Point* out);        // EndPortalObject::getSpawnPos
using GetPositionFn = const Point& (*)(void* node);             // CCNode::getPosition (virtual)
using SetPositionFn = void (*)(void* node, const Point& point); // CCNode::setPosition (virtual)

PlayLayerFn g_levelComplete = nullptr;
PlayLayerFn g_checkForEnd = nullptr;
SpawnPosFn g_spawnPos = nullptr;
int32_t g_portalOffset = 0;     // PlayLayer::m_endPortal
int32_t g_startedOffset = 0;    // PlayLayer::m_levelEndAnimationStarted
int32_t g_playerOffset = 0;     // GJBaseGameLayer::m_player1
int32_t g_getPositionSlot = 0;  // en el vtable de CCNode
int32_t g_setPositionSlot = -1; // en el vtable de CCNode

int32_t readInt(const uint8_t* at) {
    int32_t value;
    memcpy(&value, at, 4);
    return value;
}

template <typename T>
T fieldAt(void* object, int32_t offset) {
    T value;
    memcpy(&value, static_cast<const uint8_t*>(object) + offset, sizeof(T));
    return value;
}

template <typename Fn>
Fn virtualAt(void* object, int32_t slot) {
    return fieldAt<Fn>(*static_cast<void**>(object), slot);
}

bool plausibleOffset(int32_t value) {
    return value > 0 && value < 0x10000 && value % 8 == 0;
}

// Posición de CCNode::setPosition(const CCPoint&) en el vtable de CCNode: se busca el export
// dentro del vtable exportado de la clase. Llamándola por el vtable, PlayerObject usa su propia
// versión (la que también actualiza su posición de física).
int32_t findSetPositionSlot() {
    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    if (!cocos) return -1;
    const auto* vtable = reinterpret_cast<void* const*>(GetProcAddress(cocos, "??_7CCNode@cocos2d@@6B@"));
    const void* setPosition = reinterpret_cast<const void*>(
        GetProcAddress(cocos, "?setPosition@CCNode@cocos2d@@UEAAXAEBVCCPoint@2@@Z"));
    if (!vtable || !setPosition) return -1;
    for (int32_t i = 0; i < 128; ++i)
        if (vtable[i] == setPosition) return i * 8;
    return -1;
}

// Lleva al jugador 1 hasta el portal final y deja que el juego termine el nivel como siempre.
// false si no se pudo (nivel de plataformas sin portal, o faltan firmas).
bool finishAtPortal(void* playLayer) {
    if (!g_checkForEnd) return false;
    void* portal = fieldAt<void*>(playLayer, g_portalOffset);
    void* player = fieldAt<void*>(playLayer, g_playerOffset);
    if (!portal || !player) return false;
    if (fieldAt<bool>(playLayer, g_startedOffset)) return true; // ya está terminando

    Point spawn{};
    g_spawnPos(portal, &spawn);
    // Misma altura y la x del portal: justo la condición que comprueba checkForEnd
    const Point current = virtualAt<GetPositionFn>(player, g_getPositionSlot)(player);
    virtualAt<SetPositionFn>(player, g_setPositionSlot)(player, Point{spawn.x, current.y});
    g_checkForEnd(playLayer);
    return fieldAt<bool>(playLayer, g_startedOffset);
}

} // namespace

void InstantComplete::init(bool gameAvailable) {
    HMODULE exe = GetModuleHandleW(nullptr);
    g_levelComplete = reinterpret_cast<PlayLayerFn>(core::memory::findPattern(exe, sig::kLevelComplete));
    m_available = gameAvailable && g_levelComplete;
    if (!m_available) {
        core::logMsg("Instant complete: no disponible (firma kLevelComplete no encontrada)");
        return;
    }

    // Llegar al portal: opcional; sin esto se completa directamente, como en la fase 4
    if (uint8_t* check = core::memory::findPattern(exe, sig::kCheckForEnd)) {
        g_portalOffset = readInt(check + sig::kCheckForEndPortal);
        g_startedOffset = readInt(check + sig::kCheckForEndStarted);
        g_playerOffset = readInt(check + sig::kCheckForEndPlayer);
        g_getPositionSlot = readInt(check + sig::kCheckForEndGetPosition);
        g_spawnPos = reinterpret_cast<SpawnPosFn>(check + sig::kCheckForEndSpawnPos + 4 +
                                                  readInt(check + sig::kCheckForEndSpawnPos));
        g_setPositionSlot = findSetPositionSlot();
        const bool ok = plausibleOffset(g_portalOffset) && g_startedOffset > 0 && g_startedOffset < 0x10000 &&
                        plausibleOffset(g_playerOffset) && plausibleOffset(g_getPositionSlot) && g_setPositionSlot >= 0;
        if (ok) g_checkForEnd = reinterpret_cast<PlayLayerFn>(check);
    }
    core::logMsg("Instant complete: listo (%s)", g_checkForEnd ? "lleva al jugador al portal final"
                                                                : "sin teletransporte, completa directamente");
}

void InstantComplete::drawUI() {
    // Fuera de un nivel el botón (y su tecla) no hace nada
    void* playLayer = core::game::playLayer();
    if (!gui::button("Instant complete", m_available) || !playLayer) return;
    if (!finishAtPortal(playLayer)) g_levelComplete(playLayer);
}

} // namespace features
