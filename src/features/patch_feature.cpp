#include "patch_feature.hpp"

#include <windows.h>

#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "gui/widgets.hpp"

namespace features {

PatchFeature::PatchFeature(const char* label, const char* key, const char* signature, int offset,
                           std::vector<uint8_t> bytes)
    : m_label(label), m_key(key), m_signature(signature), m_offset(offset), m_bytes(std::move(bytes)) {}

void PatchFeature::init() {
    uint8_t* site = core::memory::findPattern(GetModuleHandleW(nullptr), m_signature);
    if (!site) {
        core::logMsg("%s: no disponible (firma no encontrada)", m_label);
        return;
    }
    m_patch.set(site + m_offset, m_bytes);
    m_available = true;
    core::logMsg("%s: listo (firma en %p)", m_label, static_cast<void*>(site));
}

void PatchFeature::enable() {
    m_enabled = true;
    if (m_available && !m_patch.apply()) {
        m_enabled = false;
        core::logMsg("%s: no se pudo aplicar el parche", m_label);
    }
}

void PatchFeature::disable() {
    m_enabled = false;
    m_patch.revert();
}

bool PatchFeature::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle(m_label, m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void PatchFeature::load(const nlohmann::json& j) {
    j.value(m_key, false) ? enable() : disable();
}

void PatchFeature::save(nlohmann::json& j) const {
    j[m_key] = m_enabled;
}

} // namespace features
