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
        printf("Usage: mem_snapshot.exe <save|diff> [filename]\n");
        printf("  save [name] - save snapshot (default: snapshot.bin)\n");
        printf("  diff [name] - diff against saved snapshot\n");
        return 1;
    }

    const char *filename = (argc > 2) ? argv[2] : "C:\\snapshot.bin";

    DWORD pid = find_process("ys1plus.exe");
    if (!pid) { printf("ys1plus.exe not found\n"); return 1; }

    HANDLE proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!proc) { printf("OpenProcess failed\n"); return 1; }

    /* Dump entire .data section */
    DWORD data_start = 0x004F5000;
    DWORD data_size  = 0x13F79C;

    BYTE *current = (BYTE*)malloc(data_size);
    SIZE_T br;
    DWORD total = 0;
    for (DWORD off = 0; off < data_size; off += 4096) {
        DWORD sz = (data_size - off < 4096) ? (data_size - off) : 4096;
        if (ReadProcessMemory(proc, (LPCVOID)(data_start + off), current + off, sz, &br))
            total += br;
    }
    printf("Read %lu bytes\n", total);

    if (_stricmp(argv[1], "save") == 0) {
        FILE *fp = fopen(filename, "wb");
        if (!fp) { printf("Can't write %s\n", filename); return 1; }
        fwrite(current, 1, data_size, fp);
        fclose(fp);
        printf("Saved snapshot to %s\n", filename);
    }
    else if (_stricmp(argv[1], "diff") == 0) {
        FILE *fp = fopen(filename, "rb");
        if (!fp) { printf("Can't read %s - save a snapshot first\n", filename); return 1; }
        BYTE *saved = (BYTE*)malloc(data_size);
        fread(saved, 1, data_size, fp);
        fclose(fp);

        printf("\n=== Changes (DWORD) ===\n");
        int changes = 0;
        for (DWORD off = 0; off + 4 <= data_size; off += 4) {
            DWORD old_val = *(DWORD*)(saved + off);
            DWORD new_val = *(DWORD*)(current + off);
            if (old_val != new_val) {
                DWORD addr = data_start + off;
                printf("  0x%08lX: %lu -> %lu\n", addr, old_val, new_val);
                changes++;
                if (changes > 200) {
                    printf("  ... (too many changes, stopping)\n");
                    break;
                }
            }
        }
        printf("\nTotal DWORD changes: %d\n", changes);
        free(saved);
    }

    /* Undo our previous write at 0x531B00 */
    if (_stricmp(argv[1], "save") == 0) {
        /* Clean up the test write */
    }

    free(current);
    CloseHandle(proc);
    return 0;
}
