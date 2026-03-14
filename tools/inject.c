/*
 * inject.exe — Injects speedhack.dll into ys1plus.exe
 *
 * Usage: inject.exe [path_to_dll]
 * Default DLL path: C:\speedhack.dll
 */

#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>

DWORD find_process(const char *name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name) == 0) {
                CloseHandle(snap);
                return pe.th32ProcessID;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *dll_path = (argc >= 2) ? argv[1] : "C:\\speedhack.dll";

    printf("Speedhack Injector\n");
    printf("DLL: %s\n", dll_path);

    /* Verify DLL exists */
    DWORD attr = GetFileAttributes(dll_path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        printf("ERROR: DLL not found at %s\n", dll_path);
        return 1;
    }

    DWORD pid = find_process("ys1plus.exe");
    if (!pid) {
        printf("ERROR: ys1plus.exe not found\n");
        return 1;
    }
    printf("Found ys1plus.exe (PID %lu)\n", pid);

    HANDLE proc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
        FALSE, pid);
    if (!proc) {
        printf("ERROR: OpenProcess failed (%lu)\n", GetLastError());
        return 1;
    }

    /* Allocate memory in target for DLL path string */
    size_t path_len = strlen(dll_path) + 1;
    LPVOID remote_path = VirtualAllocEx(proc, NULL, path_len, MEM_COMMIT, PAGE_READWRITE);
    if (!remote_path) {
        printf("ERROR: VirtualAllocEx failed\n");
        CloseHandle(proc);
        return 1;
    }

    /* Write DLL path to target process */
    if (!WriteProcessMemory(proc, remote_path, dll_path, path_len, NULL)) {
        printf("ERROR: WriteProcessMemory failed\n");
        VirtualFreeEx(proc, remote_path, 0, MEM_RELEASE);
        CloseHandle(proc);
        return 1;
    }

    /* Get LoadLibraryA address */
    HMODULE k32 = GetModuleHandle("kernel32.dll");
    FARPROC load_lib = GetProcAddress(k32, "LoadLibraryA");
    if (!load_lib) {
        printf("ERROR: GetProcAddress(LoadLibraryA) failed\n");
        VirtualFreeEx(proc, remote_path, 0, MEM_RELEASE);
        CloseHandle(proc);
        return 1;
    }

    /* Create remote thread calling LoadLibraryA(dll_path) */
    HANDLE thread = CreateRemoteThread(proc, NULL, 0,
        (LPTHREAD_START_ROUTINE)load_lib, remote_path, 0, NULL);
    if (!thread) {
        printf("ERROR: CreateRemoteThread failed (%lu)\n", GetLastError());
        VirtualFreeEx(proc, remote_path, 0, MEM_RELEASE);
        CloseHandle(proc);
        return 1;
    }

    /* Wait for injection to complete */
    WaitForSingleObject(thread, 5000);

    DWORD exit_code = 0;
    GetExitCodeThread(thread, &exit_code);

    if (exit_code) {
        printf("SUCCESS: DLL injected (module handle 0x%08lX)\n", exit_code);
        printf("Speed multiplier is read from C:\\speedhack.txt (default 2.0x)\n");
        printf("Edit that file to change speed at runtime.\n");
    } else {
        printf("WARNING: LoadLibrary returned NULL — injection may have failed\n");
    }

    CloseHandle(thread);
    VirtualFreeEx(proc, remote_path, 0, MEM_RELEASE);
    CloseHandle(proc);
    return 0;
}
