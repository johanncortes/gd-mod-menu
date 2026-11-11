#include "input.hpp"

#include <atomic>
#include <bitset>

#include <MinHook.h>

#include "core/log.hpp"

namespace core::input {

namespace {

// Los lee también el hilo de raw input del juego, por eso son atómicos
std::atomic<bool> g_wantMouse{false};
std::atomic<bool> g_wantKeys{false};

// Regla común a los dos caminos: si ImGui se queda el "down" de un botón o tecla,
// también se queda su "up" (GD activa sus botones con solo recibir el "up").
// Si el "down" llegó al juego, su "up" siempre pasa, para no dejar nada pegado.
struct HeldState {
    bool buttons[5] = {};
    std::bitset<256> keysDown; // teclas pulsadas ahora mismo
    std::bitset<256> keysHeld; // de esas, las que se quedó ImGui

    bool buttonDown(int b) { return buttons[b] = g_wantMouse; }
    bool buttonUp(int b) {
        const bool held = buttons[b];
        buttons[b] = false;
        return held;
    }
    bool keyDown(size_t vk) {
        // En la auto-repetición se mantiene lo decidido en la primera pulsación
        if (!keysDown[vk]) {
            keysDown[vk] = true;
            keysHeld[vk] = g_wantKeys;
        }
        return keysHeld[vk];
    }
    bool keyUp(size_t vk) {
        const bool held = keysHeld[vk];
        keysDown[vk] = keysHeld[vk] = false;
        return held;
    }
};

HeldState g_window; // camino 1: mensajes de ventana (hilo principal)
HeldState g_raw;    // camino 2: raw input (hilo de input del juego)

// --- Contador de clics (label de CPS). Lo escribe el hilo de raw input y lo lee el de render ---
constexpr size_t kClickHistory = 64;
std::atomic<ULONGLONG> g_clickTimes[kClickHistory];
std::atomic<size_t> g_clickNext{0};
std::atomic<int> g_jumpsHeld{0};

void jumpPressed() {
    g_clickTimes[g_clickNext++ % kClickHistory] = GetTickCount64();
    ++g_jumpsHeld;
}

void jumpReleased() {
    if (--g_jumpsHeld < 0) g_jumpsHeld = 0;
}

bool isJumpKey(size_t vk) {
    return vk == VK_SPACE || vk == VK_UP || vk == 'W';
}

using GetRawInputDataFn = UINT(WINAPI*)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);
GetRawInputDataFn g_origGetRawInputData = nullptr;

// Borra del evento lo que se queda ImGui, antes de que lo vea el juego
void filterRawMouse(RAWMOUSE& mouse) {
    USHORT& flags = mouse.usButtonFlags;
    for (int b = 0; b < 5; ++b) {
        // RI_MOUSE_BUTTON_n_DOWN / _UP son pares de bits consecutivos
        const USHORT down = static_cast<USHORT>(1 << (2 * b));
        const USHORT up = static_cast<USHORT>(down << 1);
        if (flags & down) {
            if (g_raw.buttonDown(b)) flags &= ~down;
            else if (b == 0) jumpPressed();
        }
        if (flags & up) {
            if (g_raw.buttonUp(b)) flags &= ~up;
            else if (b == 0) jumpReleased();
        }
    }
    const USHORT wheel = RI_MOUSE_WHEEL | RI_MOUSE_HWHEEL;
    if ((flags & wheel) && g_wantMouse) {
        flags &= ~wheel;
        mouse.usButtonData = 0;
    }
}

void filterRawKeyboard(RAWKEYBOARD& kb) {
    const size_t vk = kb.VKey & 0xFF;
    const bool isUp = kb.Flags & RI_KEY_BREAK;
    const bool repeat = !isUp && g_raw.keysDown[vk]; // auto-repetición de una tecla mantenida
    const bool wasDown = g_raw.keysDown[vk];
    if (isUp ? g_raw.keyUp(vk) : g_raw.keyDown(vk)) {
        // No se puede quitar el evento: se convierte en "ninguna tecla"
        kb.VKey = 0xFF;
        kb.MakeCode = 0;
        kb.Message = WM_NULL;
    } else if (isJumpKey(vk)) {
        if (!isUp && !repeat) jumpPressed();
        else if (isUp && wasDown) jumpReleased();
    }
}

UINT WINAPI getRawInputDataHook(HRAWINPUT handle, UINT command, LPVOID data, PUINT size, UINT headerSize) {
    const UINT ret = g_origGetRawInputData(handle, command, data, size, headerSize);
    if (command != RID_INPUT || !data || ret == static_cast<UINT>(-1) || ret < sizeof(RAWINPUTHEADER))
        return ret;

    auto* raw = static_cast<RAWINPUT*>(data);
    if (raw->header.dwType == RIM_TYPEMOUSE)
        filterRawMouse(raw->data.mouse);
    else if (raw->header.dwType == RIM_TYPEKEYBOARD)
        filterRawKeyboard(raw->data.keyboard);
    return ret;
}

// Índice (0-4) del botón de un mensaje de ratón
int mouseButtonIndex(UINT msg, WPARAM wp) {
    switch (msg) {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_LBUTTONUP: return 0;
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: case WM_RBUTTONUP: return 1;
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: case WM_MBUTTONUP: return 2;
    default: return GET_XBUTTON_WPARAM(wp) == XBUTTON1 ? 3 : 4;
    }
}

} // namespace

void setCapture(bool mouse, bool keyboard) {
    g_wantMouse = mouse;
    g_wantKeys = keyboard;
}

bool swallowWindowMessage(UINT msg, WPARAM wp, LPARAM) {
    switch (msg) {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        return g_window.buttonDown(mouseButtonIndex(msg, wp));
    case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP: case WM_XBUTTONUP:
        return g_window.buttonUp(mouseButtonIndex(msg, wp));
    case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
        return g_wantMouse;
    case WM_KEYDOWN:
        return g_window.keyDown(wp & 0xFF);
    case WM_KEYUP:
        return g_window.keyUp(wp & 0xFF);
    case WM_CHAR:
        return g_wantKeys;
    default:
        return false;
    }
}

int clicksPerSecond() {
    const ULONGLONG now = GetTickCount64();
    int count = 0;
    for (const auto& time : g_clickTimes) {
        const ULONGLONG t = time;
        if (t != 0 && now - t < 1000) ++count;
    }
    return count;
}

bool jumpHeld() { return g_jumpsHeld > 0; }

bool installRawInputHook() {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    void* target = user32 ? reinterpret_cast<void*>(GetProcAddress(user32, "GetRawInputData")) : nullptr;
    if (!target ||
        MH_CreateHook(target, &getRawInputDataHook, reinterpret_cast<void**>(&g_origGetRawInputData)) != MH_OK ||
        MH_EnableHook(target) != MH_OK) {
        logMsg("No se pudo hookear GetRawInputData: los clics sobre el panel llegarán al juego");
        return false;
    }
    logMsg("Hook de GetRawInputData instalado");
    return true;
}

} // namespace core::input
