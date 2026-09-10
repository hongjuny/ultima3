# Ultima III Modernization Roadmap

This document is the working master plan for modernizing the LairWare Ultima III
codebase while preserving the original game behavior.

## Guiding Principle

The modernization target is not just a newer macOS build. The target is a
portable game architecture where the original game rules can survive future UI,
renderer, audio, storage, and platform changes.

The core strategy is:

- preserve the legacy code as the behavioral reference
- separate game logic from platform APIs
- route all system interaction through explicit abstraction layers
- replace backend implementations incrementally
- keep each step small enough to validate against the original behavior

## Current Understanding

The repository is a layered historical codebase:

- original game logic is represented as C routines and Apple II-style data arrays
- many routines retain comments referencing original 6502 memory addresses
- the game state is mostly global, including `Player`, `Party`, `Monsters`,
  `Dungeon`, `Macro`, and `zp`
- the Mac runtime is built around Carbon, QuickDraw, Resource Manager,
  QuickTime, Sound Manager, and a small Cocoa bridge
- the app entry point is `main.m -> Ultima3_main()`, not `NSApplicationMain`
- Cocoa currently acts mostly as a compatibility bridge, not as the main app
  architecture

The legacy Xcode project currently targets obsolete technology:

- `SDKROOT = macosx10.6`
- `ARCHS = i386` in Debug
- `ARCHS = ppc i386` in Release
- `MACOSX_DEPLOYMENT_TARGET = 10.4`
- `Carbon.framework`
- `QuickTime.framework`
- `PBXRezBuildPhase`
- `com.apple.compilers.gcc.4_0`

On a current Xcode, the project can be inspected but does not build.

## Target Architecture

The desired long-term shape is:

- `core`: game state, rules, turn processing, combat, spells, map mutation
- `platform`: time, random numbers, event pump, preferences, paths, system glue
- `renderer`: tiles, text, frames, images, cursors, screen updates
- `audio`: sound effects, music, speech
- `io`: resource loading, save/load, data migration
- `ui shell`: windowing, menus, options dialogs, fullscreen, input mapping

The core should not call Carbon, Cocoa, QuickDraw, QuickTime, filesystem APIs, or
sound APIs directly. It should express intent through stable interfaces such as:

- render this map or tile region
- print this message
- play this sound
- request directional input
- read or write game data
- wait for or poll an input event

## Phase 0: Preserve the Baseline

Goal: make sure the historical starting point is understood and protected.

Action items:

- record the current branch and working tree state
- avoid rewriting or reformatting legacy files before boundaries exist
- capture the current Xcode build failure as expected baseline behavior
- normalize source and text metadata files to UTF-8 before larger refactors
- treat the legacy project as the behavioral oracle, even though it does not
  build on current macOS

Completion criteria:

- baseline limitations are documented
- source and text metadata files decode cleanly as UTF-8
- no accidental source cleanup or unrelated asset churn
- future work can distinguish intentional modernization from historical code

## Phase 1: Architecture Inventory

Goal: map ownership and dependencies before moving code.

Working inventory: `docs/architecture-inventory.md`

Action items:

- document each major source file's role
- list global state and its owning subsystem
- list every direct platform dependency by category
- identify routines that mix game rules with rendering, input, sound, or storage
- identify data still coming from classic resource files

Initial file map:

- `Sources/UltimaMain.c`: main lifecycle, game loop, menu flow, movement,
  commands, inventory actions, player status
- `Sources/UltimaMisc.c`: maps, resource files, roster/party persistence,
  monsters, shops, world state helpers
- `Sources/UltimaSpellCombat.c`: spells, combat, projectiles, battle flow
- `Sources/UltimaDngn.c`: dungeon state, dungeon movement, dungeon rendering
- `Sources/UltimaAutocombat.c`: automated combat decisions
- `Sources/UltimaGraphics.c`: QuickDraw/GWorld rendering and image loading
- `Sources/UltimaText.c`: text layout, messages, input text, string helpers
- `Sources/UltimaMacIF.c`: Carbon windows, menus, events, dialogs, cursors,
  system integration
