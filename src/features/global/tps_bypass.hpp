#pragma once
#include <nlohmann/json_fwd.hpp>

#include "core/memory.hpp"

namespace features {

// Physics bypass: cambia los ticks de física por segundo (TPS; el juego usa 240 fijos).
// El juego calcula los pasos con dos constantes (1/240 y 4.0 = 240/60). No se tocan las
// constantes, que comparte otro código: se redirigen las dos instrucciones que las leen
// hacia una copia propia con el valor elegido.
class TpsBypass {
public:
    void init(); // busca las firmas; si falla alguna queda "no disponible"
    void enable();
    void disable();
    void shutdown(); // deshace los parches y libera la memoria
    bool drawUI();   // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const { return m_enabled && m_available && m_tps != 240.f; }

private:
    void writeConstants();

    bool m_available = false;
    bool m_enabled = false;
    float m_tps = 240.f;

    double* m_constants = nullptr; // [0] = 1/TPS, [1] = TPS/60; reservadas cerca del exe
    core::memory::Patch m_tickLengthPatch;
    core::memory::Patch m_stepsPatch;
};

} // namespace features
