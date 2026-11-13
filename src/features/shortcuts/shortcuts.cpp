#include "shortcuts.hpp"

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <cstdint>
#include <cstring>
#include <string>

#include <imgui.h>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/keybinds.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

using PlayLayerFn = void (*)(void* playLayer);
using TogglePracticeFn = void (*)(void* playLayer, bool practice);
using UiHandlerFn = void (*)(void* uiLayer, void* sender);

int32_t g_resetLevelSlot = 0;   // posición de resetLevel() en el vtable de PlayLayer
int32_t g_practiceOffset = 0;   // PlayLayer::m_isPracticeMode
TogglePracticeFn g_togglePractice = nullptr;
// Los manejadores de los botones de checkpoint de la interfaz del nivel. No usan ninguno de
// sus dos argumentos (sacan el PlayLayer de GameManager) y solo actúan en modo práctica.
UiHandlerFn g_placeCheckpoint = nullptr;
UiHandlerFn g_removeCheckpoint = nullptr;

// GJBaseGameLayer::handleButton(down, botón, jugador 1). Se llama a la dirección original: si el
// bot la tiene hookeada, la pulsación pasa por él (y se graba) como una del teclado.
using HandleButtonFn = void (*)(void* gameLayer, bool down, int button, bool isPlayer1);
HandleButtonFn g_handleButton = nullptr;

// Acciones de Utility: texto del botón (y de su tecla), botón de GD (1 salto, 2 izq., 3 der.), jugador
struct PlayerAction {
    const char* label;
    int button;
    bool player1;
    bool down = false; // ya enviamos "pulsado" al juego
    bool tap = false;  // clic en el panel: pulsar ahora y soltar en el frame siguiente
};
PlayerAction g_actions[] = {
    {"P1 jump", 1, true}, {"P2 jump", 1, false}, {"P1 left", 2, true}, {"P1 right", 3, true}};

// Uncomplete level: offsets de los porcentajes de GJGameLevel (kSavePercentage), en este orden:
// práctica; normal: valor, semilla, aleatorio; "normal nuevo": aleatorio, semilla, valor
constexpr int kPercentSites[] = {sig::kSavePercentPractice,   sig::kSavePercentNormalValue, sig::kSavePercentNormalSeed,
                                 sig::kSavePercentNormalRandom, sig::kSavePercentNewRandom, sig::kSavePercentNewSeed,
                                 sig::kSavePercentNewValue};
int32_t g_percent[7] = {};
constexpr ULONGLONG kConfirmMs = 3000;
ULONGLONG g_uncompleteArmed = 0; // primera pulsación: espera la segunda
ULONGLONG g_uncompleteDone = 0;

// Pone a 0 los porcentajes guardados del nivel (normal y práctica). Los valores con semilla se
// guardan como valor + semilla. No toca estrellas, monedas, orbes ni la lista de completados.
void uncompleteLevel(void* level) {
    auto field = [level](int index) -> int32_t& {
        return *reinterpret_cast<int32_t*>(static_cast<uint8_t*>(level) + g_percent[index]);
    };
    field(0) = 0;
    field(1) = 0;
    field(3) = field(2);
    field(6) = 0;
    field(4) = field(5);
}

void openFolder(const wchar_t* path) {
    ShellExecuteW(nullptr, L"open", path, nullptr, nullptr, SW_SHOWNORMAL);
}

// Carpeta del exe + "\Resources"
void openResources() {
    wchar_t path[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return;
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) return;
    wcscpy_s(slash, static_cast<size_t>(MAX_PATH - (slash - path)), L"\\Resources");
    openFolder(path);
}

// %LOCALAPPDATA%\GeometryDash (partidas guardadas)
void openSaveFolder() {
    PWSTR local = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local))) {
        std::wstring path = std::wstring(local) + L"\\GeometryDash";
        openFolder(path.c_str());
    }
    CoTaskMemFree(local);
}

int32_t readInt(const uint8_t* at) {
    int32_t value;
    memcpy(&value, at, 4);
    return value;
}

bool plausibleOffset(int32_t value) {
    return value > 0 && value < 0x10000;
}

} // namespace

