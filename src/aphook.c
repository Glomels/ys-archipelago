/*
 * aphook.dll — Complete in-process AP client for Ys I Chronicles.
 *
 * Single DLL handles everything:
 *   - Hooks give_item to intercept vanilla item grants
 *   - Monitors location flags for location checks
 *   - Connects to AP server via WebSocket (WinSock2)
 *   - Sends/receives AP protocol messages
 *   - Guards Darm Tower entry
 *   - Checks goal completion
 *
 * Config file: ap_connect.txt (same directory as the DLL)
 *   server=localhost:38281
 *   slot=Adol
 *   password=
 *
 * Compile (32-bit!):
 *   i686-w64-mingw32-gcc -shared -O2 -o aphook.dll aphook.c -lws2_32 -lkernel32
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define ITEM_COUNT       52
#define MAX_LOCATIONS    128
#define MAX_TOWER_ITEMS  16
#define MAX_CHECKED      256
#define MAX_ITEM_QUEUE   128
#define POLL_MS          16
#define WS_BUF_SIZE      262144  /* 256 KB */
#define MSG_BUF_SIZE     65536
#define GIVE_ITEM_ADDR   0x44F850
#define IN_GAME_ADDR     0x53152C
#define GOBAN_FLAG_1     0x531CE4
#define GOBAN_FLAG_2     0x531CE8
#define GOBAN_FLAG_3     0x531CEC
#define GOLD_ADDR        0x53180C

/* Text rendering (reverse-engineered from FPS display code) */
#define DRAW_TEXT_FUNC    0x451370
#define TEXT_STRUCT_ADDR  0x52C900
#define TEXT_OUTPUT_ADDR  0x52CA10
#define RENDER_HOOK_SITE  0x453D1D  /* call 0x452fd0 right after FPS draw */
#define RENDER_ORIG_FUNC  0x452FD0

static volatile DWORD *arr1 = (volatile DWORD *)0x531990;
static volatile DWORD *arr2 = (volatile DWORD *)0x53207C;

/* AP constants */
#define YS1_BASE_ID          0x59530000
#define YS1_LOCATION_BASE_ID 0x59530000

/* ========================================================================= */
/* Location Flag Table                                                       */
/* ========================================================================= */

typedef struct {
    DWORD flag_addr;
    DWORD ap_loc_code;
    int   reverse;      /* 1 = flag goes 1→0 */
    DWORD prev_value;
    BOOL  checked;
} LocEntry;

/* Hardcoded location table — same data as apworld/locations.py */
static LocEntry g_locations[] = {
    {0x531B20, YS1_LOCATION_BASE_ID + 2,   0, 0, FALSE}, /* Slaff's Gift */
    {0x531BB4, YS1_LOCATION_BASE_ID + 1,   0, 0, FALSE}, /* Sara's Gift */
    {0x531BC0, YS1_LOCATION_BASE_ID + 31,  0, 0, FALSE}, /* Franz's Gift */
    {0x531C6C, YS1_LOCATION_BASE_ID + 3,   0, 0, FALSE}, /* Jeba's Gift */
    {0x531CAC, YS1_LOCATION_BASE_ID + 4,   0, 0, FALSE}, /* Silver Bell Reward */
    {0x531B2C, YS1_LOCATION_BASE_ID + 5,   0, 0, FALSE}, /* Slaff's Second Gift */
    {0x531C28, YS1_LOCATION_BASE_ID + 7,   0, 0, FALSE}, /* Golden Vase */
    {0x531894, YS1_LOCATION_BASE_ID + 8,   0, 0, FALSE}, /* Bestiary Potion Chest */
    {0x531898, YS1_LOCATION_BASE_ID + 9,   0, 0, FALSE}, /* Locked Chest */
    {0x531C30, YS1_LOCATION_BASE_ID + 6,   1, 0, FALSE}, /* Southern Roda Tree (REVERSE) */
    {0x5318A0, YS1_LOCATION_BASE_ID + 11,  0, 0, FALSE}, /* Shrine Shield Ring */
    {0x5318A4, YS1_LOCATION_BASE_ID + 10,  0, 0, FALSE}, /* Shrine Ruby */
    {0x5318A8, YS1_LOCATION_BASE_ID + 12,  0, 0, FALSE}, /* Shrine Necklace */
    {0x5318B0, YS1_LOCATION_BASE_ID + 22,  0, 0, FALSE}, /* Shrine B2 Silver Bell */
    {0x5318AC, YS1_LOCATION_BASE_ID + 23,  0, 0, FALSE}, /* Shrine B2 Mask of Eyes */
    {0x5318B4, YS1_LOCATION_BASE_ID + 20,  0, 0, FALSE}, /* Shrine B2 Prison Key */
    {0x5318B8, YS1_LOCATION_BASE_ID + 21,  0, 0, FALSE}, /* Shrine B2 Treasure Box Key */
    {0x5318BC, YS1_LOCATION_BASE_ID + 26,  0, 0, FALSE}, /* Shrine B3 Marble Key */
    {0x5318C0, YS1_LOCATION_BASE_ID + 25,  0, 0, FALSE}, /* Shrine B3 Ivory Key */
    {0x5318C4, YS1_LOCATION_BASE_ID + 28,  0, 0, FALSE}, /* Shrine B3 Silver Shield */
    {0x5318C8, YS1_LOCATION_BASE_ID + 27,  0, 0, FALSE}, /* Shrine B3 Heal Potion */
    {0x5318CC, YS1_LOCATION_BASE_ID + 29,  0, 0, FALSE}, /* Shrine B3 Volume Hadal */
    {0x531950, YS1_LOCATION_BASE_ID + 100, 0, 0, FALSE}, /* Boss: Jenocres */
    {0x5318D0, YS1_LOCATION_BASE_ID + 37,  0, 0, FALSE}, /* Mine F1 Silver Armor */
    {0x5318D4, YS1_LOCATION_BASE_ID + 35,  0, 0, FALSE}, /* Mine F1 Heal Potion */
    {0x5318D8, YS1_LOCATION_BASE_ID + 36,  0, 0, FALSE}, /* Mine F1 Timer Ring */
    {0x5318DC, YS1_LOCATION_BASE_ID + 40,  0, 0, FALSE}, /* Mine B1 Heal Ring */
    {0x5318E0, YS1_LOCATION_BASE_ID + 42,  0, 0, FALSE}, /* Mine B1 Roda Tree Seed */
    {0x5318E4, YS1_LOCATION_BASE_ID + 41,  0, 0, FALSE}, /* Mine B1 Silver Harmonica */
    {0x5318E8, YS1_LOCATION_BASE_ID + 46,  0, 0, FALSE}, /* Mine B2 Heal Potion */
    {0x5318EC, YS1_LOCATION_BASE_ID + 45,  0, 0, FALSE}, /* Mine B2 Darm Key */
    {0x5318F0, YS1_LOCATION_BASE_ID + 47,  0, 0, FALSE}, /* Mine B2 Volume Dabbie */
    {0x531958, YS1_LOCATION_BASE_ID + 101, 0, 0, FALSE}, /* Boss: Nygtilger */
    {0x531AC0, YS1_LOCATION_BASE_ID + 102, 0, 0, FALSE}, /* Boss: Vagullion */
    {0x5318FC, YS1_LOCATION_BASE_ID + 50,  0, 0, FALSE}, /* Tower F2 Heal Potion */
    {0x531900, YS1_LOCATION_BASE_ID + 51,  0, 0, FALSE}, /* Tower F2 Mirror */
    {0x531904, YS1_LOCATION_BASE_ID + 53,  0, 0, FALSE}, /* Tower F2 Evil Ring */
    {0x531944, YS1_LOCATION_BASE_ID + 52,  0, 0, FALSE}, /* Tower F2 Talwar */
    {0x531948, YS1_LOCATION_BASE_ID + 54,  0, 0, FALSE}, /* Tower F4 Reflex */
    {0x53194C, YS1_LOCATION_BASE_ID + 55,  0, 0, FALSE}, /* Tower F6 Large Shield */
    {0x531914, YS1_LOCATION_BASE_ID + 56,  0, 0, FALSE}, /* Tower F7 Silver Sword */
    {0x531968, YS1_LOCATION_BASE_ID + 103, 0, 0, FALSE}, /* Boss: Pictimos */
    {0x531918, YS1_LOCATION_BASE_ID + 62,  0, 0, FALSE}, /* Tower F9 Hammer */
    {0x53191C, YS1_LOCATION_BASE_ID + 63,  0, 0, FALSE}, /* Tower F9 Volume Mesa */
    {0x531920, YS1_LOCATION_BASE_ID + 65,  0, 0, FALSE}, /* Tower F9 Silver Shield */
    {0x531924, YS1_LOCATION_BASE_ID + 68,  0, 0, FALSE}, /* Tower F13 Silver Armor */
    {0x531D38, YS1_LOCATION_BASE_ID + 66,  0, 0, FALSE}, /* Dogi's Gift */
    {0x531D00, YS1_LOCATION_BASE_ID + 67,  0, 0, FALSE}, /* Raba Trade */
    {0x531D64, YS1_LOCATION_BASE_ID + 69,  0, 0, FALSE}, /* Reah's Gift */
    {0x531D58, YS1_LOCATION_BASE_ID + 80,  0, 0, FALSE}, /* Luta Gemma's Gift */
    {0x531928, YS1_LOCATION_BASE_ID + 72,  0, 0, FALSE}, /* Tower F14 Rod */
    {0x53192C, YS1_LOCATION_BASE_ID + 73,  0, 0, FALSE}, /* Tower F14 Volume Gemma */
    {0x531978, YS1_LOCATION_BASE_ID + 104, 0, 0, FALSE}, /* Boss: Khonsclard */
    {0x531930, YS1_LOCATION_BASE_ID + 75,  0, 0, FALSE}, /* Tower F15 Battle Shield */
    {0x531934, YS1_LOCATION_BASE_ID + 78,  0, 0, FALSE}, /* Tower F17 Heal Potion */
    {0x531938, YS1_LOCATION_BASE_ID + 76,  0, 0, FALSE}, /* Tower F19 Battle Armor */
    {0x53193C, YS1_LOCATION_BASE_ID + 77,  0, 0, FALSE}, /* Tower F20 Flame Sword */
    {0x531ACC, YS1_LOCATION_BASE_ID + 105, 0, 0, FALSE}, /* Boss: Yogleks & Omulgun */
    {0x531A98, YS1_LOCATION_BASE_ID + 106, 0, 0, FALSE}, /* Boss: Dark Fact */
};
#define NUM_LOCATIONS (sizeof(g_locations) / sizeof(g_locations[0]))

