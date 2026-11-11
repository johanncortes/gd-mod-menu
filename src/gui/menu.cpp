#include "menu.hpp"

#include <windows.h>

#include <imgui.h>

#include "core/log.hpp"
#include "features/features.hpp"

namespace gui {

namespace {

bool g_open = false;

// Una ventana por categoría; se irán llenando en las fases siguientes
struct Window {
    const char* name;
    void (*draw)(); // nullptr = todavía sin contenido
};
const Window kWindows[] = {
    {"Global", features::drawGlobal},
    {"Level", features::drawLevel},
    {"Bypass", features::drawBypass},
    {"Player", features::drawPlayer},
    {"Bot", nullptr},
    {"Creator", features::drawCreator},
    {"Labels", nullptr},
    {"Shortcuts", nullptr},
    {"Interface", nullptr},
};

// El juego oculta el cursor durante un nivel; en ese caso ImGui dibuja el suyo
bool osCursorHidden() {
    CURSORINFO ci{sizeof(ci)};
    if (!GetCursorInfo(&ci)) return false;
    return !(ci.flags & CURSOR_SHOWING) || !ci.hCursor;
}

} // namespace

void toggleMenu() {
    g_open = !g_open;
    core::logMsg("Panel %s", g_open ? "abierto" : "cerrado");
}
bool isMenuOpen() { return g_open; }

void draw() {
    ImGui::GetIO().MouseDrawCursor = g_open && osCursorHidden();
    features::drawOverlay();
    if (!g_open) return;

    // Posición inicial: rejilla de 5 columnas; el alto se ajusta al contenido
    const float w = 250.f, h = 200.f, gap = 10.f;
    int i = 0;
    for (const Window& window : kWindows) {
        const float x = gap + (i % 5) * (w + gap);
        const float y = gap + (i / 5) * (h + gap);
        ImGui::SetNextWindowPos({x, y}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({w, 0.f}, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(window.name)) {
            if (window.draw) window.draw();
            else ImGui::TextDisabled("(vacío)");
        }
        ImGui::End();
        ++i;
    }
}

} // namespace gui
