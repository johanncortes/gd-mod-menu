#pragma once

// Acceso a objetos del juego que usan varias features.
namespace core::game {

// Localiza el puntero global a GameManager (firma kSwitchStartPos). false si no se encontró.
bool init();

// PlayLayer del nivel que se está jugando, o nullptr si no hay ninguno (menús, editor).
void* playLayer();

} // namespace core::game