/* AP item code → game_id mapping */
static const struct { unsigned int ap_code; int game_id; } g_item_map[] = {
    {YS1_BASE_ID + 1,   0},  /* Short Sword */
    {YS1_BASE_ID + 2,   1},  /* Long Sword */
    {YS1_BASE_ID + 3,   2},  /* Talwar */
    {YS1_BASE_ID + 4,   3},  /* Silver Sword */
    {YS1_BASE_ID + 5,   4},  /* Flame Sword */
    {YS1_BASE_ID + 20,  5},  /* Small Shield */
    {YS1_BASE_ID + 21,  6},  /* Middle Shield */
    {YS1_BASE_ID + 22,  7},  /* Large Shield */
    {YS1_BASE_ID + 23,  8},  /* Battle Shield */
    {YS1_BASE_ID + 24,  9},  /* Silver Shield */
    {YS1_BASE_ID + 10, 10},  /* Chain Mail */
    {YS1_BASE_ID + 11, 11},  /* Plate Mail */
    {YS1_BASE_ID + 12, 12},  /* Reflex */
    {YS1_BASE_ID + 13, 13},  /* Silver Armor */
    {YS1_BASE_ID + 14, 14},  /* Battle Armor */
    {YS1_BASE_ID + 30, 15},  /* Power Ring */
    {YS1_BASE_ID + 31, 16},  /* Shield Ring */
    {YS1_BASE_ID + 32, 17},  /* Timer Ring */
    {YS1_BASE_ID + 33, 18},  /* Heal Ring */
    {YS1_BASE_ID + 34, 19},  /* Evil Ring */
    {YS1_BASE_ID + 40, 20},  /* Book of Ys (Hadal) */
    {YS1_BASE_ID + 41, 21},  /* Book of Ys (Tovah) */
    {YS1_BASE_ID + 42, 22},  /* Book of Ys (Dabbie) */
    {YS1_BASE_ID + 43, 23},  /* Book of Ys (Mesa) */
    {YS1_BASE_ID + 44, 24},  /* Book of Ys (Gemma) */
    {YS1_BASE_ID + 45, 25},  /* Book of Ys (Fact) */
    {YS1_BASE_ID + 50, 26},  /* Treasure Box Key */
    {YS1_BASE_ID + 51, 27},  /* Prison Key */
    {YS1_BASE_ID + 52, 28},  /* Shrine Key */
    {YS1_BASE_ID + 53, 29},  /* Ivory Key */
    {YS1_BASE_ID + 54, 30},  /* Marble Key */
    {YS1_BASE_ID + 55, 31},  /* Darm Key */
    {YS1_BASE_ID + 60, 32},  /* Sara's Crystal */
    {YS1_BASE_ID + 61, 33},  /* Roda Tree Seed */
    {YS1_BASE_ID + 62, 34},  /* Silver Bell */
    {YS1_BASE_ID + 63, 35},  /* Silver Harmonica */
    {YS1_BASE_ID + 64, 36},  /* Idol */
    {YS1_BASE_ID + 65, 37},  /* Rod */
    {YS1_BASE_ID + 66, 38},  /* Monocle */
    {YS1_BASE_ID + 67, 39},  /* Blue Amulet */
    {YS1_BASE_ID + 83, 40},  /* Ruby */
    {YS1_BASE_ID + 85, 42},  /* Necklace */
    {YS1_BASE_ID + 84, 43},  /* Golden Vase */
    {YS1_BASE_ID + 80, 44},  /* Heal Potion */
    {YS1_BASE_ID + 81, 45},  /* Mirror */
    {YS1_BASE_ID + 70, 46},  /* Hammer */
    {YS1_BASE_ID + 82, 47},  /* Wing */
    {YS1_BASE_ID + 68, 48},  /* Mask of Eyes */
    {YS1_BASE_ID + 69, 49},  /* Blue Necklace */
    {YS1_BASE_ID + 71, 41},  /* Sapphire Ring */
    {YS1_BASE_ID + 86, 50},  /* Bestiary Potion */
    {YS1_BASE_ID + 87, 51},  /* Piece of Paper */
};
#define NUM_ITEMS (sizeof(g_item_map) / sizeof(g_item_map[0]))

