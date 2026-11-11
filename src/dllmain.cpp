#include <windows.h>

#include "config/config.hpp"
#include "core/hooks.hpp"
#include "core/log.hpp"

namespace {

DWORD WINAPI startThread(LPVOID param) {
    HMODULE self = static_cast<HMODULE>(param);

#ifdef GDMENU_CONSOLE
    core::logInit(self, true);
#else
    core::logInit(self, false);
#endif
    core::logMsg("DLL cargada");
    config::init(self);

    if (!core::hooks::install()) {
        // Sin hooks no hay menú, pero el juego sigue funcionando normal
        core::logMsg("Inicio fallido: el menú queda desactivado");
        return 0;
    }
    core::logMsg("Listo. Tab abre/cierra el panel");

#ifdef GDMENU_DEV
    // En desarrollo: End descarga la DLL para poder recompilar e inyectar de nuevo
    core::logMsg("End descarga la DLL");
    while (!core::hooks::readyToUnload())
        Sleep(50);
    core::hooks::uninstall();
    core::logMsg("DLL descargada");
    core::logClose();
    FreeLibraryAndExitThread(self, 0);
#else
    return 0;
#endif
}

} // namespace

BOOL APIENTRY DllMain(HMODULE self, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(self);
        // Nada pesado dentro de DllMain: todo el inicio va en un hilo aparte
        if (HANDLE thread = CreateThread(nullptr, 0, startThread, self, 0, nullptr))
            CloseHandle(thread);
    }
    return TRUE;
}
