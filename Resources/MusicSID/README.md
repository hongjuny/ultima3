# C64 SID soundtrack

These lossless FLAC files contain the original Ultima III C64 music program
running in libsidplayfp 2.12.0 with reSIDfp. The app plays the captured PCM and
its recorded loop markers; it does not run a live SID emulator. The capture
retains the C64 arrangement and chip synthesis. It does not use MIDI or a
General MIDI instrument bank.

The first pass includes the opening through the first complete score cycle.
Playback then repeats the interval from the first score wrap to the second.
Only the final 128 samples before the loop end receive the edge conditioning
described below. FLAC preserves that resulting PCM losslessly. The rendered
files are therefore not entirely untouched emulator output.

## Source and emulation

The SID file was obtained directly from the
[High Voltage SID Collection](https://hvsc.c64.org/download/C64Music/MUSICIANS/A/Arnold_Kenneth/Ultima_III-Exodus.sid).
Its PSID v2 header identifies ten subtunes, NTSC timing, and a MOS6581 SID:
flags `0x0018`, VBI speed mask `0`, initialization address `$99E8`, and playback
address `$99F4`. The embedded credits read **Kenneth W. Arnold** and
**1983 Origin Systems**.

Pinned source hashes:

| Input | SHA-256 |
|---|---|
| `Ultima_III-Exodus.sid` (5,782 bytes) | `488a9528d0f764e1e03592b5d3aaa41c0165357bdd547429aa9b37e6067543b3` |
| [libsidplayfp 2.12.0 release archive](https://github.com/libsidplayfp/libsidplayfp/releases/download/v2.12.0/libsidplayfp-2.12.0.tar.gz) | `bc4f4fa203dcf0736fe48c23dce9aa0db825370e5941e7595e4851efe6937cdc` |

Rendering uses NTSC-M, MOS6581, mono 44.1 kHz signed 16-bit PCM, the
`RESAMPLE_INTERPOLATE` sampling method, and enabled filters with 6581 curve
and range both `0.5`. Power-on delay is fixed at zero, CIA model is MOS6526,
combined waveforms use `AVERAGE`, and digi boost is disabled. Render requests
use blocks of 441 samples, including a final partial block when necessary.

The deterministic lookup-table patch serializes initialization of the filter
tables. The upstream parallel initializers share a dither sequence and can
otherwise change a few low PCM bits across processes. The patch does not
change SID register writes or the selected synthesis and filter settings.
`manifest.json` records the compiler, encoder, renderer and patch hashes, and
both FLAC-file and decoded-PCM hashes. Different floating-point toolchains
may regenerate slightly different low bits; the manifest identifies the
canonical assets.

## Game cue mapping

The original SID initialization code provides the mapping. At `$9A5F-$9A6B`
it adds one to the zero-based subtune number, adds one again when the result
is at least nine, and stores the result at `$58` and `$59`:

```text
CLC
ADC #$01
CMP #$09
BCC skip_extra_increment
CLC
ADC #$01
skip_extra_increment:
STA $58
STA $59
```

Consequently subtunes 1-8 select internal songs 1-8, subtune 9 selects song
10 (`A`), and subtune 10 selects song 11 (`B`). These indices agree with the
game's music selection. The cue names below describe those game contexts;
they are not a claim that every C64 arrangement matches the Mac QuickTime
arrangement. Matching note sequences independently confirm the surface,
combat, Exodus castle, Rule Britannia, and shrine cues. For example, portions
of the surface melody appear seven semitones above their Mac counterparts.

Internal song 9 points to a rest/end placeholder at `$AD13`. It is skipped
by the SID wrapper and contains no additional death melody. Executing that
internal entry for 1,000 VBI calls left all three frequency-register pairs at
zero. No replacement melody has been invented for this slot.

The [HVSC STIL](https://hvsc.c64.org/download/C64Music/DOCUMENTS/STIL.txt)
explicitly credits subtune 8 as **Rule Britannia**, with music by Thomas Arne
and lyrics by James Thompson. That spelling reproduces the STIL credit.

## Exact loop evidence

Loop markers come from executing the original score driver, rather than
choosing rounded song lengths or matching an audio waveform by ear. The
driver enters `$9CAC` to restart the whole score; the following branch
re-enters setup at `$9AC4`. The optional trace patch logs these program
counters with libsidplayfp's event-scheduler PHI2 CPU-cycle timestamps.

For this NTSC configuration, the library's CPU clock is
`3579545.455 * 4 / 14 = 1022727.2728571428 Hz`. A recorded cycle timestamp
becomes a sample marker through
`round(cycles * 44100 / cpu_clock_hz)`. The first and second score wraps
define the half-open loop interval `[loop_start_frame, loop_end_frame)`.
`source-loops.json` preserves the exact cycle timestamps and sample markers.

| SID subtune | Game song | Game cue | Loop start sample | Loop end sample | Loop seconds |
|---:|---:|---|---:|---:|---:|
| 1 | 1 | Wanderer / surface | 5,838,884 | 11,677,741 | 132.400385 |
| 2 | 2 | Towns | 2,831,367 | 5,662,708 | 64.202744 |
| 3 | 3 | Castles | 1,911,421 | 3,822,815 | 43.342268 |
| 4 | 4 | Dungeons | 945,772 | 1,891,518 | 21.445488 |
| 5 | 5 | Combat | 2,123,716 | 4,247,406 | 48.156236 |
| 6 | 6 | Shops | 708,414 | 1,416,802 | 16.063220 |
| 7 | 7 | Exodus castle | 3,310,506 | 6,620,985 | 75.067551 |
| 8 | 8 | Lord British / Rule Britannia | 708,406 | 1,416,794 | 16.063220 |
| 9 | A / 10 | Shrines and healing | 708,414 | 1,416,802 | 16.063220 |
| 10 | B / 11 | Ambrosia | 849,944 | 1,699,863 | 19.272540 |

The SID oscillators and filters do not necessarily return to identical
analog state when the score wraps. Some raw joins therefore contain a larger
sample jump than nearby natural waveform changes. To condition the join
without shortening the musical period, the generator blends only the final
128 samples (about 2.9 ms) toward the original 128 samples immediately
preceding the loop start. The raised-cosine weight rises from zero to one.
The resulting final sample equals `source[loop_start - 1]`, so the repeated
seam has the original adjacent derivative
`source[loop_start] - source[loop_start - 1]`. There is no gain normalization,
EQ, compression, or stereo processing.

## Reproduction

From the repository root, build the pinned offline renderer in a new scratch
directory and regenerate the soundtrack:

```sh
sid_build=$(mktemp -d "${TMPDIR:-/tmp}/ultima3-sid-build.XXXXXX")
sh scripts/build-sid-renderer.sh "$sid_build"
python3 scripts/generate-sid-soundtrack.py "$sid_build/render-sid"
```

The build requires a C++20 compiler and the usual configure/make tools; FLAC
encoding requires FFmpeg. The build script downloads and verifies the pinned
release archive, retains its source and GPL notices in the scratch directory,
and installs nothing systemwide. The generator replaces the ten FLAC assets
and `manifest.json`. It does not launch the game or open an audio device.

To reproduce the loop evidence separately, apply the optional trace patch to
a scratch build and relink the same renderer source. Keep this analysis
binary separate from the canonical asset renderer:

```sh
repo_dir="$PWD"
trace_build=$(mktemp -d "${TMPDIR:-/tmp}/ultima3-sid-trace.XXXXXX")
sh scripts/build-sid-renderer.sh "$trace_build"
(
    cd "$trace_build/libsidplayfp-2.12.0"
    patch -p1 < "$repo_dir/scripts/sid-score-trace.patch"
    make -j4
)
c++ -std=c++20 -O2 -I "$trace_build/libsidplayfp-2.12.0/src" \
    scripts/render-sid.cpp "$trace_build/libsidplayfp-2.12.0/src/.libs/libsidplayfp.a" \
    -o "$trace_build/render-sid-trace"
python3 scripts/trace-sid-score.py "$trace_build/render-sid-trace" \
    --output "$trace_build/retraced-loops.json"
```

The analysis script uses Python's standard library, verifies the original
SID hash and mapping instructions, captures enough audio for two score
cycles, and extracts the actual CPU timestamps. It writes analysis JSON and
temporary WAVs; it does not overwrite production resources. A shorter check
can use `--subtune 8 --seconds 35`. Tracing is enabled only by
`U3_SID_SCORE_TRACE=1`; the script supplies that environment variable.

## Credits and notices

Original Ultima III music: **Kenneth W. Arnold**. Original SID metadata:
**1983 Origin Systems**. Rule Britannia composition: **Thomas Arne**, with
the lyric credit recorded in STIL above. HVSC supplies the preserved SID
program and its collection metadata.

The repository's MIT license applies to its code and does not grant rights
to the separately copyrighted Ultima music, game names, or original SID
program. Rendering and FLAC encoding do not change those credits or rights.
The offline renderer and its libsidplayfp/reSIDfp components retain their
GPL-2.0-or-later notices; see `scripts/SID-RENDERER-NOTICE.txt` and
`scripts/sid-renderer-COPYING.txt`. The application bundles audio captures,
the original SID source, and provenance data; the GPL renderer and emulator
library are build-time tools and are not linked into the app.
