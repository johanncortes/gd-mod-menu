#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Smart StartPos: al entrar a un nivel, ajusta cada StartPos al estado que tendría el jugador en
// ese punto (modo de juego, velocidad, mini, dual, gravedad y espejo), mirando los portales que
// quedan a su izquierda. No distingue caminos por altura ni portales que se mueven o se
// desactivan con triggers, y no actúa en niveles de plataformas.
// StartPos reset camera: activa en todos los StartPos su opción "Reset camera".
// Las dos se aplican al entrar al nivel.
class SmartStartPos {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_available = false;
    bool m_cameraAvailable = false;
    bool m_enabled = false;
    bool m_resetCamera = false;
};

} // namespace features
