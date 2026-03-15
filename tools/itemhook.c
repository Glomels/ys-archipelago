/*
 * itemhook.dll — Intercepts vanilla item grants in Ys I Chronicles.
 *
 * Once injected into ys1plus.exe, monitors the two item arrays and
 * suppresses any item that hasn't been explicitly permitted by the AP client.
 * The AP client writes permitted game_ids to C:\ap_items.txt.
 *
 * Item arrays:
 *   Array 1 (ownership):  0x531990, DWORD[52]
 *   Array 2 (visibility): 0x53207C, DWORD[52]
 *
 * Suppression order: zero Array 2 first, then Array 1 (safety rule).
 */

#include <windows.h>
#include <stdio.h>

#define ITEM_COUNT      52
#define PERMIT_FILE     "C:\\ap_items.txt"
#define LOG_FILE        "C:\\itemhook.log"
#define POLL_MS         16

/* In-process direct memory pointers */
static volatile DWORD *arr1 = (volatile DWORD *)0x531990;
static volatile DWORD *arr2 = (volatile DWORD *)0x53207C;

/* Permitted item set — only items in this set are allowed to exist */
static volatile BOOL permitted[ITEM_COUNT];

static FILE *g_log = NULL;

static void log_msg(const char *fmt, ...) {
    if (!g_log) return;
    va_list ap;
    va_start(ap, fmt);

    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(g_log, "[%02d:%02d:%02d.%03d] ",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    vfprintf(g_log, fmt, ap);
    fprintf(g_log, "\n");
    fflush(g_log);

    va_end(ap);
}

/* Reload permitted set from file */
static void reload_permitted(void) {
    FILE *f = fopen(PERMIT_FILE, "r");
    if (!f) return;

    char line[64];
    while (fgets(line, sizeof(line), f)) {
        int id = -1;
        if (sscanf(line, "%d", &id) == 1 && id >= 0 && id < ITEM_COUNT) {
            permitted[id] = TRUE;
        }
    }
    fclose(f);
}

/* Check if the permitted file exists — DLL is inactive until it does */
static BOOL permit_file_exists(void) {
    DWORD attr = GetFileAttributesA(PERMIT_FILE);
    return (attr != INVALID_FILE_ATTRIBUTES);
}

/* Main monitor thread */
static DWORD WINAPI item_monitor(LPVOID param) {
    (void)param;

    log_msg("itemhook monitor started");

    /* Wait for the permitted file to appear before doing anything.
     * The client creates it only when the player is in-game. */
    while (!permit_file_exists()) {
        Sleep(500);
    }
    log_msg("permit file found, suppression active");

    while (1) {
        BOOL found_unpermitted = FALSE;

        /* Scan item arrays for unauthorized items */
        for (int i = 0; i < ITEM_COUNT; i++) {
            if (arr1[i] != 0 && !permitted[i]) {
                found_unpermitted = TRUE;
                break;
            }
        }

        if (found_unpermitted) {
            /* Re-read the file — the client may have just updated it */
            reload_permitted();

            /* Now suppress anything still not permitted */
            for (int i = 0; i < ITEM_COUNT; i++) {
                if (arr1[i] != 0 && !permitted[i]) {
                    arr2[i] = 0;
                    arr1[i] = 0;
                    log_msg("suppressed item %d", i);
                }
            }
        }

        Sleep(POLL_MS);
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved) {
    (void)hInst; (void)reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        g_log = fopen(LOG_FILE, "a");
        log_msg("itemhook.dll loaded");

        /* Initialize: nothing permitted until file says otherwise */
        memset((void *)permitted, 0, sizeof(permitted));

        /* Do an initial file load */
        reload_permitted();

        /* Start monitor thread */
        CreateThread(NULL, 0, item_monitor, NULL, 0, NULL);
    }

    return TRUE;
}
