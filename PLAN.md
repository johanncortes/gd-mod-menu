# Plan — GD Mod Menu

## Estado actual
- Fase: 11 (Bypass, segundo bloque de la lista ampliada) con código completo, **pendiente de probar en el juego**,
  rama `fase-11`. Fases 1 a 10 probadas y mergeadas en `main`.
- Último agente: Claude (Opus 5.5)
- Próximo paso: que el usuario pruebe la fase 11. Después: merge a `main` y el siguiente bloque de la lista ampliada.
- Notas / bloqueos:
  - **Bypass (fase 11)**: los desbloqueos solo saltan la comprobación del botón que abre cada sitio (niveles
    principales, The Vault, Vault of Secrets, sótano del Keymaster, The Challenge, Treasure Room, tiendas). No escriben
    nada en la partida guardada; lo que el juego guarde al usar esos sitios (diálogos vistos, cofres abiertos) lo
    guarda el juego como siempre. **Anti-Kick** de Mega Hack no se hizo: no está claro qué "kick" evita y puede tocar
    servidores.
  - **Cosmetic (fase 10)**: ventana nueva "Cosmetic" (`features::drawCosmetic`). Sin efecto de muerte y Sin parpadeo
    pasaron ahí desde Player, pero su config sigue en `player`. Las nuevas guardan en la sección `cosmetic`. Casi
    todas son parches de bytes (`PatchFeature`); No mirror y No particles son hooks. "No spider dash" parchea un salto
    dentro de `playSpiderDashEffect`, cuyo inicio ya hookea Show trajectory: su firma se busca antes que ese hook.
    Al añadir la ventana se subió `kLayoutVersion` a 3 para que el panel se reordene solo una vez.
  - **Show trajectory** (`src/features/player/trajectory.cpp`): un PlayerObject propio, invisible, dentro de una capa
    invisible añadida al nivel. Cada frame copia al jugador 1 (`copyAttributes`) y simula "manteniendo" (verde) y
    "soltando" (rojo) con `update` + `checkCollisions(..., ignorar daño)` + `updateRotation`. Para no tocar la partida
    real: a `collisionCheckObjects` solo le llegan sólidos, peligros y rampas, y mientras se simula no hacen nada
    `gameEventTriggered`, `destroyObject` ni `playSpiderDashEffect`. **Limitaciones**: la predicción no sigue
    portales, pads, anillos ni teletransportes; solo jugador 1; desactivado en niveles de plataformas. Probado por el
    usuario (2026-11-11).
  - **Lo que pidió el usuario para la fase 8** (2026-11-11): panel en **Segoe UI** por defecto (le gustó); labels y
    aviso del StartPos switcher con la **fuente de Geometry Dash**, ajustable, bien alineados; labels movibles dentro
    del juego; Instant complete que lleve al jugador al final mientras sale la animación de completar.
  - **Fase 8, qué se corrigió** del otro agente: (1) su *Show trajectory* dibujaba una parábola con una gravedad
    inventada, sin la física del juego: **se quitó**; (2) Instant complete usaba offsets fijos y una altura inventada:
    se rehízo con `PlayLayer::checkForEnd` (firma `kCheckForEnd`); (3) los labels iban dentro de una ventana de ImGui
    con contorno a mano, y luego con una fuente Pusab TTF bajada de internet (descuadrada): ahora usan las fuentes de
    bitmap del propio juego (`src/gui/game_font.cpp`); (4) la rejilla fija de 2 filas solapaba ventanas: ahora se
    colocan por columnas según su tamaño real (`gui/menu.cpp`). El TTF de Pusab sigue en el historial de git
    (commit 9ddd584, ya borrado del árbol); quitarlo del todo exige reescribir historia y force-push: solo si el
    usuario lo pide.
  - **Labels**: se dibujan en la lista de dibujo de fondo de ImGui (delante del juego, detrás del panel). Con el panel
    abierto, una ventana invisible encima del bloque recoge el ratón para arrastrarlo (el clic no llega al juego).
    La posición se guarda en fracción de pantalla; en la mitad derecha las líneas se alinean a la derecha. Fuente,
    tamaño y opacidad son comunes a labels y StartPos switcher. El contador del noclip ahora son los labels
    "Noclip deaths" y "Noclip accuracy" (antes iba abajo a la izquierda por su cuenta).
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
- [x] Show trajectory (hecho en la fase 9)

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

## Fase 8 — Retoques
- [x] Rediseño visual del panel: tema oscuro, Segoe UI por defecto, títulos centrados (hecho por el otro agente, el
  usuario lo aprobó)
- [x] Labels y aviso del StartPos switcher con las fuentes de GD (Big, Gold, Chat o la del panel), tamaño y opacidad
- [x] Mover los labels de sitio dentro del juego (arrastrar con el panel abierto)
- [x] Ventanas del panel colocadas por columnas sin taparse (botón "Reordenar" en Interface)
- [x] Instant complete: lleva al jugador al portal final y el juego hace la animación y completa el nivel
- [x] Probado dentro del juego (2026-11-11)

