#include "menu.hpp"

#include <windows.h>

#include <algorithm>
#include <cfloat>
#include <vector>

#include <imgui.h>

#include "core/log.hpp"
#include "features/features.hpp"
#include "gui/keybinds.hpp"
#include "gui/theme.hpp"
#include "gui/widgets.hpp"

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
    {"Cosmetic", features::drawCosmetic},
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

namespace {

constexpr size_t kWindowCount = sizeof(kWindows) / sizeof(kWindows[0]);
ImVec2 g_sizes[kWindowCount] = {}; // tamaño de cada ventana en el último frame
int g_arrangeFrames = 0;           // > 0: hay que recolocar (se esperan dos frames: ImGui tarda en medir una ventana nueva)
int g_matches[kWindowCount] = {};  // opciones que coincidieron con la búsqueda en el último frame
char g_searchText[64] = {};

// Cuadro de búsqueda (arriba en el centro la primera vez)
void drawSearchBox() {
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, 8.f}, ImGuiCond_FirstUseEver, {0.5f, 0.f});
    if (ImGui::Begin("Search", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::SetNextItemWidth(220.f * io.FontGlobalScale);
        if (ImGui::InputTextWithHint("##search", "Buscar opción...", g_searchText, sizeof(g_searchText)))
            setSearch(g_searchText);
        if (g_searchText[0]) {
            ImGui::SameLine();
            if (ImGui::SmallButton("x")) {
                g_searchText[0] = 0;
                setSearch("");
            }
        }
    }
    ImGui::End();
}

// Coloca las ventanas en columnas, de izquierda a derecha, cada una en la columna más baja
// (como el panel de Mega Hack). Usa los tamaños del frame anterior.
void arrange(ImVec2 (&positions)[kWindowCount]) {
    const float gap = 8.f;
    float columnWidth = 0.f;
    for (const ImVec2& size : g_sizes) columnWidth = std::max(columnWidth, size.x);
    const float screenWidth = ImGui::GetIO().DisplaySize.x;
    const int columns = std::max(1, static_cast<int>((screenWidth - gap) / (columnWidth + gap)));
    std::vector<float> bottoms(columns, gap);
    for (size_t i = 0; i < kWindowCount; ++i) {
        const int column = static_cast<int>(std::min_element(bottoms.begin(), bottoms.end()) - bottoms.begin());
        positions[i] = {gap + column * (columnWidth + gap), bottoms[column]};
        bottoms[column] += g_sizes[i].y + gap;
    }
}

} // namespace

void resetWindowPositions() {
    g_arrangeFrames = 3;
}

void drawWindows() {
    drawSearchBox();
    if (theme::takeArrangeRequest()) resetWindowPositions();
    const bool arrangeNow = g_arrangeFrames == 1;
    ImVec2 positions[kWindowCount];
    if (arrangeNow) arrange(positions);

    // El tamaño se ajusta siempre al contenido, con un ancho mínimo
    const float minWidth = 240.f * ImGui::GetIO().FontGlobalScale;
    for (size_t i = 0; i < kWindowCount; ++i) {
        const Window& window = kWindows[i];
        // Buscando: una ventana sin coincidencias en el último frame se dibuja fuera de la
        // pantalla (así se vuelven a contar y sus teclas siguen funcionando)
        if (searching() && g_matches[i] == 0 && !arrangeNow) {
            ImGui::SetNextWindowPos({-10000.f, -10000.f});
            const ImGuiWindowFlags hidden = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;
            if (ImGui::Begin("##search_hidden", nullptr, hidden) && window.draw) {
                beginSearchFilter();
                window.draw();
                g_matches[i] = endSearchFilter();
            }
            ImGui::End();
            continue;
        }
        if (arrangeNow) ImGui::SetNextWindowPos(positions[i], ImGuiCond_Always);
        else ImGui::SetNextWindowPos({8.f + (i % 5) * 250.f, 8.f + (i / 5) * 300.f}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints({minWidth, 0.f}, {FLT_MAX, FLT_MAX});
        if (ImGui::Begin(window.name, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
            beginSearchFilter();
            if (window.draw) window.draw();
            else ImGui::TextDisabled("(vacío)");
            g_matches[i] = endSearchFilter();
        }
        g_sizes[i] = ImGui::GetWindowSize();
        ImGui::End();
    }
    if (g_arrangeFrames > 0) --g_arrangeFrames;
}

} // namespace gui
