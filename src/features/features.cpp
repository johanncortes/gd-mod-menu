#include "features.hpp"

#include <windows.h>

#include <nlohmann/json.hpp>

#include "config/config.hpp"
#include "core/game.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/signatures.hpp"
#include "features/bot/bot.hpp"
#include "features/bypass/instant_complete.hpp"
#include "features/cosmetic/no_mirror.hpp"
#include "features/cosmetic/no_particles.hpp"
#include "features/global/fps_bypass.hpp"
#include "features/global/speedhack.hpp"
#include "features/global/tps_bypass.hpp"
#include "features/labels/labels.hpp"
#include "features/level/auto_practice.hpp"
#include "features/level/hide_pause.hpp"
#include "features/level/startpos_switcher.hpp"
#include "features/patch_feature.hpp"
#include "features/player/hitboxes.hpp"
#include "features/player/noclip.hpp"
#include "features/player/respawn_delay.hpp"
#include "features/player/trajectory.hpp"
#include "features/shortcuts/shortcuts.hpp"
#include "gui/game_font.hpp"
#include "gui/keybinds.hpp"
#include "gui/theme.hpp"

namespace features {

namespace {

namespace sig = core::signatures;

Speedhack g_speedhack;
FpsBypass g_fpsBypass;
TpsBypass g_tpsBypass;

Noclip g_noclip;
Hitboxes g_hitboxes;
RespawnDelay g_respawnDelay;
Trajectory g_trajectory;
// je -> jmp: playerDestroyed siempre toma el camino "ocultar al jugador sin efecto"
PatchFeature g_noDeathEffect("Sin efecto de muerte", "no_death_effect", core::signatures::kDeathEffect,
                             core::signatures::kDeathEffectJump, {0xEB});
// ret al inicio de la función del parpadeo
PatchFeature g_noRespawnFlash("Sin parpadeo al reaparecer", "no_respawn_flash", core::signatures::kSpawnEffect,
                              0, {0xC3});

// --- Level ---
AutoPractice g_autoPractice;
HidePause g_hidePause;
StartPosSwitcher g_startPosSwitcher;
// m_practiceMusicSync = 1 siempre: en práctica suena la música normal del nivel
PatchFeature g_practiceMusicSync("Practice music sync", "practice_music_sync", sig::kPracticeMusicSync,
                                 sig::kPracticeMusicSyncValue, {0x01});

// --- Bypass ---
// "mov al,1; ret" al inicio de las dos comprobaciones
PatchFeature g_unlockIcons("Unlock icons", "unlock_icons",
                           {{sig::kIsIconUnlocked, 0, {0xB0, 0x01, 0xC3}}, {sig::kIsColorUnlocked, 0, {0xB0, 0x01, 0xC3}}});
// Los campos de texto dejan de recortar al llegar al máximo de letras
PatchFeature g_textLimit("Character limit bypass", "character_limit",
                         {{sig::kTextLimitPaste, sig::kTextLimitPasteJump, {0x90, 0xE9}},
                          {sig::kTextLimitType, sig::kTextLimitTypeJump, {0xEB}}});
// Los campos de texto aceptan cualquier carácter
PatchFeature g_textFilter("Character filter bypass", "character_filter", sig::kTextFilter, sig::kTextFilterJump, {0xEB});
InstantComplete g_instantComplete;
// Comprobaciones de desbloqueo en el botón de cada sitio (fase 11). No cambian la partida guardada.
PatchFeature g_unlockMainLevels("Unlock main levels", "unlock_main_levels", sig::kMainLevelLock,
                                sig::kMainLevelLockJump, {0x90, 0xE9});
PatchFeature g_unlockVaults("Unlock vaults", "unlock_vaults",
                            {{sig::kVaultLock, sig::kVaultLockJump, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90}},
                             {sig::kVaultOfSecretsLock, sig::kVaultOfSecretsLockJump, {0x90, 0x90}}});
PatchFeature g_unlockBasement("Unlock basement (Keymaster)", "unlock_basement", sig::kBasementLock,
                              sig::kBasementLockJump, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90});
