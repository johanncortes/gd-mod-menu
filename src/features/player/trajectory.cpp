#include "trajectory.hpp"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <MinHook.h>
#include <imgui.h>
#include <nlohmann/json.hpp>

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

constexpr int kJumpButton = 1;                      // PlayerButton::Jump
constexpr int kKeptTypes[] = {0, 2, 0x15, 0x19, 0x2f}; // sólidos, peligros y rampas (ver docs/firmas.md)
constexpr float kMinSeconds = 0.25f, kMaxSeconds = 3.f;

// --- Funciones del juego ---
using CreateFn = void* (*)(int icon, int ship, void* gameLayer, void* parentLayer, bool playLayer);
using PlayerFn = void (*)(void* player, void* other);
using PlayerFloatFn = void (*)(void* player, float dt);
using ButtonFn = bool (*)(void* player, int button);
using CheckCollisionsFn = int (*)(void* gameLayer, void* player, float dt, bool ignoreDamage);
using CollisionObjectsFn = void (*)(void* gameLayer, void* player, void* objects, int count, float dt);
using SpiderDashFn = void (*)(void* player, uint64_t from, uint64_t to); // dos CCPoint por valor
using GameEventFn = void (*)(void* gameLayer, int event, int material, int playerId);
using DestroyObjectFn = void (*)(void* gameLayer, void* object);

CreateFn g_create = nullptr;
PlayerFn g_copyAttributes = nullptr;
PlayerFloatFn g_updateRotation = nullptr;
ButtonFn g_pushButton = nullptr;
ButtonFn g_releaseButton = nullptr;
CheckCollisionsFn g_checkCollisions = nullptr;
CollisionObjectsFn g_origCollisionObjects = nullptr;
SpiderDashFn g_origSpiderDash = nullptr;
GameEventFn g_origGameEvent = nullptr;
DestroyObjectFn g_origDestroyObject = nullptr;

// Offsets leídos de las firmas
int32_t g_lastPosOffset = 0;      // PlayerObject: posición del tick anterior
int32_t g_typeOffset = 0;         // GameObject::m_objectType
int32_t g_platformerOffset = 0;   // GJBaseGameLayer::m_isPlatformer
int32_t g_logDicts[4] = {};       // PlayerObject: registros de colisiones (CCDictionary*)
int32_t g_logCopy[4] = {};        // origen, destino, origen, destino (int)
int32_t g_logClear[2] = {};       // int64 que pasan a -1
const uint8_t* g_stepsDisp = nullptr; // desplazamiento RIP de la constante "pasos por 1/60 s"

// --- libcocos2d ---
using NodeCreateFn = void* (*)();
using NodeFn = void (*)(void* node);
using NodeChildFn = void (*)(void* parent, void* child);
using NodeBoolFn = void (*)(void* node, bool value);
using GetParentFn = void* (*)(void* node);
using ToWorldFn = Point* (*)(void* node, Point* out, const Point* in);
using GetPositionFn = const Point& (*)(void* node);
using UpdateFn = void (*)(void* node, float dt);

NodeCreateFn g_layerCreate = nullptr;
NodeChildFn g_addChild = nullptr;
NodeBoolFn g_setVisible = nullptr;
NodeBoolFn g_removeFromParent = nullptr;
GetParentFn g_getParent = nullptr;
NodeFn g_retain = nullptr;
NodeFn g_release = nullptr;
NodeFn g_dictClear = nullptr;
ToWorldFn g_toWorld = nullptr;
int32_t g_updateSlot = -1;      // CCNode::update(float) en el vtable
int32_t g_getPositionSlot = -1; // CCNode::getPosition() en el vtable

// --- Estado (solo hilo del juego) ---
bool g_enabled = false;
float g_seconds = 1.f;
void* g_container = nullptr; // CCLayer invisible dentro del nivel, retenido por nosotros
void* g_fake = nullptr;      // jugador simulado, hijo de g_container
bool g_simulating = false;

