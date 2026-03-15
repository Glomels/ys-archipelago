# Ys I Chronicles — Archipelago Randomizer

An [Archipelago](https://archipelago.gg) multiworld randomizer for **Ys I Chronicles+** (Steam PC version). Currently supports Ys I only; Ys II support may be added in the future.

## Requirements

- **Ys I & II Chronicles+** (Steam, PC version)
- **Archipelago** 0.6.6+

## Setup

### Quick install (precompiled)

1. Back up your game's `steam_api.dll` by renaming it to `steam_api_orig.dll`
2. Copy from the `player/` directory into the same folder as `ys1plus.exe`:
   - `steam_api.dll` — proxy that loads the AP client
   - `aphook.dll` — full AP client
   - `ap_connect.txt` — edit with your server info
3. Install `player/ys_chronicles.apworld` into your Archipelago worlds directory
4. Edit `player/Ys I Chronicles.yaml` to configure options, then generate a seed
5. Launch the game normally

### Build from source

```bash
# Build everything
tools/build_apworld.sh

# Deploy to game directory
tools/setup_mod.sh [server] [slot] [password]
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
