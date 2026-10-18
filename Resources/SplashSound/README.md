# Original Mac title sound

`Whoosh.snd` is the complete, unchanged `snd ` resource 400, named **Whoosh**,
from LairWare's released Ultima III source. `Whoosh.wav` contains the exact
785 unsigned 8-bit sample bytes for convenient inspection. It is a short,
recorded noisy transient, rather than newly generated white noise. The title
sound remains the Mac version's effect; the existing MIDI music path is
unchanged.

The runtime receives the original resource through the game's resource
loader. Carbon translates the sound headers to native little endian when
loading this resource; the decoder accepts that form as well as the original
big-endian resource file. `U3SplashSoundPlayer` reads its fractional sample rate and base note,
resamples that sample to 44.1 kHz PCM, and retriggers it as each image
dissolves. It caches the 32 pitch variants and alternates two voices. Volume
changes affect both active voices immediately; mute and stop release the
voices and clear the cache. There is no normalization, added noise, or gain
boost. The complete offline preview peaks at 8,849 out of 32,768, with no
clipped samples.

## Source evidence

The source is [beastie/ultima3](https://github.com/beastie/ultima3),
`Resources/English.lproj/MainResources.rsrc`. The resource fork and extracted
resource are pinned by SHA-256:

| File | SHA-256 |
|---|---|
| `MainResources.rsrc` | `1cf448f844f991624da667ab767bd3add7bc613fe0cbeb48750eeaf5d68cd783` |
| `Whoosh.snd` | `8dec31a7551569c47866d3cf997ee266de30a5fcc943ef6ec932b34d0580b721` |
| Original sample bytes | `564ff8f2c8020812a092fab9bea99baecfad22a8b39c29ef8058134a976a7c3d` |

The resource is format 1 with a sampled-synth modifier and an inline standard
SoundHeader at offset 20. It has 785 mono unsigned 8-bit samples, a 16.16
Fixed rate of `729236944` (11,127.272705078125 Hz), base MIDI note 60, and
loop points 783 and 784 in the silent tail. The WAV rate is rounded to
11,127 Hz because its rate field is an integer; runtime resampling reads the
exact Fixed rate from the original resource. `provenance.json` records these
fields and all output hashes.

Carbon's `GetResource` translates the sound resource's multibyte headers to
native byte order on modern little-endian Macs, while preserving its sample
bytes. The helper accepts both that runtime representation and the original
big-endian `.snd` file. The native regression test loads the real resource
through the production I/O loader and Carbon, then checks that every pitch
renders identically from both representations.

The original `FadeOnExodusUltima` in `Sources/UltimaGraphics.c` starts at
pass -32,767 and adds 1,536 through 31,745: 43 triggers for each image. The
first image advances every five 60 Hz ticks, then waits 60 ticks; the second
advances every four ticks, then holds for 120 ticks. An offline preview
starting at the first trigger therefore lasts 567 ticks, or 9.45 seconds.
Natural sample tails can extend into the pauses. The active `OpenChannel`
path in `Sources/UltimaSound.m` creates two voices. The optional three extra
voices in that file are commented out in the released original.

## Pitch compatibility limitation

The original issues `freqDurationCmd` with parameter 1 equal to 60 and
parameter 2 equal to:

```c
0xFF00E0A0 | ((32768 - pass) / 2048)
```

[Apple's Sound Manager documentation](https://developer.apple.com/library/archive/documentation/mac/Sound/Sound-54.html)
defines parameter 2 as a MIDI note from 0 through 127, derives sampled-sound
pitch from the header's base frequency, and describes different duration
behavior when this command is issued immediately. The original's high bits
are outside that documented note range. A
[1992 report from examining the Sound Manager](https://groups.google.com/g/comp.sys.mac.programmer/c/oirtJekZKTQ)
describes the low seven bits as the note used by its implementation.

This reconstruction uses those low seven bits, producing notes 63 down to
32, and the standard equal-tempered sample-rate ratio
`2^((note - baseNote) / 12)`. It plays the original one-shot sample to its
silent tail or until the alternating voice is retriggered. Linear
interpolation replaces the historical resampler. This is an explicit
compatibility interpretation of the released source and original sample,
not a bit-exact recording validated against a historical Mac.

## Reproduction and silent checks

From the repository root:

```sh
python3 scripts/extract-splash-sound.py
python3 -m unittest tests/test_splash_sound.py -v
```

Extraction uses Python's standard library, checks the pinned original
resource hash, and preserves every sample byte. On macOS the test compiles
the actual native helper with a silent player substitute under AddressSanitizer
and UndefinedBehaviorSanitizer. It opens the actual bundled resource through the production loader and
Carbon's Resource Manager, then verifies all 43 pitch renders against the raw
source file. It also checks sample decoding, fractional-rate resampling,
pitch changes, two-voice limits, second-image retriggering,
volume, mute, cache cleanup, and offline diagnostics. It never opens an
audio device.

To save the four diagnostic WAVs while running those silent tests:

```sh
U3_SPLASH_RENDER_DIRECTORY=/tmp/ultima3-splash-preview \
    python3 -m unittest tests/test_splash_sound.py -v
```

The output contains `splash-start.wav`, `splash-middle.wav`, `splash-end.wav`,
and the complete `splash-sequence.wav`. The app's headless
`U3_AUDIO_SELF_TEST=1` diagnostic includes the same renderer and optional
environment variable.

The sample is supplied in Leon McNeill's LairWare source release. Preserve
the upstream copyright and asset-rights notices. Recovering or converting
these bytes does not establish a separate license for the original game
assets.
