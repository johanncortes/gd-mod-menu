#pragma once
#include <nlohmann/json_fwd.hpp>

// Aspecto del panel: tema, color de acento, escala y fuente. También guarda la posición
// de las ventanas (gdmenu_layout.ini junto a la DLL).
namespace gui::theme {

// Llamar una vez con el contexto de ImGui creado, antes del primer frame.
void init();

// Llamar antes de ImGui::NewFrame(): reconstruye la fuente si cambió (no se puede a mitad de frame).
void applyPending();

// Contenido de la ventana Interface. true si el usuario cambió algo.
bool drawUI();

void load(const nlohmann::json& j);
void save(nlohmann::json& j);

} // namespace gui::theme