struct Path {
    std::vector<Point> points; // en coordenadas de la capa de objetos
    bool died = false;
};
Path g_paths[2];             // 0 = manteniendo, 1 = soltando
void* g_pathsLayer = nullptr; // nivel al que pertenecen los caminos

template <typename T>
T& fieldAt(void* object, int32_t offset) {
    return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
}

template <typename Fn>
Fn virtualAt(void* object, int32_t slot) {
    return fieldAt<Fn>(*static_cast<void**>(object), slot);
}

int32_t readInt(const uint8_t* at) {
    int32_t value;
    memcpy(&value, at, 4);
    return value;
}

// Posición de un método virtual de CCNode en su vtable (se busca el export dentro del vtable
// exportado de la clase)
int32_t nodeSlot(HMODULE cocos, const char* method) {
    const auto* vtable = reinterpret_cast<void* const*>(GetProcAddress(cocos, "??_7CCNode@cocos2d@@6B@"));
    const void* function = reinterpret_cast<const void*>(GetProcAddress(cocos, method));
    if (!vtable || !function) return -1;
    for (int32_t i = 0; i < 128; ++i)
        if (vtable[i] == function) return i * 8;
    return -1;
}

bool isFake(void* player) {
    return g_simulating && player && player == g_fake;
}

// GJBaseGameLayer::collisionCheckObjects: al jugador simulado solo le pasan sólidos, peligros y
// rampas. El resto de objetos (portales, monedas, triggers...) cambiaría la partida real.
void collisionObjectsHook(void* gameLayer, void* player, void* objects, int count, float dt) {
    if (!isFake(player) || !objects) {
        g_origCollisionObjects(gameLayer, player, objects, count, dt);
        return;
    }
    static std::vector<void*> kept;
    kept.clear();
    void** data = *static_cast<void***>(objects); // el vector empieza por el puntero a sus datos
    for (int i = 0; i < count; ++i) {
        const int type = fieldAt<int>(data[i], g_typeOffset);
        if (std::find(std::begin(kKeptTypes), std::end(kKeptTypes), type) != std::end(kKeptTypes))
            kept.push_back(data[i]);
    }
    void* begin = kept.data();
    g_origCollisionObjects(gameLayer, player, &begin, static_cast<int>(kept.size()), dt);
}

// PlayerObject::playSpiderDashEffect: crea un sprite nuevo cada vez; el simulado no lo necesita
void spiderDashHook(void* player, uint64_t from, uint64_t to) {
    if (isFake(player)) return;
    g_origSpiderDash(player, from, to);
}

// GJBaseGameLayer::gameEventTriggered: saltos y aterrizajes activan los triggers de evento del
// nivel. Los del jugador simulado no deben activar nada.
void gameEventHook(void* gameLayer, int event, int material, int playerId) {
    if (g_simulating) return;
    g_origGameEvent(gameLayer, event, material, playerId);
}

// GJBaseGameLayer::destroyObject: chocar con un bloque rompible lo destruye
void destroyObjectHook(void* gameLayer, void* object) {
    if (g_simulating) return;
    g_origDestroyObject(gameLayer, object);
}

void releaseFake() {
    if (!g_container) return;
    if (g_getParent(g_container)) g_removeFromParent(g_container, true);
    g_release(g_container); // se lleva consigo al jugador simulado
    g_container = g_fake = nullptr;
}

// Crea (o reutiliza) el jugador simulado dentro de este nivel
bool ensureFake(void* playLayer) {
    if (g_container && g_getParent(g_container) == playLayer) return g_fake != nullptr;
    releaseFake(); // era de otro nivel (o el nivel ya no existe)

    g_container = g_layerCreate();
    if (!g_container) return false;
    g_retain(g_container);
    g_setVisible(g_container, false);
    g_addChild(playLayer, g_container);
    // playLayer = false: así el juego no cuenta sus saltos en las estadísticas
    g_fake = g_create(1, 1, playLayer, g_container, false);
    if (!g_fake) {
        releaseFake();
        return false;
    }
    g_addChild(g_container, g_fake);
    return true;
}

