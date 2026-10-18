#!/usr/bin/env python3
"""Verify real gameplay, quit alerts and whole-process exit using isolated saves.

Requires a logged-in macOS WindowServer session. Copies the app, gives it a
random preference domain with audio muted, and creates a fresh temporary save
directory for every scenario. Does not use the normal game's data or settings.
"""
import argparse
import os
from pathlib import Path
import platform
import plistlib
import shutil
import subprocess
import tempfile
import uuid


SCENARIOS = ("journey-save", "journey-discard", "journey-cancel", "healer-discard")


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("app", type=Path, nargs="?",
                        default=root / "Build/Products/Release/Ultima III.app")
    parser.add_argument("--scenario", choices=SCENARIOS, action="append",
                        help="Run only this scenario; may be repeated")
    args = parser.parse_args()
    if platform.system() != "Darwin":
        parser.error("This diagnostic requires macOS and a logged-in WindowServer session")
    if not (args.app / "Contents/MacOS/Ultima III").is_file():
        parser.error("Build Ultima III.app first, or pass the built app's path")
    print(f"Native process quit: macOS {platform.mac_ver()[0]}, "
          f"{platform.machine()}, Darwin {platform.release()}", flush=True)
    domain = "local.ultima3.nativequit." + uuid.uuid4().hex
    with tempfile.TemporaryDirectory(prefix="u3-native-quit-") as temporary:
        directory = Path(temporary)
        app = directory / "Ultima III.app"
        shutil.copytree(args.app, app, symlinks=True)
        info = app / "Contents/Info.plist"
        properties = plistlib.loads(info.read_bytes())
        print("Native process quit: app version=" + str(properties.get("CFBundleShortVersionString", "unknown")) +
              " build=" + str(properties.get("CFBundleVersion", "unknown")) +
              " LWCommit=" + str(properties.get("LWCommit", "unknown")), flush=True)
        properties["CFBundleIdentifier"] = domain
        info.write_bytes(plistlib.dumps(properties))
        subprocess.run(["codesign", "--force", "--deep", "--sign", "-", str(app)],
                       check=True, capture_output=True)
        preferences = directory / "muted.plist"
        preferences.write_bytes(plistlib.dumps({"soi": True, "mui": True, "spi": True}))
        subprocess.run(["defaults", "import", domain, str(preferences)], check=True, capture_output=True)
        try:
            for scenario in args.scenario or SCENARIOS:
                save = directory / scenario / "save"
                save.mkdir(parents=True)
                environment = {key: value for key, value in os.environ.items() if not key.startswith("U3_")}
                environment.update(U3_NATIVE_QUIT_GAME_CHECK=scenario, U3_SAVE_DIRECTORY=str(save),
                                   U3_TRACE_NATIVE_INPUT="1", U3_BASIC_PLAY="1")
                print(f"Native process quit: running {scenario}", flush=True)
                try:
                    result = subprocess.run([str(app / "Contents/MacOS/Ultima III")], env=environment,
                                            capture_output=True, text=True, timeout=35)
                except subprocess.TimeoutExpired as error:
                    print(f"Native process quit: {scenario} timed out", flush=True)
                    if error.stderr:
                        print(error.stderr.decode(errors="replace"), flush=True)
                    return 1
                print(result.stdout + result.stderr, end="", flush=True)
                marker = f"Native game quit: {scenario} passed after actual Game and cleanup returned"
                if result.returncode != 0 or marker not in result.stderr:
                    print(f"Native process quit: {scenario} FAILED (exit {result.returncode})", flush=True)
                    return 1
                if scenario == "journey-cancel" and "Cancel resumed actual Game input" not in result.stderr:
                    print("Native process quit: Cancel did not resume gameplay", flush=True)
                    return 1
                print(f"Native process quit: {scenario} exited cleanly", flush=True)
            return 0
        finally:
            subprocess.run(["defaults", "delete", domain], capture_output=True)


if __name__ == "__main__":
    raise SystemExit(main())
