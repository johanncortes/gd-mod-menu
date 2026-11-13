#include "smart_startpos.hpp"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

// Estado del jugador que guarda un StartPos (LevelSettingsObject)
struct StartState {
    int mode = 0;  // 0 cubo, 1 nave, 2 bola, 3 ovni, 4 onda, 5 robot, 6 araña, 7 swing
    int speed = 0; // 0 normal, 1 lenta, 2 rápida, 3 más rápida, 4 la más rápida
    bool mini = false;
    bool dual = false;
    bool flipped = false; // gravedad invertida
    bool mirror = false;
};

// Un portal del nivel: su X y lo que cambia
struct Portal {
    float x;
    void (*apply)(StartState& state, int value);
    int value;
};

std::vector<Portal> g_portals;
void* g_layer = nullptr;  // PlayLayer del que son los portales
bool g_pending = false;   // falta ajustar los StartPos de ese nivel (se hace en su primer intento)
bool g_smart = false;
bool g_resetCamera = false;

// Offsets leídos de las firmas
int32_t g_listOffset = 0;        // GJBaseGameLayer::m_startPosObjects (CCArray*)
int32_t g_levelSettingsOffset = 0; // GJBaseGameLayer::m_levelSettings
int32_t g_settingsOffset = 0;    // StartPosObject::m_startSettings
int32_t g_platformerOffset = 0;  // LevelSettingsObject::...
int32_t g_modeOffset = 0, g_speedOffset = 0, g_miniOffset = 0, g_dualOffset = 0, g_flippedOffset = 0;
int32_t g_mirrorOffset = 0, g_resetCameraOffset = 0;

// libcocos2d
using ArrayCountFn = unsigned (*)(const void* array);
using ArrayAtFn = void* (*)(void* array, unsigned index);
using GetPositionFn = const float* (*)(void* node); // devuelve const CCPoint&
ArrayCountFn g_arrayCount = nullptr;
ArrayAtFn g_arrayAt = nullptr;
GetPositionFn g_getPosition = nullptr;

template <typename T>
T& fieldAt(void* object, int32_t offset) {
    return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
}

// Qué hace cada portal, por tipo de objeto (GameObjectType) o, los de velocidad, por número
void setMode(StartState& s, int value) { s.mode = value; }
void setSpeed(StartState& s, int value) { s.speed = value; }
void setMini(StartState& s, int value) { s.mini = value != 0; }
void setDual(StartState& s, int value) { s.dual = value != 0; }
void setMirror(StartState& s, int value) { s.mirror = value != 0; }
void setGravity(StartState& s, int value) { s.flipped = value == 2 ? !s.flipped : value != 0; }

struct PortalKind {
    int key; // tipo de objeto o número de objeto
    void (*apply)(StartState& state, int value);
    int value;
};
constexpr PortalKind kByType[] = {
    {6, setMode, 0},  {5, setMode, 1},  {16, setMode, 2}, {19, setMode, 3},  {26, setMode, 4},
    {27, setMode, 5}, {33, setMode, 6}, {41, setMode, 7},
    {3, setGravity, 1}, {4, setGravity, 0}, {42, setGravity, 2},
    {14, setMirror, 1}, {15, setMirror, 0}, {17, setMini, 0}, {18, setMini, 1},
    {23, setDual, 1},   {24, setDual, 0},
};
constexpr PortalKind kSpeedById[] = {
    {200, setSpeed, 1}, {201, setSpeed, 0}, {202, setSpeed, 2}, {203, setSpeed, 3}, {1334, setSpeed, 4},
};

void onLevelInit(void* playLayer) {
    g_portals.clear();
    g_layer = playLayer;
    g_pending = true;
}

void onAddObject(void* playLayer, void* object) {
    if (playLayer != g_layer || !g_smart) return;
    const int type = core::game::objectType(object);
    const int id = core::game::objectId(object);
    for (const PortalKind& kind : kByType)
        if (kind.key == type) {
            g_portals.push_back({g_getPosition(object)[0], kind.apply, kind.value});
            return;
        }
    for (const PortalKind& kind : kSpeedById)
        if (kind.key == id) {
            g_portals.push_back({g_getPosition(object)[0], kind.apply, kind.value});
            return;
        }
}

