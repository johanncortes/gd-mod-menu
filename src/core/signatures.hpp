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

} // namespace core::signatures
