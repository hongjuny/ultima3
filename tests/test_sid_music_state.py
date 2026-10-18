"""Exercise the actual game music controller with a silent SID-player double."""

from pathlib import Path
import platform
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


def function_source(source, signature):
    start = source.index(signature)
    body = source.index("{", start)
    depth, end = 1, body + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


HARNESS = r'''
#import <Foundation/Foundation.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
enum { U3PreferenceSoundVolume, U3PreferenceMusicVolume, U3PreferenceSoundDisabled, U3PreferenceMusicDisabled };
static BOOL headless, disabled, soundDisabled, failLoad, failInit, failRestart, throwRestart;
static int loads, starts, stops, constructions, soundGain;
static short loadedSong, gSongCurrent, gSongNext, gSongPlaying, gQTMusicVolume = 100, sFailedSong;
static uint64_t sLastLoopCount, nextLoopStart = 100;
static unsigned char Party[65], strk;
BOOL U3CocoaIsHeadlessDiagnostic(void) { return headless; }
BOOL U3PlatformGetBooleanPreference(int key) { return key == U3PreferenceMusicDisabled ? disabled : soundDisabled; }
BOOL U3PlatformHasPreference(int key) { return YES; }
int U3PlatformGetIntegerPreference(int key) { return key == U3PreferenceMusicVolume ? 25 : 33; }
void SetSoundVolumePercent(short percent) { soundGain = percent; }
static void check(BOOL condition, const char *message) {
    if (!condition) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}
static NSError *MusicError(NSString *message, NSInteger code) {
    return [NSError errorWithDomain:@"SIDStateTest" code:code userInfo:@{NSLocalizedDescriptionKey:message}];
}
@interface SilentSIDPlayer : NSObject {
    BOOL _playing;
    uint64_t _framesPlayed, _completedLoops;
    int64_t _loopStartFrame;
    NSInteger _volume;
}
@property(nonatomic) BOOL isPlaying;
@property(nonatomic) uint64_t framesPlayed, completedLoops;
@property(nonatomic) int64_t loopStartFrame;
- (id)initWithOffline:(BOOL)offline error:(NSError **)error;
- (BOOL)restart:(NSError **)error;
- (void)stop;
- (void)setVolumePercent:(NSInteger)percent;
@end
@implementation SilentSIDPlayer
@synthesize isPlaying = _playing, framesPlayed = _framesPlayed, completedLoops = _completedLoops;
@synthesize loopStartFrame = _loopStartFrame;
- (id)initWithOffline:(BOOL)offline error:(NSError **)error {
    if (!(self = [super init])) return nil;
    ++constructions;
    if (failInit) { [self release]; return nil; }
    return self;
}
- (BOOL)restart:(NSError **)error {
    ++starts;
    if (throwRestart) @throw [NSException exceptionWithName:@"DeviceFailure" reason:@"silent simulated device failure" userInfo:nil];
    if (failRestart) return NO;
    _playing = YES; _framesPlayed = _completedLoops = 0;
    return YES;
}
- (void)stop { ++stops; _playing = NO; }
- (void)setVolumePercent:(NSInteger)percent { _volume = percent; }
@end
#define U3SIDMusicPlayer SilentSIDPlayer
static SilentSIDPlayer *songPlayer;
static BOOL LoadSIDMusic(SilentSIDPlayer *player, short song, NSError **error) {
    ++loads; loadedSong = song;
    [player stop]; player.loopStartFrame = nextLoopStart;
    return !failLoad;
}
void MusicUpdate(void);
void EndSong(void);
static bool gDone, hasSurface = true;
static uint32_t fakeTick;
static int musicCalls;
bool U3CocoaHasMainSurface(void) { return hasSurface; }
uint32_t U3PlatformTickCount(void) { return fakeTick; }
static void U3ServiceNativeMusic(void);
void U3AudioUpdateMusic(void) { ++musicCalls; U3ServiceNativeMusic(); }
/* IMPLEMENTATION */
static void reset(void) {
    [songPlayer release]; songPlayer = nil;
    headless = disabled = soundDisabled = failLoad = failInit = failRestart = throwRestart = NO;
    loads = starts = stops = constructions = 0;
    gSongCurrent = gSongNext = gSongPlaying = sFailedSong = 0;
    sLastLoopCount = 0; nextLoopStart = 100; Party[3] = 0; gQTMusicVolume = 100;
}
static void startSong(short song) { gSongCurrent = gSongNext = song; MusicUpdate(); }
int main(void) { @autoreleasepool {
    reset(); startSong(1);
    check(loads == 1 && starts == 1 && gSongPlaying == 1, "initial song starts");
    gSongCurrent = 3; MusicUpdate();
    check(loads == 2 && gSongPlaying == 3, "explicit scene change is immediate");

    reset(); startSong(1); gSongNext = 2; songPlayer.framesPlayed = 99; MusicUpdate();
    check(loads == 1 && gSongPlaying == 1, "queued next waits before intro boundary");
    songPlayer.framesPlayed = 100; MusicUpdate();
    check(loads == 2 && loadedSong == 2 && gSongPlaying == 2, "queued next changes at first score wrap");

    reset(); startSong(1); songPlayer.framesPlayed = 100; MusicUpdate();
    check(loads == 1, "same song keeps looping at intro boundary");
    gSongNext = 4; songPlayer.framesPlayed = 150; MusicUpdate();
    check(loads == 1, "late queued next waits for subsequent boundary");
    songPlayer.framesPlayed = 220; songPlayer.completedLoops = 1; MusicUpdate();
    check(loads == 2 && gSongPlaying == 4, "queued next changes at later loop boundary");

    reset(); nextLoopStart = 0; startSong(1); gSongNext = 2; MusicUpdate();
    check(loads == 1, "zero-intro loop must not count startup as a completed score");
    songPlayer.framesPlayed = 100; songPlayer.completedLoops = 1; MusicUpdate();
    check(loads == 2 && gSongPlaying == 2, "zero-intro queued next changes after first full loop");

    reset(); startSong(1); Party[3] = 0x80; MusicUpdate();
    check(gSongCurrent == 5 && gSongNext == 5 && gSongPlaying == 5, "combat overrides queued music");
    gSongNext = 1; MusicUpdate(); check(gSongNext == 5, "combat music remains selected during combat");

    reset(); startSong(1); gSongCurrent = 9; MusicUpdate();
    check(!songPlayer.isPlaying && gSongPlaying == 0, "unmapped song 9 is silence");
    startSong(1); gSongCurrent = 12; MusicUpdate();
    check(!songPlayer.isPlaying && gSongPlaying == 0, "unmapped song 12 is silence");
    startSong(1); gSongCurrent = gSongNext = 0; MusicUpdate();
    check(!songPlayer.isPlaying && gSongPlaying == 0, "explicit silence stops playback");

    reset(); startSong(1); disabled = YES; MusicUpdate();
    check(!songPlayer.isPlaying && gSongPlaying == 0, "music disabled stops playback");
    disabled = NO; MusicUpdate(); check(gSongPlaying == 1 && songPlayer.isPlaying, "reenabling music restarts scene");
    reset(); headless = YES; startSong(1);
    check(!loads && !constructions, "headless game never opens audio player");

    reset(); failLoad = YES; startSong(1); MusicUpdate();
    check(loads == 1 && !gSongPlaying && sFailedSong == 1, "failed asset logs once and fails closed");
    failLoad = NO; soundDisabled = YES; ApplyVolumePreferences();
    check(loads == 2 && gSongPlaying == 1 && !sFailedSong && gQTMusicVolume == 25,
        "preference change retries failed song and restores configured music gain");
    SetMusicVolumePercent(0); check(gQTMusicVolume == 0, "music volume accepts zero");
    check(soundGain == 33, "muting music leaves the effects gain independent");
    SetMusicVolumePercent(150); check(gQTMusicVolume == 100, "music volume clamps high values");
    reset(); failInit = YES; startSong(1); MusicUpdate();
    check(constructions == 1 && !songPlayer && !gSongPlaying, "failed device construction is suppressed");
    failInit = NO; ApplyVolumePreferences();
    check(constructions == 2 && gSongPlaying == 1, "preference change retries device construction");
    reset(); throwRestart = YES; startSong(1); MusicUpdate();
    check(starts == 1 && !gSongPlaying && !songPlayer.isPlaying, "device exception fails closed once");
    throwRestart = NO; ApplyVolumePreferences();
    check(starts == 2 && gSongPlaying == 1, "device exception can recover after preference change");
    reset(); headless = true;
    fakeTick = 100; U3ServiceNativeMusic(); check(musicCalls == 0, "headless music skip");
    headless = false; hasSurface = false; U3ServiceNativeMusic(); check(musicCalls == 0, "missing surface music skip");
    hasSurface = true; gDone = true; U3ServiceNativeMusic(); check(musicCalls == 0, "quit music skip");
    gDone = false; U3ServiceNativeMusic(); check(musicCalls == 1, "first update and recursive reentry guard");
    fakeTick = 102; U3ServiceNativeMusic(); check(musicCalls == 1, "music throttle");
    fakeTick = 103; U3ServiceNativeMusic(); check(musicCalls == 2, "music resumes at three ticks");
    fakeTick = UINT32_MAX - 1; U3ServiceNativeMusic(); check(musicCalls == 3, "clock near wrap");
    fakeTick = 1; U3ServiceNativeMusic(); check(musicCalls == 4, "unsigned tick wrap");
    puts("Silent SID scene, queue, loop, combat, silence, mute, failure-state and native music-service tests passed");
} return 0; }
'''