- `Sources/CocoaBridge.m`: Cocoa compatibility bridge for windows, prefs,
  strings, cursors, and QuickTime sound playback
- `Sources/UltimaSound.m`: sound, music, speech, QuickTime/Sound Manager

Completion criteria:

- a contributor can tell where a behavior lives before editing
- platform calls have a known migration owner
- the next abstraction layer can be designed from evidence rather than guesses

## Phase 2: Define Abstraction Boundaries

Goal: introduce stable interfaces without changing behavior.

Action items:

- add abstraction headers for platform, rendering, audio, I/O, and game state
- keep initial implementations as wrappers around the existing legacy functions
- avoid introducing a new UI framework in this phase
- prefer intent-oriented APIs over backend-specific APIs

Initial interfaces added:

- `U3Types.h`
- `U3GameState.h`
- `U3Platform.h`
- `U3Renderer.h`
- `U3Audio.h`
- `U3IO.h`

Candidate calls:

- `U3PlatformPollInput`
- `U3PlatformWaitTicks`
- `U3PlatformRandom`
- `U3RenderDrawMap`
- `U3RenderDrawDungeon`
- `U3RenderPrintMessage`
- `U3RenderPrintText`
- `U3AudioPlaySound`
- `U3AudioStartMusic`
- `U3AudioStopMusic`
- `U3IOLoadResource`
- `U3IOSaveGame`
- `U3IOLoadGame`

Completion criteria:

- all new code has a clear dependency direction
- legacy platform APIs are reachable through wrappers
- no new direct Carbon, QuickDraw, QuickTime, or Resource Manager calls are added
  outside adapter implementations

## Phase 3: First Isolation Pass

Goal: move direct effects behind the new boundaries while preserving legacy flow.

Action items:

- wrap `Draw*`, `UPrint*`, `PlaySoundFile`, `GetKeyMouse`, and `WaitKeyMouse`
  call sites gradually
- keep wrapper implementations delegating to the current legacy code
- avoid changing game rules while moving call sites
- use small commits grouped by dependency category

Suggested order:

1. audio calls
2. text/message calls
3. input polling/waiting
4. map and tile rendering calls
5. resource loading and save/load calls

Completion criteria:

- core-like routines no longer know the concrete backend function names
- behavioral changes are either absent or explicitly documented
- call sites become searchable by abstraction category

## Phase 4: Encapsulate Game State

Goal: turn global memory-style state into an explicit state object.

Action items:

- define `U3GameState`
- move the highest-value global arrays into that structure first:
  `Player`, `Party`, `Monsters`, `Dungeon`, `Macro`, `zp`
- add accessor helpers where raw byte offsets have domain meaning
- migrate new or recently touched functions to accept `U3GameState *state`
- keep old globals temporarily as a compatibility bridge where needed

Completion criteria:

- new logic can be written without adding more globals
- game state can eventually be serialized, tested, or run headless
- platform/UI state is not mixed into `U3GameState`

## Phase 5: Modernize Resources and Storage

Goal: remove classic Resource Manager and resource fork assumptions.

Action items:

- inventory `MainResources.rsrc` contents
- identify equivalent modern files already present in `Resources/English.lproj`
  and `Resources/Graphics`
- move strings toward plist/json/string tables
- move maps, monsters, roster templates, and demo data toward explicit data
  files or binary blobs with documented layouts
- put save/load behind `U3IO`
- preserve legacy save migration only as an adapter concern

Completion criteria:

- the game can load required data without `GetResource`
- save/load format ownership is explicit
- resource formats are documented enough for future tools

## Phase 6: Build a New Runtime Shell

Goal: run the isolated core from a modern frontend.

Action items:

