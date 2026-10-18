// Source-sample playback for the original two-stage title dissolve.
#ifndef U3_SPLASH_SOUND_H
#define U3_SPLASH_SOUND_H

#import <Foundation/Foundation.h>
#include <stdint.h>

uint32_t U3SplashSoundLegacyCommand(int32_t pass);
uint8_t U3SplashSoundNoteForPass(int32_t pass);
// Pure offline rendering. These functions never create a player or audio device.
NSData *U3SplashSoundWaveData(NSData *resource, int32_t pass, NSError **error);
NSData *U3SplashSoundIntroWaveData(NSData *resource, NSError **error);
BOOL U3SplashSoundSelfTest(void);

@class AVAudioPlayer;
@interface U3SplashSoundPlayer : NSObject {
@private
    NSData *_resource;
    NSMutableDictionary *_notes;
    AVAudioPlayer *_voices[2];
    NSUInteger _nextVoice;
    short _volumePercent;
    BOOL _enabled;
}
- (id)initWithSoundResourceData:(NSData *)data error:(NSError **)error;
- (void)triggerPass:(int32_t)pass;
- (void)setVolumePercent:(short)percent enabled:(BOOL)enabled;
- (void)stop;
@end

#endif
