#pragma once

namespace core::hooks {

// Instala el hook de wglSwapBuffers (el de WndProc se pone en el primer frame).
// Devuelve false si algo falla; en ese caso el juego sigue sin tocar.
bool install();

// true cuando el usuario pidió descargar la DLL y ImGui ya se cerró en el hilo de render.
bool readyToUnload();

// Quita todos los hooks. Llamar solo después de readyToUnload().
void uninstall();

} // namespace core::hooks