static int ap_code_to_game_id(unsigned int ap_code) {
    for (int i = 0; i < (int)NUM_ITEMS; i++)
        if (g_item_map[i].ap_code == ap_code) return g_item_map[i].game_id;
    return -1;
}

/* Shop location table: game_id → AP location code */
#define SHOP_CALLER_LO  0x493400
#define SHOP_CALLER_HI  0x494100

static const struct { int game_id; DWORD ap_loc_code; } g_shop_locs[] = {
    { 41, YS1_LOCATION_BASE_ID + 200},  /* Pim - Sapphire Ring */
    { 47, YS1_LOCATION_BASE_ID + 201},  /* Pim - Mirror */
    { 45, YS1_LOCATION_BASE_ID + 202},  /* Pim - Wing */
    {  0, YS1_LOCATION_BASE_ID + 210},  /* Shop - Short Sword */
    {  1, YS1_LOCATION_BASE_ID + 211},  /* Shop - Long Sword */
    {  2, YS1_LOCATION_BASE_ID + 212},  /* Shop - Talwar */
    {  5, YS1_LOCATION_BASE_ID + 213},  /* Shop - Small Shield */
    {  6, YS1_LOCATION_BASE_ID + 214},  /* Shop - Middle Shield */
    {  7, YS1_LOCATION_BASE_ID + 215},  /* Shop - Large Shield */
    { 10, YS1_LOCATION_BASE_ID + 216},  /* Shop - Chain Mail */
    { 11, YS1_LOCATION_BASE_ID + 217},  /* Shop - Plate Mail */
    { 12, YS1_LOCATION_BASE_ID + 218},  /* Shop - Reflex */
    { 44, YS1_LOCATION_BASE_ID + 219},  /* Shop - Heal Potion */
};
#define NUM_SHOP_LOCS (sizeof(g_shop_locs) / sizeof(g_shop_locs[0]))

/* Track which shop locations have been checked */
static volatile BOOL shop_checked[NUM_SHOP_LOCS];

static int shop_loc_index(int game_id) {
    for (int i = 0; i < (int)NUM_SHOP_LOCS; i++)
        if (g_shop_locs[i].game_id == game_id) return i;
    return -1;
}

/* ========================================================================= */
/* Shared State (protected by critical sections)                             */
/* ========================================================================= */

/* Location checks: monitor → network */
static DWORD pending_locs[MAX_CHECKED];
static int num_pending_locs = 0;
static CRITICAL_SECTION loc_cs;

/* Item receives: network → monitor */
typedef struct { int index; int game_id; } ItemQueueEntry;
static ItemQueueEntry item_queue[MAX_ITEM_QUEUE];
static int item_queue_count = 0;
static CRITICAL_SECTION item_cs;

/* AP state (written by network thread) */
static volatile int ap_slot = 0;
static volatile int ap_goal = 0;
static volatile BOOL ap_connected = FALSE;
static volatile BOOL ap_goal_complete = FALSE;
static int ap_received_index = 0;

/* Tower items from slot_data */
static int tower_items[MAX_TOWER_ITEMS];
static int num_tower_items = 0;

/* Hook state */
static volatile LONG ap_giving = 0;
static volatile LONG hook_active = 0;
static BYTE trampoline[16];
static volatile BOOL permitted[ITEM_COUNT];

/* On-screen status text (updated by network thread)
 * Color via inline codes: #XY sets color to hex byte XY
 * (parsed by game's text renderer at 0x451370) */
static char ap_status_text[64] = "#0EAP: Starting...";

/* Config */
static char cfg_host[256] = "localhost";
static int  cfg_port = 38281;
static char cfg_slot[64] = "Adol";
static char cfg_password[64] = "";

/* Paths resolved at load time (relative to DLL location) */
static char g_log_path[MAX_PATH];
static char g_config_path[MAX_PATH];
static char g_base_dir[MAX_PATH];

static FILE *g_log = NULL;

/* ========================================================================= */
/* Logging                                                                   */
/* ========================================================================= */

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

/* ========================================================================= */
/* Item Hook (give_item at 0x44F850)                                         */
/* ========================================================================= */

typedef void (__attribute__((fastcall)) *give_item_fn)(int game_id, int value);
static give_item_fn call_original;

void __attribute__((fastcall)) hook_give_item(int game_id, int value) {
    if (InterlockedCompareExchange(&ap_giving, 0, 0) ||
        !InterlockedCompareExchange(&hook_active, 0, 0)) {
        call_original(game_id, value);
        return;
    }
    if (game_id < 0 || game_id >= ITEM_COUNT) {
        call_original(game_id, value);
        return;
    }

    unsigned int caller = (unsigned int)(uintptr_t)__builtin_return_address(0);

    /* Shop purchase detection */
    if (caller >= SHOP_CALLER_LO && caller < SHOP_CALLER_HI) {
        int idx = shop_loc_index(game_id);
        if (idx >= 0 && !shop_checked[idx]) {
            /* First purchase → AP location check */
            shop_checked[idx] = TRUE;
            log_msg("SHOP CHECK game_id=%d loc=0x%08X caller=0x%08X",
                    game_id, g_shop_locs[idx].ap_loc_code, caller);
            EnterCriticalSection(&loc_cs);
            if (num_pending_locs < MAX_CHECKED)
                pending_locs[num_pending_locs++] = g_shop_locs[idx].ap_loc_code;
            LeaveCriticalSection(&loc_cs);
            return; /* intercept: don't give vanilla item */
        }
        /* Already checked or unknown shop item → allow through */
        call_original(game_id, value);
        permitted[game_id] = TRUE;
        log_msg("SHOP REPEAT game_id=%d caller=0x%08X", game_id, caller);
        return;
    }

    if (permitted[game_id]) {
        call_original(game_id, value);
        return;
    }
    log_msg("INTERCEPT game_id=%d caller=0x%08X", game_id, caller);
}

static BOOL install_hook(void) {
    DWORD old_protect;
    BYTE orig[8];
    memcpy(orig, (void *)GIVE_ITEM_ADDR, 8);

    if (!VirtualProtect(trampoline, sizeof(trampoline),
                        PAGE_EXECUTE_READWRITE, &old_protect))
        return FALSE;
    memcpy(trampoline, orig, 8);
    trampoline[8] = 0xE9;
    *(DWORD *)(trampoline + 9) =
        (DWORD)(GIVE_ITEM_ADDR + 8) - (DWORD)(uintptr_t)(trampoline + 13);
    call_original = (give_item_fn)(uintptr_t)trampoline;

    if (!VirtualProtect((void *)GIVE_ITEM_ADDR, 8,
                        PAGE_EXECUTE_READWRITE, &old_protect))
        return FALSE;
    *(BYTE *)GIVE_ITEM_ADDR = 0xE9;
    *(DWORD *)(GIVE_ITEM_ADDR + 1) =
        (DWORD)(uintptr_t)hook_give_item - (GIVE_ITEM_ADDR + 5);
    memset((void *)(GIVE_ITEM_ADDR + 5), 0x90, 3);
    FlushInstructionCache(GetCurrentProcess(), (void *)GIVE_ITEM_ADDR, 8);

    log_msg("hook installed at 0x%08X", GIVE_ITEM_ADDR);
    return TRUE;
}

