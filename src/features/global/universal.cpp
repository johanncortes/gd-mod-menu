#include "universal.hpp"

#include <windows.h>

#include <MinHook.h>
#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "gui/widgets.hpp"

namespace features {

// --- Lock cursor ---

namespace {

// Ventana del juego si es la que está en primer plano
HWND gameWindowInFront() {
    HWND window = GetForegroundWindow();
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    return pid == GetCurrentProcessId() ? window : nullptr;
}

} // namespace

void LockCursor::enable() {
    m_enabled = true;
}

void LockCursor::disable() {
    m_enabled = false;
    if (m_clipped) ClipCursor(nullptr);
    m_clipped = false;
}

void LockCursor::update(bool menuOpen) {
    HWND window = (m_enabled && !menuOpen && core::game::levelRunning()) ? gameWindowInFront() : nullptr;
    RECT client;
    if (window && GetClientRect(window, &client)) {
        POINT corners[2] = {{client.left, client.top}, {client.right, client.bottom}};
        MapWindowPoints(window, nullptr, corners, 2);
        const RECT area{corners[0].x, corners[0].y, corners[1].x, corners[1].y};
        ClipCursor(&area); // Windows lo quita al cambiar de ventana: se pone cada frame
        m_clipped = true;
    } else if (m_clipped) {
        ClipCursor(nullptr);
        m_clipped = false;
    }
}

bool LockCursor::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Lock cursor", true, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void LockCursor::load(const nlohmann::json& j) {
    j.value("lock_cursor", false) ? enable() : disable();
}

void LockCursor::save(nlohmann::json& j) const {
    j["lock_cursor"] = m_enabled;
}

// --- No transition ---

namespace {

using TransitionFadeFn = void* (*)(float duration, void* scene);
TransitionFadeFn g_origTransitionFade = nullptr;
bool g_noTransition = false;

void* transitionFadeHook(float duration, void* scene) {
    return g_origTransitionFade(g_noTransition ? 0.f : duration, scene);
}

} // namespace

void NoTransition::init() {
    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    void* create = cocos ? reinterpret_cast<void*>(GetProcAddress(
                               cocos, "?create@CCTransitionFade@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z"))
                         : nullptr;
    if (!create) {
        core::logMsg("No transition: no disponible (falta CCTransitionFade::create)");
        return;
    }
    m_available = MH_CreateHook(create, &transitionFadeHook, reinterpret_cast<void**>(&g_origTransitionFade)) ==
                      MH_OK &&
                  MH_EnableHook(create) == MH_OK;
    core::logMsg("No transition: %s", m_available ? "listo" : "no disponible (hook)");
}

void NoTransition::enable() {
    m_enabled = true;
    g_noTransition = m_available;
}

void NoTransition::disable() {
    m_enabled = false;
    g_noTransition = false;
}

bool NoTransition::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("No transition", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void NoTransition::load(const nlohmann::json& j) {
    j.value("no_transition", false) ? enable() : disable();
}

void NoTransition::save(nlohmann::json& j) const {
    j["no_transition"] = m_enabled;
}

// --- High priority ---

void HighPriority::enable() {
    m_enabled = true;
    if (!m_previous) m_previous = GetPriorityClass(GetCurrentProcess());
    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
        core::logMsg("High priority: SetPriorityClass falló (%lu)", GetLastError());
}

void HighPriority::disable() {
    m_enabled = false;
    if (m_previous) SetPriorityClass(GetCurrentProcess(), m_previous);
    m_previous = 0;
}

bool HighPriority::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("High priority", true, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void HighPriority::load(const nlohmann::json& j) {
    j.value("high_priority", false) ? enable() : disable();
}

void HighPriority::save(nlohmann::json& j) const {
    j["high_priority"] = m_enabled;
}

} // namespace features
