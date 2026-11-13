#pragma once
#include <nlohmann/json_fwd.hpp>

#include "core/memory.hpp"

namespace features {

// Random seed: los triggers aleatorios (random, advanced random, spawn con retardo aleatorio...)
// salen igual en todos los intentos. El juego elige la semilla en PlayLayer::resetLevel: usa la
// hora salvo que tenga una guardada; el parche hace que use siempre la guardada y antes de cada
// intento se escribe ahí la elegida. Trampa.
class RandomSeed {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const { return m_enabled && m_available; }

private:
    bool m_available = false;
    bool m_enabled = false;
    float m_seed = 1.f; // entero; se guarda como float por el campo numérico del panel
    core::memory::Patch m_patch;
};

// Stop triggers on death: mientras el jugador está muerto el nivel se congela (triggers, objetos
// en movimiento, cámara); el efecto de muerte y la reaparición siguen. Filtro de
// GJBaseGameLayer::update: no se llama mientras PlayLayer tenga marcado "jugador muerto".
class StopOnDeath {
public:
    void init(bool gameAvailable); // antes que Hitboxes::init (comparten firma)
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
