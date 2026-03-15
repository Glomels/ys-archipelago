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

    DWORD data_start = 0x004F5000;
    DWORD data_size  = 0x13F79C;

    BYTE *prev = (BYTE*)malloc(data_size);
    BYTE *curr = (BYTE*)malloc(data_size);
    if (!prev || !curr) { printf("malloc failed\n"); return 1; }

    /* Initial read */
    SIZE_T br;
    for (DWORD off = 0; off < data_size; off += 4096) {
        DWORD sz = (data_size - off < 4096) ? (data_size - off) : 4096;
        ReadProcessMemory(proc, (LPCVOID)(data_start + off), prev + off, sz, &br);
    }

    /* Output file for logging changes */
    FILE *log = fopen("C:\\mem_changes.log", "w");
    if (!log) { printf("Can't open log\n"); return 1; }

    printf("Monitoring .data section every 2s. Changes logged to C:\\mem_changes.log\n");
    printf("Press Ctrl+C to stop.\n\n");
    fflush(stdout);

    int tick = 0;
    while (1) {
        Sleep(2000);
        tick++;

        /* Read current state */
        for (DWORD off = 0; off < data_size; off += 4096) {
            DWORD sz = (data_size - off < 4096) ? (data_size - off) : 4096;
            ReadProcessMemory(proc, (LPCVOID)(data_start + off), curr + off, sz, &br);
        }

        /* Diff */
        int changes = 0;
        for (DWORD off = 0; off + 4 <= data_size; off += 4) {
            DWORD old_val = *(DWORD*)(prev + off);
            DWORD new_val = *(DWORD*)(curr + off);
            if (old_val != new_val) {
                DWORD addr = data_start + off;
                /* Skip known noisy addresses (timers, frame counters, etc.) */
                /* Print to both stdout and log */
                printf("[%4d] 0x%08lX: %lu -> %lu\n", tick, addr, old_val, new_val);
                fprintf(log, "[%4d] 0x%08lX: %lu -> %lu\n", tick, addr, old_val, new_val);
                changes++;
            }
        }

        if (changes > 0) {
            printf("[%4d] --- %d changes ---\n", tick, changes);
            fprintf(log, "[%4d] --- %d changes ---\n", tick, changes);
            fflush(log);
            fflush(stdout);
        }

        /* Swap buffers */
        BYTE *tmp = prev;
        prev = curr;
        curr = tmp;
    }

    fclose(log);
    free(prev);
    free(curr);
    CloseHandle(proc);
    return 0;
}