static void ap_give(int game_id) {
    if (game_id < 0 || game_id >= ITEM_COUNT) return;
    InterlockedExchange(&ap_giving, 1);
    call_original(game_id, 1);
    InterlockedExchange(&ap_giving, 0);
    permitted[game_id] = TRUE;
    log_msg("AP_GIVE game_id=%d", game_id);
}

/* ========================================================================= */
/* On-Screen Status Display (hooked into FPS render path)                    */
/* ========================================================================= */

/*
 * The game draws FPS text at 0x453CD8 using:
 *   ecx = 0x52C900 (text renderer object)
 *   push output_buf, x, y, text, flags → call 0x451370
 * Right after that, it calls 0x452FD0 (at address 0x453D1D).
 * We detour that call to also draw our AP status text.
 *
 * draw_text is __thiscall: ecx = this, callee cleans stack.
 * Signature: void draw_text(void *this, void *output, int x, int y,
 *                           const char *text, int flags)
 */

typedef void (__attribute__((thiscall)) *draw_text_fn)(
    void *self, void *output, int x, int y, const char *text, int flags);

static draw_text_fn game_draw_text = (draw_text_fn)DRAW_TEXT_FUNC;

static void draw_ap_status(void) {
    void *text_struct = (void *)TEXT_STRUCT_ADDR;
    void *text_output = (void *)TEXT_OUTPUT_ADDR;

    /* Save text struct fields we modify */
    DWORD save_opacity  = *(volatile DWORD *)(TEXT_STRUCT_ADDR + 0x54);
    DWORD save_align    = *(volatile DWORD *)(TEXT_STRUCT_ADDR + 0x28);
    DWORD save_maxchars = *(volatile DWORD *)(TEXT_STRUCT_ADDR + 0x24);

    /* Set up text struct fields (same pattern as FPS code) */
    *(volatile DWORD *)(TEXT_STRUCT_ADDR + 0x54) = 0x10;  /* opacity */
    *(volatile DWORD *)(TEXT_STRUCT_ADDR + 0x28) = 2;          /* alignment */
    *(volatile DWORD *)(TEXT_STRUCT_ADDR + 0x24) = 0x7FFFFFFF; /* max chars */

    /* Draw below FPS counter: FPS is at y=30, we use y=44 */
    game_draw_text(text_struct, text_output, 100, 44, ap_status_text, 0);

    /* Restore text struct fields */
    *(volatile DWORD *)(TEXT_STRUCT_ADDR + 0x54) = save_opacity;
    *(volatile DWORD *)(TEXT_STRUCT_ADDR + 0x28) = save_align;
    *(volatile DWORD *)(TEXT_STRUCT_ADDR + 0x24) = save_maxchars;
}

/* Naked-ish trampoline: call original function, then draw status */
static void __cdecl render_hook_impl(void) {
    ((void (__cdecl *)(void))RENDER_ORIG_FUNC)();
    draw_ap_status();
}

static BOOL install_render_hook(void) {
    DWORD old_protect;
    BYTE *site = (BYTE *)RENDER_HOOK_SITE;

    /* Verify it's a call instruction (E8) */
    if (*site != 0xE8) {
        log_msg("render hook: expected E8 at 0x%08X, got 0x%02X", RENDER_HOOK_SITE, *site);
        return FALSE;
    }

    if (!VirtualProtect(site, 5, PAGE_EXECUTE_READWRITE, &old_protect))
        return FALSE;

    /* Patch relative call target: E8 [new_offset] */
    DWORD new_target = (DWORD)(uintptr_t)render_hook_impl;
    DWORD new_offset = new_target - (RENDER_HOOK_SITE + 5);
    *(DWORD *)(site + 1) = new_offset;

    FlushInstructionCache(GetCurrentProcess(), site, 5);
    VirtualProtect(site, 5, old_protect, &old_protect);

    log_msg("render hook installed at 0x%08X", RENDER_HOOK_SITE);
    return TRUE;
}

/* ========================================================================= */
/* Minimal JSON Parser                                                       */
/* ========================================================================= */

static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

/* Skip a complete JSON value, return pointer past it */
static const char *skip_value(const char *p) {
    p = skip_ws(p);
    if (*p == '"') {
        p++;
        while (*p && *p != '"') { if (*p == '\\') p++; p++; }
        if (*p == '"') p++;
        return p;
    }
    if (*p == '{') {
        int depth = 1; p++;
        while (*p && depth > 0) {
            if (*p == '{') depth++;
            else if (*p == '}') depth--;
            else if (*p == '"') { p++; while (*p && *p != '"') { if (*p == '\\') p++; p++; } }
            p++;
        }
        return p;
    }
    if (*p == '[') {
        int depth = 1; p++;
        while (*p && depth > 0) {
            if (*p == '[') depth++;
            else if (*p == ']') depth--;
            else if (*p == '"') { p++; while (*p && *p != '"') { if (*p == '\\') p++; p++; } }
            p++;
        }
        return p;
    }
    /* number, true, false, null */
    while (*p && *p != ',' && *p != '}' && *p != ']' && *p != ' ' && *p != '\n') p++;
    return p;
}

/* Find key in object, return pointer to value. obj should point at '{' */
static const char *json_find(const char *obj, const char *key) {
    if (!obj) return NULL;
    obj = skip_ws(obj);
    if (*obj != '{') return NULL;
    obj++;
    int keylen = (int)strlen(key);
    while (*obj && *obj != '}') {
        obj = skip_ws(obj);
        if (*obj != '"') break;
        obj++;
        if (strncmp(obj, key, keylen) == 0 && obj[keylen] == '"') {
            obj += keylen + 1;
            obj = skip_ws(obj);
            if (*obj == ':') { obj++; return skip_ws(obj); }
        }
        /* skip this key */
        while (*obj && *obj != '"') { if (*obj == '\\') obj++; obj++; }
        if (*obj == '"') obj++;
        obj = skip_ws(obj);
        if (*obj == ':') obj++;
        obj = skip_value(obj);
        obj = skip_ws(obj);
        if (*obj == ',') obj++;
    }
    return NULL;
}

/* Read a string value, copy into buf. Returns length. */
static int json_read_str(const char *p, char *buf, int maxlen) {
    if (!p || *p != '"') { buf[0] = 0; return 0; }
    p++;
    int i = 0;
    while (*p && *p != '"' && i < maxlen - 1) {
        if (*p == '\\') { p++; }
        buf[i++] = *p++;
    }
    buf[i] = 0;
    return i;
}

/* Read an integer value */
static long json_read_int(const char *p) {
    if (!p) return 0;
    p = skip_ws(p);
    return strtol(p, NULL, 10);
}

