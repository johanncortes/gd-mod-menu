// GDModMenu.exe: el lanzador para usuarios. Lleva gdmenu.dll dentro (recurso), así que es un solo
// archivo: al abrirlo deja la DLL en %LOCALAPPDATA%\GDModMenu, abre Geometry Dash por Steam si no
// está abierto, espera a que cargue y la inyecta con LoadLibraryW. Sin consola: avisa con ventanas.
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <tlhelp32.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

const wchar_t* const kTitle = L"GD Mod Menu";
const wchar_t* const kProcessName = L"GeometryDash.exe";
const wchar_t* const kDllName = L"gdmenu.dll";
const wchar_t* const kSteamUrl = L"steam://rungameid/322170"; // Geometry Dash en Steam
constexpr DWORD kLaunchTimeoutMs = 120000; // espera a que el juego arranque
constexpr DWORD kSettleMs = 4000;          // margen tras aparecer la ventana (carga de recursos)
constexpr int kDllResource = 1;

int message(const std::wstring& text, UINT icon) {
    MessageBoxW(nullptr, text.c_str(), kTitle, MB_OK | icon | MB_SETFOREGROUND | MB_TOPMOST);
    return icon == MB_ICONERROR ? 1 : 0;
}

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

bool isModuleLoaded(DWORD pid, const wchar_t* module) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snap == INVALID_HANDLE_VALUE) return false;
    MODULEENTRY32W entry{sizeof(entry)};
    bool found = false;
    for (BOOL ok = Module32FirstW(snap, &entry); ok; ok = Module32NextW(snap, &entry)) {
        if (_wcsicmp(entry.szModule, module) == 0) {
            found = true;
            break;
        }
    }
    CloseHandle(snap);
    return found;
}

// true si el proceso ya tiene una ventana visible (el juego terminó de abrirse)
bool hasVisibleWindow(DWORD pid) {
    struct Search {
        DWORD pid;
        bool found;
    } search{pid, false};
    EnumWindows(
        [](HWND window, LPARAM param) -> BOOL {
            auto* s = reinterpret_cast<Search*>(param);
            DWORD owner = 0;
            GetWindowThreadProcessId(window, &owner);
            if (owner != s->pid || !IsWindowVisible(window)) return TRUE;
            s->found = true;
            return FALSE;
        },
        reinterpret_cast<LPARAM>(&search));
    return search.found;
}

// %LOCALAPPDATA%\GDModMenu (ahí quedan también config.json y gdmenu.log)
std::filesystem::path installFolder() {
    PWSTR local = nullptr;
    std::filesystem::path folder;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local))) folder = local;
    CoTaskMemFree(local);
    return folder.empty() ? folder : folder / L"GDModMenu";
}

// Saca la DLL del recurso al disco. false si no se pudo escribir.
bool extractDll(const std::filesystem::path& target) {
    HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(kDllResource), RT_RCDATA);
    HGLOBAL loaded = resource ? LoadResource(nullptr, resource) : nullptr;
    const void* data = loaded ? LockResource(loaded) : nullptr;
    const DWORD size = resource ? SizeofResource(nullptr, resource) : 0;
    if (!data || size == 0) return false;
    std::error_code error;
    std::filesystem::create_directories(target.parent_path(), error);
    std::ofstream file(target, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    file.write(static_cast<const char*>(data), size);
    return file.good();
}

bool inject(DWORD pid, const std::wstring& dllPath) {
    HANDLE process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, pid);
    if (!process) return false;
    const SIZE_T bytes = (dllPath.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    bool ok = remote && WriteProcessMemory(process, remote, dllPath.c_str(), bytes, nullptr);
    if (ok) {
        // kernel32 está en la misma dirección en todos los procesos x64 de la sesión
        auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
            GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
        HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
        ok = thread != nullptr;
        if (thread) {
            WaitForSingleObject(thread, 15000);
            CloseHandle(thread);
        }
    }
    if (remote) VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);
    return ok && isModuleLoaded(pid, kDllName);
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    DWORD pid = findProcess(kProcessName);
    if (pid && isModuleLoaded(pid, kDllName))
        return message(L"El mod menu ya está cargado.\n\nDentro del juego, pulsa Tab para abrirlo.", MB_ICONINFORMATION);

    // La DLL se deja en disco antes de abrir el juego
    const std::filesystem::path folder = installFolder();
    const std::filesystem::path dll = folder / kDllName;
    if (folder.empty() || !extractDll(dll))
        return message(L"No se pudo guardar el mod menu en:\n" + dll.wstring() +
                           L"\n\nComprueba que el antivirus no lo esté bloqueando.",
                       MB_ICONERROR);

    if (!pid) {
        // Geometry Dash no está abierto: se abre por Steam y se espera
        ShellExecuteW(nullptr, L"open", kSteamUrl, nullptr, nullptr, SW_SHOWNORMAL);
        const DWORD start = GetTickCount();
        while (!(pid = findProcess(kProcessName)) && GetTickCount() - start < kLaunchTimeoutMs) Sleep(500);
        if (!pid)
            return message(L"Geometry Dash no se abrió.\n\nÁbrelo desde Steam y vuelve a ejecutar este programa.",
                           MB_ICONERROR);
    }
    // Hasta que tenga ventana, más un margen para que termine de cargar
    const DWORD start = GetTickCount();
    while (!hasVisibleWindow(pid) && GetTickCount() - start < kLaunchTimeoutMs) {
        if (!findProcess(kProcessName)) return message(L"Geometry Dash se cerró antes de cargar.", MB_ICONERROR);
        Sleep(500);
    }
    Sleep(kSettleMs);

    if (!inject(pid, dll.wstring()))
        return message(L"No se pudo cargar el mod menu en el juego.\n\n"
                       L"Prueba a ejecutar este programa como administrador y revisa que el antivirus no lo bloquee.",
                       MB_ICONERROR);
    return message(L"¡Listo! El mod menu está cargado.\n\nDentro del juego, pulsa Tab para abrirlo.",
                   MB_ICONINFORMATION);
}
