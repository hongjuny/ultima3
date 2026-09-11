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

### 2026-09-10: Audio asset compatibility

- Converted the legacy IMA ADPCM WAV effects into `Resources/SoundsPCM` as PCM16 WAV assets. Current macOS `afplay` rejects the original files with AudioFileOpen error `-9405`.
- The Cocoa audio boundary now resolves effects from the PCM bundle while retaining the original legacy assets for reference.
- Verified that all bundled `Song_*.mov` files contain only QuickTime `musi` data tracks and no playable audio track. The modern AVFoundation path now reports this explicitly instead of silently attempting playback.
- Converted all ten `musi` tracks into standard MIDI files with a repeatable converter at `scripts/convert-quicktime-music.py`. The game now loads those files through `AVMIDIPlayer`, avoiding QuickTime movie playback entirely.
- Remaining audio validation: compare MIDI playback on a real macOS audio device with the original QuickTime rendering and adjust General MIDI instrument/channel mappings if needed.

### 2026-09-10: Party formation and text bridge

- Routed Carbon theme text calls through the Cocoa renderer. This fixes blank
  modern-font messages after forming a party while preserving the classic
  bitmap-font path.
- Exposed the complete party-save transition as `U3StorePartySelection` and
  added `U3_PARTY_FLOW_CHECK`, which exercises character creation, roster/party
  persistence, world reset, and the first world render in one headless test.
- The party flow now passes through `Game()` on Apple Silicon without a crash;
  GUI validation of the native picker and Journey Onward remains a manual test.
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
39. recover bitmap rendering (in progress)
   - basic rectangle and text shunts now enqueue Cocoa drawing commands
   - added platform-independent `U3Bitmap` storage with top-down RGBX8 pixels,
     nearest-neighbor source copies, clipping, and overlapping self-copy support
   - validation: `sh scripts/test-bitmap.sh` exercises scaling, clipping,
     self-copy, invalid dimensions, and disposal under address/undefined-behavior
     sanitizers; no Apple frameworks are needed for these tests
   - integrated `U3Bitmap` into the app target: NewGWorld now allocates owned
     32-bit storage and a PixMap handle; disposal and current-port selection
     track these allocations. Requested legacy depths are normalized to 32-bit
   - basic rectangle/text drawing targets the selected offscreen buffer;
     CopyBits handles srcCopy/ditherCopy between registered buffers and sends
     pixels to the Cocoa main surface. Dithering is unnecessary for this
     32-bit path; other transfer modes remain unsupported
   - `U3_RENDER_SELF_TEST=1` runs an app-linked diagnostic for nonzero-origin
     GWorld allocation, drawing, PixMap access, scaled CopyBits, and disposal
   - replaced FSSpec/QuickTime image import in GetGraphicTiledFile and
     DrawNamedImage with URL-based decoding into owned bitmap storage. Raster
     tiles are cropped individually and scaled with nearest-neighbor sampling;
     PDFs render their first page through Core Graphics at the requested size
   - image diagnostics verify asymmetric tile orientation/scaling, missing-file
     failure, and nonblack decoded pixels from bundled PNG, JPEG, and PDF assets
   - main surface now retains a framebuffer instead of a capped command history;
     CopyBits supports screen readback, restore, and overlapping screen copies
   - CopyMask uses binary dark-copy/light-preserve masks, including scaled and
     aliased masks. Unit tests and app-linked screen compositing tests cover it
   - Cocoa view rendering passes a pixel comparison against decoded Exodus.png;
     `U3_RENDER_PREVIEW=/tmp/u3-render-check.png` alongside the self-test flag
     exports the diagnostic image. This is an offscreen view test, not a complete
     game-window or gameplay verification
   - next: verify the real startup/game rendering sequence and its tile layout
   - remaining fidelity work includes transfer modes, clipping regions,
     legacy pixel-format assumptions, and per-port pen/color/text state
40. trace the real startup path (in progress)
   - corrected prior smoke-test interpretation: ExitToShell returned zero after
     MenuBarInit failed, before window creation. Zero exit alone did not prove
     successful initialization. Fatal HandleError exits now return failure and
     log their error code, description ID, and resource ID
   - missing legacy menu resources now select a minimal native application menu
     with Quit; the remaining game menu commands still need native equivalents
   - WindowInit captures the main port after creating the Cocoa window; added
     explicit rendering prototypes to prevent implicit-int pointer truncation
     on arm64, plus a GetPixBaseAddr bridge for owned PixMap handles
   - widened transitional GWorld storage to accommodate the existing 7168-pixel
     font atlas; rowBytes is interpreted internally using the legacy 0x7fff mask
   - U3_STARTUP_RENDER_CHECK=/tmp/u3-startup-render.png runs WindowInit,
     SetUpGWorlds, GetGraphics, DrawFrame, and a named logo draw, then exports
     the framebuffer and exits. Verify the completed marker AND image, not only
     the exit code. Observed a 1280x768 frame and correctly oriented Exodus logo
   - this explicit diagnostic bypasses roster loading and is not the normal
     intro or gameplay sequence. Normal startup now reaches OpenRstr, where
     U3IOOpenSaveContainer fails through the remaining FSSpec/resource-fork shims
   - next prerequisite for normal startup: modernize save-container I/O, then
     continue tracing resource loading, intro animation, and the main menu
