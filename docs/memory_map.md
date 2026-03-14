# Ys I & II Chronicles - Memory Map

Game: Ys I & II Chronicles (USA)
Game ID: ULUS-10547
Platform: PSP

## Ys I Addresses

### Player Stats
| Address    | Size | Description |
|------------|------|-------------|
| 0x0175C24  | 4    | HP          |
| 0x0175C28  | 4    | Attack      |
| 0x0175C2C  | 4    | Defense     |
| 0x0175C30  | 4    | Gold        |

### Movement
| Address    | Size | Description |
|------------|------|-------------|
| 0x0074510  | 4    | Move Speed  |
| 0x0074514  | 4    | Move Speed 2|

### Equipment Slots (Currently Equipped)
| Address    | Size | Description | Values |
|------------|------|-------------|--------|
| 0x0175A98  | 4    | Weapon      | 0=Short, 1=Long, 2=Talwar, 3=Silver, 4=Flame |
| 0x0175A9C  | 4    | Armor       | 10=Chain, 11=Plate, 12=Reflex, 13=Battle, 14=Silver |
| 0x0175AA0  | 4    | Shield      | 5=Small, 6=Middle, 7=Large, 8=Battle, 9=Silver |
| 0x0175AA4  | 4    | Ring        | 15=Power, 16=Shield, 17=Timer, 18=Heal, 19=Evil |
| 0x0175AA8  | 4    | Consumable  | 44=Heal Potion, 45=Mirror |

Note: Value 0xFFFFFFFF (-1) = Nothing equipped

### Ring Effect Flags (Active Effects)
| Address    | Size | Description                |
|------------|------|----------------------------|
| 0x0175EA8  | 4    | Unknown ring effect flag   |
| 0x0175EBC  | 4    | Heal Ring effect (1=active)|

### Item Arrays (BOTH required for items to work)
| Address    | Size | Description                          |
|------------|------|--------------------------------------|
| 0x0175DB4  | 208  | Ownership flags (makes selectable)   |
| 0x01764A0  | 208  | Discovered flags (shows in list)     |

**IMPORTANT**: Both arrays must be set to 1 for an item to be fully usable!
- Ownership only: Item won't show in inventory
- Discovered only: Item shows but is greyed out/unavailable
- Both set: Item is fully usable and equippable

Each item ID corresponds to offset: base + (item_id * 4)

### Consumable Refill Addresses
To refill a used consumable (restore ownership without changing discovered):
| Item | ID | Ownership Address | Formula |
|------|----|--------------------|---------|
| Heal Potion | 44 | 0x0175E64 | 0x0175DB4 + (44 * 4) |
| Mirror | 45 | 0x0175E68 | 0x0175DB4 + (45 * 4) |

**Refill behavior**: When a consumable is used, the game clears the ownership flag but keeps the discovered flag. To make it usable again, only write 1 to the ownership address.

**Note**: The game only holds 1 of each consumable at a time - the ownership field is a boolean flag, not a count.

### Item IDs (for equipment slots)
| ID | Name | Category | Notes |
|----|------|----------|-------|
| 0  | Short Sword | Weapon | Verified |
| 1  | Long Sword | Weapon | Verified |
| 2  | Talwar | Weapon | Verified |
| 3  | Silver Sword | Weapon | Verified |
| 4  | Flame Sword | Weapon | Verified |
| 5  | Small Shield | Shield | Verified |
| 6  | Middle Shield | Shield | Verified |
| 7  | Large Shield | Shield | Verified |
| 8  | Battle Shield | Shield | Verified |
| 9  | Silver Shield | Shield | Verified |
| 10 | Chain Mail | Armor | Verified |
| 11 | Plate Mail | Armor | Verified |
| 12 | Reflex | Armor | Verified |
| 13 | Battle Armor | Armor | Verified |
| 14 | Silver Armor | Armor | Verified |
| 15 | Power Ring | Ring (2x ATK) | Verified |
| 16 | Shield Ring | Ring (50% DMG) | Verified |
| 17 | Timer Ring | Ring (Slow) | Verified |
| 18 | Heal Ring | Ring (HP Regen) | Verified |
| 19 | Evil Ring | Ring (HP Drain) | Verified |

