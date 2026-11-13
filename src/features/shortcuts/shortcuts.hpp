#pragma once

namespace features {

// Shortcuts: botones para acciones del nivel (reiniciar, modo práctica, checkpoints).
// Como cualquier opción del panel, cada botón admite una tecla con clic derecho.
// Todas llaman a funciones del propio juego.
// Utility: P1/P2 jump y P1 izquierda/derecha son pulsaciones del jugador mientras se mantiene su
// tecla (GJBaseGameLayer::handleButton, la misma vía que el bot). Con clic hacen una pulsación corta.
// También abren las carpetas Resources y de datos del juego (AppData).
// Uncomplete level pone a 0 los porcentajes guardados del nivel que se está jugando.
class Shortcuts {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void update();                 // una vez por frame: teclas mantenidas de Utility
    void drawUI();

private:
    bool m_restart = false;     // reiniciar nivel
    bool m_practice = false;    // alternar modo práctica
    bool m_checkpoints = false; // poner y quitar checkpoint
    bool m_buttons = false;     // pulsaciones de jugador (handleButton)
    bool m_uncomplete = false;  // poner a 0 los porcentajes del nivel
};

} // namespace features
