# Plan — GD Mod Menu

## Estado actual
- Fase: 5 (Labels) en curso, rama `fase-5`. Fases 1 a 4 probadas por el usuario (de la 3 falta *Show trajectory*).
- Último agente: Claude (Fable 5.2)
- Próximo paso: terminar y probar la fase 5. Después: fase 6. Pendientes sueltos: *Show trajectory* y que
  *Instant complete* teletransporte al jugador al final del nivel mientras sale la animación (idea del usuario).
- Notas / bloqueos:
  - **Verify bypass e Instant complete**: el usuario pidió hacerlos (2026-11-11) aunque rozan la regla de
    `AGENTS.md` sobre servidores; su argumento es que el anti-trampas del propio juego ya se ocupa. Por eso *Instant
    complete* solo llama a `PlayLayer::levelComplete()` y **no** toca el anti-trampas del juego. No añadir bypass de
    ese anti-trampas sin que el usuario lo pida.
  - **StartPos switcher**: usa `m_startPosObjects` directamente. La función del juego
    `getStartPosObjectsForCurrentMode` devuelve lista vacía en niveles online en modo normal (por eso falló la
    primera prueba; ya probado y funciona). Si no hay StartPos, al pulsar Q/E sale el aviso "Este nivel no tiene StartPos".
  - **Object limit bypass**: en 2.2091 ya no existe ese límite en el editor; en su lugar se hizo *Custom object
    bypass* (los dos límites de los objetos personalizados), que es lo que trae la lista final.
  - **Show trajectory sin hacer**: hay que crear un PlayerObject falso, copiarle el estado del real y simular
    `checkCollisions` + `update` cientos de ticks por frame, con hooks para que esa simulación no mate al jugador,
    no active triggers ni suene. Son unos 10-15 hooks nuevos; hacerlo en pasos cortos con el usuario probando.
  - **Physics bypass**: solo se redirigen las dos constantes del bucle de física (probado por el usuario; no se
    comprobó a fondo el porcentaje con TPS distinto de 240).
  - **Reversing**: lo más rápido es mirar la dirección en los bindings de Geode (ver `docs/firmas.md`) y decompilar
    con Ghidra headless (`analyzeHeadless` + un script de `DecompInterface`); funciona con el Ghidra 12.1.4 instalado.
  - **REA/Ghidra**: REA exige Ghidra 12.1.x. El 2026-11-11 se instaló Ghidra 12.1.4 en
    `C:\Tools\ghidra_12.1.4_PUBLIC` (`GHIDRA_INSTALL_DIR` de usuario apunta ahí) y Temurin JDK 25; el modo headless
    arranca. **Falta comprobar REA** tras reiniciar VS Code (el servidor MCP lee la variable al arrancar).
    El reversing de las fases 2 y 3 se hizo con `dumpbin /disasm` (Build Tools) y scripts de Python: referencias
    RIP, tabla `.pdata` para los límites de función, IAT para los imports y RTTI para los vtables. Sirve de plan B.
  - **Git**: `origin` es `github.com/johanncortes/gd-mod-menu` (privado). Desde la fase 4, cada fase va en su rama
    `fase-N`: commits pequeños, push tras cada commit y merge a `main` cuando el usuario la confirme probada
    (el usuario dio permiso para que el agente haga los merges).
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
- [x] Probado dentro del juego

## Fase 3 — Player
- [x] Noclip (+ contador de muertes y precisión)
- [x] Mostrar hitboxes
- [x] No death effect / no respawn flash
- [x] Respawn delay
- [x] Probado dentro del juego (todo lo anterior, 2026-11-11)
- [ ] Show trajectory (sin hacer, ver Estado actual)

## Fase 4 — Level / Bypass / Creator (parches de bytes)
- [x] Practice music sync, auto practice mode, hide pause menu
- [x] Unlock icons, character limit bypass, character filter bypass
- [x] Custom object bypass (sustituye a object limit bypass), free scroll, level copy bypass
- [x] StartPos switcher (Q / E)
- [x] Verify bypass
- [x] Instant complete (botón)
- [x] Probado dentro del juego (2026-11-11)

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

