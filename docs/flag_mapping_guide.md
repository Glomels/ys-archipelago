# Flag Mapping Guide — Ys I Chronicles (PC)

How to discover and verify memory flags for items, chests, NPCs, and field pickups in ys1plus.exe.

## Prerequisites

### Environment Setup
```bash
export WINEPREFIX="/Users/luis/Library/Application Support/CrossOver/Bottles/Steam"
export WINEDEBUG=-all
export WINEMSYNC=1
WINE="/Users/luis/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/lib/wine/x86_64-unix/wine"
```

### Tools (all cross-compiled Windows .exe, run via Wine)
| Tool | Location | Purpose |
|------|----------|---------|
| `mem_read.exe` | `C:\mem_read.exe` | Read memory safely (no writes) |
| `mem_write.exe` | `C:\mem_write.exe` | Write to any address or named stat |
| `mem_full_snapshot.exe` | `C:\mem_full_snapshot.exe` | Save/diff ALL writable memory regions |
| `mem_items2.exe` | `C:\mem_items2.exe` | Dump both item ownership arrays |

### Helper functions
```bash
# Run wine command, strip \r from Windows output
wrun() { "$WINE" "$@" 2>&1 | tr -d '\r'; }

# Read a single DWORD at hex address
rmem() { wrun "C:\\mem_read.exe" "$1" | tail -1 | awk -F': ' '{print $2}'; }

# Write a DWORD value to hex address
wmem() { wrun "C:\\mem_write.exe" "$1" "$2" | tail -1; }

# Read a named stat (hp, maxhp, str, def, gold, exp, level)
rstat() { wrun "C:\\mem_read.exe" "$1" | tail -1 | awk -F': ' '{print $2}'; }
```

---

## Memory Layout Overview

```
0x5317FC  Player stats (HP, MaxHP, STR, DEF, Gold, EXP, Level)
0x531834  Equipment flags (tentative)
0x53184C  Chest/pickup opened flags
0x531990  Item Array 1 — ownership (DWORD[60])
0x531A80  NPC gift/event flags region
0x53207C  Item Array 2 — visibility (DWORD[60])
0x532074  Play timer
```

---

## The Snapshot/Diff Method (Primary Technique)

This is the most reliable way to find ANY flag. It captures all writable memory before an event, then diffs after.

### Step 1: Save a snapshot BEFORE the event
```bash
wrun "C:\\mem_full_snapshot.exe" save "C:\\pre_next.bin"
```
This saves every committed writable memory page in the process to a binary file.

### Step 2: Trigger the in-game event
- Open a chest
- Talk to an NPC and receive a gift
- Pick up a field item
- Defeat a boss
- Whatever you're trying to map

### Step 3: Diff current memory against the snapshot
```bash
wrun "C:\\mem_full_snapshot.exe" diff "C:\\pre_next.bin"
```

### Step 4: Interpret the output
The diff tool outputs lines like:
```
FLAG 0x00531894: 0 -> 1        ← A flag flipped from 0 to 1
FLAG 0x00531A58: 0 -> 1        ← Another flag
     0x0053180C: 1200 -> 1250  ← Non-flag change (e.g., gold increased)
     0x00532074: 48820 -> 48840 ← Timer (noise)
```

- **`FLAG` prefix** = value changed between 0 and 1 (most interesting)
- **No prefix** = non-boolean change (stat updates, timers, etc.)
- The tool already filters known noisy regions (RNG, rendering, tile data)

### Step 5: Filter noise
Some addresses change every time regardless of what you do. Known noisy addresses to ignore:
- `0x532074` — play timer (increments constantly)
- `0x51AFA0–0x51B2F0` — rendering data
- `0x527D00–0x529600` — tile/sprite data
- `0x5DB9E0–0x5DC3B0` — RNG
- `0x608000–0x60C000` — more rendering
- `0x633000–0x635000` — misc counters

When running through `command_watcher.sh`, additional grep filters are applied:
```bash
grep "^FLAG" | grep -v "0x0022\|0x0164\|0x0165\|0x0166\|0x0167\|0x0168\|0x00519\|0x0029"
```

### What to look for in the diff
After a single in-game event, you're looking for FLAG transitions in these regions:

