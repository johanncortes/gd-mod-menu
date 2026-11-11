#include "memory.hpp"

#include <cstdlib>
#include <cstring>

namespace core::memory {

namespace {

struct PatternByte {
    uint8_t value;
    bool wildcard;
};

std::vector<PatternByte> parsePattern(const char* text) {
    std::vector<PatternByte> out;
    for (const char* p = text; *p;) {
        if (*p == ' ') {
            ++p;
        } else if (*p == '?') {
            out.push_back({0, true});
            while (*p == '?') ++p;
        } else {
            char* end = nullptr;
            out.push_back({static_cast<uint8_t>(strtoul(p, &end, 16)), false});
            if (end == p) return {}; // carácter inválido en la firma
            p = end;
        }
    }
    return out;
}

bool writeBytes(uint8_t* address, const std::vector<uint8_t>& bytes) {
    DWORD oldProtect = 0;
    if (!VirtualProtect(address, bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;
    memcpy(address, bytes.data(), bytes.size());
    VirtualProtect(address, bytes.size(), oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), address, bytes.size());
    return true;
}

} // namespace

uint8_t* findPattern(HMODULE module, const char* pattern) {
    const std::vector<PatternByte> pat = parsePattern(pattern);
    if (!module || pat.empty() || pat[0].wildcard) return nullptr;

    auto* base = reinterpret_cast<uint8_t*>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);

    uint8_t* found = nullptr;
    for (WORD s = 0; s < nt->FileHeader.NumberOfSections; ++s, ++section) {
        if (!(section->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        uint8_t* begin = base + section->VirtualAddress;
        const size_t size = section->Misc.VirtualSize;
        if (size < pat.size()) continue;
        uint8_t* last = begin + size - pat.size();

        for (uint8_t* p = begin; p <= last; ++p) {
            // memchr salta rápido hasta el siguiente candidato por el primer byte
            p = static_cast<uint8_t*>(memchr(p, pat[0].value, static_cast<size_t>(last - p) + 1));
            if (!p) break;
            size_t i = 1;
            while (i < pat.size() && (pat[i].wildcard || p[i] == pat[i].value)) ++i;
            if (i < pat.size()) continue;
            if (found) return nullptr; // segunda coincidencia: firma ambigua
            found = p;
        }
    }
    return found;
}

void* allocNear(const void* target, size_t size) {
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    const uintptr_t step = si.dwAllocationGranularity;
    const uintptr_t center = reinterpret_cast<uintptr_t>(target) & ~(step - 1);
    const uintptr_t maxDistance = 0x7FF00000; // margen bajo los 2 GB del desplazamiento de 32 bits

    for (uintptr_t distance = step; distance < maxDistance; distance += step) {
        if (distance < center) {
            if (void* mem = VirtualAlloc(reinterpret_cast<void*>(center - distance), size,
                                         MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))
                return mem;
        }
        if (void* mem = VirtualAlloc(reinterpret_cast<void*>(center + distance), size,
                                     MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))
            return mem;
    }
    return nullptr;
}

void Patch::set(void* address, std::vector<uint8_t> bytes) {
    m_address = static_cast<uint8_t*>(address);
    m_bytes = std::move(bytes);
    m_original.assign(m_address, m_address + m_bytes.size());
    m_applied = false;
}

bool Patch::apply() {
    if (m_applied) return true;
    if (!m_address || !writeBytes(m_address, m_bytes)) return false;
    m_applied = true;
    return true;
}

bool Patch::revert() {
    if (!m_applied) return true;
    if (!writeBytes(m_address, m_original)) return false;
    m_applied = false;
    return true;
}

} // namespace core::memory