/* Read an unsigned integer value */
static unsigned long json_read_uint(const char *p) {
    if (!p) return 0;
    p = skip_ws(p);
    return strtoul(p, NULL, 10);
}

/* Parse an array of integers into out[], return count */
static int json_read_int_array(const char *p, unsigned int *out, int max) {
    if (!p || *p != '[') return 0;
    p++;
    int count = 0;
    while (*p && *p != ']' && count < max) {
        p = skip_ws(p);
        if (*p == ']') break;
        out[count++] = strtoul(p, NULL, 10);
        while (*p && *p != ',' && *p != ']') p++;
        if (*p == ',') p++;
    }
    return count;
}

/* Iterate array: returns pointer to first element. p should point at '[' */
static const char *json_array_first(const char *p) {
    if (!p || *p != '[') return NULL;
    p = skip_ws(p + 1);
    if (*p == ']') return NULL;
    return p;
}

static const char *json_array_next(const char *p) {
    p = skip_value(p);
    p = skip_ws(p);
    if (*p == ',') { p++; return skip_ws(p); }
    return NULL;
}

/* ========================================================================= */
/* WebSocket Client (WinSock2)                                               */
/* ========================================================================= */

static SOCKET ws_sock = INVALID_SOCKET;
static char ws_recv_buf[WS_BUF_SIZE];
static int ws_recv_len = 0;

/* Base64 encode (minimal, for WebSocket key) */
static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void base64_encode(const BYTE *in, int len, char *out) {
    int i, j = 0;
    for (i = 0; i < len - 2; i += 3) {
        out[j++] = b64[(in[i] >> 2) & 0x3F];
        out[j++] = b64[((in[i] & 0x3) << 4) | (in[i+1] >> 4)];
        out[j++] = b64[((in[i+1] & 0xF) << 2) | (in[i+2] >> 6)];
        out[j++] = b64[in[i+2] & 0x3F];
    }
    if (i < len) {
        out[j++] = b64[(in[i] >> 2) & 0x3F];
        if (i + 1 < len) {
            out[j++] = b64[((in[i] & 0x3) << 4) | (in[i+1] >> 4)];
            out[j++] = b64[((in[i+1] & 0xF) << 2)];
        } else {
            out[j++] = b64[((in[i] & 0x3) << 4)];
            out[j++] = '=';
        }
        out[j++] = '=';
    }
    out[j] = 0;
}

static BOOL ws_connect(const char *host, int port) {
    struct addrinfo hints, *res;
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        log_msg("getaddrinfo failed for %s:%d", host, port);
        return FALSE;
    }

    ws_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (ws_sock == INVALID_SOCKET) {
        freeaddrinfo(res);
        return FALSE;
    }

    if (connect(ws_sock, res->ai_addr, (int)res->ai_addrlen) != 0) {
        log_msg("connect failed to %s:%d", host, port);
        closesocket(ws_sock);
        ws_sock = INVALID_SOCKET;
        freeaddrinfo(res);
        return FALSE;
    }
    freeaddrinfo(res);

    /* WebSocket handshake */
    BYTE key_bytes[16];
    srand((unsigned)GetTickCount());
    for (int i = 0; i < 16; i++) key_bytes[i] = (BYTE)(rand() & 0xFF);
    char ws_key[32];
    base64_encode(key_bytes, 16, ws_key);

    char req[512];
    int req_len = snprintf(req, sizeof(req),
        "GET / HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n", host, port, ws_key);

    send(ws_sock, req, req_len, 0);

    char resp[1024];
    int resp_len = recv(ws_sock, resp, sizeof(resp) - 1, 0);
    if (resp_len <= 0) {
        closesocket(ws_sock);
        ws_sock = INVALID_SOCKET;
        return FALSE;
    }
    resp[resp_len] = 0;

    if (!strstr(resp, "101")) {
        log_msg("WebSocket handshake failed: %s", resp);
        closesocket(ws_sock);
        ws_sock = INVALID_SOCKET;
        return FALSE;
    }

    ws_recv_len = 0;
    log_msg("WebSocket connected to %s:%d", host, port);
    return TRUE;
}

static BOOL ws_send_text(const char *text, int len) {
    if (ws_sock == INVALID_SOCKET) return FALSE;

    BYTE header[14];
    int hlen = 0;
    DWORD mask_key = GetTickCount();

    header[0] = 0x81; /* FIN + text opcode */

    if (len < 126) {
        header[1] = 0x80 | (BYTE)len; /* MASK bit + length */
        hlen = 2;
    } else if (len < 65536) {
        header[1] = 0x80 | 126;
        header[2] = (BYTE)(len >> 8);
        header[3] = (BYTE)(len & 0xFF);
        hlen = 4;
    } else {
        header[1] = 0x80 | 127;
        memset(header + 2, 0, 4);
        header[6] = (BYTE)((len >> 24) & 0xFF);
        header[7] = (BYTE)((len >> 16) & 0xFF);
        header[8] = (BYTE)((len >> 8) & 0xFF);
        header[9] = (BYTE)(len & 0xFF);
        hlen = 10;
    }

    /* Append mask key */
    memcpy(header + hlen, &mask_key, 4);
    hlen += 4;

    /* Send header */
    if (send(ws_sock, (char *)header, hlen, 0) != hlen) return FALSE;

    /* Mask and send payload */
    BYTE *masked = (BYTE *)malloc(len);
    BYTE *mk = (BYTE *)&mask_key;
    for (int i = 0; i < len; i++)
        masked[i] = ((BYTE *)text)[i] ^ mk[i & 3];

    int sent = send(ws_sock, (char *)masked, len, 0);
    free(masked);
    return sent == len;
}

/* Ensure we have at least 'need' bytes in ws_recv_buf.
 * Returns: 1 = OK, 0 = timeout, -1 = error/disconnect */
static int ws_fill(int need) {
    while (ws_recv_len < need) {
        int n = recv(ws_sock, ws_recv_buf + ws_recv_len,
                     WS_BUF_SIZE - ws_recv_len, 0);
        if (n > 0) {
            ws_recv_len += n;
            continue;
        }
        if (n == 0) return -1; /* connection closed */
        int err = WSAGetLastError();
        if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK) return 0;
        return -1;
    }
    return 1;
}

/* Receive one WebSocket text frame.
 * Returns payload length, 0 on timeout, -1 on error.
 * Payload is written to 'out' (null-terminated). */
