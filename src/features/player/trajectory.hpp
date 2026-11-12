#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Show trajectory: dibuja por dónde irá el jugador si mantiene pulsado (verde) o si suelta (rojo).
// Un PlayerObject propio e invisible copia el estado del jugador real (copyAttributes) y avanza
// tick a tick con las funciones del propio juego: update, checkCollisions y updateRotation.
// Para no tocar la partida real, al jugador simulado solo le llegan sólidos, peligros y rampas:
// portales, anillos, pads, monedas y triggers no le afectan (la predicción no los sigue).
class Trajectory {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo
    void drawOverlay();

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_available = false;
    bool m_enabled = false;
};

} // namespace features
