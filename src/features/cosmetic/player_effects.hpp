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

} // namespace features
