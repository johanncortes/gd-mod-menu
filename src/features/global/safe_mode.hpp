#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Safe mode: el intento no guarda progreso (ni nuevo récord al morir, ni completado, monedas o
// estrellas al terminar). Usa las dos banderas con las que el propio juego descarta un intento
// (las que pone al usar herramientas de práctica o cambiar la velocidad): se ponen a 1 tras cada
// frame del nivel. El juego las recalcula al reiniciar el nivel.
// Auto safe mode: lo mismo, pero solo mientras haya alguna trampa activa.
class SafeMode {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void disable();
    void update(bool cheatsActive); // una vez por frame, desde features::update
    bool drawUI();                  // true si el usuario cambió algo

    bool active() const; // ahora mismo el intento no guarda progreso

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_available = false;
    bool m_safe = false;
    bool m_auto = true;
};

} // namespace features