41. replace resource-fork save storage
   - `U3IOOpenSaveContainer` now opens a versioned binary plist at
     `~/Library/Application Support/LairWare/Ultima III/Roster-v1.plist`.
     Resource keys preserve the four-character type (hex) and signed resource ID;
     values preserve the original game bytes. Bundled defaults still load through
     the read-only legacy resource loader
   - mutable resources are private copies: closing without commit discards edits;
     commit atomically writes a candidate file before publishing the new in-memory
     state. New-roster initialization stays in memory until its final flush
   - required records and minimum lengths are validated before opening/writing;
     malformed or unsupported containers fail without being replaced
   - legacy rosters in Preferences or next to the app are detected when no modern
     save exists; creation stops with a migration-required error. Importing those
     resource forks is still pending; no legacy roster is modified
   - `U3_IO_SELF_TEST=1` executes real OpenRstr initialization in an isolated
     temporary directory, then verifies reopen, commit, discard, resize, write
     failure rollback, malformed plist rejection, and truncated-record rejection
   - `U3_SAVE_DIRECTORY` overrides storage for isolated diagnostics. Tests passed
     without touching the user's save directory; full normal startup after this
     change has not yet been verified
   - remaining: legacy import, user-visible save failure reporting, multi-record
     game-save transactions, and concurrent-process coordination. The high-level
     U3IOLoadGame/U3IOSaveGame state APIs remain stubs; existing gameplay currently
     uses the resource-level U3IO calls implemented here
42. reach the main menu through normal initialization
   - normal startup now passes roster loading. Found and removed the Cocoa-path
     wait on an unavailable Carbon display-mode dialog; legacy display-resolution
     changes and restoration are skipped for the Cocoa-owned surface without
     changing the stored fullscreen preference
   - declared SetRect/OffsetRect/InsetRect explicitly. Fractional logo dimensions
     previously crossed an implicitly declared call with the wrong argument ABI,
     causing invalid intro GWorld dimensions on arm64
   - AppKit input polling now returns no-input to its caller when a Cocoa window
     exists, instead of falling back into Carbon WaitNextEvent after a timeout
   - U3_BOOT_CHECK=/tmp/u3-boot-menu.png plus an explicit U3_SAVE_DIRECTORY runs
     normal initialization and intro animation, posts space key events at intro
     and demo waits, captures the actual main menu, then exits. It has a 90-second
     timeout and requires isolated save storage
   - verified intro-ready and main-menu-completed markers, successful exit, and
     the exported 1280x768 framebuffer with Exodus and all four menu buttons.
     This test uses synthesized keys; it does not validate physical input,
     mouse hit-testing, menu actions, or playable gameplay
   - next: exercise Organize a Party and Journey Onward, replace remaining dialog
     dependencies, and verify movement/rendering in the game world
43. replace the Form Party dialog (in progress toward game entry)
   - Cocoa surfaces use a native four-slot party picker. Empty and duplicate
     selections disable Form Party; occupied roster members are disabled and
     cancellation leaves the game state untouched
   - the bridge receives only bounded display names and availability flags;
     selection validation and party construction remain in UltimaNew.c. The
     legacy dialog shares the same validation/construction/persistence workflow
   - selection tests cover empty, duplicate, invalid, nonexistent, occupied, and
     already-formed cases, plus slot ordering, member flags, and starting position.
     These tests run with U3_RENDER_SELF_TEST and restore the original game globals
   - arm64 build, selection tests, renderer checks, and save-container tests pass;
     interactive native dialog behavior and end-to-end Form Party persistence
     have not yet been verified. Multi-record save atomicity remains pending
   - next blocker for a new roster: CharacterCreateDialog and its roster-slot
     selection still use legacy UI. Restore character creation before claiming
     that a new player can use Journey Onward to enter the world
44. restore new-character creation on the Cocoa path
   - CreateChar now uses one native editor for empty roster slot, MacRoman name,
     sex, race, class, and four ability scores; it bypasses the unavailable legacy
     roster-selection and character dialogs without mutating the roster on cancel
   - portable draft validation retains the legacy rules: 1-12 name bytes,
     scores 5-25 each, and total at most 50 (unspent points remain permitted).
     The editor displays remaining points and disables Create for invalid input
   - record construction stays in game code and retains initial health, HP,
     food, gold, cloth, and dagger values. Existing slots cannot be overwritten;
     failed roster writes restore the previous in-memory slot
   - character validation/default-record tests and an isolated creation/save/
     reopen test pass, together with the arm64 build and renderer checks
   - interactive native editor behavior is not yet verified. Suggested class
     presets, random-name selection, and legacy race/class help are not yet
     reproduced; the current editor provides manual allocation and selection
   - next: validate native character creation plus party formation end to end,
     then execute Journey Onward and inspect world rendering and movement
