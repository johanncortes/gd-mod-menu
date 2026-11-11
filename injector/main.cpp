// Inyector de desarrollo: carga gdmenu.dll en GeometryDash.exe con LoadLibraryW.
// Uso: injector.exe [ruta\a\gdmenu.dll]   (por defecto, la gdmenu.dll junto al .exe)
#include <windows.h>
#include <tlhelp32.h>

#include <cstdio>
#include <filesystem>
#include <string>

namespace {

const wchar_t* const kProcessName = L"GeometryDash.exe";

DWORD findProcess(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W entry{sizeof(entry)};
    DWORD pid = 0;
    for (BOOL ok = Process32FirstW(snap, &entry); ok; ok = Process32NextW(snap, &entry)) {
        if (_wcsicmp(entry.szExeFile, name) == 0) {
            pid = entry.th32ProcessID;
            break;
        }
    }
    CloseHandle(snap);
    return pid;
}

bool isModuleLoaded(DWORD pid, const std::wstring& dllName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snap == INVALID_HANDLE_VALUE) return false;
    MODULEENTRY32W entry{sizeof(entry)};
    bool found = false;
    for (BOOL ok = Module32FirstW(snap, &entry); ok; ok = Module32NextW(snap, &entry)) {
        if (_wcsicmp(entry.szModule, dllName.c_str()) == 0) {
            found = true;
            break;
        }
    }
    CloseHandle(snap);
    return found;
}

int fail(const char* what) {
    fprintf(stderr, "Error: %s (codigo %lu)\n", what, GetLastError());
    return 1;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    std::filesystem::path dll;
    if (argc > 1) {
        dll = std::filesystem::absolute(argv[1]);
    } else {
        wchar_t self[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, self, MAX_PATH);
        dll = std::filesystem::path(self).parent_path() / L"gdmenu.dll";
    }
    if (!std::filesystem::exists(dll)) {
        fprintf(stderr, "Error: no existe %ls\n", dll.c_str());
        return 1;
    }

    const DWORD pid = findProcess(kProcessName);
    if (!pid) {
        fprintf(stderr, "Error: %ls no esta abierto\n", kProcessName);
        return 1;
    }
    if (isModuleLoaded(pid, dll.filename().wstring())) {
        fprintf(stderr, "%ls ya esta cargada en el juego (pulsa End en el juego para descargarla)\n",
                dll.filename().c_str());
        return 1;
    }

    HANDLE process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
        FALSE, pid);
    if (!process) return fail("OpenProcess");

    // Escribir la ruta de la DLL en el juego y llamar LoadLibraryW desde un hilo remoto
    const std::wstring path = dll.wstring();
    const SIZE_T bytes = (path.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) return fail("VirtualAllocEx");
    if (!WriteProcessMemory(process, remote, path.c_str(), bytes, nullptr))
        return fail("WriteProcessMemory");

    // kernel32 está en la misma dirección en todos los procesos x64 de la sesión
    auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
    if (!thread) return fail("CreateRemoteThread");

    WaitForSingleObject(thread, 10000);
    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);

    // El código de salida del hilo no sirve en x64 (HMODULE truncado): comprobar el módulo
    if (!isModuleLoaded(pid, dll.filename().wstring())) {
        fprintf(stderr, "Error: LoadLibraryW fallo dentro del juego\n");
        return 1;
    }
    printf("Inyectada %ls en %ls (pid %lu)\n", dll.filename().c_str(), kProcessName, pid);
    return 0;
}
