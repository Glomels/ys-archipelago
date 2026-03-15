/*
 * test_chest_swap.exe — Watches a chest flag and gives a replacement item.
 * Runs inside Wine, polls the game process memory directly.
 *
 * Usage: test_chest_swap.exe
 * Watches chest 0x531894 (Bestiary Potion), gives item 15 (Timer Ring).
 */

#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>

#define CHEST_FLAG   0x531894
#define GIVE_ID      15
#define ITEM_ARR1    0x531990
#define ITEM_ARR2    0x53207C
#define PERMIT_FILE  "C:\\ap_items.txt"

static DWORD find_process(const char *name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name) == 0) {
                DWORD pid = pe.th32ProcessID;
                CloseHandle(snap);
                return pid;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return 0;
}

static DWORD read_u32(HANDLE proc, DWORD addr) {
    DWORD val = 0;
    SIZE_T br;
    ReadProcessMemory(proc, (LPCVOID)(DWORD_PTR)addr, &val, 4, &br);
    return val;
}

static void write_u32(HANDLE proc, DWORD addr, DWORD val) {
    SIZE_T bw;
    WriteProcessMemory(proc, (LPVOID)(DWORD_PTR)addr, &val, 4, &bw);
}

int main(void) {
    printf("Looking for ys1plus.exe...\n");
    DWORD pid = find_process("ys1plus.exe");
    if (!pid) {
        printf("Game not found!\n");
        return 1;
    }
    printf("Found PID %lu\n", pid);

    HANDLE proc = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
        FALSE, pid
    );
    if (!proc) {
        printf("Can't open process\n");
        return 1;
    }

    /* Check initial state */
    DWORD flag = read_u32(proc, CHEST_FLAG);
    printf("Chest flag = %lu\n", flag);
    if (flag != 0) {
        printf("Chest already opened! Reload save first.\n");
        CloseHandle(proc);
        return 1;
    }

    printf("Watching chest 0x%X (Bestiary Potion)...\n", CHEST_FLAG);
    printf("Will give item %d (Timer Ring) when opened.\n", GIVE_ID);
    printf("GO OPEN THE CHEST!\n\n");
    fflush(stdout);

    /* Poll until chest opens */
    while (1) {
        flag = read_u32(proc, CHEST_FLAG);
        if (flag != 0) {
            printf(">>> CHEST OPENED! (flag=%lu)\n", flag);

            /* Permit the item */
            FILE *f = fopen(PERMIT_FILE, "a");
            if (f) {
                fprintf(f, "%d\n", GIVE_ID);
                fclose(f);
            }
            printf("Permitted item %d\n", GIVE_ID);

            /* Small delay to let DLL suppress vanilla item and re-read permit file */
            Sleep(50);

            /* Write replacement item */
            DWORD addr1 = ITEM_ARR1 + GIVE_ID * 4;
            DWORD addr2 = ITEM_ARR2 + GIVE_ID * 4;
            write_u32(proc, addr1, 1);
            write_u32(proc, addr2, 1);
            printf("Gave Timer Ring!\n");

            /* Verify */
            Sleep(100);
            DWORD v = read_u32(proc, addr1);
            printf("Item %d in memory: %lu\n", GIVE_ID, v);
            break;
        }
        Sleep(50);
    }

    CloseHandle(proc);
    return 0;
}