- create a new macOS target or separate app shell
- choose AppKit, SwiftUI, SDL, or another backend deliberately
- start with a minimal playable loop: tile view, text log, keyboard input
- connect through `U3Platform`, `U3Renderer`, `U3Audio`, and `U3IO`
- avoid depending on the old Xcode target for the new runtime

Completion criteria:

- a modern build can launch independently of Carbon/QuickTime/Rez
- the same core-facing interfaces can support another backend later
- legacy rendering and modern rendering can coexist during transition

## Phase 7: Verification Harness

Goal: make refactoring safer by testing behavior below the UI.

Action items:

- add deterministic random support through `U3PlatformRandom`
- add headless command execution for core turns
- record small golden scenarios for movement, combat, spells, inventory, and
  dungeon behavior
- compare key state arrays before and after selected actions

Completion criteria:

- core behavior can be tested without opening a window
- regressions can be detected before UI testing
- modernization work can proceed in smaller, safer pieces

## Working Rules

- Do not rewrite large legacy files just for style.
- Do not mix UI modernization with game-rule changes in the same step.
- Prefer adapters first, replacement backends second.
- Keep original byte layouts documented until they are fully replaced.
- When changing behavior intentionally, record the reason.
- Treat portability as a dependency-direction problem, not just a framework
  choice.

## Near-Term Next Steps

Recommended immediate sequence:

0. normalize legacy text encodings before larger source edits
   - status: done for source files, markdown, project metadata, plist/string
     metadata, and workspace metadata; `.editorconfig` now declares UTF-8
1. finish Phase 1 inventory in this document or a companion architecture file
   - status: done in `docs/architecture-inventory.md`
2. add Phase 2 abstraction headers with no behavior changes
   - status: done for `U3Types`, `U3GameState`, `U3Platform`, `U3Renderer`,
     `U3Audio`, and `U3IO`
3. implement the first adapter for audio calls
   - status: started with `Sources/U3AudioLegacy.m`
4. wrap sound effect call sites
   - status: game-facing `PlaySoundFile(CFSTR(...))` call sites have been
     moved behind `U3AudioPlaySound`; `UltimaSound.m` still owns the legacy
     implementation
5. add a small verification note showing that wrappers still delegate to legacy
   behavior
   - status: `U3AudioLegacy.m` syntax-checks with the legacy prefix header;
     full Xcode build still fails for the known legacy project reasons
6. move remaining game-facing audio lifecycle, music, error tone, and speech
   calls behind `U3Audio`
   - status: done for `ApplyVolumePreferences`, `OpenChannel`, `CloseChannel`,
     `SetUpMusic`, `CloseMusic`, `SetUpSpeech`, `MusicUpdate`, `EndSong`,
     `ErrorTone`, `SpeakMessages`, and direct `Speech` call sites outside the
     legacy audio backend
7. start the renderer/text isolation pass
   - status: started with `Sources/U3RendererLegacy.m`; game-facing
     `UPrintMessage`, `UPrintMessageRewrapped`, `DrawPrompt`, `ClearBottom`,
     and `ClearTiles` call sites now route through `U3Renderer`
8. continue renderer/text isolation for cursor-positioned text output
   - status: game-facing `UPrintWin`, `UPrint`, `UPrintChar`, `UPrintNum`, and
     `UPrintNumPad` call sites now route through transitional `U3Renderer`
     Pascal-string and numeric output APIs
9. start platform/input isolation
   - status: started with `Sources/U3PlatformLegacy.m`; game-facing
     `GetKeyMouse`, `WaitKeyMouse`, `CursorKey`, `GetDirection`, `RandNum`, and
     `ThreadSleepTicks` call sites now route through `U3Platform`; legacy
     definitions and backend-internal calls remain as adapter implementation
     details