45. verify first world frame after Journey Onward setup
   - added a bounded `U3_WORLD_RENDER_CHECK=/tmp/u3-world-render.png` diagnostic:
     it runs normal initialization, creates a default character in an isolated
     save container, forms a one-member party, enters `Game`, loads dungeon and
     portrait graphics plus Sosaria, draws the first map frame, exports the
     framebuffer, and exits
   - fixed diagnostic setup to clear stale party state before forming a new party;
     the normal save path may contain an existing active party
   - verified markers through `Game` and a 1280x768 image containing terrain,
     water, mountains, castles, the party character, portrait, and status-panel
     frame. This is the first end-to-end evidence that the recovered bitmap
     renderer is receiving real game-world draw calls
   - music playback is disabled only for this diagnostic while the audio
     backend is being migrated; the legacy `NSSound` movie path is no longer
     used after item 47
   - next: validate physical keyboard/mouse input and movement from this world
     frame, then restore music through a current audio decoder/player and replace
     remaining Carbon dialogs and menu actions
46. verify keyboard movement through the Cocoa input boundary
   - `U3_WORLD_INPUT_CHECK=/tmp/u3-world-input.png` runs the same isolated
     character/party setup, enters `Game`, posts an AppKit `6` key event, and
     captures the resulting world frame after one command
   - observed `World input: before (42,20)` and `key=54 after (43,20)`, proving
     the event reached `U3PlatformGetKeyMouse`, the game command switch, and
     `East()`. The exported frame still contains the live map and status panel
   - the diagnostic disables music to keep the input check deterministic.
     Physical keyboard focus, mouse movement, held-key repeat,
     blocked terrain, diagonal movement, and command menus remain unverified
   - next: validate real AppKit key/mouse events interactively and continue
     removing legacy modal dialogs
47. replace legacy QuickTime `NSSound` music playback with AVFoundation `AVMIDIPlayer` backed by portable MIDI assets
   - preserve looping, volume, stop, and song-transition behavior while keeping
     the legacy game-facing audio contract unchanged
   - link AVFoundation/CoreMedia explicitly and add a bundled music decode
     self-test for the arm64 build
48. replace Carbon-era `NSApplicationLoad` bootstrap with AppKit application
   initialization through `sharedApplication`, avoiding current macOS aborts
   during headless renderer/input diagnostics
   - headless diagnostics now bypass AppKit windows/menus, retain the bitmap
     framebuffer, and provide a virtual key queue for deterministic input tests
   - screen-size fallback keeps offscreen QuickDraw worlds valid when no display
     session is available
   - rebuilt arm64 world-input diagnostic passes again and exports a 1280x768
     world frame; bitmap, save-container, audio, and boundary checks also pass
49. route synchronous sound-effect waits through the Cocoa event pump whenever
   the modern surface exists, retaining Carbon event handling only as fallback
50. preserve raw keyboard/mouse metadata at the platform input boundary
   - `U3PlatformPollInput` now fills `U3InputEvent.rawKey` and `isMouse` instead
     of discarding the event
   - existing character-based game input remains compatible; world movement
     regression still passes on arm64
   - next: carry Cocoa mouse coordinates into the legacy `Point` path and map
     clicks to the existing game widgets/commands
51. carry Cocoa mouse-down coordinates through `GetMouse` and preserve the
   legacy mouse-input flag in the platform adapter
52. translate Cocoa world clicks through the existing `CursorUpdate` rules
   - directional and diagonal cursor regions now produce the same raw movement
     keys used by the keyboard path
   - special cursor actions continue to use the legacy `gCurMouseDir` mapping
   - automated click diagnostic at `(600,384)` produces east key `29` and moves
     the party from `(42,20)` to `(43,20)`
   - all cursor IDs with an existing legacy key equivalent now preserve that
     equivalent, including attack, unlock, enter, board, exit, torch, and dungeon actions
53. bypass the obsolete Carbon display-mode dialog on the native Cocoa path
   - current macOS startup explicitly selects windowed native UI and records the
     choice, leaving the original dialog only for a future legacy platform path
54. replace legacy NSSound effects with AVFoundation AVAudioPlayer
   - preserve asynchronous overlap, synchronous wait behavior, and volume updates
   - audio self-test checks bundled music metadata and the bundled WAV effect
     asset; actual device audio-session playback remains a GUI validation item
55. add a reproducible arm64 Release-app build path
   - `scripts/build-release-app.sh` bypasses the obsolete DMG/Rez packaging
     phase and produces an unsigned Release `.app`
   - signing, notarization, and modern DMG/ZIP packaging remain release work
56. route splash/intro interruption checks through Cocoa input polling
   - startup key and mouse events now use the same AppKit boundary as gameplay
   - Carbon `WaitNextEvent` remains only as a non-Cocoa legacy fallback
