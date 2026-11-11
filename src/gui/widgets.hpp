#pragma once

namespace gui {

// Casilla. Si `available` es false sale desactivada con la nota "no disponible".
// Devuelve true si cambió (por clic o por su tecla asignada: clic derecho para elegirla).
bool toggle(const char* label, bool available, bool& enabled);

// Fila con casilla + campo numérico. El número se aplica al pulsar Enter o al salir del campo.
// Devuelve true si cambió la casilla o el valor.
bool toggleValue(const char* label, bool available, bool& enabled, float& value,
                 float min, float max, const char* format);

// Botón de acción. Devuelve true si se pulsó (clic o tecla asignada).
bool button(const char* label, bool available);

} // namespace gui