PatchFeature g_unlockChallenge("Unlock challenge level", "unlock_challenge", sig::kChallengeLevelLock,
                               sig::kChallengeLevelLockJump, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90});
PatchFeature g_unlockTreasureRoom("Unlock treasure room", "unlock_treasure_room", sig::kTreasureRoomLock,
                                  sig::kTreasureRoomLockJump, {0x90, 0x90});
PatchFeature g_unlockShops("Unlock shops", "unlock_shops",
                           {{sig::kShopLocks, sig::kShopLockJumps[0], {0x90, 0xE9}},
                            {sig::kShopLocks, sig::kShopLockJumps[1], {0x90, 0xE9}},
                            {sig::kShopLocks, sig::kShopLockJumps[2], {0xEB}},
                            {sig::kShopLocks, sig::kShopLockJumps[3], {0xEB}}});
// Los deslizadores (editor, ajustes) pueden pasar de su mínimo y su máximo
PatchFeature g_sliderLimit("Slider limit bypass", "slider_limit",
                           {{sig::kSliderClamp, sig::kSliderClampLow, {0xEB}},
                            {sig::kSliderClamp, sig::kSliderClampHigh, {0xEB}}});

// --- Creator ---
// El nivel se puede subir aunque no esté verificado
PatchFeature g_verify("Verify bypass", "verify", sig::kVerify, sig::kVerifyJump, {0x90, 0xE9});
// ret al inicio de la función que devuelve la cámara del editor a los límites del nivel
PatchFeature g_freeScroll("Free scroll", "free_scroll", sig::kConstrainGameLayer, 0, {0xC3});
// El botón de copiar sale siempre y nunca pide contraseña
PatchFeature g_levelCopy("Level copy bypass", "level_copy",
                         {{sig::kCloneButton, sig::kCloneButtonSetne, {0xB2, 0x01, 0x90}},
                          {sig::kTryCloneLevel, sig::kTryCloneLevelJump, {0x90, 0x90}}});
// jbe/jb -> jmp en los dos límites de los objetos personalizados
PatchFeature g_customObjects("Custom object bypass", "custom_objects",
                             {{sig::kCustomObjectSize, sig::kCustomObjectSizeJump, {0x90, 0xE9}},
                              {sig::kCustomObjectCount, sig::kCustomObjectCountJump, {0x90, 0xE9}}});
// Se puede poner un objeto encima de otro igual en la misma posición
PatchFeature g_placeOver("Place over", "place_over",
                         {{sig::kPlaceOver, sig::kPlaceOverExists, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90}},
                          {sig::kPlaceOver, sig::kPlaceOverCustom, {0x90, 0xE9}}});
// Botón de editar en el menú de pausa de cualquier nivel (el juego pide confirmación antes de abrirlo)
PatchFeature g_levelEdit("Level edit", "level_edit",
                         {{sig::kPauseEditButton, sig::kPauseEditButtonJump, {0xEB}},
                          {sig::kPauseOnEdit, sig::kPauseOnEditJump, {0xEB}}});
// Las flechas de temas por defecto llegan a los ocultos (hasta el 40)
PatchFeature g_defaultSongs("Default song bypass", "default_song_bypass",
                            {{sig::kSongSelectArrows, sig::kSongSelectArrowsLimits[0], {0x28}},
                             {sig::kSongSelectArrows, sig::kSongSelectArrowsLimits[1], {0x28}},
                             {sig::kSongSelectArrows, sig::kSongSelectArrowsLimits[2], {0x28}}});
// Opciones del editor: hasta 24 botones por fila y 6 filas en la caja de herramientas
PatchFeature g_toolboxButtons("Toolbox button bypass", "toolbox_buttons",
                              {{sig::kToolboxPerRow, sig::kToolboxMaxCompare, {0x18}},
                               {sig::kToolboxPerRow, sig::kToolboxMaxValue, {0x18}},
                               {sig::kToolboxRows, sig::kToolboxMaxCompare, {0x06}},
                               {sig::kToolboxRows, sig::kToolboxMaxValue, {0x06}}});