57. normalize physical arrow-key events at the Cocoa input boundary
   - AppKit arrow key codes now map to the legacy movement values `28..31`
   - numeric keypad and character-command input remain unchanged
   - MIDI remains an optional experimental backend; original QuickTime timbres
     are not treated as preserved until GUI playback is compared
58. expose the existing game save command through the native Cocoa menu
   - `File > Save Game` posts the legacy `Q` command through the normal input
     boundary, preserving the existing save validation and resource writes
   - native `Cmd-S` now reaches the same path; menu behavior outside the world
     screen remains governed by the original command handling
59. implement portable game/world state persistence behind `U3IO`
   - game and world snapshots use a versioned envelope and explicit big-endian
     field serialization in the existing atomic save container, with map-specific world keys
   - round-trip tests now cover party bytes, coordinates, map ID, and malformed
     state rejection
   - legacy roster/resource migration remains a separate compatibility layer

### 2026-09-10: Basic Play Checkpoint

Immediate goal: enter Journey Onward in the actual window, complete a turn,
and continue playing. The user has confirmed this basic gameplay milestone.

- Corrected native menu hit testing: buttons start at y=219, not y=55
  at the base scale. Drawing and native hit testing share button bounds.
- Added native organize-party button commands, including disabled states.
- Removed Carbon FindWindow dispatch from native mouse input; restored
  consumption of the legacy command queue. Other native mouse interactions
  (especially character panels and context actions) still need verification.
- Added `scripts/play-basic.command`: classic appearance, music disabled,
  isolated saves in `/tmp/u3-basic-play-save`. Preferences are overridden
  in the running process; the options dialog may subsequently save them.
- Release arm64 build passed. Synthetic menu checks cover all four buttons
  and outside clicks at 1x and 2x. Party persistence plus world input checks
  completed east/west turns (42,20 -> 43,20 -> 42,20), including Routine6E35
  and re-entry into the game loop. The resulting bitmap was exported.
- These checks bypass native party dialogs and the Journey menu action.
  They do not establish that the user's missing names or GUI crashes are fixed.
- Subsequent manual verification reported by the user: Journey Onward starts
  the game, automatic combat runs, quitting and Resume work, and the cursor
  changes with mouse position. These reports establish the playable checkpoint;
  they do not establish exhaustive gameplay or persistence correctness.
- The user also hears background music, but reports unfamiliar timbre. Playback
  currently uses converted MIDI through AVMIDIPlayer with no explicit sound bank.
  Original timbre preservation and conversion fidelity remain unverified.
- Next: audit the 64-byte Party array's one-based resource copy and verify
  save/load boundaries. Character-panel actions and other contextual mouse
  commands still need coverage. Modern fonts and music fidelity remain deferred.

### 2026-09-10: Fixed-Bank MIDI

- Agreed direction: preserve the musical arrangement with MIDI and a fixed
  sound bank. Exact QuickTime timbre or AY-3-8910 emulation is not a goal.
- Bundled GeneralUser GS v2.0.3 with its upstream license and pinned provenance.
  AVMIDIPlayer now requires this bank instead of silently using the OS default.
- Reconverted all ten tracks with GM instrument assignments and percussion
  routing. Song_1 uses SynthStrings 1, not the previous default piano.
- Replaced atom byte searching with bounded container traversal and sample
  mapping. Fixed extended-note bit fields and variable-length event traversal;
  retained source timing and supported main-sequence controllers.
- The original DemoUpdate selects a playlist after the intro. Demo now calls
  the music update explicitly, restoring the service previously supplied by
  legacy input polling. Listening through the whole demo remains a manual check.
- Three converter tests pass, including all ten tracks' program/note counts,
  percussion routing, extended-event traversal, and truncation rejection.
  Release arm64 build passes. On-device Song_1 playback with the bundled bank
  reports duration 133.72 seconds and playing=1. Two-turn gameplay regression
  still passes. Listening quality has not been established by these checks.
- Music volume/fading remains an existing limitation of the AVMIDIPlayer
  integration. The basic-play launcher intentionally disables music; use the
  app normally with music enabled to evaluate this checkpoint.

### 2026-09-11: Keyboard Commands and Text

User priority: defer music timbre and looping; make command prompts and dialogue
reliable. T invokes Transact, followed by character selection and direction.

- Implemented the previously empty ScrollRect adapter with clipped bitmap
  scrolling and background clearing. Prior dialogue lines now remain visible.
- Expanded text iteration counters beyond signed-char range. Added bounds to
  Talk traversal, concatenation, search/replacement, and text entry; corrected
  end-of-string matching and long-word wrapping that could fail to advance.
- Added scroll-area wrapping, carriage-return recognition, pen position access,
  and explicit presentation before blocking input. Modern temporary text ports
  restore the main text insertion position after drawing.
- Native keys no longer get truncated from Unicode. Added physical command-key
  fallback for non-Latin input sources, Return/keypad Enter and Delete mapping,
  and exclusion of Command shortcuts from game commands. Game input is filtered
  by window and modal state; consumed keys are not also dispatched to Cocoa.
