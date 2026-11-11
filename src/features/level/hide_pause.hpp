#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Hide pause menu: el menú de pausa se crea pero no se dibuja (el juego sigue en pausa y
// Esc o Espacio lo cierran igual). Hook a PauseLayer::customSetup + CCNode::setVisible(false).
class HidePause {
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
