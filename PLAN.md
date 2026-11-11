# Plan — GD Mod Menu

## Estado actual
- Fase: 1 completada (esqueleto probado dentro del juego el 2026-11-10).
- Último agente: Claude (Fable 5.2)
- Próximo paso: Fase 2, Global (speedhack con hook a `CCScheduler::update`).
- Notas / bloqueos:
  - **Input de GD**: el juego lee clics y teclas por *raw input* en un hilo aparte
    (`GetRawInputData` en libcocos2d), además de los mensajes de ventana. Bloquear solo en el
    WndProc no basta: el filtro está en `src/core/input.cpp` y cubre los dos caminos. Cualquier
    feature que lea o simule input (atajos, bot) debe tenerlo en cuenta.
  - ImGui solo captura el teclado cuando hay un campo de texto activo; con el panel abierto
    las teclas siguen llegando al juego (se puede jugar con el panel abierto).
  - Opciones de CMake: `GDMENU_DEV` (ON: End descarga la DLL) y `GDMENU_CONSOLE` (OFF: la consola
    quita el foco y minimiza el juego a pantalla completa). El log va siempre a
    `build/Release/gdmenu.log`.
  - Flujo de prueba: abrir GD, `build/Release/injector.exe`, Tab. Para recompilar: End en el
    juego (libera la DLL), compilar, inyectar otra vez.
  - Las capturas de pantalla externas (GDI/PrintWindow) no reflejan lo que dibuja el juego a
    pantalla completa; para verificar el render hay que leer el framebuffer desde la DLL.
  - Pendiente menor: la posición de las ventanas no se guarda (`IniFilename = nullptr`), va con
    la config de la fase 2/6. No se probó el cambio ventana <-> pantalla completa con el menú cargado.

## Fase 1 — Esqueleto
- [x] CMakeLists con MinHook, ImGui y nlohmann/json vía FetchContent
- [x] `dllmain.cpp` + hilo de inicio + consola de debug opcional
- [x] Hook `wglSwapBuffers` y `WndProc`; ImGui dibujando
- [x] Panel con Tab y ventanas vacías: Global, Level, Bypass, Player, Bot, Creator, Labels, Shortcuts, Interface
- [x] Inyector para desarrollo
- [x] Probado dentro del juego

## Fase 2 — Global (cocos2d, estables)
- [x] Speedhack (hook `CCScheduler::update`)
- [x] FPS bypass
- [x] Physics bypass (TPS)
- [x] Guardar/cargar config.json

## Fase 3 — Player
- [ ] Noclip (+ contador de muertes y precisión)
- [ ] Mostrar hitboxes
- [ ] No death effect / no respawn flash
- [ ] Respawn delay
- [ ] Show trajectory

## Fase 4 — Level / Bypass / Creator (parches de bytes)
- [ ] Practice music sync, auto practice mode, hide pause menu
- [ ] Unlock icons, character/text limit bypass
- [ ] Object limit bypass, free scroll, copy level bypass, verify bypass
- [ ] StartPos switcher

## Fase 5 — Labels
- [ ] FPS, CPS, porcentaje, intentos, tiempo, reloj
- [ ] Cheat indicator, noclip accuracy

## Fase 6 — Interface y Shortcuts
- [ ] Temas, color de acento, escala, fuente
- [ ] Keybinds (clic derecho en cualquier opción)
- [ ] Atajos: reiniciar nivel, toggle practice, checkpoints

## Fase 7 — Bot (al final)
- [ ] Grabar inputs
- [ ] Reproducir inputs
- [ ] Guardar/cargar replays

## Mantenimiento por actualización de GD
1. Ver qué firmas de `docs/firmas.md` fallan (el panel las marca "no disponible").
2. Usar REA para volver a encontrarlas.
3. Actualizar `signatures.hpp` y `docs/firmas.md` con la nueva versión.
