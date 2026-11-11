#pragma once
#include <cstdint>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "core/memory.hpp"

namespace features {

// Un sitio a parchear: firma + desplazamiento dentro de ella + bytes que se escriben.
struct PatchSite {
    const char* signature;
    int offset;
    std::vector<uint8_t> bytes;
};

// Feature que consiste en uno o varios parches de bytes en GeometryDash.exe:
// una casilla que los aplica o los deshace todos a la vez.
class PatchFeature {
public:
    // label: texto de la casilla. key: clave en config.json. cheat: cuenta para el Cheat Indicator.
    PatchFeature(const char* label, const char* key, std::vector<PatchSite> sites, bool cheat = false);
    PatchFeature(const char* label, const char* key, const char* signature, int offset,
                 std::vector<uint8_t> bytes);

    void init(); // busca la firma; si no aparece queda "no disponible"
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const { return m_enabled && m_available; }
    bool cheatActive() const { return m_cheat && active(); }

private:
    const char* m_label;
    const char* m_key;
    std::vector<PatchSite> m_sites;
    bool m_cheat;

    bool m_available = false;
    bool m_enabled = false;
    std::vector<core::memory::Patch> m_patches;
};

} // namespace features
