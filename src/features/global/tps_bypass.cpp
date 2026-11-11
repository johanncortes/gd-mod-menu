#include "tps_bypass.hpp"

#include <windows.h>

#include <algorithm>
#include <cstring>

#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "core/signatures.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

constexpr float kMinTps = 30.f;
constexpr float kMaxTps = 4000.f;

// Dirección a la que apunta un operando relativo a RIP (desplazamiento de 32 bits en `disp`;
// la instrucción termina justo después de él).
uint8_t* ripTarget(uint8_t* disp) {
    int32_t offset;
    memcpy(&offset, disp, sizeof(offset));
    return disp + 4 + offset;
}

// Bytes del desplazamiento que hace que el operando en `disp` apunte a `target`
std::vector<uint8_t> ripDisp(const uint8_t* disp, const void* target) {
    const auto offset = static_cast<int32_t>(static_cast<const uint8_t*>(target) - (disp + 4));
    std::vector<uint8_t> bytes(sizeof(offset));
    memcpy(bytes.data(), &offset, sizeof(offset));
    return bytes;
}

} // namespace

void TpsBypass::init() {
    HMODULE exe = GetModuleHandleW(nullptr);
    uint8_t* tickSite = core::memory::findPattern(exe, core::signatures::kTickLength);
    uint8_t* stepsSite = core::memory::findPattern(exe, core::signatures::kStepsPerFrame);
    if (!tickSite || !stepsSite) {
        core::logMsg("Physics bypass: no disponible (firma no encontrada: %s%s)",
                     tickSite ? "" : "kTickLength ", stepsSite ? "" : "kStepsPerFrame");
        return;
    }
    uint8_t* tickDisp = tickSite + core::signatures::kTickLengthDisp;
    uint8_t* stepsDisp = stepsSite + core::signatures::kStepsPerFrameDisp;

    // Comprobación extra: las instrucciones tienen que leer las constantes que esperamos
    double tickLength, stepsPerFrame;
    memcpy(&tickLength, ripTarget(tickDisp), sizeof(double));
    memcpy(&stepsPerFrame, ripTarget(stepsDisp), sizeof(double));
    if (tickLength != 1.0 / 240.0 || stepsPerFrame != 4.0) {
        core::logMsg("Physics bypass: no disponible (constantes inesperadas: %g y %g)", tickLength, stepsPerFrame);
        return;
    }

    m_constants = static_cast<double*>(core::memory::allocNear(tickSite, 2 * sizeof(double)));
    if (!m_constants) {
        core::logMsg("Physics bypass: no disponible (sin memoria cerca del exe)");
        return;
    }
    writeConstants();
    m_tickLengthPatch.set(tickDisp, ripDisp(tickDisp, &m_constants[0]));
    m_stepsPatch.set(stepsDisp, ripDisp(stepsDisp, &m_constants[1]));
    m_available = true;
    core::logMsg("Physics bypass: listo (firmas en %p y %p)", static_cast<void*>(tickSite), static_cast<void*>(stepsSite));
}

void TpsBypass::writeConstants() {
    m_constants[0] = 1.0 / m_tps;
    m_constants[1] = m_tps / 60.0;
}

void TpsBypass::enable() {
    m_enabled = true;
    if (!m_available) return;
    writeConstants();
    if (!m_tickLengthPatch.apply() || !m_stepsPatch.apply()) {
        // Nunca dejar un parche sí y otro no
        m_tickLengthPatch.revert();
        m_stepsPatch.revert();
        m_enabled = false;
        core::logMsg("Physics bypass: no se pudo aplicar el parche");
    }
}

void TpsBypass::disable() {
    m_enabled = false;
    m_tickLengthPatch.revert();
    m_stepsPatch.revert();
}

void TpsBypass::shutdown() {
    disable();
    m_available = false;
    if (m_constants) VirtualFree(m_constants, 0, MEM_RELEASE);
    m_constants = nullptr;
}

bool TpsBypass::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggleValue("Physics bypass (TPS)", m_available, enabled, m_tps, kMinTps, kMaxTps, "%.0f");
    if (changed) enabled ? enable() : disable();
    return changed;
}

void TpsBypass::load(const nlohmann::json& j) {
    m_tps = std::clamp(j.value("tps", m_tps), kMinTps, kMaxTps);
    j.value("enabled", false) ? enable() : disable();
}

void TpsBypass::save(nlohmann::json& j) const {
    j = {{"enabled", m_enabled}, {"tps", m_tps}};
}

} // namespace features