// Lo que hace GJBaseGameLayer::update con el jugador al empezar cada tick
void resetCollisionLog(void* player) {
    for (int32_t dict : g_logDicts)
        if (void* log = fieldAt<void*>(player, dict)) g_dictClear(log);
    fieldAt<int32_t>(player, g_logCopy[1]) = fieldAt<int32_t>(player, g_logCopy[0]);
    fieldAt<int32_t>(player, g_logCopy[3]) = fieldAt<int32_t>(player, g_logCopy[2]);
    fieldAt<int64_t>(player, g_logClear[0]) = -1;
    fieldAt<int64_t>(player, g_logClear[1]) = -1;
}

void simulate(void* playLayer, void* real, bool hold, float stepDt, int steps, Path& path) {
    path.points.clear();
    path.died = false;
    g_copyAttributes(g_fake, real);
    hold ? g_pushButton(g_fake, kJumpButton) : g_releaseButton(g_fake, kJumpButton);
    path.points.push_back(virtualAt<GetPositionFn>(g_fake, g_getPositionSlot)(g_fake));

    for (int i = 0; i < steps; ++i) {
        resetCollisionLog(g_fake);
        virtualAt<UpdateFn>(g_fake, g_updateSlot)(g_fake, stepDt);
        const bool died = g_checkCollisions(playLayer, g_fake, stepDt, true) == 1;
        g_updateRotation(g_fake, stepDt);
        const Point position = virtualAt<GetPositionFn>(g_fake, g_getPositionSlot)(g_fake);
        fieldAt<Point>(g_fake, g_lastPosOffset) = position;
        path.points.push_back(position);
        if (died) {
            path.died = true;
            break;
        }
    }
}

// Pasos de física por cada 1/60 s (4 a 240 TPS). Se lee la constante que usa el juego, así que
// sigue al Physics bypass.
float stepDelta() {
    const double* steps = reinterpret_cast<const double*>(g_stepsDisp + 4 + readInt(g_stepsDisp));
    return *steps > 0.0 ? static_cast<float>(1.0 / *steps) : 0.25f;
}

// Después de cada frame de nivel (aviso de core::game)
void onUpdate(void* gameLayer, float) {
    if (!g_enabled || gameLayer != core::game::playLayer()) return;
    void* real = core::game::player1(gameLayer);
    if (!real || fieldAt<bool>(gameLayer, g_platformerOffset)) {
        g_paths[0].points.clear();
        g_paths[1].points.clear();
        return;
    }
    if (!ensureFake(gameLayer)) return;

    const float dt = stepDelta();
    const int steps = std::clamp(static_cast<int>(g_seconds * 60.f / dt), 1, 4000);
    g_simulating = true;
    simulate(gameLayer, real, true, dt, steps, g_paths[0]);
    simulate(gameLayer, real, false, dt, steps, g_paths[1]);
    g_simulating = false;
    g_pathsLayer = gameLayer;
}

void onResetLevel(void*) {
    g_paths[0].points.clear();
    g_paths[1].points.clear();
}

bool hookAt(void* target, void* hook, void** original) {
    return target && MH_CreateHook(target, hook, original) == MH_OK && MH_EnableHook(target) == MH_OK;
}

} // namespace

