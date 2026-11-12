#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Hide player: los iconos de los jugadores no se ven (siguen jugando igual). Tras cada frame del
// nivel se ocultan con CCNode::setVisible(false); al desactivar se vuelven a mostrar.
class HidePlayer {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
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
