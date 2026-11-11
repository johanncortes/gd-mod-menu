# Registro de firmas

| Función | Módulo | Firma (patrón) | Versión GD | Notas |
|---|---|---|---|---|
| CCScheduler::update | libcocos2d.dll | export por nombre | — | estable |
| wglSwapBuffers | opengl32.dll | export por nombre | — | hook de render: aquí se dibuja ImGui (fase 1) |
| GetRawInputData | user32.dll | export por nombre | — | hook de input: GD lee clics y teclas por raw input en un hilo aparte (fase 1) |
| CCScheduler::update | libcocos2d.dll | export `?update@CCScheduler@cocos2d@@UEAAXM@Z` | — | hook del speedhack: multiplica `dt` (fase 2) |
| FMOD::System::update | fmod.dll | export `?update@System@FMOD@@QEAA?AW4FMOD_RESULT@@XZ` | — | hook: de aquí sale el puntero al sistema de FMOD para el audio del speedhack (fase 2) |
| FMOD::System::getMasterChannelGroup / ChannelControl::setPitch | fmod.dll | exports por nombre | — | tono del grupo maestro = velocidad del speedhack (fase 2) |
| CCApplication::setAnimationInterval | libcocos2d.dll | export `?setAnimationInterval@CCApplication@cocos2d@@UEAAXN@Z` | — | hook del FPS bypass. El bucle de `CCApplication::run` espera ese intervalo solo con VSync apagado (fase 2) |
| CCApplication::sharedApplication, getVerticalSyncEnabled; CCDirector::sharedDirector, getAnimationInterval | libcocos2d.dll | exports por nombre | — | FPS bypass: el director guarda el intervalo que pidió el juego, se usa para restaurarlo (fase 2) |
| GJBaseGameLayer::getModifiedDelta (`mulsd xmm7,[1/240]`) | GeometryDash.exe | `0F 5A F8 0F 57 C0 F2 0F 59 3D ?? ?? ?? ?? F3 0F 5A C1 F2 0F 58 83` (disp en +10) | build Steam 26002420 (exe 2026-10-16) | `kTickLength`. Función en 0x140340630, instrucción en 0x140340671. Physics bypass: se redirige el operando a una constante propia (fase 2) |
| GJBaseGameLayer::update (`mulsd xmm1,[4.0]`) | GeometryDash.exe | `0F 5A C0 41 0F 28 CF F2 0F 5E C8 F2 0F 59 0D ?? ?? ?? ?? 0F 57 C0 F2 0F 5A C1 FF 15` (disp en +15) | build Steam 26002420 (exe 2026-10-16) | `kStepsPerFrame`. Función en 0x1403406D0, instrucción en 0x140340900. pasos = round(dt*60 / timewarp * 4) (fase 2) |
| PlayLayer::destroyPlayer(PlayerObject*, GameObject*) | GeometryDash.exe | inicio de función, ver `kDestroyPlayer` en `signatures.hpp` (hasta el final de la comprobación anti-trampas) | build Steam 26002420 (exe 2026-10-16) | 0x140512390. Virtual, slot 0x478 del vtable de PlayLayer. Hook del noclip. De la firma se leen `m_anticheatSpike` (+0x38E0, byte 0x90 de la firma) y `m_uniqueID` (+0x39C, byte 0x9B) (fase 3) |
| PlayLayer::resetLevel() | GeometryDash.exe | inicio de función, ver `kResetLevel` | build Steam 26002420 (exe 2026-10-16) | 0x140518C00. Virtual, slot 0x5B0. Hook: reinicia el contador de noclip (fase 3) |
| GJBaseGameLayer::update(float) | GeometryDash.exe | inicio de función, ver `kGameLayerUpdate` | build Steam 26002420 (exe 2026-10-16) | 0x1403406D0. Hook: cuenta frames para la precisión del noclip (fase 3) |
| PlayLayer: decide si se dibujan las hitboxes (virtual, slot 0x488) | GeometryDash.exe | `80 B9 ?? ?? ?? ?? 00 74 09 80 B9 ?? ?? ?? ?? 00 75 12 80 B9 ?? ?? ?? ?? 00 74 0C 80 B9 ?? ?? ?? ?? 00 74 03 B0 01 C3 32 C0 C3 CC CC CC CC CC CC 80 B9 ?? ?? ?? ?? 00 74 09` | build Steam 26002420 (exe 2026-10-16) | `kShowHitboxes`, 0x140505BC0. Devuelve (variable 0166 y modo práctica) o (variable 0179 y jugador muerto). Hook: devuelve true. La firma incluye el inicio de la función gemela del slot 0x490 para ser única (fase 3) |
| PlayLayer: mostrar/ocultar el nodo de hitboxes | GeometryDash.exe | `48 89 5C 24 08 57 48 83 EC 20 48 8B 01 48 8B D9 FF 90 ?? ?? ?? ?? 84 C0 75 15 48 8B 03 48 8B CB FF 90 ?? ?? ?? ?? 84 C0 75 05 40 32 FF EB 03 40 B7 01 48 8B 8B ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ?? 40 3A F8 74` | build Steam 26002420 (exe 2026-10-16) | `kUpdateHitboxVisibility`, 0x140505E40. Se llama al activar o desactivar la opción (fase 3) |
| PlayerObject::playerDestroyed(bool): salto al efecto de muerte | GeometryDash.exe | `80 BB ?? ?? ?? ?? 00 74 20 40 84 FF 75 1B 40 38 BB ?? ?? ?? ?? 75 12 48 8B CB 48 8B 5C 24 30 48 83 C4 20 5F E9` (parche en +7) | build Steam 26002420 (exe 2026-10-16) | `kDeathEffect`, 0x1404F6099 (función en 0x1404F5FF0). Parche `74` -> `EB`: nunca salta a playDeathEffect (0x1404DD160) (fase 3) |
| PlayerObject: parpadeo al reaparecer (CCBlink) | GeometryDash.exe | inicio de función, ver `kSpawnEffect` | build Steam 26002420 (exe 2026-10-16) | 0x1404F5710. La llama resetLevel para cada jugador. Parche: `C3` (ret) en el primer byte (fase 3) |
| CCDelayTime::create | libcocos2d.dll | export `?create@CCDelayTime@cocos2d@@SAPEAV12@M@Z` | — | hook del respawn delay: cambia la duración solo si la dirección de retorno está dentro de destroyPlayer (fase 3) |
| PlayLayer::toggleMusicInPractice: valor de m_practiceMusicSync | GeometryDash.exe | ver `kPracticeMusicSync` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x1405114AF (función en 0x1405113B0). Parche en +31: el `mov byte [m_practiceMusicSync],0` pasa a 1 (fase 4) |
| PlayLayer::setupHasCompleted() | GeometryDash.exe | inicio de función, ver `kSetupHasCompleted` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x140504710. Hook del auto practice: después llama a togglePracticeMode(true) (fase 4) |
| PlayLayer::togglePracticeMode(bool) | GeometryDash.exe | inicio de función, ver `kTogglePracticeMode` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x140519FF0. Solo se llama, no se hookea (fase 4) |
| PauseLayer::customSetup() | GeometryDash.exe | inicio de función, ver `kPauseLayerSetup` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x1404D68E0. Hook: tras construir el menú de pausa lo oculta (fase 4) |
| GJBaseGameLayer::getStartPosObjectsForCurrentMode() | GeometryDash.exe | inicio de función, ver `kStartPosObjects` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x1403190F0. No se llama (fuera del editor y en modo normal devuelve una lista vacía): de la firma se lee en +41 el offset de `m_startPosObjects` (+0x3278), el CCArray con todos los StartPos, que `PlayLayer::addObject` rellena en cualquier nivel (fase 4) |
| PauseLayer::onResume: tramo final (switchToStartPosObject en línea) | GeometryDash.exe | ver `kSwitchStartPos` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x1404DAD4C. No se parchea: de la firma se leen el global de GameManager (0x14090BFD0), m_playLayer (+0x230), m_startPosObject (+0x3270), el byte de test mode (+0x3328), los slots 0x5B0 (resetLevel) y 0x430, y las direcciones de resetLevelFromStart (0x140518A50), updateCamera (0x140345C60), updateTestModeLabel (0x140505ED0) y startMusic (0x14051AED0) (fase 4) |
| GameManager::isIconUnlocked(int, IconType) | GeometryDash.exe | inicio de función, ver `kIsIconUnlocked` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x1402773D0. Parche `B0 01 C3` (mov al,1; ret) (fase 4) |
| GameManager::isColorUnlocked(int, UnlockType) | GeometryDash.exe | inicio de función, ver `kIsColorUnlocked` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x1402777A0. Parche `B0 01 C3` (fase 4) |
| EditorUI::constrainGameLayerPosition(float, float) | GeometryDash.exe | inicio de función, ver `kConstrainGameLayer` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x1401BF010. Parche `C3` (ret): free scroll (fase 4) |
| LevelInfoLayer::tryCloneLevel | GeometryDash.exe | inicio de función, ver `kTryCloneLevel` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x140422680. Parche en +0x33: `75 1A` -> `90 90`, copia sin pedir contraseña (fase 4) |
| LevelInfoLayer::init: visibilidad del botón de copiar | GeometryDash.exe | ver `kCloneButton` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x14041DABF. Parche en +26: `0F 95 C2` (setne dl) -> `B2 01 90` (fase 4) |
| EditorUI::onNewCustomItem: límite de 1000 objetos seleccionados | GeometryDash.exe | ver `kCustomObjectSize` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x1401F5AC1. Parche en +27: `0F 86` -> `90 E9` (fase 4) |
| EditorUI::onNewCustomItem: límite de 200 objetos guardados | GeometryDash.exe | ver `kCustomObjectCount` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x1401F5C61. Parche en +18: `0F 82` -> `90 E9` (fase 4) |
| CCTextInputNode::onTextFieldInsertText: recorte al pegar | GeometryDash.exe | ver `kTextLimitPaste` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x14005751E (función en 0x1400571F0). Parche en +9: `0F 8E` -> `90 E9` (fase 4) |
| CCTextInputNode::onTextFieldInsertText: máximo al escribir | GeometryDash.exe | ver `kTextLimitType` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x1400578A0. Parche en +16: `7C` -> `EB` (fase 4) |
| Filtro de caracteres de los campos de texto | GeometryDash.exe | ver `kTextFilter` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x1400540F1 (función en 0x140053E20). Parche en +6: `74` -> `EB`, acepta cualquier carácter (fase 4) |
| CCNode::setVisible; CCArray::count, objectAtIndex; CCObject::retain, release | libcocos2d.dll | exports por nombre | — | hide pause menu y StartPos switcher (fase 4) |
| ShareLevelLayer::onShare: comprobación de nivel verificado | GeometryDash.exe | ver `kVerify` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x140631DF6 (función en 0x140631CE0). Parche en +15: `0F 84` -> `90 E9`, no sale el aviso "Unverified" (fase 4) |
| PlayLayer::levelComplete() | GeometryDash.exe | inicio de función, ver `kLevelComplete` | 2.2091, build Steam 26002420 | 0x1405061B0. Solo se llama (botón Instant complete); no se toca su anti-trampas (fase 4) |
| PlayLayer::getCurrentPercent() | GeometryDash.exe | inicio de función, ver `kCurrentPercent` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x140512300. Devuelve float. Solo se llama, para el label de porcentaje (fase 5) |
| PlayLayer::updateAttempts() | GeometryDash.exe | inicio de función, ver `kUpdateAttempts` | 2.2091, build Steam 26002420 | 0x140519D30. No se llama: de la firma se lee en +13 el offset de `m_attempts` (+0x3164) (fase 5) |
| UILayer::onCheck(CCObject*) | GeometryDash.exe | función entera, ver `kUiOnCheck` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x140673050. Botón de poner checkpoint. No usa sus argumentos (saca el PlayLayer de GameManager) y solo actúa en modo práctica. Solo se llama (fase 6) |
| UILayer::onDeleteCheck(CCObject*) | GeometryDash.exe | función entera, ver `kUiOnDeleteCheck` | 2.2091, build Steam 26002420 | 0x1406730D0. Botón de quitar checkpoint: llama a `PlayLayer::removeCheckpoint(false)`. Solo se llama (fase 6) |
| GJBaseGameLayer::processCommands(float, bool, bool) | GeometryDash.exe | inicio de función, ver `kProcessCommands` en `signatures.hpp` | 2.2091, build Steam 26002420 | 0x140342870. Hook del bot (un paso de física). De la firma se lee en +17 el offset del contador de ticks, `m_gameState.m_commandIndex` (+0x3E0) (fase 7) |
| GJBaseGameLayer::handleButton(bool, int, bool) | GeometryDash.exe | inicio de función, ver `kHandleButton` | 2.2091, build Steam 26002420 | 0x14033C5B0. Hook del bot: aquí se graban los inputs y con el original se reproducen (fase 7) |
| PlayLayer::playEndAnimationToPos(CCPoint) | GeometryDash.exe | inicio de función, ver `kPlayEndAnimation` | 2.2091, build Steam 26002420 | 0x14050A2A0. Reproduce la animación de fin de nivel hacia una posición (fase 8) |
| CCNode::setPosition | libcocos2d.dll | export `?setPosition@CCNode@cocos2d@@UEAAXAEBVCCPoint@2@@Z` | — | teletransporta al jugador a la meta en Instant complete (fase 8) |

