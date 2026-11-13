#pragma once
#include <cstdint>

#include <nlohmann/json_fwd.hpp>

namespace features {

// Noclip: el jugador no muere. Hook a PlayLayer::destroyPlayer que ignora la llamada,
// salvo la del pincho anti-trampas del juego (esa tiene que seguir llegando).
// Cuenta las "muertes" evitadas y la precisión (frames sin tocar nada / frames totales).
// Límites (opcionales): con N muertes evitadas, o con la precisión por debajo de un mínimo, el
// siguiente golpe mata de verdad.
// Auto kill (va aquí porque usa el mismo PlayLayer::destroyPlayer): al llegar a un porcentaje el
// jugador muere, una vez por intento, esté o no el noclip activo.
class Noclip {
public:
    // destroyPlayer: inicio de PlayLayer::destroyPlayer (nullptr si no se encontró la firma)
    void init(uint8_t* destroyPlayer);
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const { return m_enabled && m_available; }

private:
    bool m_available = false;
    bool m_statsAvailable = false; // hooks de nivel de core::game (contador y precisión)
    bool m_enabled = false;

    void applyLimits();
    bool m_deathLimitOn = false;
    float m_deathLimit = 10.f; // entero; float por el campo numérico del panel
    bool m_accuracyOn = false;
    float m_minAccuracy = 90.f; // %
    bool m_autoKillOn = false;
    float m_autoKill = 50.f; // %
};

// Estadísticas del intento actual, para los labels "Noclip deaths" y "Noclip accuracy".
// false si el noclip no está activo, si no hay hooks de nivel o si el nivel no ha empezado.
struct NoclipStats {
    int deaths = 0;
    float accuracy = 100.f; // % de frames sin tocar nada
    bool hitting = false;   // está atravesando algo ahora mismo
};
bool noclipStats(NoclipStats& out);

} // namespace features
