#include "patch_feature.hpp"

#include <windows.h>

#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "gui/widgets.hpp"

namespace features {

PatchFeature::PatchFeature(const char* label, const char* key, std::vector<PatchSite> sites, bool cheat)
    : m_label(label), m_key(key), m_sites(std::move(sites)), m_cheat(cheat) {}

PatchFeature::PatchFeature(const char* label, const char* key, const char* signature, int offset,
                           std::vector<uint8_t> bytes)
    : PatchFeature(label, key, {{signature, offset, std::move(bytes)}}) {}

void PatchFeature::init() {
    // Solo queda disponible si aparecen todas las firmas
    HMODULE exe = GetModuleHandleW(nullptr);
    for (const PatchSite& site : m_sites) {
        uint8_t* match = core::memory::findPattern(exe, site.signature);
        if (!match) {
            core::logMsg("%s: no disponible (firma %zu de %zu no encontrada)", m_label, m_patches.size() + 1,
                         m_sites.size());
            m_patches.clear();
            return;
        }
        m_patches.emplace_back().set(match + site.offset, site.bytes);
    }
    m_available = true;
    core::logMsg("%s: listo", m_label);
}

void PatchFeature::enable() {
    m_enabled = true;
    if (!m_available) return;
    for (core::memory::Patch& patch : m_patches) {
        if (patch.apply()) continue;
        core::logMsg("%s: no se pudo aplicar el parche", m_label);
        disable();
        return;
    }
}

void PatchFeature::disable() {
    m_enabled = false;
    for (core::memory::Patch& patch : m_patches) patch.revert();
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
