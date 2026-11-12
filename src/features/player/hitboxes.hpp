#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Mostrar hitboxes siempre, usando el dibujado de hitboxes del propio juego (el de la opción
// "Show Hitboxes" del modo práctica). Hook a la función de PlayLayer que decide si se dibujan.
// "Hitboxes al morir": solo mientras el jugador está muerto, en cualquier modo.
class Hitboxes {
public:
    void init();
    void enable();
    void disable();
    void setOnDeath(bool on);
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_available = false;
    bool m_onDeathAvailable = false;
    bool m_enabled = false;
    bool m_onDeath = false;
};

} // namespace features
