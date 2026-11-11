#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Labels: textos informativos sobre el juego mientras se juega un nivel, apilados en la
// esquina superior izquierda. Cada uno se activa por separado.
class Labels {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    bool drawUI();                 // true si el usuario cambió algo
    void drawOverlay(bool cheatsActive);
    bool takePositionChanged();

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_levelInfo = false; // porcentaje e intentos (necesitan funciones del juego)
    bool m_time = false;      // tiempo del intento (necesita los hooks de nivel)

    bool m_show = true;
    bool m_showCheat = true;
    bool m_showFps = true;
    bool m_showCps = false;
    bool m_showPercent = false;
    bool m_showAttempt = false;
    bool m_showTime = false;
    bool m_showClock = false;
    bool m_showBackground = false;
    bool m_usePusab = true;

    float m_scale = 1.0f;
    float m_outlineWidth = 1.8f;
    float m_lineSpacing = 3.0f;

    float m_posX = 10.f;
    float m_posY = 10.f;
    bool m_resetPos = false;
    bool m_posChanged = false;
};

} // namespace features