// Primer intento del nivel: todos los objetos están cargados y aún no se usó ningún StartPos
void onResetLevel(void* playLayer) {
    if (playLayer != g_layer || !g_pending) return;
    g_pending = false;
    void* list = fieldAt<void*>(playLayer, g_listOffset);
    const unsigned total = list ? g_arrayCount(list) : 0;
    void* levelSettings = fieldAt<void*>(playLayer, g_levelSettingsOffset);
    if (total == 0 || !levelSettings) return;

    const bool smart = g_smart && !fieldAt<bool>(levelSettings, g_platformerOffset);
    std::stable_sort(g_portals.begin(), g_portals.end(), [](const Portal& a, const Portal& b) { return a.x < b.x; });
    StartState initial;
    initial.mode = fieldAt<int32_t>(levelSettings, g_modeOffset);
    initial.speed = fieldAt<int32_t>(levelSettings, g_speedOffset);
    initial.mini = fieldAt<bool>(levelSettings, g_miniOffset);
    initial.dual = fieldAt<bool>(levelSettings, g_dualOffset);
    initial.flipped = fieldAt<bool>(levelSettings, g_flippedOffset);
    initial.mirror = fieldAt<bool>(levelSettings, g_mirrorOffset);

    for (unsigned i = 0; i < total; ++i) {
        void* startPos = g_arrayAt(list, i);
        void* settings = startPos ? fieldAt<void*>(startPos, g_settingsOffset) : nullptr;
        if (!settings) continue;
        if (g_resetCamera && g_resetCameraOffset) fieldAt<bool>(settings, g_resetCameraOffset) = true;
        if (!smart) continue;
        const float x = g_getPosition(startPos)[0];
        StartState state = initial;
        for (const Portal& portal : g_portals) {
            if (portal.x > x) break;
            portal.apply(state, portal.value);
        }
        fieldAt<int32_t>(settings, g_modeOffset) = state.mode;
        fieldAt<int32_t>(settings, g_speedOffset) = state.speed;
        fieldAt<bool>(settings, g_miniOffset) = state.mini;
        fieldAt<bool>(settings, g_dualOffset) = state.dual;
        fieldAt<bool>(settings, g_flippedOffset) = state.flipped;
        fieldAt<bool>(settings, g_mirrorOffset) = state.mirror;
    }
    g_portals.clear();
}

} // namespace

void SmartStartPos::init(bool gameAvailable) {
    HMODULE exe = GetModuleHandleW(nullptr);
    const auto readOffset = [exe](const char* signature, int at) -> int32_t {
        const uint8_t* found = core::memory::findPattern(exe, signature);
        int32_t offset = 0;
        if (found) memcpy(&offset, found + at, 4);
        return offset > 0 && offset < 0x10000 ? offset : 0;
    };
    g_listOffset = readOffset(sig::kStartPosObjects, sig::kStartPosObjectsArray);
    g_levelSettingsOffset = readOffset(sig::kLevelSettingsPlayers, sig::kLevelSettingsObject);
    g_platformerOffset = readOffset(sig::kLevelSettingsPlayers, sig::kLevelSettingsPlatformer);
    g_settingsOffset = readOffset(sig::kLoadStartPosSettings, sig::kLoadStartPosSettingsOffset);
    g_resetCameraOffset = readOffset(sig::kStartPosResetCamera, sig::kStartPosResetCameraOffset);
    g_flippedOffset = readOffset(sig::kSetupLevelStart, sig::kSetupLevelStartFlipped);
    g_dualOffset = readOffset(sig::kSetupLevelStartDual, sig::kSetupLevelStartDualOffset);
    g_miniOffset = readOffset(sig::kSetupLevelStartMini, sig::kSetupLevelStartMiniOffset);
    g_mirrorOffset = readOffset(sig::kSetupLevelStartMirror, sig::kSetupLevelStartMirrorOffset);
    g_speedOffset = readOffset(sig::kSetupLevelStartMirror, sig::kSetupLevelStartSpeed);
    g_modeOffset = readOffset(sig::kPlatformerStartMode, sig::kPlatformerStartModeOffset);

    if (HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll")) {
        g_arrayCount = reinterpret_cast<ArrayCountFn>(GetProcAddress(cocos, "?count@CCArray@cocos2d@@QEBAIXZ"));
        g_arrayAt = reinterpret_cast<ArrayAtFn>(
            GetProcAddress(cocos, "?objectAtIndex@CCArray@cocos2d@@QEAAPEAVCCObject@2@I@Z"));
        g_getPosition = reinterpret_cast<GetPositionFn>(
            GetProcAddress(cocos, "?getPosition@CCNode@cocos2d@@UEAAAEBVCCPoint@2@XZ"));
    }

    const bool base = gameAvailable && core::game::loadHooksAvailable() && g_arrayCount && g_arrayAt && g_getPosition &&
                      g_listOffset && g_levelSettingsOffset && g_settingsOffset;
    m_available = base && g_platformerOffset && g_modeOffset && g_speedOffset && g_miniOffset && g_dualOffset &&
                  g_flippedOffset && g_mirrorOffset;
    m_cameraAvailable = base && g_resetCameraOffset;
    if (base) {
        core::game::onLevelInit(&onLevelInit);
        core::game::onAddObject(&onAddObject);
        core::game::onResetLevel(&onResetLevel);
    }
    core::logMsg("Smart StartPos: %s; reset camera: %s", m_available ? "listo" : "no disponible",
                 m_cameraAvailable ? "listo" : "no disponible");
}

void SmartStartPos::enable() {
    m_enabled = true;
    g_smart = m_available;
}

void SmartStartPos::disable() {
    m_enabled = false;
    g_smart = false;
}

bool SmartStartPos::drawUI() {
    bool enabled = m_enabled;
    bool changed = gui::toggle("Smart StartPos", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    if (gui::toggle("StartPos reset camera", m_cameraAvailable, m_resetCamera)) {
        g_resetCamera = m_resetCamera && m_cameraAvailable;
        changed = true;
    }
    return changed;
}

void SmartStartPos::load(const nlohmann::json& j) {
    m_resetCamera = j.value("reset_camera", false);
    g_resetCamera = m_resetCamera && m_cameraAvailable;
    j.value("enabled", false) ? enable() : disable();
}

void SmartStartPos::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"reset_camera", m_resetCamera}};
}

} // namespace features