## Notas de ingeniería inversa

- **Paso de física (build 26002420)**: `getModifiedDelta` redondea el `dt` a múltiplos de `min(timewarp,1) * 1/240`
  y `update` calcula `pasos = round(dt*60 / min(timewarp,1) * 4.0)`. La constante `double 1/240` (0x140835040) la leen
  otras 3 instrucciones (0x1403208CF, 0x140320B88, 0x14033EA82: conversión tiempo -> índice del historial de posiciones
  que usa `[this+0x4D4]`), y el `4.0` lo comparte mucho código: por eso no se parchean las constantes sino las instrucciones.
- Las direcciones absolutas de esta tabla son solo orientativas (imagen base 0x140000000); el código usa siempre la firma.
- **Muerte y reaparición (build 26002420)**: `destroyPlayer` sale sin hacer nada si el jugador ya está muerto o si el
  objeto es el pincho anti-trampas (compara `m_uniqueID`; solo marca un flag). Después llama a
  `PlayerObject::playerDestroyed` y programa el reinicio con `CCSequence(CCDelayTime(t), CCCallFunc(...))`:
  hay exactamente dos llamadas a `CCDelayTime::create` dentro de la función (0x140513093 práctica, 0x14051361C normal).
- **Hitboxes (build 26002420)**: el juego ya sabe dibujarlas (`GJBaseGameLayer::updateDebugDraw`, slot 0x480, 0x1403169C0).
  Cada frame pregunta a los slots 0x488/0x490 si toca dibujar; la visibilidad del nodo (`[PlayLayer+0x3870]`) solo se
  actualiza en `kUpdateHitboxVisibility`.