// --- Cosmetic ---
// (Sin efecto de muerte y Sin parpadeo al reaparecer, arriba, también salen en esta ventana)
NoMirror g_noMirror;
NoParticles g_noParticles;
// ret al inicio: la cámara no tiembla (portales, final del nivel, triggers de shake)
PatchFeature g_noShake("No shake", "no_shake", sig::kShakeCamera, 0, {0xC3});
// ret al inicio: sin los rayos de los portales y del final del nivel
PatchFeature g_noLightning("No portal lightning", "no_portal_lightning", sig::kLightningFlash, 0, {0xC3});
// ret al inicio de CCCircleWave::draw: sin los círculos de portales, anillos y monedas
PatchFeature g_noCircles("No circle effect", "no_circle_effect", sig::kCircleWaveDraw, 0, {0xC3});
// la estela normal del icono nunca se reanuda (la llamada a resumeStroke pasa a NOPs)
PatchFeature g_noTrail("No trail", "no_trail", sig::kActivateStreakResume, sig::kActivateStreakResumeCall,
                       {0x90, 0x90, 0x90, 0x90, 0x90, 0x90});
// ret al inicio de HardStreak::addPoint: la estela de la onda no recibe puntos
PatchFeature g_noWaveTrail("No wave trail", "no_wave_trail", sig::kHardStreakAddPoint, 0, {0xC3});
// jne -> jmp: el efecto fantasma (portal de "ghost") nunca se crea
PatchFeature g_noGhostTrail("No ghost trail", "no_ghost_trail", sig::kToggleGhostEffect, sig::kToggleGhostEffectJump,
                            {0x90, 0xE9});
// je -> jmp: el teletransporte de la araña no crea su efecto
PatchFeature g_noSpiderDash("No spider dash", "no_spider_dash", sig::kSpiderDashSkip, sig::kSpiderDashSkipJump,
                            {0x90, 0xE9});

// --- Labels ---
Labels g_labels;

// --- Shortcuts ---
Shortcuts g_shortcuts;