10. start preferences isolation
   - status: `U3PlatformLegacy.m` now maps portable preference keys onto the
     existing `U3Pref*` keys, including boolean, integer, existence, and
     transitional Pascal-string preference reads; direct `CFPreferences` access
     has been removed from `UltimaMain.c`, `UltimaMisc.c`,
     `UltimaSpellCombat.c`, `UltimaAutocombat.c`, `UltimaNew.c`,
     `UltimaText.c`, `UltimaGraphics.c`, `UltimaSound.m`, and
     `UltimaMacIF.c`; `PrefsDialog.m` now routes direct preference reads,
     default writes, removals, and volume normalization through `U3Platform`;
     remaining live `NSUserDefaults` access is limited to Cocoa UI-only
     behavior, existing KVO bindings, and Cocoa bridge app-notification helpers
11. add repeatable boundary checks
   - status: `scripts/check-modernization-boundaries.sh` now verifies that
     runtime files do not access preference storage directly, and that selected
     core-like files do not regress to legacy input/timing calls
12. start I/O and Resource Manager isolation
   - status: started with `Sources/U3IOLegacy.m`; `U3IOLoadResource` and
     `U3IOOpenMutableResource` now wrap legacy
     `GetResource`/`LoadResource`/`ChangedResource`/`WriteResource`/
     `ReleaseResource` access, and `GetMiscStuff`/`PutMiscStuff` now load and
     save `MISC` tables through `U3IO`
13. route roster and party resources through U3IO
   - status: `GetRoster`/`PutRoster` and `GetParty`/`PutParty` now use
     `U3IOLoadResource` and `U3IOOpenMutableResource`; the old `gRoster` and
     `gParty` global resource handles have been removed
14. route Sosaria save resources through U3IO
   - status: `PutSosaria` now writes current `MAPS` and `MONS` resources
     through `U3IOOpenMutableResource`, with size checks before commit
15. route map load resources through U3IO
   - status: `LoadUltimaMap` now opens `MAPS` through
     `U3IOOpenMutableResource`, handles the legacy map 419 resize through
     `U3IOResizeMutableResource`, and loads `MONS`/`TLKS` through
     `U3IOLoadResource`; the in-memory `Map` handle is still legacy Mac heap
     state and should move during the game-state encapsulation pass
16. route Sosaria reset resources through U3IO
   - status: `ResetSosaria` now copies original `MAPS`, `MONS`, and selected
     `MISC` resources into current resources through `U3IO`, removing direct
     Resource Manager access from the reset path; remaining direct resource
     calls in `UltimaMisc.c` are now concentrated around demo loading and new
     game/template initialization
17. route demo data through U3IO
   - status: `GetDemoRsrc` now loads the `DEMO` resource into a `U3DataBuffer`,
     and `DemoUpdate` reads that buffer without seeing a Resource Manager
     `Handle`; remaining direct resource creation/copy calls are concentrated
     in `OpenRstr`
18. route roster resource template creation through U3IO
   - status: `OpenRstr` now creates empty `MONS`/`PREF` resources and copies
     template `MAPS`, `PRTY`, `ROST`, and `MISC` resources through `U3IO`;
     direct Resource Manager creation/copy calls have been removed from the
     function
19. move save-container file handling into U3IO
   - status: `U3IOOpenSaveContainer` now owns the legacy Preferences-folder
     roster lookup, resource-file open/create, old `:Roster` migration, and
     save-container refnum; `OpenRstr` now only reacts to the high-level open
     result and initializes template resources when the container is empty
20. route graphics-only legacy resources through U3IO
   - status: `FadeOnExodusUltima` now reads the legacy `snd ` sample resource
     through `U3IO`, `WriteLordBritish` reads `SGNT` signature data through
     `U3IO`, and remaining `BlockMoveData` calls in `UltimaGraphics.c` have
     been replaced with standard `memcpy`
