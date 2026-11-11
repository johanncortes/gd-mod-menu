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

} // namespace core::signatures