### Quest Items (IDs 20+)
| ID | Name | Category | Notes |
|----|------|----------|-------|
| 20-25 | Books of Ys (6) | Key Item | |
| 26 | Treasure Box Key | Key Item | Verified (swapped with 28) |
| 27 | Prison Key | Key Item | |
| 28 | Shrine Key | Key Item | Verified (swapped with 26) |
| 29 | Ivory Key | Key Item | |
| 30 | Marble Key | Key Item | |
| 31 | Darm Key | Key Item | |
| 32 | Sara's Crystal | Quest Item | |
| 33 | Roda Tree Seed | Quest Item | |
| 34 | Silver Bell | Quest Item | |
| 35 | Silver Harmonica | Quest Item | |
| 36 | Idol | Quest Item | |
| 37 | Rod | Quest Item | |
| 38 | Monocle | Quest Item | |
| 39 | Blue Amulet | Quest Item | |
| 40 | Ruby | Treasure | |
| 41 | Sapphire Ring | Treasure | Verified (save state) |
| 42 | Ring (treasure) | Treasure | |
| 43 | Golden Vase | Treasure | Verified (pickup in Minea Fields) |
| 44 | Heal Potion | Consumable | Equippable in consumable slot |
| 45 | Mirror | Consumable | Verified (save state) |
| 46 | Hammer | Item | Swapped with Wing — needs verification |
| 47 | Wing | Item | Verified (save state) |
| 48 | Mask of Eyes | Accessory | Auto-equips when obtained |
| 49 | Blue Necklace | Item | |
| 50 | Bestiary Potion | Item | |
| 51 | Piece of Paper | Item | |

Total: 52 items (IDs 0-51)

### TODO: Map Individual Items
- [x] Swords (0-4)
- [x] Shields (5-9)
- [x] Armor (10-14)
- [x] Rings (15-19)
- [x] Quest Items (20-51) - Complete

---

## Ys II Addresses

### Player Stats
| Address    | Size | Description |
|------------|------|-------------|
| 0x027809C  | 4    | HP          |
| 0x02780A4  | 4    | MP          |
| 0x02780A8  | 4    | Attack      |
| 0x02780AC  | 4    | Defense     |
| 0x02780B4  | 4    | Gold        |

### Magic & Rings
| Address    | Size | Description           |
|------------|------|-----------------------|
| 0x06C8800  | ?    | Magic Spells (0x12-0x17) |
| 0x06C8804  | ?    | Rings (0x1B-0x1D)     |

### Items & Equipment
| Address    | Size | Description       |
|------------|------|-------------------|
| 0x027757C  | ?    | Items block       |
| 0x02775C4  | ?    | Items block (alt) |

### TODO: Map Individual Items
- [ ] Swords
- [ ] Armor
- [ ] Shields
- [ ] Magic spells (Fire, Light, Return, Time Stop, Telepathy, Alter)
- [ ] Quest Items (Goddess Statues, Keys, etc.)

---

## Archipelago Location Types

For the randomizer, we need to track various "location checks" - places where items can be obtained.
Each category can be toggled on/off in randomizer options.

### Location Categories

| Category | Detection | Optional | Notes |
|----------|-----------|----------|-------|
| Chests | Memory flag | No (core) | Game has chest opened flags |
| NPC Gifts | Memory flag | No (core) | One-time NPC item gifts |
| Boss Drops | Memory flag | No (core) | Tied to boss defeated flags |
| **Shops** | Client-tracked | **Yes** | Optional category - see below |

---

## Shop Locations (OPTIONAL CATEGORY)

