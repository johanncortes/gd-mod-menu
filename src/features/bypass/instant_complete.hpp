#pragma once

namespace features {

// Instant complete: botón que termina el nivel actual.
// En niveles normales lleva al jugador hasta el portal final y deja que el juego haga lo de
// siempre al llegar (PlayLayer::checkForEnd): animación de fin de nivel y levelComplete().
// En niveles de plataformas (sin portal) llama directamente a PlayLayer::levelComplete().
// No toca las comprobaciones anti-trampas del juego.
class InstantComplete {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void drawUI();

private:
    bool m_available = false;
};

} // namespace features
