// Comprueba las firmas de src/core/signatures.hpp contra un GeometryDash.exe en disco,
// sin abrir el juego. Uso:  sigcheck.exe "C:\ruta\a\GeometryDash.exe"
// Tras una actualización de GD dice qué firmas hay que volver a buscar.
#include <windows.h>

#include <cstdio>
#include <cstring>

#include "core/memory.hpp"
#include "core/signatures.hpp"

namespace sig = core::signatures;

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: sigcheck.exe <ruta a GeometryDash.exe>\n");
        return 2;
    }
    // Mapea el exe como imagen, sin ejecutarlo ni cargar sus DLL
    HMODULE exe = LoadLibraryExA(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!exe) {
        printf("No se pudo abrir %s (error %lu)\n", argv[1], GetLastError());
        return 2;
    }
    const auto* base = reinterpret_cast<const uint8_t*>(exe);

    const struct {
        const char* name;
        const char* pattern;
    } signatures[] = {
        {"kTickLength", sig::kTickLength},
        {"kStepsPerFrame", sig::kStepsPerFrame},
        {"kDestroyPlayer", sig::kDestroyPlayer},
        {"kResetLevel", sig::kResetLevel},
        {"kGameLayerUpdate", sig::kGameLayerUpdate},
        {"kShowHitboxes", sig::kShowHitboxes},
        {"kUpdateHitboxVisibility", sig::kUpdateHitboxVisibility},
        {"kDeathEffect", sig::kDeathEffect},
        {"kSpawnEffect", sig::kSpawnEffect},
        {"kPracticeMusicSync", sig::kPracticeMusicSync},
        {"kSetupHasCompleted", sig::kSetupHasCompleted},
        {"kTogglePracticeMode", sig::kTogglePracticeMode},
        {"kPauseLayerSetup", sig::kPauseLayerSetup},
        {"kStartPosObjects", sig::kStartPosObjects},
        {"kSwitchStartPos", sig::kSwitchStartPos},
        {"kIsIconUnlocked", sig::kIsIconUnlocked},
        {"kIsColorUnlocked", sig::kIsColorUnlocked},
        {"kConstrainGameLayer", sig::kConstrainGameLayer},
        {"kTryCloneLevel", sig::kTryCloneLevel},
        {"kCloneButton", sig::kCloneButton},
        {"kCustomObjectSize", sig::kCustomObjectSize},
        {"kCustomObjectCount", sig::kCustomObjectCount},
        {"kTextLimitPaste", sig::kTextLimitPaste},
        {"kTextLimitType", sig::kTextLimitType},
        {"kTextFilter", sig::kTextFilter},
        {"kVerify", sig::kVerify},
        {"kLevelComplete", sig::kLevelComplete},
        {"kCurrentPercent", sig::kCurrentPercent},
        {"kUpdateAttempts", sig::kUpdateAttempts},
        {"kUiOnCheck", sig::kUiOnCheck},
        {"kUiOnDeleteCheck", sig::kUiOnDeleteCheck},
        {"kProcessCommands", sig::kProcessCommands},
        {"kHandleButton", sig::kHandleButton},
        {"kCheckForEnd", sig::kCheckForEnd},
        {"kPlayerCreate", sig::kPlayerCreate},
        {"kCopyAttributes", sig::kCopyAttributes},
        {"kUpdateRotation", sig::kUpdateRotation},
        {"kPushButton", sig::kPushButton},
        {"kReleaseButton", sig::kReleaseButton},
        {"kCheckCollisions", sig::kCheckCollisions},
        {"kCollisionCheckObjects", sig::kCollisionCheckObjects},
        {"kObjectTypeRead", sig::kObjectTypeRead},
        {"kSpiderDashEffect", sig::kSpiderDashEffect},
        {"kCollisionLogReset", sig::kCollisionLogReset},
        {"kGameEventTriggered", sig::kGameEventTriggered},
        {"kDestroyObject", sig::kDestroyObject},
        {"kShakeCamera", sig::kShakeCamera},
        {"kLightningFlash", sig::kLightningFlash},
        {"kCircleWaveDraw", sig::kCircleWaveDraw},
        {"kHardStreakAddPoint", sig::kHardStreakAddPoint},
        {"kActivateStreakResume", sig::kActivateStreakResume},
        {"kToggleGhostEffect", sig::kToggleGhostEffect},
        {"kSpiderDashSkip", sig::kSpiderDashSkip},
        {"kToggleFlipped", sig::kToggleFlipped},
        {"kMainLevelLock", sig::kMainLevelLock},
        {"kVaultLock", sig::kVaultLock},
        {"kVaultOfSecretsLock", sig::kVaultOfSecretsLock},
        {"kTreasureRoomLock", sig::kTreasureRoomLock},
        {"kBasementLock", sig::kBasementLock},
        {"kChallengeLevelLock", sig::kChallengeLevelLock},
        {"kShopLocks", sig::kShopLocks},
        {"kSliderClamp", sig::kSliderClamp},
        {"kPlaceOver", sig::kPlaceOver},
        {"kPauseEditButton", sig::kPauseEditButton},
        {"kPauseOnEdit", sig::kPauseOnEdit},
        {"kSongSelectArrows", sig::kSongSelectArrows},
        {"kToolboxPerRow", sig::kToolboxPerRow},
        {"kToolboxRows", sig::kToolboxRows},
        {"kPauseGameEnding", sig::kPauseGameEnding},
        {"kCheckpointLimit", sig::kCheckpointLimit},
        {"kLoadLevelSettings", sig::kLoadLevelSettings},
        {"kAttemptDiscardFlags", sig::kAttemptDiscardFlags},
        {"kEndLevelJumps", sig::kEndLevelJumps},
        {"kMusicSliderPause", sig::kMusicSliderPause},
        {"kSfxSlider", sig::kSfxSlider},
        {"kMusicSliderOptions", sig::kMusicSliderOptions},
        {"kEditorLayerStore", sig::kEditorLayerStore},
        {"kPushButtonModes", sig::kPushButtonModes},
        {"kAddGlow", sig::kAddGlow},
        {"kStreakBlend", sig::kStreakBlend},
        {"kLevelSettingsPlayers", sig::kLevelSettingsPlayers},
        {"kPlayLayerInit", sig::kPlayLayerInit},
        {"kPlayLayerAddObject", sig::kPlayLayerAddObject},
        {"kAddObjectSection", sig::kAddObjectSection},
        {"kAddToSection", sig::kAddToSection},
        {"kObjectRectScale", sig::kObjectRectScale},
        {"kObjectRadius", sig::kObjectRadius},
        {"kShowNewBest", sig::kShowNewBest},
        {"kRewardStep2", sig::kRewardStep2},
        {"kRewardShowClose", sig::kRewardShowClose},
        {"kLevelAttempts", sig::kLevelAttempts},
        {"kRandomSeed", sig::kRandomSeed},
        {"kEditorTrailInterval", sig::kEditorTrailInterval},
        {"kSavePercentage", sig::kSavePercentage},
        {"kLoadStartPosSettings", sig::kLoadStartPosSettings},
        {"kStartPosResetCamera", sig::kStartPosResetCamera},
        {"kSetupLevelStart", sig::kSetupLevelStart},
        {"kSetupLevelStartDual", sig::kSetupLevelStartDual},
        {"kSetupLevelStartMini", sig::kSetupLevelStartMini},
        {"kSetupLevelStartMirror", sig::kSetupLevelStartMirror},
        {"kPlatformerStartMode", sig::kPlatformerStartMode},
        {"kPlatformerWave", sig::kPlatformerWave},
        {"kPlatformerSwing", sig::kPlatformerSwing},
        {"kAudioScale", sig::kAudioScale},
        {"kRingSpawnCircle", sig::kRingSpawnCircle},
        {"kSecretCoinPractice", sig::kSecretCoinPractice},
        {"kUserCoinPractice", sig::kUserCoinPractice},
        {"kPauseOnRestart", sig::kPauseOnRestart},
        {"kPauseOnRestartFull", sig::kPauseOnRestartFull},
        {"kPauseOnPracticeMode", sig::kPauseOnPracticeMode},
        {"kPauseOnNormalMode", sig::kPauseOnNormalMode},
        {"kPlayLayerResume", sig::kPlayLayerResume},
    };

    int failed = 0;
    for (const auto& s : signatures) {
        const uint8_t* match = core::memory::findPattern(exe, s.pattern);
        if (match)
            printf("OK     %-24s RVA 0x%llX\n", s.name, static_cast<unsigned long long>(match - base));
        else
            printf("FALLA  %-24s (no aparece o aparece más de una vez)\n", s.name);
        failed += !match;
    }

    // Valores que el menú lee a través de las firmas
    if (const uint8_t* fn = core::memory::findPattern(exe, sig::kDestroyPlayer)) {
        int32_t spike, id;
        memcpy(&spike, fn + sig::kDestroyPlayerSpikeOffset, 4);
        memcpy(&id, fn + sig::kDestroyPlayerIdOffset, 4);
        printf("destroyPlayer: m_anticheatSpike en +0x%X, m_uniqueID en +0x%X\n", spike, id);
    }
    const uint8_t* tick = core::memory::findPattern(exe, sig::kTickLength);
    const uint8_t* steps = core::memory::findPattern(exe, sig::kStepsPerFrame);
    if (tick && steps) {
        auto constantAt = [](const uint8_t* disp) {
            int32_t offset;
            double value;
            memcpy(&offset, disp, 4);
            memcpy(&value, disp + 4 + offset, 8);
            return value;
        };
        const double tickLength = constantAt(tick + sig::kTickLengthDisp);
        const double stepsPerFrame = constantAt(steps + sig::kStepsPerFrameDisp);
        const bool ok = tickLength == 1.0 / 240.0 && stepsPerFrame == 4.0;
        printf("%s  constantes de física: %.10g y %g (se esperan 1/240 y 4)\n", ok ? "OK    " : "FALLA ", tickLength, stepsPerFrame);
        failed += !ok;
    }

    printf(failed ? "\n%d comprobaciones fallidas\n" : "\nTodas las firmas coinciden\n", failed);
    return failed ? 1 : 0;
}
