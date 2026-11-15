#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Click teleport: con el panel cerrado, un clic derecho dentro de un nivel lleva al jugador 1 al
// punto del nivel que hay bajo el cursor. Usa CCNode::convertToNodeSpace (export de libcocos2d)
// sobre la capa que contiene al jugador y su setPosition virtual. Trampa.
class ClickTeleport {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void enable();
    void disable();
    void update(); // una vez por frame, en el hilo del juego
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const { return m_enabled && m_available; }

private:
    bool m_available = false;
    bool m_enabled = false;
    bool m_wasDown = false; // botón derecho pulsado en el frame anterior
};

} // namespace features
