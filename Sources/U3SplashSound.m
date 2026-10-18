#import "U3SplashSound.h"
#import <AVFoundation/AVFoundation.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const uint8_t *samples;
    uint32_t frames;
    double rate;
    uint8_t baseNote;
} U3SplashSample;

static uint16_t U3SplashBE16(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}
static uint32_t U3SplashBE32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}
static uint16_t U3SplashRead16(const uint8_t *p, BOOL littleEndian) {
    return littleEndian ? ((uint16_t)p[1] << 8) | p[0] : U3SplashBE16(p);
}
static uint32_t U3SplashRead32(const uint8_t *p, BOOL littleEndian) {
    return littleEndian ? ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) |
                          ((uint32_t)p[1] << 8) | p[0] : U3SplashBE32(p);
}
static void U3SplashLE(uint8_t *p, uint32_t value, unsigned bytes) {
    for (unsigned i = 0; i < bytes; ++i) p[i] = (uint8_t)(value >> (8 * i));
}
static NSData *U3SplashFail(NSError **error, NSString *message) {
    if (error) *error = [NSError errorWithDomain:@"U3SplashSound" code:1
        userInfo:@{NSLocalizedDescriptionKey: message}];
    return nil;
}

static BOOL U3SplashParse(NSData *resource, U3SplashSample *sample, NSError **error) {
    const uint8_t *bytes = resource.bytes;
    const NSUInteger size = resource.length;
    if (size < 6) {
        U3SplashFail(error, @"Expected a format-1 sampled sound resource"); return NO;
    }
    // Carbon's GetResource translates sound-resource header fields to native
    // endian on little-endian Macs. Raw .snd files retain their big-endian
    // headers. The format word unambiguously distinguishes these two layouts;
    // the PCM bytes and byte-sized header fields are unchanged.
    BOOL littleEndian = bytes[0] == 1 && bytes[1] == 0;
    if (U3SplashRead16(bytes, littleEndian) != 1) {
        U3SplashFail(error, @"Expected a format-1 sampled sound resource"); return NO;
    }
    NSUInteger modifiers = U3SplashRead16(bytes + 2, littleEndian);
    if (modifiers > (size - 6) / 6) {
        U3SplashFail(error, @"Truncated sound modifier list"); return NO;
    }
    NSUInteger position = 4 + modifiers * 6;
    NSUInteger commands = U3SplashRead16(bytes + position, littleEndian);
    position += 2;
    if (commands > (size - position) / 8) {
        U3SplashFail(error, @"Truncated sound command list"); return NO;
    }
    for (NSUInteger i = 0; i < commands; ++i, position += 8) {
        uint16_t command = U3SplashRead16(bytes + position, littleEndian);
        if (!(command & 0x8000) || ((command & 0x7fff) != 80 && (command & 0x7fff) != 81))
            continue;
        uint32_t offset = U3SplashRead32(bytes + position + 4, littleEndian);
        if (offset > size || size - offset < 22) break;
        const uint8_t *header = bytes + offset;
        uint32_t frames = U3SplashRead32(header + 4, littleEndian);
        uint32_t fixedRate = U3SplashRead32(header + 8, littleEndian);
        uint32_t loopStart = U3SplashRead32(header + 12, littleEndian);
        uint32_t loopEnd = U3SplashRead32(header + 16, littleEndian);
        if (U3SplashRead32(header, littleEndian) || header[20] != 0 || !frames || frames > size - offset - 22 ||
            !fixedRate || !header[21] || header[21] > 127 || loopStart > loopEnd || loopEnd >= frames)
            break;
        *sample = (U3SplashSample){header + 22, frames, fixedRate / 65536.0, header[21]};
        return YES;
    }
    U3SplashFail(error, @"Missing or invalid inline 8-bit sampled sound header");
    return NO;
}

uint32_t U3SplashSoundLegacyCommand(int32_t pass) {
    if (pass < -32767) pass = -32767;
    if (pass > 32767) pass = 32767;
    return 0xFF00E0A0u | (uint32_t)((32768 - pass) / 2048);
}

uint8_t U3SplashSoundNoteForPass(int32_t pass) {
    // Compatibility interpretation of the historic packed note command.
    // Its out-of-range high bits are not part of Apple's documented MIDI API.
    return (uint8_t)(U3SplashSoundLegacyCommand(pass) & 0x7f);
}