## Fase 9 — Show trajectory
- [x] Simular al jugador con la física real del juego (PlayerObject de prueba + checkCollisions) y dibujar el camino
- [x] Probado dentro del juego (2026-11-11)
- [ ] Más adelante: seguir portales, pads y anillos en la predicción (hay que gestionar su activación por jugador)

## Fase 10 — Cosmetic (lista ampliada, bloque 1)
- [x] Ventana "Cosmetic" (con Sin efecto de muerte y Sin parpadeo, que estaban en Player)
- [x] No shake, No portal lightning, No circle effect, No particles
- [x] No mirror, No trail, No wave trail, No ghost trail, No spider dash
- [x] Probado dentro del juego (2026-11-11)

## Fase 11 — Bypass (lista ampliada, bloque 2)
- [x] Unlock main levels, Unlock vaults (The Vault y Vault of Secrets), Unlock basement (Keymaster)
- [x] Unlock challenge level, Unlock treasure room, Unlock shops
- [x] Slider limit bypass
- [ ] Probado dentro del juego

## Lista final de funciones (objetivo)
Referencia: captura del panel que pasó el usuario (2026-11-11). `[x]` = hecho y probado, `[`]` = hecho sin probar,
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
[ ] Mute Level SFX · [x] No Mirror · [ ] No Object Glow · [x] No Particles · [x] No Portal Lightning ·
[ ] No Shaders · [ ] Pause Countdown · [ ] Pause During Complete · [ ] Practice Coins · [ ] Practice Fix

**Bypass**: [ ] Allow Low Volume · [x] Character Filter Bypass · [x] Character Limit Bypass · [ ] Checkpoint Limit ·
[ ] Comment History Bypass · [ ] Quick Chests · [x] Instant Complete (*) · [ ] Level Kick · [ ] Menu Gameplay ·
[ ] Music Customizer · [ ] Pause Buffering · [x] Practice Music Sync · [x] Unlock Icons · [`] Unlock Main Levels ·
[ ] Unlock Paths · [`] Unlock Shops · [`] Unlock Vaults

**Player**: [ ] Always Show Trail · [ ] AutoClicker · [ ] Auto Kill · [ ] Ball Rotation Bug · [ ] Click Teleport ·
[ ] Custom Wave Trail · [ ] Frame Stepper · [ ] Jump Hack · [ ] Mute Level Audio on Death ·
[ ] Mute Rewards SFX on Death · [x] No Death Effect · [x] No Ghost Trail · [ ] No Hide Trigger ·
[ ] No Platformer Jump Anim · [x] No Respawn Flash · [ ] No Robot Fire · [x] No Spider Dash · [x] No Trail ·
[ ] No Vehicle Particles · [x] Noclip · [x] Show Trajectory · [ ] Solid Wave Trail · [x] Respawn Delay ·
[x] Mostrar hitboxes (no sale en la captura, se mantiene)

**Bot**: [x] Disabled / Record / Playback · [x] Replays (New, Save, Load, Delete) · [x] Ignore Inputs

**Creator**: [x] Custom Object Bypass · [ ] Default Song Bypass · [x] Free Scroll · [ ] Hide Triggers · [ ] Hide UI ·
[x] Level Copy Bypass · [ ] Level Edit · [ ] No (C) Mark · [ ] Reset Percent on Save · [`] Slider Limit ·
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

- **Bypass** (F): Anti-Kick (no se hace, ver Estado actual) · [`] Challenge Level · [`] Keymaster ·
  [`] Main Levels · Music Customizer (M) · [`] Slider Limit · [x] Text Length · [`] Treasure Room ·
  [x] Unlock Icons · [`] Unlock Shops · [`] Unlock Vaults
- **Creator**: Accurate Save (M) · [x] Copy Hack · [x] Custom Object Bypass · Default Song Bypass (F) ·
  Editor Extension (D) · [x] Free Scroll · Hide UI (F) · Level Edit (F) · Multiple Editor Trails (M) · No C Mark (F) ·
  Place Over (F) · Smooth Editor Trail (F) · Toolbox Button Bypass (F) · Trigger Value Bypass (M) · [x] Verify Hack
- **Cosmetic** (casi todo F, un parche por opción): Coin Shower (M) · Frozen Animations · Hide Pause Button ·
  [x] Hide Pause Menu · Hide Player · Icon Randomizer (M) · No Camera (M) · No Camera Zoom · [x] No Circle Effect ·
  No Dash Fire · [x] No Death Effect · No Do Not Flip · No End Shake · [x] No Ghost Trail · No Glow · [x] No Mirror ·
  No New Best Popup · No Orb Ring · [x] No Particles · No Particles Classic · No Portal Circle · [x] No Portal Lightning ·
  No Pulse · [x] No Respawn Flash · No Robot Fire · No Shaders · [x] No Shake · [x] No Spider Dash · No Swing Fire ·
  [x] No Trail · No Wave Pulse · [x] No Wave Trail · No Trail Behind Wave · Player 1 on Top · Player on Top ·
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
  Show Hitboxes Trail (F, el juego ya guarda el rastro) · Show Layout (D) · [x] Show Trajectory · Show Triggers (M) ·
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
