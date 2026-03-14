I now have a thorough understanding of the entire codebase. Let me produce the implementation plan.

---

# Ys I Chronicles Archipelago Integration -- Implementation Plan

## Table of Contents
1. [Critical Data Reconciliation](#1-critical-data-reconciliation)
2. [items.py Rewrite](#2-itemspy-rewrite)
3. [locations.py Rewrite](#3-locationspy-rewrite)
4. [regions.py Update](#4-regionspy-update)
5. [client.py Rewrite](#5-clientpy-rewrite)
6. [__init__.py Update](#6-initpy-update)
7. [Testing Strategy](#7-testing-strategy)

---

## 1. Critical Data Reconciliation

There are THREE item ID schemes in play, and they conflict. The flag_mapping_guide.md (`/Users/luis/ys-archipelago/docs/flag_mapping_guide.md`) has the **PC version truth**, while `docs/items.md` has an **old PSP-era table** with wrong IDs. The user's prompt also has verified corrections. Here is the reconciled, canonical ID table to use:

### Canonical Game Item ID Table (PC Version)

The flag_mapping_guide.md at line 292-346 is authoritative. The "Item Array Reference" there gives exact addresses that back-compute to these IDs:

| Game ID | Item Name | Array 1 Addr | Array 2 Addr | Notes |
|---------|-----------|-------------|-------------|-------|
| 0 | Short Sword | 0x531990 | 0x53207C | |
| 1 | Long Sword | 0x531994 | 0x532080 | |
| 2 | Talwar | 0x531998 | 0x532084 | |
| 3 | Silver Sword | 0x53199C | 0x532088 | |
| 4 | Flame Sword | 0x5319A0 | 0x53208C | |
| 5 | Small Shield | 0x5319A4 | 0x532090 | |
| 6 | Middle Shield | 0x5319A8 | 0x532094 | |
| 7 | Large Shield | 0x5319AC | 0x532098 | |
| 8 | Battle Shield | 0x5319B0 | 0x53209C | |
| 9 | Silver Shield | 0x5319B4 | 0x5320A0 | |
| 10 | Chain Mail | 0x5319B8 | 0x5320A4 | |
| 11 | Plate Mail | 0x5319BC | 0x5320A8 | |
| 12 | Reflex | 0x5319C0 | 0x5320AC | |
| 13 | Battle Armor | 0x5319C4 | 0x5320B0 | **NOT Silver Armor** |
| 14 | Silver Armor | 0x5319C8 | 0x5320B4 | **NOT Battle Armor** |
| 15 | Power Ring | 0x5319CC | 0x5320B8 | |
| 16 | Timer Ring | 0x5319D0 | 0x5320BC | **flag_mapping_guide says "Timer Ring" at ID 16** |
| 17 | Heal Ring | 0x5319D4 | 0x5320C0 | **flag_mapping_guide says "Heal Ring" at ID 17** |
| 18 | Evil Ring | 0x5319D8 | 0x5320C4 | |
| 19 | Goddess Ring | 0x5319DC | 0x5320C8 | |
| 20 | Volume Hadal | 0x5319E0 | 0x5320CC | |
| 21 | Volume Tovah | 0x5319E4 | 0x5320D0 | |
| 22 | Volume Dabbie | 0x5319E8 | 0x5320D4 | |
| 23 | Volume Mesa | 0x5319EC | 0x5320D8 | |
| 24 | Volume Gemma | 0x5319F0 | 0x5320DC | |
| 25 | Volume Fact | 0x5319F4 | 0x5320E0 | |
| 26 | Treasure Box Key | 0x5319F8 | 0x5320E4 | |
| 27 | Prison Key | 0x5319FC | 0x5320E8 | |
| 28 | Shrine Key | 0x531A00 | 0x5320EC | |
| 29 | Ivory Key | 0x531A04 | 0x5320F0 | |
| 30 | Marble Key | 0x531A08 | 0x5320F4 | |
| 31 | Darm Key | 0x531A0C | 0x5320F8 | |
| 32 | Sara's Crystal | 0x531A10 | 0x5320FC | |
| 33 | Roda Tree Seed | 0x531A14 | 0x532100 | flag_mapping_guide calls it "Tovah's Amulet" internally |
| 34 | Silver Bell | 0x531A18 | 0x532104 | flag_mapping_guide calls it "Rado's Amulet" |
| 35 | Silver Harmonica | 0x531A1C | 0x532108 | flag_mapping_guide calls it "Dabbie's Pendant" |
| 36 | Idol | 0x531A20 | 0x53210C | flag_mapping_guide calls it "Tarf's Seed" |
| 37 | Rod | 0x531A24 | 0x532110 | flag_mapping_guide calls it "Ruby" |
| 38 | Monocle | 0x531A28 | 0x532114 | flag_mapping_guide calls it "Silver Harmonica" |
| 39 | Blue Amulet | 0x531A2C | 0x532118 | flag_mapping_guide calls it "Silver Bell" |
| 40 | Ruby | 0x531A30 | 0x53211C | flag_mapping_guide calls it "Idol" |
| 41 | Sapphire Ring | 0x531A34 | 0x532120 | |
| 42 | Necklace | 0x531A38 | 0x532124 | flag_mapping_guide calls it "Ring Treasure" |
| 43 | Golden Vase | 0x531A3C | 0x532128 | |
| 44 | Heal Potion | 0x531A40 | 0x53212C | |
| 45 | Mirror | 0x531A44 | 0x532130 | |
| 46 | Hammer | 0x531A48 | 0x532134 | |
| 47 | Wing | 0x531A4C | 0x532138 | |
| 48 | Mask of Eyes | 0x531A50 | 0x53213C | |
| 49 | Blue Necklace | 0x531A54 | 0x532140 | flag_mapping_guide calls it "Necklace" |
| 50 | Bestiary Potion | 0x531A58 | 0x532144 | |
| 51 | Piece of Paper | 0x531A5C | 0x532148 | |

**IMPORTANT DISCREPANCY**: The user's prompt says ID 16 = "Shield Ring" but the flag_mapping_guide.md (line 313) says ID 16 = "Timer Ring". The flag_mapping_guide is based on verified memory reads. The current `items.py` has BOTH "Shield Ring" (AP code +31) and "Timer Ring" (AP code +32) as separate items. Looking at the chest flags: `0x5318A0` is labeled "Shield Ring chest" with item "Shield Ring (ID 16)" in the user prompt, but the flag guide says the chest at `0x5318D8` is "Timer Ring chest" with ID 17. 

**Resolution**: The game has FOUR rings at IDs 15-18 and one at 19. The flag_mapping_guide.md is the ground truth from actual memory dumps. ID 16 = Timer Ring, ID 17 = Heal Ring, ID 18 = Evil Ring, ID 19 = Goddess Ring. The user's prompt contradicts the flag_mapping_guide on this point (claiming ID 16 = Shield Ring). I will follow the flag_mapping_guide since it comes from verified memory reads. The item named "Shield Ring" appears in the chest at `0x5318A0` -- but looking at the flag guide, it says that chest gives ID 16 which is Timer Ring in the memory array. This needs clarification at implementation time, but for now use the flag_mapping_guide labels.

Actually, re-reading more carefully: the memory_map.md (line 83-86) says `16 | Shield Ring | Ring (50% DMG)` and `17 | Timer Ring | Ring (Slow)`. But the flag_mapping_guide (line 313) says `16 | Timer Ring`. These docs disagree. The memory_map.md appears to be the more carefully verified version (it says "Verified" for rings). The flag_mapping_guide's item name table appears to just use internal game names that may not match display names. 

**Final resolution**: Use the names from memory_map.md lines 82-86 since those were verified through gameplay:
- ID 15 = Power Ring
- ID 16 = Shield Ring  
- ID 17 = Timer Ring
- ID 18 = Heal Ring
- ID 19 = Evil Ring (note: NOT Goddess Ring -- the user prompt says 19 = Goddess Ring but memory_map says Evil Ring)

Wait, memory_map.md line 86 says `19 | Evil Ring`. But the flag_mapping_guide line 313 says `19 | Goddess Ring`. Another conflict. The user's prompt explicitly says `19=Goddess Ring` and lists Evil Ring as ID 18.

Looking at the flag_mapping_guide more carefully -- line 310: `17 | Heal Ring`, line 311: `18 | Evil Ring`, line 312-313: `19 | Goddess Ring`. But memory_map.md says: `17 | Timer Ring`, `18 | Heal Ring`, `19 | Evil Ring`.

These are SHIFTED by one. The memory_map.md was for PSP. The flag_mapping_guide.md is for PC. The PC item table in flag_mapping_guide is authoritative for the PC version.

**FINAL DEFINITIVE ANSWER** (from flag_mapping_guide.md, the PC-verified source):
- ID 16 = Timer Ring
- ID 17 = Heal Ring  
- ID 18 = Evil Ring
- ID 19 = Goddess Ring

And from memory_map.md (PSP-verified):
- ID 16 = Shield Ring
- ID 17 = Timer Ring
- ID 18 = Heal Ring
- ID 19 = Evil Ring

The SHIELD RING does not appear at all in the flag_mapping_guide table. This means either:
1. Shield Ring doesn't exist in the PC version, or
2. Shield Ring is at a different ID

Looking at the user's prompt which says verified chest flags: `0x5318A0 | Shield Ring chest | Shield Ring (16)` -- this is the user's claim. And the chest at `0x5318D8` is "Timer Ring chest" with "Timer Ring (ID 17)" in the user prompt, but the flag guide says `Timer Ring chest opened | Timer Ring (ID 17)` at `0x5318D8`. 

The user's prompt is the most recent verified data. I'll trust it: **ID 16 = Shield Ring (was Timer Ring in old table), ID 17 = Heal Ring (was Timer Ring), ID 18 = Evil Ring, ID 19 = Goddess Ring**. Actually wait -- the user prompt says `16=Shield Ring (was Timer Ring in old table)` and `17=Heal Ring (was Timer Ring)`. This means: ID 16's OLD name was "Timer Ring" but it's actually "Shield Ring". ID 17's OLD name was "Timer Ring" but it's actually "Heal Ring". This is confusing. 

Looking at what the game actually does functionally would resolve this, but we don't have that. I'll use the names from the user's prompt header since those are the "corrected" names. So:

- **ID 16 = Shield Ring** (the flag_mapping_guide calls it "Timer Ring" -- old internal name)
- **ID 17 = Heal Ring** (the flag_mapping_guide calls it "Heal Ring" -- matches)
- **ID 18 = Evil Ring**
- **ID 19 = Goddess Ring**

Now, the current `items.py` has BOTH Shield Ring (AP +31) and Timer Ring (AP +32). Since ID 16 is Shield Ring, there is no separate Timer Ring item. Timer Ring doesn't exist as a distinct item. The current `items.py` Timer Ring entry should be REMOVED. The Heal Ring entry is correct at AP +33.

Wait -- but the user prompt also says the chest at `0x5318D8` is "Timer Ring chest" with "Timer Ring (17)". So there IS a Timer Ring and it's at ID 17? No -- looking again, the user says `17=Heal Ring (was Timer Ring)`. The flag_mapping_guide says the chest at `0x5318D8` contains "Timer Ring (ID 17)" using the OLD name. The CORRECTED name per the user is "Heal Ring". 

But then what about ID 16? The user says `16=Shield Ring (was Timer Ring in old table)`. The chest at `0x5318A0` gives "Shield Ring (ID 16)". 

OK so the reconciliation is:
- ID 16 = Shield Ring (internal name was "Timer Ring" in some tables)
- ID 17 = Heal Ring (internal name was "Timer Ring" in the chest flag table -- this is the SECOND ring that was called Timer Ring, actually Heal Ring)

But wait, the flag guide says chest `0x5318D8` = "Timer Ring chest opened | Timer Ring (ID 17)". And chest `0x5318DC` = "Heal Ring chest opened | Heal Ring (ID 18)". So in the flag guide, ID 17 = Timer Ring and ID 18 = Heal Ring. But the user says ID 17 = Heal Ring.

There's a clear conflict. The user's prompt header section says one thing, but some of the chest data in the same prompt uses the old names. Let me just go with the flag_mapping_guide.md item table (lines 292-346) since those are the raw internal game names at each ID offset. The AP world can use whatever display names we want -- but the GAME ID mapping must be correct. The critical thing is: when AP wants to give "Heal Ring", which game ID do we write to?

For the implementation plan, I'll flag this as needing a quick verification test at implementation time. The plan will use:
- ID 16 = "Shield Ring" (display name for AP)
- ID 17 = "Timer Ring" (display name for AP) -- may actually function as Heal Ring
- ID 18 = "Heal Ring" (display name for AP) -- may actually function as Evil Ring  
- Wait, this doesn't work either.

**OK, simplest approach**: The flag_mapping_guide.md has the names associated with each ID from the game's own perspective. Use those names as the AP item names, since that's what the player sees in-game. The flag_mapping_guide says:
- 16 = Timer Ring
- 17 = Heal Ring
- 18 = Evil Ring
- 19 = Goddess Ring

The user says the old table was wrong and gives corrections. But the corrections in the user prompt header say `16=Shield Ring (was Timer Ring in old table)`. This means the user RENAMED it. Looking at the actual game: "Shield Ring" is a real Ys I item (halves damage taken). "Timer Ring" is also a real Ys I item (slows enemies). They're different items. The question is which ID each occupies.

I'll use the user's prompt corrections as authoritative since they claim to be verified through gameplay:
- **16 = Shield Ring**
- **17 = Heal Ring**  
- **18 = Evil Ring**
- **19 = Goddess Ring**

And Timer Ring simply doesn't exist as a separate item in this version, or Shield Ring IS what was previously called Timer Ring (perhaps a localization difference). Either way, the AP world will use these names.

---

## 2. items.py Rewrite

**File**: `/Users/luis/ys-archipelago/apworld/ys_chronicles/items.py`

### Changes Required

#### A. Fix the Enum -- Remove TREASURE, add GODDESS_RING handling
The `YsItemType.TREASURE` is used on lines 305-312 but doesn't exist in the enum (lines 12-20). Golden Vase and Necklace should use `YsItemType.CONSUMABLE` (they're sell items, functionally filler).

#### B. Add `game_id` field to `YsItemData`
Currently `YsItemData` has `memory_offset` which is barely used. Replace it with `game_id: int` -- the PC game's internal item ID (0-51). This is essential for the client to know which memory address to write.

New `YsItemData`:
```python
class YsItemData(NamedTuple):
    code: int              # AP item ID (YS1_BASE_ID + offset)
    item_type: YsItemType
    classification: ItemClassification
    game_id: int           # PC game internal item ID (0-51)
```

#### C. Complete Item Table

Remove "Timer Ring" entry (AP +32). It doesn't exist as a separate item.

Add missing items:
- **Goddess Ring** (game_id=19, type=RING, classification=useful)
- **Bestiary Potion** (game_id=50, type=CONSUMABLE, classification=filler)
- **Piece of Paper** (game_id=51, type=CONSUMABLE, classification=filler)  
- **Wing** (game_id=47, type=CONSUMABLE, classification=useful)
- **Sapphire Ring** (game_id=41, type=QUEST, classification=filler -- it's a treasure/sell item)

Fix existing names:
- "Roda Seed" -> "Roda Tree Seed" (game_id=33)
- "Silver Bells" -> "Silver Bell" (game_id=34)

Fix game_id mappings (add game_id to every entry):
```python
"Short Sword":      game_id=0
"Long Sword":       game_id=1
"Talwar":           game_id=2
"Silver Sword":     game_id=3
"Flame Sword":      game_id=4
"Chain Mail":       game_id=10
"Plate Mail":       game_id=11
"Reflex":           game_id=12
"Silver Armor":     game_id=14   # NOT 13
"Battle Armor":     game_id=13   # NOT 14
"Small Shield":     game_id=5
"Middle Shield":    game_id=6
"Large Shield":     game_id=7
"Silver Shield":    game_id=9
"Battle Shield":    game_id=8
"Power Ring":       game_id=15
"Shield Ring":      game_id=16
"Heal Ring":        game_id=17   # was "Timer Ring" at AP +32
"Evil Ring":        game_id=18
"Goddess Ring":     game_id=19   # NEW
"Book of Ys (Hadal)": game_id=20
"Book of Ys (Tovah)": game_id=21
"Book of Ys (Dabbie)": game_id=22
"Book of Ys (Messa)":  game_id=23
"Book of Ys (Gemma)":  game_id=24
"Book of Ys (Fact)":   game_id=25
"Shrine Key":       game_id=28
"Prison Key":       game_id=27
"Treasure Box Key": game_id=26
"Ivory Key":        game_id=29
"Marble Key":       game_id=30
"Darm Key":         game_id=31
"Sara's Crystal":   game_id=32
"Roda Tree Seed":   game_id=33
"Silver Bell":      game_id=34
"Silver Harmonica": game_id=35
"Idol":             game_id=36
"Blue Necklace":    game_id=49
"Monocle":          game_id=38
"Blue Amulet":      game_id=39
"Hammer":           game_id=46
"Rod":              game_id=37
"Mask of Eyes":     game_id=48
"Heal Potion":      game_id=44
"Mirror":           game_id=45
"Ruby":             game_id=40
"Golden Vase":      game_id=43
"Necklace":         game_id=42
"Wing":             game_id=47   # NEW
"Sapphire Ring":    game_id=41   # NEW
"Bestiary Potion":  game_id=50   # NEW
"Piece of Paper":   game_id=51   # NEW
"Goddess Ring":     game_id=19   # NEW
```

Fix classifications:
- "Battle Armor" should be `progression` (needed for Tower F19 area, requires Blue Necklace to access the chest)
- "Silver Armor" should be `useful` (not progression)
- "Shield Ring" stays `useful`
- "Heal Ring" should be `progression` (HP regen needed for survival in mines/tower)
- Remove "Timer Ring" entirely
- "Golden Vase" and "Necklace" change type from `TREASURE` to `CONSUMABLE`

#### D. Add helper dictionaries

Add a `GAME_ID_TO_AP_NAME` dict and an `AP_NAME_TO_GAME_ID` dict for the client to use:
```python
GAME_ID_TO_AP_NAME: Dict[int, str] = {
    data.game_id: name for name, data in YS1_ITEMS.items()
}
AP_NAME_TO_GAME_ID: Dict[str, int] = {
    name: data.game_id for name, data in YS1_ITEMS.items()
}
```

---

## 3. locations.py Rewrite

**File**: `/Users/luis/ys-archipelago/apworld/ys_chronicles/locations.py`

### Changes Required

#### A. Add `memory_flag` to YsLocationData

Change the NamedTuple so `memory_flag` is required (not defaulting to 0). For NPC gifts that have multiple flags, use the PRIMARY flag (the one that controls whether the gift interaction fires).

#### B. Complete Location Table with All Verified Flags

Every location needs its `memory_flag` set from the verified data in flag_mapping_guide.md. Here is the complete mapping:

**Minea / Barbado / Zepik NPCs:**
```python
"Slaff's Gift":           memory_flag=0x531B20, region="Barbado", vanilla="Short Sword"
"Sara's Gift":            memory_flag=0x531BB4, region="Minea", vanilla="Sara's Crystal"
    # Sara has TWO flags: 0x531BAC (cutscene) + 0x531BB4 (gift). Use 0x531BB4 as the check flag.
"Franz's Gift":           memory_flag=0x531BC0, region="Minea", vanilla="Book of Ys (Tovah)"  # NEW
    # Franz has TWO flags: 0x531BC0 + 0x531BC4
"Jeba's Gift":            memory_flag=0x531C6C, region="Zepik", vanilla="Shrine Key"
    # Jeba has TWO flags: 0x531C6C + 0x53206C
"Silver Bell Reward":     memory_flag=0x531CAC, region="Zepik", vanilla="Power Ring"
    # Mayor Robels gives Power Ring when you bring Silver Bell
"Slaff's Second Gift":    memory_flag=0x531B2C, region="Zepik", vanilla="Talwar"
```

**Rename "Sara's Second Gift" to "Franz's Gift"**: The current code at location line 209-214 calls this "Sara's Second Gift" with vanilla item "Book of Ys (Tovah)". But the user's flag data says FRANZ gives Volume Tovah (0x531BC0), not Sara. Sara gives Sara's Crystal. Fix the name and flag.

**Minea Fields:**
```python
"Minea Fields - Bestiary Potion":  memory_flag=0x531894, vanilla="Bestiary Potion"
"Minea Fields - Locked Chest":     memory_flag=0x531898, vanilla="Piece of Paper"
"Minea Fields - Golden Vase":      memory_flag=0x531C28, vanilla="Golden Vase"  # field pickup
"Southern Roda Tree":              memory_flag=0x531C30, vanilla="Silver Sword"
    # SPECIAL: This flag is REVERSE (1->0 when claimed). Client must handle specially.
```

**Shrine F1:**
```python
"Shrine F1 - Shield Ring Chest":   memory_flag=0x5318A0, vanilla="Shield Ring"
"Shrine F1 - Ruby Chest":          memory_flag=0x5318A4, vanilla="Ruby"
"Shrine F1 - Necklace Chest":      memory_flag=0x5318A8, vanilla="Necklace"  # locked chest
```

**Shrine B1:**
The flag mapping guide doesn't have specific Shrine B1 chest flags verified. These are the locked chests. Looking at the flag addresses: `0x5318C8` is "Heal Potion chest opened" which could be a Shrine B1 chest. But the assignment is uncertain.

Actually, looking more carefully at the chest flags, the Shrine B3 chests are well-mapped. For Shrine B1:
```python
"Shrine B1 - Locked Chest 1":     memory_flag=0x5318C8, vanilla="Heal Potion"
    # 0x5318C8 = "Heal Potion chest" -- this could be a Shrine B1 locked chest
"Shrine B1 - Locked Chest 2":     memory_flag=0x5318D4, vanilla="Heal Potion"
    # 0x5318D4 = "Heal Potion chest 2"
```
These assignments are TENTATIVE. The implementer should verify these by testing in-game.

**Shrine B2:**
```python
"Shrine B2 - Prison Key Chest":    memory_flag=0x5318B4, vanilla="Prison Key"
"Shrine B2 - Treasure Box Key Chest": memory_flag=0x5318B8, vanilla="Treasure Box Key"
"Shrine B2 - Silver Bell Chest":   memory_flag=0x5318B0, vanilla="Silver Bell"
"Shrine B2 - Mask of Eyes Chest":  memory_flag=0x5318AC, vanilla="Mask of Eyes"
```

**Shrine B3:**
```python
"Shrine B3 - Ivory Key Chest":    memory_flag=0x5318C0, vanilla="Ivory Key"
"Shrine B3 - Marble Key Chest":   memory_flag=0x5318BC, vanilla="Marble Key"
"Shrine B3 - Heal Potion Chest":  memory_flag=0x5318C8, vanilla="Heal Potion"  # May conflict with B1
"Shrine B3 - Silver Shield Chest": memory_flag=0x5318C4, vanilla="Silver Shield"
"Boss: Jenocres":                  memory_flag=0x531950, vanilla="Book of Ys (Hadal)"
```

Note: Heal Potion chest at 0x5318C8 -- I assigned this to BOTH B1 and B3 which is wrong. The implementer needs to test which is which. One of these should be `0x5318D4` (Heal Potion chest 2).

**Mine F1:**
```python
"Mine F1 - Silver Armor Chest":    memory_flag=0x5318D0, vanilla="Silver Armor"
    # Flag guide says ID 13 (Battle Armor) but user says Silver Armor. Check the ID column.
    # Flag guide line 369: "Silver Armor chest opened | Silver Armor (ID 13)"
    # BUT ID 13 = Battle Armor in our canonical table! This is the old naming.
    # The actual item at this chest is game_id=14 (Silver Armor). The flag guide's "(ID 13)" is WRONG.
    # Actually wait -- looking at the diff flags more carefully, the item given at chest 0x5318D0
    # would flip item array 1 at 0x5319C8 (=game_id 14=Silver Armor). So the item IS Silver Armor.
    # The "(ID 13)" in the flag guide was using the OLD PSP ID scheme. Use game_id=14.
"Mine F1 - Heal Potion Chest":    memory_flag=0x5318D4, vanilla="Heal Potion"
"Mine F1 - Timer Ring Chest":      memory_flag=0x5318D8, vanilla="Shield Ring"
    # Chest at 0x5318D8 is labeled "Timer Ring chest" in flag guide, but per user's corrections,
    # game_id 16 is actually Shield Ring. Wait -- BUT the user also calls this "Timer Ring Chest" 
    # in the locations.py. Confusing. The LOCATION NAME can stay as-is or be fixed.
    # Actually: the user's prompt chest table says: 0x5318D8 | Timer Ring chest | Timer Ring (17)
    # And the corrections say: 17=Heal Ring (was Timer Ring)
    # So this chest gives Heal Ring (game_id 17). The location should be named:
"Mine F1 - Heal Ring Chest":       memory_flag=0x5318D8, vanilla="Heal Ring"
    # OR keep it "Timer Ring Chest" for the location name but set vanilla_item="Heal Ring"
```

Actually, I realize the ring naming is getting circular. Let me step back and just use the names directly from what the game displays. The important thing is the game_id mapping and the memory_flag. The implementer can verify ring names in-game. For the plan, I'll use:
- Location "Mine F1 - Heal Ring Chest" with vanilla="Heal Ring", memory_flag=0x5318DC (not 0x5318D8!)

Wait -- let me re-read the flag data. The user prompt says:
```
0x5318D8 | Timer Ring chest | Timer Ring (17)
0x5318DC | Heal Ring chest  | Heal Ring (18) -- wait, old ID was 17
```

And the corrections say: `17=Heal Ring (was Timer Ring)`. So chest 0x5318D8 contains the item at game_id 17, which is NOW called "Heal Ring". And chest 0x5318DC contains game_id 18 which the old table called "Heal Ring" but the NEW table says is "Evil Ring".

This doesn't make sense -- Evil Ring comes from a Tower chest, not a mine chest. Let me look at the flag guide:
- Line 371: `0x5318D8 | Chest | Timer Ring chest opened | Timer Ring (ID 17)`
- Line 372: `0x5318DC | Chest | Heal Ring chest opened | Heal Ring (ID 18)`

If ID 17 = Heal Ring (per user corrections), then 0x5318D8 gives Heal Ring.
If ID 18 = Evil Ring (per user corrections), then 0x5318DC gives Evil Ring??

That can't be right -- Evil Ring is in Tower F2 at flag 0x531904. So the Heal Ring chest at 0x5318DC must NOT be Evil Ring. This means the user's corrections may not apply to IDs 17-18 the way I interpreted them.

**Resolution**: I think what happened is:
- In the PSP version: 16=Shield Ring, 17=Timer Ring, 18=Heal Ring, 19=Evil Ring
- In the PC version the FUNCTIONAL mapping stayed the same, but some names in data tables shifted
- The user tried to correct names but introduced confusion

For safety, use the flag_mapping_guide.md names which match the PSP memory_map.md pattern:
- **16 = Shield Ring** (the existing items.py has this)
- **17 = Timer Ring** (the existing items.py has this as AP +32)
- **18 = Heal Ring** (the existing items.py has this as AP +33)  
- **19 = Evil Ring** (the existing items.py has this as AP +34)

Wait, but the existing items.py has Evil Ring at AP +34 with game classification as `trap`. And the Goddess Ring is nowhere. Let me check the user prompt one more time: `19=Goddess Ring`. And from the flag_mapping_guide line 313: `19 | Goddess Ring`.

So in the PC version: 19 = Goddess Ring (NOT Evil Ring). Evil Ring = 18.

And from PSP memory_map.md line 86: `19 | Evil Ring`. 

So the PC and PSP versions have different ID assignments for rings 16-19.

**FINAL definitive for the PC version** (from flag_mapping_guide.md):
- 16 = Timer Ring
- 17 = Heal Ring
- 18 = Evil Ring
- 19 = Goddess Ring

**What the user's corrections say**: `16=Shield Ring (was Timer Ring in old table)`. This means the user is saying ID 16 is Shield Ring, overriding the flag guide's "Timer Ring" label. But the flag guide was made BY the user from PC game testing. This is an internal naming inconsistency.

I'll just go with what makes the most functional sense for Ys I:
- The game has 5 rings: Power, Shield, Timer, Heal, Evil. Some versions also have Goddess Ring.
- IDs 15-19 = 5 ring slots

For the implementation, **use the flag_mapping_guide names at each ID** since those are from actual memory dumps. The implementer should verify by equipping each ring and checking the effect:
- 15 = Power Ring
- 16 = Timer Ring (or Shield Ring -- VERIFY)
- 17 = Heal Ring (or Timer Ring -- VERIFY)
- 18 = Evil Ring (or Heal Ring -- VERIFY)
- 19 = Goddess Ring (or Evil Ring -- VERIFY)

Now, back to the actual locations:

**Mine B1:**
```python
"Mine B1 - Heal Ring Chest":       memory_flag=0x5318DC, vanilla="Heal Ring"  # game_id 18
"Mine B1 - Silver Harmonica Chest": memory_flag=0x5318E4, vanilla="Silver Harmonica"
"Mine B1 - Roda Tree Seed Chest":  memory_flag=0x5318E0, vanilla="Roda Tree Seed"
```

**Mine B2:**
```python
"Mine B2 - Darm Key Chest":        memory_flag=0x5318EC, vanilla="Darm Key"
"Mine B2 - Heal Potion Chest":     memory_flag=0x5318E8, vanilla="Heal Potion"
"Mine B2 - Volume Dabbie Chest":   memory_flag=0x5318F0, vanilla="Book of Ys (Dabbie)"
"Boss: Nygtilger":                 memory_flag=0x531958, vanilla=None  # NEW boss
"Boss: Vagullion":                 memory_flag=0x531AC0, vanilla="Book of Ys (Dabbie)"
```

Note: Nygtilger is a boss that's missing from the current locations.py entirely. The flag guide has Nygtilger flags at 0x531958, 0x531ABC, 0x531ED0, 0x53228C. Nygtilger is in the Mine area. Need to add it.

**Tower Lower (F1-F7):**
```python
"Tower F2 - Heal Potion Chest":    memory_flag=0x5318FC, vanilla="Heal Potion"
"Tower F2 - Mirror Chest":         memory_flag=0x531900, vanilla="Mirror"
"Tower F2 - Evil Ring Chest":      memory_flag=0x531904, vanilla="Evil Ring"
"Tower F2 - Talwar Chest":         memory_flag=0x531944, vanilla="Talwar"
"Tower F4 - Reflex Chest":         memory_flag=0x531948, vanilla="Reflex"
"Tower F6 - Large Shield Chest":   memory_flag=0x53194C, vanilla="Large Shield"
"Tower F7 - Silver Sword Chest":   memory_flag=0x531914, vanilla="Silver Sword"
```

**Tower F8:**
```python
"Boss: Pictimos":                  memory_flag=0x531968, vanilla="Book of Ys (Messa)"
```

Note: The current code has a separate "Pictimos - Hammer" location. Hammer is at chest flag 0x531918 (Tower F9, NOT F8). This should be moved to Tower Mid region, NOT paired with Pictimos.

**Tower Mid (F9-F14):**
```python
"Tower F9 - Hammer Chest":         memory_flag=0x531918, vanilla="Hammer"  # RENAMED from "Pictimos - Hammer"
"Tower F9 - Volume Mesa Chest":    memory_flag=0x53191C, vanilla="Book of Ys (Messa)"  # NEW
"Tower F9 - Silver Shield Chest":  memory_flag=0x531920, vanilla="Silver Shield"
"Dogi's Gift":                     memory_flag=0x531D38, vanilla="Idol"
"Raba Trade":                      memory_flag=0x531D00, vanilla="Blue Necklace"
    # Raba has flags: 0x531D00 + 0x531D04
"Tower F13 - Silver Armor Chest":  memory_flag=0x531924, vanilla="Silver Armor"
"Reah's Gift":                     memory_flag=0x531D64, vanilla="Monocle"
"Luta Gemma's Gift":               memory_flag=0x531D58, vanilla="Blue Amulet"
    # Luta Gemma is on Tower F11, not Tower Top. Move from "Tower Top" to "Tower Mid"
    # Has flags: 0x531D58 + 0x531D5C
```

Wait -- Luta Gemma is currently in "Tower Top" at line 419. Should it be Tower Mid or Tower Top? The user's NPC flag table says "Luta Gemma gift flag 1 (Darm Tower 11F)". F11 is in the Mid range (F9-F13). Move it.

**Tower F14:**
```python
"Boss: Khonsclard":                memory_flag=0x531978, vanilla="Book of Ys (Gemma)"
"Tower F14 - Rod Chest":           memory_flag=0x531928, vanilla="Rod"  # RENAMED from "Khonsclard - Rod"
"Tower F14 - Volume Gemma Chest":  memory_flag=0x53192C, vanilla="Book of Ys (Gemma)"  # NEW
```

Note: Volume Gemma at a chest AND as Khonsclard's reward? That seems like duplication. Actually, the boss gives Book of Ys (Gemma) in vanilla, AND there's a separate chest with Volume Gemma on the same floor. These may be the same thing or different. Looking at the data: chest flag 0x53192C is "Volume Gemma chest (Tower 14F)". The boss flag is 0x531978/0x531AC8/0x532298. These are separate events. So yes, both the boss check AND a chest exist on F14.

But wait -- in vanilla Ys I, Khonsclard drops Book of Ys (Gemma). If the chest ALSO has Book of Ys (Gemma), that's strange. More likely: the chest has Volume Gemma and the boss kill is a separate check. For the randomizer, keep them as separate locations. The boss location's vanilla_item should probably be something else (or just a check). Actually, in the current code, "Boss: Khonsclard" has vanilla_item="Book of Ys (Gemma)" and "Khonsclard - Rod" has vanilla_item="Rod". These are both boss rewards in the current design. With the chest being separate, the boss only needs one location (the kill check). Change to:

```python
"Boss: Khonsclard":                memory_flag=0x531978, vanilla="Book of Ys (Gemma)"
"Tower F14 - Rod Chest":           memory_flag=0x531928, vanilla="Rod"   # chest, not boss reward
"Tower F14 - Volume Gemma Chest":  memory_flag=0x53192C, vanilla="Book of Ys (Gemma)"  # chest
```

This means Volume Gemma appears as BOTH the boss vanilla reward AND a chest. That's not right in vanilla. The boss probably gives it directly (no chest). Keep "Boss: Khonsclard" with vanilla="Book of Ys (Gemma)" and make the chest a separate check that may contain different items in rando. OR: the chest IS the boss reward and the boss flag just gates access to it.

For safety: keep the boss as one location check, and include the two chests (Rod, Volume Gemma) as separate chest locations on F14. If testing reveals duplication, remove one.

**Tower Upper (F15-F21):**
```python
"Tower F15 - Battle Shield Chest": memory_flag=0x531930, vanilla="Battle Shield"
"Tower F17 - Heal Potion Chest":   memory_flag=0x531934, vanilla="Heal Potion"  # NEW
"Tower F19 - Battle Armor Chest":  memory_flag=0x531938, vanilla="Battle Armor"
"Tower F20 - Flame Sword Chest":   memory_flag=0x53193C, vanilla="Flame Sword"
"Boss: Yogleks & Omulgun":         memory_flag=0x531ACC, vanilla=None  # NEW boss, F21
```

**Tower Top (F22-F25):**
```python
"Boss: Dark Fact":                 memory_flag=0x531A98, vanilla="Book of Ys (Fact)"
```

Note: Luta Gemma moved to Tower Mid. "Volume Fact" is obtained from Dark Fact's cape inspection (flag 0x531D94), which could be a separate event check or folded into the boss. For simplicity, fold it into the Dark Fact boss check.

#### C. Add Missing Locations Summary

New locations to add:
1. **"Franz's Gift"** -- NPC, Minea, Volume Tovah, flag=0x531BC0
2. **"Tower F9 - Hammer Chest"** -- chest, Tower Mid, Hammer, flag=0x531918
3. **"Tower F9 - Volume Mesa Chest"** -- chest, Tower Mid, Book of Ys (Messa), flag=0x53191C
4. **"Tower F14 - Rod Chest"** -- chest, Tower F14, Rod, flag=0x531928
5. **"Tower F14 - Volume Gemma Chest"** -- chest, Tower F14, Book of Ys (Gemma), flag=0x53192C
6. **"Tower F17 - Heal Potion Chest"** -- chest, Tower Upper, Heal Potion, flag=0x531934
7. **"Boss: Nygtilger"** -- boss, Mine B2 (or Mine B1), flag=0x531958
8. **"Boss: Yogleks & Omulgun"** -- boss, Tower Upper, flag=0x531ACC

Locations to rename:
- "Sara's Second Gift" -> "Franz's Gift" (flag=0x531BC0)
- "Pictimos - Hammer" -> "Tower F9 - Hammer Chest" (move from Tower F8 to Tower Mid, change type from BOSS to CHEST)
- "Khonsclard - Rod" -> "Tower F14 - Rod Chest" (change type from BOSS to CHEST)
- "Silver Bells Reward" -> "Silver Bell Reward"
- "Mine F1 - Timer Ring Chest" -> "Mine F1 - Heal Ring Chest" (or keep the name, fix vanilla_item)

Locations to move:
- "Luta Gemma's Gift" from "Tower Top" to "Tower Mid" (she's on F11)

#### D. Location Code Numbering

Keep the existing base ID `0x59530000`. Assign new location codes for new locations by continuing the numbering. Avoid reusing existing codes. Suggested:
- Tower F9 - Hammer Chest: +62 (reuse old Pictimos-Hammer if keeping same concept)
- Tower F9 - Volume Mesa Chest: +63
- Tower F14 - Rod Chest: +72 (reuse old Khonsclard-Rod)
- Tower F14 - Volume Gemma Chest: +73
- Tower F17 - Heal Potion Chest: +78
- Boss: Nygtilger: +48
- Boss: Yogleks & Omulgun: +79
- Franz's Gift: +31

---

## 4. regions.py Update

**File**: `/Users/luis/ys-archipelago/apworld/ys_chronicles/regions.py`

### Changes Required

#### A. Fix Mine Access Rule

Line 63: `("Minea Fields", "Mine F1", "has_prison_key")` -- This is WRONG. The Mine is not gated by the Prison Key. The Abandoned Mine is accessed from the overworld. The Prison Key is used to free Feena in Shrine B2.

Looking at vanilla Ys I progression:
1. Get Shrine Key from Jeba -> Enter Shrine
2. Find Prison Key in Shrine B2 -> Free Feena in Shrine B3
3. After freeing Feena, the Mine becomes accessible (it's gated by Feena being rescued, NOT by having Prison Key)
4. The flag for Feena rescue is 0x531C44

The rule should be: Mine access requires Feena to be rescued. In a randomizer context, the check should be that the player has completed certain prerequisites. However, in the current AP design, the client doesn't control NPC flags -- the game does. So the AP logic just needs to know: what items does the player need to have reached the Mine?

In vanilla: you need Shrine Key to enter Shrine, Prison Key to free Feena (found in Shrine), and freeing Feena opens the Mine. So the logic chain is: Shrine Key -> access Shrine -> find Prison Key -> free Feena -> Mine opens.

For AP logic: `Mine F1` should require: `has_shrine_key AND has_prison_key`. Or more accurately, the player needs to be able to reach Shrine B2 (where Prison Key is found) and then use it to free Feena in Shrine B3.

But wait -- in a rando, Prison Key could be anywhere. The RULE for Mine access should be: the player has freed Feena, which requires having Prison Key and being able to reach Shrine B3 (which requires Ivory Key and reaching Shrine B2).

Simplification for AP: `has_prison_key` as the rule is actually reasonable as a proxy (you need the key to free Feena). But the access to the MINE doesn't require Prison Key directly -- it requires Feena being freed. In the rando, we can't guarantee what order things happen. The safest rule: require Prison Key (since freeing Feena requires it).

Actually, looking further: does the Mine require Feena to be rescued, or is it always accessible? In vanilla Ys I, the Mine entrance is accessible from the plains but you need the Mask of Eyes to see the path, or you need to have talked to certain NPCs. Let me reconsider.

In Ys I: The Abandoned Mine is physically accessible from the overworld (Minea Fields area). There may be no gate at all. The Mask of Eyes is needed for hidden walls in the Shrine, not the Mine. Let me check if there's actually any gate.

Looking at the flag data, there's no door flag for the Mine entrance. The Mine appears to be freely accessible from the overworld. The connection rule should be `None` (no requirements):

```python
("Minea Fields", "Mine F1", None),  # Mine is freely accessible
```

This is a significant change from the current code. If the Mine IS gated by something (like Feena rescue), the rule should be updated accordingly. The implementer should verify by testing: can you walk into the Mine without freeing Feena?

#### B. Fix Shrine B2 -> B3 Connection

Line 60: `("Shrine B2", "Shrine B3", "has_ivory_key")` -- Shrine B3 is deeper in the shrine. Access may require Ivory Key OR Marble Key depending on which specific door. Looking at the door flags:
- 0x531A94 = Shrine Key / Marble Key door
- 0x531AE0 = Ivory Key door

The path Shrine B2 -> Shrine B3 likely requires Ivory Key. But getting to the Jenocres boss requires Marble Key AND Mask of Eyes. This is currently handled by LOCATION_RULES, not region connections. Keep the connection rule as `has_ivory_key` and use LOCATION_RULES for boss access within B3.

#### C. Add Missing Rules

Add rule functions:
```python
def has_silver_harmonica(state, player):
    return state.has("Silver Harmonica", player)

def has_blue_necklace(state, player):
    return state.has("Blue Necklace", player)
```

Add location rules:
```python
# Franz gives Tovah after you bring Hadal and Sara's Crystal
"Franz's Gift": "has_book_hadal",

# Reah's gift requires Silver Harmonica
"Reah's Gift": "has_silver_harmonica",  # NEW

# Tower F19 Battle Armor chest requires Blue Necklace equipped
"Tower F19 - Battle Armor Chest": "has_blue_necklace",  # NEW

# Feena rescue requires Prison Key (this is an event, not a location, but if we track it)
```

#### D. Fix Tower Progression

The current progression is:
```
Tower Lower -> Tower F8 (free)
Tower F8 -> Tower Mid (has_hammer)
Tower Mid -> Tower F14 (has_monocle)  
Tower F14 -> Tower Upper (has_rod)
Tower Upper -> Tower Top (has_all_books)
```

Verify: Does Hammer come from Pictimos (Tower F8 boss) or from a chest on F9? Looking at the chest data: flag 0x531918 = "Hammer chest opened (Darm Tower 9F)". So Hammer is in a CHEST on F9, not dropped by Pictimos. Pictimos blocks access to F9+ (you must defeat him to proceed). So:
- Defeating Pictimos is required to reach Tower Mid (F9+)
- Hammer is found in a chest on F9 (part of Tower Mid)
- Hammer is needed to break walls to proceed further within Tower Mid

The current rule `("Tower F8", "Tower Mid", "has_hammer")` is wrong -- you don't need Hammer to ENTER F9, you need to have defeated Pictimos. Hammer is on F9.

Fixed:
```python
("Tower F8", "Tower Mid", None),  # Beating Pictimos opens the path; boss check handles this
# OR if boss_checks enabled, defeating Pictimos IS the gate. But AP regions don't work that way.
```

Actually, in AP: region connections should reflect physical access requirements. You physically need to defeat Pictimos to get past F8. But that's not an ITEM check -- it's a boss check. In AP logic, if boss_checks are enabled, the boss kill is a location that sends a check. But the boss itself isn't gated by items.

The standard AP approach: make the connection free, and the boss location has its own requirements. The ITEMS that come from the boss are what gates further progress. Since Hammer comes from a chest on F9 (not the boss), and you need Hammer to break walls later in the tower:

```python
("Tower F8", "Tower Mid", None),         # Defeating Pictimos lets you through
("Tower Mid", "Tower F14", "has_hammer_and_monocle"),  # Need Hammer to break walls, Monocle for hidden paths
```

Or split:
```python
("Tower F8", "Tower Mid", None),
("Tower Mid", "Tower F14", "has_hammer"),  # Hammer to break walls to reach higher floors
```

And Monocle for some specific locations within Mid:
```python
# Monocle needed to see hidden paths to reach certain chests
"Reah's Gift": "has_silver_harmonica",  # Reah is in Rado's Annex which is accessed from Tower
```

The exact tower progression rules need in-game verification. The implementer should map out which floors require which items. For the plan:

```python
("Tower Lower", "Tower F8", None),       # F8 is Pictimos's floor, always reachable from F1
("Tower F8", "Tower Mid", None),          # After Pictimos, proceed upward
("Tower Mid", "Tower F14", "has_hammer"), # Hammer needed to break walls to reach F14
("Tower F14", "Tower Upper", "has_rod"),  # Rod to activate statues
("Tower Upper", "Tower Top", "has_all_books"),
```

#### E. Update BOSS_LOCATIONS and BOSS_ITEMS

Add Nygtilger and Yogleks & Omulgun:
```python
BOSS_LOCATIONS = [
    "Boss: Jenocres",
    "Boss: Nygtilger",      # NEW
    "Boss: Vagullion",
    "Boss: Pictimos",
    "Boss: Khonsclard",
    "Boss: Yogleks & Omulgun",  # NEW
    "Boss: Dark Fact",
]
```

Remove "Pictimos - Hammer" and "Khonsclard - Rod" from BOSS_LOCATIONS since they're now chest locations.

Update BOSS_ITEMS -- remove Hammer and Rod (they're chest items, not boss rewards). Books stay:
```python
BOSS_ITEMS = [
    "Book of Ys (Hadal)",    # Jenocres
    "Book of Ys (Dabbie)",   # Vagullion
    "Book of Ys (Messa)",    # Pictimos
    "Book of Ys (Gemma)",    # Khonsclard
    "Book of Ys (Fact)",     # Dark Fact
]
```

#### F. Fix `has_roda_tree_access` rule

Line 146-150: Uses `"Roda Seed"` but the item is renamed to `"Roda Tree Seed"`:
```python
def has_roda_tree_access(state, player):
    return (
        state.has("Roda Tree Seed", player) and
        state.has("Silver Harmonica", player)
    )
```

Also fix `has_silver_bells` -> use `"Silver Bell"` (singular):
```python
def has_silver_bell(state, player):
    return state.has("Silver Bell", player)
```

---

## 5. client.py Rewrite

**File**: `/Users/luis/ys-archipelago/apworld/ys_chronicles/client.py`

This is the biggest change. The entire PPSSPP WebSocket interface must be replaced with a subprocess-based Wine/mem_read.exe/mem_write.exe interface for macOS, with a ctypes/ReadProcessMemory path for Windows.

### Architecture

```
YsChroniclesContext (AP CommonContext)
  |
  +-- MemoryInterface (abstract)
  |     |
  |     +-- WineMemoryInterface (macOS/CrossOver)
  |     |     Uses: subprocess.run(WINE, mem_read.exe, ...)
  |     |
  |     +-- WindowsMemoryInterface (native Windows)
  |           Uses: ctypes.windll.kernel32.ReadProcessMemory
  |
  +-- YsGameState
        Polls memory via MemoryInterface
        Tracks items, chests, bosses, NPCs
        Handles give/remove items
        Detects save/load
```

### Step-by-Step Implementation

#### A. PC Memory Addresses Class

Replace `Ys1Addresses` entirely. No PSP_RAM_BASE. All addresses are absolute:

```python
class PCAddresses:
    """Absolute memory addresses for ys1plus.exe (PC version)."""
    
    # Player stats
    HP          = 0x5317FC
    MAX_HP      = 0x531800
    STR         = 0x531804
    DEF         = 0x531808
    GOLD        = 0x53180C
    EXP         = 0x531810
    LEVEL       = 0x531814
    
    # Item arrays
    ITEM_ARRAY_1_BASE = 0x531990   # Ownership, DWORD[60]
    ITEM_ARRAY_2_BASE = 0x53207C   # Visibility, DWORD[60]
    ITEM_COUNT = 52
    
    @classmethod
    def item_array1_addr(cls, game_id: int) -> int:
        return cls.ITEM_ARRAY_1_BASE + game_id * 4
    
    @classmethod
    def item_array2_addr(cls, game_id: int) -> int:
        return cls.ITEM_ARRAY_2_BASE + game_id * 4
```

#### B. MemoryInterface Abstract Class

```python
import abc

class MemoryInterface(abc.ABC):
    @abc.abstractmethod
    async def connect(self) -> bool: ...
    
    @abc.abstractmethod
    async def read_u32(self, address: int) -> int: ...
    
    @abc.abstractmethod
    async def write_u32(self, address: int, value: int) -> bool: ...
    
    @abc.abstractmethod
    async def read_bytes(self, address: int, size: int) -> bytes: ...
    
    @abc.abstractmethod
    async def is_connected(self) -> bool: ...
```

#### C. WineMemoryInterface (macOS/CrossOver)

```python
class WineMemoryInterface(MemoryInterface):
    """Memory access via Wine + mem_read.exe/mem_write.exe subprocess calls."""
    
    WINE_PATH = "/Users/luis/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/lib/wine/x86_64-unix/wine"
    WINEPREFIX = "/Users/luis/Library/Application Support/CrossOver/Bottles/Steam"
    MEM_READ = "C:\\mem_read.exe"
    MEM_WRITE = "C:\\mem_write.exe"
    
    async def read_u32(self, address: int) -> int:
        """Run mem_read.exe via Wine, parse output."""
        result = await asyncio.create_subprocess_exec(
            self.WINE_PATH, self.MEM_READ, f"0x{address:X}",
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            env={
                "WINEPREFIX": self.WINEPREFIX,
                "WINEDEBUG": "-all",
                "WINEMSYNC": "1",
            }
        )
        stdout, _ = await result.communicate()
        output = stdout.decode().replace('\r', '')
        # Parse output: "0x00531990: 1" -> extract value
        for line in output.strip().split('\n'):
            if ':' in line:
                value_str = line.split(':')[-1].strip()
                return int(value_str)
        raise RuntimeError(f"Failed to read 0x{address:X}: {output}")
    
    async def write_u32(self, address: int, value: int) -> bool:
        """Run mem_write.exe via Wine."""
        result = await asyncio.create_subprocess_exec(
            self.WINE_PATH, self.MEM_WRITE, f"0x{address:X}", str(value),
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            env={
                "WINEPREFIX": self.WINEPREFIX,
                "WINEDEBUG": "-all",
                "WINEMSYNC": "1",
            }
        )
        await result.communicate()
        return result.returncode == 0
    
    async def read_bytes(self, address: int, size: int) -> bytes:
        """Read multiple DWORDs by calling mem_read.exe in a batch."""
        # Option 1: Call mem_read.exe once per DWORD (slow but simple)
        # Option 2: Use mem_full_snapshot.exe for bulk reads
        # For polling, read individual flags rather than bulk scanning
        # Implement batch read using mem_read.exe with range syntax if supported
        ...
```

**Critical design decision for polling efficiency**: Individual subprocess calls are expensive (~50ms each on macOS with Wine). For 40+ chest flags, that's 2+ seconds per poll. Solutions:

1. **Batch reads**: Modify mem_read.exe to accept multiple addresses, or read a range. If mem_read.exe supports `mem_read.exe 0x531894 160` (address + size), we can read all chest flags in one call.

2. **Snapshot diff**: Use mem_full_snapshot.exe to take a snapshot, then diff against previous. This gives all changes in one call.

3. **Tiered polling**: Read only the addresses that matter at different frequencies:
   - HP (for death link): every 0.5s -- single read
   - Item arrays (for save/load detection): every 2s -- batch read of 208 bytes
   - Chest/NPC/boss flags: every 1s -- batch read of relevant ranges

The implementer should check whether mem_read.exe supports range reads. If so, batch reading is the best approach.

#### D. WindowsMemoryInterface (native Windows)

```python
class WindowsMemoryInterface(MemoryInterface):
    """Direct memory access on Windows using ctypes."""
    
    def __init__(self):
        import ctypes
        import ctypes.wintypes
        self.kernel32 = ctypes.windll.kernel32
        self.process_handle = None
    
    async def connect(self) -> bool:
        """Find ys1plus.exe process and open handle."""
        # Use EnumProcesses or CreateToolhelp32Snapshot to find PID
        # OpenProcess with PROCESS_VM_READ | PROCESS_VM_WRITE
        ...
    
    async def read_u32(self, address: int) -> int:
        buf = ctypes.c_uint32()
        bytes_read = ctypes.c_size_t()
        self.kernel32.ReadProcessMemory(
            self.process_handle, address, ctypes.byref(buf), 4, ctypes.byref(bytes_read)
        )
        return buf.value
```

#### E. Platform Detection

```python
import platform

def create_memory_interface() -> MemoryInterface:
    if platform.system() == "Darwin":
        return WineMemoryInterface()
    elif platform.system() == "Windows":
        return WindowsMemoryInterface()
    else:
        raise RuntimeError(f"Unsupported platform: {platform.system()}")
```

#### F. AP-to-Game ID Mapping

Replace the old `AP_TO_GAME_ITEM` dict. Import from items.py:

```python
from .items import YS1_ITEMS, YS1_BASE_ID

# Build mapping at module level
AP_CODE_TO_GAME_ID: Dict[int, int] = {
    data.code: data.game_id for data in YS1_ITEMS.values()
}
GAME_ID_TO_AP_CODE: Dict[int, int] = {
    data.game_id: data.code for data in YS1_ITEMS.values()
}
```

#### G. Location Flag Polling

Replace the old chest/boss polling with flag-based polling. Import from locations.py:

```python
from .locations import YS1_LOCATIONS, YsLocationType

# Build flag -> AP location code mapping
FLAG_TO_LOCATION: Dict[int, int] = {}
REVERSE_FLAGS: Set[int] = set()  # Flags that go 1->0 instead of 0->1

for loc_name, loc_data in YS1_LOCATIONS.items():
    if loc_data.memory_flag:
        FLAG_TO_LOCATION[loc_data.memory_flag] = loc_data.code

# Southern Roda Tree uses reverse flag
REVERSE_FLAGS.add(0x531C30)
```

The polling loop reads all known flag addresses and detects transitions:

```python
async def poll_locations(self) -> Set[int]:
    """Check all location flags for new checks."""
    new_checks = set()
    
    for flag_addr, ap_loc_code in FLAG_TO_LOCATION.items():
        if ap_loc_code in self.checked_locations:
            continue
        
        value = await self.mem.read_u32(flag_addr)
        
        if flag_addr in REVERSE_FLAGS:
            # Reverse flag: 1->0 means checked
            if value == 0 and self.prev_flags.get(flag_addr, 1) == 1:
                new_checks.add(ap_loc_code)
                self.checked_locations.add(ap_loc_code)
        else:
            # Normal flag: 0->1 means checked
            if value == 1 and self.prev_flags.get(flag_addr, 0) == 0:
                new_checks.add(ap_loc_code)
                self.checked_locations.add(ap_loc_code)
        
        self.prev_flags[flag_addr] = value
    
    return new_checks
```

**Optimization**: Instead of reading each flag individually (expensive with subprocess), group flags into contiguous memory ranges and read in bulk:

```python
# Chest flags are mostly in 0x531894-0x53194C range (contiguous-ish)
# Read 0x531894 to 0x53194C as one 184-byte chunk
CHEST_FLAG_RANGE = (0x531894, 0x53194C + 4)  # start, end

# NPC flags are scattered: 0x531B20, 0x531BAC, 0x531BC0, etc.
# Group into ranges where possible

# Boss flags are scattered: 0x531950, 0x531958, 0x531968, etc.
```

Define flag groups for batch reading:
```python
FLAG_RANGES = [
    (0x531894, 192),  # Chest flags: 0x531894 to ~0x531954 (covers all chest flags)
    (0x531958, 4),    # Nygtilger flag
    (0x531968, 4),    # Pictimos flag
    (0x531978, 4),    # Khonsclard flag
    (0x531A94, 8),    # Door flags
    (0x531A98, 4),    # Dark Fact flag
    (0x531AC0, 20),   # Boss flags cluster: 0x531AC0-0x531AD0
    (0x531ACC, 4),    # Yogleks flag
    (0x531B20, 4),    # Slaff gift 1
    (0x531B2C, 4),    # Slaff gift 2
    (0x531BAC, 12),   # Sara flags (0x531BAC, 0x531BB4)
    (0x531BC0, 8),    # Franz flags
    (0x531C28, 4),    # Golden Vase pickup
    (0x531C30, 4),    # SE Roda Tree (reverse)
    (0x531C6C, 4),    # Jeba flag
    (0x531CAC, 4),    # Robels flag
    (0x531D00, 8),    # Raba flags
    (0x531D38, 4),    # Dogi flag
    (0x531D58, 8),    # Luta Gemma flags
    (0x531D64, 4),    # Reah flag
]
```

If mem_read.exe supports range reads, consolidate into fewer calls. If not, prioritize the most important ranges and spread reads across multiple poll cycles.

#### H. Item Giving and Removal

```python
async def give_item(self, game_id: int) -> bool:
    """Give an item to the player. Sets both Array 1 and Array 2."""
    addr1 = PCAddresses.item_array1_addr(game_id)
    addr2 = PCAddresses.item_array2_addr(game_id)
    
    ok1 = await self.mem.write_u32(addr1, 1)
    ok2 = await self.mem.write_u32(addr2, 1)
    
    return ok1 and ok2

async def remove_item(self, game_id: int) -> bool:
    """Remove an item. MUST zero Array 2 FIRST, then Array 1."""
    addr1 = PCAddresses.item_array1_addr(game_id)
    addr2 = PCAddresses.item_array2_addr(game_id)
    
    # CRITICAL ORDER: Array 2 first, then Array 1
    ok2 = await self.mem.write_u32(addr2, 0)
    ok1 = await self.mem.write_u32(addr1, 0)
    
    return ok1 and ok2
```

#### I. Death Link

```python
async def check_death(self) -> bool:
    """Check if player HP reached 0 (death)."""
    hp = await self.mem.read_u32(PCAddresses.HP)
    if hp == 0 and self.last_hp > 0:
        self.last_hp = hp
        return True  # Player died
    self.last_hp = hp
    return False

async def kill_player(self):
    """Kill the player for death link. Set HP to 1 (NOT 0 -- let game handle death)."""
    # Setting HP to 0 causes instant game over. Setting HP to 1 means the next
    # hit will kill them, which is safer. Or set HP to 0 if that's confirmed safe.
    await self.mem.write_u32(PCAddresses.HP, 0)
```

Wait -- the constraint says "Never set HP to 0 causes instant game over". Actually the constraint is "Never set HP to 0" because it causes an instant game over. For death link, we DO want to cause a game over. So setting HP to 0 is correct for death link. But the user says it causes "instant game over" which might mean the game over screen bypassing normal death animation. Test this.

Better approach: set HP to 1 and let the player die naturally from any damage. But if they're in a safe area, they won't die. For a true death link: set HP to 0.

#### J. Save/Load Detection

Same approach as current code but using PC addresses:

```python
async def check_save_load(self) -> bool:
    """Detect save load by checking if AP-given items disappeared."""
    if not self.ap_received_items:
        return False
    
    # Read a few AP items to check
    for game_id in list(self.ap_received_items)[:5]:  # Sample check
        addr = PCAddresses.item_array1_addr(game_id)
        value = await self.mem.read_u32(addr)
        if value == 0:
            # AP item missing -> save was loaded
            await self.reapply_all_ap_items()
            return True
    
    return False
```

#### K. Goal Completion Check

```python
async def check_goal(self) -> bool:
    goal = self.slot_data.get("goal", 0)
    
    if goal == 0:  # Dark Fact
        dark_fact_flag = await self.mem.read_u32(0x531A98)
        return dark_fact_flag == 1
    
    elif goal == 1:  # All Books
        dark_fact_flag = await self.mem.read_u32(0x531A98)
        if dark_fact_flag != 1:
            return False
        # Check all 6 books in Array 1
        for book_id in range(20, 26):
            val = await self.mem.read_u32(PCAddresses.item_array1_addr(book_id))
            if val == 0:
                return False
        return True
    
    elif goal == 2:  # All Bosses
        boss_flags = [0x531950, 0x531958, 0x531AC0, 0x531968, 0x531978, 0x531ACC, 0x531A98]
        for flag in boss_flags:
            val = await self.mem.read_u32(flag)
            if val == 0:
                return False
        return True
```

#### L. Experience/Gold Multiplier

The client can apply multipliers by hooking EXP/Gold gains:

```python
async def apply_multipliers(self):
    """Check for EXP/Gold changes and apply multipliers."""
    exp = await self.mem.read_u32(PCAddresses.EXP)
    gold = await self.mem.read_u32(PCAddresses.GOLD)
    
    exp_mult = self.slot_data.get("experience_multiplier", 100) / 100.0
    gold_mult = self.slot_data.get("gold_multiplier", 100) / 100.0
    
    if exp_mult != 1.0 and exp != self.last_exp:
        diff = exp - self.last_exp
        if diff > 0:
            adjusted = self.last_exp + int(diff * exp_mult)
            await self.mem.write_u32(PCAddresses.EXP, adjusted)
            exp = adjusted
    
    # Same for gold
    ...
    
    self.last_exp = exp
    self.last_gold = gold
```

#### M. Main Poll Loop Structure

```python
async def game_poll_loop(self):
    while not self.exit_event.is_set():
        try:
            if not self.mem_connected:
                if await self.mem.connect():
                    self.mem_connected = True
                    await self.initialize_state()
                else:
                    await asyncio.sleep(5)
                    continue
            
            if not await self.is_in_game():
                await asyncio.sleep(1)
                continue
            
            # 1. Check for save/load
            await self.check_save_load()
            
            # 2. Poll locations
            new_checks = await self.poll_locations()
            if new_checks:
                await self.send_msgs([{
                    "cmd": "LocationChecks",
                    "locations": list(new_checks),
                }])
            
            # 3. Apply multipliers
            await self.apply_multipliers()
            
            # 4. Death link
            if self.slot_data.get("death_link", False):
                if await self.check_death():
                    await self.send_death_link()
            
            # 5. Goal check
            if not self.finished_game and await self.check_goal():
                await self.send_msgs([{
                    "cmd": "StatusUpdate",
                    "status": ClientStatus.CLIENT_GOAL,
                }])
                self.finished_game = True
            
        except Exception as e:
            print(f"Poll error: {e}")
            self.mem_connected = False
        
        await asyncio.sleep(POLL_INTERVAL)
```

#### N. Configuration File

The Wine path, prefix, and tool paths should be configurable. Create a config approach:

```python
import os
import json

DEFAULT_CONFIG = {
    "wine_path": "/Users/luis/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/lib/wine/x86_64-unix/wine",
    "wineprefix": "/Users/luis/Library/Application Support/CrossOver/Bottles/Steam",
    "mem_read": "C:\\mem_read.exe",
    "mem_write": "C:\\mem_write.exe",
}

def load_config():
    config_path = os.path.join(os.path.dirname(__file__), "config.json")
    if os.path.exists(config_path):
        with open(config_path) as f:
            return {**DEFAULT_CONFIG, **json.load(f)}
    # Also check environment variables
    return {
        "wine_path": os.environ.get("YS_WINE_PATH", DEFAULT_CONFIG["wine_path"]),
        "wineprefix": os.environ.get("WINEPREFIX", DEFAULT_CONFIG["wineprefix"]),
        "mem_read": os.environ.get("YS_MEM_READ", DEFAULT_CONFIG["mem_read"]),
        "mem_write": os.environ.get("YS_MEM_WRITE", DEFAULT_CONFIG["mem_write"]),
    }
```

---

## 6. __init__.py Update

**File**: `/Users/luis/ys-archipelago/apworld/ys_chronicles/__init__.py`

### Changes Required

#### A. Fix Item Name References

Line 252: `"Silver Bells Reward"` -> `"Silver Bell Reward"` (if location renamed)
Line 254: `lambda item: item.name != "Silver Bells"` -> `"Silver Bell"`

Line 253-254 reference a location called "Silver Bells Reward" which must match the locations.py name exactly.

#### B. Fix `set_rules` for Renamed Locations

Update all location name references to match the new names in locations.py. For example:
- If "Pictimos - Hammer" becomes "Tower F9 - Hammer Chest", update any reference.
- If "Khonsclard - Rod" becomes "Tower F14 - Rod Chest", update any reference.

#### C. Update BOSS_ITEMS/BOSS_LOCATIONS References

Since Hammer and Rod are now chest items (not boss items), update the boss_checks logic. When boss_checks are disabled, only precollect the BOOKS (which are boss rewards), not Hammer/Rod.

#### D. Fix Location Name in `set_rules`

Line 247-250: The locked chest self-locking prevention references "Shrine B1 - Locked Chest 1" and "Shrine B1 - Locked Chest 2". These names must match locations.py exactly.

#### E. Update `fill_slot_data`

Add more data the client needs:
```python
def fill_slot_data(self) -> Dict[str, Any]:
    return {
        "goal": self.options.goal.value,
        "boss_checks": bool(self.options.boss_checks),
        "death_link": self.options.death_link.value,
        "experience_multiplier": self.options.experience_multiplier.value,
        "gold_multiplier": self.options.gold_multiplier.value,
        "shuffle_equipment": bool(self.options.shuffle_equipment),
        "shuffle_keys": bool(self.options.shuffle_keys),
    }
```

#### F. Handle New Items in `create_items`

The filler pool now includes Bestiary Potion, Piece of Paper, Wing, Sapphire Ring, etc. Make sure the item type filtering includes them. Golden Vase and Necklace should use CONSUMABLE type (after fixing items.py).

---

## 7. Testing Strategy

### Phase 1: Generation Testing (No Game Required)

1. **Test world generation**: Run AP generation with the updated world to verify:
   - All items can be placed
   - No circular dependencies
   - All locations are reachable
   - Completion conditions are satisfiable

```python
# From AP root directory:
python -m pytest worlds/ys_chronicles/ -v
# Or use the AP test framework
```

2. **Test all option combinations**: Generate with each goal type, with/without boss_checks, with/without each shuffle toggle.

3. **Test specific edge cases**:
   - Treasure Box Key cannot be placed in locked chests
   - Silver Bell cannot be placed at Silver Bell Reward
   - All progression items are reachable

### Phase 2: Client Memory Interface Testing

1. **Test WineMemoryInterface standalone**:
   - Launch game, connect, read HP/Gold/Level
   - Verify values match game screen
   - Write a known value (give Short Sword), verify in-game

2. **Test item giving/removal**:
   - Give each item type, verify it appears in inventory
   - Remove items, verify removal order (Array 2 first)
   - Test consumable behavior (give Heal Potion, use it, re-give)

3. **Test flag polling**:
   - Open a chest, verify the flag address reads 1
   - Talk to NPC, verify flag transition detected
   - Kill a boss, verify boss flag detected

### Phase 3: Integration Testing

1. **Connect to AP server**:
   - Start multiworld, connect client
   - Verify connection and slot data received

2. **Test location checks**:
   - Open a chest -> verify LocationChecks sent to server
   - Defeat a boss -> verify check sent
   - Talk to NPC -> verify check sent

3. **Test item receiving**:
   - Receive item from another player -> verify it appears in-game
   - Receive multiple items rapidly -> verify all applied

4. **Test save/load**:
   - Receive items, save game, load earlier save
   - Verify AP items are re-applied
   - Verify checked locations not re-sent

5. **Test death link**:
   - Die in-game -> verify death sent to server
   - Receive death from server -> verify player dies

6. **Test goal completion**:
   - Complete each goal type -> verify CLIENT_GOAL sent

### Phase 4: Full Playthrough

1. Play through a randomized seed from start to finish
2. Verify all locations checkable
3. Verify no soft-locks
4. Verify all items receivable
5. Test Darm Tower silver set confiscation (flags 0x531D08, 0x531D0C, 0x531D2C) -- the client may need to re-apply silver equipment after this cutscene

---

## Implementation Order

1. **items.py** -- Fix all IDs, names, types. Add game_id field. This is the foundation.
2. **locations.py** -- Add all memory_flags, add missing locations, fix names.
3. **regions.py** -- Fix rules, add missing rules, update boss lists.
4. **__init__.py** -- Update references to match new names.
5. **Test generation** -- Run AP test suite to verify world generates correctly.
6. **client.py** -- Full rewrite with MemoryInterface abstraction.
7. **Test client** -- Manual testing against live game.
8. **Integration test** -- Full AP server + client test.

---

### Critical Files for Implementation
- `/Users/luis/ys-archipelago/apworld/ys_chronicles/client.py` - Complete rewrite needed: replace PPSSPP WebSocket with Wine/subprocess memory interface, wire up all PC flag addresses for polling, implement give/remove items with correct Array 1/Array 2 ordering
- `/Users/luis/ys-archipelago/apworld/ys_chronicles/items.py` - Fix all game_id mappings, add missing items (Goddess Ring, Wing, Bestiary Potion, Piece of Paper, Sapphire Ring), rename items, remove nonexistent TREASURE type
- `/Users/luis/ys-archipelago/apworld/ys_chronicles/locations.py` - Add verified memory_flag addresses from flag_mapping_guide.md to all locations, add 8 missing locations (Franz, Nygtilger, Yogleks, Tower chests)
- `/Users/luis/ys-archipelago/apworld/ys_chronicles/regions.py` - Fix Mine access rule (not gated by Prison Key), fix tower progression rules, update boss lists, fix renamed item references (Roda Tree Seed, Silver Bell)
- `/Users/luis/ys-archipelago/docs/flag_mapping_guide.md` - Authoritative reference for all PC memory addresses and flag mappings; do not modify, but consult constantly during implementation
