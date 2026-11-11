#pragma once
#include <windows.h>

#include <string>

#include <nlohmann/json.hpp>

// config.json junto a la DLL.
namespace config {

// Calcula la ruta del archivo. Llamar una vez al cargar la DLL.
void init(HMODULE self);

// Ruta de otro archivo en la misma carpeta que config.json (junto a la DLL).
std::string siblingPath(const char* fileName);

// Devuelve un objeto vacío si el archivo no existe o está dañado.
nlohmann::json load();

bool save(const nlohmann::json& data);

} // namespace config
