#include "config.hpp"

#include <filesystem>
#include <fstream>

#include "core/log.hpp"

namespace config {

namespace {
std::filesystem::path g_path;
}

void init(HMODULE self) {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(self, path, MAX_PATH);
    g_path = std::filesystem::path(path).parent_path() / L"config.json";
}

std::string siblingPath(const char* fileName) {
    return (g_path.parent_path() / fileName).string();
}

nlohmann::json load() {
    std::ifstream file(g_path);
    if (!file) return nlohmann::json::object();
    // Sin excepciones: un archivo dañado devuelve un valor "discarded"
    nlohmann::json data = nlohmann::json::parse(file, nullptr, false);
    if (!data.is_object()) {
        core::logMsg("config.json dañado: se usan los valores por defecto");
        return nlohmann::json::object();
    }
    return data;
}

bool save(const nlohmann::json& data) {
    // Se escribe a un temporal y se renombra, para no dejar el archivo a medias
    std::filesystem::path tmp = g_path;
    tmp += L".tmp";
    {
        std::ofstream file(tmp, std::ios::trunc);
        if (!file) return false;
        file << data.dump(2);
        if (!file) return false;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, g_path, ec);
    if (ec) core::logMsg("No se pudo guardar config.json");
    return !ec;
}

} // namespace config