@unittest.skipUnless(platform.system() == "Darwin" and shutil.which("clang"),
                     "Native silent state harness requires macOS developer tools")
class SIDMusicStateTests(unittest.TestCase):
    def test_actual_game_music_controller(self):
        source = (ROOT / "Sources/UltimaSound.m").read_text()
        implementation = "\n".join(function_source(source, signature) for signature in (
            "void SetMusicVolumePercent(short volumePercent) {",
            "void ApplyVolumePreferences(void) {", "void MusicUpdate(void) {", "void EndSong(void) {",
        ))
        implementation += "\n" + function_source(
            (ROOT / "Sources/U3PlatformLegacy.m").read_text(),
            "static void U3ServiceNativeMusic(void) {",
        )
        with tempfile.TemporaryDirectory(prefix="u3-sid-state-") as temporary:
            harness = Path(temporary) / "state.m"
            binary = Path(temporary) / "state"
            harness.write_text(HARNESS.replace("/* IMPLEMENTATION */", implementation))
            compiled = subprocess.run(
                ["clang", "-fsanitize=address,undefined", "-g", str(harness),
                 "-framework", "Foundation", "-o", str(binary)],
                capture_output=True, text=True, timeout=45,
            )
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            checked = subprocess.run([str(binary)], capture_output=True, text=True, timeout=15)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
            self.assertIn("tests passed", checked.stdout)


if __name__ == "__main__":
    unittest.main()
