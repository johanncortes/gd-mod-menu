#include "safe_mode.hpp"

#include <windows.h>

#include <cstring>

#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

int32_t g_toolsUsedOffset = 0;   // PlayLayer: "se usaron herramientas" (descarta el intento)
int32_t g_speedChangedOffset = 0; // PlayLayer: "se cambió la velocidad" (descarta el intento)
bool g_active = false;            // hay que marcar el intento

int32_t readOffset(const uint8_t* at) {
    int32_t value = 0;
    std::memcpy(&value, at, sizeof(value));
    return value;
}

// Tras cada frame del nivel (GJBaseGameLayer::update). En el editor no hay PlayLayer.
void onUpdate(void* gameLayer, float) {
    if (!g_active || gameLayer != core::game::playLayer()) return;
    auto* layer = static_cast<uint8_t*>(gameLayer);
    layer[g_toolsUsedOffset] = 1;
    layer[g_speedChangedOffset] = 1;
}

} // namespace

void SafeMode::init(bool gameAvailable) {
    uint8_t* flags = core::memory::findPattern(GetModuleHandleW(nullptr), core::signatures::kAttemptDiscardFlags);
    if (!gameAvailable || !core::game::levelHooksAvailable() || !flags) {
        core::logMsg("Safe mode: no disponible (%s)", flags ? "hooks de nivel" : "firma kAttemptDiscardFlags");
        return;
    }
    g_toolsUsedOffset = readOffset(flags + core::signatures::kAttemptDiscardToolsUsed);
    g_speedChangedOffset = readOffset(flags + core::signatures::kAttemptDiscardSpeedChanged);
    const auto plausible = [](int32_t offset) { return offset > 0 && offset < 0x10000; };
    if (!plausible(g_toolsUsedOffset) || !plausible(g_speedChangedOffset)) {
        core::logMsg("Safe mode: no disponible (offsets inesperados)");
        return;
    }
    core::game::onUpdate(&onUpdate);
    m_available = true;
    core::logMsg("Safe mode: listo (banderas en +0x%X y +0x%X)", g_toolsUsedOffset, g_speedChangedOffset);
}

void SafeMode::disable() {
    g_active = false;
}

void SafeMode::update(bool cheatsActive) {
    g_active = m_available && (m_safe || (m_auto && cheatsActive));
}

bool SafeMode::active() const {
    return g_active;
}

bool SafeMode::drawUI() {
    bool changed = gui::toggle("Safe mode", m_available, m_safe);
    changed |= gui::toggle("Auto safe mode", m_available, m_auto);
    return changed;
}

void SafeMode::load(const nlohmann::json& j) {
    m_safe = j.value("safe_mode", false);
    m_auto = j.value("auto_safe_mode", true);
}

void SafeMode::save(nlohmann::json& j) const {
    j["safe_mode"] = m_safe;
    j["auto_safe_mode"] = m_auto;
}

} // namespace features
