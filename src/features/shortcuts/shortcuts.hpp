#pragma once

namespace features {

// Shortcuts: botones para acciones del nivel (reiniciar, modo práctica, checkpoints).
// Como cualquier opción del panel, cada botón admite una tecla con clic derecho.
// Todas llaman a funciones del propio juego.
class Shortcuts {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void drawUI();

private:
    bool m_restart = false;     // reiniciar nivel
    bool m_practice = false;    // alternar modo práctica
    bool m_checkpoints = false; // poner y quitar checkpoint
};

} // namespace features
