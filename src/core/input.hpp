#pragma once
#include <windows.h>

// Filtro de input: evita que el juego reciba los clics y teclas que se queda ImGui.
// GD lee el input por dos caminos y hay que cubrir los dos:
//   1. mensajes de ventana (WM_LBUTTONDOWN, WM_KEYDOWN...) -> se filtran en el WndProc
//   2. raw input (GetRawInputData), que es por donde le llegan los clics -> hook aparte
namespace core::input {

// Qué quiere capturar ImGui ahora mismo. Se llama cada frame desde el hilo de render.
void setCapture(bool mouse, bool keyboard);

// true si el mensaje de ventana se lo queda ImGui y no debe llegar al juego.
bool swallowWindowMessage(UINT msg, WPARAM wp, LPARAM lp);

// Hookea GetRawInputData. Requiere MinHook ya inicializado.
// Si falla, el menú funciona igual pero los clics sobre el panel también llegan al juego.
bool installRawInputHook();

// Para el label de CPS: pulsaciones de salto (clic izquierdo, Espacio, Flecha arriba, W)
// que llegaron al juego en el último segundo, y si alguna sigue pulsada.
int clicksPerSecond();
bool jumpHeld();

} // namespace core::input
