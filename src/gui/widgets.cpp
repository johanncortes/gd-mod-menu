#include "widgets.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include <imgui.h>

#include "gui/keybinds.hpp"

namespace gui {

namespace {

std::string g_search;   // en minúsculas
bool g_filter = false;  // dentro de beginSearchFilter / endSearchFilter
int g_matches = 0;

std::string lower(const char* text) {
    std::string out(text);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

// true si la opción queda oculta por la búsqueda (y cuenta las que coinciden)
bool hiddenBySearch(const char* label) {
    if (!g_filter || g_search.empty()) return false;
    if (lower(label).find(g_search) == std::string::npos) return true;
    ++g_matches;
    return false;
}

} // namespace

void setSearch(const char* text) { g_search = lower(text); }
bool searching() { return !g_search.empty(); }

void beginSearchFilter() {
    g_filter = true;
    g_matches = 0;
}

int endSearchFilter() {
    g_filter = false;
    return g_matches;
}

bool toggle(const char* label, bool available, bool& enabled) {
    if (hiddenBySearch(label)) { // sin dibujar, pero su tecla sigue funcionando
        if (!available || !keybinds::consume(label)) return false;
        enabled = !enabled;
        return true;
    }
    ImGui::BeginDisabled(!available);
    bool changed = ImGui::Checkbox(label, &enabled);
    ImGui::EndDisabled();
    if (!available) {
        ImGui::SameLine();
        ImGui::TextDisabled("(no disponible)");
        return changed;
    }
    keybinds::contextMenu(label);
    if (keybinds::consume(label)) {
        enabled = !enabled;
        changed = true;
    }
    return changed;
}

bool button(const char* label, bool available) {
    if (hiddenBySearch(label)) return available && keybinds::consume(label);
    ImGui::BeginDisabled(!available);
    bool pressed = ImGui::Button(label);
    ImGui::EndDisabled();
    if (!available) {
        ImGui::SameLine();
        ImGui::TextDisabled("(no disponible)");
        return false;
    }
    keybinds::contextMenu(label);
    return keybinds::consume(label) || pressed;
}

bool toggleValue(const char* label, bool available, bool& enabled, float& value,
                 float min, float max, const char* format) {
    if (g_filter && !g_search.empty() && lower(label).find(g_search) == std::string::npos)
        return toggle(label, available, enabled); // oculta: toggle solo atiende la tecla
    ImGui::PushID(label);
    bool changed = toggle(label, available, enabled);

    if (available) {
        const float width = 70.f;
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - width);
        ImGui::SetNextItemWidth(width);
        // Lo que se va escribiendo se guarda aparte y solo se aplica al terminar de editar
        float* pending = ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("pending"), value);
        float typed = value;
        if (ImGui::InputFloat("##value", &typed, 0.f, 0.f, format))
            *pending = typed;
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            value = std::clamp(*pending, min, max);
            changed = true;
        }
    }
    ImGui::PopID();
    return changed;
}

} // namespace gui
