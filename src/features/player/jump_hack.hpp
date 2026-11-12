#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Jump hack: saltar en el aire. Mientras el jugador 1 mantiene el salto en cubo, robot o araña,
// tras cada frame del nivel se le marca "en el suelo", así que el juego vuelve a saltar (al
// mantener, sube a saltos seguidos). No toca nave, bola, UFO, onda ni swing. Trampa.
class JumpHack {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    bool active() const { return m_enabled && m_available; }

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_available = false;
    bool m_enabled = false;
};

} // namespace features
