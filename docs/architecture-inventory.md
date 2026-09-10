# Ultima III Architecture Inventory

This document records the current architecture as observed in the legacy source.
It is intentionally descriptive rather than prescriptive. The goal is to make
future refactoring decisions traceable.

## Entry Point and Runtime Shape

The app enters through `Sources/main.m`, which imports Cocoa but does not call
`NSApplicationMain`. It immediately calls `Ultima3_main()`.

`Ultima3_main()` in `Sources/UltimaMain.c` owns the legacy startup sequence:

- initialize classic Mac toolbox state
- install Apple Event handlers
- initialize graphics, sound, menus, preferences, windows, and resources
- optionally enter fullscreen
- show the splash/intro/demo/menu flow
- enter and leave the game loop

This means the current app is still structured as a classic Mac game loop with
Cocoa added around it.

## Major Source Files

`Sources/UltimaMain.c`

- owns most global state definitions
- owns `Ultima3_main()`, `MainLoop()`, `MainMenu()`, `Game()`
- dispatches letter commands and movement commands
- handles many player actions directly: attack, board, enter, equip, get chest,
  ignite, join gold, peer, steal, transact, unlock, yell, stats, whirlpool
- mixes rules with drawing, text output, input polling, sound, preferences, and
  timing

`Sources/UltimaMisc.c`

- handles map/resource data and world helpers
- reads and writes classic resources
- manages roster/party/Sosaria persistence paths
- contains monsters, moon gates, shops, shrines, item/gold helpers, map tile
  accessors
- mixes rules with Resource Manager, filesystem, drawing, text, and sound

`Sources/UltimaSpellCombat.c`

- handles spell selection, spell effects, combat setup, projectiles, battle
  flow, monster actions, victory/defeat transitions
- heavily uses `Player`, `Party`, `Monsters`, `Dungeon`, and `zp`
- mixes combat rules with input, text, drawing, sound, and map mutation

`Sources/UltimaDngn.c`

- handles dungeon movement, dungeon state, traps, secret doors, ladders, chests,
  and first-person dungeon drawing
- mixes dungeon rules with QuickDraw/GWorld rendering, text output, sound, and
  timing

`Sources/UltimaAutocombat.c`

- contains automated combat decision logic
- depends on global party, player, monster, and temporary combat state
- is comparatively closer to pure game logic, but still calls input polling in
  at least one place

`Sources/UltimaGraphics.c`

- owns tile, frame, image, intro, map, minimap, portrait, and update rendering
- depends on QuickDraw, GWorld, PixMap, CopyBits/CopyMask, image import, and
  direct pixel access
- also includes timing and some sound-triggered visual effects

`Sources/UltimaText.c`

- owns game text rendering, message lookup, prompt drawing, typed input, string
  manipulation, and speech dispatch
- depends on QuickDraw text APIs, update ports, Pascal strings, handles, and
  input functions

`Sources/UltimaMacIF.c`

- owns Carbon windows, menus, event handling, dialogs, cursors, fullscreen,
  display setup, reference windows, pause/hibernate behavior, and errors
- allocates and tears down major GWorlds
- is the current center of platform integration

`Sources/CocoaBridge.m`

- bridges Carbon windows into Cocoa `NSWindow`
- provides Cocoa-backed preferences/string/cursor helpers
- loads bundle resources
- includes QuickTime-backed sound effect playback
- uses manual Objective-C memory management

`Sources/UltimaSound.m`

- owns sound effects, music, and speech setup
- uses QuickTime, Sound Manager, and Speech Manager APIs
- exposes game-facing calls such as `PlaySoundFile`, `MusicUpdate`, `EndSong`,
  and `Speech`

## Global State

Most shared state is defined in `Sources/UltimaMain.c`. Other files import it
with `extern`, which makes dependency direction implicit.

### Candidate Game State

These values appear to represent portable game/domain state and are candidates
for `U3GameState`:

