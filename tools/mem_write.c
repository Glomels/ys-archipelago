#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
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
    if (argc < 3) {
        printf("Usage: mem_write.exe <address_hex> <value_decimal>\n");
        printf("   or: mem_write.exe hp <value>\n");
        printf("   or: mem_write.exe maxhp <value>\n");
        printf("   or: mem_write.exe str <value>\n");
        printf("   or: mem_write.exe def <value>\n");
        printf("   or: mem_write.exe gold <value>\n");
        printf("   or: mem_write.exe level <value>\n");
        return 1;
    }

    DWORD pid = find_process("ys1plus.exe");
    if (!pid) { printf("ys1plus.exe not found\n"); return 1; }

    HANDLE proc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!proc) { printf("OpenProcess failed (error %lu)\n", GetLastError()); return 1; }

    DWORD BASE = 0x5317FC;
    DWORD addr;
    DWORD value = atoi(argv[2]);

    if (_stricmp(argv[1], "hp") == 0)         addr = BASE + 0x00;
    else if (_stricmp(argv[1], "maxhp") == 0)  addr = BASE + 0x04;
    else if (_stricmp(argv[1], "str") == 0)    addr = BASE + 0x08;
    else if (_stricmp(argv[1], "def") == 0)    addr = BASE + 0x0C;
    else if (_stricmp(argv[1], "gold") == 0)   addr = BASE + 0x10;
    else if (_stricmp(argv[1], "level") == 0)  addr = BASE + 0x18;
    else addr = strtoul(argv[1], NULL, 16);

    /* Read current value */
    DWORD old_val = 0;
    SIZE_T br;
    ReadProcessMemory(proc, (LPCVOID)addr, &old_val, 4, &br);
    printf("0x%08lX: %lu -> %lu\n", addr, old_val, value);

    /* Write new value */
    if (!WriteProcessMemory(proc, (LPVOID)addr, &value, 4, &br)) {
        printf("WriteProcessMemory failed (error %lu)\n", GetLastError());
        CloseHandle(proc);
        return 1;
    }

    /* Verify */
    DWORD new_val = 0;
    ReadProcessMemory(proc, (LPCVOID)addr, &new_val, 4, &br);
    printf("Verified: %lu\n", new_val);

    CloseHandle(proc);
    return 0;
}
