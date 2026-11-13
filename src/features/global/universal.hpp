#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Lock cursor: mientras se juega un nivel (sin pausa y con el panel cerrado) el cursor no puede
// salir de la ventana del juego (ClipCursor). Útil con varias pantallas.
class LockCursor {
public:
    void enable();
    void disable();
    void update(bool menuOpen); // una vez por frame
    bool drawUI();              // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_enabled = false;
    bool m_clipped = false; // hay un ClipCursor nuestro puesto
};

// No transition: las transiciones entre pantallas (fundido) duran 0 s. Hook al export
// CCTransitionFade::create(float, CCScene*) de libcocos2d.
// Transition speed: con el mismo hook, las transiciones van N veces más rápidas (o lentas).
class NoTransition {
public:
    void init();
    void enable();
    void disable();
    bool drawUI();

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_available = false;
    bool m_enabled = false;
    bool m_speedOn = false;
    float m_speed = 2.f;
};

// High priority: el proceso del juego pasa a prioridad alta (como en el Administrador de tareas).
// Al desactivar vuelve la prioridad que tenía.
class HighPriority {
public:
    void enable();
    void disable();
    bool drawUI();

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_enabled = false;
    unsigned long m_previous = 0; // clase de prioridad antes de activarlo (0 = sin guardar)
};

} // namespace features