static int ws_recv_text(char *out, int maxlen) {
    for (;;) {
        int rc = ws_fill(2);
        if (rc <= 0) return rc;

        BYTE b0 = (BYTE)ws_recv_buf[0];
        BYTE b1 = (BYTE)ws_recv_buf[1];
        int opcode = b0 & 0x0F;
        int masked = (b1 & 0x80) != 0;
        unsigned long long payload_len = b1 & 0x7F;
        int hdr_len = 2;

        if (payload_len == 126) {
            if (ws_fill(4) != 1) return -1;
            payload_len = ((BYTE)ws_recv_buf[2] << 8) | (BYTE)ws_recv_buf[3];
            hdr_len = 4;
        } else if (payload_len == 127) {
            if (ws_fill(10) != 1) return -1;
            payload_len = 0;
            for (int i = 0; i < 8; i++)
                payload_len = (payload_len << 8) | (BYTE)ws_recv_buf[2 + i];
            hdr_len = 10;
        }

        if (masked) hdr_len += 4;
        int total = hdr_len + (int)payload_len;
        if (ws_fill(total) != 1) return -1;

        char *payload = ws_recv_buf + hdr_len;

        /* Unmask if needed */
        if (masked) {
            BYTE *mk = (BYTE *)(ws_recv_buf + hdr_len - 4);
            for (int i = 0; i < (int)payload_len; i++)
                payload[i] ^= mk[i & 3];
        }

        /* Handle frame BEFORE consuming from buffer (memmove destroys payload) */
        int result = 0;
        int done = 0;

        if (opcode == 0x9) {
            /* Ping → respond with pong echoing payload */
            BYTE pong_hdr[2] = {0x8A, 0x80 | (BYTE)payload_len};
            DWORD mk = 0;
            send(ws_sock, (char *)pong_hdr, 2, 0);
            send(ws_sock, (char *)&mk, 4, 0);
            if (payload_len > 0)
                send(ws_sock, payload, (int)payload_len, 0);
        } else if (opcode == 0x8) {
            log_msg("WebSocket close frame received");
            result = -1;
            done = 1;
        } else if (opcode == 0x1 || opcode == 0x2) {
            int copy = ((int)payload_len < maxlen - 1) ? (int)payload_len : maxlen - 1;
            memcpy(out, payload, copy);
            out[copy] = 0;
            result = copy;
            done = 1;
        }

        /* NOW consume this frame from buffer */
        int remaining = ws_recv_len - total;
        if (remaining > 0)
            memmove(ws_recv_buf, ws_recv_buf + total, remaining);
        ws_recv_len = remaining;

        if (done) {
            if (result > 0)
                log_msg("WS_FRAME opcode=%d hdr=%d payload=%d remaining=%d",
                        opcode, hdr_len, (int)payload_len, ws_recv_len);
            return result;
        }
        /* Ping handled, loop for next frame */
    }
}

static void ws_close(void) {
    if (ws_sock != INVALID_SOCKET) {
        closesocket(ws_sock);
        ws_sock = INVALID_SOCKET;
    }
}

/* ========================================================================= */
/* AP Protocol                                                               */
/* ========================================================================= */

static char msg_buf[MSG_BUF_SIZE];

static BOOL ap_send_connect(void) {
    int len = snprintf(msg_buf, MSG_BUF_SIZE,
        "[{\"cmd\":\"Connect\","
        "\"game\":\"Ys I Chronicles\","
        "\"name\":\"%s\","
        "\"password\":\"%s\","
        "\"uuid\":\"%s-aphook\","
        "\"version\":{\"major\":0,\"minor\":6,\"build\":6,\"class\":\"Version\"},"
        "\"items_handling\":7,"
        "\"tags\":[\"AP\"],"
        "\"slot_data\":true}]",
        cfg_slot, cfg_password, cfg_slot);
    return ws_send_text(msg_buf, len);
}

static BOOL ap_send_location_checks(DWORD *codes, int count) {
    if (count == 0) return TRUE;
    char *p = msg_buf;
    int rem = MSG_BUF_SIZE;
    int n = snprintf(p, rem, "[{\"cmd\":\"LocationChecks\",\"locations\":[");
    p += n; rem -= n;
    for (int i = 0; i < count; i++) {
        n = snprintf(p, rem, "%s%u", i > 0 ? "," : "", codes[i]);
        p += n; rem -= n;
    }
    n = snprintf(p, rem, "]}]");
    p += n;
    return ws_send_text(msg_buf, (int)(p - msg_buf));
}

static BOOL ap_send_goal_complete(void) {
    int len = snprintf(msg_buf, MSG_BUF_SIZE,
        "[{\"cmd\":\"StatusUpdate\",\"status\":30}]");
    return ws_send_text(msg_buf, len);
}

static void ap_handle_connected(const char *obj) {
    const char *v;

    v = json_find(obj, "slot");
    if (v) ap_slot = (int)json_read_int(v);

    /* Parse slot_data */
    const char *sd = json_find(obj, "slot_data");
    if (sd) {
        v = json_find(sd, "goal");
        if (v) ap_goal = (int)json_read_int(v);

        /* Tower items */
        const char *ti = json_find(sd, "tower_items_in_overworld");
        if (ti) {
            unsigned int arr[MAX_TOWER_ITEMS];
            num_tower_items = json_read_int_array(ti, arr, MAX_TOWER_ITEMS);
            for (int i = 0; i < num_tower_items; i++)
                tower_items[i] = (int)arr[i];
        }
    }

    /* Mark already-checked locations */
    const char *cl = json_find(obj, "checked_locations");
    if (cl) {
        unsigned int arr[MAX_CHECKED];
        int count = json_read_int_array(cl, arr, MAX_CHECKED);
        for (int c = 0; c < count; c++) {
            for (int i = 0; i < (int)NUM_LOCATIONS; i++) {
                if (g_locations[i].ap_loc_code == arr[c])
                    g_locations[i].checked = TRUE;
            }
            /* Also mark shop locations */
            for (int i = 0; i < (int)NUM_SHOP_LOCS; i++) {
                if (g_shop_locs[i].ap_loc_code == arr[c])
                    shop_checked[i] = TRUE;
            }
        }
        log_msg("  %d locations already checked", count);
    }

    ap_connected = TRUE;
    snprintf(ap_status_text, sizeof(ap_status_text), "#0AAP: Connected (%s)", cfg_slot);
    log_msg("Connected! slot=%d goal=%d tower_items=%d",
            ap_slot, ap_goal, num_tower_items);
}

static void ap_handle_received_items(const char *obj) {
    const char *v = json_find(obj, "index");
    int start_index = v ? (int)json_read_int(v) : 0;

    const char *items = json_find(obj, "items");
    if (!items) return;

    int idx = start_index;
    const char *elem = json_array_first(items);
    while (elem) {
        if (idx >= ap_received_index) {
            const char *item_code_v = json_find(elem, "item");
            if (item_code_v) {
                unsigned int ap_code = (unsigned int)json_read_uint(item_code_v);
                int game_id = ap_code_to_game_id(ap_code);

                EnterCriticalSection(&item_cs);
                if (item_queue_count < MAX_ITEM_QUEUE && game_id >= 0) {
                    item_queue[item_queue_count].index = idx;
                    item_queue[item_queue_count].game_id = game_id;
                    item_queue_count++;
                }
                LeaveCriticalSection(&item_cs);
            }
            ap_received_index = idx + 1;
        }
        idx++;
        elem = json_array_next(elem);
    }
}

static void ap_handle_room_update(const char *obj) {
    const char *cl = json_find(obj, "checked_locations");
    if (cl) {
        unsigned int arr[MAX_CHECKED];
        int count = json_read_int_array(cl, arr, MAX_CHECKED);
        for (int c = 0; c < count; c++) {
            for (int i = 0; i < (int)NUM_LOCATIONS; i++) {
                if (g_locations[i].ap_loc_code == arr[c])
                    g_locations[i].checked = TRUE;
            }
        }
    }
}

