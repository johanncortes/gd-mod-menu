#include "hooks.hpp"

#include <windows.h>

#include <atomic>

#include <MinHook.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>

#include "core/log.hpp"
#include "gui/menu.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace core::hooks {

namespace {

using SwapBuffersFn = BOOL(WINAPI*)(HDC);

void* g_swapTarget = nullptr;
SwapBuffersFn g_origSwap = nullptr;

HWND g_hwnd = nullptr;
WNDPROC g_origWndProc = nullptr;
bool g_imguiReady = false; // solo se toca desde el hilo de render

std::atomic<bool> g_unloadRequested{false};
std::atomic<bool> g_renderDown{false};

// ¿Este mensaje es un input que ImGui quiere para sí?
// Los "up" siempre pasan al juego para que no se queden teclas/botones pegados.
bool imguiWantsMessage(UINT msg) {
    const ImGuiIO& io = ImGui::GetIO();
    switch (msg) {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
    case WM_MOUSEWHEEL:  case WM_MOUSEHWHEEL:
        return io.WantCaptureMouse;
    case WM_KEYDOWN:
    case WM_CHAR:
        return io.WantCaptureKeyboard;
    default:
        return false;
    }
}

LRESULT CALLBACK wndProcHook(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    const bool firstPress = msg == WM_KEYDOWN && !(lp & (1 << 30)); // sin auto-repetición
    if (firstPress && wp == VK_TAB) {
        gui::toggleMenu();
        return 0;
    }
#ifdef GDMENU_DEV
    if (firstPress && wp == VK_END) {
        g_unloadRequested = true;
        return 0;
    }
#endif

    if (g_imguiReady) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
        if (gui::isMenuOpen() && imguiWantsMessage(msg))
            return 0;
    }
    return CallWindowProcW(g_origWndProc, hwnd, msg, wp, lp);
}

void initImGui(HWND hwnd) {
    if (!ImGui::GetCurrentContext()) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr; // la posición de ventanas se guardará con la config (fase 6)
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange; // el cursor del SO lo maneja el juego
        ImGui::StyleColorsDark();
    }

    g_hwnd = hwnd;
    g_origWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndProcHook)));

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplOpenGL3_Init();
    g_imguiReady = true;
    logMsg("ImGui iniciado (hwnd=%p)", static_cast<void*>(hwnd));
}

// Cierra los backends y devuelve el WndProc original. El contexto de ImGui se conserva.
void shutdownBackends() {
    g_imguiReady = false;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    if (IsWindow(g_hwnd) && g_origWndProc)
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origWndProc));
    g_hwnd = nullptr;
    g_origWndProc = nullptr;
}

BOOL WINAPI swapBuffersHook(HDC hdc) {
    if (g_unloadRequested) {
        if (!g_renderDown) {
            if (g_imguiReady) shutdownBackends();
            if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
            g_renderDown = true;
        }
        return g_origSwap(hdc);
    }

    // El juego recrea la ventana al cambiar a pantalla completa: reiniciar los backends
    HWND hwnd = WindowFromDC(hdc);
    if (hwnd && hwnd != g_hwnd) {
        if (g_imguiReady) shutdownBackends();
        initImGui(hwnd);
    }

    if (g_imguiReady) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        gui::draw();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    return g_origSwap(hdc);
}

} // namespace

bool install() {
    // opengl32.dll puede no estar cargada todavía si nos inyectan muy pronto
    HMODULE gl = nullptr;
    for (int i = 0; i < 200 && !(gl = GetModuleHandleW(L"opengl32.dll")); ++i)
        Sleep(50);
    if (!gl) {
        logMsg("opengl32.dll no se cargó: menú desactivado");
        return false;
    }

    g_swapTarget = reinterpret_cast<void*>(GetProcAddress(gl, "wglSwapBuffers"));
    if (!g_swapTarget) {
        logMsg("wglSwapBuffers no encontrado: menú desactivado");
        return false;
    }

    if (MH_Initialize() != MH_OK) {
        logMsg("MH_Initialize falló");
        return false;
    }
    if (MH_CreateHook(g_swapTarget, &swapBuffersHook, reinterpret_cast<void**>(&g_origSwap)) != MH_OK ||
        MH_EnableHook(g_swapTarget) != MH_OK) {
        logMsg("No se pudo hookear wglSwapBuffers");
        MH_Uninitialize();
        return false;
    }

    logMsg("Hook de wglSwapBuffers instalado");
    return true;
}

bool readyToUnload() {
    return g_renderDown;
}

void uninstall() {
    MH_DisableHook(MH_ALL_HOOKS);
    Sleep(100); // dejar que salga cualquier llamada que siga dentro de un hook
    MH_Uninitialize();
}

} // namespace core::hooks
