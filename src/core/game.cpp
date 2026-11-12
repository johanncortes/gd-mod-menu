#include "game.hpp"

#include <windows.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include <MinHook.h>

#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"

namespace core::game {

namespace {

void** g_gameManager = nullptr; // variable global del exe con el puntero a GameManager
int32_t g_playLayerOffset = 0;  // GameManager::m_playLayer
int32_t g_player1Offset = 0;    // GJBaseGameLayer::m_player1 (leído de kCheckForEnd)

using ResetLevelFn = void (*)(void* playLayer);
using UpdateFn = void (*)(void* gameLayer, float dt);
ResetLevelFn g_origResetLevel = nullptr;
UpdateFn g_origUpdate = nullptr;
bool g_levelHooks = false;

std::vector<ResetCallback> g_resetCallbacks;
std::vector<UpdateCallback> g_updateCallbacks;

double g_attemptTime = 0.0;
ULONGLONG g_lastUpdate = 0;

void resetLevelHook(void* layer) {
    g_attemptTime = 0.0;
    for (ResetCallback callback : g_resetCallbacks) callback(layer);
    g_origResetLevel(layer);
}

// GJBaseGameLayer::update corre una vez por frame mientras el nivel no está en pausa
void updateHook(void* gameLayer, float dt) {
    g_origUpdate(gameLayer, dt);
    if (gameLayer == playLayer()) {
        g_attemptTime += dt;
        g_lastUpdate = GetTickCount64();
    }
    for (UpdateCallback callback : g_updateCallbacks) callback(gameLayer, dt);
}

bool hookAt(void* target, void* hook, void** original) {
    return target && MH_CreateHook(target, hook, original) == MH_OK && MH_EnableHook(target) == MH_OK;
}

} // namespace

bool init() {
    HMODULE exe = GetModuleHandleW(nullptr);
    // Las firmas se buscan antes de instalar los hooks
    uint8_t* code = memory::findPattern(exe, signatures::kSwitchStartPos);
    uint8_t* resetLevel = memory::findPattern(exe, signatures::kResetLevel);
    uint8_t* update = memory::findPattern(exe, signatures::kGameLayerUpdate);
    if (const uint8_t* checkForEnd = memory::findPattern(exe, signatures::kCheckForEnd)) {
        int32_t offset;
        memcpy(&offset, checkForEnd + signatures::kCheckForEndPlayer, 4);
        if (offset > 0 && offset < 0x10000) g_player1Offset = offset;
    }

    g_levelHooks = hookAt(resetLevel, &resetLevelHook, reinterpret_cast<void**>(&g_origResetLevel)) &&
                   hookAt(update, &updateHook, reinterpret_cast<void**>(&g_origUpdate));
    if (!g_levelHooks)
        logMsg("Hooks de nivel: no disponibles (firma: %s%s)", resetLevel ? "" : "kResetLevel ",
               update ? "" : "kGameLayerUpdate");

    if (!code) {
        logMsg("GameManager: no disponible (firma kSwitchStartPos no encontrada)");
        return false;
    }
    int32_t disp;
    memcpy(&disp, code + signatures::kSwitchGameManager, 4);
    memcpy(&g_playLayerOffset, code + signatures::kSwitchPlayLayer, 4);
    if (g_playLayerOffset <= 0 || g_playLayerOffset > 0x10000) {
        logMsg("GameManager: no disponible (offset inesperado 0x%X)", g_playLayerOffset);
        return false;
    }
    g_gameManager = reinterpret_cast<void**>(code + signatures::kSwitchGameManager + 4 + disp);
    return true;
}

void* playLayer() {
    if (!g_gameManager || !*g_gameManager) return nullptr;
    void* layer;
    memcpy(&layer, static_cast<uint8_t*>(*g_gameManager) + g_playLayerOffset, sizeof(layer));
    return layer;
}

void* player1(void* gameLayer) {
    if (!gameLayer || g_player1Offset == 0) return nullptr;
    void* player;
    memcpy(&player, static_cast<uint8_t*>(gameLayer) + g_player1Offset, sizeof(player));
    return player;
}

bool levelHooksAvailable() { return g_levelHooks; }
void onResetLevel(ResetCallback callback) { g_resetCallbacks.push_back(callback); }
void onUpdate(UpdateCallback callback) { g_updateCallbacks.push_back(callback); }
double attemptTime() { return g_attemptTime; }
bool levelRunning() { return GetTickCount64() - g_lastUpdate < 250; }

} // namespace core::game
