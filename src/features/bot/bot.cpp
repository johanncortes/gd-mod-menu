#include "bot.hpp"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <MinHook.h>
#include <imgui.h>
#include <nlohmann/json.hpp>

#include "config/config.hpp"
#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

enum class Mode { Disabled, Record, Playback };

struct Input {
    uint32_t tick;
    int button;   // 1 = saltar, 2 = izquierda, 3 = derecha
    bool player1;
    bool down;
};

using ProcessCommandsFn = void (*)(void* gameLayer, float dt, bool isHalfTick, bool isLastTick);
using HandleButtonFn = void (*)(void* gameLayer, bool down, int button, bool isPlayer1);

ProcessCommandsFn g_origProcessCommands = nullptr;
HandleButtonFn g_origHandleButton = nullptr;
int32_t g_tickOffset = 0; // GJBaseGameLayer: contador de ticks de física, leído de la firma

// Todo esto se toca solo desde el hilo del juego
Mode g_mode = Mode::Disabled;
bool g_ignoreInputs = true; // al reproducir, descartar los inputs del usuario
std::vector<Input> g_inputs; // ordenados por tick
size_t g_next = 0;           // reproducción: siguiente input por lanzar
uint32_t g_lastTick = 0;     // tick del último paso visto
bool g_inStep = false;       // estamos dentro de processCommands
uint32_t g_stepTick = 0;     // tick del paso en curso
bool g_injecting = false;    // el handleButton en curso lo lanza el bot

std::string g_status; // último resultado de guardar/cargar, para el panel
char g_name[64] = "replay";

uint32_t currentTick(void* gameLayer) {
    uint32_t tick;
    memcpy(&tick, static_cast<const uint8_t*>(gameLayer) + g_tickOffset, sizeof(tick));
    return tick;
}

// El tick retrocedió (reinicio o vuelta a un checkpoint): lo grabado de ahí en adelante
// pertenece a un futuro que ya no existe.
void rewind(uint32_t tick) {
    if (g_mode == Mode::Record) {
        const auto firstDiscarded = std::lower_bound(g_inputs.begin(), g_inputs.end(), tick,
                                                     [](const Input& input, uint32_t t) { return input.tick < t; });
        g_inputs.erase(firstDiscarded, g_inputs.end());
        // El juego suelta todos los botones al reaparecer: si en lo grabado alguno quedó
        // pulsado, se apunta que se suelta aquí.
        for (int button = 1; button <= 3; ++button) {
            for (const bool player1 : {true, false}) {
                const auto last = std::find_if(g_inputs.rbegin(), g_inputs.rend(), [&](const Input& input) {
                    return input.button == button && input.player1 == player1;
                });
                if (last != g_inputs.rend() && last->down) g_inputs.push_back({tick, button, player1, false});
            }
        }
    } else if (g_mode == Mode::Playback) {
        g_next = static_cast<size_t>(std::lower_bound(g_inputs.begin(), g_inputs.end(), tick,
                                                      [](const Input& input, uint32_t t) { return input.tick < t; }) -
                                     g_inputs.begin());
    }
}

// GJBaseGameLayer::processCommands: un paso de física. Dentro, el juego pasa los inputs
// pendientes del usuario a handleButton.
void processCommandsHook(void* gameLayer, float dt, bool isHalfTick, bool isLastTick) {
    if (g_mode == Mode::Disabled || gameLayer != core::game::playLayer()) {
        g_origProcessCommands(gameLayer, dt, isHalfTick, isLastTick);
        return;
    }
    const uint32_t tick = currentTick(gameLayer);
    if (tick < g_lastTick) rewind(tick);
    g_lastTick = tick;

    g_inStep = true;
    g_stepTick = tick;
    g_origProcessCommands(gameLayer, dt, isHalfTick, isLastTick);
    g_inStep = false;

    if (g_mode == Mode::Playback) {
        g_injecting = true;
        while (g_next < g_inputs.size() && g_inputs[g_next].tick <= tick) {
            const Input& input = g_inputs[g_next++];
            g_origHandleButton(gameLayer, input.down, input.button, input.player1);
        }
        g_injecting = false;
    }
}

