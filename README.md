# Ys I Chronicles — Archipelago Randomizer

An [Archipelago](https://archipelago.gg) multiworld randomizer for **Ys I Chronicles+** (Steam PC version).

## Overview

Randomizes item locations across the game — chests, NPC gifts, boss rewards, and shop purchases become location checks that can contain any item from the pool. Connect with other Archipelago players for multiworld sessions.

### Features

- **72 randomized locations** — chests, NPC gifts, bosses, and 13 shop purchases
- **51 items** in the pool — weapons, armor, shields, rings, keys, quest items, books, consumables
- **Full in-process AP client** — DLL injected into the game handles everything automatically
- **On-screen status** — connection status displayed in-game
- **Save sync** — AP server is source of truth; game state re-syncs on every save load
- **Tower safety** — Darm Tower point-of-no-return is guarded by the client
- **Death Link** support
- **Configurable options** — shuffle categories, goals, XP/gold multipliers, starting weapon

## Requirements

- **Ys I & II Chronicles+** (Steam, PC version)
- **Archipelago** 0.6.6+

## Setup

### Quick install (precompiled)

1. Back up your game's `steam_api.dll` by renaming it to `steam_api_orig.dll`
2. Copy both DLLs from the `patch/` directory into the same folder as `ys1plus.exe`:
   - `steam_api.dll` — proxy that loads the AP client
   - `aphook.dll` — full AP client
3. Create `ap_connect.txt` in the same folder as `ys1plus.exe`:
   ```
   server=localhost:38281
   slot=Adol
   password=
   ```
4. Launch the game normally

### Build from source

See [SETUP.md](SETUP.md) for full instructions. Requires `i686-w64-mingw32-gcc`.

```bash
# Build everything
./build_apworld.sh

# Deploy to game directory
./setup_mod.sh [server] [slot] [password]
```

## How It Works

The mod injects `aphook.dll` into the game via a `steam_api.dll` proxy. The DLL:

1. **Hooks `give_item()`** — intercepts all vanilla item grants (chests, NPCs, bosses, shops)
2. **Connects to AP server** — WebSocket client sends location checks and receives items
3. **Suppresses unauthorized items** — only items granted by AP are allowed in inventory
4. **Monitors memory flags** — detects location checks via flag transitions
5. **Syncs on save load** — re-applies AP state whenever a save file is loaded

## Legal

This project requires a legally purchased copy of Ys I & II Chronicles+. No game code or assets are distributed. All interaction is through runtime memory hooks and the Archipelago protocol.

Ys I & II Chronicles+ is developed by Nihon Falcom and published by XSEED Games.

## License

MIT

## Links

- [Archipelago](https://archipelago.gg)
- [Ys Wiki](https://isu.fandom.com/wiki/Ys_Wiki)
