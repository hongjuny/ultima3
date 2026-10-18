#!/usr/bin/env python3
"""Extract the original splash Whoosh sample from the Mac resource fork.

This preserves the original uncompressed sample bytes and the complete snd
resource. No oscillator, noise, gain adjustment, or audio output is used.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import wave


ROOT = Path(__file__).resolve().parents[1]
EXPECTED = "8dec31a7551569c47866d3cf997ee266de30a5fcc943ef6ec932b34d0580b721"


def sound_resource(data, resource_id):
    data_offset, map_offset, data_size, map_size = struct.unpack_from(">IIII", data)
    if data_offset + data_size > len(data) or map_offset + map_size > len(data):
        raise ValueError("Truncated resource fork")
    types = map_offset + struct.unpack_from(">H", data, map_offset + 24)[0]
    names = map_offset + struct.unpack_from(">H", data, map_offset + 26)[0]
    count = struct.unpack_from(">H", data, types)[0] + 1
    for index in range(count):
        kind, entries, references = struct.unpack_from(">4sHH", data, types + 2 + index * 8)
        if kind != b"snd ":
            continue
        for entry in range(entries + 1):
            position = types + references + entry * 12
            number, name_offset = struct.unpack_from(">hH", data, position)
            if number != resource_id:
                continue
            relative = int.from_bytes(data[position + 5:position + 8], "big")
            location = data_offset + relative
            length = struct.unpack_from(">I", data, location)[0]
            payload = data[location + 4:location + 4 + length]
            if len(payload) != length:
                raise ValueError("Truncated sound resource")
            name = None if name_offset == 65535 else data[
                names + name_offset + 1:names + name_offset + 1 + data[names + name_offset]
            ].decode("mac_roman")
            return payload, name
    raise ValueError(f"Missing snd resource {resource_id}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path,
                        default=ROOT / "Resources/English.lproj/MainResources.rsrc")
    parser.add_argument("--output", type=Path, default=ROOT / "Resources/SplashSound")
    args = parser.parse_args()
    original = args.source.read_bytes()
    resource, name = sound_resource(original, 400)
    if hashlib.sha256(resource).hexdigest() != EXPECTED or name != "Whoosh":
        raise ValueError("Original Whoosh source does not match the pinned resource")
    pointer, length, rate, loop_start, loop_end, encoding, base_note = struct.unpack_from(
        ">IIIIIBB", resource, 20)
    samples = resource[42:42 + length]
    if pointer or encoding or len(samples) != length:
        raise ValueError("Expected the original inline unsigned 8-bit sample")
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "Whoosh.snd").write_bytes(resource)
    output = args.output / "Whoosh.wav"
    with wave.open(str(output), "wb") as audio:
        audio.setparams((1, 1, round(rate / 65536), length, "NONE", "not compressed"))
        audio.writeframes(samples)
    provenance = {
        "source_repository": "https://github.com/beastie/ultima3",
        "source_file": "Resources/English.lproj/MainResources.rsrc",
        "source_file_sha256": hashlib.sha256(original).hexdigest(),
        "resource_type": "snd ", "resource_id": 400, "resource_name": name,
        "resource_file": "Whoosh.snd", "resource_bytes": len(resource),
        "resource_sha256": EXPECTED,
        "sample_header_offset": 20, "sample_data_offset": 42,
        "sample_frames": length, "sample_bits": 8, "channels": 1,
        "sample_encoding": "unsigned PCM; 128 is silence",
        "sample_rate_fixed": rate, "sample_rate_hz": rate / 65536,
        "base_midi_note": base_note, "loop_start": loop_start, "loop_end": loop_end,
        "sample_pcm_sha256": hashlib.sha256(samples).hexdigest(),
        "wav_file": output.name, "wav_rate_hz": round(rate / 65536),
        "wav_sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
        "wav_note": "PCM bytes are unchanged; WAV's integer rate rounds the original Fixed rate. Runtime synthesis reads the exact rate from Whoosh.snd.",
        "original_code": "Sources/UltimaGraphics.c: FadeOnExodusUltima",
        "trigger_passes": {"first": -32767, "step": 1536, "count_per_image": 43},
        "alternating_voices": 2,
        "cadence_ticks": {"ticks_per_second": 60, "exodus_step": 5,
                          "between_images": 60, "ultima_step": 4, "after_ultima": 120},
        "legacy_pitch_command": "0xFF00E0A0 | ((32768 - pass) / 2048)",
        "compatibility_pitch": "Use the command's low seven bits as a MIDI note (63 down to 32); see README.md for the historical compatibility limitation.",
        "credits": "Original LairWare Ultima III sample supplied in Leon McNeill's source release. Preserve the repository's non-code asset rights notices.",
    }
    (args.output / "provenance.json").write_text(json.dumps(provenance, indent=2) + "\n")
    print(f"Extracted {name}: {length} original sample bytes, {rate / 65536:.8f} Hz")


if __name__ == "__main__":
    main()