Shop purchases are an **optional randomizer category**. When enabled:
- Each shop item slot = one location check
- Buying from that slot sends the check (first purchase only)
- Client tracks purchases (game doesn't have native flags)

### Detection Method
Monitor for: gold decrease + specific item ownership increase
The client maintains its own "shop_purchases" set for tracking.

### Research Notes
- `0x018632C` appears to mirror gold value (backup/display copy?)
- `0x018304C` changes on various actions (shop, NPC) - likely a general "action performed" flag, not shop-specific
- **No dedicated shop purchase flags found** - client must track purchases itself

### Conclusion
Shops don't have native "purchased" flags. The client will track purchases by monitoring:
1. Gold decrease
2. Item ownership increase
3. Correlating the two to identify which shop item was bought

### Ys I Shops

**Minea Town (Starting Area)**
| Slot | Default Item | Price | Availability |
|------|--------------|-------|--------------|
| 1 | Short Sword | 0 (free) | Start |
| 2 | Small Shield | 200 | Start |
| 3 | Chain Mail | 400 | Start |
| 4 | Long Sword | 1000 | After Shrine? |
| 5 | Middle Shield | 1500 | After Shrine? |

**Zepik Village**
| Slot | Default Item | Price | Availability |
|------|--------------|-------|--------------|
| TBD | TBD | TBD | TBD |

**TODO**: Map all shop inventories and availability triggers

---

## Chest Flags

Chest opened flags discovered through memory diffing.

### Flag Array Location
| Address    | Size | Description |
|------------|------|-------------|
| 0x0175CB8  | 4*N  | Chest flags array (sequential 32-bit flags) |

**Pattern**: `chest_flag_address = 0x0175CB8 + (chest_id * 4)`

Each chest has a 4-byte flag that changes from 0 to 1 when opened.

### Discovered Chest Flags
| Chest ID | Contents | Flag Address | Notes |
|----------|----------|--------------|-------|
| 0 | Bestiary Potion | 0x0175CB8 | First chest tested |
| 1 | Piece of Paper | 0x0175CBC | Confirmed sequential pattern |

### Memory Layout
```
0x0175CB8  Chest 0 flag (Bestiary Potion)
0x0175CBC  Chest 1 flag (Piece of Paper)
0x0175CC0  Chest 2 flag (?)
...
0x0175DB4  Item Ownership Array starts (63 chests max before overlap)
```

### TODO
- [x] Confirm flags are sequential (4 bytes apart) - CONFIRMED
- [ ] Find total number of chests in Ys I
- [ ] Map chest IDs to locations/contents
- [ ] Determine if chest IDs are consistent across saves

---

## NPC/Dialogue Flags

Addresses discovered through memory diffing NPC interactions:

| Address    | Size | Description | Notes |
|------------|------|-------------|-------|
| 0x017600C  | 4    | NPC interaction flag? | Changed 0→1 on accepting NPC dialogue |
| 0x018304C  | 4    | General action flag | Changes on shop purchases, NPC interactions - generic "action performed" |

**Note**: These may be temporary flags that reset, or persistent quest flags. More testing needed.

---

## Progression Flags

### Ys I Story/Cutscene Flags
| Address    | Size | Description | Notes |
|------------|------|-------------|-------|
| 0x0177AC8  | 4    | Story progression? | 0→10 after Roda Tree cutscene |

**Note**: Value went to 10 (0x0A), not 1. May be a counter or multi-state flag.

### Boss Defeated Flags (TODO — Phase A2)

**Status:** NOT YET DISCOVERED — placeholder base address 0x0177B00 used in client code.

**Discovery method:**
1. Use `tools/memory_diff.py` to take snapshot before each boss fight
2. Defeat the boss
3. Take snapshot after and diff
4. Look for 0→1 transitions in the 0x0177xxx region (near story flags)

**Expected layout:**
| Boss Index | Boss Name | Flag Address (placeholder) |
|------------|-----------|---------------------------|
| 0 | Jenocres | 0x0177B00 |
| 1 | Vagullion | 0x0177B04 |
| 2 | Pictimos | 0x0177B08 |
| 3 | Khonsclard | 0x0177B0C |
| 4 | Dark Fact | 0x0177B10 |

**Notes:**
- Flags may not be sequential — they could be scattered in the story flags region
- Each flag is likely 4 bytes (0=alive, 1=defeated) based on other flag patterns
- Story flag at 0x0177AC8 went to 10 (not 1), so boss flags might use different values

### Ys I TODO
- [x] Boss defeated flags — placeholder addresses added, need verification
- [ ] Door/area unlock flags
- [ ] More story progression flags
- [ ] Complete chest ID → location mapping

### Ys II
- [ ] Boss defeated flags
- [ ] Area unlock flags
- [ ] Story progression flags

---

## Save/Load Detection

### The Problem
When a player loads a save file, memory gets overwritten with save data. The AP client needs to detect this and re-apply received items.

### Implemented: Client-Side State Tracking

The client (`ys_client.py`) tracks its own state and detects discrepancies:

```python
# YsGameState tracks:
ap_received_items: Set[int]    # Items received from AP server
last_state_checksum: str       # MD5 hash of item ownership state

# Each poll cycle (check_save_load):
current_items = read_all_items()
current_checksum = compute_state_checksum(current_items)

if current_checksum != last_state_checksum:
    current_owned = {id for id, owned in current_items.items() if owned}
    missing = ap_received_items - current_owned
    if missing:
        print(f"Save load detected - {len(missing)} AP items missing")
        reapply_ap_items()  # Re-give all AP items
```

**Key methods in YsGameState:**
- `compute_state_checksum()` - MD5 hash of item ownership bits
- `check_save_load()` - Detects save loads, re-applies AP items
- `receive_ap_item()` - Gives item and tracks for persistence
- `get_sync_data()` / `load_sync_data()` - Serialize state for AP server

**Why this works:**
- No native flag needed
- Handles save loads, new games, death reloads
- Simple: if AP items are gone, re-give them
- Checksum sent to AP server for persistence across client restarts

---

## Research Notes

### How to Find New Addresses
1. Open PPSSPP with debug mode
2. Use Memory View (Debug > Memory View)
3. Search for known values (e.g., current gold amount)
4. Modify in-game, search for changed value
5. Repeat until single address found

### CWCheat Code Format
- `0x0XXXXXXX` = 8-bit write
- `0x1XXXXXXX` = 16-bit write
- `0x2XXXXXXX` = 32-bit write
- `0x4XXXXXXX` = multi-write
- `0x5XXXXXXX` = 32-bit conditional
- `0x8XXXXXXX` = 8-bit serial write

The actual address is `XXXXXXX` (7 hex digits after the type prefix).