21. route remaining graphics timing/random/sound effects through abstractions
   - status: direct `TickCount`, `Random`, `ThreadSleepTicks`,
     `GetKeyMouse`, `SndCommand`, and `SndDoImmediate` usage in
     `UltimaGraphics.c` now routes through `U3Platform` or transitional
     `U3Audio` APIs; low-level Sound Manager calls for the Exodus fade effect
     are isolated in `U3AudioLegacy.m`
22. route combat screen resources and cursor hiding through abstractions
   - status: `GetScreen` now loads `CONS` combat screen templates through
     `U3IO`, validates the buffer before copying into combat arrays, and no
     longer sees Resource Manager handles; combat auto-mode cursor hiding now
     routes through `U3PlatformObscureCursor`; `DamageMonster` no longer needs
     Carbon `NumToString`/`BlockMove` for its experience message append
23. shrink the legacy audio public surface
   - status: `UltimaSound.h` now exposes only the non-gameplay UI hook still
     used by `UltimaMacIF.c`; `U3AudioLegacy.m` calls the old sound, music, and
     speech backend entry points through private prototypes, so gameplay code
     has fewer ways to bypass `U3Audio`
24. start random-map generation boundary cleanup
   - status: `UltimaNewMap.c` no longer calls legacy `Random`, `TickCount`, or
     `ReleaseResource` directly; map generation randomness and map-preview
     cursor blink timing now route through `U3Platform`, while the temporary
     `PICT` resource release goes through `U3IOReleaseLegacyResourceHandle`
25. route dungeon timing and combat event flushing through platform services
   - status: the active dungeon pass timer in `UltimaDngn.c` now uses
     `U3PlatformTickCount`; combat startup no longer calls Carbon
     `FlushEvents` directly and instead uses `U3PlatformFlushInputEvents`
26. route main-flow timing and event flushing through platform services
   - status: active `TickCount`, `FlushEvents`, and `ObscureCursor` calls in
     `UltimaMain.c` now route through `U3Platform`; only the legacy backend
     definitions for `RandNum`, `WaitKeyMouse`, and `GetKeyMouse` still remain
     in that file as transitional implementation details
27. route Exodus victory effect timing and raw random through platform services
   - status: the remaining active `Random` and `TickCount` calls in
     `UltimaMisc.c` now route through `U3PlatformRandomRaw` and
     `U3PlatformTickCount`; boundary checks prevent those direct calls from
     returning to the miscellaneous gameplay file
28. clean up new-game timing and dead Resource Manager fallbacks
   - status: `UltimaNew.c` no longer uses active `TickCount` or stale
     `ReleaseResource` fallback blocks; button-highlight timing now routes
     through `U3PlatformTickCount`, and obsolete commented PICT/prefs/TLKS dump
     implementations were removed so boundary checks reflect live code
29. route Mac interface timing, event flushing, and resource release through abstractions
   - status: `UltimaMacIF.c` no longer calls active `FlushEvents`,
     `TickCount`, or `ReleaseResource` directly; toolbox event flushing and
     drag timing now route through `U3Platform`, character dialog resource
     release goes through `U3IO`, and the obsolete commented About animation
     fallback was removed so the boundary script can guard the file directly
30. clean up text-input legacy input fallbacks
   - status: `UltimaText.c` already routes active cursor and mouse/key polling
     through `U3Platform`; obsolete commented numeric input implementations
     that still referenced `CursorKey` were removed, and the boundary script
     now guards text input against direct legacy input/timing calls
31. clean up dungeon legacy picture fallbacks
   - status: `UltimaDngn.c` already loads dungeon shape and mask artwork
     through modern image assets; obsolete commented PICT fallback code was
     removed, and the boundary script now guards the dungeon file against
     direct Resource Manager picture access
