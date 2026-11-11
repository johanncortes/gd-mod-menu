#pragma once
#include <windows.h>

namespace core {

// Abre gdmenu.log junto a la DLL y, si se pide, una consola de debug.
void logInit(HMODULE self, bool withConsole);
void logClose();

// Escribe una línea (formato printf) en el log y en la consola.
void logMsg(const char* fmt, ...);

} // namespace core
