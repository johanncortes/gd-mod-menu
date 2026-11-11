#include "startpos_switcher.hpp"

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <imgui.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

constexpr int kPrevKey = 'Q';
constexpr int kNextKey = 'E';

using PlayLayerFn = void (*)(void* playLayer);
using PlayLayerFloatFn = void (*)(void* playLayer, float value);
using StartPosListFn = void* (*)(void* gameLayer);          // devuelve un CCArray*
using ArrayCountFn = unsigned (*)(const void* array);
using ArrayAtFn = void* (*)(void* array, unsigned index);
using RefFn = void (*)(void* object);

// Del exe (todo sale de la firma kSwitchStartPos)
void** g_gameManager = nullptr; // puntero global al GameManager
int32_t g_playLayerOffset = 0;  // GameManager::m_playLayer
int32_t g_startPosOffset = 0;   // PlayLayer::m_startPosObject
int32_t g_testModeOffset = 0;   // PlayLayer: byte "se juega desde un StartPos"
int32_t g_resetLevelSlot = 0;   // posición en el vtable
int32_t g_virtualSlot = 0;
PlayLayerFn g_resetFromStart = nullptr;
PlayLayerFloatFn g_updateCamera = nullptr;
PlayLayerFn g_updateTestLabel = nullptr;
PlayLayerFn g_startMusic = nullptr;
StartPosListFn g_startPosList = nullptr;

// De libcocos2d.dll
ArrayCountFn g_arrayCount = nullptr;
ArrayAtFn g_arrayAt = nullptr;
RefFn g_retain = nullptr;
RefFn g_release = nullptr;

// Para el aviso en pantalla
int g_shownIndex = 0;
int g_shownTotal = 0;
ULONGLONG g_shownAt = 0;

int32_t readInt(const uint8_t* at) {
    int32_t value;
    memcpy(&value, at, 4);
    return value;
}

// Destino de un call/jmp rel32 cuyo desplazamiento está en `at`
template <typename Fn>
Fn relTarget(uint8_t* at) {
    return reinterpret_cast<Fn>(at + 4 + readInt(at));
}

template <typename T>
T& fieldAt(void* object, int32_t offset) {
    return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
}

template <typename Fn>
Fn virtualAt(void* object, int32_t slot) {
    return fieldAt<Fn>(*static_cast<void**>(object), slot);
}

bool plausibleOffset(int32_t value) {
    return value > 0 && value < 0x10000;
}

void* currentPlayLayer() {
    void* gameManager = *g_gameManager;
    return gameManager ? fieldAt<void*>(gameManager, g_playLayerOffset) : nullptr;
}

// Lo mismo que PlayLayer::switchToStartPosObject (está en línea en PauseLayer::onResume).
// startPos == nullptr es el inicio del nivel.
void switchTo(void* playLayer, void* startPos) {
    void*& current = fieldAt<void*>(playLayer, g_startPosOffset);
    if (current == startPos) {
        virtualAt<PlayLayerFn>(playLayer, g_resetLevelSlot)(playLayer);
    } else {
        if (startPos) g_retain(startPos);
        if (current) g_release(current);
        current = startPos;
        fieldAt<bool>(playLayer, g_testModeOffset) = startPos != nullptr;
        g_resetFromStart(playLayer);
        virtualAt<PlayLayerFloatFn>(playLayer, g_virtualSlot)(playLayer, 0.f);
        g_updateCamera(playLayer, 0.f);
        g_updateTestLabel(playLayer);
    }
    g_startMusic(playLayer);
}

bool gameFocused() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

} // namespace

