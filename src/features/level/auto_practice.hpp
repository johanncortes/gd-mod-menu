#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Auto practice mode: al terminar de cargar un nivel entra directamente en modo práctica.
// Hook a PlayLayer::setupHasCompleted que después llama a PlayLayer::togglePracticeMode(true).
class AutoPractice {
public:
    void init();
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