- Event pumping preserves queued game input; explicit flush discards it.
  Cleared stale key values, ignored untranslated clicks, restored character-panel
  selection at Who prompts, and made waits observe quit state. Retained the
  Cocoa menu action target for the lifetime of the installed menu.
- Diagnostics exercise T -> 1 -> east -> a controlled NPC reply through the
  command functions, text editing, numeric filtering, Escape, 255-byte text,
  search/replacement limits, and native key translation. They export Who,
  direction, reply, and long-text screenshots. Run `sh scripts/test-command-text.sh`
  after building; saves are isolated. Native event injection is opt-in via
  `U3_VERIFY_NATIVE_INPUT=1` with the same command diagnostic environment.
- Classic and modern headless command checks passed. A native-window run using
  posted Cocoa events also passed. Bitmap scrolling tests pass under ASan/UBSan.
  Upper screen and map pixels remain unchanged by dialogue scrolling. These
  tests use a controlled NPC, not every town, shop, spell, or combat conversation.
- User listening feedback: the fixed-bank timbre is not preferred and music
  looping is faulty. Both remain explicitly deferred.
- Subsequent user play verification: commands now respond correctly and tiles
  no longer appear corrupted. Record this as the checkpoint before persistence
  safety work; it is not exhaustive coverage of all game scenarios.

### 2026-09-11: Save and Memory Safety

- Playable checkpoint: commit `597b24e` includes the verified command/text work
  and the preceding fixed-bank MIDI work. Music timbre and looping remain open.
- Corrected the legacy Party array to 65 bytes: padding at index zero, followed
  by all 64 PRTY resource bytes. A shared declaration removes inconsistent
  extern sizes. File records remain 64 bytes; the version-1 portable snapshot
  field is unchanged. No save-format migration is required.
- Added actual GetParty/PutParty/reopen coverage with a nonzero final byte and
  coordinate checks. Invalid and duplicate party-member indices are rejected
  when opening/writing save containers; malformed input is not overwritten.
- AddressSanitizer exposed another overread during startup: bundled career,
  weapon, and armour tables have 11 bytes, and Experience has 16, while the
  old loops copied 12 and 17. Reads/writes now respect resource lengths and
  initialize unused in-memory padding. All six miscellaneous resources retain
  their exact bytes and lengths after load/save/reopen.
- Release and AddressSanitizer builds pass the save-container tests. The
  sanitizer build also passes party formation and two consecutive world turns.
  Classic and modern command/dialogue diagnostics also pass under ASan.
  The ASan runtime is linked into the dedicated Debug build under
  `/tmp/ultima3-party-asan`; the normal Release app remains uninstrumented.
- Next: expand scenario coverage to shops, spells, manual combat, dungeons,
  death/resurrection, and map changes. Broader save error propagation and
  validation of other game-state fields remain work, not completed guarantees.

### 2026-09-11: Shop and Town Regression Checks

- Fixed Enter scanning 32 entries in the 20-entry LocationX/LocationY arrays.
  The search now uses the actual array capacity.
- Added `sh scripts/test-gameplay-scenarios.sh [app-path]`. Each run creates
  isolated temporary saves and a world screenshot. The diagnostic uses actual
  food-shop input handling to check purchases, insufficient gold, and Escape.
- All nine town locations are exercised through Enter and Routine6E6B, checking
  map type and entry/return coordinates. This tests the transition functions,
  not walking across a town boundary or the full merchant discovery workflow.
- Release and AddressSanitizer checks passed. Spells, manual combat, dungeon entry/exit, and
  death/resurrection remain the next scenario-coverage tasks. Music is deferred.

### 2026-09-11: Spell and Dungeon Scenarios

- Extended gameplay diagnostics through C -> character 1 -> Lorum, asserting
  mana cost and torch state. Escape and insufficient mana leave effects unchanged.
- All seven dungeon entrances now run Enter -> DungeonStart -> keyboard K ->
  world return, checking exit state, level, restored coordinates, and consumed input.
  This does not cover internal movement, deeper levels, traps, or encounters.
- Fixed Flashriek indexing a 34-entry sound table with bonus spell 34
  (Flotellum). Unlisted spell sounds now use the existing generic spell effect.
  A bonus-spell fixture unlocks Flotellum and supplies adjacent water, checking
  its 90-mana cost and ship creation through the ordinary spell-selection path.
- Release and AddressSanitizer runs passed all expanded gameplay scenarios;
  bitmap regression tests also passed. These are isolated classic-mode diagnostics,
  not a claim of exhaustive spell coverage or a full manual playthrough.
- Next: manual combat, dungeon traversal and level changes, other spell classes,
  and death/resurrection. Music timbre and looping remain deferred.

### 2026-09-11: Manual Combat Regression

- Added a controlled one-character, one-enemy arena to the isolated gameplay
  diagnostic. The normal Combat loop, enemy attack logic, CombatAttack,
  DamageMonster, and Victory execute unchanged; only fixture setup and counters
  are diagnostic-specific. Input is queued after the normal combat-entry flush.
