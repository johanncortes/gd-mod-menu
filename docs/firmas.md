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
