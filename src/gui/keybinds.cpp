#include "keybinds.hpp"

#include <windows.h>

#include <bitset>
#include <cstdio>
#include <map>
#include <set>
#include <string>

#include <imgui.h>
#include <nlohmann/json.hpp>

namespace gui::keybinds {

namespace {

std::map<std::string, int> g_binds; // texto de la opción -> tecla virtual
std::set<std::string> g_fired;      // opciones cuya tecla se pulsó este frame
std::bitset<256> g_down;            // estado de las teclas en el frame anterior
bool g_changed = false;
bool g_capturing = false;           // hay un menú de asignación abierto: no disparar nada
bool g_capturingThisFrame = false;

bool isDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool gameFocused() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

// Teclas que se pueden asignar: ni botones del ratón, ni Tab (abre el panel), ni las
// variantes izquierda/derecha de Shift, Ctrl y Alt (ya cuentan las genéricas)
bool bindable(int vk) {
    return vk >= 0x08 && vk <= 0xFE && vk != VK_TAB && vk != VK_ESCAPE && vk != VK_DELETE && vk != VK_BACK &&
           !(vk >= VK_LSHIFT && vk <= VK_RMENU);
}

std::string keyName(int vk) {
    char name[32] = {};
    UINT scan = MapVirtualKeyA(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
    // Las teclas de cursor y similares necesitan el bit de tecla extendida
    if ((vk >= VK_PRIOR && vk <= VK_DOWN) || vk == VK_INSERT) scan |= 0x100;
    if (scan == 0 || GetKeyNameTextA(static_cast<LONG>(scan << 16), name, sizeof(name)) == 0)
        snprintf(name, sizeof(name), "0x%02X", vk);
    return name;
}

} // namespace

void update() {
    g_capturing = g_capturingThisFrame;
    g_capturingThisFrame = false;
    g_fired.clear();

    // Solo con el juego en primer plano y sin estar escribiendo en un campo del panel
    const bool listen = gameFocused() && !ImGui::GetIO().WantTextInput && !g_capturing;
    std::bitset<256> down;
    for (const auto& [label, vk] : g_binds) {
        down[vk] = listen && isDown(vk);
        if (down[vk] && !g_down[vk]) g_fired.insert(label);
    }
    g_down = down;
}

bool anyFired() { return !g_fired.empty(); }

bool consume(const char* label) {
    return !g_fired.empty() && g_fired.erase(label) > 0;
}

void contextMenu(const char* label) {
    const auto bound = g_binds.find(label);
    if (ImGui::BeginPopupContextItem(label)) {
        g_capturingThisFrame = true;
        static std::bitset<256> held; // teclas que ya estaban pulsadas al abrir el menú
        if (ImGui::IsWindowAppearing())
            for (int vk = 0; vk < 256; ++vk) held[vk] = isDown(vk);

        ImGui::Text("Pulsa una tecla para \"%s\"", label);
        if (bound != g_binds.end())
            ImGui::TextDisabled("Actual: %s   (Supr la quita, Esc cancela)", keyName(bound->second).c_str());
        else
            ImGui::TextDisabled("Sin tecla   (Esc cancela)");

        if (isDown(VK_ESCAPE)) {
            ImGui::CloseCurrentPopup();
        } else if (isDown(VK_DELETE) || isDown(VK_BACK)) {
            if (bound != g_binds.end()) {
                g_binds.erase(bound);
                g_changed = true;
            }
            ImGui::CloseCurrentPopup();
        } else {
            for (int vk = 0; vk < 256; ++vk) {
                const bool down = isDown(vk);
                if (down && !held[vk] && bindable(vk)) {
                    g_binds[label] = vk;
                    g_down[vk] = true; // que esta misma pulsación no dispare la opción
                    g_changed = true;
                    ImGui::CloseCurrentPopup();
                    break;
                }
                held[vk] = down;
            }
        }
        ImGui::EndPopup();
        return;
    }
    if (bound != g_binds.end()) {
        ImGui::SameLine();
        ImGui::TextDisabled("[%s]", keyName(bound->second).c_str());
    }
}

bool takeChanged() {
    const bool changed = g_changed;
    g_changed = false;
    return changed;
}

void load(const nlohmann::json& j) {
    g_binds.clear();
    for (const auto& [label, vk] : j.items())
        if (vk.is_number_integer() && bindable(vk.get<int>())) g_binds[label] = vk.get<int>();
}

void save(nlohmann::json& j) {
    j = nlohmann::json::object();
    for (const auto& [label, vk] : g_binds) j[label] = vk;
}

} // namespace gui::keybinds
