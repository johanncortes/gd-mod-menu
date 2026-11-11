#pragma once

namespace features {

// Instant complete: botón que termina el nivel actual llamando a PlayLayer::levelComplete().
// No toca las comprobaciones anti-trampas del juego: lo que el juego decida sobre ese
// completado (contarlo o no) sigue igual.
class InstantComplete {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void drawUI();

private:
    bool m_available = false;
};

} // namespace features
