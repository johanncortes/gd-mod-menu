#include "player_effects.hpp"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <MinHook.h>
#include <nlohmann/json.hpp>

#include "core/game.hpp"
#include "core/log.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

// --- No robot & swing fire ---
void noVisit(void*) {}

// Posición de CCNode::visit() en el vtable (se busca el export dentro del vtable exportado de
// CCNode). Se usa visit y no draw porque CCNode::draw está vacía y comparte dirección con otras
// funciones vacías: buscarla daría una posición equivocada.
int visitSlot(HMODULE cocos, const void* visit) {
    const auto* vtable = reinterpret_cast<void* const*>(GetProcAddress(cocos, "??_7CCNode@cocos2d@@6B@"));
    if (!vtable || !visit) return -1;
    for (int i = 0; i < 200; ++i)
        if (vtable[i] == visit) return i;
    return -1;
}

// A dónde lleva una entrada de vtable del exe: la función, o el destino de su "jmp [import]"
const void* resolveEntry(const void* entry) {
    const auto* code = static_cast<const uint8_t*>(entry);
    if (code[0] != 0xFF || code[1] != 0x25) return entry;
    int32_t disp;
    memcpy(&disp, code + 2, 4);
    const void* target;
    memcpy(&target, code + 6 + disp, sizeof(target));
    return target;
}

constexpr float kMinWaveSize = 0.f, kMaxWaveSize = 5.f;

// --- Trail always on ---
using StreakFn = void (*)(void* streak);
StreakFn g_origStopStroke = nullptr;
bool g_trailAlwaysOn = false;

void stopStrokeHook(void* streak) {
    if (!g_trailAlwaysOn) g_origStopStroke(streak);
}

} // namespace

void NoFire::init() {
    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    uint8_t* create = core::memory::findPattern(GetModuleHandleW(nullptr), sig::kFireBoostCreate);
    const void* nodeVisit =
        cocos ? reinterpret_cast<const void*>(GetProcAddress(cocos, "?visit@CCNode@cocos2d@@UEAAXXZ")) : nullptr;
    const int slot = cocos ? visitSlot(cocos, nodeVisit) : -1;
    if (!create || slot < 0) {
        core::logMsg("No robot & swing fire: no disponible (firma o vtable de CCNode)");
        return;
    }
    int32_t disp;
    memcpy(&disp, create + sig::kFireBoostCreateVtable, 4);
    void** entry = reinterpret_cast<void**>(create + sig::kFireBoostCreateVtable + 4 + disp) + slot;
    // La entrada tiene que ser CCNode::visit (la clase no la redefine): si no, no se toca
    MEMORY_BASIC_INFORMATION info{};
    const bool readable = VirtualQuery(entry, &info, sizeof(info)) && info.State == MEM_COMMIT && *entry &&
                          VirtualQuery(*entry, &info, sizeof(info)) && info.State == MEM_COMMIT;
    if (!readable || resolveEntry(*entry) != nodeVisit) {
        core::logMsg("No robot & swing fire: no disponible (el vtable no es el esperado)");
        return;
    }
    void (*replacement)(void*) = &noVisit;
    std::vector<uint8_t> bytes(sizeof(replacement));
    memcpy(bytes.data(), &replacement, sizeof(replacement));
    m_patch.set(entry, std::move(bytes));
    m_available = true;
    core::logMsg("No robot & swing fire: listo");
}

void NoFire::enable() {
    m_enabled = true;
    if (m_available) m_patch.apply();
}

void NoFire::disable() {
    m_enabled = false;
    if (m_available) m_patch.revert();
}

bool NoFire::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("No robot & swing fire", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void NoFire::load(const nlohmann::json& j) {
    j.value("no_fire", false) ? enable() : disable();
}

void NoFire::save(nlohmann::json& j) const {
    j["no_fire"] = m_enabled;
}

void TrailAlwaysOn::init() {
    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    void* stopStroke =
        cocos ? reinterpret_cast<void*>(GetProcAddress(cocos, "?stopStroke@CCMotionStreak@cocos2d@@QEAAXXZ")) : nullptr;
    m_available = stopStroke &&
                  MH_CreateHook(stopStroke, &stopStrokeHook, reinterpret_cast<void**>(&g_origStopStroke)) == MH_OK &&
                  MH_EnableHook(stopStroke) == MH_OK;
    core::logMsg("Trail always on: %s", m_available ? "listo" : "no disponible");
}

void TrailAlwaysOn::enable() {
    m_enabled = true;
    g_trailAlwaysOn = m_available;
}

void TrailAlwaysOn::disable() {
    m_enabled = false;
    g_trailAlwaysOn = false;
}

bool TrailAlwaysOn::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("Trail always on", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void TrailAlwaysOn::load(const nlohmann::json& j) {
    j.value("trail_always_on", false) ? enable() : disable();
}

void TrailAlwaysOn::save(nlohmann::json& j) const {
    j["trail_always_on"] = m_enabled;
}

void WaveTrailSize::init(bool gameAvailable) {
    uint8_t* site = core::memory::findPattern(GetModuleHandleW(nullptr), sig::kWavePulse);
    if (site) {
        int32_t field;
        memcpy(&field, site + sig::kWavePulseField, 4);
        if (field > 0 && field < 0x10000) m_field = field;
        const std::vector<uint8_t> nops(8, 0x90);
        m_store1.set(site + sig::kWavePulseStore1, nops);
        m_store2.set(site + sig::kWavePulseStore2, nops);
    }
    m_available = gameAvailable && site && m_field;
    core::logMsg("Wave trail size: %s", m_available ? "listo" : "no disponible");
}

void WaveTrailSize::enable() {
    m_enabled = true;
    if (m_available) {
        m_store1.apply();
        m_store2.apply();
    }
}

void WaveTrailSize::disable() {
    m_enabled = false;
    if (m_available) {
        m_store1.revert();
        m_store2.revert();
    }
}

void WaveTrailSize::update() {
    if (!m_enabled || !m_available) return;
    void* layer = core::game::playLayer();
    if (!layer) return;
    for (void* player : {core::game::player1(layer), core::game::player2(layer)})
        if (player) memcpy(static_cast<uint8_t*>(player) + m_field, &m_size, sizeof(m_size));
}

bool WaveTrailSize::drawUI() {
    bool enabled = m_enabled;
    const bool changed =
        gui::toggleValue("Wave trail size", m_available, enabled, m_size, kMinWaveSize, kMaxWaveSize, "x%.2f");
    if (changed) enabled ? enable() : disable();
    return changed;
}

void WaveTrailSize::load(const nlohmann::json& j) {
    m_size = std::clamp(j.value("size", m_size), kMinWaveSize, kMaxWaveSize);
    j.value("enabled", false) ? enable() : disable();
}

void WaveTrailSize::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"size", m_size}};
}

} // namespace features
