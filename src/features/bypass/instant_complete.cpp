#include "instant_complete.hpp"

#include <windows.h>

#include <imgui.h>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"

namespace features {

namespace {

using LevelCompleteFn = void (*)(void* playLayer);
LevelCompleteFn g_levelComplete = nullptr;

} // namespace

void InstantComplete::init(bool gameAvailable) {
    g_levelComplete = reinterpret_cast<LevelCompleteFn>(
        core::memory::findPattern(GetModuleHandleW(nullptr), core::signatures::kLevelComplete));
    m_available = gameAvailable && g_levelComplete;
    core::logMsg("Instant complete: %s", m_available ? "listo" : "no disponible (firma no encontrada)");
}

void InstantComplete::drawUI() {
    // Solo se puede pulsar dentro de un nivel
    void* playLayer = m_available ? core::game::playLayer() : nullptr;
    ImGui::BeginDisabled(!playLayer);
    if (ImGui::Button("Instant complete")) g_levelComplete(playLayer);
    ImGui::EndDisabled();
    if (!m_available) {
        ImGui::SameLine();
        ImGui::TextDisabled("(no disponible)");
    }
}

} // namespace features