static NSMutableData *U3SplashWAV(uint32_t frames) {
    const uint32_t dataSize = frames * 2;
    NSMutableData *data = [NSMutableData dataWithLength:44 + dataSize];
    uint8_t *bytes = data.mutableBytes;
    memcpy(bytes, "RIFF", 4); U3SplashLE(bytes + 4, 36 + dataSize, 4);
    memcpy(bytes + 8, "WAVEfmt ", 8); U3SplashLE(bytes + 16, 16, 4);
    U3SplashLE(bytes + 20, 1, 2); U3SplashLE(bytes + 22, 1, 2);
    U3SplashLE(bytes + 24, 44100, 4); U3SplashLE(bytes + 28, 88200, 4);
    U3SplashLE(bytes + 32, 2, 2); U3SplashLE(bytes + 34, 16, 2);
    memcpy(bytes + 36, "data", 4); U3SplashLE(bytes + 40, dataSize, 4);
    return data;
}

NSData *U3SplashSoundWaveData(NSData *resource, int32_t pass, NSError **error) {
    U3SplashSample sample;
    if (!U3SplashParse(resource, &sample, error)) return nil;
    double speed = pow(2.0, ((int)U3SplashSoundNoteForPass(pass) - sample.baseNote) / 12.0);
    double step = sample.rate * speed / 44100.0;
    double frameCount = ceil(sample.frames / step);
    if (!isfinite(frameCount) || frameCount <= 0 || frameCount > 44100 * 2)
        return U3SplashFail(error, @"Splash sample duration is outside the supported range");
    NSMutableData *output = U3SplashWAV((uint32_t)frameCount);
    uint8_t *pcm = (uint8_t *)output.mutableBytes + 44;
    for (uint32_t frame = 0; frame < (uint32_t)frameCount; ++frame) {
        double source = frame * step;
        uint32_t left = MIN((uint32_t)source, sample.frames - 1);
        uint32_t right = MIN(left + 1, sample.frames - 1);
        double fraction = source - left;
        double value = ((int)sample.samples[left] - 128) * (1.0 - fraction) +
                       ((int)sample.samples[right] - 128) * fraction;
        int32_t converted = (int32_t)lrint(value * 256.0);
        converted = MAX(-32768, MIN(32767, converted));
        U3SplashLE(pcm + frame * 2, (uint16_t)(int16_t)converted, 2);
    }
    return output;
}

