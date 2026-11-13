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
int32_t g_editorLayerOffset = 0; // GameManager::m_levelEditorLayer (leído de kEditorLayerStore)
int32_t g_player1Offset = 0;    // GJBaseGameLayer::m_player1 (leído de kCheckForEnd)
int32_t g_player2Offset = 0;    // GJBaseGameLayer::m_player2 (leído de kLevelSettingsPlayers)
int32_t g_levelOffset = 0;      // PlayLayer::m_level (leído de kLevelAttempts)
int32_t g_objectIdOffset = 0;   // GameObject::m_objectID (leído de kAddObjectSection)
int32_t g_objectTypeOffset = 0; // GameObject::m_objectType (leído de kObjectTypeRead)

using ResetLevelFn = void (*)(void* playLayer);
using UpdateFn = void (*)(void* gameLayer, float dt);
using InitFn = bool (*)(void* playLayer, void* level, bool dontCreateObjects);
using AddObjectFn = void (*)(void* playLayer, void* object);
ResetLevelFn g_origResetLevel = nullptr;
UpdateFn g_origUpdate = nullptr;
InitFn g_origInit = nullptr;
AddObjectFn g_origAddObject = nullptr;
bool g_levelHooks = false;
bool g_loadHooks = false;
void* g_loadingLayer = nullptr; // PlayLayer que está cargando sus objetos (hasta su primer intento)

std::vector<ResetCallback> g_resetCallbacks;
std::vector<UpdateCallback> g_updateCallbacks;
std::vector<UpdateFilter> g_updateFilters;
std::vector<LevelInitCallback> g_initCallbacks;
std::vector<AddObjectCallback> g_addObjectCallbacks;

double g_attemptTime = 0.0;
ULONGLONG g_lastUpdate = 0;

void resetLevelHook(void* layer) {
    g_attemptTime = 0.0;
    for (ResetCallback callback : g_resetCallbacks) callback(layer);
    if (layer == g_loadingLayer) g_loadingLayer = nullptr; // primer intento: la carga terminó
    g_origResetLevel(layer);
}

bool playLayerInitHook(void* layer, void* level, bool dontCreateObjects) {
    g_loadingLayer = layer;
    for (LevelInitCallback callback : g_initCallbacks) callback(layer);
    return g_origInit(layer, level, dontCreateObjects);
}

void addObjectHook(void* layer, void* object) {
    if (object && layer == g_loadingLayer)
        for (AddObjectCallback callback : g_addObjectCallbacks) callback(layer, object);
    g_origAddObject(layer, object);
}

// GJBaseGameLayer::update corre una vez por frame mientras el nivel no está en pausa
void updateHook(void* gameLayer, float dt) {
    for (UpdateFilter filter : g_updateFilters)
        if (!filter(gameLayer, dt)) return;
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
    if (const uint8_t* settings = memory::findPattern(exe, signatures::kLevelSettingsPlayers)) {
        int32_t offset;
        memcpy(&offset, settings + signatures::kLevelSettingsPlayer2, 4);
        if (offset > 0 && offset < 0x10000) g_player2Offset = offset;
    }
    uint8_t* playLayerInit = memory::findPattern(exe, signatures::kPlayLayerInit);
    uint8_t* addObject = memory::findPattern(exe, signatures::kPlayLayerAddObject);
    const auto readOffset = [exe](const char* signature, int at) -> int32_t {
        const uint8_t* found = memory::findPattern(exe, signature);
        int32_t offset = 0;
        if (found) memcpy(&offset, found + at, 4);
        return offset > 0 && offset < 0x10000 ? offset : 0;
    };
    g_levelOffset = readOffset(signatures::kLevelAttempts, signatures::kLevelAttemptsLevel);
    g_objectIdOffset = readOffset(signatures::kAddObjectSection, signatures::kAddObjectId);
    g_objectTypeOffset = readOffset(signatures::kObjectTypeRead, signatures::kObjectTypeOffset);
    if (const uint8_t* editorStore = memory::findPattern(exe, signatures::kEditorLayerStore)) {
        int32_t offset;
        memcpy(&offset, editorStore + signatures::kEditorLayerStoreOffset, 4);
        if (offset > 0 && offset < 0x10000) g_editorLayerOffset = offset;
    }

    g_levelHooks = hookAt(resetLevel, &resetLevelHook, reinterpret_cast<void**>(&g_origResetLevel)) &&
                   hookAt(update, &updateHook, reinterpret_cast<void**>(&g_origUpdate));
    if (!g_levelHooks)
        logMsg("Hooks de nivel: no disponibles (firma: %s%s)", resetLevel ? "" : "kResetLevel ",
               update ? "" : "kGameLayerUpdate");

    g_loadHooks = g_levelHooks && g_objectIdOffset && g_objectTypeOffset &&
                  hookAt(playLayerInit, &playLayerInitHook, reinterpret_cast<void**>(&g_origInit)) &&
                  hookAt(addObject, &addObjectHook, reinterpret_cast<void**>(&g_origAddObject));
    if (!g_loadHooks) logMsg("Hooks de carga del nivel: no disponibles");

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

void* editorLayer() {
    if (!g_gameManager || !*g_gameManager || g_editorLayerOffset == 0) return nullptr;
    void* layer;
    memcpy(&layer, static_cast<uint8_t*>(*g_gameManager) + g_editorLayerOffset, sizeof(layer));
    return layer;
}

void* player1(void* gameLayer) {
    if (!gameLayer || g_player1Offset == 0) return nullptr;
    void* player;
    memcpy(&player, static_cast<uint8_t*>(gameLayer) + g_player1Offset, sizeof(player));
    return player;
}

void* player2(void* gameLayer) {
    if (!gameLayer || g_player2Offset == 0) return nullptr;
    void* player;
    memcpy(&player, static_cast<uint8_t*>(gameLayer) + g_player2Offset, sizeof(player));
    return player;
}

bool levelHooksAvailable() { return g_levelHooks; }
void onResetLevel(ResetCallback callback) { g_resetCallbacks.push_back(callback); }
void onUpdate(UpdateCallback callback) { g_updateCallbacks.push_back(callback); }
void addUpdateFilter(UpdateFilter filter) { g_updateFilters.push_back(filter); }
bool loadHooksAvailable() { return g_loadHooks; }
void onLevelInit(LevelInitCallback callback) { g_initCallbacks.push_back(callback); }
void onAddObject(AddObjectCallback callback) { g_addObjectCallbacks.push_back(callback); }

int objectId(void* object) {
    if (!object || g_objectIdOffset == 0) return -1;
    int32_t id;
    memcpy(&id, static_cast<uint8_t*>(object) + g_objectIdOffset, sizeof(id));
    return id;
}

int objectType(void* object) {
    if (!object || g_objectTypeOffset == 0) return -1;
    int32_t type;
    memcpy(&type, static_cast<uint8_t*>(object) + g_objectTypeOffset, sizeof(type));
    return type;
}

void* level(void* playLayer) {
    if (!playLayer || g_levelOffset == 0) return nullptr;
    void* level;
    memcpy(&level, static_cast<uint8_t*>(playLayer) + g_levelOffset, sizeof(level));
    return level;
}
double attemptTime() { return g_attemptTime; }
bool levelRunning() { return GetTickCount64() - g_lastUpdate < 250; }

} // namespace core::game
