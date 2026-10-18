#import "U3SIDMusic.h"
#include <math.h>
#include <string.h>

static const double U3SIDSampleRate = 44100.0;
static const uint64_t U3SIDMaximumBytes = 100 * 1024 * 1024;

static BOOL U3SIDError(NSError **error, NSString *message, NSInteger code) {
    if (error) *error = [NSError errorWithDomain:@"UltimaIIISIDAudio" code:code
        userInfo:@{NSLocalizedDescriptionKey: message}];
    return NO;
}

static AVAudioPCMBuffer *U3SIDSlice(AVAudioPCMBuffer *source,
    AVAudioFramePosition start, AVAudioFramePosition end) {
    AVAudioFrameCount count = (AVAudioFrameCount)(end - start);
    if (!count) return nil;
    AVAudioPCMBuffer *result = [[AVAudioPCMBuffer alloc] initWithPCMFormat:source.format
        frameCapacity:count];
    if (result) {
        result.frameLength = count;
        memcpy(result.floatChannelData[0], source.floatChannelData[0] + start,
            count * sizeof(float));
    }
    return result;
}

@implementation U3SIDMusicPlayer
- (id)initWithOffline:(BOOL)offline error:(NSError **)error {
    if (!(self = [super init])) return nil;
    _offline = offline;
    _volumePercent = 100;
    @try {
        _engine = [[AVAudioEngine alloc] init];
        AVAudioFormat *format = [[[AVAudioFormat alloc]
            initStandardFormatWithSampleRate:U3SIDSampleRate channels:1] autorelease];
        /* Set manual mode before touching mixer/output nodes so tests cannot
         * configure the hardware audio device, even briefly. */
        if (offline && ![_engine enableManualRenderingMode:AVAudioEngineManualRenderingModeOffline
            format:format maximumFrameCount:4096 error:error]) {
            [self release];
            return nil;
        }
        _player = [[AVAudioPlayerNode alloc] init];
        [_engine attachNode:_player];
        [_engine connect:_player to:_engine.mainMixerNode format:format];
        /* Keep the mixer mono: a stereo intermediate adds center-pan downmix
         * attenuation. Live output uses the device's clock rate; the mixer
         * performs the required rate conversion without changing SID speed. */
        AVAudioFormat *outputFormat = format;
        if (!offline) {
            AVAudioFormat *hardware = [_engine.outputNode inputFormatForBus:0];
            if (!isfinite(hardware.sampleRate) || hardware.sampleRate <= 0 || !hardware.channelCount) {
                U3SIDError(error, @"No usable audio output device is available.", -19);
                [self release];
                return nil;
            }
            outputFormat = [[[AVAudioFormat alloc] initStandardFormatWithSampleRate:
                hardware.sampleRate channels:1] autorelease];
        }
        [_engine connect:_engine.mainMixerNode to:_engine.outputNode format:outputFormat];
    } @catch (NSException *exception) {
        U3SIDError(error, exception.reason, -1);
        [self release];
        return nil;
    }
    return self;
}

- (void)dealloc {
    [self stop];
    if (_player.engine) [_engine detachNode:_player];
    [_player release];
    [_engine release];
    [_intro release];
    [_loop release];
    [super dealloc];
}