- **RTTI**: el exe conserva RTTI de MSVC (`.?AVPlayLayer@@`, vtable en 0x1407D59B8). Sirve para identificar funciones
  virtuales a partir del vtable cuando no hay cadenas de texto cerca.
- **Comprobar firmas sin abrir el juego**: `build/Release/sigcheck.exe "<ruta>/GeometryDash.exe"`.
- **Bindings de Geode**: el exe del build 26002420 es GD **2.2091**. El repositorio público
  `geode-sdk/bindings` (`bindings/2.2091/GeometryDash.bro`) trae nombre y dirección `win` de casi todas las
  funciones y coincide con este exe (comprobado con destroyPlayer y resetLevel). Sirve para saber *dónde* mirar;
  en el código siguen yendo firmas, nunca esas direcciones.
- **StartPos (2.2091)**: el juego ya trae selector de StartPos en el menú de pausa (`PauseLayer::onChangeStartpos`,
  `getQueuedStartPos`). Al reanudar hace `switchToStartPosObject`: si es el mismo, `resetLevel()`; si no, cambia
  `m_startPosObject` (retain/release), pone el byte de test mode, `resetLevelFromStart()`, virtual 0x430 con 0.0,
  `updateCamera(0.0)`, `updateTestModeLabel()` y siempre `startMusic()`.