static void ap_handle_print(const char *obj) {
    const char *data = json_find(obj, "data");
    if (!data) return;
    /* Just log the first text element */
    const char *elem = json_array_first(data);
    if (elem) {
        const char *tv = json_find(elem, "text");
        if (tv) {
            char text[256];
            json_read_str(tv, text, sizeof(text));
            if (text[0]) log_msg("[AP] %s", text);
        }
    }
}

/* Process one AP packet (JSON object) */
static void ap_handle_packet(const char *obj) {
    const char *cmd_v = json_find(obj, "cmd");
    if (!cmd_v) return;

    char cmd[32];
    json_read_str(cmd_v, cmd, sizeof(cmd));
    log_msg("PACKET cmd='%s'", cmd);

    if (strcmp(cmd, "RoomInfo") == 0) {
        log_msg("RoomInfo received, sending Connect...");
        ap_send_connect();
    }
    else if (strcmp(cmd, "Connected") == 0) {
        ap_handle_connected(obj);
    }
    else if (strcmp(cmd, "ReceivedItems") == 0) {
        ap_handle_received_items(obj);
    }
    else if (strcmp(cmd, "RoomUpdate") == 0) {
        ap_handle_room_update(obj);
    }
    else if (strcmp(cmd, "PrintJSON") == 0) {
        ap_handle_print(obj);
    }
    else if (strcmp(cmd, "ConnectionRefused") == 0) {
        snprintf(ap_status_text, sizeof(ap_status_text), "#0CAP: Refused");
        log_msg("Connection refused!");
    }
}

/* Process a received WebSocket message (JSON array of packets) */
static void ap_process_message(const char *msg) {
    const char *p = skip_ws(msg);
    if (*p != '[') {
        log_msg("MSG not array: %.80s", msg);
        return;
    }

    /* Log first 200 chars of raw message for debugging */
    log_msg("MSG (len=%d): %.200s", (int)strlen(msg), msg);

    const char *elem = json_array_first(p);
    int pkt_idx = 0;
    while (elem) {
        if (*elem == '{')
            ap_handle_packet(elem);
        pkt_idx++;
        elem = json_array_next(elem);
    }
    log_msg("MSG parsed %d packets", pkt_idx);
}

/* ========================================================================= */
/* Config Loading                                                            */
/* ========================================================================= */

static BOOL load_config(void) {
    FILE *f = fopen(g_config_path, "r");
    if (!f) return FALSE;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *nl = strchr(line, '\n'); if (nl) *nl = 0;
        nl = strchr(line, '\r'); if (nl) *nl = 0;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char *key = line, *val = eq + 1;

        if (strcmp(key, "server") == 0) {
            /* Parse host:port */
            char *colon = strchr(val, ':');
            if (colon) {
                *colon = 0;
                strncpy(cfg_host, val, sizeof(cfg_host) - 1);
                cfg_port = atoi(colon + 1);
            } else {
                strncpy(cfg_host, val, sizeof(cfg_host) - 1);
            }
        }
        else if (strcmp(key, "slot") == 0)
            strncpy(cfg_slot, val, sizeof(cfg_slot) - 1);
        else if (strcmp(key, "password") == 0)
            strncpy(cfg_password, val, sizeof(cfg_password) - 1);
    }
    fclose(f);
    log_msg("config: server=%s:%d slot=%s", cfg_host, cfg_port, cfg_slot);
    return TRUE;
}

/* ========================================================================= */
/* Monitor Thread — game interaction (hooks, flags, items, tower)            */
/* ========================================================================= */

static void init_location_flags(void) {
    int nonzero = 0;
    int sent = 0;
    for (int i = 0; i < (int)NUM_LOCATIONS; i++) {
        volatile DWORD *ptr = (volatile DWORD *)(uintptr_t)g_locations[i].flag_addr;
        DWORD val = *ptr;
        g_locations[i].prev_value = val;
        if (val != 0) {
            nonzero++;
            /* If flag is set but not yet checked on server, send it now */
            if (!g_locations[i].checked) {
                g_locations[i].checked = TRUE;
                EnterCriticalSection(&loc_cs);
                if (num_pending_locs < MAX_CHECKED)
                    pending_locs[num_pending_locs++] = g_locations[i].ap_loc_code;
                LeaveCriticalSection(&loc_cs);
                sent++;
            }
        }
    }
    log_msg("init_location_flags: %d/%d already non-zero, %d new checks queued",
            nonzero, (int)NUM_LOCATIONS, sent);
}

static void poll_location_flags(void) {
    for (int i = 0; i < (int)NUM_LOCATIONS; i++) {
        if (g_locations[i].checked) continue;

        volatile DWORD *ptr = (volatile DWORD *)(uintptr_t)g_locations[i].flag_addr;
        DWORD value = *ptr;
        BOOL fired = FALSE;

        if (g_locations[i].reverse) {
            if (value == 0 && g_locations[i].prev_value != 0) fired = TRUE;
        } else {
            if (value != 0 && g_locations[i].prev_value == 0) fired = TRUE;
        }
        g_locations[i].prev_value = value;

        if (fired) {
            g_locations[i].checked = TRUE;
            log_msg("LOCATION 0x%08X -> ap_code=%u",
                    g_locations[i].flag_addr, g_locations[i].ap_loc_code);

            EnterCriticalSection(&loc_cs);
            if (num_pending_locs < MAX_CHECKED)
                pending_locs[num_pending_locs++] = g_locations[i].ap_loc_code;
            LeaveCriticalSection(&loc_cs);
        }
    }
}

static void process_item_queue(void) {
    EnterCriticalSection(&item_cs);
    for (int i = 0; i < item_queue_count; i++) {
        ap_give(item_queue[i].game_id);
    }
    item_queue_count = 0;
    LeaveCriticalSection(&item_cs);
}

static void suppress_unauthorized(void) {
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (arr1[i] != 0 && !permitted[i]) {
            arr2[i] = 0;
            arr1[i] = 0;
            log_msg("SUPPRESS game_id=%d", i);
        }
    }
}

static void guard_tower_entry(void) {
    if (num_tower_items == 0) return;
    volatile DWORD *g1 = (volatile DWORD *)GOBAN_FLAG_1;
    volatile DWORD *g2 = (volatile DWORD *)GOBAN_FLAG_2;
    volatile DWORD *g3 = (volatile DWORD *)GOBAN_FLAG_3;
    if (*g1 == 0 && *g2 == 0 && *g3 == 0) return;

    for (int i = 0; i < num_tower_items; i++) {
        int gid = tower_items[i];
        if (gid >= 0 && gid < ITEM_COUNT && arr1[gid] == 0) {
            *g1 = 0; *g2 = 0; *g3 = 0;
            log_msg("TOWER BLOCKED");
            return;
        }
    }
}

