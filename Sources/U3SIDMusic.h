#import <AVFoundation/AVFoundation.h>
#include <stdint.h>

/* Lossless playback of pre-rendered C64 SID audio. Frame positions refer to
 * the original mono 44,100 Hz PCM, including any intro before the loop. */
@interface U3SIDMusicPlayer : NSObject {
    AVAudioEngine *_engine;
    AVAudioPlayerNode *_player;
    AVAudioPCMBuffer *_intro;
    AVAudioPCMBuffer *_loop;
    AVAudioFramePosition _frameCount;
    AVAudioFramePosition _loopStartFrame;
    AVAudioFramePosition _loopEndFrame;
    AVAudioFramePosition _manualStartFrame;
    uint64_t _lastFramesPlayed;
    NSInteger _volumePercent;
    BOOL _offline;
    BOOL _playing;
}
- (id)initWithOffline:(BOOL)offline error:(NSError **)error;
- (BOOL)loadURL:(NSURL *)url loopStartFrame:(AVAudioFramePosition)loopStart
    loopEndFrame:(AVAudioFramePosition)loopEnd error:(NSError **)error;
- (BOOL)restart:(NSError **)error;
- (void)stop;
- (void)setVolumePercent:(NSInteger)percent;
@property(nonatomic, readonly) BOOL isPlaying;
@property(nonatomic, readonly) double sampleRate;
@property(nonatomic, readonly) AVAudioFramePosition frameCount;
@property(nonatomic, readonly) AVAudioFramePosition loopStartFrame;
@property(nonatomic, readonly) AVAudioFramePosition loopEndFrame;
@property(nonatomic, readonly) uint64_t framesPlayed;
@property(nonatomic, readonly) uint64_t completedLoops;
@property(nonatomic, readonly) AVAudioFramePosition currentFrame;
/* Diagnostic only: requires initWithOffline:YES and never opens an audio
 * output device. An optional WAV URL receives exactly the rendered frames. */
- (BOOL)renderFrames:(AVAudioFrameCount)frames toURL:(NSURL *)url
    rms:(double *)rms peak:(double *)peak error:(NSError **)error;
@end
