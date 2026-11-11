# GD Mod Menu

Panel de mods para **Geometry Dash** (Steam, Windows x64), al estilo Mega Hack: una DLL propia que se
carga dentro del juego, **sin Geode**. Está pensado para practicar niveles y para opciones de editor.

> El proyecto está en desarrollo. No sube completions ni habla con los servidores de GD.

## Qué incluye

| Ventana | Opción | Estado |
| --- | --- | --- |
| Global | Speedhack (con opción de acelerar el audio) | Hecho, falta probar en el juego |
| Global | FPS bypass | Hecho, falta probar en el juego |
| Global | Physics bypass (TPS) | Hecho, falta probar en el juego |
| Player | Noclip con contador de muertes y precisión | Hecho, falta probar en el juego |
| Player | Mostrar hitboxes | Hecho, falta probar en el juego |
| Player | Sin efecto de muerte / sin parpadeo al reaparecer | Hecho, falta probar en el juego |
| Player | Respawn delay | Hecho, falta probar en el juego |
| Player | Show trajectory | Pendiente |
| Level, Bypass, Creator, Labels, Shortcuts, Interface, Bot | — | Pendiente (ver [PLAN.md](PLAN.md)) |

Las opciones se guardan en `config.json`, junto a la DLL.

## Requisitos

- Windows 10 u 11 de 64 bits.
- Geometry Dash de Steam. Las firmas están hechas para el build de Steam **26002420**; con otra versión
  algunas opciones pueden salir como "(no disponible)" (ver [Si GD se actualiza](#si-gd-se-actualiza)).
- Para compilar:
  - [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/es/downloads/) con la carga
    "Desarrollo para el escritorio con C++" (o Visual Studio 2022 completo).
  - [CMake](https://cmake.org/download/) 3.24 o más nuevo.
  - [Git](https://git-scm.com/download/win).

MinHook, Dear ImGui y nlohmann/json se descargan solos al configurar; no hay que instalarlos.

## Instalación

Todavía no hay binarios publicados, así que hay que compilarlo.

1. Clona el repositorio:
   ```
   git clone https://github.com/johanncortes/gd-mod-menu.git
   cd gd-mod-menu
   ```
2. Compila en Release (la primera vez tarda porque baja las dependencias):
   ```
   cmake -B build -A x64
   cmake --build build --config Release
   ```
3. En `build/Release/` quedan `gdmenu.dll`, `injector.exe` y `sigcheck.exe`.

## Uso

1. Abre Geometry Dash y espera a que cargue el menú principal.
2. Ejecuta `build/Release/injector.exe`. Carga la `gdmenu.dll` que tiene al lado; también acepta otra
   ruta: `injector.exe C:\ruta\a\gdmenu.dll`.
3. Dentro del juego, pulsa **Tab** para abrir o cerrar el panel.

Hay que inyectar cada vez que se abre el juego.

- Los campos numéricos se aplican al pulsar **Enter** o al salir del campo.
- **End** descarga la DLL sin cerrar el juego (modo desarrollo, activado por defecto).
- Si una opción aparece como "(no disponible)", el motivo está en `build/Release/gdmenu.log`.

## Si GD se actualiza

Las funciones del juego se localizan con firmas de bytes, no con direcciones fijas. Si una firma deja de
coincidir tras una actualización, esa opción se desactiva sola y el juego sigue funcionando.

Para ver qué firmas fallan sin abrir el juego:

```
build\Release\sigcheck.exe "C:\Program Files (x86)\Steam\steamapps\common\Geometry Dash\GeometryDash.exe"
```

Las firmas están en [src/core/signatures.hpp](src/core/signatures.hpp) y documentadas en
[docs/firmas.md](docs/firmas.md).

## Opciones de compilación

| Opción de CMake | Por defecto | Qué hace |
| --- | --- | --- |
| `GDMENU_DEV` | `ON` | La tecla End descarga la DLL, para recompilar e inyectar sin cerrar el juego |
| `GDMENU_CONSOLE` | `OFF` | Abre una consola de debug al cargar la DLL (le quita el foco al juego) |

Ejemplo: `cmake -B build -A x64 -DGDMENU_DEV=OFF`.

## Estructura

```
src/
  dllmain.cpp   entrada de la DLL
  core/         hooks base, input, memoria, firmas
  gui/          panel de ImGui
  features/     una carpeta por categoría (global, player, ...)
  config/       guardar y cargar config.json
injector/       inyector de desarrollo
tools/          sigcheck
docs/firmas.md  registro de firmas y notas de ingeniería inversa
```

Para colaborar, las reglas del proyecto están en [AGENTS.md](AGENTS.md) y el estado de cada fase en
[PLAN.md](PLAN.md).

## Aviso

Es un proyecto personal sin relación con RobTop Games. Úsalo solo para practicar y en el editor; no para
conseguir completions.