32. modernize the first Xcode build boundary
   - status: the project now targets the current macOS SDK with standard
     architectures, code signing disabled for local builds, no legacy Rez
     build phase, and no QuickTime framework dependency; QuickTime movie audio
     playback has been replaced with a transitional `NSSound` backend, and the
     source tree now compiles for arm64 until the link step
   - remaining: the link step is blocked by removed Carbon, QuickDraw, Sound
     Manager, Resource Manager, Display Manager, and Navigation Services
     symbols; the next modernization pass should replace or shim those APIs at
     the renderer, audio, windowing, and save-file boundaries instead of
     scattering compatibility stubs through gameplay code
33. remove low-risk removed Toolbox link blockers
   - status: legacy memory-copy, numeric Pascal-string conversion, raw random,
     system beep, and memory-query calls now resolve through `CarbonShunts`;
     `CarbonShunts` itself no longer calls removed Carbon port/menu/window
     helper APIs, and old Display Manager screen-device probing in
     `UltimaNew.c` has been reduced to current-display bounds plus a fixed
     32-bit display depth assumption
   - remaining: the build still links against a broad set of removed
     QuickDraw, Dialog Manager, Menu Manager, Resource Manager, Graphics
     Importer, and Sound Manager symbols; those should be grouped behind
     renderer, dialog/window, asset-loading, and audio compatibility seams next
34. make the arm64 app target link on the current macOS SDK
   - status: removed Sound Manager immediate-command usage and concentrated
     the remaining removed Toolbox, QuickDraw, Dialog/Menu, GWorld, Graphics
     Importer, and FSSpec resource-file entry points into transitional
     `CarbonShunts` implementations; the Debug app target now links for arm64
     with the current macOS SDK
   - remaining: many of these shunts are deliberately non-rendering or
     placeholder implementations, so the next pass must replace them with real
     `U3Renderer`, `U3IO`, and Cocoa window/dialog implementations before the
     app can be considered playable
35. stabilize transitional startup on current macOS
   - status: guarded legacy cursor and display-depth initialization against nil
     Toolbox state, assumed a modern 32-bit display surface, and shunted early
     Menu Manager, Window Manager, and region calls that can abort when the
     transitional Carbon window/menu handles are absent; the Debug arm64 app
     now builds and a direct executable smoke run returns exit code 0
   - remaining: this is still a non-rendering compatibility bridge; the next
     milestone should create a real Cocoa-owned window/surface and route draw
     calls through `U3Renderer` instead of relying on no-op window/menu shims
36. introduce a Cocoa-owned main surface fallback
   - status: `CocoaBridge` now owns a minimal AppKit window and black drawing
     view that can be created without a Carbon `WindowRef`; `WindowInit` falls
     back to this surface when legacy `NewCWindow` is unavailable, giving the
     modernization effort a real Cocoa display owner to grow into
   - remaining: the process still exits quickly under the transitional
     `WaitNextEvent` input path, and game rendering still flows through
     QuickDraw-shaped calls; the next pass should route event pumping and
     keyboard input through `U3Platform`/AppKit, then attach renderer output to
     the Cocoa view
37. start routing platform input through AppKit
   - status: `CocoaBridge` now exposes a minimal AppKit key/mouse polling path
     and event pump; `U3PlatformLegacy` uses that path for input polling,
     blocking key waits, and event flushing before falling back to the legacy
     game event code
   - remaining: the app still starts from a synchronous `Ultima3_main` entry
     instead of an `NSApplication` delegate/run-loop model, so a direct smoke
     run returns cleanly instead of staying resident; the next lifecycle pass
     should move launch ownership to AppKit and run the game loop from a
     controlled application callback
38. keep the Cocoa app resident after transitional startup
   - status: `main.m` now enters the AppKit run loop after `Ultima3_main`
     returns when a Cocoa main surface exists, while `U3_SKIP_APP_RUN=1`
     preserves a deterministic command-line smoke path for automated checks
   - remaining: this is a bridge, not the final lifecycle; the game loop still
     runs synchronously before AppKit owns launch, so the next pass should move
     game startup behind an application delegate or controller object that can
     coordinate rendering, input, and shutdown explicitly
