#!/usr/bin/env python3
"""
Ys I Chronicles - Archipelago Client Launcher

Thin launcher that:
  1. Writes C:\ap_connect.txt (config for aphook.dll)
  2. Injects aphook.dll into ys1plus.exe
  3. Tails C:\aphook.log for live output

All game logic (item hooks, location checks, AP protocol, tower guard,
goal detection) runs inside aphook.dll.
"""

import asyncio
import os
import platform
import sys
import time


# =============================================================================
# Paths
# =============================================================================

def get_drive_c() -> str:
    if platform.system() == "Darwin":
        return os.path.expanduser(
            "~/Library/Application Support/CrossOver/Bottles/Steam/drive_c"
        )
    elif platform.system() == "Windows":
        return "C:\\"
    else:
        raise RuntimeError(f"Unsupported platform: {platform.system()}")


def get_wine_path() -> str:
    return os.environ.get(
        "WINE_PATH",
        os.path.expanduser(
            "~/Applications/CrossOver.app/Contents/SharedSupport/"
            "CrossOver/lib/wine/x86_64-unix/wine"
        )
    )


def get_wineprefix() -> str:
    return os.environ.get(
        "WINEPREFIX",
        os.path.expanduser(
            "~/Library/Application Support/CrossOver/Bottles/Steam"
        )
    )


def wine_env() -> dict:
    return {
        "WINEPREFIX": get_wineprefix(),
        "WINEDEBUG": "-all",
        "WINEMSYNC": "1",
        "PATH": os.environ.get("PATH", ""),
    }


# =============================================================================
# Config File
# =============================================================================

def write_config(drive_c: str, server: str, slot: str, password: str):
    """Write ap_connect.txt that aphook.dll reads on startup."""
    config_path = os.path.join(drive_c, "ap_connect.txt")
    with open(config_path, "w") as f:
        f.write(f"server={server}\n")
        f.write(f"slot={slot}\n")
        f.write(f"password={password}\n")
    print(f"Config written: server={server} slot={slot}")


# =============================================================================
# DLL Injection
# =============================================================================

async def inject_dll(dll_name: str = "aphook.dll"):
    """Inject aphook.dll into ys1plus.exe via inject.exe."""
    dll_path = f"C:\\{dll_name}"

    if platform.system() == "Darwin":
        wine = get_wine_path()
        result = await asyncio.create_subprocess_exec(
            wine, "C:\\inject.exe", dll_path,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            env=wine_env(),
        )
    else:
        result = await asyncio.create_subprocess_exec(
            "inject.exe", dll_path,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )

    stdout, stderr = await result.communicate()
    out = stdout.decode().strip()
    err = stderr.decode().strip()

    if result.returncode == 0:
        print(f"Injected {dll_name}")
        if out:
            print(f"  {out}")
        return True
    else:
        print(f"Injection failed (rc={result.returncode})")
        if out:
            print(f"  stdout: {out}")
        if err:
            print(f"  stderr: {err}")
        return False


# =============================================================================
# Log Tail
# =============================================================================

def tail_log(drive_c: str):
    """Tail C:\aphook.log, printing new lines as they appear."""
    log_path = os.path.join(drive_c, "aphook.log")
    print(f"Tailing {log_path} (Ctrl+C to stop)\n")

    # Wait for log file to appear
    while not os.path.exists(log_path):
        time.sleep(0.5)

    with open(log_path, "r") as f:
        # Seek to end
        f.seek(0, 2)
        while True:
            line = f.readline()
            if line:
                print(line, end="", flush=True)
            else:
                time.sleep(0.1)


# =============================================================================
# Main
# =============================================================================

def main():
    import argparse
    parser = argparse.ArgumentParser(
        description="Ys I Chronicles - Archipelago Client Launcher"
    )
    parser.add_argument(
        "server", nargs="?", default="localhost:38281",
        help="AP server address host:port (default: localhost:38281)"
    )
    parser.add_argument("--slot", "-s", default="Adol", help="Slot name")
    parser.add_argument("--password", "-p", default="", help="Server password")
    parser.add_argument(
        "--no-inject", action="store_true",
        help="Skip DLL injection (if already injected)"
    )
    parser.add_argument(
        "--no-tail", action="store_true",
        help="Don't tail the log file"
    )
    args = parser.parse_args()

    print("=" * 50)
    print("  Ys I Chronicles - Archipelago Client")
    print("=" * 50)
    print()

    drive_c = get_drive_c()

    # Step 1: Write config
    write_config(drive_c, args.server, args.slot, args.password)

    # Step 2: Inject DLL
    if not args.no_inject:
        success = asyncio.run(inject_dll())
        if not success:
            print("\nFailed to inject. Is ys1plus.exe running?")
            sys.exit(1)
        print()

    # Step 3: Tail log
    if not args.no_tail:
        try:
            tail_log(drive_c)
        except KeyboardInterrupt:
            print("\nStopped.")


if __name__ == "__main__":
    main()
