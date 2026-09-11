# MIDI Playback Assets

The MIDI files are generated from the original `Resources/Music/Song_*.mov`
tracks with `python3 scripts/convert-quicktime-music.py`.

Playback uses the bundled GeneralUser GS v2.0.3 SoundFont by S. Christian
Collins, rather than the operating system's default bank. The goal is a
consistent musical arrangement, not QuickTime or vintage-chip emulation.

Upstream: https://github.com/mrbumpy409/GeneralUser-GS
Revision: `684543d5e5efaef08d02be50dcda8d552478fa60`
SoundFont SHA-256:
`9575028c7a1f589f5770fccc8cff2734566af40cd26ed836944e9a5152688cfe`

The unmodified upstream license is included as `GeneralUser-GS-LICENSE.txt`.
The SoundFont is third-party content, subject to that license.

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
