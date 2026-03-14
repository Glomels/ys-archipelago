# Ys I & II Chronicles - Archipelago Integration

An Archipelago multiworld randomizer integration for Ys I & II Chronicles (PSP).

## Project Status

🔬 **Active Development** - Memory research ongoing, core tools functional

## Structure

```
ys-archipelago/
├── docs/
│   ├── memory_map.md    # Known memory addresses
│   ├── locations.md     # Item location tracking
│   └── items.md         # Item list and classifications
├── tools/
│   ├── memory_gui.py    # GUI for exploring/testing memory flags
│   ├── memory_diff.py   # Snapshot comparison for flag discovery
│   └── client/
│       └── ys_client.py # Archipelago client for PPSSPP
└── world/
    └── __init__.py      # Archipelago world definition (WIP)
```

## Quick Start (macOS)

```bash
# Run the setup script
./setup_mac.sh

# Activate the virtual environment
source venv/bin/activate

# Start PPSSPP with debugger enabled (port 54453)
# Then run the memory GUI
python tools/memory_gui.py
```

## Requirements

- PPSSPP emulator with remote debugger enabled
- Ys I & II Chronicles (USA) ISO - ULUS-10547
- Python 3.11+
- macOS (setup script downloads Archipelago 0.6.6 for macOS x64)

## Development Roadmap

### Phase 1: Research
- [x] Map item memory addresses (dual-array system: ownership + discovered)
- [x] Map player stats (HP, Attack, Defense, Gold)
- [x] Map equipped items
- [x] Map chest flags (sequential array at 0x0175CB8)
- [ ] Map boss defeated flags
- [ ] Map progression triggers

### Phase 2: Client Development
- [x] PPSSPP memory interface (WebSocket)
- [x] Item ownership read/write
- [x] Memory diff tool for flag discovery
- [x] GUI for memory exploration
- [ ] Location check detection
- [ ] Item receiving/injection
- [ ] Game state synchronization

### Phase 3: World Development
- [x] Define all items with IDs (52 items mapped)
- [ ] Define all locations with IDs
- [ ] Create region graph
- [ ] Implement access logic
- [ ] Add randomization options

### Phase 4: Testing & Polish
- [ ] Single-world testing
- [ ] Multiworld testing
- [ ] Logic validation
- [ ] Documentation

## Key Discoveries

### Dual-Array Item System
Items use two parallel arrays at:
- **Ownership**: `0x0175DB4` - Whether you have the item (0/1)
- **Discovered**: `0x01764A0` - Whether item appears in inventory menu (0/1)

Both must be set to 1 for an item to be usable.

### Memory Addresses (Ys I)
| Address | Description |
|---------|-------------|
| `0x0175C20` | Current HP |
| `0x0175C24` | Max HP |
| `0x0175C28` | Attack |
| `0x0175C2C` | Defense |
| `0x0175C30` | Gold |
| `0x0175A98` | Equipped Weapon |
| `0x0175A9C` | Equipped Armor |
| `0x0175AA0` | Equipped Shield |
| `0x0175AA4` | Equipped Ring |

## Resources

- [Archipelago Documentation](https://github.com/ArchipelagoMW/Archipelago/blob/main/docs/adding%20games.md)
- [PPSSPP Debugger API](https://forums.ppsspp.org/showthread.php?tid=23592)
- [Ys Wiki](https://isu.fandom.com/wiki/Ys_Wiki)

## Contributing

This is a personal project. Feel free to fork if you want to help!
