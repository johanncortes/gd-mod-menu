#include "menu.hpp"

#include <windows.h>

#include <imgui.h>

#include "core/log.hpp"

namespace gui {

namespace {

bool g_open = false;

// Una ventana por categoría; se irán llenando en las fases siguientes
const char* const kWindows[] = {
    "Global", "Level", "Bypass", "Player", "Bot",
    "Creator", "Labels", "Shortcuts", "Interface",
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
    if (!g_open) return;

    // Posición inicial: rejilla de 5 columnas
    const float w = 180.f, h = 140.f, gap = 10.f;
    int i = 0;
    for (const char* name : kWindows) {
        const float x = gap + (i % 5) * (w + gap);
        const float y = gap + (i / 5) * (h + gap);
        ImGui::SetNextWindowPos({x, y}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({w, h}, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(name))
            ImGui::TextDisabled("(vacío)");
        ImGui::End();
        ++i;
    }
}

} // namespace gui
