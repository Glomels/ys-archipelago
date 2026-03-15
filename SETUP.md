# Ys I Chronicles — Archipelago Setup Guide

## Requirements

- **Ys I & II Chronicles+** (Steam PC version, via CrossOver/Wine on macOS or native on Windows)
- **Archipelago** 0.6.6+ ([archipelago.gg](https://archipelago.gg))
- **i686-w64-mingw32-gcc** (32-bit Windows cross-compiler, for building the DLL)
  - macOS: `brew install mingw-w64`
  - Linux: `apt install gcc-mingw-w64-i686`

## Quick Start

### 1. Build the mod

```bash
# Build the apworld and DLL
./build_apworld.sh

# Deploy to game directory (backs up original steam_api.dll)
./setup_mod.sh [server] [slot] [password]
# Defaults: localhost:38281, Adol, no password
```

### 2. Install the apworld

Copy the generated `ys_chronicles.apworld` to your Archipelago worlds directory:

```bash
# macOS
cp ys_chronicles.apworld "/Applications/Archipelago.app/Contents/MacOS/lib/worlds/"

# Windows/Linux
cp ys_chronicles.apworld "path/to/Archipelago/lib/worlds/"
```

### 3. Generate a seed

Edit `Ys I Chronicles.yaml` to configure your options, then:

```bash
# macOS
/Applications/Archipelago.app/Contents/MacOS/ArchipelagoGenerate \
  --player_files_path ./player_configs

# The YAML must be in its own directory (not mixed with .sh/.py files)
```

### 4. Host the server

```bash
/Applications/Archipelago.app/Contents/MacOS/ArchipelagoServer \
  "~/Library/Application Support/Archipelago/output/AP_<seed>.zip"
```

### 5. Configure the client

Create `ap_connect.txt` in your game directory:

```
server=localhost:38281
slot=Adol
password=
```

### 6. Launch the game

Start Ys I Chronicles normally. The mod DLL loads automatically via the steam_api.dll proxy. You should see connection status text on screen (green = connected).

## How It Works

### Architecture

```
AP Server (Archipelago)          aphook.dll (injected into ys1plus.exe)
    ↕ WebSocket                      |
    ↕                                ├── Hooks give_item() at 0x44F850
    ↕ ReceivedItems → ap_give()      ├── Monitors location flags (memory polling)
    ↕ LocationChecks ← flag/shop     ├── Detects shop purchases (caller address)
    ↕                                ├── Suppresses unauthorized items
                                     ├── Guards Darm Tower entry
                                     └── Renders status text on screen
```

### DLL Injection

The mod uses a **steam_api.dll proxy** approach:
1. Original `steam_api.dll` is renamed to `steam_api_orig.dll`
2. Our proxy `steam_api.dll` forwards all Steam API calls to the original
3. The proxy loads `aphook.dll` which contains the full AP client

### Item Interception

- `give_item()` is hooked via a 5-byte JMP detour
- Vanilla item grants are intercepted and blocked
- Only items permitted by the AP server are allowed through
- The AP server is the **source of truth** — save file state is overridden on every load

### Location Checks

Two detection methods:
1. **Memory flag polling** — NPC gifts, chest pickups, boss kills (flag address transitions 0→non-zero)
2. **Shop purchase detection** — Caller address range 0x493400-0x494100 identifies shop context

### Save File Sync

On every save load (`in_game` 0→1 transition):
- All AP-permitted items are re-given
- Unauthorized items from the save are suppressed
- Location flags are re-scanned and new checks sent to server

### Darm Tower Safety

Tower is a point of no return. The client monitors Goban's flags and blocks entry if the player is missing tower-internal items that were placed in overworld locations.

## Options

| Option | Default | Description |
|--------|---------|-------------|
| Goal | Dark Fact | Win condition: Dark Fact, All Books, or All Bosses |
| Shuffle Equipment | On | Weapons, armor, shields in the pool |
| Shuffle Rings | On | Rings in the pool |
| Shuffle Keys | On | Keys in the pool |
| Shuffle Quest Items | On | Books of Ys, Harmonica, etc. in the pool |
| Shuffle Consumables | Off | Heal Potions, Mirrors in the pool |
| Shuffle Shops | On | Shop purchases are location checks |
| Boss Checks | On | Boss kills are location checks |
| Starting Weapon | None | Start with Short Sword, Long Sword, or nothing |
| Experience Multiplier | 100% | 25-400% XP scaling |
| Gold Multiplier | 100% | 25-400% gold scaling |
| Death Link | Off | Shared death across multiworld |

## Debug Tools

```bash
# Give items through the DLL (safe, sets permitted flag)
./tools/ys.sh ap-give <game_id> [game_id] ...

# Set gold
./tools/ys.sh set gold <amount>

# Set stats
./tools/ys.sh set hp/maxhp/str/def/level <value>
```

## Troubleshooting

- **"AP: Connecting..."** — Check that the server is running and `ap_connect.txt` has the right address
- **Items disappearing** — The DLL suppresses items not granted by AP. Use `ys.sh ap-give` instead of `ys.sh give`
- **Crash on memory read** — Never use `ys.sh items` while the game is running; it crashes reliably
- **GOG version** — The steam_api.dll proxy won't work. Alternative injection method needed (future work)
- **Check logs** — `aphook.log` in the game directory has detailed diagnostics
