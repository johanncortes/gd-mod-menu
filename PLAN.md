# Plan — GD Mod Menu

## Estado actual
- Fase: 8 (Retoques) lista para probar en la rama `fase-8`.
- Último agente: Antigravity
- Próximo paso: probar en el juego los retoques de la fase 8.
- Completado en fase 8:
  - Rediseño visual del panel estilo Mega Hack: grafito refinado (#17171c), bordes suaves, títulos centrados sin flechas de colapso, fuente Segoe UI por defecto.
  - Rediseño de labels con outline de 4 direcciones para máxima legibilidad, y opción de fondo HUD translúcido.
  - Labels arrastrables libremente con el ratón cuando el menú está abierto (Tab), con coordenadas persistidas en `config.json` y botón para restablecer posición.
  - Instant complete mejorado: teletransporta físicamente al jugador (y al segundo jugador en dual) a la meta y reproduce la animación de completado usando `playEndAnimationToPos` y el export `CCNode::setPosition` de `libcocos2d.dll`.
  - Outline nítido en el overlay del noclip y en el aviso del StartPos switcher.
  - Show trajectory: implementado con `CCDrawNode` nativo exportado por `libcocos2d.dll`, dibujando la parábola de trayectoria verde en tiempo real según la velocidad y gravedad del jugador.
  - Rejilla del panel mod reorganizada en 2 filas limpias y espaciadas (fila 0: Global, Level, Bypass, Player, Bot; fila 1: Creator, Labels, Shortcuts, Interface) para eliminar por completo solapamientos entre ventanas, con botón "Reordenar ventanas del panel" en Interface.
  - Labels unificados y consistentes en la fuente Segoe UI nativa con contorno limpio en 4 direcciones, arrastrables y con tamaño proporcionado y perfectamente alineado.
- Pendientes para siguientes etapas:
  - Funciones opcionales de la lista final (audios, cosméticos, etc.).
- Notas / bloqueos:
  - **Bot**: graba en el hook de `handleButton` con el tick de `m_commandIndex` y reproduce desde el hook de
    `processCommands` (`src/features/bot/bot.cpp`). Si el tick retrocede (reinicio o checkpoint) se descarta lo
    grabado desde ese tick. Probado por el usuario en Future Funk (3 minutos idénticos). Un replay solo vale con los
    mismos TPS con los que se grabó. Formato: JSON en `replays/` junto a la DLL, `inputs` = lista de
    `[tick, botón, jugador, pulsado]`.
  - **Ventanas del panel**: se ajustan siempre a su contenido (`AlwaysAutoResize`, ancho mínimo 250 x escala). Antes
    se quedaban con el tamaño guardado en `gdmenu_layout.ini` y la del bot salía recortada.
  - **Keybinds**: clic derecho sobre cualquier casilla o botón (`gui/keybinds.cpp`). La opción se identifica por
    su texto: **no puede haber dos opciones con el mismo texto**. Con el panel cerrado, las opciones se dibujan un
    frame en una ventana invisible para que la tecla use el mismo código que el clic (`gui/menu.cpp`).
    El StartPos switcher sigue con Q/E fijas.
  - **Labels**: salen arriba a la izquierda y solo dentro de un nivel. El tiempo es la suma de los `dt` de
    `GJBaseGameLayer::update` desde el último `resetLevel`. El CPS cuenta clic izquierdo, Espacio, Flecha arriba
    y W en el hook de raw input. Faltan de la lista final: Best Run, Run From, Testmode, labels personalizados,
    presets y "Show in editor".
  - **Hooks de nivel compartidos**: `PlayLayer::resetLevel` y `GJBaseGameLayer::update` se hookean una sola vez en
    `src/core/game.cpp`; las features se apuntan con `core::game::onResetLevel` / `onUpdate` (MinHook no admite dos
    hooks sobre la misma función). `core::game::playLayer()` da el nivel actual.
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
  - La posición de las ventanas se guarda en `gdmenu_layout.ini` junto a la DLL. No se probó el cambio
    ventana <-> pantalla completa con el menú cargado.

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
- [x] FPS, CPS, porcentaje, intentos, tiempo, reloj
- [x] Cheat indicator, noclip accuracy
- [x] Probado dentro del juego (2026-11-11)

## Fase 6 — Interface y Shortcuts
- [x] Temas, color de acento, escala, fuente
- [x] Keybinds (clic derecho en cualquier opción)
- [x] Atajos: reiniciar nivel, toggle practice, checkpoints
- [x] Probado dentro del juego (2026-11-11)

## Fase 7 — Bot (al final)
- [x] Grabar inputs
- [x] Reproducir inputs
- [x] Guardar/cargar replays
- [x] Probado dentro del juego (2026-11-11)

## Fase 8 — Retoques (en curso)
- [x] Rediseño visual del panel (estilo Mega Hack / oscuro moderno con grafito y contraste limpio)
- [x] Rediseño visual de los labels (outline nítido de 4 direcciones para legibilidad total, opción de fondo HUD translúcido)
- [x] Mover los labels de sitio dentro del juego (arrastrable libremente con el menú abierto, botón de reset y persistencia en config.json)
- [x] Instant complete mejorado (teletransportar al jugador al final del nivel mientras se ejecuta la animación con `kPlayEndAnimation` y `CCNode::setPosition`)
- [x] Show trajectory (predicción y renderizado de la trayectoria de salto y caída con CCDrawNode nativo)
- [ ] Probado dentro del juego

## Lista final de funciones (objetivo)
Referencia: captura del panel que pasó el usuario (2026-11-11). `[x]` = hecho y probado, `[~]` = hecho sin probar,
`[ ]` = pendiente. Las fases de arriba dicen en qué orden se hacen; lo que no aparece en ninguna fase va después
de la fase 7.

**Global**: [x] FPS Bypass · [x] Physics Bypass · [x] Speedhack · [ ] Pitch Shift · [x] Audio Speed (opción del
speedhack) · [ ] Sync with Speedhack · [ ] Auto Safe Mode · [ ] Compact Editor Levels · [ ] Compact Profile Comments ·
[ ] Discord RPC · [ ] Lock Cursor · [ ] No Short Numbers · [ ] Safe Mode · [ ] Show Taps · [ ] Transition Speed ·
[ ] Transparent Lists

**Level**: [ ] 0% Practice Completion · [ ] Accurate Percentage · [ ] All Modes in Platformer · [ ] All Passable ·
[ ] Always Show Coins · [ ] Auto LDM · [ ] Auto Pickup Coins · [x] Auto Practice Mode · [ ] Auto Song Download ·
[ ] Checkpoint Delay · [ ] Confirm Full Restart · [ ] Confirm Practice Mode · [ ] Confirm Restart ·
[ ] Force Platformer · [ ] Freeze Attempts · [ ] Hide Level Complete VFX · [x] Hide Pause Menu ·
[ ] Hitbox Multiplier · [ ] Instant Mirror · [ ] Legacy Reverse Physics · [ ] Match Percent/Time Fonts ·
[ ] Mute Level SFX · [ ] No Mirror · [ ] No Object Glow · [ ] No Particles · [ ] No Portal Lightning ·
[ ] No Shaders · [ ] Pause Countdown · [ ] Pause During Complete · [ ] Practice Coins · [ ] Practice Fix

**Bypass**: [ ] Allow Low Volume · [x] Character Filter Bypass · [x] Character Limit Bypass · [ ] Checkpoint Limit ·
[ ] Comment History Bypass · [ ] Quick Chests · [x] Instant Complete (*) · [ ] Level Kick · [ ] Menu Gameplay ·
[ ] Music Customizer · [ ] Pause Buffering · [x] Practice Music Sync · [x] Unlock Icons · [ ] Unlock Main Levels ·
[ ] Unlock Paths · [ ] Unlock Shops · [ ] Unlock Vaults

**Player**: [ ] Always Show Trail · [ ] AutoClicker · [ ] Auto Kill · [ ] Ball Rotation Bug · [ ] Click Teleport ·
[ ] Custom Wave Trail · [ ] Frame Stepper · [ ] Jump Hack · [ ] Mute Level Audio on Death ·
[ ] Mute Rewards SFX on Death · [x] No Death Effect · [ ] No Ghost Trail · [ ] No Hide Trigger ·
[ ] No Platformer Jump Anim · [x] No Respawn Flash · [ ] No Robot Fire · [ ] No Spider Dash · [ ] No Trail ·
[ ] No Vehicle Particles · [x] Noclip · [~] Show Trajectory · [ ] Solid Wave Trail · [x] Respawn Delay ·
[x] Mostrar hitboxes (no sale en la captura, se mantiene)

**Bot**: [x] Disabled / Record / Playback · [x] Replays (New, Save, Load, Delete) · [x] Ignore Inputs

**Creator**: [x] Custom Object Bypass · [ ] Default Song Bypass · [x] Free Scroll · [ ] Hide Triggers · [ ] Hide UI ·
[x] Level Copy Bypass · [ ] Level Edit · [ ] No (C) Mark · [ ] Reset Percent on Save · [ ] Slider Limit ·
[ ] Smooth Editor Trail · [x] Verify Bypass (*)

**Labels**: [x] Show Labels · [ ] Show in Editor · [x] Cheat Indicator · [ ] Import Label · [ ] Add New Label ·
[ ] Presets · [x] FPS · [ ] Testmode · [ ] Run From · [x] Attempt · [x] Percentage · [x] Level Time · [ ] Best Run ·
[x] Clock · [x] CPS · [x] Noclip Accuracy · [x] Noclip Deaths

**Shortcuts**: [x] StartPos switcher (no sale en la captura, se mantiene) · [ ] P1 Jump · [ ] P2 Jump · [ ] Show Options · [ ] Uncomplete Level · [x] Restart Level ·
[x] Toggle Practice Mode · [x] Place Checkpoint · [x] Remove Checkpoint · [ ] Inject DLL · [ ] Save Folder ·
[ ] Reset BG Volume · [ ] Reset SFX Volume

**Keybinds**: [x] Open Menu (Tab) · [ ] In-game only · [x] Clic derecho en cualquier opción para asignarle una tecla

**Interface**: [x] Theme · [x] UI Scale · [x] Font · [x] Font Size · [ ] Reload Fonts · [ ] Layout Type · [ ] Style ·
[ ] Enable Blur · [ ] Animate Windows · [x] Accent Color · [ ] Background Color · [ ] Language · [ ] Search

(*) Chocan con la regla de `AGENTS.md` "nada de funciones para subir completions ni interactuar con servidores de
GD". El usuario decidió hacerlas igualmente (2026-11-11), ver Estado actual.

## Lista ampliada (segunda captura, Mega Hack v9.1)
Captura más reciente que pasó el usuario (2026-11-11). Va **después** de la fase 7 y de la fase de retoques: primero
dejar algo terminado y luego ir añadiendo esto por bloques. Dificultad estimada: **F** fácil (un parche o un hook
como los de la fase 4), **M** media, **D** difícil (días de trabajo y muchas pruebas), **N/A** no aplica a este
proyecto. Lo que ya está hecho lleva `[x]`.

- **Bypass** (F): Anti-Kick · Challenge Level · Keymaster · Main Levels · Music Customizer (M) · Slider Limit ·
  [x] Text Length · Treasure Room · [x] Unlock Icons · Unlock Shops · Unlock Vaults
- **Creator**: Accurate Save (M) · [x] Copy Hack · [x] Custom Object Bypass · Default Song Bypass (F) ·
  Editor Extension (D) · [x] Free Scroll · Hide UI (F) · Level Edit (F) · Multiple Editor Trails (M) · No C Mark (F) ·
  Place Over (F) · Smooth Editor Trail (F) · Toolbox Button Bypass (F) · Trigger Value Bypass (M) · [x] Verify Hack
- **Cosmetic** (casi todo F, un parche por opción): Coin Shower (M) · Frozen Animations · Hide Pause Button ·
  [x] Hide Pause Menu · Hide Player · Icon Randomizer (M) · No Camera (M) · No Camera Zoom · No Circle Effect ·
  No Dash Fire · [x] No Death Effect · No Do Not Flip · No End Shake · No Ghost Trail · No Glow · No Mirror ·
  No New Best Popup · No Orb Ring · No Particles · No Particles Classic · No Portal Circle · No Portal Lightning ·
  No Pulse · [x] No Respawn Flash · No Robot Fire · No Shaders · No Shake · No Spider Dash · No Swing Fire ·
  No Trail · No Wave Pulse · No Wave Trail · No Trail Behind Wave · Player 1 on Top · Player on Top ·
  Show Total Attempts · Solid Wave Trail · StartPos Reset Camera (M) · Stop Triggers on Death (M) ·
  Trail Always On · Trail Cutting · Wave Pulse Size · Wave Trail on Death
- **Level**: 0% Practice Complete (F) · Allow Pause Buffering (F) · All Modes Platformer (F) · Auto Clicker (M) ·
  Auto Deafen (M, pulsa el atajo de Discord a cierto porcentaje) · Auto Kill (F) · Auto Music Sync (M) ·
  Auto Pickup Coins (M) · Auto Song Download (M) · Click Between Frames (D) · Click Between Steps (D) ·
  Click on Steps (D) · Checkpoint Limit Bypass (F) · Collect Coins In Practice (F) · Confirm Exit / Full Reset /
  Normal / Practice / Reset (F) · Force Ice (F) · Force Platformer (F) · Frame Stepper (M) · Hitbox Multiplier (M) ·
  [x] Instant Complete · Jumpscare (M) · Jump Hack (F) · [x] Noclip · Noclip Limits (F) · No Collision (F) ·
  Pause During Complete (F) · Practice Bug Fix (D) · [x] Practice Music · Random Seed (M) ·
  Replay Last Checkpoint (M) · [x] Respawn Time · Shipcopter (M) · [x] Show Hitboxes · Show Hitboxes on Death (F) ·
  Show Hitboxes Trail (F, el juego ya guarda el rastro) · Show Layout (D) · Show Trajectory (D) · Show Triggers (M) ·
  [x] Smart StartPos (lo que hay es el StartPos switcher; el "smart" que ajusta velocidad y modo es D)
- **Status** (labels): Field Formatting (M) · Font · Scale · Opacity · Hide Status (F) · [x] Noclip Accuracy ·
  [x] Noclip Deaths · [x] Attempts · Jumps (F) · [x] Percentage · [x] Level Time · Session Time (F) · [x] Clock ·
  Frame Counter (F) · Position (F) · Velocity (F) · Dead (F) · Replay State (con el bot)
- **Universal**: Allow Low Volume (F) · Compact Lists (M) · Custom Background (M) · Fast Chests (F) ·
  Load Audio to Memory (M) · Lock Cursor (F) · Main Menu Play (D) · No Music Fade Out (F) · No Transition (F) ·
  Pitch Shifter (F) · Thread Priority (F) · Transition Customizer (M) · Transparent Lists (M)
- **Display**: [x] FPS · Unlock FPS (F) · [x] Physics TPS · Frame Extrapolation (D) · Vertical Sync (F) ·
  Lock Delta (M) · Real Time (M) · Borderless (M) · Fullscreen (F)
- **Speedhack**: [x] Speed · [x] Speedhack Audio · Classic Mode (F)
- **Cheat Safety**: Safe Mode (M) · Auto Safe Mode (M) · Safe Mode Popup (F) · Disable Cheats (F) ·
  Ruleset (N/A, es propio de Mega Hack)
- **Utility**: P1 Click / P2 Click / Left / Right (M) · Uncomplete Level (M) · [x] Restart Level ·
  [x] Practice Mode · Settings (F) · Resources / AppData (F, abrir carpeta) · Toggle DevTools (N/A) · Crash Game (N/A)
- **Replay**: es la fase 7 (Record, Play, guardar y cargar). Convert (.json, .gdr) es M.
- **Keybinds**: [x] asignar tecla a una opción · View Keybinds (F) · Remove Keybinds (F) · Disable in Editor (F)
- **Mega Hack / Interface**: Search (M) · [x] Theme · [x] Interface Scale · Animations (M) · Sort Interface (F) ·
  Language (M) · Hide Endscreen Cheats / Extras (F) · Hide Menu Snow (F) · Auto-Update, Contribute Translations,
  Hide Iconic on Pause, Hide RobsVault Shortcut (N/A: son cosas del propio Mega Hack)
- **Screenshot** (M): captura a archivo y al portapapeles.

## Mantenimiento por actualización de GD
1. Ver qué firmas fallan: `build/Release/sigcheck.exe "<ruta>/GeometryDash.exe"` (o el panel, que las marca "no disponible").
2. Usar REA para volver a encontrarlas.
3. Actualizar `signatures.hpp` y `docs/firmas.md` con la nueva versión.
