#pragma once
#include <nlohmann/json_fwd.hpp>

namespace features {

// Bot: graba los inputs del jugador y los reproduce.
// Cada input se guarda con el número de tick de física en que el juego lo procesó
// (contador de GJBaseGameLayer::processCommands). Al reproducir, en ese mismo tick se
// llama a GJBaseGameLayer::handleButton, igual que hace el juego con un input real.
// Los replays se guardan como JSON en la carpeta "replays" junto a la DLL.
class Bot {
public:
    void init(bool gameAvailable); // gameAvailable: core::game::init() funcionó
    void disable();                // deja el bot en "Disabled"
    bool drawUI();                 // true si cambió algo que va a config.json

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const; // reproduciendo: cuenta para el Cheat Indicator

private:
    bool m_available = false;
};

} // namespace features
