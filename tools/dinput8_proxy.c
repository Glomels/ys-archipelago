/*
 * dinput8.dll proxy — loads the real DINPUT8.dll and forwards
 * DirectInput8Create, while also loading aphook.dll on startup.
 *
 * Install:
 *   1. Rename original DINPUT8.dll → DINPUT8_orig.dll
 *   2. Place this as DINPUT8.dll in the game folder
 *   3. Place aphook.dll + ap_connect.txt in the game folder
 *   4. Launch game normally — aphook loads automatically
 *
 * Compile (32-bit!):
 *   i686-w64-mingw32-gcc -shared -O2 -o dinput8.dll dinput8_proxy.c -lkernel32
 */

#include <windows.h>

/* The real DirectInput8Create from the original DLL */
typedef HRESULT (WINAPI *DI8Create_t)(
    HINSTANCE hinst, DWORD dwVersion, REFIID riidltf,
    LPVOID *ppvOut, LPVOID punkOuter
);

static HMODULE g_real_dinput8 = NULL;
static DI8Create_t g_real_DI8Create = NULL;

/* Exported: forwarded to real DLL */
__declspec(dllexport) HRESULT WINAPI DirectInput8Create(
    HINSTANCE hinst, DWORD dwVersion, REFIID riidltf,
    LPVOID *ppvOut, LPVOID punkOuter)
{
    if (!g_real_DI8Create)
        return E_FAIL;
    return g_real_DI8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved) {
    (void)hInst; (void)reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        /* Load the original DINPUT8.dll (renamed to DINPUT8_orig.dll) */
        g_real_dinput8 = LoadLibraryA("DINPUT8_orig.dll");
        if (!g_real_dinput8) {
            /* Fallback: try system directory */
            char sys_path[MAX_PATH];
            GetSystemDirectoryA(sys_path, MAX_PATH);
            lstrcatA(sys_path, "\\DINPUT8.dll");
            g_real_dinput8 = LoadLibraryA(sys_path);
        }

        if (g_real_dinput8) {
            g_real_DI8Create = (DI8Create_t)GetProcAddress(
                g_real_dinput8, "DirectInput8Create"
            );
        }

        /* Load aphook.dll — the full AP client */
        LoadLibraryA("aphook.dll");
    }

    if (reason == DLL_PROCESS_DETACH) {
        if (g_real_dinput8)
            FreeLibrary(g_real_dinput8);
    }

    return TRUE;
}
