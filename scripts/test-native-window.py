#!/usr/bin/env python3
"""Run real AppKit input/close tests in an isolated copy of the built app.

Creates a temporary window and preference domain; opens no game save or audio
device. Exercises Cmd-Q and close with the real journey Cancel/Discard alert.
Requires a logged-in macOS WindowServer session, not accessibility or
screen-recording permissions. The app's own events exercise its event queue.
"""
import argparse
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import tempfile
import uuid


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("app", type=Path, nargs="?",
                        default=root / "Build/Products/Release/Ultima III.app")
    args = parser.parse_args()
    if not (args.app / "Contents/MacOS/Ultima III").is_file():
        parser.error("Build Ultima III.app first, or pass the built app's path")
    domain = "local.codex.ultima3.native." + uuid.uuid4().hex
    with tempfile.TemporaryDirectory(prefix="u3-native-window-") as temporary:
        app = Path(temporary) / "Ultima III.app"
        shutil.copytree(args.app, app, symlinks=True)
        info = app / "Contents/Info.plist"
        properties = plistlib.loads(info.read_bytes())
        properties["CFBundleIdentifier"] = domain
        info.write_bytes(plistlib.dumps(properties))
        subprocess.run(["codesign", "--force", "--deep", "--sign", "-", str(app)],
                       check=True, capture_output=True)
        try:
            environment = {key: value for key, value in os.environ.items() if not key.startswith("U3_")}
            environment.update(U3_NATIVE_UI_SELF_TEST="1", U3_TRACE_NATIVE_INPUT="1")
            result = subprocess.run([str(app / "Contents/MacOS/Ultima III")],
                env=environment,
                capture_output=True, text=True, timeout=20)
            print(result.stdout + result.stderr, end="", flush=True)
            expected = ("Native window input test: passed",
                        "Native quit alert: Cmd-Q / Cancel passed",
                        "Native quit alert: Cmd-Q / Quit Without Saving passed",
                        "Native quit alert: close / Cancel passed",
                        "Native quit alert: close / Quit Without Saving passed")
            if result.returncode == 0 and any(marker not in result.stderr for marker in expected):
                print("The active-journey quit checks did not all run; rebuild the app.", flush=True)
                return 1
            return result.returncode
        except subprocess.TimeoutExpired as error:
            print("Native window test timed out.", flush=True)
            if error.stderr:
                print(error.stderr.decode(errors="replace"), flush=True)
            return 1
        finally:
            # Only the random test app's domain is removed. The normal game's
            # preferences and save directory are never opened by this test.
            subprocess.run(["defaults", "delete", domain], capture_output=True)


if __name__ == "__main__":
    raise SystemExit(main())