void Trajectory::init(bool gameAvailable) {
    HMODULE exe = GetModuleHandleW(nullptr);
    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    auto find = [exe](const char* pattern) { return core::memory::findPattern(exe, pattern); };

    uint8_t* create = find(sig::kPlayerCreate);
    uint8_t* copy = find(sig::kCopyAttributes);
    uint8_t* rotation = find(sig::kUpdateRotation);
    uint8_t* push = find(sig::kPushButton);
    uint8_t* release = find(sig::kReleaseButton);
    uint8_t* checkCollisions = find(sig::kCheckCollisions);
    uint8_t* collisionObjects = find(sig::kCollisionCheckObjects);
    uint8_t* typeRead = find(sig::kObjectTypeRead);
    uint8_t* spiderDash = find(sig::kSpiderDashEffect);
    uint8_t* logReset = find(sig::kCollisionLogReset);
    uint8_t* percent = find(sig::kCurrentPercent);
    uint8_t* steps = find(sig::kStepsPerFrame);
    uint8_t* gameEvent = find(sig::kGameEventTriggered);
    uint8_t* destroyObject = find(sig::kDestroyObject);
    if (!gameAvailable || !core::game::levelHooksAvailable() || !cocos || !create || !copy || !rotation || !push ||
        !release || !checkCollisions || !collisionObjects || !typeRead || !spiderDash || !logReset || !percent ||
        !steps || !gameEvent || !destroyObject) {
        core::logMsg("Show trajectory: no disponible (falta alguna firma o los hooks de nivel)");
        return;
    }

    g_create = reinterpret_cast<CreateFn>(create);
    g_copyAttributes = reinterpret_cast<PlayerFn>(copy);
    g_updateRotation = reinterpret_cast<PlayerFloatFn>(rotation);
    g_pushButton = reinterpret_cast<ButtonFn>(push);
    g_releaseButton = reinterpret_cast<ButtonFn>(release);
    g_checkCollisions = reinterpret_cast<CheckCollisionsFn>(checkCollisions);
    g_lastPosOffset = readInt(copy + sig::kCopyAttributesLastPos);
    g_typeOffset = readInt(typeRead + sig::kObjectTypeOffset);
    g_platformerOffset = readInt(percent + sig::kCurrentPercentPlatformer);
    for (int i = 0; i < 4; ++i) g_logDicts[i] = readInt(logReset + sig::kCollisionLogDicts[i]);
    for (int i = 0; i < 4; ++i) g_logCopy[i] = readInt(logReset + sig::kCollisionLogCopy[i]);
    for (int i = 0; i < 2; ++i) g_logClear[i] = readInt(logReset + sig::kCollisionLogClear[i]);
    g_stepsDisp = steps + sig::kStepsPerFrameDisp;

    auto get = [cocos](const char* name) { return reinterpret_cast<void*>(GetProcAddress(cocos, name)); };
    g_layerCreate = reinterpret_cast<NodeCreateFn>(get("?create@CCLayer@cocos2d@@SAPEAV12@XZ"));
    g_addChild = reinterpret_cast<NodeChildFn>(get("?addChild@CCNode@cocos2d@@UEAAXPEAV12@@Z"));
    g_setVisible = reinterpret_cast<NodeBoolFn>(get("?setVisible@CCNode@cocos2d@@UEAAX_N@Z"));
    g_removeFromParent = reinterpret_cast<NodeBoolFn>(get("?removeFromParentAndCleanup@CCNode@cocos2d@@UEAAX_N@Z"));
    g_getParent = reinterpret_cast<GetParentFn>(get("?getParent@CCNode@cocos2d@@UEAAPEAV12@XZ"));
    g_retain = reinterpret_cast<NodeFn>(get("?retain@CCObject@cocos2d@@QEAAXXZ"));
    g_release = reinterpret_cast<NodeFn>(get("?release@CCObject@cocos2d@@QEAAXXZ"));
    g_dictClear = reinterpret_cast<NodeFn>(get("?removeAllObjects@CCDictionary@cocos2d@@QEAAXXZ"));
    g_toWorld = reinterpret_cast<ToWorldFn>(get("?convertToWorldSpace@CCNode@cocos2d@@QEAA?AVCCPoint@2@AEBV32@@Z"));
    g_updateSlot = nodeSlot(cocos, "?update@CCNode@cocos2d@@UEAAXM@Z");
    g_getPositionSlot = nodeSlot(cocos, "?getPosition@CCNode@cocos2d@@UEAAAEBVCCPoint@2@XZ");

    const auto plausible = [](int32_t offset) { return offset > 0 && offset < 0x10000; };
    bool ok = g_layerCreate && g_addChild && g_setVisible && g_removeFromParent && g_getParent && g_retain &&
              g_release && g_dictClear && g_toWorld && g_updateSlot >= 0 && g_getPositionSlot >= 0 &&
              plausible(g_lastPosOffset) && plausible(g_typeOffset) && plausible(g_platformerOffset);
    for (int32_t offset : g_logDicts) ok = ok && plausible(offset);
    for (int32_t offset : g_logCopy) ok = ok && plausible(offset);
    for (int32_t offset : g_logClear) ok = ok && plausible(offset);
    if (!ok) {
        core::logMsg("Show trajectory: no disponible (exports de cocos2d u offsets inesperados)");
        return;
    }

    m_available = hookAt(collisionObjects, &collisionObjectsHook, reinterpret_cast<void**>(&g_origCollisionObjects)) &&
                  hookAt(spiderDash, &spiderDashHook, reinterpret_cast<void**>(&g_origSpiderDash)) &&
                  hookAt(gameEvent, &gameEventHook, reinterpret_cast<void**>(&g_origGameEvent)) &&
                  hookAt(destroyObject, &destroyObjectHook, reinterpret_cast<void**>(&g_origDestroyObject));
    if (m_available) {
        core::game::onUpdate(&onUpdate);
        core::game::onResetLevel(&onResetLevel);
    }
    core::logMsg("Show trajectory: %s", m_available ? "listo" : "no disponible (hook)");
}

