cmake_minimum_required(VERSION 3.24)
project(gdmenu LANGUAGES C CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Solo x64: configura con  cmake -B build -A x64")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# CRT estático: la DLL no depende del redistribuible de VC++
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Modo desarrollo: la tecla End descarga la DLL (para recompilar e inyectar sin cerrar el juego)
option(GDMENU_DEV "Descarga de la DLL con End" ON)
# Consola de debug. Apagada por defecto: al abrirse le quita el foco al juego y, si está
# a pantalla completa, lo minimiza. El log siempre se escribe en gdmenu.log junto a la DLL.
option(GDMENU_CONSOLE "Abrir una consola de debug al cargar la DLL" OFF)

include(FetchContent)

# MinHook e ImGui se bajan solo como fuentes (SOURCE_SUBDIR inexistente = no usar su CMake)
FetchContent_Declare(minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        v1.3.4
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  _sin_cmake)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9b
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  _sin_cmake)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(minhook imgui json)

# --- MinHook ---
add_library(minhook STATIC
    ${minhook_SOURCE_DIR}/src/buffer.c
    ${minhook_SOURCE_DIR}/src/hook.c
    ${minhook_SOURCE_DIR}/src/trampoline.c
    ${minhook_SOURCE_DIR}/src/hde/hde64.c)
target_include_directories(minhook PUBLIC ${minhook_SOURCE_DIR}/include)

# --- Dear ImGui (backends Win32 + OpenGL3) ---
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

# --- La DLL del menú ---
add_library(gdmenu SHARED
    src/dllmain.cpp
    src/core/log.cpp
    src/core/hooks.cpp
    src/core/input.cpp
    src/core/memory.cpp
    src/core/game.cpp
    src/config/config.cpp
    src/gui/menu.cpp
    src/gui/widgets.cpp
    src/features/features.cpp
    src/features/global/speedhack.cpp
    src/features/global/fps_bypass.cpp
    src/features/global/tps_bypass.cpp
    src/features/patch_feature.cpp
    src/features/bypass/instant_complete.cpp
    src/features/level/auto_practice.cpp
    src/features/level/hide_pause.cpp
    src/features/level/startpos_switcher.cpp
    src/features/player/noclip.cpp
    src/features/player/hitboxes.cpp
    src/features/player/respawn_delay.cpp)
target_include_directories(gdmenu PRIVATE src)
target_link_libraries(gdmenu PRIVATE minhook imgui nlohmann_json::nlohmann_json opengl32)
target_compile_definitions(gdmenu PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE
    $<$<BOOL:${GDMENU_DEV}>:GDMENU_DEV>
    $<$<BOOL:${GDMENU_CONSOLE}>:GDMENU_CONSOLE>)
target_compile_options(gdmenu PRIVATE /W4 /utf-8)

# --- Inyector para desarrollo ---
add_executable(injector injector/main.cpp)
target_compile_definitions(injector PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
target_compile_options(injector PRIVATE /W4 /utf-8)

# --- Comprobador de firmas: sigcheck.exe <GeometryDash.exe> (sin abrir el juego) ---
add_executable(sigcheck tools/sigcheck.cpp src/core/memory.cpp)
target_include_directories(sigcheck PRIVATE src)
target_compile_definitions(sigcheck PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(sigcheck PRIVATE /W4 /utf-8)

# Todo junto en build/<Config>/
set_target_properties(gdmenu injector sigcheck PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
