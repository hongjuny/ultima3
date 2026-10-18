#import "U3SIDMusic.h"
#include <math.h>
#include <stdlib.h>

static void Require(BOOL condition, const char *message) {
    if (!condition) { fprintf(stderr, "SID audio test FAILED: %s\n", message); exit(1); }
}

static void WriteFixture(NSURL *url, const float *samples, AVAudioFrameCount frames,
    double sampleRate, AVAudioChannelCount channels) {
    NSError *error = nil;
    AVAudioFormat *format = [[[AVAudioFormat alloc]
        initStandardFormatWithSampleRate:sampleRate channels:channels] autorelease];
    AVAudioPCMBuffer *buffer = [[[AVAudioPCMBuffer alloc] initWithPCMFormat:format
        frameCapacity:frames] autorelease];
    buffer.frameLength = frames;
    for (AVAudioChannelCount c = 0; c < channels; ++c)
        memcpy(buffer.floatChannelData[c], samples, frames * sizeof(float));
    AVAudioFile *file = [[AVAudioFile alloc] initForWriting:url settings:format.settings error:&error];
    Require(file && [file writeFromBuffer:buffer error:&error], "write isolated fixture");
    [file release];
}

static AVAudioPCMBuffer *ReadPCM(NSURL *url) {
    NSError *error = nil;
    AVAudioFile *file = [[AVAudioFile alloc] initForReading:url commonFormat:AVAudioPCMFormatFloat32
        interleaved:NO error:&error];
    Require(file != nil, "read offline output");
    AVAudioPCMBuffer *buffer = [[[AVAudioPCMBuffer alloc] initWithPCMFormat:file.processingFormat
        frameCapacity:(AVAudioFrameCount)file.length] autorelease];
    AVAudioPCMBuffer *chunk = [[[AVAudioPCMBuffer alloc] initWithPCMFormat:file.processingFormat
        frameCapacity:4096] autorelease];
    AVAudioFramePosition offset = 0;
    while (offset < file.length) {
        AVAudioFrameCount count = (AVAudioFrameCount)MIN(file.length - offset, chunk.frameCapacity);
        Require([file readIntoBuffer:chunk frameCount:count error:&error] && chunk.frameLength,
            "decode all offline output frames, including short reads");
        memcpy(buffer.floatChannelData[0] + offset, chunk.floatChannelData[0], chunk.frameLength * sizeof(float));
        offset += chunk.frameLength;
    }
    buffer.frameLength = (AVAudioFrameCount)offset;
    [file release];
    return buffer;
}

static double CompareExpected(AVAudioPCMBuffer *actual, AVAudioPCMBuffer *source,
    AVAudioFramePosition loopStart, AVAudioFramePosition loopEnd, float gain) {
    double difference = 0;
    for (AVAudioFrameCount i = 0; i < actual.frameLength; ++i) {
        AVAudioFramePosition position = i < loopStart ? i :
            loopStart + (i - loopStart) % (loopEnd - loopStart);
        double expected = source.floatChannelData[0][position] * gain;
        difference = MAX(difference, fabs(actual.floatChannelData[0][i] - expected));
    }
    return difference;
}