// --- Bot ---
Bot g_bot;

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
        g_trajectory.load(section(player, "trajectory"));
        g_respawnDelay.load(section(player, "respawn_delay"));
        g_noDeathEffect.load(player);
        g_noRespawnFlash.load(player);

        const nlohmann::json level = section(data, "level");
        g_autoPractice.load(section(level, "auto_practice"));
        g_hidePause.load(section(level, "hide_pause"));
        g_startPosSwitcher.load(section(level, "startpos_switcher"));
        g_practiceMusicSync.load(level);

        const nlohmann::json bypass = section(data, "bypass");
        g_unlockIcons.load(bypass);
        g_textLimit.load(bypass);
        g_textFilter.load(bypass);
        g_unlockMainLevels.load(bypass);
        g_unlockVaults.load(bypass);
        g_unlockBasement.load(bypass);
        g_unlockChallenge.load(bypass);
        g_unlockTreasureRoom.load(bypass);
        g_unlockShops.load(bypass);
        g_sliderLimit.load(bypass);

        const nlohmann::json creator = section(data, "creator");
        g_freeScroll.load(creator);
        g_levelCopy.load(creator);
        g_customObjects.load(creator);
        g_verify.load(creator);
        g_placeOver.load(creator);
        g_levelEdit.load(creator);
        g_defaultSongs.load(creator);
        g_toolboxButtons.load(creator);

        const nlohmann::json cosmetic = section(data, "cosmetic");
        g_noShake.load(cosmetic);
        g_noLightning.load(cosmetic);
        g_noCircles.load(cosmetic);
        g_noTrail.load(cosmetic);
        g_noWaveTrail.load(cosmetic);
        g_noGhostTrail.load(cosmetic);
        g_noSpiderDash.load(cosmetic);
        g_noMirror.load(cosmetic);
        g_noParticles.load(cosmetic);

        g_labels.load(section(data, "labels"));

        g_bot.load(section(data, "bot"));

        gui::theme::load(section(data, "interface"));
        if (gui::theme::configOutdated()) markDirty(); // se guarda con el formato nuevo
        gui::keybinds::load(section(data, "keybinds"));
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
    g_trajectory.save(player["trajectory"]);
    g_respawnDelay.save(player["respawn_delay"]);
    g_noDeathEffect.save(player);
    g_noRespawnFlash.save(player);

    nlohmann::json& level = data["level"];
    g_autoPractice.save(level["auto_practice"]);
    g_hidePause.save(level["hide_pause"]);
    g_startPosSwitcher.save(level["startpos_switcher"]);
    g_practiceMusicSync.save(level);

    nlohmann::json& bypass = data["bypass"];
    g_unlockIcons.save(bypass);
    g_textLimit.save(bypass);
    g_textFilter.save(bypass);
    g_unlockMainLevels.save(bypass);
    g_unlockVaults.save(bypass);
    g_unlockBasement.save(bypass);
    g_unlockChallenge.save(bypass);
    g_unlockTreasureRoom.save(bypass);
    g_unlockShops.save(bypass);
    g_sliderLimit.save(bypass);

    nlohmann::json& creator = data["creator"];
    g_freeScroll.save(creator);
    g_levelCopy.save(creator);
    g_customObjects.save(creator);
    g_verify.save(creator);
    g_placeOver.save(creator);
    g_levelEdit.save(creator);
    g_defaultSongs.save(creator);
    g_toolboxButtons.save(creator);

    nlohmann::json& cosmetic = data["cosmetic"];
    g_noShake.save(cosmetic);
    g_noLightning.save(cosmetic);
    g_noCircles.save(cosmetic);
    g_noTrail.save(cosmetic);
    g_noWaveTrail.save(cosmetic);
    g_noGhostTrail.save(cosmetic);
    g_noSpiderDash.save(cosmetic);
    g_noMirror.save(cosmetic);
    g_noParticles.save(cosmetic);

    g_labels.save(data["labels"]);

    g_bot.save(data["bot"]);

    gui::theme::save(data["interface"]);
    gui::keybinds::save(data["keybinds"]);
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
    const bool gameAvailable = core::game::init(); // GameManager y hooks de nivel compartidos
    g_noDeathEffect.init();
    g_noShake.init();
    g_noLightning.init();
    g_noCircles.init();
    g_noTrail.init();
    g_noWaveTrail.init();
    g_noGhostTrail.init();
    g_noSpiderDash.init(); // antes que g_trajectory, que hookea el inicio de esa función
    g_noMirror.init();
    g_noParticles.init();
    g_noRespawnFlash.init();
    g_practiceMusicSync.init();
    g_labels.init(gameAvailable);
    g_unlockIcons.init();
    g_textLimit.init();
    g_textFilter.init();
    g_unlockMainLevels.init();
    g_unlockVaults.init();
    g_unlockBasement.init();
    g_unlockChallenge.init();
    g_unlockTreasureRoom.init();
    g_unlockShops.init();
    g_sliderLimit.init();
    g_freeScroll.init();
    g_levelCopy.init();
    g_customObjects.init();
    g_startPosSwitcher.init(gameAvailable);
    g_instantComplete.init(gameAvailable);
    g_shortcuts.init(gameAvailable);
    g_bot.init(gameAvailable);
    g_verify.init();
    g_placeOver.init();
    g_levelEdit.init();
    g_defaultSongs.init();
    g_toolboxButtons.init();
    g_autoPractice.init();
    g_hidePause.init();
    g_respawnDelay.init(destroyPlayer);
    g_hitboxes.init();
    g_trajectory.init(gameAvailable);
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
    g_trajectory.disable();
    g_respawnDelay.disable();
    g_noDeathEffect.disable();
    g_noRespawnFlash.disable();
    g_noShake.disable();
    g_noLightning.disable();
    g_noCircles.disable();
    g_noTrail.disable();
    g_noWaveTrail.disable();
    g_noGhostTrail.disable();
    g_noSpiderDash.disable();
    g_noMirror.disable();
    g_noParticles.disable();
    g_autoPractice.disable();
    g_hidePause.disable();
    g_startPosSwitcher.disable();
    g_practiceMusicSync.disable();
    g_unlockIcons.disable();
    g_textLimit.disable();
    g_textFilter.disable();
    g_unlockMainLevels.disable();
    g_unlockVaults.disable();
    g_unlockBasement.disable();
    g_unlockChallenge.disable();
    g_unlockTreasureRoom.disable();
    g_unlockShops.disable();
    g_sliderLimit.disable();
    g_freeScroll.disable();
    g_levelCopy.disable();
    g_customObjects.disable();
    g_verify.disable();
    g_placeOver.disable();
    g_levelEdit.disable();
    g_defaultSongs.disable();
    g_toolboxButtons.disable();
    g_bot.disable();
    gui::gamefont::shutdown(); // suelta las texturas de las fuentes de GD
    g_ready = false;
}

