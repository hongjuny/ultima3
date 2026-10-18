"""Check the original splash sample and native playback without an audio device."""

from pathlib import Path
import hashlib
import json
import os
import platform
import plistlib
import shutil
import struct
import subprocess
import tempfile
import unittest
import wave


ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "Resources/SplashSound"

HARNESS = r'''
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <Carbon/Carbon.h>
#import "U3IO.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static NSUInteger created, destroyed, playing, maximumPlaying;
static NSMutableArray *livePlayers;
static void check(BOOL condition, const char *message) {
    if (!condition) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}
@interface U3SilentSplashPlayer : NSObject {
    BOOL _playing;
    float _volume;
    NSData *_data;
}
@property float volume;
@property (readonly) NSData *data;
- (id)initWithData:(NSData *)data error:(NSError **)error;
- (BOOL)prepareToPlay;
- (BOOL)play;
- (void)stop;
@end
@implementation U3SilentSplashPlayer
@synthesize volume = _volume, data = _data;
- (id)initWithData:(NSData *)data error:(NSError **)error {
    if ((self = [super init])) {
        _data = [data retain]; ++created;
        [livePlayers addObject:[NSValue valueWithNonretainedObject:self]];
    }
    return self;
}
- (BOOL)prepareToPlay { return YES; }
- (BOOL)play {
    if (!_playing) { _playing = YES; ++playing; maximumPlaying = MAX(maximumPlaying, playing); }
    return YES;
}
- (void)stop { if (_playing) { _playing = NO; --playing; } }
- (void)dealloc {
    [self stop]; ++destroyed;
    [livePlayers removeObject:[NSValue valueWithNonretainedObject:self]];
    [_data release]; [super dealloc];
}
@end
#define AVAudioPlayer U3SilentSplashPlayer
#include "U3SplashSound.m"

static int32_t sLastError;
static NSMutableDictionary *sResources;
/* RESOURCE_LOADER_IMPLEMENTATION */

static uint32_t little32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void checkVolumes(float volume) {
    for (NSValue *value in livePlayers)
        check(fabs(((U3SilentSplashPlayer *)value.nonretainedObjectValue).volume - volume) < 0.00001,
              "volume immediately reaches both active voices");
}
int main(void) {
 @autoreleasepool {
    livePlayers = [[NSMutableArray alloc] init];
    NSURL *url = [[NSBundle mainBundle] URLForResource:@"Whoosh" withExtension:@"snd" subdirectory:@"SplashSound"];
    NSData *resource = [NSData dataWithContentsOfURL:url];
    NSError *error = nil;
    check(resource.length == 827, "original sample resource bundled");
    check(U3SplashSoundLegacyCommand(-32767) == 0xff00e0bf &&
          U3SplashSoundLegacyCommand(31745) == 0xff00e0a0, "original packed pitch commands");
    check(U3SplashSoundNoteForPass(-32767) == 63 && U3SplashSoundNoteForPass(31745) == 32,
          "pitch follows pass and resets for second dissolve");
    // Pass -26623 selects note 60, the sample's untransposed base frequency.
    NSData *base = U3SplashSoundWaveData(resource, -26623, &error);
    check(base && !error, "original sound decodes");
    U3DataBuffer loaded;
    check(U3IOLoadResource(U3ResourceKindSoundResource, 400, &loaded),
          "actual game resource loader finds bundled Whoosh");
    NSData *nativeResource = [NSData dataWithBytes:loaded.bytes length:loaded.size];
    U3IOReleaseResource(&loaded);
    check(((const uint8_t *)nativeResource.bytes)[0] == 1,
          "Carbon returns its translated native-endian sound headers");
    check([U3SplashSoundWaveData(nativeResource, -26623, NULL) isEqualToData:base],
          "actual resource-loader bytes render identically to raw source bytes");
    for (int32_t pass = -32767; pass < 32768; pass += 1536)
        check([U3SplashSoundWaveData(nativeResource, pass, NULL)
               isEqualToData:U3SplashSoundWaveData(resource, pass, NULL)],
              "every splash pass supports Carbon-translated headers");
    const uint8_t *wave = base.bytes, *source = (const uint8_t *)resource.bytes + 42;
    double step = (729236944.0 / 65536.0) / 44100.0;
    check(little32(wave + 24) == 44100 && little32(wave + 40) / 2 == ceil(785 / step),
          "fractional legacy sample rate is preserved in resampling");
    for (uint32_t frame = 0; frame < little32(wave + 40) / 2; ++frame) {
        double position = frame * step;
        unsigned left = MIN((unsigned)position, 784), right = MIN(left + 1, 784);
        double fraction = position - left;
        long expected = lrint(((source[left] - 128) * (1 - fraction) +
                              (source[right] - 128) * fraction) * 256);
        int16_t actual = (int16_t)((unsigned)wave[44 + frame * 2] |
                                 (unsigned)wave[45 + frame * 2] << 8);
        check(labs(actual - expected) <= 1, "render contains interpolated original samples");
    }
    for (NSUInteger length = 0; length < resource.length; ++length) {
        NSData *truncated = [resource subdataWithRange:NSMakeRange(0, length)];
        error = nil;
        check(!U3SplashSoundWaveData(truncated, 0, &error) && error, "truncated input rejected");
    }
    NSMutableData *bad = [resource mutableCopy];
    ((uint8_t *)bad.mutableBytes)[40] = 0xff;
    check(!U3SplashSoundWaveData(bad, 0, NULL), "unsupported encoding rejected");
    [bad release];
    U3SplashSoundPlayer *player = [[U3SplashSoundPlayer alloc] initWithSoundResourceData:nativeResource error:NULL];
    check(player && created == 0, "initialization does not open audio");
    [player setVolumePercent:100 enabled:NO]; [player triggerPass:-32767];
    [player setVolumePercent:0 enabled:YES]; [player triggerPass:-32767];
    check(created == 0, "disabled and muted effects never create a player");
    [player setVolumePercent:25 enabled:YES];
    for (unsigned i = 0; i < 43; ++i) [player triggerPass:-32767 + (int32_t)i * 1536];
    check(created == 43 && playing == 2 && maximumPlaying == 2, "43 passes use two bounded voices");
    checkVolumes(0.25);
    NSDictionary *cache = [player valueForKey:@"notes"];
    check(cache.count == 32, "pitch cache bounded by 32 source commands");
    [player setVolumePercent:75 enabled:YES]; checkVolumes(0.75);
    [player triggerPass:-32767];
    U3SilentSplashPlayer *last = [[livePlayers lastObject] nonretainedObjectValue];
    check([last.data isEqualToData:U3SplashSoundWaveData(resource, -32767, NULL)],
          "second dissolve resets pitch and retriggers");
    [player setVolumePercent:150 enabled:YES]; checkVolumes(1);
    [player setVolumePercent:-1 enabled:YES];
    check(playing == 0 && livePlayers.count == 0 && cache.count == 0, "mute stops and clears cache");
    [player setVolumePercent:100 enabled:YES]; [player triggerPass:31745];
    [player stop];
    check(playing == 0 && cache.count == 0 && created == destroyed, "stop releases voices and cached sounds");
    [player release];
    NSUInteger beforeDiagnostic = created;
    check(U3SplashSoundSelfTest() && created == beforeDiagnostic,
          "headless diagnostic renders without live playback");
    [livePlayers release];
    puts("Original splash PCM, pitch, cadence, mute, volume, and bounded playback passed");
 }
 return 0;
}
'''


