#include "game.hpp"

#include <windows.h>

#include <cstdint>
#include <cstring>

#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"

namespace core::game {

namespace {

void** g_gameManager = nullptr; // variable global del exe con el puntero a GameManager
int32_t g_playLayerOffset = 0;  // GameManager::m_playLayer

} // namespace

bool init() {
    uint8_t* code = memory::findPattern(GetModuleHandleW(nullptr), signatures::kSwitchStartPos);
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

} // namespace core::game
