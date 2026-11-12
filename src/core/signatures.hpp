#pragma once

// Firmas de GeometryDash.exe (?? = comodín). Cada una lleva la versión de GD donde se comprobó
// y está registrada en docs/firmas.md. Si alguna deja de encontrarse, su función se desactiva.
namespace core::signatures {

// GD Steam build 26002420 (exe del 2026-10-16).
// GJBaseGameLayer::getModifiedDelta: "mulsd xmm7, [1/240]" (duración de un tick de física).
// kTickLengthDisp es la posición, dentro de la firma, del desplazamiento RIP de la constante.
inline constexpr char kTickLength[] =
    "0F 5A F8 0F 57 C0 F2 0F 59 3D ?? ?? ?? ?? F3 0F 5A C1 F2 0F 58 83";
inline constexpr int kTickLengthDisp = 10;

// GD Steam build 26002420 (exe del 2026-10-16).
// GJBaseGameLayer::update: "mulsd xmm1, [4.0]" (pasos de física por cada 1/60 s; 4 = 240 TPS).
// kStepsPerFrameDisp es la posición del desplazamiento RIP de la constante.
inline constexpr char kStepsPerFrame[] =
    "0F 5A C0 41 0F 28 CF F2 0F 5E C8 F2 0F 59 0D ?? ?? ?? ?? 0F 57 C0 F2 0F 5A C1 FF 15";
inline constexpr int kStepsPerFrameDisp = 15;

// --- Fase 3 (Player). Todas: GD Steam build 26002420 (exe del 2026-10-16). ---

// PlayLayer::destroyPlayer(PlayerObject*, GameObject*), desde el inicio de la función hasta el
// final de la comprobación del pincho anti-trampas. De la propia firma se leen dos offsets:
//   kDestroyPlayerSpikeOffset -> PlayLayer::m_anticheatSpike  (mov rax,[rcx+X])
//   kDestroyPlayerIdOffset    -> GameObject::m_uniqueID       (mov eax,[rax+X])
inline constexpr char kDestroyPlayer[] =
    "48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 88 48 81 EC 40 01 00 00 "
    "0F 29 70 B8 0F 29 78 A8 44 0F 29 40 98 44 0F 29 48 88 44 0F 29 90 78 FF FF FF "
    "44 0F 29 98 68 FF FF FF 44 0F 29 A0 58 FF FF FF 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 C0 "
    "4C 8B EA 48 8B F1 48 85 D2 75 07 4C 8B A9 ?? ?? ?? ?? 48 8B 81 ?? ?? ?? ?? "
    "80 B8 ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 80 B9 ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? "
    "4D 85 C0 74 27 48 8B 81 ?? ?? ?? ?? 48 85 C0 74 1B 8B 80 ?? ?? ?? ?? "
    "41 39 80 ?? ?? ?? ?? 75 0C C6 81 ?? ?? ?? ?? 01 E9";
inline constexpr int kDestroyPlayerSpikeOffset = 0x90;
inline constexpr int kDestroyPlayerIdOffset = 0x9B;

// PlayLayer::resetLevel(), inicio de la función.
inline constexpr char kResetLevel[] =
    "48 8B C4 48 89 58 10 48 89 70 18 55 57 41 54 41 56 41 57 48 8D 68 A1 48 81 EC D0 00 00 00 "
    "0F 29 70 C8 0F 29 78 B8 44 0F 29 40 A8 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 FF "
    "48 8B F1 48 8B 89 ?? ?? ?? ?? 45 33 E4 44 38 A6 ?? ?? ?? ?? 75";

// GJBaseGameLayer::update(float), inicio de la función.
inline constexpr char kGameLayerUpdate[] =
    "48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 90 48 81 EC 70 01 00 00 "
    "0F 29 70 B8 0F 29 78 A8 44 0F 29 40 98 44 0F 29 48 88 44 0F 29 90 78 FF FF FF "
    "44 0F 29 98 68 FF FF FF 44 0F 29 A0 58 FF FF FF 44 0F 29 A8 48 FF FF FF "
    "44 0F 29 B0 38 FF FF FF 44 0F 29 B8 28 FF FF FF 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 C0 "
    "0F 28 F1 48 8B F1 80 3D ?? ?? ?? ?? 00 75 14";

// Función virtual de PlayLayer que decide si se dibujan las hitboxes:
// (opción "Show Hitboxes" && modo práctica) || (hitboxes al morir && jugador muerto).
// La firma incluye el inicio de la función siguiente, casi idéntica, para que sea única.
inline constexpr char kShowHitboxes[] =
    "80 B9 ?? ?? ?? ?? 00 74 09 80 B9 ?? ?? ?? ?? 00 75 12 80 B9 ?? ?? ?? ?? 00 74 0C "
    "80 B9 ?? ?? ?? ?? 00 74 03 B0 01 C3 32 C0 C3 CC CC CC CC CC CC 80 B9 ?? ?? ?? ?? 00 74 09";
// Desplazamiento de "jugador muerto" en PlayLayer, dentro de kShowHitboxes (fase 13).
inline constexpr int kShowHitboxesDead = 29;

// Función de PlayLayer que muestra u oculta el nodo de hitboxes según la función anterior.
inline constexpr char kUpdateHitboxVisibility[] =
    "48 89 5C 24 08 57 48 83 EC 20 48 8B 01 48 8B D9 FF 90 ?? ?? ?? ?? 84 C0 75 15 "
    "48 8B 03 48 8B CB FF 90 ?? ?? ?? ?? 84 C0 75 05 40 32 FF EB 03 40 B7 01 "
    "48 8B 8B ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ?? 40 3A F8 74";

// PlayerObject::playerDestroyed(bool): el `je` (byte kDeathEffectJump) que decide entre
// lanzar el efecto de muerte u ocultar al jugador sin más. Cambiado a `jmp` nunca hay efecto.
inline constexpr char kDeathEffect[] =
    "80 BB ?? ?? ?? ?? 00 74 20 40 84 FF 75 1B 40 38 BB ?? ?? ?? ?? 75 12 "
    "48 8B CB 48 8B 5C 24 30 48 83 C4 20 5F E9";
inline constexpr int kDeathEffectJump = 7;

// PlayerObject: función del parpadeo al reaparecer (CCBlink), inicio de la función.
// La llama PlayLayer::resetLevel para cada jugador; con un `ret` al principio no hace nada.
inline constexpr char kSpawnEffect[] =
    "48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 30 0F 29 74 24 20 48 8B F1 80 B9 ?? ?? ?? ?? 00 "
    "0F 85 ?? ?? ?? ?? BA 0B 00 00 00 FF 15 ?? ?? ?? ?? BA 04 00 00 00 F3 0F 10 05";

// --- Fase 4 (Level / Bypass / Creator). Todas: GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---

// PlayLayer::toggleMusicInPractice: final de la función, donde decide m_practiceMusicSync.
// kPracticeMusicSyncValue es el inmediato del "mov byte [m_practiceMusicSync], 0": con 1 siempre sincroniza.
inline constexpr char kPracticeMusicSync[] =
    "48 8B 87 ?? ?? ?? ?? 83 B8 ?? ?? ?? ?? 02 75 09 C6 87 ?? ?? ?? ?? 01 EB 07 C6 87 ?? ?? ?? ?? 00 "
    "48 8B 4C 24 48 48 33 CC";
inline constexpr int kPracticeMusicSyncValue = 31;

// PlayLayer::setupHasCompleted(), inicio de la función (el nivel terminó de cargar).
inline constexpr char kSetupHasCompleted[] =
    "48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D 68 B8 "
    "48 81 EC 20 01 00 00 0F 29 78 C8 44 0F 29 40 B8 44 0F 29 48 A8 48 8B 05 ?? ?? ?? ??";

// PlayLayer::togglePracticeMode(bool), inicio de la función.
inline constexpr char kTogglePracticeMode[] =
    "48 89 5C 24 08 57 48 83 EC 20 0F B6 FA 48 8B D9 38 91 ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? "
    "84 D2 74 34 E8 ?? ?? ?? ?? 48 8B CB";
inline constexpr int kTogglePracticeModeFlag = 18; // offset de PlayLayer::m_isPracticeMode

// PauseLayer::customSetup(), inicio de la función (construye el menú de pausa).
inline constexpr char kPauseLayerSetup[] =
    "48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D A8 08 FF FF FF "
    "48 81 EC D0 01 00 00 0F 29 70 C8 0F 29 78 B8 44 0F 29 40 A8 44 0F 29 48 98 44 0F 29 50 88 "
    "44 0F 29 A0 78 FF FF FF 44 0F 29 A8 68 FF FF FF 44 0F 29 B0 58 FF FF FF 44 0F 29 B8 48 FF FF FF "
    "48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 30 4C 8B F1";

// GJBaseGameLayer::getStartPosObjectsForCurrentMode(), inicio de la función.
// Esa función filtra (en niveles online y modo normal devuelve una lista vacía), así que no se
// llama: de la firma solo se lee kStartPosObjectsArray, el offset de m_startPosObjects
// (CCArray con todos los StartPos del nivel, ordenados).
inline constexpr char kStartPosObjects[] =
    "48 89 74 24 10 57 48 83 EC 20 48 8B F9 FF 15 ?? ?? ?? ?? 48 8B 97 ?? ?? ?? ?? 48 8B F0 "
    "83 BA ?? ?? ?? ?? 02 75 1E 48 8B 97 ?? ?? ?? ??";
inline constexpr int kStartPosObjectsArray = 41;

// PauseLayer::onResume: el tramo final, que es PlayLayer::switchToStartPosObject en línea.
// El switcher repite esos mismos pasos; todo lo que necesita se lee de la propia firma:
inline constexpr char kSwitchStartPos[] =
    "40 84 F6 0F 84 ?? ?? ?? ?? 48 8B 1D ?? ?? ?? ?? 48 85 DB 75 32 B9 E8 06 00 00 E8 ?? ?? ?? ?? "
    "48 89 44 24 30 48 8B C8 E8 ?? ?? ?? ?? 90 48 89 05 ?? ?? ?? ?? 48 8B 08 48 8B 51 48 48 8B C8 FF D2 "
    "48 8B 1D ?? ?? ?? ?? 48 8B 9B ?? ?? ?? ?? 48 39 BB ?? ?? ?? ?? 75 0E 48 8B 03 48 8B CB "
    "FF 90 ?? ?? ?? ?? EB 5D 48 85 FF 74 09 48 8B CF FF 15 ?? ?? ?? ?? 48 8B 8B ?? ?? ?? ?? 48 85 C9 74 06 "
    "FF 15 ?? ?? ?? ?? 48 89 BB ?? ?? ?? ?? 48 85 FF 0F 95 C0 88 83 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? "
    "48 8B 03 0F 57 C9 48 8B CB FF 90 ?? ?? ?? ?? 0F 57 C9 48 8B CB E8 ?? ?? ?? ?? 48 8B CB "
    "E8 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ??";
inline constexpr int kSwitchGameManager = 0x0C;        // disp RIP del puntero global a GameManager
inline constexpr int kSwitchPlayLayer = 0x4A;          // GameManager::m_playLayer
inline constexpr int kSwitchStartPosObject = 0x51;     // PlayLayer::m_startPosObject
inline constexpr int kSwitchResetLevelSlot = 0x5F;     // desplazamiento en el vtable de resetLevel()
inline constexpr int kSwitchTestMode = 0x94;           // byte "se juega desde un StartPos"
inline constexpr int kSwitchResetFromStart = 0x9C;     // rel32 de PlayLayer::resetLevelFromStart
inline constexpr int kSwitchVirtualSlot = 0xAB;        // desplazamiento en el vtable de una virtual (float)
inline constexpr int kSwitchUpdateCamera = 0xB6;       // rel32 de GJBaseGameLayer::updateCamera(float)
inline constexpr int kSwitchUpdateTestLabel = 0xBE;    // rel32 de PlayLayer::updateTestModeLabel
inline constexpr int kSwitchStartMusic = 0xC6;         // rel32 de PlayLayer::startMusic

// GameManager::isIconUnlocked(int, IconType) y GameManager::isColorUnlocked(int, UnlockType),
// inicio de cada función. Parche: "mov al,1; ret".
inline constexpr char kIsIconUnlocked[] =
    "40 53 56 57 48 83 EC 50 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 48 41 8B F8 8B DA 48 8B F1 "
    "45 85 C0 0F 85 ?? ?? ?? ??";
inline constexpr char kIsColorUnlocked[] =
    "40 53 56 57 48 83 EC 50 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 48 41 8B F0 8B FA 48 8B D9 "
    "83 FA 04 7D 07 B0 01 E9 ?? ?? ?? ??";

// EditorUI::constrainGameLayerPosition(float, float), inicio de la función. Parche: "ret".
inline constexpr char kConstrainGameLayer[] =
    "48 89 5C 24 10 48 89 7C 24 18 55 48 8B EC 48 81 EC 80 00 00 00 48 8B 81 ?? ?? ?? ?? 48 8B F9 "
    "0F 29 74 24 70 0F 29 7C 24 60 0F 28 FA 44 0F 29 44 24 50";

// LevelInfoLayer::tryCloneLevel: "contraseña == 1 (copia libre)? si no, pedirla".
// kTryCloneLevelJump es ese `jne`; con dos `nop` siempre copia directamente.
inline constexpr char kTryCloneLevel[] =
    "48 89 5C 24 10 57 48 81 EC 90 00 00 00 48 8B F9 80 B9 ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? "
    "48 8B 89 ?? ?? ?? ?? 8B 81 ?? ?? ?? ?? 2B 81 ?? ?? ?? ?? 83 F8 01 75 1A 33 D2";
inline constexpr int kTryCloneLevelJump = 0x33;

// LevelInfoLayer::init: "botón de copiar visible = (contraseña != 0)".
// kCloneButtonSetne es el `setne dl`; con "mov dl,1; nop" el botón sale siempre.
inline constexpr char kCloneButton[] =
    "49 8B 8E ?? ?? ?? ?? 4D 8B 86 ?? ?? ?? ?? 8B 81 ?? ?? ?? ?? 3B 81 ?? ?? ?? ?? 0F 95 C2 "
    "49 8B 00 49 8B C8 FF 90 ?? ?? ?? ??";
inline constexpr int kCloneButtonSetne = 26;

// EditorUI::onNewCustomItem: los dos límites (1000 objetos seleccionados, 200 objetos guardados).
// En cada firma el salto condicional "está dentro del límite" pasa a ser un `jmp` (90 E9).
inline constexpr char kCustomObjectSize[] =
    "48 8B 89 ?? ?? ?? ?? 48 85 C9 0F 84 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 3D E8 03 00 00 0F 86 ?? ?? ?? ??";
inline constexpr int kCustomObjectSizeJump = 27;
inline constexpr char kCustomObjectCount[] =
    "48 8B 89 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 3D C8 00 00 00 0F 82 ?? ?? ?? ?? BA C8 00 00 00 48 8D 0D ?? ?? ?? ??";
inline constexpr int kCustomObjectCountJump = 18;

// CCTextInputNode::onTextFieldInsertText: los dos sitios donde se aplica m_maxLabelLength.
// 1) al pegar texto: "if (max > 0) recortar"  -> el `jle` pasa a `jmp` (90 E9)
// 2) al escribir: "if (letras >= max && max > 0) no insertar" -> el `jl` pasa a `jmp` (EB)
inline constexpr char kTextLimitPaste[] =
    "41 8B BD ?? ?? ?? ?? 85 FF 0F 8E ?? ?? ?? ?? 4C 8D 4C 24 38 4C 8B 64 24 38 4C 8B 7C 24 50 49 83 FF 0F";
inline constexpr int kTextLimitPasteJump = 9;
inline constexpr char kTextLimitType[] =
    "41 8B 85 ?? ?? ?? ?? 41 8B 97 ?? ?? ?? ?? 3B D0 7C 08 85 C0 0F 8F ?? ?? ?? ?? 41 8B 8D ?? ?? ?? ??";
inline constexpr int kTextLimitTypeJump = 16;

// Filtro de caracteres de los campos de texto (función que usa onTextFieldInsertText):
// "if (permitidos está vacío) aceptar la letra". El `je` pasa a `jmp` y se acepta siempre.
inline constexpr char kTextFilter[] =
    "0F B6 2E 4D 85 ED 74 42 45 84 E4 74 06 40 80 FD 0A 74 37 49 8B FE 49 83 7E 18 0F 76 03 49 8B 3E 49 8B 46 10";
inline constexpr int kTextFilterJump = 6;

// ShareLevelLayer::onShare: "nivel verificado (== 1) o no listado? subir : aviso Unverified".
// kVerifyJump es el primer `je`; como `jmp` (90 E9) siempre sigue con la subida.
inline constexpr char kVerify[] =
    "8B 87 ?? ?? ?? ?? 2B 87 ?? ?? ?? ?? 83 F8 01 0F 84 ?? ?? ?? ?? 80 BF ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ??";
inline constexpr int kVerifyJump = 15;

// PlayLayer::levelComplete(), inicio de la función. Solo se llama (Instant complete).
inline constexpr char kLevelComplete[] =
    "48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D A8 D8 FD FF FF "
    "48 81 EC 00 03 00 00 0F 29 70 C8 0F 29 78 B8 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 ?? ?? ?? ??";

// --- Fase 5 (Labels). GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---

// PlayLayer::getCurrentPercent(), función entera hasta la lectura del jugador. Solo se llama.
inline constexpr char kCurrentPercent[] =
    "40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? 00 48 8B D9 74 18 48 8B 81 ?? ?? ?? ?? 66 0F 6E 80 ?? ?? ?? ?? "
    "0F 5B C0 48 83 C4 20 5B C3 48 8B 89 ?? ?? ?? ??";

// PlayLayer::updateAttempts(), inicio de la función: "inc dword [rcx + m_attempts]".
// kUpdateAttemptsOffset es la posición de ese offset dentro de la firma.
inline constexpr char kUpdateAttempts[] =
    "40 53 56 57 48 83 EC 20 48 8B F1 FF 81 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 85 C9 75 32 B9 E8 06 00 00";
inline constexpr int kUpdateAttemptsOffset = 13;

// --- Fase 6 (Shortcuts). GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---

// UILayer::onCheck(CCObject*) y UILayer::onDeleteCheck(CCObject*): los botones de poner y quitar
// checkpoint del modo práctica, función entera. No usan sus argumentos. Solo se llaman.
inline constexpr char kUiOnCheck[] =
    "48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 85 C9 75 32 B9 E8 06 00 00 E8 ?? ?? ?? ?? 48 89 44 24 40 48 8B C8 "
    "E8 ?? ?? ?? ?? 90 48 89 05 ?? ?? ?? ?? 48 8B 08 48 8B 51 48 48 8B C8 FF D2 48 8B 0D ?? ?? ?? ?? "
    "48 8B 81 ?? ?? ?? ?? 48 85 C0 74 27 80 B8 ?? ?? ?? ?? 00 74 1E C6 80 ?? ?? ?? ?? 01 48 8B 89 ?? ?? ?? ?? "
    "48 8B 89 ?? ?? ?? ?? 48 83 C4 28 E9 ?? ?? ?? ??";
inline constexpr char kUiOnDeleteCheck[] =
    "48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 85 C9 75 32 B9 E8 06 00 00 E8 ?? ?? ?? ?? 48 89 44 24 40 48 8B C8 "
    "E8 ?? ?? ?? ?? 90 48 89 05 ?? ?? ?? ?? 48 8B 08 48 8B 51 48 48 8B C8 FF D2 48 8B 0D ?? ?? ?? ?? "
    "48 8B 89 ?? ?? ?? ?? 48 85 C9 74 14 80 B9 ?? ?? ?? ?? 00 74 0B 33 D2 48 83 C4 28 E9 ?? ?? ?? ??";

// --- Fase 7 (Bot). GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---

// GJBaseGameLayer::processCommands(float, bool, bool), inicio de la función: un paso de física.
// kProcessCommandsTick es la posición del offset del contador de ticks ("mov eax,[rcx+X]").
inline constexpr char kProcessCommands[] =
    "48 89 5C 24 08 48 89 74 24 18 57 48 83 EC 40 8B 81 ?? ?? ?? ?? 41 0F B6 F1 0F 29 74 24 30 "
    "41 0F B6 F8 0F 29 7C 24 20 0F 57 F6";
inline constexpr int kProcessCommandsTick = 17;

// GJBaseGameLayer::handleButton(bool down, int button, bool isPlayer1), inicio de la función.
inline constexpr char kHandleButton[] =
    "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 56 41 57 48 83 EC 30 41 0F B6 E9 41 8B F8 "
    "0F B6 F2 48 8B D9 0F B6 89 ?? ?? ?? ??";

// --- Fase 8 (Retoques). GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---

// PlayLayer::checkForEnd(): si el jugador 1 ya llegó al portal final, lanza la animación de fin
// de nivel (que llama a levelComplete al terminar). Instant complete lo llama después de llevar
// al jugador al portal. De la firma se leen:
inline constexpr char kCheckForEnd[] =
    "48 89 5C 24 20 57 48 83 EC 20 48 8B F9 48 8B 89 ?? ?? ?? ?? 48 85 C9 0F 84 ?? ?? ?? ?? "
    "80 BF ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 80 B9 ?? ?? ?? ?? 00 75 26 48 8D 54 24 30 E8 ?? ?? ?? ?? "
    "48 8B D8 48 8B 8F ?? ?? ?? ?? 48 8B 11 FF 92 ?? ?? ?? ?? F3 0F 10 00 0F 2F 03 73 3E "
    "48 8B 8F ?? ?? ?? ?? 80 B9 ?? ?? ?? ?? 00";
inline constexpr int kCheckForEndPortal = 16;      // PlayLayer::m_endPortal
inline constexpr int kCheckForEndStarted = 31;     // PlayLayer::m_levelEndAnimationStarted
inline constexpr int kCheckForEndSpawnPos = 57;    // rel32 de EndPortalObject::getSpawnPos(CCPoint*)
inline constexpr int kCheckForEndPlayer = 67;      // GJBaseGameLayer::m_player1
inline constexpr int kCheckForEndGetPosition = 76; // posición de getPosition() en el vtable

// --- Fase 9 (Show trajectory). GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---
// Un PlayerObject propio, invisible, copia el estado del jugador real y avanza tick a tick
// con las mismas funciones que usa GJBaseGameLayer::update.

// PlayerObject::create(int, int, GJBaseGameLayer*, CCLayer*, bool playLayer), inicio de la función.
inline constexpr char kPlayerCreate[] =
    "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 40 49 8B F9 49 8B F0 "
    "8B EA 44 8B F1 B9 50 0C 00 00";

// PlayerObject::copyAttributes(PlayerObject*), inicio de la función. De la firma se lee el offset
// de la "posición anterior" del jugador, que el juego actualiza al final de cada tick.
inline constexpr char kCopyAttributes[] =
    "48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 20 C6 81 ?? ?? ?? ?? 01 48 8B F2 48 8B 82 ?? ?? ?? ?? "
    "48 8B F9 48 89 81 ?? ?? ?? ?? 48 81 C2 ?? ?? ?? ?? 48 8D 4C 24 30 FF 15 ?? ?? ?? ?? 48 8D 8F ?? ?? ?? ?? "
    "48 8D 54 24 30 FF 15 ?? ?? ?? ?? 48 8B 06 48 8B CE 48 8B 1F FF 90 ?? ?? ?? ?? 48 8B D0 48 8B CF "
    "FF 93 ?? ?? ?? ?? 48 8D 96 ?? ?? ?? ?? 48 8D 4C 24 30 FF 15 ?? ?? ?? ?? 48 8D 8F ?? ?? ?? ??";
inline constexpr int kCopyAttributesLastPos = 108;

// PlayerObject::updateRotation(float), inicio de la función.
inline constexpr char kUpdateRotation[] =
    "40 57 48 83 EC 50 80 B9 ?? ?? ?? ?? 00 48 8B F9 0F 29 74 24 40 0F 28 F1 0F 85 ?? ?? ?? ?? "
    "80 B9 ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ??";

// PlayerObject::pushButton(PlayerButton) y releaseButton(PlayerButton), inicio de cada función.
inline constexpr char kPushButton[] =
    "40 53 57 48 83 EC 28 80 B9 ?? ?? ?? ?? 00 8B FA 48 8B D9 75 18 89 54 24 40 48 81 C1 ?? ?? ?? ?? "
    "48 8D 54 24 40 E8 ?? ?? ?? ??";
inline constexpr char kReleaseButton[] =
    "48 89 5C 24 08 57 48 83 EC 20 8B FA 48 8B D9 83 FA 05 75 2C BA 02 00 00 00 E8 ?? ?? ?? ?? "
    "BA 03 00 00 00 48 8B CB E8 ?? ?? ?? ??";

// GJBaseGameLayer::checkCollisions(PlayerObject*, float, bool ignoreDamage), inicio de la función.
// Con ignoreDamage, al tocar un peligro devuelve 1 sin llamar a destroyPlayer.
inline constexpr char kCheckCollisions[] =
    "48 8B C4 44 88 48 20 F3 0F 11 50 18 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 68 B8 48 81 EC 08 01 00 00 "
    "0F 29 70 A8 0F 29 78 98 44 0F 29 40 88";

// GJBaseGameLayer::collisionCheckObjects(PlayerObject*, vector<GameObject*>*, int, float), inicio.
// Recorre los objetos cerca del jugador: sólidos y peligros los apunta para checkCollisions, y
// los especiales (portales, anillos, monedas, triggers) los activa. Hook: para el jugador simulado
// solo le llegan sólidos, peligros y rampas.
inline constexpr char kCollisionCheckObjects[] =
    "48 8B C4 48 89 58 08 4C 89 40 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 F8 FE FF FF "
    "48 81 EC D0 01 00 00 0F 29 70 B8 0F 29 78 A8 44 0F 29 40 98 44 0F 29 48 88 44 0F 29 90 78 FF FF FF";

// Dentro de collisionCheckObjects: lectura del tipo de objeto (GameObject::m_objectType).
inline constexpr char kObjectTypeRead[] =
    "48 8B 07 4A 8B 3C E0 48 89 7C 24 58 8B 87 ?? ?? ?? ?? 83 F8 07 0F 84 ?? ?? ?? ??";
inline constexpr int kObjectTypeOffset = 14;

// PlayerObject::playSpiderDashEffect(CCPoint, CCPoint), inicio. Hook: nada para el jugador simulado.
inline constexpr char kSpiderDashEffect[] =
    "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 54 41 55 41 56 41 57 48 83 EC 50 0F 29 74 24 40 "
    "0F 29 7C 24 30 4D 8B F8 48 8B EA 48 8B F9 44 0F B6 A9 ?? ?? ?? ??";

// GJBaseGameLayer::update: al empezar cada tick vacía los 4 registros de colisiones del jugador 1
// (CCDictionary) y mueve dos pares de campos. El simulador repite eso; de la firma se leen:
inline constexpr char kCollisionLogReset[] =
    "E8 ?? ?? ?? ?? 48 8B 9E ?? ?? ?? ?? 48 8B 8B ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8B 8B ?? ?? ?? ?? "
    "FF 15 ?? ?? ?? ?? 48 8B 8B ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8B 8B ?? ?? ?? ?? FF 15 ?? ?? ?? ?? "
    "8B 83 ?? ?? ?? ?? 89 83 ?? ?? ?? ?? 8B 83 ?? ?? ?? ?? 89 83 ?? ?? ?? ?? "
    "48 C7 83 ?? ?? ?? ?? FF FF FF FF 48 C7 83 ?? ?? ?? ?? FF FF FF FF";
inline constexpr int kCollisionLogDicts[4] = {15, 28, 41, 54};       // los 4 CCDictionary
inline constexpr int kCollisionLogCopy[4] = {66, 72, 78, 84};        // origen, destino, origen, destino
inline constexpr int kCollisionLogClear[2] = {91, 102};              // dos int64 que pasan a -1

// GJBaseGameLayer::gameEventTriggered(GJGameEvent, int, int), inicio. Lo llaman el salto, el
// aterrizaje y los choques del jugador (triggers de evento del nivel). Hook: nada mientras se simula.
inline constexpr char kGameEventTriggered[] =
    "4C 8B DC 49 89 5B 10 49 89 6B 18 49 89 73 20 57 41 56 41 57 48 83 EC 60 41 8B E9 45 8B F0 44 8B FA "
    "48 8B F1 41 89 53 08";

// GJBaseGameLayer::destroyObject(GameObject*), inicio. Lo llama el jugador al chocar con un bloque
// rompible. Hook: nada mientras se simula (si no, el jugador simulado rompería bloques de verdad).
inline constexpr char kDestroyObject[] =
    "40 57 48 83 EC 30 48 8B 02 48 8B F9 48 89 5C 24 40 48 8B CA 48 8B DA FF 90 ?? ?? ?? ?? 83 F8 1E "
    "0F 85 ?? ?? ?? ?? 80 BB ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ??";

// En kCurrentPercent: "cmp byte [rcx+X],0" con X = GJBaseGameLayer::m_isPlatformer.
inline constexpr int kCurrentPercentPlatformer = 8;

// --- Fase 10 (Cosmetic). GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---

// GJBaseGameLayer::shakeCamera(float, float, float), inicio. No shake: ret.
inline constexpr char kShakeCamera[] =
    "40 53 48 83 EC 50 0F 29 74 24 40 0F 57 C0 0F 29 7C 24 30 0F 28 F2 0F 28 F9";

// GJBaseGameLayer::lightningFlash(from, to, color, ...), inicio. No portal lightning: ret.
inline constexpr char kLightningFlash[] =
    "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 40 4D 8B F1 49 8B C0 "
    "48 8B EA 48 8B F1 4C 8B C2 48 8D 54 24 20 48 8B C8 FF 15 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ??";

// CCCircleWave::draw() (clase de GD), inicio. Los círculos de portales, anillos y recogidas. ret.
inline constexpr char kCircleWaveDraw[] =
    "40 53 48 83 EC 30 80 B9 ?? ?? ?? ?? 00 48 8B D9 74 10 BA 01 00 00 00 B9 02 03 00 00";

// HardStreak::addPoint(CCPoint), inicio. Sin puntos la estela de la onda queda vacía. ret.
inline constexpr char kHardStreakAddPoint[] =
    "48 89 5C 24 10 55 56 57 48 83 EC 20 48 8B DA 48 8B E9 48 8B 89 ?? ?? ?? ??";

// PlayerObject::activateStreak: "m_streakActive = 1; m_regularTrail->resumeStroke()". Es la única
// llamada a CCMotionStreak::resumeStroke del juego. No trail: la llamada (6 bytes) pasa a NOPs.
inline constexpr char kActivateStreakResume[] =
    "C6 83 ?? ?? ?? ?? 01 FF 15 ?? ?? ?? ?? 80 BB ?? ?? ?? ?? 00 74 6F 48 8B 03";
inline constexpr int kActivateStreakResumeCall = 7;

// PlayerObject::toggleGhostEffect(GhostType), inicio. Al final de la firma, "if (tipo != 1) return"
// tras quitar la estela anterior. No ghost trail: jne -> jmp (nunca se crea).
inline constexpr char kToggleGhostEffect[] =
    "48 89 5C 24 18 57 48 83 EC 20 48 8B D9 39 91 ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 89 91 ?? ?? ?? ?? "
    "48 8B B9 ?? ?? ?? ?? 48 85 FF 74 3D 48 8B 8F ?? ?? ?? ?? 48 85 C9 74 08 48 8B 01 48 8B D7 FF 10 "
    "48 8B CF FF 15 ?? ?? ?? ?? 48 8B CF FF 15 ?? ?? ?? ?? 48 8B 07 48 8B CF FF 90 ?? ?? ?? ?? "
    "48 C7 83 ?? ?? ?? ?? 00 00 00 00 83 BB ?? ?? ?? ?? 01 0F 85 ?? ?? ?? ??";
inline constexpr int kToggleGhostEffectJump = 111;

// PlayerObject::playSpiderDashEffect (la misma firma que kSpiderDashEffect, alargada hasta el
// primer "if"). No spider dash: je -> jmp (sale sin crear el efecto). El parche cae después
// del inicio de la función, así que convive con el hook de Show trajectory.
inline constexpr char kSpiderDashSkip[] =
    "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 54 41 55 41 56 41 57 48 83 EC 50 0F 29 74 24 40 "
    "0F 29 7C 24 30 4D 8B F8 48 8B EA 48 8B F9 44 0F B6 A9 ?? ?? ?? ?? 80 B9 ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ??";
inline constexpr int kSpiderDashSkipJump = 62;

// GJBaseGameLayer::toggleFlipped(bool flip, bool noEffects), inicio. No mirror: hook que pasa
// siempre flip = false.
inline constexpr char kToggleFlipped[] =
    "48 8B C4 48 89 58 18 48 89 68 20 56 48 81 EC 80 00 00 00 41 0F B6 E8 0F B6 F2";

// --- Fase 11 (Bypass, lista ampliada). GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---
// Todas son comprobaciones de "¿lo tienes desbloqueado?" en el botón que abre cada sitio.

// LevelPage::onPlay: "if (requiredCoins <= secretCoins) jugar; si no, aviso". jle -> jmp.
inline constexpr char kMainLevelLock[] = "8B 91 ?? ?? ?? ?? 3B D0 0F 8E ?? ?? ?? ??";
inline constexpr int kMainLevelLockJump = 8;

// OptionsLayer::onSecretVault (The Vault): "if (getStat(...) < 10) aviso". El jl (6 bytes) pasa a NOPs.
inline constexpr char kVaultLock[] =
    "48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 F8 0A 0F 8C ?? ?? ?? ??";
inline constexpr int kVaultLockJump = 22;

// CreatorLayer::onSecretVault (Vault of Secrets): "if (getStat(...) < 50) aviso". jl corto -> NOPs.
inline constexpr char kVaultOfSecretsLock[] =
    "48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 F8 32 7C 30";
inline constexpr int kVaultOfSecretsLockJump = 22;

// CreatorLayer::onTreasureRoom: "if (getUGV(...)) entrar; si no, pedir 5 llaves". je corto -> NOPs.
inline constexpr char kTreasureRoomLock[] = "48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 32";
inline constexpr int kTreasureRoomLockJump = 14;

// SecretLayer2::onDoor (sótano del Keymaster): "if (!getUGV(...)) aviso". je (6 bytes) -> NOPs.
inline constexpr char kBasementLock[] =
    "48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? "
    "48 8B F8 FF 15 ?? ?? ?? ?? 48 89 B8 ?? ?? ?? ??";
inline constexpr int kBasementLockJump = 21;

// SecretLayer2::onSecretLevel (The Challenge): "if (getStat(...) < 200) aviso". jl (6 bytes) -> NOPs.
inline constexpr char kChallengeLevelLock[] =
    "48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 3D C8 00 00 00 0F 8C ?? ?? ?? ?? "
    "48 8B 0D ?? ?? ?? ?? 48 85 C9 75 31 B9 90 06 00 00";
inline constexpr int kChallengeLevelLockJump = 24;

// SecretRewardsLayer::onShop: las 4 tiendas con candado hacen "if (getUGV(...)) abrir tienda".
// Los cuatro jne pasan a jmp (dos largos y dos cortos).
inline constexpr char kShopLocks[] =
    "48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 48 8B CF 48 8B 5C 24 30 "
    "48 83 C4 20 5F E9 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 85 C9 75 32 B9 E8 06 00 00 E8 ?? ?? ?? ?? "
    "48 89 44 24 38 48 8B C8 E8 ?? ?? ?? ?? 90 48 89 05 ?? ?? ?? ?? 48 8B 08 48 8B 51 48 48 8B C8 FF D2 "
    "48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 48 8B CF 48 8B 5C 24 30 "
    "48 83 C4 20 5F E9 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 85 C9 75 32 B9 E8 06 00 00 E8 ?? ?? ?? ?? "
    "48 89 44 24 38 48 8B C8 E8 ?? ?? ?? ?? 90 48 89 05 ?? ?? ?? ?? 48 8B 08 48 8B 51 48 48 8B C8 FF D2 "
    "48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 75 72 48 8B CF 48 8B 5C 24 30 "
    "48 83 C4 20 5F E9 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 85 C9 75 32 B9 E8 06 00 00 E8 ?? ?? ?? ?? "
    "48 89 44 24 38 48 8B C8 E8 ?? ?? ?? ?? 90 48 89 05 ?? ?? ?? ?? 48 8B 08 48 8B 51 48 48 8B C8 FF D2 "
    "48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 75 12";
inline constexpr int kShopLockJumps[4] = {0x15, 0x79, 0xDD, 0x13D}; // largo, largo, corto, corto

// SliderTouchLogic::ccTouchMoved: limita el pulgar a [-largo/2, largo/2]. Los dos jbe cortos
// (por debajo y por encima) pasan a jmp: el valor del deslizador puede salir de 0..1.
inline constexpr char kSliderClamp[] =
    "F3 0F 10 8B ?? ?? ?? ?? F3 0F 10 1D ?? ?? ?? ?? 0F 28 D1 0F 57 15 ?? ?? ?? ?? F3 0F 59 D3 0F 2F D0 76 17 "
    "84 C0 74 08 0F 28 C2 0F 57 C9 EB 37 0F 57 C0 0F 28 CA 0F 28 D0 EB 2C F3 0F 59 CB 0F 2F C1 76 11 84 C0 74 08 "
    "0F 28 D1";
inline constexpr int kSliderClampLow = 33;
inline constexpr int kSliderClampHigh = 65;

// --- Fase 12 (Creator, lista ampliada). GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---

// EditorUI::onCreateObject: "if (typeExistsAtPosition(...)) no crear" (jne largo en +31) y, para
// objetos personalizados (id < 0), un bucle que busca otro en la misma posición (jns en +43).
// Place over: el jne pasa a NOPs y el jns a jmp (se salta el bucle).
inline constexpr char kPlaceOver[] =
    "4C 8B C0 F3 0F 11 74 24 28 44 88 74 24 20 44 0F B6 CF 41 8B D5 48 8B CB E8 ?? ?? ?? ?? 84 C0 "
    "0F 85 ?? ?? ?? ?? 45 33 F6 45 85 ED 0F 89 ?? ?? ?? ??";
inline constexpr int kPlaceOverExists = 31;
inline constexpr int kPlaceOverCustom = 43;

// PauseLayer::customSetup: el botón de editar solo sale si el nivel es del editor (tipo 2):
// "if (tipo == 2 || ...)". je corto en +28 -> jmp.
inline constexpr char kPauseEditButton[] =
    "48 8B 15 ?? ?? ?? ?? 48 8B 82 ?? ?? ?? ?? 48 8B 88 ?? ?? ?? ?? 83 B9 ?? ?? ?? ?? 02 74 10 8B 82 ?? ?? ?? ?? "
    "2B 82 ?? ?? ?? ?? 85 C0 7E 74";
inline constexpr int kPauseEditButtonJump = 28;

// PauseLayer::onEdit: la misma comprobación antes de ir al editor. je corto en +28 -> jmp.
inline constexpr char kPauseOnEdit[] =
    "48 8B 15 ?? ?? ?? ?? 48 8B 82 ?? ?? ?? ?? 48 8B 88 ?? ?? ?? ?? 83 B9 ?? ?? ?? ?? 02 74 14 8B 82 ?? ?? ?? ??";
inline constexpr int kPauseOnEditJump = 28;

// SongSelectNode::audioNext y audioPrevious (seguidas): limitan el tema por defecto a 0..21 (0x15).
// Default song bypass: los tres "21" pasan a 40 (0x28), el último que conoce LevelTools::getAudioTitle.
inline constexpr char kSongSelectArrows[] =
    "8B 81 ?? ?? ?? ?? 83 F8 15 74 25 FF C0 C6 81 ?? ?? ?? ?? 01 BA 15 00 00 00 3B C2 0F 4F C2 33 D2 85 C0 "
    "0F 48 C2 89 81 ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC "
    "8B 81 ?? ?? ?? ?? 85 C0 74 25 FF C8 C6 81 ?? ?? ?? ?? 01 BA 15 00 00 00 3B C2";
inline constexpr int kSongSelectArrowsLimits[3] = {8, 21, 84}; // byte bajo de cada "21"

// EditorOptionsLayer: flechas de "Buttons Per Row" (6..12) y "Button Rows" (2..3). El editor solo
// impone el mínimo al leerlos (EditorUI). Toolbox button bypass: el máximo sube a 24 y a 6.
inline constexpr char kToolboxPerRow[] =
    "BA 01 00 00 00 B9 FF FF FF FF 3B C2 0F 45 D1 03 93 ?? ?? ?? ?? 83 FA 06 7C 0C 83 FA 0C 7C 0C "
    "BA 0C 00 00 00 EB 05 BA 06 00 00 00";
inline constexpr char kToolboxRows[] =
    "BA 01 00 00 00 B9 FF FF FF FF 3B C2 0F 45 D1 03 93 ?? ?? ?? ?? 83 FA 02 7C 0C 83 FA 03 7C 0C "
    "BA 03 00 00 00 EB 05 BA 02 00 00 00";
inline constexpr int kToolboxMaxCompare = 28; // "cmp edx, máximo"
inline constexpr int kToolboxMaxValue = 32;   // "mov edx, máximo"

// --- Fase 13 (Level, lista ampliada). GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---

// PlayLayer::pauseGame: "if (m_levelEnding) return" (bandera que ponen levelComplete,
// playEndAnimationToPos y activatePlatformerEndTrigger). Pause during complete: el jne -> NOPs.
inline constexpr char kPauseGameEnding[] =
    "0F B6 F2 48 8B D9 80 B9 ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 80 B9 ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ??";
inline constexpr int kPauseGameEndingJump = 13;

// PlayLayer::storeCheckpoint: guarda como mucho 500 checkpoints (1 en plataformas sin práctica) y
// borra los más viejos. Checkpoint limit bypass: el 500 (imm32 en +24) pasa a 100000.
inline constexpr char kCheckpointLimit[] =
    "80 BB ?? ?? ?? ?? 00 74 0E 80 BB ?? ?? ?? ?? 00 BE 01 00 00 00 74 05 BE F4 01 00 00";
inline constexpr int kCheckpointLimitValue = 24;

// GJBaseGameLayer::loadLevelSettings: "platformer = m_levelSettings->m_platformerMode" y lo copia
// al nivel y a los dos jugadores. Force platformer: "movzx edx, byte [rax+X]" (7 bytes, +10) ->
// "mov dl, 1" + NOPs.
inline constexpr char kLoadLevelSettings[] =
    "48 8B 81 ?? ?? ?? ?? 4C 8B C1 0F B6 90 ?? ?? ?? ?? 48 8B 81 ?? ?? ?? ?? 88 91 ?? ?? ?? ??";
inline constexpr int kLoadLevelSettingsPlatformer = 10;

// --- Fase 14 (Cheat Safety). GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---

// PlayLayer::destroyPlayer: antes de guardar el nuevo récord comprueba, entre otras, dos banderas
// de PlayLayer que descartan el intento: "se usaron herramientas" (+25 en la firma) y "se cambió
// la velocidad" (+38). levelComplete y EndLevelLayer miran las mismas. resetLevel las recalcula.
inline constexpr char kAttemptDiscardFlags[] =
    "44 38 BE ?? ?? ?? ?? 74 0D 44 38 BE ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 44 38 BE ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? "
    "44 38 BE ?? ?? ?? ??";
inline constexpr int kAttemptDiscardToolsUsed = 25;
inline constexpr int kAttemptDiscardSpeedChanged = 38;

// --- Fase 15 (Status, labels nuevos). GD 2.2091, Steam build 26002420 (exe del 2026-10-16). ---

// EndLevelLayer::customSetup: "Jumps: %i" con m_playLayer->m_jumps. De la firma (+26) se lee el
// desplazamiento de los saltos del intento en PlayLayer.
inline constexpr char kEndLevelJumps[] =
    "F3 0F 10 44 24 74 F3 41 0F 58 C0 F3 0F 11 44 24 74 49 8B 97 ?? ?? ?? ?? 8B 92 ?? ?? ?? ?? "
    "48 8D 0D ?? ?? ?? ??";
inline constexpr int kEndLevelJumpsOffset = 26;

} // namespace core::signatures
