"""Compile the real SID PCM player and verify it through silent offline rendering."""

import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(platform.system() == "Darwin" and shutil.which("clang"),
                     "Native offline SID rendering requires macOS developer tools")
class SIDNativeAudioTests(unittest.TestCase):
    def test_lossless_player_fixtures_and_canonical_flac(self):
        assets = ROOT / "Resources/MusicSID"
        manifest = json.loads((assets / "manifest.json").read_text())
        track = next(item for item in manifest["tracks"] if item["game_song"] == 8)
        canonical = assets / track["file"]
        self.assertTrue(canonical.is_file(), "Canonical C64 SID FLAC must be bundled")
        with tempfile.TemporaryDirectory(prefix="u3-native-sid-") as temporary:
            base = Path(temporary)
            executable = base / "sid-audio-test"
            compiled = subprocess.run(
                ["clang", "-fsanitize=address,undefined", "-g", "-Wall", "-Wextra",
                 "-mmacosx-version-min=13.0", "-I", str(ROOT / "Sources"),
                 "-framework", "Foundation", "-framework", "AVFoundation",
                 str(ROOT / "Sources/U3SIDMusic.m"), str(ROOT / "tests/sid-audio-test.m"),
                 "-o", str(executable)],
                capture_output=True, text=True, timeout=45,
            )
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            scratch = base / "offline-artifacts"
            scratch.mkdir()
            checked = subprocess.run(
                [str(executable), str(canonical), str(track["loop_start_frame"]),
                 str(track["loop_end_frame"])],
                env={**os.environ, "TMPDIR": str(scratch) + "/"},
                capture_output=True, text=True, timeout=60,
            )
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
            self.assertIn("tests passed", checked.stderr)
            self.assertEqual(checked.stderr.count("maxPCMerror=0.000000000"), 9)


if __name__ == "__main__":
    unittest.main()
