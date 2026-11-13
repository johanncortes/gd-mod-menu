#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// No new best popup: al morir con un récord nuevo no sale el cartel "New Best". Hook de
// PlayLayer::showNewBest: sin orbes ni diamantes esa función solo crea el cartel, así que se salta;
// si hay recompensa se deja pasar entera (la animación que suma orbes y diamantes va en ella).
class NoNewBest {
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
