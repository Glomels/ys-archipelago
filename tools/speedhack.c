/*
 * speedhack.dll — Hooks timing functions to speed up/slow down the game.
 *
 * Once injected, reads speed multiplier from C:\speedhack.txt (default 1.0).
 * To change speed at runtime, just edit that file.
 *
 * Hooks: QueryPerformanceCounter, GetTickCount, timeGetTime
 * Method: IAT patching (overwrites import table entries in the target process)
 */

#include <windows.h>
#include <stdio.h>

static double g_speed = 2.0;
static const char *SPEED_FILE = "C:\\speedhack.txt";

/* Original function pointers */
static BOOL (WINAPI *orig_QPC)(LARGE_INTEGER *) = NULL;
static DWORD (WINAPI *orig_GetTickCount)(void) = NULL;
static DWORD (WINAPI *orig_timeGetTime)(void) = NULL;

/* Baseline values captured at hook install */
static LARGE_INTEGER qpc_base_real;
static LARGE_INTEGER qpc_base_fake;
static DWORD tick_base_real;
static DWORD tick_base_fake;
static DWORD tgt_base_real;
static DWORD tgt_base_fake;

static void reload_speed(void) {
    FILE *f = fopen(SPEED_FILE, "r");
    if (f) {
        double s;
        if (fscanf(f, "%lf", &s) == 1 && s > 0.0 && s <= 100.0) {
            g_speed = s;
        }
        fclose(f);
    }
}

/* Hooked QueryPerformanceCounter */
static BOOL WINAPI hook_QPC(LARGE_INTEGER *out) {
    LARGE_INTEGER real;
    if (!orig_QPC(&real)) return FALSE;

    LONGLONG real_delta = real.QuadPart - qpc_base_real.QuadPart;
    LONGLONG fake_delta = (LONGLONG)(real_delta * g_speed);
    out->QuadPart = qpc_base_fake.QuadPart + fake_delta;
    return TRUE;
}

/* Hooked GetTickCount */
static DWORD WINAPI hook_GetTickCount(void) {
    DWORD real = orig_GetTickCount();
    DWORD real_delta = real - tick_base_real;
    DWORD fake_delta = (DWORD)(real_delta * g_speed);
    return tick_base_fake + fake_delta;
}

/* Hooked timeGetTime */
static DWORD WINAPI hook_timeGetTime(void) {
    DWORD real = orig_timeGetTime();
    DWORD real_delta = real - tgt_base_real;
    DWORD fake_delta = (DWORD)(real_delta * g_speed);
    return tgt_base_fake + fake_delta;
}

/* Patch one IAT entry */
static int patch_iat(HMODULE module, const char *dll_name, void *orig_func, void *hook_func) {
    BYTE *base = (BYTE *)module;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR *imp = (IMAGE_IMPORT_DESCRIPTOR *)(base +
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    for (; imp->Name; imp++) {
        const char *name = (const char *)(base + imp->Name);
        if (_stricmp(name, dll_name) != 0) continue;

        IMAGE_THUNK_DATA *thunk = (IMAGE_THUNK_DATA *)(base + imp->FirstThunk);
        for (; thunk->u1.Function; thunk++) {
            if (thunk->u1.Function == (DWORD_PTR)orig_func) {
                DWORD old;
                VirtualProtect(&thunk->u1.Function, sizeof(DWORD_PTR), PAGE_READWRITE, &old);
                thunk->u1.Function = (DWORD_PTR)hook_func;
                VirtualProtect(&thunk->u1.Function, sizeof(DWORD_PTR), old, &old);
                return 1;
            }
        }
    }
    return 0;
}

/* Background thread: periodically reload speed from file */
static DWORD WINAPI speed_monitor(LPVOID param) {
    (void)param;
    while (1) {
        reload_speed();
        Sleep(1000);
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved) {
    (void)hInst; (void)reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        reload_speed();

        HMODULE exe = GetModuleHandle(NULL);
        HMODULE kernel32 = GetModuleHandle("kernel32.dll");
        HMODULE winmm = GetModuleHandle("winmm.dll");
        if (!winmm) winmm = LoadLibrary("winmm.dll");

        /* Save originals */
        orig_QPC = (void *)GetProcAddress(kernel32, "QueryPerformanceCounter");
        orig_GetTickCount = (void *)GetProcAddress(kernel32, "GetTickCount");
        if (winmm)
            orig_timeGetTime = (void *)GetProcAddress(winmm, "timeGetTime");

        /* Capture baselines */
        if (orig_QPC) {
            orig_QPC(&qpc_base_real);
            qpc_base_fake = qpc_base_real;
        }
        if (orig_GetTickCount) {
            tick_base_real = orig_GetTickCount();
            tick_base_fake = tick_base_real;
        }
        if (orig_timeGetTime) {
            tgt_base_real = orig_timeGetTime();
            tgt_base_fake = tgt_base_real;
        }

        /* Patch IAT */
        if (orig_QPC)
            patch_iat(exe, "kernel32.dll", orig_QPC, hook_QPC);
        /* Also try KERNEL32.dll (Wine sometimes uses different case) */
        if (orig_QPC)
            patch_iat(exe, "KERNEL32.dll", orig_QPC, hook_QPC);
        if (orig_GetTickCount) {
            patch_iat(exe, "kernel32.dll", orig_GetTickCount, hook_GetTickCount);
            patch_iat(exe, "KERNEL32.dll", orig_GetTickCount, hook_GetTickCount);
        }
        if (orig_timeGetTime && winmm) {
            patch_iat(exe, "winmm.dll", orig_timeGetTime, hook_timeGetTime);
            patch_iat(exe, "WINMM.dll", orig_timeGetTime, hook_timeGetTime);
        }

        /* Start monitor thread to reload speed from file */
        CreateThread(NULL, 0, speed_monitor, NULL, 0, NULL);
    }

    return TRUE;
}
