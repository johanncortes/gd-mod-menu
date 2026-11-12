#include "jump_hack.hpp"

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

namespace sig = core::signatures;

// Desplazamientos en PlayerObject, leídos de kPushButtonModes
int32_t g_modeOffsets[5] = {}; // bola, nave, UFO, onda, swing (en esos modos no se toca nada)
int32_t g_holdingOffset = 0;   // mantiene el salto
int32_t g_onGroundOffset = 0;  // en el suelo
bool g_enabled = false;

// Tras cada frame del nivel
void onUpdate(void* gameLayer, float) {
    if (!g_enabled || gameLayer != core::game::playLayer()) return;
    auto* player = static_cast<uint8_t*>(core::game::player1(gameLayer));
    if (!player || !player[g_holdingOffset]) return;
    for (int32_t mode : g_modeOffsets)
        if (player[mode]) return;
    player[g_onGroundOffset] = 1;
}

} // namespace

void JumpHack::init(bool gameAvailable) {
    const uint8_t* code = core::memory::findPattern(GetModuleHandleW(nullptr), sig::kPushButtonModes);
    if (!gameAvailable || !core::game::levelHooksAvailable() || !code) {
        core::logMsg("Jump hack: no disponible (%s)", code ? "hooks de nivel" : "firma kPushButtonModes");
        return;
    }
    auto read = [code](int at) {
        int32_t value = 0;
        memcpy(&value, code + at, sizeof(value));
        return value;
    };
    bool ok = true;
    for (int i = 0; i < 5; ++i) {
        g_modeOffsets[i] = read(sig::kPushButtonModeFlags[i]);
        ok = ok && g_modeOffsets[i] > 0 && g_modeOffsets[i] < 0x10000;
    }
    g_holdingOffset = read(sig::kPushButtonHolding);
    g_onGroundOffset = read(sig::kPushButtonOnGround);
    ok = ok && g_holdingOffset > 0 && g_holdingOffset < 0x10000 && g_onGroundOffset > 0 && g_onGroundOffset < 0x10000;
    if (!ok) {
        core::logMsg("Jump hack: no disponible (offsets inesperados)");
        return;
    }
    core::game::onUpdate(&onUpdate);
    m_available = true;
    core::logMsg("Jump hack: listo (en el suelo +0x%X, manteniendo +0x%X)", g_onGroundOffset, g_holdingOffset);
}

void JumpHack::enable() {
    m_enabled = true;
    g_enabled = m_available;
}

void JumpHack::disable() {
    m_enabled = false;
    g_enabled = false;
}

bool JumpHack::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Jump hack", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void JumpHack::load(const nlohmann::json& j) {
    j.value("enabled", false) ? enable() : disable();
}

void JumpHack::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}};
}

} // namespace features