- `Player[21][65]`: roster/player records
- `oldplr[21][65]`: previous player records
- `Party[64]`: active party and world/player context
- `Monsters[256]`: active monster records
- `Talk[256]`: conversation/talk data
- `Dungeon[2048]`: dungeon map/state data
- `Macro[32]`: queued macro/input commands
- `TileArray[128]`: current tile/window data
- `careerTable[12]`, `wpnUseTable[12]`, `armUseTable[12]`
- `MoonXTable[8]`, `MoonYTable[8]`
- `LocationX[20]`, `LocationY[20]`
- `Experience[17]`
- `xpos`, `ypos`, `xs`, `ys`, `dx`, `dy`
- `gCurMapID`, `gCurMapSize`, `gMapOffset`
- `gTorch`, `gTimeNegate`, `gMoon`, `gMoonDisp`
- `dungeonLevel`, `heading`, `gExitDungeon`
- `CharX`, `CharY`, `CharTile`, `CharShape`
- `MonsterX`, `MonsterY`, `MonsterTile`, `MonsterHP`
- `gMonType`, `gMonVarType`, `gBallTileBackground`
- `zp[255]`: Apple II-style scratch/state memory

### Candidate UI/Platform State

These values are likely platform or presentation state and should not become
part of the portable game core:

- `gMainWindow`, `gShroudWindow`
- `gAppleMenu`, `gFileMenu`, `gEditMenu`, `gSpecialMenu`, `gRefMenu`
- `gTheEvent`
- `mainPort`, `gamePort`, `tilesPort`, `framePort`, `textPort`, `updatePort`
- all `PixMapHandle` and `CGrafPtr` rendering buffers
- `mainDevice`
- `UpdateRgn`
- `DialogFilterProc`
- `gCurCursor`, `gMouseState`, `gMouseKey`, `gCurMouseDir`
- `mouseX`, `mouseY`
- `gPaused`, `gMenuDone`, `gInBackground`
- `gSoundIncapable`, `gMusicIncapable`
- QuickTime `Movie` objects and Sound Manager channels
- Cocoa window/controller/cache objects

### Mixed or Transitional State

These values need more inspection before assigning ownership:

- `gDone`, `gAbort`, `gInterrupt`, `gResurrect`
- `gUpdateWhere`, `gUpdateStore`
- `gSongCurrent`, `gSongNext`, `gSongPlaying`
- `tx`, `ty`, `wx`, `wy`
- `blkSiz`, `gDepth`, `gOrgDepth`, `gUnusualSize`
- `lastSaveNumberOfMoves`
- `gDemoData`, `gParty`, `gRoster`, `gCurrentTlk`

## Platform Dependency Categories

### Event and Timing

Observed APIs:

- `WaitNextEvent`
- `TickCount`
- `Delay`
- `StillDown`
- `MenuKey`
- `AEProcessAppleEvent`

Migration direction:

- route through `U3PlatformPollInput`
- route sleeps and frame pacing through `U3PlatformWaitTicks`
- represent user intent as game commands rather than raw Carbon events

### Windows, Menus, Dialogs, Cursors

Observed APIs:

- `GetNewCWindow`
- `WindowPtr`, `WindowRef`
- `GetMenuHandle`
- `DrawMenuBar`
- `ModalDialog`
- `GetNewDialog`
- `GetDialogItem`
- `ControlHandle`
- `SetCursor`, `InitCursor`
- Cocoa `NSWindow`, `NSCursor`, `NSNib`

Migration direction:

- keep all app shell concerns outside the core
- translate platform UI events into portable commands
- move menus/options dialogs to the eventual modern shell

### Rendering

Observed APIs:

- `CGrafPtr`
- `PixMapHandle`
- `GWorld`
- `NewGWorld`
- `DisposeGWorld`
- `SetGWorld`
- `GetGWorld`
- `CopyBits`
- `CopyMask`
- `DrawPicture`
- direct pixel buffer access through `GetPixBaseAddr`

Migration direction:

- introduce `U3Renderer`
- preserve tile and text drawing semantics first
- move backend-specific buffers behind renderer implementations

### Audio and Speech

Observed APIs:

