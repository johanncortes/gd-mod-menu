#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Dos opciones que tocan el tiempo que recibe el juego, sobre el hook de CCScheduler::update
// del speedhack (features/global/speedhack.hpp).

// Fast chests: al abrir un cofre, la animación de las recompensas va kBoost veces más rápida.
// Solo desde que el cofre se abre (RewardUnlockLayer::step2) hasta que sale el botón de cerrar
// (showCloseButton): antes no, porque algunos cofres esperan ahí la respuesta del servidor.
class FastChests {
public:
    void init(); // después de Speedhack::init
    void enable();
    void disable();
    void update(); // una vez por frame: corta la aceleración si el final no llegó
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_available = false;
    bool m_enabled = false;
};

// Lock delta: cada frame avanza el juego exactamente 1/FPS segundos, tarde lo que tarde en
// dibujarse. Si el equipo no llega a esos FPS el juego va más lento en vez de dar saltos (por eso
// cuenta como trampa); si los supera, va más rápido: hay que poner los FPS reales.
class LockDelta {
public:
    void init(); // después de Speedhack::init
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const { return m_enabled && m_available; }

private:
    bool m_available = false;
    bool m_enabled = false;
    float m_fps = 240.f;
};

} // namespace features