- **Práctica (2.2091)**: `m_isPracticeMode` en PlayLayer+0x32E0 y `m_practiceMusicSync` en +0x32E1.
  `setupHasCompleted` llama a `resetLevel()` y programa `startGame` con un CCDelayTime.
- **Copiar nivel**: la contraseña del nivel es `[level+0x41C] - [level+0x420]` (valor con semilla): 0 = no se puede
  copiar, 1 = copia libre, otro = pide contraseña.
- **Campos de texto**: `onTextFieldInsertText` recibe el puntero al sub-objeto delegado (this+0x1A0); desde ahí
  `m_maxLabelLength` está en +0xC0.
- **Inputs y ticks (2.2091)**: `processCommands` copia `m_commandIndex` (+0x3E0) a `m_stepCommandIndex` (+0x3EC), lo
  incrementa (2 por paso normal, 1 por medio paso) y llama a `processQueuedButtons`, que pasa los inputs pendientes
  a `handleButton`. `m_commandIndex` se pone a 0 en `PlayLayer::resetLevel` y se restaura al cargar un checkpoint.
  El juego trae además un replay propio: con el byte +0x3280 activo, recorre un vector en +0x3478 de entradas de
  16 bytes (botón, pulsado, jugador 2, tick) y llama a `handleButton` cuando el tick coincide con +0x3EC. El bot
  hace lo mismo desde su hook, sin usar ese vector.
