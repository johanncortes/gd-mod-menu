# Plan — GD Mod Menu

## Estado actual
- Fase: 2 (Global) con código completo, **pendiente de probar en el juego**. Fase 3 (Player) con código completo
  salvo *Show trajectory*, también **pendiente de probar**. El usuario pidió hacer las dos seguidas (2026-11-11).
- Último agente: Claude (Fable 5.2)
- Próximo paso: que el usuario pruebe las fases 2 y 3. Después: *Show trajectory* (con el usuario presente, ver nota)
  y Fase 4.
- Notas / bloqueos:
  - **Nada de las fases 2 y 3 se ha ejecutado dentro del juego.** Compila en Release sin warnings y las 9 firmas se
    comprobaron contra el exe instalado con `build/Release/sigcheck.exe` (todas únicas, constantes y offsets correctos).
  - **Show trajectory sin hacer**: hay que simular al jugador con funciones internas de `PlayerObject` y copiar
    decenas de campos por offset. A ciegas, sin poder probar en un nivel, es muy probable que crashee el juego, así que
    no se escribió. Hacerlo en una sesión con el usuario probando (o con permiso para manejar el juego).
  - **Physics bypass**: solo se redirigen las dos constantes del bucle de física. Falta comprobar en el juego que el
    porcentaje y el tiempo del nivel siguen bien con TPS distinto de 240 (no se encontró dónde se calculan).
  - **REA/Ghidra no funciona en este equipo**: REA exige Ghidra 12.1.x, y Ghidra 12.2 exige JDK 25 (hay JDK 21).
    El reversing de las fases 2 y 3 se hizo con `dumpbin /disasm` (Build Tools) y scripts de Python: referencias
    RIP, tabla `.pdata` para los límites de función, IAT para los imports y RTTI para los vtables.
  - **Input de GD**: el juego lee clics y teclas por *raw input* en un hilo aparte
    (`GetRawInputData` en libcocos2d), además de los mensajes de ventana. Bloquear solo en el
    WndProc no basta: el filtro está en `src/core/input.cpp` y cubre los dos caminos. Cualquier
    feature que lea o simule input (atajos, bot) debe tenerlo en cuenta.
  - ImGui solo captura el teclado cuando hay un campo de texto activo; con el panel abierto
    las teclas siguen llegando al juego (se puede jugar con el panel abierto).
  - **Orden de inicio**: las firmas se buscan antes de instalar hooks (`features::init`), porque un hook reescribe el
    inicio de la función y su firma deja de coincidir. Todo se inicia en el hilo de render, en el primer frame.
  - Los campos numéricos del panel se aplican al pulsar Enter o al salir del campo, no mientras se escribe.
  - El contador de noclip se dibuja abajo a la izquierda; el Cheat Indicator y el resto de labels son de la fase 5
    (`features::cheatsActive()` ya existe).
  - Opciones de CMake: `GDMENU_DEV` (ON: End descarga la DLL) y `GDMENU_CONSOLE` (OFF: la consola
    quita el foco y minimiza el juego a pantalla completa). El log va siempre a
    `build/Release/gdmenu.log`; ahí sale qué features quedaron "listo" o "no disponible".
  - Flujo de prueba: abrir GD, `build/Release/injector.exe`, Tab. Para recompilar: End en el
    juego (libera la DLL), compilar, inyectar otra vez.
  - Las capturas de pantalla externas (GDI/PrintWindow) no reflejan lo que dibuja el juego a
    pantalla completa; para verificar el render hay que leer el framebuffer desde la DLL.
  - Pendiente menor: la posición de las ventanas no se guarda (`IniFilename = nullptr`), va con
    la fase 6. No se probó el cambio ventana <-> pantalla completa con el menú cargado.

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
- [ ] Probado dentro del juego

## Fase 3 — Player
- [x] Noclip (+ contador de muertes y precisión)
- [x] Mostrar hitboxes
- [x] No death effect / no respawn flash
- [x] Respawn delay
- [ ] Show trajectory (sin hacer, ver Estado actual)
- [ ] Probado dentro del juego

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
1. Ver qué firmas fallan: `build/Release/sigcheck.exe "<ruta>/GeometryDash.exe"` (o el panel, que las marca "no disponible").
2. Usar REA para volver a encontrarlas.
3. Actualizar `signatures.hpp` y `docs/firmas.md` con la nueva versión.
