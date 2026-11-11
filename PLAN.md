# Plan — GD Mod Menu

## Estado actual
- Fase: 0 (proyecto recién creado, sin código).
- Último agente: —
- Próximo paso: Fase 1, esqueleto.
- Notas / bloqueos: ninguno.

## Fase 1 — Esqueleto
- [ ] CMakeLists con MinHook, ImGui y nlohmann/json vía FetchContent
- [ ] `dllmain.cpp` + hilo de inicio + consola de debug opcional
- [ ] Hook `wglSwapBuffers` y `WndProc`; ImGui dibujando
- [ ] Panel con Tab y ventanas vacías: Global, Level, Bypass, Player, Bot, Creator, Labels, Shortcuts, Interface
- [ ] Inyector para desarrollo
- [ ] Probado dentro del juego

## Fase 2 — Global (cocos2d, estables)
- [ ] Speedhack (hook `CCScheduler::update`)
- [ ] FPS bypass
- [ ] Physics bypass (TPS)
- [ ] Guardar/cargar config.json

## Fase 3 — Player
- [ ] Noclip (+ contador de muertes y precisión)
- [ ] Mostrar hitboxes
- [ ] No death effect / no respawn flash
- [ ] Respawn delay
- [ ] Show trajectory

## Fase 4 — Level / Bypass / Creator (parches de bytes)
- [ ] Practice music sync, auto practice mode, hide pause menu
- [ ] Unlock icons, character/text limit bypass
- [ ] Object limit bypass, free scroll, copy level bypass, verify bypass
- [ ] StartPos switcher

## Fase 5 — Labels
- [ ] FPS, CPS, porcentaje, intentos, tiempo, reloj
- [ ] Cheat indicator, noclip accuracy

## Fase 6 — Interface y Shortcuts
- [ ] Temas, color de acento, escala, fuente
- [ ] Keybinds (clic derecho en cualquier opción)
- [ ] Atajos: reiniciar nivel, toggle practice, checkpoints

## Fase 7 — Bot (al final)
- [ ] Grabar inputs
- [ ] Reproducir inputs
- [ ] Guardar/cargar replays

## Mantenimiento por actualización de GD
1. Ver qué firmas de `docs/firmas.md` fallan (el panel las marca "no disponible").
2. Usar REA para volver a encontrarlas.
3. Actualizar `signatures.hpp` y `docs/firmas.md` con la nueva versión.
