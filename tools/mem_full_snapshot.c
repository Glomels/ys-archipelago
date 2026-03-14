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
        printf("Usage:\n");
        printf("  mem_full_snapshot.exe save <file>   Save all writable memory\n");
        printf("  mem_full_snapshot.exe diff <file>   Diff current vs saved snapshot\n");
        return 1;
    }

    DWORD pid = find_process("ys1plus.exe");
    if (!pid) { printf("ys1plus.exe not found\n"); return 1; }

    HANDLE proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!proc) { printf("OpenProcess failed\n"); return 1; }

    const char *file = (argc >= 3) ? argv[2] : "C:\\full_snap.bin";

    if (_stricmp(argv[1], "save") == 0) {
        FILE *f = fopen(file, "wb");
        if (!f) { printf("Can't open %s\n", file); return 1; }

        MEMORY_BASIC_INFORMATION mbi;
        DWORD addr = 0x10000;
        SIZE_T br;
        int regions = 0;
        DWORD total = 0;

        while (addr < 0x7FFFFFFF) {
            if (VirtualQueryEx(proc, (LPCVOID)addr, &mbi, sizeof(mbi)) == 0)
                break;

            if (mbi.State == MEM_COMMIT &&
                (mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE))) {

                DWORD base = (DWORD)mbi.BaseAddress;
                DWORD size = mbi.RegionSize;
                BYTE *buf = (BYTE*)malloc(size);

                if (buf && ReadProcessMemory(proc, (LPCVOID)base, buf, size, &br)) {
                    /* Write: base address, size, then data */
                    fwrite(&base, 4, 1, f);
                    fwrite(&size, 4, 1, f);
                    fwrite(buf, 1, size, f);
                    regions++;
                    total += size;
                }
                if (buf) free(buf);
            }

            addr = (DWORD)mbi.BaseAddress + mbi.RegionSize;
            if (addr <= (DWORD)mbi.BaseAddress) break;
        }

        fclose(f);
        printf("Saved %d regions (%lu bytes) to %s\n", regions, total, file);

    } else if (_stricmp(argv[1], "diff") == 0) {
        FILE *f = fopen(file, "rb");
        if (!f) { printf("Can't open %s\n", file); return 1; }

        SIZE_T br;
        int changes = 0;
        int regions = 0;

        while (!feof(f)) {
            DWORD base, size;
            if (fread(&base, 4, 1, f) != 1) break;
            if (fread(&size, 4, 1, f) != 1) break;

            BYTE *old_buf = (BYTE*)malloc(size);
            if (!old_buf || fread(old_buf, 1, size, f) != size) {
                if (old_buf) free(old_buf);
                break;
            }

            BYTE *new_buf = (BYTE*)malloc(size);
            if (new_buf && ReadProcessMemory(proc, (LPCVOID)base, new_buf, size, &br)) {
                for (DWORD off = 0; off + 4 <= size; off += 4) {
                    DWORD old_val = *(DWORD*)(old_buf + off);
                    DWORD new_val = *(DWORD*)(new_buf + off);
                    if (old_val != new_val) {
                        DWORD a = base + off;
                        /* Skip known noisy regions */
                        if (a >= 0x5DB9E0 && a <= 0x5DC3B0) continue; /* RNG */
                        if (a >= 0x51AFA0 && a <= 0x51B2F0) continue; /* rendering */
                        if (a >= 0x527D00 && a <= 0x529600) continue; /* tile data */
                        if (a >= 0x608000 && a <= 0x60C000) continue; /* more rendering */
                        if (a >= 0x633000 && a <= 0x635000) continue; /* misc counters */

                        /* Focus on 0->1 and 1->0 transitions */
                        if ((old_val == 0 && new_val == 1) || (old_val == 1 && new_val == 0)) {
                            printf("FLAG 0x%08lX: %lu -> %lu\n", a, old_val, new_val);
                        } else {
                            printf("     0x%08lX: %lu -> %lu\n", a, old_val, new_val);
                        }
                        changes++;
                    }
                }
            }
            if (new_buf) free(new_buf);
            free(old_buf);
            regions++;
        }

        fclose(f);
        printf("\n%d changes across %d regions\n", changes, regions);
    }

    CloseHandle(proc);
    return 0;
}
