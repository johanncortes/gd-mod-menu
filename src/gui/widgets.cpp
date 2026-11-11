#include "widgets.hpp"

#include <algorithm>

#include <imgui.h>

#include "gui/keybinds.hpp"

namespace gui {

bool toggle(const char* label, bool available, bool& enabled) {
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