- `QuickTime/QuickTime.h`
- `EnterMovies`, `ExitMovies`, `MoviesTask`
- `Movie`, `NewMovieFromFile`, `StartMovie`, `StopMovie`
- Sound Manager channels and commands
- Speech Manager voices and speech channels

Migration direction:

- introduce `U3Audio`
- begin with sound-effect wrappers because those call sites are easy to identify
- keep music and speech as separate adapter work

### Resources and I/O

Observed APIs:

- `GetResource`
- `LoadResource`
- `ReleaseResource`
- `ChangedResource`
- `WriteResource`
- `AddResource`
- `CreateResFile`
- `OpenResFile`
- `UseResFile`
- `CloseResFile`
- `FSSpec`
- `FSMakeFSSpec`
- Cocoa bundle lookup through `NSBundle`
- preferences through `CFPreferences` and `NSUserDefaults`

Migration direction:

- introduce `U3IO`
- inventory `MainResources.rsrc`
- keep save migration separate from the new save format
- prefer explicit data files over resource fork assumptions

## Known High-Coupling Examples

`Game()` in `Sources/UltimaMain.c`

- performs the main game turn loop
- draws map and character status
- polls input
- reads preferences
- updates cursor state
- calls movement and command handlers directly

`GetKeyMouse()` in `Sources/UltimaMain.c`

- reads Carbon events
- updates music and movies on null events
- updates render state
- manages cursor/menu behavior
- dispatches menu commands
- mutates game input state through `gKeyPress`

`Cast()` and `ProcessMagic()` in `Sources/UltimaSpellCombat.c`

- perform spell selection and magic point checks
- read and write player state
- print messages
- request input
- dispatch spell effects

`DrawDungeon()` in `Sources/UltimaDngn.c`

- interprets dungeon data
- draws the first-person scene directly through QuickDraw
- emits text and update effects

These are likely early targets for wrapper introduction, but not necessarily for
large rewrites.

## Initial Refactoring Strategy

The safest first code changes should be adapter-only:

1. Add abstraction headers with no call-site changes.
2. Add legacy adapter implementations that call the existing functions.
3. Start with audio wrappers around `PlaySoundFile`.
4. Move text/message calls behind render/text wrappers.
5. Move input polling behind platform wrappers.
6. Only then begin extracting `U3GameState`.

This order keeps behavior changes small and gives every later platform port a
stable dependency direction.

## Initial Boundary Headers

The first modernization boundary headers have been added without changing legacy
call sites:

- `Sources/U3Types.h`: shared portable primitive types, directions, commands,
  and input events
- `Sources/U3GameState.h`: portable state container that mirrors the highest
  value legacy global arrays and world-position fields
- `Sources/U3Platform.h`: host services for input, timing, random numbers,
  preferences, and quit state
- `Sources/U3Renderer.h`: drawing/text intent API for map, dungeon, tiles,
  prompt, messages, and characters
- `Sources/U3Audio.h`: sound effect, music, volume, and speech intent API
- `Sources/U3IO.h`: resource and save/load intent API

These headers intentionally avoid Carbon, QuickDraw, QuickTime, Cocoa, and
Resource Manager types. Adapter implementations may still use those APIs while
the legacy backend exists.

## First Audio Adapter Step

`Sources/U3AudioLegacy.m` implements the portable `U3Audio` API by mapping
`U3SoundEffect` values back to the existing legacy sound names and delegating to
`PlaySoundFile`.

The first conversion pass moved game-facing sound effect calls from direct
`PlaySoundFile(CFSTR(...))` use to `U3AudioPlaySound(...)` in:

- `Sources/UltimaDngn.c`
- `Sources/UltimaMain.c`
- `Sources/UltimaMisc.c`
- `Sources/UltimaSpellCombat.c`
- `Sources/UltimaGraphics.c`
- `Sources/UltimaNewMap.c`

`Sources/UltimaSound.m` intentionally still contains direct `PlaySoundFile` use
because it is now the legacy implementation behind the adapter.

Music and speech are not fully abstracted yet. Calls such as `MusicUpdate`,
`EndSong`, `SpeakMessages`, and `Speech` remain as follow-up work.
