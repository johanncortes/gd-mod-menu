#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Speedhack: multiplica el tiempo que recibe CCScheduler::update (todo el juego va más
// rápido o más lento). Opcionalmente ajusta también el tono/velocidad del audio (FMOD).
class Speedhack {
public:
    void init(); // instala los hooks; si faltan los exports queda "no disponible"
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const { return m_enabled && m_available; }

private:
    void apply();

    bool m_available = false;      // hook de CCScheduler::update
    bool m_audioAvailable = false; // hooks de FMOD
    bool m_enabled = false;
    bool m_audio = true;
    float m_speed = 1.f;
};

} // namespace features
