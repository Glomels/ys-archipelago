/*
 * steam_api.dll proxy — forwards all exports to steam_api_orig.dll
 * and loads aphook.dll on startup.
 *
 * All 1053 exports are forwarded via the .def file (linker-level,
 * no runtime overhead). This file just handles DLL load/unload.
 *
 * Compile (32-bit!):
 *   i686-w64-mingw32-gcc -shared -O2 -o steam_api.dll steam_api_proxy.c \
 *       steam_api_proxy.def -lkernel32
 */

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved) {
    (void)hInst; (void)reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        LoadLibraryA("aphook.dll");
    }

    return TRUE;
}