| Address Range | Likely Meaning |
|---------------|----------------|
| `0x53184C–0x53198F` | Chest opened flags |
| `0x531990–0x531A7F` | Item Array 1 (ownership) — item was added |
| `0x531A80–0x531BFF` | NPC gift/event flags |
| `0x531C00–0x531CFF` | Field pickup flags (tentative) |
| `0x53207C–0x53216F` | Item Array 2 (visibility) — item became visible |

---

## Mapping Specific Flag Types

### Chests

**What happens when you open a chest:**
1. A chest flag in the `0x53184C–0x53198F` region flips 0→1
2. The corresponding item's Array 1 and Array 2 entries flip 0→1

**Procedure:**
1. Save snapshot
2. Open the chest
3. Diff — you'll see 3 FLAG changes:
   - One in `0x5318xx` = the chest opened flag
   - One in `0x5319xx` = Item Array 1 (ownership)
   - One in `0x5320xx` = Item Array 2 (visibility)

**Chest flag structure (tentative):**
The region at `0x53184C` appears to have pairs at stride 8: `{chest_id, opened_flag}`.
- `0x531890` = chest ID 10
- `0x531894` = opened flag for that chest (Bestiary Potion)
- `0x531898` = opened flag for locked chest (Piece of Paper)

**Verifying a chest flag:**
```bash
# Read the chest flag
rmem 531894     # Should be 1 if chest was opened

# Zero it to reset
wmem 531894 0   # Chest graphic resets on screen reload

# Also remove the item (Array 2 first, then Array 1!)
wmem 532144 0   # Item Array 2 (visibility) — Bestiary Potion
wmem 531A58 0   # Item Array 1 (ownership) — Bestiary Potion
```

**Visual behavior:**
- Chest graphic (open/closed) only updates on screen transition
- But the interaction check is LIVE — you can re-open a zeroed chest even if it still looks open

**Locked chests:**
- Locked chests have a key check — game checks if you own the required key (Array 1) before allowing open
- The chest flag and key check are independent systems

---

### NPC Gifts

**What happens when an NPC gives you an item:**
1. One or more NPC flags in `0x531A80–0x531BFF` flip 0→1
2. The item's Array 1 and Array 2 entries flip 0→1

**Two types of NPC gift interactions:**

#### Type 1: Post-Dialogue Gifts (e.g., Slaff)
- NPC dialogue finishes, you select "Exit" from menu
- Item appears in inventory after dialogue closes
- **Single flag** controls the interaction
- Safe to reset independently

Example — Slaff gives Short Sword:
```
FLAG 0x00531B20: 0 -> 1   ← Slaff gift flag
FLAG 0x00531990: 0 -> 1   ← Short Sword Array 1 (ownership)
FLAG 0x0053207C: 0 -> 1   ← Short Sword Array 2 (visibility)
```

#### Type 2: Mid-Dialogue Gifts (e.g., Sara)
- NPC triggers a special cutscene/screen during dialogue
- Item is given DURING the special screen
- **Two flags** — cutscene flag + gift flag
- Must reset BOTH together or game freezes

Example — Sara gives Crystal:
```
FLAG 0x00531BAC: 0 -> 1   ← Sara cutscene flag
FLAG 0x00531BB4: 0 -> 1   ← Sara gift flag
FLAG 0x00531A10: 0 -> 1   ← Crystal Array 1 (ownership)
FLAG 0x005320FC: 0 -> 1   ← Crystal Array 2 (visibility)
```

**Reset procedure for NPC gifts (order matters!):**
```bash
# 1. Zero the NPC gift flag(s)
wmem 531B20 0      # Slaff gift flag

# 2. Zero Item Array 2 (visibility) FIRST
wmem 53207C 0      # Short Sword visibility

# 3. Zero Item Array 1 (ownership) LAST
wmem 531990 0      # Short Sword ownership
```

**CRITICAL SAFETY RULES:**
- NEVER zero Array 1 while Array 2 is still 1 → instant crash
- NEVER zero only one flag of a mid-dialogue gift (Sara-type) → game freeze
- The game checks BOTH the NPC gift flag AND Array 1 independently:
  - If gift flag = 0 but Array 1 = 1 → NPC does alternate dialogue (knows you have the item)
  - If gift flag = 0 AND Array 1 = 0 → NPC re-triggers the gift interaction

**Verifying an NPC flag (safe approach):**
1. Save snapshot
2. Talk to NPC, receive gift
3. Diff to find all FLAG changes
4. Identify which flags are in the NPC region vs item arrays
5. To confirm, reset ALL flags found (NPC flags + Array 2 + Array 1, in that order)
6. Re-trigger the interaction to verify it repeats

