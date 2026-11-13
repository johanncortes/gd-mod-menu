#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// No mirror: los portales espejo no voltean la pantalla. Hook a GJBaseGameLayer::toggleFlipped
// que pasa siempre "sin voltear"; así un portal de vuelta también deshace un volteo anterior.
// Instant mirror: el volteo se hace de golpe, sin la animación (el mismo hook pide "sin efectos").
class NoMirror {
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
    bool m_instant = false;
};

} // namespace features
