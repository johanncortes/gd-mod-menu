# GD Mod Menu — reglas para agentes

Mod menu **standalone** para Geometry Dash (Steam, Windows), estilo Mega Hack: una DLL propia cargada en el juego, **sin Geode**. Uso: practicar niveles y opciones de editor.

## Stack
- C++20, **x64**, MSVC (Build Tools 2022), CMake.
- Dependencias vía `FetchContent` en CMake: **MinHook** (hooks) y **Dear ImGui** (backends Win32 + OpenGL3).
- JSON de config: `nlohmann/json` (FetchContent).
- Ingeniería inversa: **REA** (MCP) con Ghidra en `C:\Tools\ghidra_12.2_PUBLIC`.

## Estructura
```
src/
  dllmain.cpp        # entrada de la DLL, crea hilo de inicio
  core/              # hooks base: wglSwapBuffers, WndProc, memoria, firmas
  gui/               # ImGui: ventanas, tema, keybinds
  features/          # una carpeta/archivo por categoría (global, player, level, bypass, creator, labels, bot)
  config/            # guardar/cargar config.json
injector/            # pequeño .exe para inyectar la DLL durante el desarrollo
docs/firmas.md       # registro de firmas encontradas
```

## Reglas técnicas
- Renderizado: hook a `wglSwapBuffers` (opengl32.dll) para dibujar ImGui; subclass/hook de `WndProc` para capturar input. Tecla para abrir/cerrar el panel: **Tab**.
- Funciones de cocos2d: usar los **exports de `libcocos2d.dll`** por nombre (`GetProcAddress`), no firmas. Son estables entre actualizaciones.
- Funciones de `GeometryDash.exe`: **firmas de bytes (pattern scanning)**, nunca offsets fijos. Cada firma va en `src/core/signatures.hpp` con un comentario de la versión de GD, y se registra en `docs/firmas.md`.
- Si una firma no se encuentra, la función se desactiva y se muestra en el panel como "no disponible": **nunca crashear el juego**.
- Parches de bytes: guardar los bytes originales y poder revertirlos al desactivar la opción.
- Config en `config.json` junto a la DLL (toggles, valores, keybinds, tema).
- Indicador de trampas (Cheat Indicator) visible cuando hay funciones de trampa activas (noclip, speedhack, etc.). Nada de funciones para subir completions ni interactuar con servidores de GD.

## Estilo de código
- Simple y legible; nombres en inglés, comentarios en español.
- Cada feature es una clase/archivo pequeño con `enable()`, `disable()` y su UI.
- Sin abstracciones innecesarias.

## Compilar
```
cmake -B build -A x64
cmake --build build --config Release
```
Salida: `build/Release/gdmenu.dll` e `injector.exe`.

## Flujo de trabajo (obligatorio para cualquier agente)
1. Antes de empezar, lee `PLAN.md` (sección **Estado actual**).
2. Trabaja en pasos pequeños; cada paso debe **compilar** antes de hacer commit.
3. Commits en español, formato: `fase N: qué se hizo`.
4. Al terminar o antes de quedarte sin contexto: actualiza **Estado actual** en `PLAN.md`, marca las casillas completadas y haz commit.
5. Toda firma o dirección nueva va a `docs/firmas.md`.
