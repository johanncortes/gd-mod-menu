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

} // namespace core::signatures
