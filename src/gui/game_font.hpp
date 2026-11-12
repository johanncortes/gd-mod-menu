#pragma once
#include <imgui.h>

// Texto con las fuentes de bitmap del propio Geometry Dash (bigFont, goldFont, chatFont).
// Lee los .fnt de la carpeta Resources del juego y dibuja con la textura que usa el juego
// (CCTextureCache de libcocos2d), así que el texto se ve igual que el de GD.
// Si algo falla, dibuja con la fuente del panel y una sombra: nunca deja de mostrar el texto.
// Todo se llama desde el hilo de render (el hilo principal del juego).
namespace gui::gamefont {

enum class Font { Big, Gold, Chat, Panel };
inline constexpr const char* kFontNames[] = {"Big Font (GD)", "Gold Font (GD)", "Chat Font (GD)", "Fuente del panel"};
inline constexpr int kFontCount = 4;

// Aspecto común de todos los textos sobre el juego (labels, StartPos switcher).
struct Style {
    Font font = Font::Big;
    float scale = 0.35f;  // 1.0 = tamaño normal de la fuente en GD (un título)
    float opacity = 1.f;  // 0..1
};
Style& style();

// Alto de una línea y ancho de un texto, en píxeles de pantalla.
float lineHeight();
float textWidth(const char* text);

// Dibuja `text` con la esquina superior izquierda en `pos`. `color` tiñe el texto
// (blanco = sin teñir); la opacidad del estilo se aplica encima.
void draw(ImDrawList* drawList, ImVec2 pos, ImU32 color, const char* text);

// Suelta las texturas retenidas (al descargar la DLL).
void shutdown();

} // namespace gui::gamefont