// GJBaseGameLayer::handleButton: el juego aplica una pulsación (o soltar) a un jugador
void handleButtonHook(void* gameLayer, bool down, int button, bool isPlayer1) {
    if (gameLayer == core::game::playLayer()) {
        if (g_mode == Mode::Playback && g_ignoreInputs && !g_injecting) return;
        if (g_mode == Mode::Record) {
            // Fuera de un paso, el input cuenta para el paso siguiente
            const uint32_t tick = g_inStep ? g_stepTick : currentTick(gameLayer);
            g_inputs.push_back({tick, button, isPlayer1, down});
        }
    }
    g_origHandleButton(gameLayer, down, button, isPlayer1);
}

void setMode(Mode mode) {
    g_mode = mode;
    g_next = 0;
    g_lastTick = 0;
    // Al empezar a grabar o reproducir a mitad de nivel, colocarse en el tick actual
    if (void* playLayer = core::game::playLayer(); playLayer && mode != Mode::Disabled) {
        g_lastTick = currentTick(playLayer);
        rewind(g_lastTick);
    }
}

std::filesystem::path replayDir() {
    return std::filesystem::path(config::siblingPath("replays"));
}

// Nombre de archivo seguro: letras, números, espacio, guion y guion bajo
std::string safeName() {
    std::string name;
    for (const char* c = g_name; *c; ++c)
        name += (isalnum(static_cast<unsigned char>(*c)) || *c == ' ' || *c == '-' || *c == '_') ? *c : '_';
    return name.empty() ? "replay" : name;
}

std::filesystem::path replayPath() {
    return replayDir() / (safeName() + ".json");
}

void saveReplay() {
    std::error_code ec;
    std::filesystem::create_directories(replayDir(), ec);
    nlohmann::json inputs = nlohmann::json::array();
    for (const Input& input : g_inputs)
        inputs.push_back({input.tick, input.button, input.player1 ? 1 : 2, input.down ? 1 : 0});
    const nlohmann::json data = {{"format", "gdmenu-replay"}, {"version", 1}, {"inputs", inputs}};
    std::ofstream file(replayPath(), std::ios::trunc);
    file << data.dump();
    g_status = file ? "Guardado: " + safeName() + ".json" : "No se pudo guardar";
}

void loadReplay() {
    std::ifstream file(replayPath());
    const nlohmann::json data = file ? nlohmann::json::parse(file, nullptr, false) : nlohmann::json();
    const auto inputs = data.is_object() ? data.find("inputs") : data.end();
    if (!data.is_object() || data.value("format", "") != "gdmenu-replay" || inputs == data.end() || !inputs->is_array()) {
        g_status = "No se pudo cargar " + safeName() + ".json";
        return;
    }
    std::vector<Input> loaded;
    for (const nlohmann::json& item : *inputs) {
        if (!item.is_array() || item.size() != 4 || !item[0].is_number_unsigned() || !item[1].is_number_integer() ||
            !item[2].is_number_integer() || !item[3].is_number_integer())
            continue;
        const int button = item[1].get<int>();
        if (button < 1 || button > 3) continue;
        loaded.push_back({item[0].get<uint32_t>(), button, item[2].get<int>() != 2, item[3].get<int>() != 0});
    }
    std::stable_sort(loaded.begin(), loaded.end(), [](const Input& a, const Input& b) { return a.tick < b.tick; });
    g_inputs = std::move(loaded);
    setMode(Mode::Disabled);
    g_status = "Cargado: " + std::to_string(g_inputs.size()) + " inputs";
}

void deleteReplay() {
    std::error_code ec;
    g_status = std::filesystem::remove(replayPath(), ec) ? "Borrado: " + safeName() + ".json" : "No existe ese replay";
}