void StartPosSwitcher::init() {
    HMODULE exe = GetModuleHandleW(nullptr);
    uint8_t* code = core::memory::findPattern(exe, sig::kSwitchStartPos);
    uint8_t* list = core::memory::findPattern(exe, sig::kStartPosObjects);
    if (!code || !list) {
        core::logMsg("StartPos switcher: no disponible (firma no encontrada: %s%s)", code ? "" : "kSwitchStartPos ",
                     list ? "" : "kStartPosObjects");
        return;
    }
    g_gameManager = relTarget<void**>(code + sig::kSwitchGameManager);
    g_playLayerOffset = readInt(code + sig::kSwitchPlayLayer);
    g_startPosOffset = readInt(code + sig::kSwitchStartPosObject);
    g_testModeOffset = readInt(code + sig::kSwitchTestMode);
    g_resetLevelSlot = readInt(code + sig::kSwitchResetLevelSlot);
    g_virtualSlot = readInt(code + sig::kSwitchVirtualSlot);
    g_resetFromStart = relTarget<PlayLayerFn>(code + sig::kSwitchResetFromStart);
    g_updateCamera = relTarget<PlayLayerFloatFn>(code + sig::kSwitchUpdateCamera);
    g_updateTestLabel = relTarget<PlayLayerFn>(code + sig::kSwitchUpdateTestLabel);
    g_startMusic = relTarget<PlayLayerFn>(code + sig::kSwitchStartMusic);
    g_startPosList = reinterpret_cast<StartPosListFn>(list);
    if (!plausibleOffset(g_playLayerOffset) || !plausibleOffset(g_startPosOffset) || !plausibleOffset(g_testModeOffset) ||
        !plausibleOffset(g_resetLevelSlot) || !plausibleOffset(g_virtualSlot)) {
        core::logMsg("StartPos switcher: no disponible (offsets inesperados)");
        return;
    }

    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    if (cocos) {
        g_arrayCount = reinterpret_cast<ArrayCountFn>(GetProcAddress(cocos, "?count@CCArray@cocos2d@@QEBAIXZ"));
        g_arrayAt = reinterpret_cast<ArrayAtFn>(GetProcAddress(cocos, "?objectAtIndex@CCArray@cocos2d@@QEAAPEAVCCObject@2@I@Z"));
        g_retain = reinterpret_cast<RefFn>(GetProcAddress(cocos, "?retain@CCObject@cocos2d@@QEAAXXZ"));
        g_release = reinterpret_cast<RefFn>(GetProcAddress(cocos, "?release@CCObject@cocos2d@@QEAAXXZ"));
    }
    m_available = g_arrayCount && g_arrayAt && g_retain && g_release;
    core::logMsg("StartPos switcher: %s", m_available ? "listo" : "no disponible (exports de cocos2d)");
}

void StartPosSwitcher::enable() { m_enabled = true; }
void StartPosSwitcher::disable() { m_enabled = false; }

void StartPosSwitcher::step(int direction) {
    void* playLayer = currentPlayLayer();
    if (!playLayer) return;
    void* list = g_startPosList(playLayer);
    const int total = list ? static_cast<int>(g_arrayCount(list)) : 0;
    if (total == 0) return;

    // Posición actual: 0 = inicio del nivel, 1..total = StartPos
    void* current = fieldAt<void*>(playLayer, g_startPosOffset);
    int index = 0;
    for (int i = 0; i < total; ++i)
        if (g_arrayAt(list, static_cast<unsigned>(i)) == current) index = i + 1;

    index = (index + direction + total + 1) % (total + 1);
    switchTo(playLayer, index == 0 ? nullptr : g_arrayAt(list, static_cast<unsigned>(index - 1)));
    g_shownIndex = index;
    g_shownTotal = total;
    g_shownAt = GetTickCount64();
}

void StartPosSwitcher::update() {
    if (!m_available || !m_enabled) return;
    // Solo con el juego en primer plano y sin estar escribiendo en el panel
    const bool listen = gameFocused() && !ImGui::GetIO().WantTextInput;
    const bool prevDown = listen && (GetAsyncKeyState(kPrevKey) & 0x8000);
    const bool nextDown = listen && (GetAsyncKeyState(kNextKey) & 0x8000);
    if (prevDown && !m_prevDown) step(-1);
    if (nextDown && !m_nextDown) step(+1);
    m_prevDown = prevDown;
    m_nextDown = nextDown;
}

bool StartPosSwitcher::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("StartPos switcher (Q / E)", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void StartPosSwitcher::drawOverlay() {
    if (!m_available || !m_enabled || GetTickCount64() - g_shownAt > 1500) return;
    char text[48];
    snprintf(text, sizeof(text), "StartPos %d/%d", g_shownIndex, g_shownTotal);
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 size = ImGui::CalcTextSize(text);
    const ImVec2 pos{viewport->Pos.x + (viewport->Size.x - size.x) * 0.5f, viewport->Pos.y + viewport->Size.y - size.y - 12.f};
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    draw->AddText({pos.x + 1.f, pos.y + 1.f}, IM_COL32(0, 0, 0, 200), text);
    draw->AddText(pos, IM_COL32(255, 255, 255, 255), text);
}

void StartPosSwitcher::load(const nlohmann::json& j) {
    j.value("enabled", false) ? enable() : disable();
}

void StartPosSwitcher::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}};
}

} // namespace features