def function_source(source, signature):
    """Compile the production resource-loader functions in the silent harness."""
    start = source.index(signature)
    body = source.index("{", start)
    depth, end = 1, body + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


class SplashAssetTests(unittest.TestCase):
    def test_extraction_is_reproducible_and_lossless(self):
        with tempfile.TemporaryDirectory(prefix="u3-splash-extract-") as temporary:
            subprocess.run(["python3", str(ROOT / "scripts/extract-splash-sound.py"),
                            "--output", temporary], check=True, capture_output=True)
            for name in ("Whoosh.snd", "Whoosh.wav", "provenance.json"):
                self.assertEqual((Path(temporary) / name).read_bytes(), (ASSETS / name).read_bytes())
        resource = (ASSETS / "Whoosh.snd").read_bytes()
        self.assertEqual(hashlib.sha256(resource).hexdigest(),
                         "8dec31a7551569c47866d3cf997ee266de30a5fcc943ef6ec932b34d0580b721")
        with wave.open(str(ASSETS / "Whoosh.wav")) as sound:
            self.assertEqual((sound.getnchannels(), sound.getsampwidth(), sound.getframerate(), sound.getnframes()),
                             (1, 1, 11127, 785))
            self.assertEqual(sound.readframes(785), resource[42:])
        provenance = json.loads((ASSETS / "provenance.json").read_text())
        self.assertEqual(provenance["resource_sha256"], hashlib.sha256(resource).hexdigest())

    @unittest.skipUnless(platform.system() == "Darwin" and shutil.which("clang"),
                         "Native silent harness requires macOS developer tools")
    def test_native_rendering_and_silent_playback(self):
        with tempfile.TemporaryDirectory(prefix="u3-splash-native-") as temporary:
            base = Path(temporary)
            contents = base / "SplashTests.app/Contents"
            assets = contents / "Resources/SplashSound"
            assets.mkdir(parents=True)
            shutil.copyfile(ASSETS / "Whoosh.snd", assets / "Whoosh.snd")
            localized = contents / "Resources/English.lproj"
            localized.mkdir()
            shutil.copyfile(ROOT / "Resources/English.lproj/MainResources.rsrc",
                            localized / "Localized.rsrc")
            executable = contents / "MacOS/SplashTests"
            executable.parent.mkdir()
            (contents / "Info.plist").write_bytes(plistlib.dumps({
                "CFBundleExecutable": "SplashTests", "CFBundleIdentifier": "local.u3.silent-splash-tests",
                "CFBundlePackageType": "APPL",
                "CFBundleDevelopmentRegion": "English",
            }))
            source = base / "harness.m"
            io = (ROOT / "Sources/U3IOLegacy.m").read_text()
            functions = ["static ResType U3LegacyResourceTypeForKind(",
                         "static NSString *U3ResourceKey(", "static NSData *U3BundledResource(",
                         "bool U3IOLoadResource(", "void U3IOReleaseResource("]
            source.write_text(HARNESS.replace("/* RESOURCE_LOADER_IMPLEMENTATION */",
                              "\n\n".join(function_source(io, signature) for signature in functions)))
            compiled = subprocess.run(["clang", "-fsanitize=address,undefined", "-g", "-Werror", "-Wall",
                            "-I", str(ROOT / "Sources"), str(source), "-framework", "Foundation",
                            "-Wno-deprecated-declarations", "-framework", "AVFoundation",
                            "-framework", "Carbon", "-o", str(executable)],
                           capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
            preview = Path(os.environ.get("U3_SPLASH_RENDER_DIRECTORY", base / "preview"))
            result = subprocess.run([str(executable)], capture_output=True, text=True,
                                    env={**os.environ, "U3_SPLASH_RENDER_DIRECTORY": str(preview),
                                         "ASAN_OPTIONS": "detect_leaks=0"})
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("bounded playback passed", result.stdout)
            with wave.open(str(preview / "splash-sequence.wav")) as sound:
                self.assertEqual((sound.getnframes(), sound.getframerate()), (416745, 44100))
                samples = struct.unpack("<" + "h" * sound.getnframes(), sound.readframes(sound.getnframes()))
                # The last low note decays into the original 60-tick pause.
                self.assertTrue(all(x == 0 for x in samples[180000:202125]))
                self.assertTrue(any(samples[:160000]) and any(samples[202125:328545]))
                self.assertLess(max(abs(x) for x in samples), 32767)


if __name__ == "__main__":
    unittest.main()
