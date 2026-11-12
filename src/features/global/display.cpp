#include "display.hpp"

#include <windows.h>

#include <imgui.h>

#include "core/log.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

using SharedFn = void* (*)();
using GetBoolFn = bool (*)(const void* object);
using SetBoolFn = void (*)(void* object, bool value);
using ToggleFullScreenFn = void (*)(void* view, bool fullscreen, bool borderless, bool fix);

SharedFn g_sharedApplication = nullptr;
SharedFn g_sharedView = nullptr;
GetBoolFn g_getVsync = nullptr;
SetBoolFn g_toggleVsync = nullptr;
GetBoolFn g_isFullscreen = nullptr;
GetBoolFn g_isBorderless = nullptr;
GetBoolFn g_borderlessFix = nullptr;
ToggleFullScreenFn g_toggleFullScreen = nullptr;

enum Pending { None = 0, Vsync = 1, Fullscreen = 2, Borderless = 3 };

} // namespace

void Display::init() {
    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    auto get = [cocos](const char* name) {
        return cocos ? reinterpret_cast<void*>(GetProcAddress(cocos, name)) : nullptr;
    };
    g_sharedApplication = reinterpret_cast<SharedFn>(get("?sharedApplication@CCApplication@cocos2d@@SAPEAV12@XZ"));
    g_sharedView = reinterpret_cast<SharedFn>(get("?sharedOpenGLView@CCEGLView@cocos2d@@SAPEAV12@XZ"));
    g_getVsync = reinterpret_cast<GetBoolFn>(get("?getVerticalSyncEnabled@CCApplication@cocos2d@@QEBA_NXZ"));
    g_toggleVsync = reinterpret_cast<SetBoolFn>(get("?toggleVerticalSync@CCApplication@cocos2d@@QEAAX_N@Z"));
    g_isFullscreen = reinterpret_cast<GetBoolFn>(get("?getIsFullscreen@CCEGLView@cocos2d@@QEBA_NXZ"));
    g_isBorderless = reinterpret_cast<GetBoolFn>(get("?getIsBorderless@CCEGLView@cocos2d@@QEBA_NXZ"));
    g_borderlessFix = reinterpret_cast<GetBoolFn>(get("?getUseBorderlessFix@CCEGLView@cocos2d@@QEBA_NXZ"));
    g_toggleFullScreen = reinterpret_cast<ToggleFullScreenFn>(get("?toggleFullScreen@CCEGLView@cocos2d@@QEAAX_N00@Z"));
    m_available = g_sharedApplication && g_sharedView && g_getVsync && g_toggleVsync && g_isFullscreen &&
                  g_isBorderless && g_borderlessFix && g_toggleFullScreen;
    core::logMsg("Display: %s", m_available ? "listo" : "no disponible (exports de libcocos2d)");
}

void Display::update() {
    const int pending = m_pending;
    m_pending = None;
    if (!m_available || pending == None) return;
    void* app = g_sharedApplication();
    void* view = g_sharedView();
    if (!app || !view) return;
    const bool fullscreen = g_isFullscreen(view);
    const bool borderless = g_isBorderless(view);
    const bool fix = g_borderlessFix(view);
    switch (pending) {
    case Vsync:
        g_toggleVsync(app, !g_getVsync(app));
        break;
    case Fullscreen:
        g_toggleFullScreen(view, !fullscreen, borderless, fix);
        break;
    case Borderless: // "sin bordes" es un modo de pantalla completa: se aplica en pantalla completa
        g_toggleFullScreen(view, true, !borderless, fix);
        break;
    }
    core::logMsg("Display: cambio %d aplicado", pending);
}

void Display::drawUI() {
    void* app = m_available ? g_sharedApplication() : nullptr;
    void* view = m_available ? g_sharedView() : nullptr;
    const bool ready = app && view;
    bool vsync = ready && g_getVsync(app);
    bool fullscreen = ready && g_isFullscreen(view);
    bool borderless = ready && g_isBorderless(view);
    // Se aplica en Display::update (siguiente frame), no en mitad del dibujado del panel
    if (gui::toggle("Vertical sync", ready, vsync)) m_pending = Vsync;
    if (gui::toggle("Fullscreen", ready, fullscreen)) m_pending = Fullscreen;
    if (gui::toggle("Borderless", ready, borderless)) m_pending = Borderless;
    if (ready) ImGui::TextDisabled("Solo esta sesión: para guardarlo,\nusa los ajustes gráficos del juego.");
}

} // namespace features
