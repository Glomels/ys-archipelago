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

int main() {
    DWORD pid = find_process("ys1plus.exe");
    if (!pid) { printf("ys1plus.exe not found\n"); return 1; }

    HANDLE proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!proc) { printf("OpenProcess failed\n"); return 1; }

    SIZE_T br;

    /* Array 1: starts at 0x531990 */
    printf("=== Array 1 (0x531990) - Item Ownership ===\n");
    DWORD arr1[60];
    ReadProcessMemory(proc, (LPCVOID)0x531990, arr1, sizeof(arr1), &br);
    for (int i = 0; i < 60; i++) {
        if (arr1[i] != 0) {
            printf("  [%2d] 0x%08lX = %lu\n", i, 0x531990 + i*4, arr1[i]);
        }
    }

    /* Array 2: starts at 0x53207C */
    printf("\n=== Array 2 (0x53207C) - Second Flags ===\n");
    DWORD arr2[60];
    ReadProcessMemory(proc, (LPCVOID)0x53207C, arr2, sizeof(arr2), &br);
    for (int i = 0; i < 60; i++) {
        if (arr2[i] != 0) {
            printf("  [%2d] 0x%08lX = %lu\n", i, 0x53207C + i*4, arr2[i]);
        }
    }

    /* Dump the full region between the two arrays */
    printf("\n=== Full region 0x531990 - 0x532200 ===\n");
    for (DWORD addr = 0x531990; addr < 0x532200; addr += 4) {
        DWORD val = 0;
        ReadProcessMemory(proc, (LPCVOID)addr, &val, 4, &br);
        if (val != 0) {
            printf("  0x%08lX: %10lu (0x%08lX)\n", addr, val, val);
        }
    }

    CloseHandle(proc);
    return 0;
}
