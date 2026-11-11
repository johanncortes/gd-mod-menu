#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// FPS bypass: fija el límite de FPS del juego a cualquier valor, a través de
// cocos2d::CCApplication::setAnimationInterval. Solo tiene efecto con el VSync apagado.
class FpsBypass {
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
    float m_fps = 240.f;
};

} // namespace features
