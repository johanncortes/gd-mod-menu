#include "trajectory.hpp"

#include <windows.h>

#include <cmath>
#include <cstring>

#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

struct CCPoint { float x; float y; };
struct _ccColor4F { float r; float g; float b; float a; };

using CCDrawNodeCreateFn = void* (*)();
using CCDrawNodeClearFn = void (*)(void* drawNode);
using CCDrawNodeDrawSegmentFn = bool (*)(void* drawNode, const CCPoint& from, const CCPoint& to, float radius, const _ccColor4F& color);
using CCNodeAddChildFn = void (*)(void* parent, void* child, int zOrder);
using CCNodeRemoveChildFn = void (*)(void* parent, void* child, bool cleanup);
using GetPositionFn = const CCPoint* (*)(void* node);

CCDrawNodeCreateFn g_create = nullptr;
CCDrawNodeClearFn g_clear = nullptr;
CCDrawNodeDrawSegmentFn g_drawSegment = nullptr;
CCNodeAddChildFn g_addChild = nullptr;
CCNodeRemoveChildFn g_removeChild = nullptr;

bool g_enabled = false;
void* g_trajectoryNode = nullptr;
void* g_currentLayer = nullptr;

float g_prevX = 0.f;
float g_prevY = 0.f;
bool g_hasPrev = false;

constexpr int32_t kPlayer1Offset = 0xDB8;

const CCPoint* getPosition(void* node) {
    if (!node) return nullptr;
    void** vtable = *reinterpret_cast<void***>(node);
    if (!vtable) return nullptr;
    auto fn = reinterpret_cast<GetPositionFn>(vtable[0xC8 / sizeof(void*)]);
    return fn ? fn(node) : nullptr;
}

void onResetLevel(void* layer) {
    g_hasPrev = false;
    if (g_trajectoryNode) g_clear(g_trajectoryNode);
    if (g_currentLayer && g_currentLayer != layer) {
        g_trajectoryNode = nullptr;
        g_currentLayer = nullptr;
    }
}

void onUpdate(void* gameLayer, float dt) {
    if (!g_enabled || !gameLayer) {
        if (g_trajectoryNode) g_clear(g_trajectoryNode);
        return;
    }

    if (g_currentLayer != gameLayer) {
        g_trajectoryNode = nullptr;
        g_currentLayer = gameLayer;
    }

    if (!g_trajectoryNode) {
        g_trajectoryNode = g_create();
        if (g_trajectoryNode) {
            g_addChild(gameLayer, g_trajectoryNode, 1000);
        }
    }
    if (!g_trajectoryNode) return;

    g_clear(g_trajectoryNode);

    void* player = nullptr;
    memcpy(&player, static_cast<const uint8_t*>(gameLayer) + kPlayer1Offset, sizeof(player));
    if (!player) return;

    const CCPoint* pos = getPosition(player);
    if (!pos) return;

    const float currX = pos->x;
    const float currY = pos->y;

    if (g_hasPrev && dt > 0.0001f && dt < 0.2f) {
        const float vx = (currX - g_prevX) / dt;
        const float vy = (currY - g_prevY) / dt;

        // Si el jugador avanza horizontalmente en el nivel
        if (vx > 30.f) {
            float simX = currX;
            float simY = currY;
            float simVy = vy;
            // Gravedad estimada de GD para predicción de caída/salto
            const float g = -2750.f;
            constexpr int kSteps = 45;
            constexpr float kStepDt = 0.016f; // ~60 FPS pasos de simulación (~0.7s)
            const _ccColor4F color{0.25f, 0.95f, 0.50f, 0.90f}; // verde brillante

            for (int i = 0; i < kSteps; ++i) {
                const CCPoint from{simX, simY};
                simX += vx * kStepDt;
                simVy += g * kStepDt;
                simY += simVy * kStepDt;
                const CCPoint to{simX, simY};

                g_drawSegment(g_trajectoryNode, from, to, 1.8f, color);
            }
        }
    }

    g_prevX = currX;
    g_prevY = currY;
    g_hasPrev = true;
}

} // namespace

void Trajectory::init() {
    HMODULE cocos = GetModuleHandleA("libcocos2d.dll");
    if (!cocos) {
        core::logMsg("Show trajectory: no disponible (libcocos2d no encontrada)");
        return;
    }

    g_create = reinterpret_cast<CCDrawNodeCreateFn>(
        GetProcAddress(cocos, "?create@CCDrawNode@cocos2d@@SAPEAV12@XZ"));
    g_clear = reinterpret_cast<CCDrawNodeClearFn>(
        GetProcAddress(cocos, "?clear@CCDrawNode@cocos2d@@QEAAXXZ"));
    g_drawSegment = reinterpret_cast<CCDrawNodeDrawSegmentFn>(
        GetProcAddress(cocos, "?drawSegment@CCDrawNode@cocos2d@@QEAA_NAEBVCCPoint@2@0MAEBU_ccColor4F@2@@Z"));
    g_addChild = reinterpret_cast<CCNodeAddChildFn>(
        GetProcAddress(cocos, "?addChild@CCNode@cocos2d@@UEAAXPEAV12@H@Z"));
    g_removeChild = reinterpret_cast<CCNodeRemoveChildFn>(
        GetProcAddress(cocos, "?removeChild@CCNode@cocos2d@@UEAAXPEAV12@_N@Z"));

    m_available = g_create && g_clear && g_drawSegment && g_addChild && g_removeChild && core::game::levelHooksAvailable();
    if (m_available) {
        core::game::onResetLevel(&onResetLevel);
        core::game::onUpdate(&onUpdate);
    }
    core::logMsg("Show trajectory: %s", m_available ? "listo" : "no disponible (exports de CCDrawNode o hooks)");
}

void Trajectory::enable() {
    m_enabled = true;
    if (!m_available) return;
    g_enabled = true;
    g_hasPrev = false;
}

void Trajectory::disable() {
    m_enabled = false;
    g_enabled = false;
    if (g_trajectoryNode) g_clear(g_trajectoryNode);
}

bool Trajectory::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Show trajectory", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void Trajectory::load(const nlohmann::json& j) {
    j.value("enabled", false) ? enable() : disable();
}

void Trajectory::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}};
}

} // namespace features