---

### Field Pickups

**What happens when you pick up a field item (e.g., Golden Vase):**
1. A pickup flag in `0x531C00–0x531CFF` (tentative) flips 0→1
2. The item's Array 1 and Array 2 entries flip 0→1
3. The item sprite disappears immediately (no screen transition needed)

Example — Golden Vase in Minea Fields:
```
FLAG 0x00531C28: 0 -> 1   ← Vase pickup flag
FLAG 0x00531A3C: 0 -> 1   ← Golden Vase Array 1 (ownership)
FLAG 0x00532128: 0 -> 1   ← Golden Vase Array 2 (visibility)
```

**Visual behavior (different from chests!):**
- Picking up: item sprite disappears IMMEDIATELY
- Resetting: item sprite reappears only after SCREEN TRANSITION (leave area and come back)
- If you zero the flags but stay on screen, interacting with the spot triggers the cutscene but the item doesn't actually appear → conflict state

**Reset procedure:**
```bash
# 1. Zero the pickup flag
wmem 531C28 0

# 2. Zero Item Array 2 (visibility)
wmem 532128 0

# 3. Zero Item Array 1 (ownership)
wmem 531A3C 0

# 4. Leave the area and re-enter for the item to visually respawn
```

---

### Shop Purchases

**What happens when you buy an item from a shop:**
1. Gold decreases
2. The item's Array 1 and Array 2 entries flip 0→1
3. No NPC flag or chest flag changes (shops don't use those systems)

Shop purchases are the simplest — only item array changes. To investigate shop inventories, you'd need to find where the shop item lists are stored in memory (not yet mapped).

---

## Item Array Reference

### Calculating addresses from Item ID
```
Array 1 (ownership): 0x531990 + (ID × 4)
Array 2 (visibility): 0x53207C + (ID × 4)
```

### Giving an item
```bash
wmem $(printf '%X' $((0x531990 + ID * 4))) 1   # Array 1
wmem $(printf '%X' $((0x53207C + ID * 4))) 1   # Array 2
```

### Removing an item (ORDER MATTERS)
```bash
wmem $(printf '%X' $((0x53207C + ID * 4))) 0   # Array 2 FIRST
wmem $(printf '%X' $((0x531990 + ID * 4))) 0   # Array 1 SECOND
```

### Full Item ID Table
| ID | Item | Array 1 | Array 2 |
|----|------|---------|---------|
| 0 | Short Sword | 531990 | 53207C |
| 1 | Long Sword | 531994 | 532080 |
| 2 | Talwar | 531998 | 532084 |
| 3 | Silver Sword | 53199C | 532088 |
| 4 | Flame Sword | 5319A0 | 53208C |
| 5 | Small Shield | 5319A4 | 532090 |
| 6 | Middle Shield | 5319A8 | 532094 |
| 7 | Large Shield | 5319AC | 532098 |
| 8 | Battle Shield | 5319B0 | 53209C |
| 9 | Silver Shield | 5319B4 | 5320A0 |
| 10 | Chain Mail | 5319B8 | 5320A4 |
| 11 | Plate Mail | 5319BC | 5320A8 |
| 12 | Reflex | 5319C0 | 5320AC |
| 13 | Battle Armor | 5319C4 | 5320B0 |
| 14 | Silver Armor | 5319C8 | 5320B4 |
| 15 | Power Ring | 5319CC | 5320B8 |
| 16 | Timer Ring | 5319D0 | 5320BC |
| 17 | Heal Ring | 5319D4 | 5320C0 |
| 18 | Evil Ring | 5319D8 | 5320C4 |
| 19 | Goddess Ring | 5319DC | 5320C8 |
| 20 | Book of Ys 1 | 5319E0 | 5320CC |
| 21 | Book of Ys 2 | 5319E4 | 5320D0 |
| 22 | Book of Ys 3 | 5319E8 | 5320D4 |
| 23 | Book of Ys 4 | 5319EC | 5320D8 |
| 24 | Book of Ys 5 | 5319F0 | 5320DC |
| 25 | Book of Ys 6 | 5319F4 | 5320E0 |
| 26 | Treasure Box Key | 5319F8 | 5320E4 |
| 27 | Prison Key | 5319FC | 5320E8 |
| 28 | Shrine Key | 531A00 | 5320EC |
| 29 | Ivory Key | 531A04 | 5320F0 |
| 30 | Marble Key | 531A08 | 5320F4 |
| 31 | Darm Key | 531A0C | 5320F8 |
| 32 | Sara's Crystal | 531A10 | 5320FC |
| 33 | Tovah's Amulet | 531A14 | 532100 |
| 34 | Rado's Amulet | 531A18 | 532104 |
| 35 | Dabbie's Pendant | 531A1C | 532108 |
| 36 | Tarf's Seed | 531A20 | 53210C |
| 37 | Ruby | 531A24 | 532110 |
| 38 | Silver Harmonica | 531A28 | 532114 |
| 39 | Silver Bell | 531A2C | 532118 |
| 40 | Idol | 531A30 | 53211C |
| 41 | Sapphire Ring | 531A34 | 532120 |
| 42 | Ring Treasure | 531A38 | 532124 |
| 43 | Golden Vase | 531A3C | 532128 |
| 44 | Heal Potion | 531A40 | 53212C |
| 45 | Mirror | 531A44 | 532130 |
| 46 | Hammer | 531A48 | 532134 |
| 47 | Wing | 531A4C | 532138 |
| 48 | Mask of Eyes | 531A50 | 53213C |
| 49 | Necklace | 531A54 | 532140 |
| 50 | Bestiary Potion | 531A58 | 532144 |
| 51 | Piece of Paper | 531A5C | 532148 |

---

## Verified Flags So Far

| Flag Address | Type | Event | Related Item |
|-------------|------|-------|-------------|
| `0x531894` | Chest | Bestiary Potion chest opened | Bestiary Potion (ID 50) |
| `0x531898` | Chest | Locked chest opened | Piece of Paper (ID 51) |
| `0x531B20` | NPC | Slaff gift (post-dialogue) | Short Sword (ID 0) |
| `0x531BAC` | NPC | Sara cutscene flag | Sara's Crystal (ID 32) |
| `0x531BB4` | NPC | Sara gift flag | Sara's Crystal (ID 32) |
| `0x531AF0` | NPC | Doctor Bludo dialogue | (no item) |
| `0x531AF8` | NPC | Doctor Bludo quest state | (no item) |
| `0x5318A0` | Chest (locked) | Shield Ring chest opened | Shield Ring (ID 16) |
| `0x5318A4` | Chest | Ruby chest opened | Ruby (ID 40) |
| `0x5318A8` | Chest (locked) | Necklace chest opened | Necklace (ID 42) |
| `0x5318AC` | Chest | Mask of Eyes chest opened | Mask of Eyes (ID 48) |
| `0x5318B0` | Chest | Silver Bell chest opened | Silver Bell (ID 34) |
| `0x5318B4` | Chest | Prison Key chest opened | Prison Key (ID 27) |
| `0x5318B8` | Chest | Treasure Box Key chest opened | Treasure Box Key (ID 26) |
| `0x5318BC` | Chest | Marble Key chest opened | Marble Key (ID 30) |
| `0x5318CC` | Chest | Book of Ys chest opened | Volume Hadal (ID 20) |
| `0x5318D0` | Chest | Silver Armor chest opened | Silver Armor (ID 13) |
| `0x5318D4` | Chest | Heal Potion chest 2 opened | Heal Potion (ID 44) |
| `0x5318D8` | Chest | Timer Ring chest opened | Timer Ring (ID 17) |
| `0x5318DC` | Chest | Heal Ring chest opened | Heal Ring (ID 18) |
| `0x5318E0` | Chest | Roda Tree Seed chest opened | Roda Tree Seed (ID 33) |
| `0x5318E4` | Chest | Silver Harmonica chest opened | Silver Harmonica (ID 35) |
| `0x5318E8` | Chest | Heal Potion chest 3 opened | Heal Potion (ID 44) |
| `0x5318EC` | Chest | Darm Key chest opened | Darm Key (ID 31) |
| `0x5318F0` | Chest | Volume Dabbie chest opened | Volume Dabbie (ID 22) |
| `0x531914` | Chest | Silver Sword chest opened (Darm Tower 7F) | Silver Sword (ID 3) |
| `0x531918` | Chest | Hammer chest opened (Darm Tower 9F) | Hammer (ID 46) |
| `0x531924` | Chest | Silver Armor chest opened (Darm Tower 13F) | Silver Armor (ID 13) |
| `0x531930` | Chest | Battle Shield chest opened (Darm Tower 15F) | Battle Shield (ID 9) |
| `0x531934` | Chest | Heal Potion chest 5 opened (Darm Tower 17F) | Heal Potion (ID 44) |
| `0x531938` | Chest | Battle Armor chest opened (Darm Tower 19F, requires Blue Necklace equipped) | Battle Armor (ID 14) |
| `0x53193C` | Chest | Flame Sword chest opened (Darm Tower 20F) | Flame Sword (ID 4) |
| `0x531928` | Chest | Rod chest opened (Darm Tower 14F) | Rod (ID 37) |
| `0x53192C` | Chest | Volume Gemma chest opened (Darm Tower 14F) | Volume Gemma (ID 24) |
| `0x53191C` | Chest | Volume Mesa chest opened (Darm Tower 9F) | Volume Mesa (ID 23) |
| `0x531920` | Chest | Silver Shield chest opened (Darm Tower 9F) | Silver Shield (ID 8) |
| `0x531944` | Chest | Talwar chest opened (Darm Tower 2F) | Talwar (ID 2) |
| `0x531948` | Chest | Reflex chest opened (Darm Tower 4F) | Reflex (ID 12) |
| `0x53194C` | Chest | Large Shield chest opened (Darm Tower 6F) | Large Shield (ID 7) |
| `0x5318FC` | Chest | Heal Potion chest 4 opened (Darm Tower 2F) | Heal Potion (ID 44) |
| `0x531900` | Chest | Mirror chest opened (Darm Tower 2F) | Mirror (ID 47) |
| `0x531904` | Chest | Evil Ring chest opened (Darm Tower 2F) | Evil Ring (ID 19) |
| `0x5318C0` | Chest | Ivory Key chest opened | Ivory Key (ID 29) |
| `0x5318C4` | Chest | Silver Shield chest opened | Silver Shield (ID 8) |
| `0x5318C8` | Chest | Heal Potion chest opened | Heal Potion (ID 44) |
| `0x531950` | Boss | Jenocres defeated flag 1 | — |
| `0x531AB8` | Boss | Jenocres defeated flag 2 | — |
| `0x531EC8` | Boss | Jenocres defeated flag 3 | — |
| `0x532288` | Boss | Jenocres defeated flag 4 | — |
| `0x531B2C` | NPC | Slaff gift 2 | Talwar (ID 2) |
| `0x531B44` | NPC | Slaff gift 2 secondary flag | Talwar (ID 2) |
| `0x531C14` | NPC | Reah takes Piece of Paper flag | — |
| `0x531C18` | NPC | Reah post-cutscene flag (after leaving area) | — |
| `0x531C2C` | Event | Roda Tree Seed consumed (can speak to Roda Trees) | — |
| `0x531AAC` | Event | Book read with Monocle (Volume Mesa or Gemma) | — |
| `0x531AB0` | Event | Book read with Monocle (Volume Mesa or Gemma) | — |
| `0x531D64` | NPC | Reah gift (Rado's Annex 3F) | Monocle (ID 38) |
| `0x531D60` | NPC | Raba injured dialogue (Darm Tower 13F) | — |
| `0x531F5C` | Event | Heal Potion used on injured Raba (Darm Tower 13F) | — |
| `0x531D58` | NPC | Luta Gemma gift flag 1 (Darm Tower 11F) | Blue Amulet (ID 39) |
| `0x531D5C` | NPC | Luta Gemma gift flag 2 | Blue Amulet (ID 39) |
| `0x531D50` | NPC | Luta Gemma dialogue (Darm Tower 11F) | — |
| `0x531D84` | Event | Pillar broken with Hammer (Darm Tower 11F balcony) | — |
| `0x531D4C` | NPC | Raba full heal (Darm Tower 11F) | — |
| `0x531D00` | NPC | Raba gift flag 1 (Darm Tower 3F, trades Idol for Blue Necklace) | Blue Necklace (ID 49) |
| `0x531D04` | NPC | Raba gift flag 2 | Blue Necklace (ID 49) |
| `0x531D44` | NPC | Dogi dialogue (Darm Tower 7F) | — |
| `0x531D30` | NPC | Dogi appearance flag (reused each jail break) | — |
| `0x531D34` | NPC | Dogi flag | — |
| `0x531D3C` | NPC | Dogi interaction counter (1=first, 2=second, 3=third — NOT boolean) | — |
| `0x531D38` | NPC | Dogi gift flag (walk through broken wall) | Idol (ID 36) |
| `0x53190C` | Event | Second jail cell capture event | — |
| `0x531D10` | NPC | Luta Gemma flag 1 (Darm Tower B1) | — |
| `0x531D14` | Event | Suspicious wall interaction (Darm Tower B1) | — |
| `0x531D1C` | NPC | Luta Gemma flag 2 | — |
| `0x531D08` | Event | Darm Tower silver set confiscation cutscene flag 1 | — |
| `0x531D0C` | Event | Darm Tower silver set confiscation cutscene flag 2 | — |
| `0x531D2C` | Event | Darm Tower silver set confiscation cutscene flag 3 | — |
| `0x531CE4–0x531CEC` | NPC | Goban interaction (Thieves Den, enters Darm Tower) (3 flags) | — |
| `0x531AA0–0x531AA8` | NPC | Jeba reads Books of Ys (3 flags) | — |
| `0x531C70–0x531C78` | NPC | Jeba/Feena interaction (3 flags) | — |
| `0x531C94–0x531C9C` | NPC | Feena dialogue after Jeba reads books (3 flags) | — |
| `0x531C30` | NPC | SE Roda Tree gift flag (1→0 on giving Silver Sword) | Silver Sword (ID 3) |
| `0x531EF4` | NPC | NW Roda Tree dialogue flag | — |
| `0x531E70` | NPC | Reah takes Piece of Paper secondary flag | — |
| `0x531C04` | NPC | Reah interaction flag 1 (takes Silver Harmonica, +3000 EXP) | — |
| `0x531C08` | NPC | Reah interaction flag 2 | — |
| `0x531EC0` | NPC | Reah flag 3 | — |
| `0x531EE0` | NPC | Reah flag 4 | — |
| `0x531EF8` | NPC | Reah flag 5 | — |
| `0x531BC0` | NPC | Franz gift (Minea) | Volume Tovah (ID 21) |
| `0x531BC4` | NPC | Franz secondary flag | Volume Tovah (ID 21) |
| `0x531CAC` | NPC | Mayor Robels gift (Zepik) | Power Ring (ID 15) |
| `0x531C44` | NPC | Feena rescue ("Take her with you") | — |
| `0x531A8C` | NPC | Feena pre-rescue state (1→0 on rescue) | — |
| `0x531AB4` | Event | Volume Fact read with Monocle | — |
| `0x531D94` | Event | Dark Fact cape inspection (Darm Tower 25F) | Volume Fact (ID 25) |
| `0x531A98` | Boss | Dark Fact defeated flag 1 (Darm Tower 25F) | — |
| `0x531AD0` | Boss | Dark Fact defeated flag 2 | — |
| `0x5322A0` | Boss | Dark Fact defeated flag 3 | — |
| `0x531ACC` | Boss | Yogleks & Omulgun defeated (Darm Tower 21F) | — |
| `0x531978` | Boss | Khonsclard defeated flag 1 (Darm Tower 14F) | — |
| `0x531AC8` | Boss | Khonsclard defeated flag 2 | — |
| `0x532298` | Boss | Khonsclard defeated flag 3 | — |
| `0x531968` | Boss | Pictimos defeated flag 1 (Darm Tower 8F) | — |
| `0x531AC4` | Boss | Pictimos defeated flag 2 | — |
| `0x531D98` | Boss | Pictimos defeated flag 3 | — |
| `0x532294` | Boss | Pictimos defeated flag 4 | — |
| `0x531AC0` | Boss | Vagullion defeated flag 1 | — |
| `0x532290` | Boss | Vagullion defeated flag 2 | — |
| `0x531958` | Boss | Nygtilger defeated flag 1 | — |
| `0x531ABC` | Boss | Nygtilger defeated flag 2 | — |
| `0x531ED0` | Boss | Nygtilger defeated flag 3 | — |
| `0x53228C` | Boss | Nygtilger defeated flag 4 | — |
| `0x531954` | Door | Door flag (Marble/Ivory Key area) | — |
| `0x531A94` | Door | Locked door opened (Shrine Key / Marble Key — reused?) | — |
| `0x531AE0` | Door | Locked door opened (Ivory Key) | — |
| `0x531AE4` | Door | Locked door opened (Darm Key) | — |
| `0x531AD4` | Door | Shrine door flag 2 (used Shrine Key) | — |
| `0x531C28` | Pickup | Golden Vase field pickup | Golden Vase (ID 43) |
| `0x531C6C` | NPC | Jeba gift flag | Shrine Key (ID 28) |
| `0x53206C` | NPC | Jeba secondary flag | Shrine Key (ID 28) |

---

## Workflow: Mapping a New Flag

1. **Get into position** — stand near the chest/NPC/pickup, make a save if possible
2. **Snapshot**: `wrun "C:\\mem_full_snapshot.exe" save "C:\\pre_next.bin"`
3. **Trigger the event** — open chest, talk to NPC, pick up item
4. **Diff**: `wrun "C:\\mem_full_snapshot.exe" diff "C:\\pre_next.bin"`
5. **Identify the flags** — look for FLAG 0→1 transitions in the relevant regions
6. **Cross-reference** — check if any of the flagged addresses match known item array slots
7. **Verify by resetting** (if you have a save):
   - Zero all flags found (NPC/chest/pickup flag, then Array 2, then Array 1)
   - Leave and re-enter the area
   - Re-trigger the event — if it repeats, you found the right flags
8. **Document** — record the flag address, type, event, and related item

### Troubleshooting
- **Too many changes in diff**: You moved or fought enemies between snapshot and event. Stay still, don't fight, snapshot immediately before triggering.
- **No FLAG changes**: The flag might not be a simple 0→1 boolean. Check non-FLAG lines too.
- **Flag is in a noisy region**: It might be real but filtered. Check `mem_full_snapshot.c` noise filter ranges.
- **Reset didn't work**: You might have the wrong flag. Or you might need to reset additional flags (some NPCs have multiple).
- **Game crashed on reset**: You zeroed Array 1 before Array 2. Always zero Array 2 first.
- **Game froze on reset**: You partially reset a mid-dialogue gift NPC. Reset ALL related flags together.

---

## Speedhack

Speeds up or slows down the game by hooking Windows timing functions (`QueryPerformanceCounter`, `GetTickCount`, `timeGetTime`). Works like Cheat Engine's speed hack.

### Files
| File | Location | Purpose |
|------|----------|---------|
| `tools/speedhack.c` | Source | DLL that hooks timing functions |
| `tools/inject.c` | Source | Injects the DLL into ys1plus.exe |
| `C:\speedhack.dll` | Wine C: | Compiled hook DLL |
| `C:\inject.exe` | Wine C: | Compiled injector |
| `C:\speedhack.txt` | Wine C: | Speed multiplier (read every 1s) |

### Compiling
```bash
cd /Users/luis/ys-archipelago/tools
i686-w64-mingw32-gcc -shared -o speedhack.dll speedhack.c -lwinmm -O2
i686-w64-mingw32-gcc -o inject.exe inject.c -O2
```

### Deploying
```bash
DRIVE_C="/Users/luis/Library/Application Support/CrossOver/Bottles/Steam/drive_c"
cp speedhack.dll "$DRIVE_C/speedhack.dll"
cp inject.exe "$DRIVE_C/inject.exe"
echo "2.0" > "$DRIVE_C/speedhack.txt"
```

### Using
```bash
# Set up Wine env
export WINEPREFIX="/Users/luis/Library/Application Support/CrossOver/Bottles/Steam"
export WINEDEBUG=-all
export WINEMSYNC=1
WINE="/Users/luis/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/lib/wine/x86_64-unix/wine"

# Inject (game must be running)
"$WINE" "C:\\inject.exe"
```

### Changing speed at runtime
Edit `C:\speedhack.txt` — the DLL re-reads it every second.
```bash
SPEED_FILE="/Users/luis/Library/Application Support/CrossOver/Bottles/Steam/drive_c/speedhack.txt"

echo "1.0" > "$SPEED_FILE"   # Normal speed
echo "2.0" > "$SPEED_FILE"   # 2x speed
echo "2.5" > "$SPEED_FILE"   # 2.5x speed
echo "5.0" > "$SPEED_FILE"   # 5x speed
echo "0.5" > "$SPEED_FILE"   # Half speed (slow motion)
```

### Notes
- Must re-inject after each game restart
- Valid range: 0.01 to 100.0
- Affects all game timing: movement, combat, animations, dialogue
- Does NOT affect real-world clock or save file timestamps
- The DLL hooks the exe's IAT (Import Address Table), not system-wide
