#include "menu.hpp"

#include <windows.h>

#include <cfloat>

#include <imgui.h>

#include "core/log.hpp"
#include "features/features.hpp"
#include "gui/keybinds.hpp"

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
    {"Bot", features::drawBot},
    {"Creator", features::drawCreator},
    {"Labels", features::drawLabels},
    {"Shortcuts", features::drawShortcuts},
    {"Interface", features::drawInterface},
};

// El juego oculta el cursor durante un nivel; en ese caso ImGui dibuja el suyo
bool osCursorHidden() {
    CURSORINFO ci{sizeof(ci)};
    if (!GetCursorInfo(&ci)) return false;
    return !(ci.flags & CURSOR_SHOWING) || !ci.hCursor;
}

} // namespace

void drawWindows();

void toggleMenu() {
    g_open = !g_open;
    core::logMsg("Panel %s", g_open ? "abierto" : "cerrado");
}
bool isMenuOpen() { return g_open; }

void draw() {
    ImGui::GetIO().MouseDrawCursor = g_open && osCursorHidden();
    features::drawOverlay();
    if (g_open) drawWindows();

    // Teclas asignadas que nadie atendió (panel cerrado o ventana plegada): las opciones se
    // dibujan una vez en una ventana invisible, y así cada una reacciona a su tecla con el
    // mismo código que cuando se hace clic.
    if (keybinds::anyFired()) {
        ImGui::SetNextWindowPos({-10000.f, -10000.f});
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                       ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;
        if (ImGui::Begin("##keybinds", nullptr, flags))
            for (const Window& window : kWindows)
                if (window.draw) window.draw();
        ImGui::End();
    }
}

static bool g_resetPositions = false;

void resetWindowPositions() {
    g_resetPositions = true;
}

void drawWindows() {
    // Posición inicial: rejilla organizada en 2 filas limpias sin solapamiento.
    // Fila 0 (y = 10): Global, Level, Bypass, Player, Bot (5 ventanas)
    // Fila 1 (y = 275): Creator, Labels, Shortcuts, Interface (4 ventanas)
    const float scale = ImGui::GetIO().FontGlobalScale;
    const float w = 240.f;
    const float gapX = 10.f;
    const float row0_Y = 10.f;
    const float row1_Y = 275.f;
    const ImGuiCond cond = g_resetPositions ? ImGuiCond_Always : ImGuiCond_FirstUseEver;

    int i = 0;
    for (const Window& window : kWindows) {
        float x = 0.f;
        float y = 0.f;
        if (i < 5) {
            x = gapX + i * (w + gapX);
            y = row0_Y;
        } else {
            const int col = i - 5;
            x = gapX + col * (w + gapX);
            y = row1_Y;
        }

        ImGui::SetNextWindowPos({x, y}, cond);
        ImGui::SetNextWindowSizeConstraints({w * scale, 0.f}, {FLT_MAX, FLT_MAX});
        if (ImGui::Begin(window.name, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
            if (window.draw) window.draw();
            else ImGui::TextDisabled("(vacío)");
        }
        ImGui::End();
        ++i;
    }
    g_resetPositions = false;
}

} // namespace gui
