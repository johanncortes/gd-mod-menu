#pragma once
#include <cstdint>

#include <nlohmann/json_fwd.hpp>

namespace features {

// Respawn delay: cambia el tiempo entre morir y reaparecer. PlayLayer::destroyPlayer programa
// el reinicio con un CCDelayTime; se hookea CCDelayTime::create (export de libcocos2d) y se
// cambia la duración solo cuando la llamada viene de destroyPlayer.
class RespawnDelay {
public:
    // destroyPlayer: inicio de PlayLayer::destroyPlayer (nullptr si no se encontró la firma)
    void init(uint8_t* destroyPlayer);
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_available = false;
    bool m_enabled = false;
    float m_delay = 0.5f; // segundos
};

} // namespace features