std::vector<std::string> listReplays() {
    std::vector<std::string> names;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(replayDir(), ec))
        if (entry.path().extension() == ".json") names.push_back(entry.path().stem().string());
    std::sort(names.begin(), names.end());
    return names;
}

bool hookAt(void* target, void* hook, void** original) {
    return target && MH_CreateHook(target, hook, original) == MH_OK && MH_EnableHook(target) == MH_OK;
}

} // namespace

void Bot::init(bool gameAvailable) {
    if (!gameAvailable) {
        core::logMsg("Bot: no disponible (GameManager)");
        return;
    }
    HMODULE exe = GetModuleHandleW(nullptr);
    uint8_t* processCommands = core::memory::findPattern(exe, sig::kProcessCommands);
    uint8_t* handleButton = core::memory::findPattern(exe, sig::kHandleButton);
    if (!processCommands || !handleButton) {
        core::logMsg("Bot: no disponible (firma no encontrada: %s%s)", processCommands ? "" : "kProcessCommands ",
                     handleButton ? "" : "kHandleButton");
        return;
    }
    memcpy(&g_tickOffset, processCommands + sig::kProcessCommandsTick, 4);
    if (g_tickOffset <= 0 || g_tickOffset > 0x10000) {
        core::logMsg("Bot: no disponible (offset inesperado 0x%X)", g_tickOffset);
        return;
    }
    m_available = hookAt(processCommands, &processCommandsHook, reinterpret_cast<void**>(&g_origProcessCommands)) &&
                  hookAt(handleButton, &handleButtonHook, reinterpret_cast<void**>(&g_origHandleButton));
    core::logMsg("Bot: %s", m_available ? "listo" : "no disponible (hook)");
}

void Bot::disable() {
    g_mode = Mode::Disabled;
}

bool Bot::active() const {
    return m_available && g_mode == Mode::Playback;
}

bool Bot::drawUI() {
    if (!m_available) {
        ImGui::TextDisabled("Bot (no disponible)");
        return false;
    }
    bool changed = false;

    // Modo: se puede cambiar con clic o con una tecla asignada a cada botón
    const Mode before = g_mode;
    Mode mode = g_mode;
    if (ImGui::RadioButton("Disabled", mode == Mode::Disabled)) mode = Mode::Disabled;
    if (ImGui::RadioButton("Record", mode == Mode::Record)) mode = Mode::Record;
    if (ImGui::RadioButton("Playback", mode == Mode::Playback)) mode = Mode::Playback;
    if (mode != before) setMode(mode);

    changed |= gui::toggle("Ignore inputs", true, g_ignoreInputs);
    ImGui::TextDisabled("%zu inputs", g_inputs.size());
    ImGui::Separator();

    // Replays guardados
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::InputText("##name", g_name, sizeof(g_name))) changed = true;
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::BeginCombo("##replays", "Replays guardados")) {
        for (const std::string& name : listReplays()) {
            if (ImGui::Selectable(name.c_str())) {
                strncpy_s(g_name, name.c_str(), _TRUNCATE);
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("New")) {
        g_inputs.clear();
        setMode(g_mode);
        g_status = "Replay vacío";
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) saveReplay();
    ImGui::SameLine();
    if (ImGui::Button("Load")) loadReplay();
    ImGui::SameLine();
    if (ImGui::Button("Delete")) deleteReplay();
    if (!g_status.empty()) ImGui::TextDisabled("%s", g_status.c_str());
    return changed;
}

void Bot::load(const nlohmann::json& j) {
    g_ignoreInputs = j.value("ignore_inputs", g_ignoreInputs);
    const std::string name = j.value("name", std::string(g_name));
    strncpy_s(g_name, name.c_str(), _TRUNCATE);
}

void Bot::save(nlohmann::json& j) const {
    j = {{"ignore_inputs", g_ignoreInputs}, {"name", g_name}};
}

} // namespace features
