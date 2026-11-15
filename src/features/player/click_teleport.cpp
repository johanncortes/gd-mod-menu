#include "click_teleport.hpp"

#include <windows.h>

#include <cstdint>

#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "gui/menu.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

struct Point {
    float x, y; // mismo formato que cocos2d::CCPoint y CCSize
};

// libcocos2d
using SharedDirectorFn = void* (*)();
using WinSizeFn = Point* (*)(void* director, Point* out);
using GetParentFn = void* (*)(void* node);
using ToNodeFn = Point* (*)(void* node, Point* out, const Point* in);
using SetPositionFn = void (*)(void* node, const Point& point); // CCNode::setPosition (virtual)
SharedDirectorFn g_sharedDirector = nullptr;
WinSizeFn g_winSize = nullptr;
GetParentFn g_getParent = nullptr;
ToNodeFn g_toNode = nullptr;
int g_setPositionSlot = -1; // posición en el vtable de CCNode

// Posición de CCNode::setPosition(const CCPoint&) en el vtable exportado de CCNode. Llamándola
// por el vtable, PlayerObject usa su versión (la que también mueve su posición de física).
int findSetPositionSlot(HMODULE cocos) {
    const auto* vtable = reinterpret_cast<void* const*>(GetProcAddress(cocos, "??_7CCNode@cocos2d@@6B@"));
    const void* setPosition =
        reinterpret_cast<const void*>(GetProcAddress(cocos, "?setPosition@CCNode@cocos2d@@UEAAXAEBVCCPoint@2@@Z"));
    if (!vtable || !setPosition) return -1;
    for (int i = 0; i < 128; ++i)
        if (vtable[i] == setPosition) return i;
    return -1;
}

// Cursor dentro de la ventana del juego (que tiene que estar en primer plano), en fracción 0..1
// con el origen abajo a la izquierda, como las coordenadas de cocos.
bool cursorFraction(Point& out) {
    HWND window = GetForegroundWindow();
    DWORD owner = 0;
    if (!window || !GetWindowThreadProcessId(window, &owner) || owner != GetCurrentProcessId()) return false;
    POINT cursor;
    RECT client;
    if (!GetCursorPos(&cursor) || !ScreenToClient(window, &cursor) || !GetClientRect(window, &client)) return false;
    if (client.right <= 0 || client.bottom <= 0 || !PtInRect(&client, cursor)) return false;
    out = {static_cast<float>(cursor.x) / static_cast<float>(client.right),
           1.f - static_cast<float>(cursor.y) / static_cast<float>(client.bottom)};
    return true;
}

void teleport(void* playLayer) {
    void* player = core::game::player1(playLayer);
    void* parent = player ? g_getParent(player) : nullptr;
    void* director = g_sharedDirector();
    Point fraction;
    if (!parent || !director || !cursorFraction(fraction)) return;
    Point size{}, target{};
    g_winSize(director, &size);
    const Point screen{fraction.x * size.x, fraction.y * size.y};
    g_toNode(parent, &target, &screen);
    auto setPosition = static_cast<SetPositionFn*>(*static_cast<void**>(player))[g_setPositionSlot];
    setPosition(player, target);
}

} // namespace

void ClickTeleport::init(bool gameAvailable) {
    if (HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll")) {
        g_sharedDirector = reinterpret_cast<SharedDirectorFn>(
            GetProcAddress(cocos, "?sharedDirector@CCDirector@cocos2d@@SAPEAV12@XZ"));
        g_winSize =
            reinterpret_cast<WinSizeFn>(GetProcAddress(cocos, "?getWinSize@CCDirector@cocos2d@@QEAA?AVCCSize@2@XZ"));
        g_getParent = reinterpret_cast<GetParentFn>(GetProcAddress(cocos, "?getParent@CCNode@cocos2d@@UEAAPEAV12@XZ"));
        g_toNode = reinterpret_cast<ToNodeFn>(
            GetProcAddress(cocos, "?convertToNodeSpace@CCNode@cocos2d@@QEAA?AVCCPoint@2@AEBV32@@Z"));
        g_setPositionSlot = findSetPositionSlot(cocos);
    }
    m_available = gameAvailable && g_sharedDirector && g_winSize && g_getParent && g_toNode && g_setPositionSlot >= 0;
    core::logMsg("Click teleport: %s", m_available ? "listo" : "no disponible");
}

void ClickTeleport::enable() {
    m_enabled = true;
}

void ClickTeleport::disable() {
    m_enabled = false;
}

void ClickTeleport::update() {
    if (!active()) return;
    const bool down = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    const bool clicked = down && !m_wasDown;
    m_wasDown = down;
    void* playLayer = core::game::playLayer();
    // Solo con el nivel corriendo y el panel cerrado (el clic derecho del panel asigna teclas)
    if (!clicked || !playLayer || gui::isMenuOpen() || !core::game::levelRunning()) return;
    teleport(playLayer);
}

bool ClickTeleport::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Click teleport", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void ClickTeleport::load(const nlohmann::json& j) {
    j.value("click_teleport", false) ? enable() : disable();
}

void ClickTeleport::save(nlohmann::json& j) const {
    j["click_teleport"] = m_enabled;
}

} // namespace features
