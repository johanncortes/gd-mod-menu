#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Speedhack: multiplica el tiempo que recibe CCScheduler::update (todo el juego va más
// rápido o más lento). Opcionalmente ajusta también el tono/velocidad del audio (FMOD).
// Pitch shift: cambia solo el tono de todo el audio (sin cambiar el tempo) con un DSP
// "pitch shifter" de FMOD en el grupo maestro. Va aparte del speedhack.
class Speedhack {
public:
    void init(); // instala los hooks; si faltan los exports queda "no disponible"
    void enable();
    void disable();
    void shutdown(); // disable + quitar el DSP de pitch shift (al descargar la DLL)
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const { return m_enabled && m_available; }

private:
    void apply();

    bool m_available = false;      // hook de CCScheduler::update
    bool m_audioAvailable = false; // hooks de FMOD
    bool m_shiftAvailable = false; // exports de DSP de FMOD 2.x
    bool m_enabled = false;
    bool m_audio = true;
    float m_speed = 1.f;
    bool m_shift = false;
    float m_shiftPitch = 1.f;
};

// Pausa (o reanuda) todo el audio del juego: grupo maestro de FMOD. Lo usa el frame stepper.
// Se aplica en el siguiente FMOD::System::update.
void setGameAudioPaused(bool paused);

// El hook de CCScheduler::update del speedhack, para otras opciones (features/global/game_time.hpp).
bool timeHookAvailable();           // false: las dos funciones de abajo no hacen nada
void setTimeBoost(float factor);    // multiplica el tiempo del juego además del speedhack (1 = nada)
void setLockedDelta(float seconds); // cada frame dura esto para el juego; 0 = lo que dure de verdad

} // namespace features
