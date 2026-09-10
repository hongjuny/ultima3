#!/usr/bin/env python3
"""Convert QuickTime 'musi' tracks into standard MIDI files.

The original movies contain QuickTime Music Architecture events rather than
sampled audio. This converter keeps note timing and velocity, using General
MIDI piano as the portable playback instrument.
"""

from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Resources" / "Music"
DESTINATION = ROOT / "Resources" / "MusicMIDI"


def read_u32(data, offset):
    return struct.unpack_from(">I", data, offset)[0]


def find_atom(data, atom_type):
    marker = atom_type.encode("ascii")
    offset = 0
    while True:
        offset = data.find(marker, offset)
        if offset < 4:
            if offset < 0:
                return []
            offset += 1
            continue
        atom_start = offset - 4
        size = read_u32(data, atom_start)
        if size >= 8 and atom_start + size <= len(data):
            yield atom_start, size
        offset += 4


def quicktime_samples(data):
    stco = next(find_atom(data, "stco"), None)
    stsz = next(find_atom(data, "stsz"), None)
    if not stco or not stsz:
        raise ValueError("missing QuickTime sample tables")

    stco_start, _ = stco
    chunk_count = read_u32(data, stco_start + 12)
    offsets = [read_u32(data, stco_start + 16 + index * 4) for index in range(chunk_count)]

    stsz_start, _ = stsz
    fixed_size = read_u32(data, stsz_start + 12)
    sample_count = read_u32(data, stsz_start + 16)
    if fixed_size:
        sizes = [fixed_size] * sample_count
    else:
        sizes = [read_u32(data, stsz_start + 20 + index * 4) for index in range(sample_count)]

    return [data[offset:offset + size] for offset, size in zip(offsets, sizes)]


def vlq(value):
    value = max(0, int(value))
    result = [value & 0x7F]
    value >>= 7
    while value:
        result.insert(0, (value & 0x7F) | 0x80)
        value >>= 7
    return bytes(result)


def midi_file(data):
    events = []
    current_time = 0
    sequence = b"".join(quicktime_samples(data))

    for offset in range(0, len(sequence) - 3, 4):
        word = read_u32(sequence, offset)
        event_type = word >> 28
        if event_type in (0, 1):
            current_time += word & 0xFFFFFF
        elif event_type in (2, 3):
            part = (word >> 24) & 0x1F
            pitch = ((word >> 18) & 0x3F) + 32
            velocity = (word >> 11) & 0x7F
            duration = word & 0x7FF
            channel = part % 16
            events.append((current_time, 1, channel, pitch, velocity))
            events.append((current_time + duration, 0, channel, pitch, 0))
        elif event_type == 9 and offset + 8 <= len(sequence):
            tail = read_u32(sequence, offset + 4)
            part = (word >> 16) & 0xFFF
            pitch = (word >> 1) & 0x7FFF
            if pitch >= 128:
                pitch = min(127, pitch >> 8)
            velocity = (tail >> 25) & 0x7F
            duration = tail & 0x3FFFFF
            channel = part % 16
            events.append((current_time, 1, channel, pitch, velocity))
            events.append((current_time + duration, 0, channel, pitch, 0))
            offset += 4

    events.sort(key=lambda event: (event[0], event[1]))
    track = bytearray()
    last_time = 0
    for timestamp, note_on, channel, pitch, velocity in events:
        track.extend(vlq(timestamp - last_time))
        track.extend(bytes([(0x90 if note_on else 0x80) | channel, pitch & 0x7F, velocity & 0x7F]))
        last_time = timestamp
    track.extend(b"\x00\xff\x2f\x00")

    header = b"MThd" + struct.pack(">IHHH", 6, 0, 1, 300)
    return header + b"MTrk" + struct.pack(">I", len(track)) + track


def main():
    DESTINATION.mkdir(parents=True, exist_ok=True)
    for source in sorted(SOURCE.glob("Song_*.mov")):
        output = DESTINATION / (source.stem + ".mid")
        output.write_bytes(midi_file(source.read_bytes()))
        print(f"{source.name} -> {output.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
