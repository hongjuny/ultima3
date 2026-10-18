#!/usr/bin/env python3
"""Recreate SID score-loop markers using an instrumented offline renderer.

Apply sid-score-trace.patch to the pinned libsidplayfp source and relink
render-sid.cpp first. This script writes temporary PCM files, reads original
6502 score-restart timestamps, and reports sample markers without playback.
The default 300-second capture covers two full cycles of every bundled tune.
"""

import argparse
import array
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import wave


ROOT = Path(__file__).resolve().parents[1]
SID_SHA256 = "488a9528d0f764e1e03592b5d3aaa41c0165357bdd547429aa9b37e6067543b3"
SAMPLE_RATE = 44100
# libsidplayfp 2.12.0 src/c64/c64.cpp, NTSC-M color burst * 4 / divider.
CPU_CLOCK_HZ = 3579545.455 * 4 / 14
MAPPING_BYTES = bytes.fromhex("18 69 01 c9 09 90 03 18 69 01 85 58 85 59")


def source_mapping(data):
    if hashlib.sha256(data).hexdigest() != SID_SHA256:
        raise ValueError("Expected the pinned original Ultima III SID source")
    offset = int.from_bytes(data[6:8], "big")
    load = int.from_bytes(data[8:10], "big")
    if load == 0:
        load = int.from_bytes(data[offset:offset + 2], "little")
        offset += 2
    position = offset + 0x9A5F - load
    if data[position:position + len(MAPPING_BYTES)] != MAPPING_BYTES:
        raise ValueError("Original subtune-to-game-song instruction sequence changed")
    return {n: n if n <= 8 else n + 1 for n in range(1, 11)}


def trace(renderer, source, subtune, frames, directory):
    output = directory / f"subtune-{subtune}.wav"
    environment = os.environ.copy()
    environment["U3_SID_SCORE_TRACE"] = "1"
    result = subprocess.run(
        [str(renderer), str(source), str(subtune), str(output), str(frames)],
        capture_output=True, text=True, env=environment, check=True, timeout=300,
    )
    markers = [(int(pc, 16), int(cycles)) for pc, cycles in
               re.findall(r"U3_SCORE_MARK pc=([0-9a-fA-F]+) cycles=(\d+)", result.stderr)]
    wraps = [cycles for pc, cycles in markers if pc == 0x9CAC]
    if len(wraps) < 2:
        raise ValueError(
            f"Subtune {subtune}: found {len(wraps)} score wraps; use a renderer "
            "built with sid-score-trace.patch and a long enough --seconds capture"
        )
    first, second = wraps[:2]
    start, end = [round(cycles * SAMPLE_RATE / CPU_CLOCK_HZ) for cycles in (first, second)]
    with wave.open(str(output), "rb") as audio:
        if (audio.getnchannels(), audio.getsampwidth(), audio.getframerate()) != (1, 2, SAMPLE_RATE):
            raise ValueError("Expected mono 44.1 kHz PCM16 from the renderer")
        samples = array.array("h", audio.readframes(audio.getnframes()))
        if sys.byteorder != "little":
            samples.byteswap()
    if not 128 <= start < end < len(samples):
        raise ValueError("Score markers fall outside the rendered capture")
    return {
        "sid_subtune": subtune,
        "game_song": subtune if subtune <= 8 else subtune + 1,
        "loop_start_cycle": first,
        "loop_end_cycle": second,
        "loop_start_frame": start,
        "loop_end_frame": end,
        "loop_sample_count": end - start,
        "loop_seconds": (end - start) / SAMPLE_RATE,
        "raw_boundary_jump_s16": abs(samples[start] - samples[end - 1]),
        "conditioned_boundary_jump_s16": abs(samples[start] - samples[start - 1]),
        "trace_markers": [{"pc": hex(pc), "cycles": cycles} for pc, cycles in markers],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("renderer", type=Path)
    parser.add_argument("--source", type=Path,
                        default=ROOT / "Resources/MusicSID/Ultima_III-Exodus.sid")
    parser.add_argument("--output", type=Path, required=True,
                        help="Write the analysis JSON here; production resources are not replaced")
    parser.add_argument("--subtune", type=int, choices=range(1, 11), action="append",
                        help="Restrict tracing to one or more subtunes")
    parser.add_argument("--seconds", type=int, default=300,
                        help="Capture length per tune, default 300 seconds")
    args = parser.parse_args()
    if not 1 <= args.seconds <= 1200:
        parser.error("--seconds must be between 1 and 1200")
    renderer, source = args.renderer.resolve(), args.source.resolve()
    output = args.output.resolve()
    if not renderer.is_file():
        parser.error("The instrumented renderer does not exist")
    asset_directory = (ROOT / "Resources/MusicSID").resolve()
    if output in (source, renderer) or output.is_relative_to(asset_directory):
        parser.error("Choose an analysis output outside the production MusicSID resources")
    source_mapping(source.read_bytes())
    tracks = []
    with tempfile.TemporaryDirectory(prefix="u3-sid-score-trace-") as temporary:
        for subtune in sorted(set(args.subtune or range(1, 11))):
            row = trace(renderer, source, subtune, args.seconds * SAMPLE_RATE, Path(temporary))
            tracks.append(row)
            print(f"Subtune {subtune}: loop [{row['loop_start_frame']}, "
                  f"{row['loop_end_frame']}) samples", flush=True)
    report = {
        "source_sha256": SID_SHA256,
        "renderer_sha256": hashlib.sha256(renderer.read_bytes()).hexdigest(),
        "trace_patch_sha256": hashlib.sha256((ROOT / "scripts/sid-score-trace.patch").read_bytes()).hexdigest(),
        "cpu_clock_hz": CPU_CLOCK_HZ,
        "sample_rate": SAMPLE_RATE,
        "marker_pc": "0x9cac",
        "sample_marker_rounding": "round(cycles * sample_rate / cpu_clock_hz)",
        "mapping_evidence": "Original instructions at $9A5F-$9A6B add 1 to the "
                            "zero-based subtune, then add 1 again for values >=9; "
                            "the result is stored at $58 and $59.",
        "tracks": tracks,
    }
    output.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
