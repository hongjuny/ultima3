#!/usr/bin/env python3
"""Reproduce the bundled SID soundtrack without opening an audio device.

Build the pinned renderer with build-sid-renderer.sh, then pass its path here.
ffmpeg encodes FLAC losslessly; no gain, EQ, or dynamics processing is applied.
"""
import argparse
import array
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import wave

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "Resources/MusicSID"
SOURCE_SHA = "488a9528d0f764e1e03592b5d3aaa41c0165357bdd547429aa9b37e6067543b3"
TITLES = ["Wanderer / surface", "Towns", "Castles", "Dungeons", "Combat", "Shops",
          "Exodus castle", "Lord British / Rule Britannia", "Shrines and healing", "Ambrosia"]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def condition_tail(samples, start, end):
    """Blend only the final128 samples toward the source preceding loopStart.

    At weight1 the final sample is exactly source[start-1], so the repeated
    seam has the source's original adjacent derivative. Period is unchanged.
    """
    length = 128
    if start < length or end - start < length:
        raise ValueError("Loop too short for canonical tail conditioning")
    for i in range(length):
        weight = (1 - math.cos(math.pi * i / (length - 1))) / 2
        index = end - length + i
        samples[index] = round(samples[index] * (1 - weight) + samples[start - length + i] * weight)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("renderer", type=Path)
    parser.add_argument("--jobs", type=int, default=3)
    args = parser.parse_args()
    renderer = args.renderer.resolve()
    if not renderer.is_file() or not shutil.which("ffmpeg"):
        parser.error("A built render-sid binary and ffmpeg are required")
    if not 1 <= args.jobs <= 10:
        parser.error("--jobs must be1..10")
    source = ASSETS / "Ultima_III-Exodus.sid"
    if digest(source.read_bytes()) != SOURCE_SHA:
        raise ValueError("Original SID checksum does not match")
    markers = json.loads((ASSETS / "source-loops.json").read_text())
    with tempfile.TemporaryDirectory(prefix="u3-sid-assets-") as directory:
        scratch = Path(directory)

        def generate(marker):
            subtune, song = marker["sid_subtune"], marker["game_song"]
            start, end = marker["loop_start_frame"], marker["loop_end_frame"]
            suffix = str(song) if song < 10 else chr(65 + song - 10)
            raw_path = scratch / f"raw-{subtune}.wav"
            subprocess.run([str(renderer), str(source), str(subtune), str(raw_path), str(end)], check=True)
            with wave.open(str(raw_path), "rb") as audio:
                if (audio.getnchannels(), audio.getsampwidth(), audio.getframerate(), audio.getnframes()) != (1, 2, 44100, end):
                    raise ValueError("Renderer emitted unexpected PCM")
                raw = audio.readframes(end)
            samples = array.array("h", raw)
            if sys.byteorder != "little":
                samples.byteswap()
            condition_tail(samples, start, end)
            if sys.byteorder != "little":
                samples.byteswap()
            pcm = samples.tobytes()
            wav = scratch / f"conditioned-{subtune}.wav"
            with wave.open(str(wav), "wb") as audio:
                audio.setparams((1, 2, 44100, end, "NONE", "not compressed"))
                audio.writeframes(pcm)
            output = ASSETS / f"Song_{suffix}.flac"
            temporary = scratch / output.name
            subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-i", str(wav), "-map_metadata", "-1",
                            "-c:a", "flac", "-compression_level", "8", "-sample_fmt", "s16", str(temporary)], check=True)
            temporary.replace(output)
            print(f"SID{subtune} → song{suffix}: {end}frames", flush=True)
            return {"game_song": song, "subtune": subtune, "title": TITLES[subtune - 1],
                    "file": output.name, "sample_rate": 44100, "channels": 1, "bits": 16,
                    "frame_count": end, "loop_start_frame": start, "loop_end_frame": end,
                    "sha256": digest(output.read_bytes()), "pcm_sha256": digest(pcm),
                    "raw_renderer_pcm_sha256": digest(raw),
                    "mapping_evidence": "SID init $9A5F–$9A6B maps subtunes1..8 to internal song1..8 and9..10 to10..11, skipping silent placeholder9; game cue names follow the original LairWare source."}

        with ThreadPoolExecutor(max_workers=args.jobs) as executor:
            tracks = list(executor.map(generate, markers["tracks"]))
    toolchain = subprocess.run(["clang", "--version"], check=True, capture_output=True, text=True).stdout.strip()
    ffmpeg_version = subprocess.run(["ffmpeg", "-version"], check=True, capture_output=True, text=True).stdout.splitlines()[0]
    manifest = {"schema_version": 1,
                "source": {"file": source.name,
                           "url": "https://hvsc.c64.org/download/C64Music/MUSICIANS/A/Arnold_Kenneth/Ultima_III-Exodus.sid",
                           "sha256": SOURCE_SHA, "bytes": source.stat().st_size,
                           "metadata": {"title": "Ultima III - Exodus", "composer": "Kenneth W. Arnold",
                                        "copyright": "1983 Origin Systems", "format": "PSIDv2", "subtunes": 10,
                                        "video_standard": "NTSC", "sid_model": "MOS6581"}},
                "renderer": {"library": "libsidplayfp", "version": "2.12.0", "sid_core": "reSIDfp",
                             "source_url": "https://github.com/libsidplayfp/libsidplayfp/releases/download/v2.12.0/libsidplayfp-2.12.0.tar.gz",
                             "source_sha256": "bc4f4fa203dcf0736fe48c23dce9aa0db825370e5941e7595e4851efe6937cdc",
                             "renderer_source_sha256": digest((ROOT / "scripts/render-sid.cpp").read_bytes()),
                             "deterministic_patch_sha256": digest((ROOT / "scripts/sid-deterministic-lut.patch").read_bytes()),
                             "generation_script_sha256": digest(Path(__file__).read_bytes()),
                             "toolchain": toolchain, "flac_encoder": ffmpeg_version,
                             "configuration": {"c64_clock": "NTSC", "sid_model": "MOS6581", "sample_rate": 44100,
                                               "channels": 1, "bits": 16, "filter_enabled": True,
                                               "filter6581_curve": 0.5, "filter6581_range": 0.5,
                                               "combined_waveforms_strength": "AVERAGE", "digi_boost": False,
                                               "cia_model": "MOS6526", "fixed_power_on_delay_cycles": 0,
                                               "sampling_method": "RESAMPLE_INTERPOLATE", "fast_sampling": False,
                                               "normalization_or_postprocessing": "128-sample raised-cosine loop-tail blend; no normalization",
                                               "lut_initialization": "serial", "render_block_frames": 441}},
                "loop_provenance": {"markers": "source-loops.json", "score_end_pc": "0x9CAC",
                                    "method": "Trace original6502 score-end execution in libsidplayfp event-scheduler PHI2cycles; first and second score wraps become loopStart and loopEnd.",
                                    "cpu_clock_hz": markers["cpu_clock_hz"], "rounding": markers["sample_marker_rounding"],
                                    "edge_conditioning_samples": 128,
                                    "edge_conditioning": "Only last128samples before loopEnd are raised-cosine blended toward original128samples preceding loopStart. No loop shortening, gain normalization, EQ or stereo processing."},
                "tracks": tracks}
    (ASSETS / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
