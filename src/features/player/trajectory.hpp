#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Show trajectory: predice y dibuja la trayectoria de salto y movimiento del jugador
// en tiempo real usando un CCDrawNode en el espacio de coordenadas del juego.
class Trajectory {
public:
    void init();
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_available = false;
    bool m_enabled = false;
};

} // namespace features

