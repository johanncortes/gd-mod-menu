#pragma once

namespace features {

// Frame stepper: congela el nivel y lo deja avanzar de un tick de física en un tick (1/240 s)
// con el botón "Next frame" (o su tecla; mantenerla avanza seguido). Mientras está activo, el
// audio del juego se pausa. Filtro en GJBaseGameLayer::update (core::game::setUpdateFilter). Trampa.
class FrameStepper {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void enable();
    void disable();
    void update();  // una vez por frame: tecla mantenida
    bool drawUI();  // true si el usuario cambió algo

    bool active() const { return m_enabled && m_available; }
    // No se guarda en config.json: al abrir el juego el nivel no debe empezar congelado

private:
    bool m_available = false;
    bool m_enabled = false;
};

} // namespace features