- Checks space/pass -> enemy attack attempt -> A/north -> enemy death -> victory.
  Assertions require manual mode, exactly two player turns, one enemy attack
  attempt, consumed input, and restored map type, update mode, and coordinates.
  High fixture dexterity and one enemy HP make the player hit deterministic;
  the enemy is allowed to hit or miss normally.
- Release and AddressSanitizer checks passed, including the preceding shop,
  spell, town, and dungeon scenarios. This is not yet coverage of four-character turn order,
  combat movement, ranged attacks, combat spells, retreat, or death/resurrection.
  Dungeon traversal and level changes also remain open; music stays deferred.

### 2026-09-11: Four-Character Combat Turns and Movement

- Extended the disposable combat fixture to four distinct roster slots. The
  original single-character scenario still runs first.
- Records and asserts the exact player sequence 1, 2, 3, 4, 1, with one enemy
  attack attempt only after all four characters complete their first turns.
- Character 1 moves east; characters 2-4 pass; character 1 attacks north and
  wins on the next round. At each completed player turn, checks verify the new
  coordinates, restored floor at the old position, and character tile at the
  destination, before Victory replaces the combat tiles with the world map.
- Release and AddressSanitizer scenarios passed. Coverage remains limited to an unobstructed cardinal
  move and living characters. Blocked/diagonal movement, incapacitated-character
  turn skipping, ranged attacks, combat spells, and death/resurrection remain open.

### 2026-09-11: Blocked Movement and Inactive Combatants

- Added a third combat scenario with a blocked east tile, member 2 dead, and
  member 4 ashes (both with zero HP). Existing one- and four-character living
  scenarios still run before this case.
- Verifies that the blocked move leaves character coordinates, the occupied
  tile, and the blocking tile unchanged throughout the round. It retains the
  legacy behavior in which the failed movement attempt consumes the turn.
- Records active input turns separately from visited party slots: only 1, 3, 1
  receive input, with the enemy acting after the first party round. Dead/ashes
  members remain off the combat grid and retain their status through victory.
- Release and AddressSanitizer checks passed. This fixture starts with inactive members; it does not
  exercise dying during combat, full-party defeat, or resurrection. Those paths,
  dungeon traversal, ranged attacks, and combat spells remain open.

### 2026-09-11: Death and Resurrection State

- Fixed CheckAlive to reject absent party slots and invalid roster indices.
  Defeat detection and resurrection now iterate actual party members, bounded
  to four, rather than reading or modifying the reserved Player[0] record for
  empty slots. Resurrection also ignores invalid member indices.
- Extended the isolated diagnostic with nonlethal and lethal HPSubtract calls,
  then CheckAllDead for a one-member party. A deliberately living reserved
  record verifies that an empty slot cannot prevent defeat or get resurrected.
- The diagnostic bypasses only the acknowledgement and resurrection-choice UI;
  normal resurrection, world reset, and save writes still run in the temporary
  save directory. Checks cover status, 100 HP, starting gold/equipment, world
  coordinates, resurrection flag, and preservation of the reserved roster record.
- Release and AddressSanitizer checks passed. This is direct damage/defeat-function coverage, not an
  enemy-caused wipeout inside Combat, a post-resurrection save reopen check, or
  coverage of the decline-resurrection dialog path. Those remain open.

### 2026-09-11: Combat Defeat and Resurrection Reload

- Added a one-HP combat fixture using the existing Exodus/no-Exotic-armour
  guaranteed-hit rule. After a manual pass, the real enemy damage path kills
  the character and invokes CheckAllDead. Only the resurrection UI choice is
  supplied by the diagnostic; death, resurrection, and save logic run normally.
- Combat now returns immediately after resurrection (or quit) in its death
  handler. Previously it could reach another combatstart/AgeChars call and then
  restore the old update mode. The test requires only one visited player turn,
  one enemy attack, the resurrection flag, and restored world state.
- Reopens the save container, discarding its resource cache, then reloads party,
  roster, and world after deliberately changing in-memory HP/status/coordinates.
  Checks that the file restores the resurrection location, 100 HP, and equipment.
- Release and AddressSanitizer scenarios passed. This is an in-process disk reload, not a separate
  application launch through the Resume menu. Declining resurrection, dungeon
  battle defeat, and broader multi-character death cases remain open.

### 2026-09-11: Fresh-Process Resume

- The gameplay script now launches a second app process only after the first
  process exits successfully, using the same isolated save directory.
- U3_RESUME_CHECK with U3_WORLD_INPUT_CHECK bypasses diagnostic character
  creation, not normal loading: startup reads the saved roster/party, the demo
  is dismissed, and an injected J goes through MainMenu's Journey Onward path.
- At Game entry, assertions check party membership, resurrection location,
  100 HP, and equipment. Two ordinary world turns then move east and west and
  reach the next input cycle; a separate resume.png records the result.