## Lista final de funciones (objetivo)
Referencia: captura del panel que pasó el usuario (2026-11-11). `[x]` = hecho y probado, `[`]` = hecho sin probar,
`[ ]` = pendiente. Las fases de arriba dicen en qué orden se hacen; lo que no aparece en ninguna fase va después
de la fase 7.

**Global**: [x] FPS Bypass · [x] Physics Bypass · [x] Speedhack · [ ] Pitch Shift · [x] Audio Speed (opción del
speedhack) · [ ] Sync with Speedhack · [ ] Auto Safe Mode · [ ] Compact Editor Levels · [ ] Compact Profile Comments ·
[ ] Discord RPC · [ ] Lock Cursor · [ ] No Short Numbers · [ ] Safe Mode · [ ] Show Taps · [ ] Transition Speed ·
[ ] Transparent Lists

**Level**: [ ] 0% Practice Completion · [ ] Accurate Percentage · [ ] All Modes in Platformer · [ ] All Passable ·
[ ] Always Show Coins · [ ] Auto LDM · [ ] Auto Pickup Coins · [`] Auto Practice Mode · [ ] Auto Song Download ·
[ ] Checkpoint Delay · [ ] Confirm Full Restart · [ ] Confirm Practice Mode · [ ] Confirm Restart ·
[ ] Force Platformer · [ ] Freeze Attempts · [ ] Hide Level Complete VFX · [`] Hide Pause Menu ·
[ ] Hitbox Multiplier · [ ] Instant Mirror · [ ] Legacy Reverse Physics · [ ] Match Percent/Time Fonts ·
[ ] Mute Level SFX · [ ] No Mirror · [ ] No Object Glow · [ ] No Particles · [ ] No Portal Lightning ·
[ ] No Shaders · [ ] Pause Countdown · [ ] Pause During Complete · [ ] Practice Coins · [ ] Practice Fix

**Bypass**: [ ] Allow Low Volume · [`] Character Filter Bypass · [`] Character Limit Bypass · [ ] Checkpoint Limit ·
[ ] Comment History Bypass · [ ] Quick Chests · [`] Instant Complete (*) · [ ] Level Kick · [ ] Menu Gameplay ·
[ ] Music Customizer · [ ] Pause Buffering · [`] Practice Music Sync · [`] Unlock Icons · [ ] Unlock Main Levels ·
[ ] Unlock Paths · [ ] Unlock Shops · [ ] Unlock Vaults

**Player**: [ ] Always Show Trail · [ ] AutoClicker · [ ] Auto Kill · [ ] Ball Rotation Bug · [ ] Click Teleport ·
[ ] Custom Wave Trail · [ ] Frame Stepper · [ ] Jump Hack · [ ] Mute Level Audio on Death ·
[ ] Mute Rewards SFX on Death · [x] No Death Effect · [ ] No Ghost Trail · [ ] No Hide Trigger ·
[ ] No Platformer Jump Anim · [x] No Respawn Flash · [ ] No Robot Fire · [ ] No Spider Dash · [ ] No Trail ·
[ ] No Vehicle Particles · [x] Noclip · [ ] Show Trajectory · [ ] Solid Wave Trail · [x] Respawn Delay ·
[x] Mostrar hitboxes (no sale en la captura, se mantiene)

**Bot**: [ ] Disabled / Record / Playback · [ ] Replays (New, Save, Load, Delete) · [ ] Ignore Inputs

**Creator**: [`] Custom Object Bypass · [ ] Default Song Bypass · [`] Free Scroll · [ ] Hide Triggers · [ ] Hide UI ·
[`] Level Copy Bypass · [ ] Level Edit · [ ] No (C) Mark · [ ] Reset Percent on Save · [ ] Slider Limit ·
[ ] Smooth Editor Trail · [`] Verify Bypass (*)

**Labels**: [ ] Show Labels · [ ] Show in Editor · [ ] Cheat Indicator · [ ] Import Label · [ ] Add New Label ·
[ ] Presets · [ ] FPS · [ ] Testmode · [ ] Run From · [ ] Attempt · [ ] Percentage · [ ] Level Time · [ ] Best Run ·
[ ] Clock · [ ] CPS · [x] Noclip Accuracy · [x] Noclip Deaths

**Shortcuts**: [`] StartPos switcher (no sale en la captura, se mantiene) · [ ] P1 Jump · [ ] P2 Jump · [ ] Show Options · [ ] Uncomplete Level · [ ] Restart Level ·
[ ] Toggle Practice Mode · [ ] Place Checkpoint · [ ] Remove Checkpoint · [ ] Inject DLL · [ ] Save Folder ·
[ ] Reset BG Volume · [ ] Reset SFX Volume

**Keybinds**: [x] Open Menu (Tab) · [ ] In-game only · [ ] Clic derecho en cualquier opción para asignarle una tecla

**Interface**: [ ] Theme · [ ] UI Scale · [ ] Font · [ ] Font Size · [ ] Reload Fonts · [ ] Layout Type · [ ] Style ·
[ ] Enable Blur · [ ] Animate Windows · [ ] Accent Color · [ ] Background Color · [ ] Language · [ ] Search

(*) Chocan con la regla de `AGENTS.md` "nada de funciones para subir completions ni interactuar con servidores de
GD". El usuario decidió hacerlas igualmente (2026-11-11), ver Estado actual.

## Mantenimiento por actualización de GD
1. Ver qué firmas fallan: `build/Release/sigcheck.exe "<ruta>/GeometryDash.exe"` (o el panel, que las marca "no disponible").
2. Usar REA para volver a encontrarlas.
3. Actualizar `signatures.hpp` y `docs/firmas.md` con la nueva versión.
