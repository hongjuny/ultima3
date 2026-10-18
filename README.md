# LairWare's Ultima III

This started out as an unofficial fan remake of the original 1983 Apple II game from Origin Systems.  Origin had made official Mac ports of a few older Ultima games, but these were all monochrome.  My remake was originally implemented in Think C for 1990s-era color Macintosh computers on Motorola processors running Mac OS 7.  I really liked how it was turning out, so I managed to get ahold of Richard Garriott over AOL and he liked it enough to give me permission to release it officially sometime in 1994 or 1995.

Some of the logic was originally gleaned through examining the Apple II version's 6502 assembly code.  You can find comments throughout the source referring to memory locations in this version!  There were no such things as "shrinkwrap" licenses back then which would forbid such reverse engineering.

In my spare time over the following 10+ years I would poke and prod at it to keep it running on current systems of the time; making it capable of running on Mac OS X without the need for Classic, compiling it for Intel processors to eliminate the need for Rosetta, adding support for alternate graphics, etc.  I had transitioned the project to CodeWarrior early on, then to Xcode when that came out.  By the time macOS Catalina was released with its removal of support for 32-bit executables, I had only barely touched this project for many many years.

For upload, I've mostly removed license key handling and update checking. The
current macOS build is maintained for Apple silicon and is documented below.

_Random fun fact: Ultima III was one of the first games to acknowledge non-binary gender!_

## Build on modern macOS

Requirements:

- macOS 13 or newer
- Xcode with the macOS platform and command-line tools installed
- Apple silicon Mac

Clone the repository and build an unsigned Release app:

```sh
git clone https://github.com/hongjuny/ultima3.git
cd ultima3
sh scripts/build-release-app.sh
```

The app is written to `Build/Products/Release/Ultima III.app`. Set
`DERIVED_DATA` to choose another intermediate-data root. The build contains all
required graphics, text, sound effects, and the C64 SID soundtrack. Music is
bundled as lossless mono FLAC recordings rendered from Kenneth W. Arnold's
original NTSC/MOS6581 program. A normal build is offline and needs no SoundFont
download. See [soundtrack provenance and regeneration](Resources/MusicSID/README.md).

Prefer not to compile? Download the prebuilt [Apple silicon Release
application](https://github.com/hongjuny/ultima3/releases/tag/v2.4.1) from
GitHub Releases. That published version predates this SID soundtrack change
and uses MIDI music. It is ad-hoc signed for bundle integrity, but is not
Developer ID signed or notarized.

For a Debug build directly through Xcode:

```sh
xcodebuild -project Ultima3.xcodeproj -scheme Ultima3 \
  -configuration Debug -derivedDataPath Build/DerivedData \
  CODE_SIGNING_ALLOWED=NO build
```

The same project can be opened in Xcode with:

```sh
open Ultima3.xcodeproj
```

Select the `Ultima3` scheme and press **Run** or choose **Product > Build**.
Both Debug and Release compile as unsigned local applications. The historical
DMG packaging and code-signing phases are reserved for explicit deployment
postprocessing and are not required for normal IDE builds.

## Modernization

This repository preserves a historically layered game: the gameplay rules and
data are rooted in the Apple II version, while the original presentation code
passed through 1990s Carbon and early Cocoa APIs. The modernization work keeps
that historical game behavior while separating it from the operating system.

The current runtime is organized around explicit boundaries:

- **Game core:** party state, maps, combat, spells, events, movement, saves,
  and the original game rules remain in the C sources.
- **Platform layer:** timing, preferences, paths, resource loading, windows,
  keyboard events, and application lifecycle are routed through `U3Platform`
  and `U3IO` adapters rather than being spread through game logic.
- **Rendering:** the legacy bitmap renderer is isolated behind renderer and
  Cocoa bridge interfaces. This keeps the classic raster presentation usable
  while leaving room for a future renderer.
- **Audio:** PCM effects use modern AVFoundation playback. Music uses
  lossless recordings of Kenneth W. Arnold's original C64 SID program,
  rendered with reSIDfp in NTSC/MOS6581 mode. AVAudioEngine preserves the mono
  samples, intro, and score loops while supporting independent music volume
  and mute. Native input waits service scene changes without advancing game
  turns. QuickTime/MUSI and MIDI conversions remain historical source material
  and are not bundled as a fallback. The Exodus intro retains its existing
  procedural Mockingboard-style digital-noise approximation.
- **Input and text:** keyboard commands, mouse actions, Pascal-style legacy
  strings, and modern text conversion are handled at the boundary so the game
  core does not depend on Carbon event structures or MacRoman display APIs.

The result is a native 64-bit Apple silicon macOS application that can be
built from a clean clone, while retaining the original assets and gameplay
identity. The code intentionally favors small adapters over a wholesale
rewrite: this makes future ports to another windowing, rendering, or audio
library practical without rewriting the game rules.

The modernization also includes release-oriented verification scripts for
bitmap loading, command/text behavior, and representative gameplay scenarios.
These checks are designed to catch regressions in the preserved game loop as
platform code continues to evolve.

The SID soundtrack adds silent verification for the original source and asset
hashes, canonical PCM decoding, the native player, and game-scene transitions:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests -p 'test_sid_*.py'
```

The app's `U3_AUDIO_SELF_TEST=1` diagnostic renders all ten tracks offline and
checks loop playback, volume, mute, and restart without opening an audio device.
The soundtrack uses recorded SID emulation rather than a live emulator. The
last 128 samples of each loop receive a 2.9 ms blend to avoid phase clicks;
there is no normalization, EQ, stereo processing, or tempo change. Original
music rights and offline renderer licenses are documented with the soundtrack.

## A Personal Note

Ultima III was one of my favorite games as a child. On an Apple II+ clone
with a Mockingboard, it was the first fantasy role-playing game that made that
world feel real to me. LairWare's Ultima III continued that dream: it wrapped
those childhood memories in color and wonder. I still remember playing the
LairWare game on a 68k Mac Centris with a Trinitron monitor, and I have never
forgotten that experience.

I discovered the source code after the developer generously open-sourced it
last year. Since then, I have worked to clear away the accumulated legacy
layers and make the game run on current macOS. With help from modern ChatGPT
5.5, I was able to take on a modernization effort that would otherwise have
been considerably harder. I am grateful for that help, and I offer this work
in solidarity with the Apple II friends who grew up with similar memories.

Modernization and current macOS/Apple silicon adaptation by **hongjuny**.

## Known Limitations and Future Work

The soundtrack captures the original C64 SID program rather than recreating
LairWare's QuickTime instruments. Its fixed NTSC/MOS6581 rendering settings
are documented with the [soundtrack](Resources/MusicSID/README.md). The sizzling
noise during the opening Exodus splash is still an approximation and needs
further work.

If you find a bug while playing, please report it with the screen or sequence
where it occurred, the macOS version, and whether the application was built
from source or downloaded as a release. Gameplay and compatibility reports are
welcome.

## License
Usage is provided under the [MIT License](http://opensource.org/licenses/mit-license.php). See LICENSE for the full details.

However, certain non-code assets (such as the project name, music, maps, etc) were not originally created by me. These assets are included under the assumption that copyright will no longer be actively enforced due to their age (40+ years). If you are a rights-holder and have concerns, please contact me.

To put it another way: I'm not claiming any copyright on the Ultima franchise name, NPC names, the specific maps found in this game, etc.  This license just refers to everything else here.  I'm presenting it merely as historical code in good faith, in hope that no one will care to litigate -- there is indeed no feasible way I am aware of to build this project to run on a modern system without an emulator.

Leon McNeill AKA "Beastie"