- Release and AddressSanitizer passed: (42,20) -> (43,20) -> (42,20). This is a headless fresh-process
  menu/keyboard test, not a physical mouse click or a Finder-launched UI test.

### 2026-09-11: Dungeon Traversal and Level Boundaries

- Added a deterministic two-level corridor, temporarily replacing and then
  restoring the dungeon buffer. The actual DungeonStart input loop handles
  right/left rotation, forward movement, blocked movement, descend, climb,
  backward movement, and ladder exit. Every command checks position, heading,
  level, and exit state. Keys are queued per prompt so wall-collision input
  flushing cannot discard later test steps. Existing seven real entrance tests remain.
- dDescend now rejects descent from level index 7 (or an invalid level), avoiding
  a level-8 access beyond the eight-level dungeon buffer. A bottom-level ladder
  fixture checks that the level remains unchanged.
- Release and AddressSanitizer checks passed, including combat, resurrection, and fresh-process
  resume. This covers a synthetic corridor, not a full real-dungeon route;
  traps, doors, treasure, encounters, wraparound, and dungeon defeat remain open.

### 2026-09-11: Dungeon Doors, Traps, and Treasure

- Fixed the floor-trap branch: StealDisarmFail returns true on failure, but the
  dungeon branch previously skipped damage on true. It now skips damage on
  successful disarm, consistent with the chest-trap callers.
- Extended the temporary dungeon fixture with a door (0xA0), checking forward
  entry, blocked rotation inside it, and backward exit through movement functions.
- A floor-trap event runs through DungeonStart(1) with guaranteed fixture disarm,
  checking tile removal and unchanged HP. BombTrap is separately called with a
  known depth to check its 8-127 damage range. This is not a forced failed-disarm
  roll through the entire floor-trap event.
- GetChest receives character selection input on a dungeon chest, checking tile
  removal and a 30-100 gold reward. Repeating the call on the emptied tile cannot
  grant more gold. High fixture dexterity makes random chest traps harmless;
  random item rewards are not asserted. Character data and dungeon data are restored.
- Release and AddressSanitizer checks passed with the existing battle, resurrection, and fresh-process
  resume scenarios. These are controlled tile/function tests, not a real dungeon
  playthrough; fountains, marks, writing, and random encounters remain open.

### 2026-09-11: Fountain, Mark, and Writing Events

- Extended the isolated dungeon fixture through the four fountain effects:
  poison, healing to maximum HP, 25 HP damage, and poison cure. Character
  selection and Escape run through the ordinary fountain input loop.
- A mark tile checks its 0x10 bit and 50 HP cost. A writing tile executes the
  message/Speak path and returns without consuming the tile. This does not
  assert the rendered text contents or cover every mark and dungeon inscription.
- Fixed the diagnostic's event-return flag lifetime: it was disabled before
  these new events, leaving tests in the interactive dungeon loop. Removed
  the attempted fountain-specific bypass; ordinary Escape closes the image.
  The earlier attribution to GetChar rejecting space was incorrect.
- Release and AddressSanitizer scenarios passed, including battle, resurrection, and fresh-process
  resume. The temporary character and dungeon records are restored afterward.

### 2026-09-11: Visible Dungeon Inscriptions

- The writing fixture temporarily supplies a known Talk inscription with a 0xFF
  separator, then executes the writing event in both classic and modern modes.
  Original Talk data and appearance preference are restored afterward.
- The gameplay script sets U3_DUNGEON_TEXT_CHECK to capture before/after PNGs
  for each mode in its isolated artifact directory (inscription-*.png).
- Release captures were visually inspected: SEEK THE SHRINE. and BEWARE THE
  FLAMES. are readable in both modes. Classic uses fixed-column wrapping;
  modern reflows at words. Pixel comparison found 21,188 and 20,994 changed
  dialogue pixels respectively, with the left dungeon viewport unchanged.
- The full Release script, including fresh-process resume and two world turns,
  passed. This verifies the controlled inscription, not every bundled message;
  the visible scene is still the synthetic event fixture.

### 2026-09-11: Preserve Auto-Combat Preference

- The combat regression fixture temporarily enables Manual Combat so it can
  drive deterministic player turns. It now saves and restores the original
  preference after every fixture, including the defeat path.
- A previous run left `com.lairware.ultima3`'s `cmb` preference enabled, which
  made normal battles pass instead of using AutoCombat. The local preference
  was restored to automatic combat (`cmb=false`).
- Release command/text checks passed after the fix. The full gameplay suite was
  already passing before this preference-only correction.

### 2026-09-11: Auto-Combat Regression

- Added a direct AutoCombat fixture with one adjacent enemy and a living melee
  character. It verifies the generated macro is `A,8`: AddMacro stores the
  newest command first, so Combat consumes the attack command and then reads
  the north direction.
- The fixture restores macro, party, character, monster, and combat globals.
  ManualCombat preference preservation is also covered by the surrounding
  manual-combat fixtures, preventing diagnostics from disabling AutoCombat in
  later user sessions.
