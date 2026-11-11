#pragma once
#include <windows.h>

#include <nlohmann/json.hpp>

// config.json junto a la DLL.
namespace config {

// Calcula la ruta del archivo. Llamar una vez al cargar la DLL.
void init(HMODULE self);

// Devuelve un objeto vacío si el archivo no existe o está dañado.
nlohmann::json load();

bool save(const nlohmann::json& data);

} // namespace config
