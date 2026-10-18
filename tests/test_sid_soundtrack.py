"""Verify original SID provenance and the bundled lossless soundtrack.

Header, FLAC STREAMINFO, mapping and file hashes use only Python's standard
library. If ffmpeg is available, decode every FLAC and check its canonical
little-endian PCM16 hash too. These checks never open an audio device.
Set U3_SID_RENDERER to a freshly built renderer for the optional process-level
determinism and argument/error checks.
"""

import array
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import wave


ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "Resources/MusicSID"
SID_SHA256 = "488a9528d0f764e1e03592b5d3aaa41c0165357bdd547429aa9b37e6067543b3"
LIB_SHA256 = "bc4f4fa203dcf0736fe48c23dce9aa0db825370e5941e7595e4851efe6937cdc"
MAPPING = {1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 10: 9, 11: 10}


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def asset_path(filename):
    """Manifest file names must resolve within the bundled resource directory."""
    if not isinstance(filename, str) or Path(filename).name != filename:
        raise ValueError("Expected a resource basename")
    path = ASSETS / filename
    if path.resolve().parent != ASSETS.resolve():
        raise ValueError("Resource path escapes MusicSID")
    return path


def flac_streaminfo(data):
    if data[:4] != b"fLaC" or len(data) < 42:
        raise ValueError("Expected a complete native FLAC stream")
    if data[4] & 0x7F or int.from_bytes(data[5:8], "big") != 34:
        raise ValueError("FLAC must begin with a 34-byte STREAMINFO block")
    packed = int.from_bytes(data[18:26], "big")
    return {"sample_rate": packed >> 44,
            "channels": ((packed >> 41) & 7) + 1,
            "bits": ((packed >> 36) & 31) + 1,
            "frame_count": packed & ((1 << 36) - 1),
            "pcm_md5": data[26:42].hex()}


class SIDSoundtrackTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads((ASSETS / "manifest.json").read_text())

    def test_original_sid_header_and_provenance(self):
        source = self.manifest["source"]
        data = asset_path(source["file"]).read_bytes()
        self.assertEqual(source["file"], "Ultima_III-Exodus.sid")
        self.assertEqual(source["url"], "https://hvsc.c64.org/download/C64Music/MUSICIANS/A/Arnold_Kenneth/Ultima_III-Exodus.sid")
        self.assertEqual(len(data), 5782)
        self.assertEqual(source["bytes"], len(data))
        self.assertEqual(source["sha256"], SID_SHA256)
        self.assertEqual(sha256(data), SID_SHA256)
        self.assertEqual(data[:4], b"PSID")
        version, offset, load, init, play, songs, start, speed = struct.unpack(">7HI", data[4:22])
        self.assertEqual((version, offset, load, init, play, songs, start, speed),
                         (2, 124, 0, 0x99E8, 0x99F4, 10, 1, 0))
        # PSID v2 bits 2–3 select NTSC, bits 4–5 select MOS6581.
        flags = int.from_bytes(data[118:120], "big")
        self.assertEqual(flags, 0x18)
        self.assertEqual((flags >> 2) & 3, 2)
        self.assertEqual((flags >> 4) & 3, 1)
        self.assertEqual(data[22:54].split(b"\0", 1)[0].decode(), "Ultima III - Exodus")
        self.assertEqual(data[54:86].split(b"\0", 1)[0].decode(), "Kenneth W. Arnold")
        self.assertEqual(data[86:118].split(b"\0", 1)[0].decode(), "1983 Origin Systems")
        metadata = source["metadata"]
        self.assertEqual(metadata["video_standard"], "NTSC")
        self.assertEqual(metadata["sid_model"], "MOS6581")

    def test_renderer_provenance(self):
        self.assertEqual(self.manifest["schema_version"], 1)
        renderer = self.manifest["renderer"]
        self.assertEqual(renderer["library"], "libsidplayfp")
        self.assertEqual(renderer["version"], "2.12.0")
        self.assertEqual(renderer["source_url"], "https://github.com/libsidplayfp/libsidplayfp/releases/download/v2.12.0/libsidplayfp-2.12.0.tar.gz")
        self.assertEqual(renderer["source_sha256"], LIB_SHA256)
        config = renderer["configuration"]
        expected = {"c64_clock": "NTSC", "sid_model": "MOS6581",
                    "sample_rate": 44100, "channels": 1, "bits": 16,
                    "filter_enabled": True, "filter6581_curve": 0.5,
                    "filter6581_range": 0.5, "fixed_power_on_delay_cycles": 0,
                    "sampling_method": "RESAMPLE_INTERPOLATE",
                    "normalization_or_postprocessing": "128-sample raised-cosine loop-tail blend; no normalization",
                    "lut_initialization": "serial", "render_block_frames": 441}
        for key, value in expected.items():
            self.assertEqual(config[key], value, key)
        for filename in ("render-sid.cpp", "build-sid-renderer.sh", "sid-deterministic-lut.patch",
                         "sid-renderer-COPYING.txt", "SID-RENDERER-NOTICE.txt"):
            self.assertTrue((ROOT / "scripts" / filename).is_file(), filename)
        for key, filename in (("renderer_source_sha256", "render-sid.cpp"),
                              ("deterministic_patch_sha256", "sid-deterministic-lut.patch"),
                              ("generation_script_sha256", "generate-sid-soundtrack.py")):
            self.assertEqual(renderer[key], sha256((ROOT / "scripts" / filename).read_bytes()), filename)

    def test_game_mapping_lossless_headers_and_asset_hashes(self):
        tracks = self.manifest["tracks"]
        self.assertEqual(len(tracks), 10)
        self.assertEqual({track["game_song"]: track["subtune"] for track in tracks}, MAPPING)
        self.assertEqual(len({track["file"] for track in tracks}), 10)
        self.assertEqual(len({track["sha256"] for track in tracks}), 10)
        expected_files = set()
        for track in tracks:
            with self.subTest(song=track["game_song"]):
                suffix = str(track["game_song"]) if track["game_song"] < 10 else chr(65 + track["game_song"] - 10)
                self.assertEqual(track["file"], f"Song_{suffix}.flac")
                expected_files.add(track["file"])
                self.assertTrue(track["title"].strip())
                self.assertTrue(track["mapping_evidence"].strip())
                data = asset_path(track["file"]).read_bytes()
                self.assertEqual(sha256(data), track["sha256"])
                info = flac_streaminfo(data)
                for key in ("sample_rate", "channels", "bits", "frame_count"):
                    self.assertEqual(info[key], track[key], key)
                self.assertEqual((info["sample_rate"], info["channels"], info["bits"]), (44100, 1, 16))
                self.assertGreater(info["frame_count"], 44100)
                self.assertLessEqual(info["frame_count"], 44100 * 60 * 20)
                self.assertTrue(0 <= track["loop_start_frame"] < track["loop_end_frame"] <= info["frame_count"])
                self.assertRegex(track["pcm_sha256"], r"^[0-9a-f]{64}$")
                self.assertNotEqual(info["pcm_md5"], "0" * 32)
        self.assertEqual({path.name for path in ASSETS.glob("*.flac")}, expected_files)

    def test_loop_frames_follow_original_driver_cycle_markers(self):
        provenance = self.manifest["loop_provenance"]
        markers = json.loads(asset_path(provenance["markers"]).read_text())
        self.assertEqual(markers["sample_rate"], 44100)
        self.assertEqual(markers["marker_pc"].lower(), "0x9cac")
        self.assertEqual(provenance["edge_conditioning_samples"], 128)
        self.assertEqual(len(markers["tracks"]), 10)
        tracks = {track["game_song"]: track for track in self.manifest["tracks"]}
        for marker in markers["tracks"]:
            with self.subTest(song=marker["game_song"]):
                track = tracks[marker["game_song"]]
                self.assertEqual(marker["sid_subtune"], track["subtune"])
                for end in ("start", "end"):
                    frames = round(marker[f"loop_{end}_cycle"] * 44100 / markers["cpu_clock_hz"])
                    self.assertEqual(marker[f"loop_{end}_frame"], frames)
                    self.assertEqual(track[f"loop_{end}_frame"], frames)
                self.assertEqual(track["frame_count"], track["loop_end_frame"])

    @unittest.skipUnless(shutil.which("ffmpeg"), "PCM verification requires ffmpeg")
    def test_decoded_pcm_hashes(self):
        for track in self.manifest["tracks"]:
            with self.subTest(song=track["game_song"]):
                decoded = subprocess.run(
                    ["ffmpeg", "-v", "error", "-i", str(asset_path(track["file"])),
                     "-map", "0:a:0", "-f", "s16le", "-acodec", "pcm_s16le", "-"],
                    capture_output=True, timeout=30, check=True,
                ).stdout
                self.assertEqual(len(decoded), track["frame_count"] * 2)
                self.assertEqual(sha256(decoded), track["pcm_sha256"])
                self.assertEqual(hashlib.md5(decoded).hexdigest(),
                                 flac_streaminfo(asset_path(track["file"]).read_bytes())["pcm_md5"])

    def test_active_music_has_no_general_midi_fallback(self):
        source = (ROOT / "Sources/UltimaSound.m").read_text()
        for obsolete in ("MusicSoundBankURL", "kAudioUnitSubType_MIDISynth", "AVAudioSequencer"):
            self.assertNotIn(obsolete, source)
        project = (ROOT / "Ultima3.xcodeproj/project.pbxproj").read_text()
        self.assertIn("MusicSID", project)

    @unittest.skipUnless(os.environ.get("U3_SID_RENDERER"), "Set U3_SID_RENDERER for regeneration checks")
    def test_renderer_is_deterministic_and_rejects_invalid_arguments(self):
        renderer = os.environ["U3_SID_RENDERER"]
        with tempfile.TemporaryDirectory(prefix="u3-sid-regeneration-") as temporary:
            first, second = (Path(temporary) / name for name in ("first.wav", "second.wav"))
            source = str(asset_path(self.manifest["source"]["file"]))
            for output in (first, second):
                subprocess.run([renderer, source, "1", str(output), "44101"],
                               capture_output=True, check=True, timeout=45)
                with wave.open(str(output)) as audio:
                    self.assertEqual((audio.getnchannels(), audio.getsampwidth(), audio.getframerate(), audio.getnframes()),
                                     (1, 2, 44100, 44101))
            self.assertEqual(first.read_bytes(), second.read_bytes())
            for song, frames in (("0", "44100"), ("11", "44100"), ("1", "0"),
                                 ("1", "-1"), ("1", "52920001"), ("1", "12x")):
                result = subprocess.run([renderer, source, song, str(first), frames],
                                        capture_output=True, timeout=10)
                self.assertNotEqual(result.returncode, 0, (song, frames))
            # A pre-existing partial file belongs to its creator and must survive errors.
            partial = first.with_suffix(".wav.part")
            partial.write_bytes(b"preserve existing partial")
            result = subprocess.run([renderer, source, "1", str(first), "44100"],
                                    capture_output=True, timeout=10)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(partial.read_bytes(), b"preserve existing partial")

    @unittest.skipUnless(os.environ.get("U3_SID_RENDERER") and shutil.which("ffmpeg"),
                         "Set U3_SID_RENDERER and install ffmpeg for raw SID asset reproduction")
    def test_assets_equal_original_sid_render_except_documented_tail(self):
        renderer = os.environ["U3_SID_RENDERER"]
        source = str(asset_path(self.manifest["source"]["file"]))
        with tempfile.TemporaryDirectory(prefix="u3-sid-source-verification-") as temporary:
            for track in self.manifest["tracks"]:
                with self.subTest(song=track["game_song"]):
                    raw_path = Path(temporary) / f"subtune-{track['subtune']}.wav"
                    subprocess.run([renderer, source, str(track["subtune"]), str(raw_path), str(track["frame_count"])],
                                   capture_output=True, check=True, timeout=90)
                    with wave.open(str(raw_path)) as audio:
                        raw = audio.readframes(audio.getnframes())
                    self.assertEqual(sha256(raw), track["raw_renderer_pcm_sha256"])
                    samples = array.array("h", raw)
                    if sys.byteorder != "little":
                        samples.byteswap()
                    start, end = track["loop_start_frame"], track["loop_end_frame"]
                    raw_final_sample = samples[start - 1]
                    for i in range(128):
                        weight = (1 - math.cos(math.pi * i / 127)) / 2
                        position = end - 128 + i
                        samples[position] = round(samples[position] * (1 - weight)
                                                  + samples[start - 128 + i] * weight)
                    self.assertEqual(samples[end - 1], raw_final_sample)
                    if sys.byteorder != "little":
                        samples.byteswap()
                    expected = samples.tobytes()
                    self.assertEqual(expected[:-256], raw[:-256])
                    self.assertEqual(sha256(expected), track["pcm_sha256"])


if __name__ == "__main__":
    unittest.main()
