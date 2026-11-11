#include "features.hpp"

#include <windows.h>

#include <nlohmann/json.hpp>

#include "config/config.hpp"
#include "core/log.hpp"
#include "features/global/fps_bypass.hpp"
#include "features/global/speedhack.hpp"
#include "features/global/tps_bypass.hpp"

namespace features {

namespace {

Speedhack g_speedhack;
FpsBypass g_fpsBypass;
TpsBypass g_tpsBypass;

bool g_ready = false;
bool g_dirty = false;      // hay cambios sin guardar
ULONGLONG g_dirtySince = 0;

void markDirty() {
    g_dirty = true;
    g_dirtySince = GetTickCount64();
}

// Sub-objeto de la config; vacío si falta o no es un objeto
nlohmann::json section(const nlohmann::json& parent, const char* key) {
    const auto it = parent.find(key);
    return it != parent.end() && it->is_object() ? *it : nlohmann::json::object();
}

void loadConfig() {
    const nlohmann::json data = config::load();
    try {
        const nlohmann::json global = section(data, "global");
        g_speedhack.load(section(global, "speedhack"));
        g_fpsBypass.load(section(global, "fps_bypass"));
        g_tpsBypass.load(section(global, "tps_bypass"));
    } catch (const nlohmann::json::exception& e) {
        // Un valor con el tipo equivocado: se queda lo cargado hasta ese punto
        core::logMsg("config.json con valores inválidos: %s", e.what());
    }
}

void saveConfig() {
    nlohmann::json data = config::load(); // conserva claves que este código no conozca
    nlohmann::json& global = data["global"];
    g_speedhack.save(global["speedhack"]);
    g_fpsBypass.save(global["fps_bypass"]);
    g_tpsBypass.save(global["tps_bypass"]);
    config::save(data);
    g_dirty = false;
}

} // namespace

void init() {
    if (g_ready) return;
    g_speedhack.init();
    g_fpsBypass.init();
    g_tpsBypass.init();
    loadConfig();
    g_ready = true;
}

void shutdown() {
    if (!g_ready) return;
    if (g_dirty) saveConfig(); // antes de desactivar: se guarda el estado que eligió el usuario
    g_speedhack.disable();
    g_fpsBypass.disable();
    g_tpsBypass.shutdown();
    g_ready = false;
}

void update() {
    // Se espera un momento tras el último cambio para no escribir el archivo en cada tecla
    if (g_ready && g_dirty && GetTickCount64() - g_dirtySince > 500) saveConfig();
}

void drawGlobal() {
    bool changed = g_speedhack.drawUI();
    changed |= g_fpsBypass.drawUI();
    changed |= g_tpsBypass.drawUI();
    if (changed) markDirty();
}

bool cheatsActive() {
    return g_speedhack.active() || g_tpsBypass.active();
}

} // namespace features
