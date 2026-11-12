#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Auto clicker: mientras se juega un nivel, el jugador 1 pulsa y suelta el salto solo, N veces
// por segundo (la mitad del tiempo pulsado). Usa GJBaseGameLayer::handleButton, como el bot.
// Como mucho una pulsación o suelta por frame del juego. Trampa.
class AutoClicker {
public:
    void init(bool gameAvailable); // antes que el bot (que hookea handleButton)
    void enable();
    void disable();
    void update(); // una vez por frame
    bool drawUI(); // true si el usuario cambió algo

    bool active() const { return m_enabled && m_available; }

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    void release(); // suelta el botón si lo tenemos pulsado

    bool m_available = false;
    bool m_enabled = false;
    float m_cps = 10.f;
    bool m_down = false;          // enviamos "pulsado" al juego
    long long m_nextToggle = 0;   // QueryPerformanceCounter del próximo cambio
};

} // namespace features
