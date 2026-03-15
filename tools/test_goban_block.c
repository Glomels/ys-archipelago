/*
 * test_goban_block.exe — Watches Goban's flags and blocks tower entry
 * if the player is missing a specified item.
 *
 * Simulates the client's guard_tower_entry behavior.
 * Pretends Hammer (game_id 46) is in the overworld and missing.
 */

#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>

#define GOBAN_FLAG1  0x531CE4
#define GOBAN_FLAG2  0x531CE8
#define GOBAN_FLAG3  0x531CEC

/* Hammer = game_id 46, Array1 addr = 0x531990 + 46*4 = 0x531A48 */
#define MISSING_ITEM_ADDR  0x531A48
#define MISSING_ITEM_NAME  "Hammer"

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
    if (!pid) { printf("Game not found!\n"); return 1; }

    HANDLE proc = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
        FALSE, pid
    );
    if (!proc) { printf("Can't open process\n"); return 1; }

    printf("Watching Goban flags. Missing item: %s\n", MISSING_ITEM_NAME);
    printf("Talk to Goban to test!\n\n");
    fflush(stdout);

    int blocked_count = 0;

    while (1) {
        DWORD g1 = read_u32(proc, GOBAN_FLAG1);
        DWORD g2 = read_u32(proc, GOBAN_FLAG2);
        DWORD g3 = read_u32(proc, GOBAN_FLAG3);

        if (g1 != 0 || g2 != 0 || g3 != 0) {
            /* Goban triggered — check if player has the item */
            DWORD has_item = read_u32(proc, MISSING_ITEM_ADDR);

            if (has_item == 0) {
                /* Missing item — block tower entry */
                write_u32(proc, GOBAN_FLAG1, 0);
                write_u32(proc, GOBAN_FLAG2, 0);
                write_u32(proc, GOBAN_FLAG3, 0);
                blocked_count++;
                printf("[BLOCKED #%d] Tower entry denied! Missing: %s (flags: %lu %lu %lu)\n",
                       blocked_count, MISSING_ITEM_NAME, g1, g2, g3);
                fflush(stdout);
            } else {
                printf("Player has %s — tower entry allowed.\n", MISSING_ITEM_NAME);
                fflush(stdout);
                break;
            }
        }

        Sleep(16);
    }

    CloseHandle(proc);
    return 0;
}