NSData *U3SplashSoundIntroWaveData(NSData *resource, NSError **error) {
    // Original graphics code: 43 passes/image, 5 then 4 ticks/pass, 60-tick
    // inter-image pause, and 120-tick final hold. Each trigger replaces one
    // of two alternating sample voices, as in the original gMaxChan=2 path.
    enum { events = 86, totalFrames = 416745 };
    uint32_t starts[events];
    for (unsigned i = 0; i < events; ++i)
        starts[i] = i < 43 ? i * 3675 : 202125 + (i - 43) * 2940;
    NSMutableData *mix = [NSMutableData dataWithLength:totalFrames * sizeof(int32_t)];
    int32_t *samples = mix.mutableBytes;
    for (unsigned i = 0; i < events; ++i) {
        int32_t pass = -32767 + (int32_t)(i % 43) * 1536;
        NSData *wave = U3SplashSoundWaveData(resource, pass, error);
        if (!wave) return nil;
        const uint8_t *pcm = (const uint8_t *)wave.bytes + 44;
        uint32_t frames = (uint32_t)((wave.length - 44) / 2);
        uint32_t end = MIN(totalFrames, starts[i] + frames);
        if (i + 2 < events) end = MIN(end, starts[i + 2]);
        for (uint32_t frame = starts[i]; frame < end; ++frame) {
            const uint8_t *p = pcm + (frame - starts[i]) * 2;
            samples[frame] += (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
        }
    }
    NSMutableData *output = U3SplashWAV(totalFrames);
    uint8_t *pcm = (uint8_t *)output.mutableBytes + 44;
    for (uint32_t frame = 0; frame < totalFrames; ++frame)
        U3SplashLE(pcm + frame * 2, (uint16_t)(int16_t)MAX(-32768, MIN(32767, samples[frame])), 2);
    return output;
}

@implementation U3SplashSoundPlayer
- (id)initWithSoundResourceData:(NSData *)data error:(NSError **)error {
    if ((self = [super init])) {
        U3SplashSample sample;
        if (!U3SplashParse(data, &sample, error)) { [self release]; return nil; }
        _resource = [data copy];
        _notes = [[NSMutableDictionary alloc] init];
        _volumePercent = 100;
        _enabled = YES;
    }
    return self;
}
- (void)setVolumePercent:(short)percent enabled:(BOOL)enabled {
    _volumePercent = MAX(0, MIN(100, percent));
    _enabled = enabled;
    if (!_enabled || !_volumePercent) { [self stop]; return; }
    for (NSUInteger i = 0; i < 2; ++i)
        _voices[i].volume = _volumePercent / 100.0f;
}
- (void)triggerPass:(int32_t)pass {
    if (!_enabled || !_volumePercent) return;
    @autoreleasepool {
        NSNumber *note = @(U3SplashSoundNoteForPass(pass));
        NSData *data = [_notes objectForKey:note];
        NSError *error = nil;
        if (!data) {
            data = U3SplashSoundWaveData(_resource, pass, &error);
            if (!data) { NSLog(@"Cannot render splash sample: %@", error.localizedDescription); return; }
            [_notes setObject:data forKey:note];
        }
        [_voices[_nextVoice] stop];
        [_voices[_nextVoice] release];
        _voices[_nextVoice] = nil;
        AVAudioPlayer *player = [[AVAudioPlayer alloc] initWithData:data error:&error];
        if (player) player.volume = _volumePercent / 100.0f;
        if (player && [player prepareToPlay] && [player play]) {
            _voices[_nextVoice] = player;
            _nextVoice = (_nextVoice + 1) % 2;
        } else {
            [player release];
            if (error) NSLog(@"Cannot play splash sample: %@", error.localizedDescription);
        }
    }
}
- (void)stop {
    for (NSUInteger i = 0; i < 2; ++i) {
        [_voices[i] stop]; [_voices[i] release]; _voices[i] = nil;
    }
    _nextVoice = 0;
    [_notes removeAllObjects];
}
- (void)dealloc {
    [self stop]; [_notes release]; [_resource release]; [super dealloc];
}
@end

BOOL U3SplashSoundSelfTest(void) {
    @autoreleasepool {
        NSURL *url = [[NSBundle mainBundle] URLForResource:@"Whoosh" withExtension:@"snd"
            subdirectory:@"SplashSound"];
        NSData *resource = url ? [NSData dataWithContentsOfURL:url] : nil;
        U3SplashSample sample;
        NSError *error = nil;
        if (!U3SplashParse(resource, &sample, &error) || sample.frames != 785 || sample.baseNote != 60)
            return NO;
        const int32_t passes[] = {-32767, -511, 31745};
        const char *names[] = {"splash-start.wav", "splash-middle.wav", "splash-end.wav"};
        const char *directory = getenv("U3_SPLASH_RENDER_DIRECTORY");
        NSString *path = directory ? [NSString stringWithUTF8String:directory] : nil;
        if (path && ![[NSFileManager defaultManager] createDirectoryAtPath:path
                withIntermediateDirectories:YES attributes:nil error:&error]) return NO;
        NSData *previous = nil;
        for (unsigned i = 0; i < 3; ++i) {
            NSData *wave = U3SplashSoundWaveData(resource, passes[i], &error);
            if (!wave || wave.length <= 44 || [wave isEqualToData:previous]) return NO;
            if (path && ![wave writeToFile:[path stringByAppendingPathComponent:
                    [NSString stringWithUTF8String:names[i]]] atomically:YES]) return NO;
            previous = wave;
        }
        NSData *intro = U3SplashSoundIntroWaveData(resource, &error);
        if (intro.length != 44 + 416745 * 2) return NO;
        if (path && ![intro writeToFile:[path stringByAppendingPathComponent:@"splash-sequence.wav"]
                atomically:YES]) return NO;
        fprintf(stderr, "Splash sound: original Whoosh PCM, pass-dependent pitch and two-stage cadence passed\n");
        return YES;
    }
}