- Release gameplay scenarios passed and the persisted `cmb` preference remains
  false afterward. This checks macro generation; a long random multi-monster
  auto-battle remains a separate playthrough check.

### 2026-09-11: Ranged Combat Attack

- Added a controlled combat-map fixture for `CombatAttack()` with a projectile
  weapon, one aligned target, and a queued north direction. The real Shoot,
  hit, accuracy, damage, and tile restoration paths execute; the test asserts
  that the target HP decreases and restores all temporary combat state.
- The fixture deliberately places the party outside the Exodus Castle special
  case, since legacy CombatAttack rejects ordinary ranged weapons inside that
  castle unless the weapon is the special exception.
- Release gameplay scenarios passed. A full AddressSanitizer run is the final
  check for this change; ranged attacks against multiple targets and all weapon
  types remain broader playthrough coverage.

### 2026-09-11: Combat Spell Casting

- Added a controlled combat-spell fixture that enters the real `Cast()` path as
  a wizard, selects Fulgar with `F`, supplies its direction with `8`, and checks
  both the 25-point mana cost and projectile damage.
- The fixture restores party, player, tile, monster, spell-selection, and global
  combat state, so the diagnostic cannot alter the following defeat and resume
  scenarios.
- Release gameplay scenarios passed. AddressSanitizer validation is still run
  before commit; multi-target spells and every individual spell remain broader
  playthrough coverage.

### 2026-09-11: Area Combat Spell

- Added a deterministic multi-target fixture for the real `Spell()`/`Necorp`
  path. Two active combat monsters are supplied and both must be reduced to the
  spell's fixed 5 HP result.
- The fixture restores monster, tile, party, spell-selection, and quit state.
  Random area spells and the complete cleric spell-selection table remain
  broader playthrough coverage.

### 2026-09-11: Multi-Target Auto-Combat Decision

- Extended the auto-combat regression fixture with two active targets and a
  controlled experience value. The wizard threat branch must recognize their
  combined value and queue the `C,P` spell macro rather than falling through to
  a movement or melee action.
- The earlier adjacent-melee assertion remains in the same fixture, and player,
  monster, macro, experience, and combat state are restored after both cases.

### 2026-09-11: Auto-Combat Spell Macro Execution

- Extended the auto-combat fixture to generate `C,B,8` for a wizard with an
  aligned target, then consumed the macro through `GetKeyMouse()` and `Cast()`.
  The test verifies the real Mittar selection, direction input, 5-point mana
  cost, and projectile damage rather than checking macro bytes only.
- Tile, character, monster, party, player, spell, macro, and quit state are
  restored after the generated command is executed.

### 2026-09-11: Auto-Combat Support Spell

- Added a two-member fixture where a cleric detects a 50 HP companion and
  generates `C,C,2`. The macro is consumed through the real `Cast()` path,
  selecting Sanctu and the injured second party member.
- The test verifies the 10-point mana cost and HP increase, then restores both
  player records, party slots, tiles, combat arrays, and input state.

### 2026-09-11: Auto-Combat Poison Cure

- Added an automatic poison-priority branch for clerics with the 35 mana cost
  required by Alcort. The first poisoned party member produces `C,H,target`,
  ahead of offensive and healing decisions.
- The fixture consumes that macro through `Cast()`, verifies the target changes
  from poisoned to good status, and preserves the existing support-spell test.
  Paralysis and other status effects are not represented by a distinct player
  status in the current engine and remain a separate legacy behavior audit.

### 2026-09-11: Auto-Combat Retreat Priority

- Extended the auto-combat fixture with a low-HP fighter facing an adjacent
  enemy. The survival branch must queue a safe south retreat (`2`) before the
  normal melee decision.
- The existing melee, threat-based spell, projectile spell, healing, and poison
  cure assertions remain in the same isolated fixture.

### 2026-09-11: Auto-Combat Retreat Execution

- Consumed the generated retreat macro through the platform input queue and ran
  the same `HandleMove()` path used by combat. The fixture verifies movement to
  the safe south tile, restoration of the origin tile, and restoration of its
  temporary state before subsequent combat assertions.

### 2026-09-11: Multi-Character Auto-Combat Turns

- Added a two-member combat fixture with an adjacent melee target for the first
  fighter and an aligned target for the second wizard. Each member generates and
  consumes its own macro: `A,8` for melee, then `C,B,8` for Mittar.
- The fixture verifies both targets take damage and the wizard's mana is charged
  only for its own turn, then restores all party, player, tile, monster, and
  input state.

### 2026-09-11: Four-Party Auto-Combat Turn Generation

- Added a four-character fixture covering three melee decisions and one wizard
  projectile spell decision. Each turn clears and regenerates its own macro,
  verifying the ordered `A,8`, `A,8`, `C,B,8`, `A,8` sequence.
- The complete party, player records, combat arrays, tiles, and input state are
  restored after the four-turn generation check. Full four-member execution in
  the live combat loop remains a broader playthrough check.
