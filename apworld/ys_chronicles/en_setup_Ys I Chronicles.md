# Ys I Chronicles Setup Guide

## Required Software

- [Archipelago](https://github.com/ArchipelagoMW/Archipelago/releases) (0.5.0 or later)
- [PPSSPP](https://www.ppsspp.org/) (PSP emulator)
- Ys I & II Chronicles (USA) ISO (ULUS-10547)
- Python 3.8+ with `websockets` library

## Installation

1. Download the Ys I Chronicles APWorld file
2. Place it in your Archipelago `custom_worlds` folder
3. Install the websockets library: `pip install websockets`

## PPSSPP Setup

1. Open PPSSPP
2. Go to **Settings** → **Tools** → **Developer tools**
3. Enable **"Allow remote debugger"**
4. Load the Ys I & II Chronicles ISO
5. Start a new game or load a save in Ys I

## Joining a Multiworld

1. Generate a seed on the Archipelago website or with the Archipelago Launcher
2. Start PPSSPP with the game loaded
3. Run the Ys Chronicles client: `python ys_client.py`
4. The client will auto-detect PPSSPP and connect
5. Enter your room info to connect to the Archipelago server

## Gameplay

- Play the game normally
- When you find items, they are sent to the multiworld
- Items you receive from other players appear automatically
- Check your client for hints and item notifications

## Troubleshooting

**Client can't connect to PPSSPP:**
- Make sure remote debugger is enabled in PPSSPP settings
- Check that the game is loaded and you're in gameplay (not title screen)

**Items not appearing:**
- The client must be running to receive items
- Some items may require exiting and re-entering an area to appear
