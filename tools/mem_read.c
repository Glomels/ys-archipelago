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
    if (argc < 2) {
        printf("Usage: mem_read.exe <addr_hex|name> [count]\n");
        printf("Names: hp, maxhp, str, def, gold, exp, level\n");
        printf("count: number of DWORDs to read (default 1)\n");
        return 1;
    }

    DWORD pid = find_process("ys1plus.exe");
    if (!pid) { printf("ys1plus.exe not found\n"); return 1; }

    HANDLE proc = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    if (!proc) { printf("OpenProcess failed\n"); return 1; }

    DWORD BASE = 0x5317FC;
    DWORD addr;

    if (_stricmp(argv[1], "hp") == 0)         addr = BASE + 0x00;
    else if (_stricmp(argv[1], "maxhp") == 0)  addr = BASE + 0x04;
    else if (_stricmp(argv[1], "str") == 0)    addr = BASE + 0x08;
    else if (_stricmp(argv[1], "def") == 0)    addr = BASE + 0x0C;
    else if (_stricmp(argv[1], "gold") == 0)   addr = BASE + 0x10;
    else if (_stricmp(argv[1], "exp") == 0)    addr = BASE + 0x14;
    else if (_stricmp(argv[1], "level") == 0)  addr = BASE + 0x18;
    else addr = strtoul(argv[1], NULL, 16);

    int count = (argc >= 3) ? atoi(argv[2]) : 1;
    SIZE_T br;

    for (int i = 0; i < count; i++) {
        DWORD val;
        if (ReadProcessMemory(proc, (LPCVOID)(addr + i * 4), &val, 4, &br)) {
            printf("0x%08lX: %lu\n", addr + i * 4, val);
        } else {
            printf("0x%08lX: READ FAILED\n", addr + i * 4);
        }
    }

    CloseHandle(proc);
    return 0;
}