static void ExercisePCM(U3SIDMusicPlayer *player, NSURL *sourceURL,
    AVAudioFramePosition loopStart, AVAudioFramePosition loopEnd, NSURL *directory) {
    NSError *error = nil;
    Require([player loadURL:sourceURL loopStartFrame:loopStart loopEndFrame:loopEnd error:&error],
        error ? error.localizedDescription.UTF8String : "load valid PCM");
    AVAudioPCMBuffer *source = ReadPCM(sourceURL);
    uint64_t frames64 = loopStart + (loopEnd - loopStart) * 2 + 101;
    Require(frames64 <= UINT32_MAX, "test length representable");
    AVAudioFrameCount frames = (AVAudioFrameCount)frames64;
    double levels[3] = {0};
    for (int pass = 0; pass < 3; ++pass) {
        NSInteger percent = pass == 0 ? 100 : (pass == 1 ? 25 : 0);
        [player setVolumePercent:percent];
        Require([player restart:&error] && player.framesPlayed == 0, "restart resets timeline");
        NSURL *output = [directory URLByAppendingPathComponent:
            [NSString stringWithFormat:@"render-%ld.wav", (long)percent]];
        double peak = 0;
        @autoreleasepool {
            Require([player renderFrames:frames toURL:output rms:&levels[pass] peak:&peak error:&error],
                "offline render without hardware audio");
        }
        AVAudioPCMBuffer *actual = ReadPCM(output);
        if (actual.frameLength != frames || actual.format.channelCount != 1 || actual.format.sampleRate != 44100)
            fprintf(stderr, "Expected %u mono frames at 44100; got %u channels=%u rate=%.1f\n",
                frames, actual.frameLength, actual.format.channelCount, actual.format.sampleRate);
        Require(actual.frameLength == frames && actual.format.channelCount == 1 &&
            actual.format.sampleRate == 44100, "preserve mono sample rate and frame count");
        double difference = CompareExpected(actual, source, loopStart, loopEnd, percent / 100.0f);
        Require(difference < 0.0000001, "exact intro and two loop periods, with gain only");
        Require(player.framesPlayed == frames && player.completedLoops == 2 &&
            player.currentFrame == loopStart + 101 % (loopEnd - loopStart), "loop accounting");
        uint64_t before = player.framesPlayed;
        [player stop];
        Require(!player.isPlaying && player.framesPlayed == before, "stop preserves position and unschedules playback");
        Require(![player renderFrames:128 toURL:nil rms:NULL peak:NULL error:&error], "stopped player cannot render");
        fprintf(stderr, "SID gain %ld%% RMS=%.8f peak=%.8f maxPCMerror=%.9f frames=%u\n",
            (long)percent, levels[pass], peak, difference, frames);
    }
    Require(levels[0] > 0 && fabs(levels[1] / levels[0] - 0.25) < 0.000001 && levels[2] == 0,
        "independent quarter gain and exact mute");
}

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        NSString *path = [NSTemporaryDirectory() stringByAppendingPathComponent:
            [@"u3-sid-audio-" stringByAppendingString:[NSUUID UUID].UUIDString]];
        NSURL *directory = [NSURL fileURLWithPath:path isDirectory:YES];
        NSError *error = nil;
        Require([[NSFileManager defaultManager] createDirectoryAtURL:directory
            withIntermediateDirectories:YES attributes:nil error:&error], "create isolated test directory");
        U3SIDMusicPlayer *player = [[[U3SIDMusicPlayer alloc] initWithOffline:YES error:&error] autorelease];
        Require(player != nil, "create manual rendering engine");
        float samples[2048];
        for (int i = 0; i < 2048; ++i) samples[i] = ((i * 17) % 701 - 350) / 512.0f;
        NSURL *fixture = [directory URLByAppendingPathComponent:@"canonical.wav"];
        WriteFixture(fixture, samples, 2048, 44100, 1);
        ExercisePCM(player, fixture, 128, 896, directory);
        ExercisePCM(player, fixture, 0, 2048, directory);
        Require(![player loadURL:fixture loopStartFrame:-1 loopEndFrame:896 error:&error], "reject negative loop start");
        Require(![player loadURL:fixture loopStartFrame:896 loopEndFrame:128 error:&error], "reject reversed bounds");
        Require(![player loadURL:fixture loopStartFrame:0 loopEndFrame:2049 error:&error], "reject loop outside asset");
        Require(![player restart:&error], "invalid load cannot replay stale asset");
        NSURL *invalid = [directory URLByAppendingPathComponent:@"invalid.wav"];
        WriteFixture(invalid, samples, 2048, 48000, 1);
        Require(![player loadURL:invalid loopStartFrame:0 loopEndFrame:2048 error:&error], "reject sample-rate conversion");
        WriteFixture(invalid, samples, 2048, 44100, 2);
        Require(![player loadURL:invalid loopStartFrame:0 loopEndFrame:2048 error:&error], "reject stereo conversion");
        memset(samples, 0, sizeof(samples));
        WriteFixture(invalid, samples, 2048, 44100, 1);
        Require(![player loadURL:invalid loopStartFrame:0 loopEndFrame:2048 error:&error], "reject silent loop");
        samples[0] = NAN;
        WriteFixture(invalid, samples, 2048, 44100, 1);
        Require(![player loadURL:invalid loopStartFrame:0 loopEndFrame:2048 error:&error], "reject nonfinite samples");
        Require(![player loadURL:[directory URLByAppendingPathComponent:@"missing.flac"]
            loopStartFrame:0 loopEndFrame:2048 error:&error], "reject missing asset");
        NSURL *oversized = [directory URLByAppendingPathComponent:@"oversized.wav"];
        Require([[NSFileManager defaultManager] createFileAtPath:oversized.path contents:[NSData data]
            attributes:nil], "create sparse size-limit fixture");
        NSFileHandle *handle = [NSFileHandle fileHandleForWritingAtPath:oversized.path];
        [handle truncateFileAtOffset:100 * 1024 * 1024];
        [handle closeFile];
        Require(![player loadURL:oversized loopStartFrame:0 loopEndFrame:2048 error:&error],
            "reject 100 MB file before decoding");
        if (argc == 4) {
            NSURL *asset = [NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[1]]];
            ExercisePCM(player, asset, strtoll(argv[2], NULL, 10), strtoll(argv[3], NULL, 10), directory);
        }
        fprintf(stderr, "SID lossless playback, looping, gain, mute, lifecycle and rejection tests passed.\nArtifacts: %s\n", path.UTF8String);
    }
    return 0;
}
