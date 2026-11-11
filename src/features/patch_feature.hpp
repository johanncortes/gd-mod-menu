#pragma once
#include <cstdint>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "core/memory.hpp"

namespace features {

// Feature que consiste en un único parche de bytes en GeometryDash.exe:
// una casilla que lo aplica o lo deshace.
class PatchFeature {
public:
    // label: texto de la casilla. key: clave en config.json.
    // signature + offset: dónde se escribe. bytes: lo que se escribe.
    PatchFeature(const char* label, const char* key, const char* signature, int offset,
                 std::vector<uint8_t> bytes);

    void init(); // busca la firma; si no aparece queda "no disponible"
    void enable();
    void disable();
    bool drawUI(); // true si el usuario cambió algo

    void load(const nlohmann::json& j);
    void save(nlohmann::json& j) const;

    bool active() const { return m_enabled && m_available; }

private:
    const char* m_label;
    const char* m_key;
    const char* m_signature;
    int m_offset;
    std::vector<uint8_t> m_bytes;

    bool m_available = false;
    bool m_enabled = false;
    core::memory::Patch m_patch;
};

} // namespace features