void update() {
    if (!g_ready) return;
    gui::keybinds::update();
    if (gui::keybinds::takeChanged() || g_labels.takePositionChanged()) markDirty();
    g_startPosSwitcher.update();
    // Se espera un momento tras el último cambio para no escribir el archivo en cada tecla
    if (g_dirty && GetTickCount64() - g_dirtySince > 500) saveConfig();
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
    changed |= g_trajectory.drawUI();
    changed |= g_respawnDelay.drawUI();
    if (changed) markDirty();
}

void drawCosmetic() {
    bool changed = g_noDeathEffect.drawUI();
    changed |= g_noRespawnFlash.drawUI();
    changed |= g_noShake.drawUI();
    changed |= g_noLightning.drawUI();
    changed |= g_noCircles.drawUI();
    changed |= g_noParticles.drawUI();
    changed |= g_noMirror.drawUI();
    changed |= g_noTrail.drawUI();
    changed |= g_noWaveTrail.drawUI();
    changed |= g_noGhostTrail.drawUI();
    changed |= g_noSpiderDash.drawUI();
    if (changed) markDirty();
}

void drawLevel() {
    bool changed = g_autoPractice.drawUI();
    changed |= g_practiceMusicSync.drawUI();
    changed |= g_hidePause.drawUI();
    changed |= g_startPosSwitcher.drawUI();
    if (changed) markDirty();
}

void drawBypass() {
    bool changed = g_unlockIcons.drawUI();
    changed |= g_textLimit.drawUI();
    changed |= g_textFilter.drawUI();
    changed |= g_sliderLimit.drawUI();
    changed |= g_unlockMainLevels.drawUI();
    changed |= g_unlockVaults.drawUI();
    changed |= g_unlockBasement.drawUI();
    changed |= g_unlockChallenge.drawUI();
    changed |= g_unlockTreasureRoom.drawUI();
    changed |= g_unlockShops.drawUI();
    if (changed) markDirty();
    g_instantComplete.drawUI();
}

void drawCreator() {
    bool changed = g_freeScroll.drawUI();
    changed |= g_levelCopy.drawUI();
    changed |= g_customObjects.drawUI();
    changed |= g_verify.drawUI();
    changed |= g_placeOver.drawUI();
    changed |= g_levelEdit.drawUI();
    changed |= g_defaultSongs.drawUI();
    changed |= g_toolboxButtons.drawUI();
    if (changed) markDirty();
}

void drawLabels() {
    if (g_labels.drawUI()) markDirty();
}

void drawBot() {
    if (g_bot.drawUI()) markDirty();
}

void drawShortcuts() {
    g_shortcuts.drawUI();
}

void drawInterface() {
    if (gui::theme::drawUI()) markDirty();
}

void drawOverlay() {
    if (!g_ready) return;
    g_trajectory.drawOverlay();
    g_labels.drawOverlay(cheatsActive());
    g_startPosSwitcher.drawOverlay();
}

bool cheatsActive() {
    return g_speedhack.active() || g_tpsBypass.active() || g_noclip.active() || g_bot.active();
}

} // namespace features