void Shortcuts::init(bool gameAvailable) {
    if (!gameAvailable) {
        core::logMsg("Shortcuts: no disponibles (GameManager)");
        return;
    }
    HMODULE exe = GetModuleHandleW(nullptr);
    if (const uint8_t* code = core::memory::findPattern(exe, sig::kSwitchStartPos)) {
        g_resetLevelSlot = readInt(code + sig::kSwitchResetLevelSlot);
        m_restart = plausibleOffset(g_resetLevelSlot);
    }
    if (uint8_t* toggle = core::memory::findPattern(exe, sig::kTogglePracticeMode)) {
        g_togglePractice = reinterpret_cast<TogglePracticeFn>(toggle);
        g_practiceOffset = readInt(toggle + sig::kTogglePracticeModeFlag);
        m_practice = plausibleOffset(g_practiceOffset);
    }
    if (const uint8_t* percent = core::memory::findPattern(exe, sig::kSavePercentage)) {
        m_uncomplete = true;
        for (int i = 0; i < 7; ++i) {
            g_percent[i] = readInt(percent + kPercentSites[i]);
            m_uncomplete = m_uncomplete && plausibleOffset(g_percent[i]);
        }
    }
    g_placeCheckpoint = reinterpret_cast<UiHandlerFn>(core::memory::findPattern(exe, sig::kUiOnCheck));
    g_removeCheckpoint = reinterpret_cast<UiHandlerFn>(core::memory::findPattern(exe, sig::kUiOnDeleteCheck));
    m_checkpoints = g_placeCheckpoint && g_removeCheckpoint;
    // Antes que el bot instale su hook (la firma dejaría de coincidir)
    g_handleButton = reinterpret_cast<HandleButtonFn>(core::memory::findPattern(exe, sig::kHandleButton));
    m_buttons = g_handleButton != nullptr;
    core::logMsg("Shortcuts: reiniciar %s, práctica %s, checkpoints %s", m_restart ? "listo" : "no disponible",
                 m_practice ? "listo" : "no disponible", m_checkpoints ? "listos" : "no disponibles");
}

void Shortcuts::update() {
    void* playLayer = m_buttons ? core::game::playLayer() : nullptr;
    for (PlayerAction& action : g_actions) {
        bool want = playLayer && (gui::keybinds::held(action.label) || action.tap);
        if (action.tap && action.down) { // la pulsación corta ya se envió: ahora se suelta
            action.tap = false;
            want = playLayer && gui::keybinds::held(action.label);
        }
        if (want == action.down) continue;
        action.down = want;
        if (playLayer) g_handleButton(playLayer, want, action.button, action.player1);
    }
}

void Shortcuts::drawUI() {
    // Fuera de un nivel los botones (y sus teclas) no hacen nada
    void* playLayer = core::game::playLayer();
    auto* bytes = static_cast<uint8_t*>(playLayer);

    if (gui::button("Restart level", m_restart) && playLayer) {
        void** vtable = *static_cast<void***>(playLayer);
        PlayLayerFn resetLevel;
        memcpy(&resetLevel, reinterpret_cast<uint8_t*>(vtable) + g_resetLevelSlot, sizeof(resetLevel));
        resetLevel(playLayer);
    }
    if (gui::button("Toggle practice mode", m_practice) && playLayer)
        g_togglePractice(playLayer, bytes[g_practiceOffset] == 0);
    if (gui::button("Place checkpoint", m_checkpoints) && playLayer) g_placeCheckpoint(nullptr, nullptr);
    if (gui::button("Remove checkpoint", m_checkpoints) && playLayer) g_removeCheckpoint(nullptr, nullptr);

    ImGui::SeparatorText("Utility");
    // Borra progreso guardado: pide una segunda pulsación
    void* level = core::game::level(playLayer);
    const ULONGLONG now = GetTickCount64();
    const bool armed = g_uncompleteArmed && now - g_uncompleteArmed < kConfirmMs;
    if (gui::button("Uncomplete level", m_uncomplete) && level) {
        if (armed) {
            uncompleteLevel(level);
            g_uncompleteDone = now;
        }
        g_uncompleteArmed = armed ? 0 : now;
    } else if (armed) {
        ImGui::TextDisabled("Pulsa otra vez: el nivel vuelve a 0%%");
    } else if (g_uncompleteDone && now - g_uncompleteDone < kConfirmMs) {
        ImGui::TextDisabled("Porcentajes del nivel puestos a 0%%");
    }
    // Con la tecla asignada la acción dura lo que se mantenga pulsada (Shortcuts::update); el
    // clic hace una pulsación corta. gui::button también devuelve true al pulsar la tecla: se ignora.
    for (PlayerAction& action : g_actions)
        if (gui::button(action.label, m_buttons) && !gui::keybinds::held(action.label) && playLayer) action.tap = true;
    if (gui::button("Abrir carpeta Resources", true)) openResources();
    if (gui::button("Abrir carpeta de partidas (AppData)", true)) openSaveFolder();
}

} // namespace features
