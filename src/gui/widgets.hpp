#pragma once

namespace gui {

// Casilla. Si `available` es false sale desactivada con la nota "no disponible".
// Devuelve true si cambió.
bool toggle(const char* label, bool available, bool& enabled);

// Fila con casilla + campo numérico. El número se aplica al pulsar Enter o al salir del campo.
// Devuelve true si cambió la casilla o el valor.
bool toggleValue(const char* label, bool available, bool& enabled, float& value,
                 float min, float max, const char* format);

} // namespace gui
