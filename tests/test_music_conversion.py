import importlib.util
from pathlib import Path
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from qtma import events, performance

spec = importlib.util.spec_from_file_location("convert_music", ROOT / "scripts/convert-quicktime-music.py")
converter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(converter)


class MusicConversionTests(unittest.TestCase):
    def test_original_instruments_and_note_counts(self):
        expected = {
            "1": ([50], 835), "2": ([52], 274), "3": ([60], 614),
            "4": ([48], 555), "5": ([50], 757), "6": ([6], 175),
            "7": ([52], 409), "8": ([57, 57, 58, 56, 0], 307),
            "A": ([11, 52, 50], 148), "B": ([60], 176),
        }
        for song, (programs, notes) in expected.items():
            with self.subTest(song=song):
                data = (ROOT / f"Resources/Music/Song_{song}.mov").read_bytes()
                result, end, scale = performance(data)
                self.assertEqual([message[1] for _, _, message in result if message[0] & 240 == 192], programs)
                self.assertEqual(sum(message[0] & 240 == 144 for _, _, message in result), notes)
                self.assertEqual(sum(message[0] & 240 == 128 for _, _, message in result), notes)
                self.assertGreater(end, 0)
                self.assertEqual(scale, 600)
                self.assertEqual(converter.midi_file(data), (ROOT / f"Resources/MusicMIDI/Song_{song}.mid").read_bytes())
        result, _, _ = performance((ROOT / "Resources/Music/Song_8.mov").read_bytes())
        self.assertIn(bytes([0xC9, 0]), [message for _, _, message in result])

    def test_extended_note_consumes_both_words(self):
        data = struct.pack(">III", 0x9000003C, 0x80000000 | (100 << 22) | 4096, 12)
        parsed = list(events(data))
        self.assertEqual([kind for kind, _, _ in parsed], [9, 0])
        self.assertEqual(len(parsed[0][2]), 8)

    def test_rejects_truncated_general_event(self):
        with self.assertRaises(ValueError):
            list(events(struct.pack(">I", 0xF0000017)))


if __name__ == "__main__":
    unittest.main()
