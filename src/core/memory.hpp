#pragma once
#include <windows.h>

#include <cstdint>
#include <vector>

// Utilidades de memoria: búsqueda de firmas y parches de bytes reversibles.
namespace core::memory {

// Busca una firma ("0F 5A ?? C0", ?? = comodín) en las secciones de código del módulo.
// Devuelve nullptr si no aparece o si aparece más de una vez (una firma ambigua no sirve).
uint8_t* findPattern(HMODULE module, const char* pattern);

// Reserva memoria de lectura/escritura a menos de 2 GB de `target`, para poder apuntarle
// con direccionamiento relativo a RIP. Devuelve nullptr si no hay hueco.
void* allocNear(const void* target, size_t size);

// Parche de bytes que guarda los originales para poder deshacerlo.
class Patch {
public:
    // Define el parche (todavía no lo aplica).
    void set(void* address, std::vector<uint8_t> bytes);
    bool apply();
    bool revert();
    bool applied() const { return m_applied; }

private:
    uint8_t* m_address = nullptr;
    std::vector<uint8_t> m_bytes;
    std::vector<uint8_t> m_original;
    bool m_applied = false;
};

} // namespace core::memory
