#include "no_particles.hpp"

#include <windows.h>

#include <MinHook.h>
#include <nlohmann/json.hpp>

#include "core/log.hpp"
#include "gui/widgets.hpp"

namespace features {

namespace {

using AddParticleFn = bool (*)(void* system);

AddParticleFn g_origAddParticle = nullptr;
bool g_enabled = false;

// CCParticleSystem::addParticle: false = "no cabe ninguna más", el sistema no emite
bool addParticleHook(void* system) {
    if (g_enabled) return false;
    return g_origAddParticle(system);
}

} // namespace

void NoParticles::init() {
    HMODULE cocos = GetModuleHandleW(L"libcocos2d.dll");
    void* addParticle =
        cocos ? reinterpret_cast<void*>(GetProcAddress(cocos, "?addParticle@CCParticleSystem@cocos2d@@QEAA_NXZ"))
              : nullptr;
    if (!addParticle) {
        core::logMsg("No particles: no disponible (falta CCParticleSystem::addParticle)");
        return;
    }
    m_available = MH_CreateHook(addParticle, &addParticleHook, reinterpret_cast<void**>(&g_origAddParticle)) == MH_OK &&
                  MH_EnableHook(addParticle) == MH_OK;
    core::logMsg("No particles: %s", m_available ? "listo" : "no disponible (hook)");
}

void NoParticles::enable() {
    m_enabled = true;
    g_enabled = m_available;
}

void NoParticles::disable() {
    m_enabled = false;
    g_enabled = false;
}

bool NoParticles::drawUI() {
    bool enabled = m_enabled;
    const bool changed = gui::toggle("No particles", m_available, enabled);
    if (changed) enabled ? enable() : disable();
    return changed;
}

void NoParticles::load(const nlohmann::json& j) {
    j.value("no_particles", false) ? enable() : disable();
}

void NoParticles::save(nlohmann::json& j) const {
    j["no_particles"] = m_enabled;
}

} // namespace features
