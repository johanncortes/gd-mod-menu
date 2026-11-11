#include "log.hpp"

#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <mutex>

namespace core {

namespace {
FILE* g_file = nullptr;
FILE* g_console = nullptr;
std::mutex g_mutex;
} // namespace

void logInit(HMODULE self, bool withConsole) {
    // El log va junto a la DLL (igual que irá config.json)
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(self, path, MAX_PATH);
    auto logPath = std::filesystem::path(path).parent_path() / L"gdmenu.log";
    _wfopen_s(&g_file, logPath.c_str(), L"w");

    // Ojo: cerrar la ventana de la consola cierra también el juego
    if (withConsole && AllocConsole()) {
        SetConsoleTitleW(L"gdmenu debug");
        SetConsoleOutputCP(CP_UTF8);
        _wfopen_s(&g_console, L"CONOUT$", L"w");
    }
}

void logClose() {
    std::lock_guard lock(g_mutex);
    if (g_file) { fclose(g_file); g_file = nullptr; }
    if (g_console) {
        fclose(g_console);
        g_console = nullptr;
        FreeConsole();
    }
}

void logMsg(const char* fmt, ...) {
    char line[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    std::lock_guard lock(g_mutex);
    for (FILE* out : {g_file, g_console}) {
        if (!out) continue;
        fprintf(out, "[gdmenu] %s\n", line);
        fflush(out);
    }
}

} // namespace core
