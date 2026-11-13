#include "pause_tweaks.hpp"

#include <windows.h>

#include <cstdio>

#include <MinHook.h>
#include <imgui.h>
#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "features/global/speedhack.hpp"
#include "gui/game_font.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

constexpr ULONGLONG kConfirmMs = 3000;   // margen para la segunda pulsación
constexpr ULONGLONG kCountdownMs = 3000; // cuenta atrás al reanudar

// --- Confirmaciones ---
using PauseHandlerFn = void (*)(void* pauseLayer, void* sender);
enum Button { Restart, FullRestart, PracticeOn, PracticeOff, ButtonCount };
PauseHandlerFn g_orig[ButtonCount] = {};
ULONGLONG g_armed[ButtonCount] = {}; // primera pulsación de cada botón
const char* g_notice = nullptr;
ULONGLONG g_noticeAt = 0;
bool g_confirmRestart = false;
bool g_confirmPractice = false;

// true = la pulsación sigue hacia el juego
bool confirmed(Button button, bool enabled, const char* notice) {
    if (!enabled) return true;
    const ULONGLONG now = GetTickCount64();
    if (g_armed[button] && now - g_armed[button] < kConfirmMs) {
        g_armed[button] = 0;
        g_noticeAt = 0;
        return true;
    }
    g_armed[button] = now;
    g_notice = notice;
    g_noticeAt = now;
    return false;
}

void onRestartHook(void* pauseLayer, void* sender) {
    if (confirmed(Restart, g_confirmRestart, "Pulsa otra vez para reiniciar")) g_orig[Restart](pauseLayer, sender);
}
void onRestartFullHook(void* pauseLayer, void* sender) {
    if (confirmed(FullRestart, g_confirmRestart, "Pulsa otra vez para reiniciar desde el principio"))
        g_orig[FullRestart](pauseLayer, sender);
}
void onPracticeHook(void* pauseLayer, void* sender) {
    if (confirmed(PracticeOn, g_confirmPractice, "Pulsa otra vez para entrar en practica"))
        g_orig[PracticeOn](pauseLayer, sender);
}
void onNormalHook(void* pauseLayer, void* sender) {
    if (confirmed(PracticeOff, g_confirmPractice, "Pulsa otra vez para salir de practica"))
        g_orig[PracticeOff](pauseLayer, sender);
}

// --- Pause countdown ---
using ResumeFn = void (*)(void* playLayer);
ResumeFn g_origResume = nullptr;
bool g_countdown = false;
ULONGLONG g_countdownEnd = 0; // 0 = no hay cuenta atrás en marcha

void endCountdown() {
    if (!g_countdownEnd) return;
    g_countdownEnd = 0;
    setGameAudioPaused(false, kAudioPauseCountdown);
}

void resumeHook(void* playLayer) {
    g_origResume(playLayer);
    if (!g_countdown) return;
    g_countdownEnd = GetTickCount64() + kCountdownMs;
    setGameAudioPaused(true, kAudioPauseCountdown);
}

// Antes de GJBaseGameLayer::update: el nivel no avanza durante la cuenta atrás
bool countdownFilter(void* gameLayer, float&) {
    return !g_countdownEnd || gameLayer != core::game::playLayer();
}

bool hookAt(void* target, void* hook, void** original) {
    return target && MH_CreateHook(target, hook, original) == MH_OK && MH_EnableHook(target) == MH_OK;
}

// Texto centrado con la fuente de GD, a `scale` veces el tamaño de los labels
void drawCentered(const char* text, float y, float scale) {
    gui::gamefont::Style& style = gui::gamefont::style();
    const gui::gamefont::Style saved = style;
    style.scale = saved.scale * scale;
    style.opacity = 1.f;
    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    const ImVec2 pos{(screen.x - gui::gamefont::textWidth(text)) * 0.5f, screen.y * y - gui::gamefont::lineHeight() * 0.5f};
    gui::gamefont::draw(ImGui::GetBackgroundDrawList(), pos, IM_COL32(255, 255, 255, 255), text);
    style = saved;
}

} // namespace

