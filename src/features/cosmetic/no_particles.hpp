#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// No particles: ningún sistema de partículas crea partículas nuevas (las que ya hay se apagan
// solas). Hook al export CCParticleSystem::addParticle de libcocos2d, que devuelve "lleno".
class NoParticles {
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
