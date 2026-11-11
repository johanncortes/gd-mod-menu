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

## Notas de ingeniería inversa

- **Paso de física (build 26002420)**: `getModifiedDelta` redondea el `dt` a múltiplos de `min(timewarp,1) * 1/240`
  y `update` calcula `pasos = round(dt*60 / min(timewarp,1) * 4.0)`. La constante `double 1/240` (0x140835040) la leen
  otras 3 instrucciones (0x1403208CF, 0x140320B88, 0x14033EA82: conversión tiempo -> índice del historial de posiciones
  que usa `[this+0x4D4]`), y el `4.0` lo comparte mucho código: por eso no se parchean las constantes sino las instrucciones.
- Las direcciones absolutas de esta tabla son solo orientativas (imagen base 0x140000000); el código usa siempre la firma.
