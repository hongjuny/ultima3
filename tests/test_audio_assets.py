"""Check the assets reachable by the modern effects player without playing audio.

Run with ``python3 -m unittest discover -s tests``. The asset checks need only
Python's standard library; comparison with the original ADPCM additionally uses
FFmpeg when available. Legacy clipping is deliberately not a test failure:
the PCM conversions preserve that distortion from the original recordings.
"""

import re
import shutil
import struct
import subprocess
import unittest
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FFMPEG = shutil.which("ffmpeg")


def effect_mappings():
    source = (ROOT / "Sources/U3AudioLegacy.m").read_text()
    return re.findall(
        r'case\s+(U3SoundEffect\w+)\s*:\s*return\s+CFSTR\("([^"]+)"\)',
        source,
    )


def effects_directory():
    # Follow the directory the native player actually searches, so an asset
    # present only in the historical Sounds directory cannot satisfy the test.
    source = (ROOT / "Sources/CocoaBridge.m").read_text()
    names = re.findall(r'pathForResource:@"(Sounds[^\"]*)"\s+ofType:nil', source)
    if len(set(names)) != 1:
        raise AssertionError("Expected one modern sound-effects resource directory")
    return ROOT / "Resources" / names[0]


def read_pcm(path):
    with wave.open(str(path), "rb") as wav:
        channels = wav.getnchannels()
        rate = wav.getframerate()
        width = wav.getsampwidth()
        frames = wav.getnframes()
        compression = wav.getcomptype()
        data = wav.readframes(frames)
    return channels, rate, width, frames, compression, data


def original_wave_format(path):
    """Read channel count and sample rate even for non-PCM legacy WAVs."""
    data = path.read_bytes()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError(f"Not a RIFF WAVE file: {path}")
    offset = 12
    while offset + 8 <= len(data):
        kind, size = struct.unpack_from("<4sI", data, offset)
        offset += 8
        if offset + size > len(data):
            raise ValueError(f"Truncated WAV chunk: {path}")
        if kind == b"fmt ":
            _, channels, rate = struct.unpack_from("<HHI", data, offset)
            return channels, rate
        offset += size + size % 2
    raise ValueError(f"Missing WAV format chunk: {path}")


class AudioAssetTests(unittest.TestCase):
    def test_all_effects_have_unique_mappings(self):
        header = (ROOT / "Sources/U3Audio.h").read_text()
        enum = re.search(r"typedef enum U3SoundEffect\s*\{(.*?)\}", header, re.S)
        self.assertIsNotNone(enum)
        effects = re.findall(r"\bU3SoundEffect\w+\b", enum.group(1))
        mappings = effect_mappings()
        self.assertEqual(len(mappings), len(effects))
        self.assertEqual({effect for effect, _ in mappings}, set(effects))
        self.assertEqual(len({name for _, name in mappings}), len(mappings))

    def test_every_mapped_effect_is_playable_pcm_in_modern_directory(self):
        directory = effects_directory()
        self.assertTrue(directory.is_dir())
        for effect, name in effect_mappings():
            with self.subTest(effect=effect, asset=name):
                candidates = list(directory.glob(name + ".*"))
                self.assertEqual(len(candidates), 1, f"Missing or ambiguous effect: {name}")
                self.assertEqual(candidates[0].suffix.lower(), ".wav")
                channels, rate, width, frames, compression, data = read_pcm(candidates[0])
                self.assertEqual(compression, "NONE")
                self.assertEqual(width, 2, "Effects must decode as signed 16-bit PCM")
                self.assertIn(channels, (1, 2))
                self.assertGreater(rate, 0)
                self.assertEqual(len(data), frames * channels * width)
                self.assertGreater(frames, 0)
                self.assertTrue(any(data), "Effect contains only silence")

    def test_legacy_wave_conversions_preserve_rate_and_channels(self):
        directory = effects_directory()
        for _, name in effect_mappings():
            original = ROOT / "Resources/Sounds" / (name + ".wav")
            if not original.exists():
                continue  # ExpLevelUp was originally stored as MP3.
            with self.subTest(asset=name):
                channels, rate, *_ = read_pcm(directory / (name + ".wav"))
                self.assertEqual((channels, rate), original_wave_format(original))

    @unittest.skipUnless(FFMPEG, "FFmpeg is optional; basic asset checks still run")
    def test_legacy_wave_conversions_preserve_decoded_samples(self):
        directory = effects_directory()
        for original in sorted((ROOT / "Resources/Sounds").glob("*.wav")):
            with self.subTest(asset=original.name):
                converted = directory / original.name
                self.assertTrue(converted.is_file())
                *_, data = read_pcm(converted)
                decoded = subprocess.run(
                    [FFMPEG, "-v", "error", "-i", str(original),
                     "-f", "s16le", "-acodec", "pcm_s16le", "-"],
                    check=True, capture_output=True, timeout=15,
                )
                self.assertEqual(data, decoded.stdout,
                                 "PCM conversion changed the original decoded waveform")


if __name__ == "__main__":
    unittest.main()
