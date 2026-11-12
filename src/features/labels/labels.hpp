#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Labels: textos informativos mientras se juega un nivel (FPS, CPS, porcentaje...), con la
// fuente del propio GD. Cada uno se activa por separado; el bloque se mueve arrastrándolo con
// el panel abierto. En la mitad derecha de la pantalla las líneas se alinean a la derecha.
class Labels {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    bool drawUI();                 // true si el usuario cambió algo
    void drawOverlay(bool cheatsActive);
    bool takePositionChanged();    // true (una vez) si se movió el bloque: hay que guardar

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    bool m_levelInfo = false; // porcentaje e intentos (necesitan funciones del juego)
    bool m_time = false;      // tiempo del intento (necesita los hooks de nivel)

    bool m_show = true;
    bool m_showCheat = true;
    bool m_showFps = true;
    bool m_showCps = false;
    bool m_showNoclipDeaths = true;   // solo salen con el noclip activo
    bool m_showNoclipAccuracy = true;
    bool m_showPercent = false;
    bool m_showAttempt = false;
    bool m_showTime = false;
    bool m_showClock = false;

    // Posición: borde de anclaje en fracción de la pantalla (0..1). Con m_alignRight el
    // borde es el derecho del bloque; si no, el izquierdo. y es siempre el borde de arriba.
    float m_x = 0.01f;
    float m_y = 0.015f;
    bool m_alignRight = false;
    bool m_positionChanged = false;
};

} // namespace features