- (BOOL)loadURL:(NSURL *)url loopStartFrame:(AVAudioFramePosition)loopStart
    loopEndFrame:(AVAudioFramePosition)loopEnd error:(NSError **)error {
    [self stop];
    [_intro release]; _intro = nil;
    [_loop release]; _loop = nil;
    _frameCount = _loopStartFrame = _loopEndFrame = 0;
    _lastFramesPlayed = 0;
    if (!url.isFileURL) return U3SIDError(error, @"SID audio requires a local file.", -2);
    NSString *extension = url.pathExtension.lowercaseString;
    if (![@[@"flac", @"wav", @"wave"] containsObject:extension])
        return U3SIDError(error, @"SID audio must be a lossless WAV or FLAC file.", -3);
    NSDictionary *attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:url.path error:error];
    if (!attributes) return NO;
    if ([attributes fileSize] == 0 || [attributes fileSize] >= U3SIDMaximumBytes)
        return U3SIDError(error, @"SID audio file exceeds the supported size.", -4);
    AVAudioFile *file = nil;
    AVAudioPCMBuffer *source = nil;
    @try {
        file = [[AVAudioFile alloc] initForReading:url
            commonFormat:AVAudioPCMFormatFloat32 interleaved:NO error:error];
        if (!file) return NO;
        AVAudioFormat *format = file.processingFormat;
        AVAudioFramePosition frames = file.length;
        if (format.channelCount != 1 || fabs(format.sampleRate - U3SIDSampleRate) > 0.001)
            return U3SIDError(error, @"SID audio must retain its original mono 44,100 Hz format.", -5);
        if (frames <= 0 || (uint64_t)frames >= U3SIDMaximumBytes / sizeof(float))
            return U3SIDError(error, @"Decoded SID audio exceeds the supported size.", -6);
        if (loopStart < 0 || loopEnd <= loopStart || loopEnd > frames)
            return U3SIDError(error, @"SID loop frame bounds are invalid.", -7);
        source = [[AVAudioPCMBuffer alloc] initWithPCMFormat:format
            frameCapacity:(AVAudioFrameCount)frames];
        AVAudioPCMBuffer *chunk = [[[AVAudioPCMBuffer alloc] initWithPCMFormat:format
            frameCapacity:4096] autorelease];
        if (!source || !chunk || !source.floatChannelData) return NO;
        AVAudioFramePosition offset = 0;
        while (offset < frames) {
            AVAudioFrameCount count = (AVAudioFrameCount)MIN(frames - offset, chunk.frameCapacity);
            if (![file readIntoBuffer:chunk frameCount:count error:error]) return NO;
            if (!chunk.frameLength) break;
            memcpy(source.floatChannelData[0] + offset, chunk.floatChannelData[0],
                chunk.frameLength * sizeof(float));
            offset += chunk.frameLength;
        }
        source.frameLength = (AVAudioFrameCount)offset;
        if (offset != frames)
            return U3SIDError(error, @"SID audio is truncated.", -8);
        BOOL nonzero = NO;
        for (AVAudioFramePosition i = 0; i < frames; ++i) {
            float value = source.floatChannelData[0][i];
            if (!isfinite(value)) return U3SIDError(error, @"SID audio contains nonfinite samples.", -9);
            if (i >= loopStart && i < loopEnd) nonzero |= value != 0;
        }
        if (!nonzero) return U3SIDError(error, @"The SID loop contains only silence.", -10);
        AVAudioPCMBuffer *intro = U3SIDSlice(source, 0, loopStart);
        AVAudioPCMBuffer *loop = U3SIDSlice(source, loopStart, loopEnd);
        if ((loopStart && !intro) || !loop) {
            [intro release]; [loop release];
            return U3SIDError(error, @"Cannot allocate SID playback buffers.", -11);
        }
        _intro = intro;
        _loop = loop;
        _frameCount = frames;
        _loopStartFrame = loopStart;
        _loopEndFrame = loopEnd;
    } @catch (NSException *exception) {
        return U3SIDError(error, exception.reason, -12);
    } @finally {
        [source release];
        [file release];
    }
    return YES;
}

- (BOOL)restart:(NSError **)error {
    [self stop];
    _lastFramesPlayed = 0;
    if (!_loop) return U3SIDError(error, @"No valid SID audio is loaded.", -13);
    @try {
        if (_intro) [_player scheduleBuffer:_intro completionHandler:nil];
        [_player scheduleBuffer:_loop atTime:nil options:AVAudioPlayerNodeBufferLoops completionHandler:nil];
        _engine.mainMixerNode.outputVolume = _volumePercent / 100.0f;
        if (![_engine startAndReturnError:error]) {
            [self stop];
            return NO;
        }
        _manualStartFrame = _offline ? _engine.manualRenderingSampleTime : 0;
        [_player play];
        _playing = YES;
    } @catch (NSException *exception) {
        [self stop];
        return U3SIDError(error, exception.reason, -14);
    }
    return YES;
}