static BOOL check_goal(void) {
    /* TODO: Dark Fact flag 0x531A98 is non-zero at game start.
     * Need to verify correct address. Disabled until fixed. */
    return FALSE;
#if 0
    if (ap_goal == 0) {
        /* Dark Fact defeated */
        volatile DWORD *df = (volatile DWORD *)0x531A98;
        return *df != 0;
    } else if (ap_goal == 1) {
        /* All Books + Dark Fact */
        volatile DWORD *df = (volatile DWORD *)0x531A98;
        if (*df == 0) return FALSE;
        for (int i = 20; i <= 25; i++)
            if (arr1[i] == 0) return FALSE;
        return TRUE;
    } else if (ap_goal == 2) {
        /* All bosses */
        DWORD boss_flags[] = {0x531950, 0x531958, 0x531AC0, 0x531968,
                              0x531978, 0x531ACC, 0x531A98};
        for (int i = 0; i < 7; i++)
            if (*(volatile DWORD *)(uintptr_t)boss_flags[i] == 0) return FALSE;
        return TRUE;
    }
    return FALSE;
#endif
}

static DWORD WINAPI monitor_thread(LPVOID param) {
    (void)param;
    volatile DWORD *in_game = (volatile DWORD *)IN_GAME_ADDR;

    /* Wait for AP connection */
    while (!ap_connected) Sleep(100);

    /* Wait for player to be in-game */
    log_msg("waiting for in-game...");
    while (*in_game != 1) Sleep(100);

    /* Install hook first (sets up call_original trampoline) */
    if (!install_hook()) {
        log_msg("FATAL: hook install failed");
        return 1;
    }

    /* AP server is source of truth — give received items before activating suppression */
    process_item_queue();

    /* Snapshot flags and send any the server hasn't seen */
    init_location_flags();

    InterlockedExchange(&hook_active, 1);
    log_msg("monitor active — hook, flags, tower guard");

    /* Debug give file path */
    char give_path[MAX_PATH];
    snprintf(give_path, MAX_PATH, "%sap_give.txt", g_base_dir);

    DWORD prev_in_game = 1; /* we entered the loop already in-game */

    while (1) {
        DWORD cur_in_game = *in_game;

        /* Detect save load (in_game transitions 0→1) */
        if (cur_in_game == 1 && prev_in_game == 0) {
            log_msg("save loaded — re-syncing AP state");
            /* Re-give all permitted items (AP is source of truth) */
            for (int i = 0; i < ITEM_COUNT; i++) {
                if (permitted[i])
                    ap_give(i);
            }
            /* Re-snapshot location flags */
            init_location_flags();
        }
        prev_in_game = cur_in_game;

        if (cur_in_game == 1) {
            process_item_queue();
            poll_location_flags();
            suppress_unauthorized();
            guard_tower_entry();

            /* Check for debug give file */
            FILE *gf = fopen(give_path, "r");
            if (gf) {
                char line[32];
                while (fgets(line, sizeof(line), gf)) {
                    int gid = atoi(line);
                    if (gid >= 0 && gid < ITEM_COUNT)
                        ap_give(gid);
                }
                fclose(gf);
                DeleteFileA(give_path);
            }

            if (!ap_goal_complete && check_goal()) {
                ap_goal_complete = TRUE;
                log_msg("GOAL COMPLETE!");
                /* Queue a goal status update */
                EnterCriticalSection(&loc_cs);
                pending_locs[num_pending_locs++] = 0; /* sentinel: 0 = goal */
                LeaveCriticalSection(&loc_cs);
            }
        }
        Sleep(POLL_MS);
    }
    return 0;
}

/* ========================================================================= */
/* Network Thread — WebSocket + AP protocol                                  */
/* ========================================================================= */

static DWORD WINAPI network_thread(LPVOID param) {
    (void)param;

    log_msg("network thread started, waiting for config...");
    while (!load_config()) Sleep(1000);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    int retry_delay = 1000;
    char *recv_buf = (char *)malloc(WS_BUF_SIZE);

    while (1) {
        snprintf(ap_status_text, sizeof(ap_status_text), "#0EAP: Connecting...");
        log_msg("connecting to %s:%d...", cfg_host, cfg_port);

        if (!ws_connect(cfg_host, cfg_port)) {
            snprintf(ap_status_text, sizeof(ap_status_text), "#0CAP: No Server");
            log_msg("connection failed, retrying in %dms...", retry_delay);
            Sleep(retry_delay);
            if (retry_delay < 30000) retry_delay *= 2;
            continue;
        }
        retry_delay = 1000;

        /* Set socket timeout for recv so we can periodically send */
        int timeout_ms = 100;
        setsockopt(ws_sock, SOL_SOCKET, SO_RCVTIMEO,
                   (char *)&timeout_ms, sizeof(timeout_ms));

        while (1) {
            /* Try to receive a message */
            int len = ws_recv_text(recv_buf, WS_BUF_SIZE - 1);
            if (len > 0) {
                recv_buf[len] = 0;
                ap_process_message(recv_buf);
            } else if (len < 0) {
                log_msg("WebSocket recv error, disconnecting");
                break;
            }
            /* len == 0 means timeout, which is fine — just check sends */

            /* Send pending location checks */
            EnterCriticalSection(&loc_cs);
            if (num_pending_locs > 0) {
                /* Check for goal sentinel */
                BOOL send_goal = FALSE;
                DWORD real_locs[MAX_CHECKED];
                int real_count = 0;
                for (int i = 0; i < num_pending_locs; i++) {
                    if (pending_locs[i] == 0)
                        send_goal = TRUE;
                    else
                        real_locs[real_count++] = pending_locs[i];
                }
                num_pending_locs = 0;
                LeaveCriticalSection(&loc_cs);

                if (real_count > 0) {
                    ap_send_location_checks(real_locs, real_count);
                    log_msg("sent %d location checks", real_count);
                }
                if (send_goal) {
                    ap_send_goal_complete();
                    log_msg("sent goal complete!");
                }
            } else {
                LeaveCriticalSection(&loc_cs);
            }
        }

        /* Disconnected — cleanup and retry */
        ws_close();
        ap_connected = FALSE;
        snprintf(ap_status_text, sizeof(ap_status_text), "#0CAP: Disconnected");
        log_msg("disconnected, retrying...");
        Sleep(retry_delay);
    }

    free(recv_buf);
    return 0;
}

/* ========================================================================= */
/* DLL Entry Point                                                           */
/* ========================================================================= */

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved) {
    (void)reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        /* Resolve base directory from DLL location */
        GetModuleFileNameA(hInst, g_base_dir, MAX_PATH);
        char *last_slash = strrchr(g_base_dir, '\\');
        if (last_slash) *(last_slash + 1) = '\0';
        else lstrcpyA(g_base_dir, ".\\");

        snprintf(g_log_path, MAX_PATH, "%saphook.log", g_base_dir);
        snprintf(g_config_path, MAX_PATH, "%sap_connect.txt", g_base_dir);

        g_log = fopen(g_log_path, "a");
        log_msg("aphook.dll loaded — full AP client");
        log_msg("base dir: %s", g_base_dir);

        InitializeCriticalSection(&loc_cs);
        InitializeCriticalSection(&item_cs);
        memset((void *)permitted, 0, sizeof(permitted));

        install_render_hook();

        CreateThread(NULL, 0, network_thread, NULL, 0, NULL);
        CreateThread(NULL, 0, monitor_thread, NULL, 0, NULL);
    }

    return TRUE;
}
