#pragma once

// Punto de entrada de todas las features. Todo se llama desde el hilo de render.
namespace features {

// Instala hooks, busca firmas y carga config.json. Llamar una sola vez (MinHook ya iniciado).
void init();

// Desactiva todo y deshace los parches (antes de quitar los hooks al descargar la DLL).
void shutdown();

// Una vez por frame, antes de dibujar: teclas asignadas, teclas de las features y guardado de la config si hay cambios pendientes.
void update();

// Contenido de cada ventana del panel.
void drawGlobal();
void drawPlayer();
void drawLevel();
void drawBypass();
void drawCreator();
void drawLabels();
void drawShortcuts();
void drawInterface();

// Lo que se dibuja sobre el juego aunque el panel esté cerrado (contador de noclip...).
void drawOverlay();

// true si hay alguna función de trampa activa (para el Cheat Indicator de la fase 5).
bool cheatsActive();

} // namespace features
