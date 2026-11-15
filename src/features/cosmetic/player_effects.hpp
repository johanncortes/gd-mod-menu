#pragma once
#include <nlohmann/json_fwd.hpp>

#include "core/memory.hpp"

namespace features {

// No robot & swing fire: no se dibuja el fuego del robot al saltar ni los del swing. Los dos son
// sprites de la clase PlayerFireBoostSprite: se cambia en su vtable la entrada de visit() por una
// función vacía (parche de 8 bytes, reversible). Solo se aplica si esa entrada es la que se espera
// (CCNode::visit).
class NoFire {
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
    core::memory::Patch m_patch;
};

// Trail always on: la estela del icono no se corta al tocar el suelo. Hook del export
// CCMotionStreak::stopStroke de libcocos2d, que deja de hacer nada.
class TrailAlwaysOn {
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

// Wave trail size: grosor fijo para la estela de la onda, en vez del pulso de la música. Se quitan
// (nop) las dos instrucciones de PlayLayer::updateVisibility que guardan el pulso en los jugadores
// y el panel escribe ese campo cada frame.
class WaveTrailSize {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void enable();
    void disable();
    void update(); // una vez por frame
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_available = false;
    bool m_enabled = false;
    float m_size = 1.f;
    int m_field = 0; // offset del pulso en PlayerObject
    core::memory::Patch m_store1, m_store2;
};

} // namespace features
