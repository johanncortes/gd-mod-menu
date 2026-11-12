#pragma once
#include <nlohmann/json_fwd.hpp>

// Keybinds: cualquier casilla o botón del panel puede tener una tecla asignada.
// Clic derecho sobre la opción abre un pequeño menú para elegirla.
// La opción se identifica por su texto, así que dos opciones no deben llamarse igual.
namespace gui::keybinds {

// Una vez por frame, antes de dibujar: detecta qué teclas asignadas se acaban de pulsar.
void update();

// true si este frame queda alguna tecla pulsada sin atender.
bool anyFired();

// true (una sola vez) si se pulsó la tecla asignada a esta opción.
bool consume(const char* label);

// true mientras la tecla asignada a esta opción está pulsada (para acciones que se mantienen).
bool held(const char* label);

// Sección "Keybinds" (en la ventana Shortcuts): opciones y lista de teclas con botón para
// quitarlas. Devuelve true si cambió algo.
bool drawUI();

// Llamar justo después de dibujar la opción: menú de clic derecho para asignar la tecla
// y, si ya tiene una, su nombre al lado.
void contextMenu(const char* label);

// true (una sola vez) si el usuario cambió alguna asignación: hay que guardar la config.
bool takeChanged();

void load(const nlohmann::json& j);
void save(nlohmann::json& j);

} // namespace gui::keybinds