void PauseTweaks::init(bool gameAvailable) {
    HMODULE exe = GetModuleHandleW(nullptr);
    // Todas las firmas antes de instalar ningún hook
    void* restart = core::memory::findPattern(exe, sig::kPauseOnRestart);
    void* fullRestart = core::memory::findPattern(exe, sig::kPauseOnRestartFull);
    void* practiceOn = core::memory::findPattern(exe, sig::kPauseOnPracticeMode);
    void* practiceOff = core::memory::findPattern(exe, sig::kPauseOnNormalMode);
    void* resume = core::memory::findPattern(exe, sig::kPlayLayerResume);

    m_restartAvailable = restart && fullRestart &&
                         hookAt(restart, &onRestartHook, reinterpret_cast<void**>(&g_orig[Restart])) &&
                         hookAt(fullRestart, &onRestartFullHook, reinterpret_cast<void**>(&g_orig[FullRestart]));
    m_practiceAvailable = practiceOn && practiceOff &&
                          hookAt(practiceOn, &onPracticeHook, reinterpret_cast<void**>(&g_orig[PracticeOn])) &&
                          hookAt(practiceOff, &onNormalHook, reinterpret_cast<void**>(&g_orig[PracticeOff]));
    m_countdownAvailable = gameAvailable && core::game::levelHooksAvailable() && resume &&
                           hookAt(resume, &resumeHook, reinterpret_cast<void**>(&g_origResume));
    if (m_countdownAvailable) core::game::addUpdateFilter(&countdownFilter);
    core::logMsg("Pausa: confirmar reinicio %s, confirmar práctica %s, cuenta atrás %s",
                 m_restartAvailable ? "listo" : "no disponible", m_practiceAvailable ? "listo" : "no disponible",
                 m_countdownAvailable ? "listo" : "no disponible");
}

void PauseTweaks::apply() {
    g_confirmRestart = m_confirmRestart && m_restartAvailable;
    g_confirmPractice = m_confirmPractice && m_practiceAvailable;
    g_countdown = m_countdown && m_countdownAvailable;
    if (!g_countdown) endCountdown();
}

void PauseTweaks::disable() {
    m_confirmRestart = m_confirmPractice = m_countdown = false;
    apply();
}

void PauseTweaks::update() {
    // Termina sola, o se corta si se salió del nivel
    if (g_countdownEnd && (GetTickCount64() >= g_countdownEnd || !core::game::playLayer())) endCountdown();
}

bool PauseTweaks::drawUI() {
    bool changed = gui::toggle("Confirm restart", m_restartAvailable, m_confirmRestart);
    changed |= gui::toggle("Confirm practice mode", m_practiceAvailable, m_confirmPractice);
    changed |= gui::toggle("Pause countdown", m_countdownAvailable, m_countdown);
    if (changed) apply();
    return changed;
}

void PauseTweaks::drawOverlay() {
    const ULONGLONG now = GetTickCount64();
    if (g_noticeAt && now - g_noticeAt < kConfirmMs && g_notice) drawCentered(g_notice, 0.12f, 1.f);
    if (g_countdownEnd && now < g_countdownEnd && core::game::playLayer()) {
        char text[8];
        snprintf(text, sizeof(text), "%d", static_cast<int>((g_countdownEnd - now + 999) / 1000));
        drawCentered(text, 0.4f, 3.f);
    }
}

void PauseTweaks::load(const nlohmann::json& j) {
    m_confirmRestart = j.value("confirm_restart", false);
    m_confirmPractice = j.value("confirm_practice", false);
    m_countdown = j.value("pause_countdown", false);
    apply();
}

void PauseTweaks::save(nlohmann::json& j) const {
    j["confirm_restart"] = m_confirmRestart;
    j["confirm_practice"] = m_confirmPractice;
    j["pause_countdown"] = m_countdown;
}

} // namespace features
