#!/usr/bin/env python3
"""Convert bundled QTMA music to MIDI with its GM instrument assignments."""
from pathlib import Path
import struct
from qtma import performance

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Resources" / "Music"
DESTINATION = ROOT / "Resources" / "MusicMIDI"


def vlq(value):
    if not 0 <= value <= 0x0FFFFFFF:
        raise ValueError("MIDI delta out of range")
    result = [value & 127]
    value >>= 7
    while value:
        result.insert(0, (value & 127) | 128)
        value >>= 7
    return bytes(result)


def midi_file(data):
    events, end, scale = performance(data)
    # One-second quarters preserve media ticks directly in the MIDI division.
    track = bytearray(b"\x00\xff\x51\x03\x0f\x42\x40")
    previous = 0
    for timestamp, _, message in events:
        track.extend(vlq(timestamp - previous))
        track.extend(message)
        previous = timestamp
    track.extend(vlq(max(end - previous, 0)))
    track.extend(b"\xff\x2f\x00")
    return b"MThd" + struct.pack(">IHHH", 6, 0, 1, scale) + b"MTrk" + struct.pack(">I", len(track)) + track


def main():
    converted = [(p, midi_file(p.read_bytes())) for p in sorted(SOURCE.glob("Song_*.mov"))]
    DESTINATION.mkdir(parents=True, exist_ok=True)
    for source, data in converted:
        destination = DESTINATION / (source.stem + ".mid")
        destination.write_bytes(data)
        print(f"{source.name} -> {destination.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
