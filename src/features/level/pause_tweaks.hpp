#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Confirm restart / Confirm practice mode: los botones del menú de pausa que hacen perder el
// intento (reiniciar, reinicio completo) o cambiar de modo (entrar y salir de práctica) piden una
// segunda pulsación en pocos segundos. Hooks de los manejadores de PauseLayer: la primera
// pulsación no llega al juego y sale un aviso en pantalla. (Confirmar al salir ya es un ajuste
// del propio juego.)
// Pause countdown: al reanudar desde la pausa el nivel sigue congelado y en silencio durante
// una cuenta atrás de 3 segundos. Hook de PlayLayer::resume + filtro de GJBaseGameLayer::update.
class PauseTweaks {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void disable();                // apaga las tres opciones (al descargar la DLL)
    void update();                 // una vez por frame: fin de la cuenta atrás
    bool drawUI();                 // true si el usuario cambió algo
    void drawOverlay();            // aviso de confirmación y números de la cuenta atrás

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

private:
    void apply();

    bool m_restartAvailable = false;
    bool m_practiceAvailable = false;
    bool m_countdownAvailable = false;
    bool m_confirmRestart = false;
    bool m_confirmPractice = false;
    bool m_countdown = false;
};

} // namespace features
