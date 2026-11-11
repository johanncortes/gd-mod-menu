#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// StartPos switcher: cambia de StartPos sin abrir el menú de pausa.
// Q = anterior, E = siguiente; la posición 0 es el inicio del nivel.
// Repite lo que hace el juego al elegir un StartPos en el menú de pausa
// (PlayLayer::switchToStartPosObject), con funciones y offsets leídos de kSwitchStartPos.
class StartPosSwitcher {
public:
    void init();
    void enable();
    void disable();
    void update();      // una vez por frame: mira las teclas
    bool drawUI();      // true si el usuario cambió algo
    void drawOverlay(); // "StartPos 2/5" durante un momento tras cambiar

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    void step(int direction);

    bool m_available = false;
    bool m_enabled = false;
    bool m_prevDown = false; // estado de las teclas en el frame anterior
    bool m_nextDown = false;
};

} // namespace features