void Trajectory::enable() {
    m_enabled = true;
    g_enabled = m_available;
}

void Trajectory::disable() {
    m_enabled = false;
    g_enabled = false;
    g_paths[0].points.clear();
    g_paths[1].points.clear();
    if (m_available) releaseFake();
}

bool Trajectory::drawUI() {
    bool enabled = m_enabled;
    bool changed = gui::toggle("Show trajectory", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    if (m_available && m_enabled) {
        ImGui::SetNextItemWidth(140.f * ImGui::GetIO().FontGlobalScale);
        if (ImGui::SliderFloat("Longitud", &g_seconds, kMinSeconds, kMaxSeconds, "%.2f s")) {
            g_seconds = std::clamp(g_seconds, kMinSeconds, kMaxSeconds);
            changed = true;
        }
    }
    return changed;
}

void Trajectory::drawOverlay() {
    void* playLayer = core::game::playLayer();
    if (!g_enabled || !playLayer || playLayer != g_pathsLayer) return;
    void* real = core::game::player1(playLayer);
    void* objectLayer = real ? g_getParent(real) : nullptr;
    if (!objectLayer) return;

    // Coordenadas de la capa de objetos -> pantalla. GD dibuja con 320 puntos de alto.
    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    const float scale = screen.y / 320.f;
    const float thickness = std::max(1.5f, screen.y / 540.f);
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    const ImU32 colors[2] = {IM_COL32(80, 255, 80, 220), IM_COL32(255, 80, 80, 220)};
    std::vector<ImVec2> line;
    for (int i = 0; i < 2; ++i) {
        const Path& path = g_paths[i];
        if (path.points.size() < 2) continue;
        line.clear();
        for (const Point& p : path.points) {
            Point world{};
            g_toWorld(objectLayer, &world, &p);
            line.push_back({world.x * scale, screen.y - world.y * scale});
        }
        draw->AddPolyline(line.data(), static_cast<int>(line.size()), colors[i], ImDrawFlags_None, thickness);
        if (path.died) {
            // Una X donde chocaría
            const ImVec2 end = line.back();
            const float r = thickness * 3.f;
            draw->AddLine({end.x - r, end.y - r}, {end.x + r, end.y + r}, colors[i], thickness);
            draw->AddLine({end.x - r, end.y + r}, {end.x + r, end.y - r}, colors[i], thickness);
        }
    }
}

void Trajectory::load(const nlohmann::json& j) {
    g_seconds = std::clamp(j.value("seconds", g_seconds), kMinSeconds, kMaxSeconds);
    j.value("enabled", false) ? enable() : disable();
}

void Trajectory::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"seconds", g_seconds}};
}

} // namespace features
