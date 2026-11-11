#include "features.hpp"

#include <windows.h>

#include <nlohmann/json.hpp>

#include "config/config.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "features/global/fps_bypass.hpp"
#include "features/global/speedhack.hpp"
#include "features/global/tps_bypass.hpp"
#include "features/patch_feature.hpp"
#include "features/player/hitboxes.hpp"
#include "features/player/noclip.hpp"
#include "features/player/respawn_delay.hpp"

namespace features {

namespace {

Speedhack g_speedhack;
FpsBypass g_fpsBypass;
TpsBypass g_tpsBypass;

Noclip g_noclip;
Hitboxes g_hitboxes;
RespawnDelay g_respawnDelay;
// je -> jmp: playerDestroyed siempre toma el camino "ocultar al jugador sin efecto"
PatchFeature g_noDeathEffect("Sin efecto de muerte", "no_death_effect", core::signatures::kDeathEffect,
                             core::signatures::kDeathEffectJump, {0xEB});
// ret al inicio de la función del parpadeo
PatchFeature g_noRespawnFlash("Sin parpadeo al reaparecer", "no_respawn_flash", core::signatures::kSpawnEffect,
                              0, {0xC3});

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

        const nlohmann::json player = section(data, "player");
        g_noclip.load(section(player, "noclip"));
        g_hitboxes.load(section(player, "hitboxes"));
        g_respawnDelay.load(section(player, "respawn_delay"));
        g_noDeathEffect.load(player);
        g_noRespawnFlash.load(player);
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

    nlohmann::json& player = data["player"];
    g_noclip.save(player["noclip"]);
    g_hitboxes.save(player["hitboxes"]);
    g_respawnDelay.save(player["respawn_delay"]);
    g_noDeathEffect.save(player);
    g_noRespawnFlash.save(player);
    config::save(data);
    g_dirty = false;
}

} // namespace

void init() {
    if (g_ready) return;
    g_speedhack.init();
    g_fpsBypass.init();
    g_tpsBypass.init();

    // Las firmas se buscan antes de instalar ningún hook: un hook reescribe el inicio de la
    // función y la firma dejaría de coincidir. destroyPlayer lo usan dos features.
    uint8_t* destroyPlayer = core::memory::findPattern(GetModuleHandleW(nullptr), core::signatures::kDestroyPlayer);
    g_noDeathEffect.init();
    g_noRespawnFlash.init();
    g_respawnDelay.init(destroyPlayer);
    g_hitboxes.init();
    g_noclip.init(destroyPlayer);

    loadConfig();
    g_ready = true;
}

void shutdown() {
    if (!g_ready) return;
    if (g_dirty) saveConfig(); // antes de desactivar: se guarda el estado que eligió el usuario
    g_speedhack.disable();
    g_fpsBypass.disable();
    g_tpsBypass.shutdown();
    g_noclip.disable();
    g_hitboxes.disable();
    g_respawnDelay.disable();
    g_noDeathEffect.disable();
    g_noRespawnFlash.disable();
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

void drawPlayer() {
    bool changed = g_noclip.drawUI();
    changed |= g_hitboxes.drawUI();
    changed |= g_noDeathEffect.drawUI();
    changed |= g_noRespawnFlash.drawUI();
    changed |= g_respawnDelay.drawUI();
    if (changed) markDirty();
}

void drawOverlay() {
    if (g_ready) g_noclip.drawOverlay();
}

bool cheatsActive() {
    return g_speedhack.active() || g_tpsBypass.active() || g_noclip.active();
}

} // namespace features