- (uint64_t)framesPlayed {
    if (_playing) {
        AVAudioFramePosition frames = 0;
        if (_offline) frames = _engine.manualRenderingSampleTime - _manualStartFrame;
        else {
            AVAudioTime *renderTime = _player.lastRenderTime;
            AVAudioTime *playerTime = renderTime ? [_player playerTimeForNodeTime:renderTime] : nil;
            if (playerTime.isSampleTimeValid) frames = playerTime.sampleTime;
        }
        if (frames > 0) _lastFramesPlayed = MAX(_lastFramesPlayed, (uint64_t)frames);
    }
    return _lastFramesPlayed;
}

- (uint64_t)completedLoops {
    uint64_t played = self.framesPlayed;
    uint64_t loopLength = (uint64_t)(_loopEndFrame - _loopStartFrame);
    return loopLength && played >= (uint64_t)_loopStartFrame ?
        (played - (uint64_t)_loopStartFrame) / loopLength : 0;
}

- (AVAudioFramePosition)currentFrame {
    uint64_t played = self.framesPlayed;
    uint64_t loopLength = (uint64_t)(_loopEndFrame - _loopStartFrame);
    if (!loopLength) return 0;
    if (played < (uint64_t)_loopStartFrame) return (AVAudioFramePosition)played;
    return _loopStartFrame + (AVAudioFramePosition)((played - _loopStartFrame) % loopLength);
}

- (void)stop {
    (void)self.framesPlayed;
    _playing = NO;
    [_player stop]; /* Unschedules all buffers and resets the player timeline. */
    [_engine stop];
}

- (void)setVolumePercent:(NSInteger)percent {
    _volumePercent = MAX(0, MIN(100, percent));
    _engine.mainMixerNode.outputVolume = _volumePercent / 100.0f;
}
- (BOOL)isPlaying { return _playing && _engine.running && _player.playing; }
- (double)sampleRate { return U3SIDSampleRate; }
- (AVAudioFramePosition)frameCount { return _frameCount; }
- (AVAudioFramePosition)loopStartFrame { return _loopStartFrame; }
- (AVAudioFramePosition)loopEndFrame { return _loopEndFrame; }

- (BOOL)renderFrames:(AVAudioFrameCount)frames toURL:(NSURL *)url
    rms:(double *)rms peak:(double *)peak error:(NSError **)error {
    if (!_offline || !frames || !self.isPlaying)
        return U3SIDError(error, @"Offline SID rendering requires a started manual-mode player.", -15);
    AVAudioFormat *format = _engine.manualRenderingFormat;
    AVAudioPCMBuffer *buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:format frameCapacity:4096];
    AVAudioFile *file = url ? [[AVAudioFile alloc] initForWriting:url settings:format.settings error:error] : nil;
    @try {
        if (!buffer || (url && !file)) return NO;
        AVAudioFrameCount remaining = frames;
        double squares = 0, maximum = 0;
        unsigned retries = 0;
        while (remaining) {
            AVAudioFrameCount count = MIN(remaining, buffer.frameCapacity);
            AVAudioEngineManualRenderingStatus status = [_engine renderOffline:count toBuffer:buffer error:error];
            if (status == AVAudioEngineManualRenderingStatusCannotDoInCurrentContext && ++retries < 100) continue;
            if (status != AVAudioEngineManualRenderingStatusSuccess || buffer.frameLength != count)
                return U3SIDError(error, @"The SID audio engine could not render the requested frames.", -16);
            retries = 0;
            for (AVAudioFrameCount frame = 0; frame < count; ++frame) {
                double value = buffer.floatChannelData[0][frame];
                if (!isfinite(value)) return U3SIDError(error, @"SID rendering produced nonfinite samples.", -17);
                squares += value * value;
                maximum = MAX(maximum, fabs(value));
            }
            if (file && ![file writeFromBuffer:buffer error:error]) return NO;
            remaining -= count;
        }
        if (rms) *rms = sqrt(squares / frames);
        if (peak) *peak = maximum;
    } @catch (NSException *exception) {
        return U3SIDError(error, exception.reason, -18);
    } @finally {
        /* Close the WAV now so its header and frame count are usable as soon
         * as this method returns, regardless of the caller's pool lifetime. */
        [file release];
        [buffer release];
    }
    return YES;
}
@end
