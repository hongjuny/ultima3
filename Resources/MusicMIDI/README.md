# MIDI Playback Assets

The MIDI files are generated from the original `Resources/Music/Song_*.mov`
tracks with `python3 scripts/convert-quicktime-music.py`.

Playback uses a bundled GM SoundFont rather than the operating system's
default bank. A locally supplied FluidR3 GM bank takes priority when present;
the distributable fallback is GeneralUser GS. The goal is a consistent musical
arrangement, not QuickTime or vintage-chip emulation.

Upstream: https://github.com/urish/cinto/blob/master/media/FluidR3%20GM.sf2
Original SoundFont: FluidR3 GM by Frank Wen, released under the MIT license.
SoundFont SHA-256:
`eebdac4b1f95625bf2902b77838769111cb478fb9d2c66aa8e9a8ae80fe7d0c0`

The FluidR3 attribution and license are included as `FluidR3-GM-LICENSE.txt`.
The retained GeneralUser GS comparison bank is covered by
`GeneralUser-GS-LICENSE.txt`.

Conversion restores General MIDI program assignments, routes percussion to
channel 10, reads source media timescale, and handles extended notes and
variable-length events. Supported main-sequence volume, pan, reverb, and chorus
controls are translated. QuickTime-specific instrument variants use their GM
fallback. Tune-difference editing sequences, used-note metadata, and header
MIDI-channel metadata are not played as additional notes. This is a reader for
the bundled tracks, not a general-purpose QuickTime music converter. Original
movies remain the source of truth.

Validation:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests -p test_music_conversion.py
```

The app's `U3_AUDIO_SELF_TEST=1 U3_VERIFY_MIDI_DEVICE=1` diagnostic loads the
bundled bank, prepares and starts Song_1, checks player state, then stops it.
It requires an accessible audio device. Without the second variable, the
headless diagnostic checks asset availability only.

For A/B listening tests, set `U3_MIDI_SOUNDBANK` to an external `.sf2` file.
The MIDI files retain
the instrument programs extracted from the original QuickTime tracks.
