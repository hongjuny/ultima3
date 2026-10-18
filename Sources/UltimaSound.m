// Sound & Speech Routines

#import "UltimaSound.h"

#import "UltimaIncludes.h"
#import "CocoaBridge.h"
#import "U3Platform.h"
#import "UltimaMacIF.h"
#import "UltimaText.h"

#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>
#import "U3SIDMusic.h"
#include <math.h>

extern Boolean          gDone;
extern unsigned char    Party[65];
extern short            zp[255];

typedef struct SndChannel *SndChannelPtr;

void PlaySoundFile(CFStringRef soundName, Boolean forceAsync);
void DisableSpeech(void);
void SetUpVoiceList(void);
void GetVoice(Str255 voicename);
void CloseSpeech(void);
void MusicUpdate(void);
void EndSong(void);

short                   gCurChan, gMaxChan;
short                   gSongCurrent, gSongNext, gSongPlaying;
SndChannelPtr           gSampChan[6], gToneChan;
Handle                  gSoundHandle[6];
Boolean                 gSpeech, gSoundDone, gSoundIncapable, gMusicIncapable;
VoiceSpec               myVoice;
VoiceDescription        myVoiceDesc;
SpeechChannel           myChannel;
short                   gCountVoices, gCurrentVoiceIndex, gCurVoiceNum;
short                   gCurSong, gSongVolRefNum;
Str255                  gCurVoiceName;
unsigned char           gVoiceName[64][64], strk;
static AVAudioPlayer    *sFadeTonePlayer = nil;
short                   gQTMusicVolume = 100;

/* The soundtrack is rendered from the original C64 SID program. Runtime
 * playback never remaps its instruments through MIDI or a system sound bank. */
static U3SIDMusicPlayer *songPlayer = nil;
static short sFailedSong;
static uint64_t sLastLoopCount;
static NSDictionary *sSIDTracks;

static NSError *MusicError(NSString *message, NSInteger code) {
    return [NSError errorWithDomain:@"UltimaIIIMusic" code:code
        userInfo:@{NSLocalizedDescriptionKey: message}];
}

static NSURL *SIDMusicDirectory(void) {
    return [[NSBundle mainBundle] URLForResource:@"MusicSID" withExtension:nil];
}

static BOOL SIDInteger(id value, int64_t minimum, int64_t maximum) {
    if (![value isKindOfClass:[NSNumber class]]) return NO;
    double number = [value doubleValue];
    return isfinite(number) && number >= minimum && number <= maximum && floor(number) == number;
}

static NSDictionary *SIDMusicTracks(NSError **error) {
    if (sSIDTracks) return sSIDTracks;
    NSURL *url = [SIDMusicDirectory() URLByAppendingPathComponent:@"manifest.json"];
    NSData *data = url ? [NSData dataWithContentsOfURL:url options:0 error:error] : nil;
    id manifest = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:error] : nil;
    id entries = [manifest isKindOfClass:[NSDictionary class]] ? manifest[@"tracks"] : nil;
    if (![manifest isKindOfClass:[NSDictionary class]] ||
        ![manifest[@"schema_version"] isEqual:@1] ||
        ![entries isKindOfClass:[NSArray class]] || [entries count] != 10) {
        if (error) *error = MusicError(@"The C64 SID soundtrack manifest is missing or invalid.", -2);
        return nil;
    }
    NSMutableDictionary *tracks = [NSMutableDictionary dictionaryWithCapacity:10];
    for (id entry in entries) {
        if (![entry isKindOfClass:[NSDictionary class]]) return nil;
        id song = entry[@"game_song"], subtune = entry[@"subtune"];
        id frames = entry[@"frame_count"], start = entry[@"loop_start_frame"], end = entry[@"loop_end_frame"];
        id file = entry[@"file"];
        if (!SIDInteger(song, 1, 11) || [song intValue] == 9 || tracks[song] ||
            !SIDInteger(subtune, 1, 10) ||
            [subtune intValue] != [song intValue] - ([song intValue] >= 10 ? 1 : 0) ||
            !SIDInteger(frames, 1, 25000000) || !SIDInteger(start, 0, [frames longLongValue] - 1) ||
            !SIDInteger(end, [start longLongValue] + 1, [frames longLongValue]) ||
            ![entry[@"sample_rate"] isEqual:@44100] || ![entry[@"channels"] isEqual:@1] ||
            ![entry[@"bits"] isEqual:@16] || ![file isKindOfClass:[NSString class]] ||
            ![[file lastPathComponent] isEqual:file] || ![[file pathExtension] isEqual:@"flac"]) {
            if (error) *error = MusicError(@"The C64 SID soundtrack contains an invalid track.", -3);
            return nil;
        }
        tracks[song] = entry;
    }
    sSIDTracks = [tracks copy];
    return sSIDTracks;
}

static BOOL LoadSIDMusic(U3SIDMusicPlayer *player, short song, NSError **error) {
    NSDictionary *track = SIDMusicTracks(error)[@(song)];
    if (!track) return NO;
    NSURL *url = [SIDMusicDirectory() URLByAppendingPathComponent:track[@"file"]];
    return [player loadURL:url loopStartFrame:[track[@"loop_start_frame"] longLongValue]
        loopEndFrame:[track[@"loop_end_frame"] longLongValue] error:error] &&
        player.frameCount == [track[@"frame_count"] longLongValue];
}

void SetMusicVolumePercent(short volumePercent) {
    gQTMusicVolume = MAX(0, MIN(100, volumePercent));
    [songPlayer setVolumePercent:gQTMusicVolume];
}

void ApplyVolumePreferences(void) {
    short soundVolume = U3PlatformGetIntegerPreference(U3PreferenceSoundVolume);
    if (soundVolume < 1)
        soundVolume = 100;
    SetSoundVolumePercent(soundVolume);

    short musicVolume = U3PlatformGetIntegerPreference(U3PreferenceMusicVolume);
    if (!U3PlatformHasPreference(U3PreferenceMusicVolume))
        musicVolume = 100;
    SetMusicVolumePercent(musicVolume);
    /* A deliberate preference change also permits retrying a failed device. */
    sFailedSong = 0;
    if (U3PlatformGetBooleanPreference(U3PreferenceMusicDisabled)) EndSong();
    else MusicUpdate();
}

bool U3AudioMusicSelfTest(void) {
    NSError *error = nil;
    U3SIDMusicPlayer *player = [[[U3SIDMusicPlayer alloc] initWithOffline:YES error:&error] autorelease];
    NSDictionary *tracks = SIDMusicTracks(&error);
    if (!player || !tracks) {
        fprintf(stderr, "Offline SID initialization failed: %s\n", error.localizedDescription.UTF8String);
        return false;
    }
    const char *renderDirectory = getenv("U3_AUDIO_RENDER_DIRECTORY");
    const char *renderPath = getenv("U3_AUDIO_RENDER_PATH");
    const char *renderSong = getenv("U3_AUDIO_RENDER_SONG");
    const char *renderSeconds = getenv("U3_AUDIO_RENDER_SECONDS");
    const char *renderVolume = getenv("U3_AUDIO_RENDER_VOLUME");
    double seconds = renderSeconds ? strtod(renderSeconds, NULL) : 15.0;
    double volume = renderVolume ? strtod(renderVolume, NULL) : 100.0;
    if (!isfinite(seconds) || seconds <= 0 || seconds > 600 ||
        !isfinite(volume) || volume < 0 || volume > 100) return false;
    NSString *suffix = renderSong ? [NSString stringWithUTF8String:renderSong] : (renderPath ? @"1" : nil);
    NSDictionary *songs = @{@"1":@1, @"2":@2, @"3":@3, @"4":@4, @"5":@5,
        @"6":@6, @"7":@7, @"8":@8, @"A":@10, @"B":@11};
    if (suffix && !songs[suffix]) return false;
    NSURL *directory = renderDirectory ? [NSURL fileURLWithPath:
        [NSString stringWithUTF8String:renderDirectory] isDirectory:YES] : nil;
    if (directory && ![[NSFileManager defaultManager] createDirectoryAtURL:directory
        withIntermediateDirectories:YES attributes:nil error:&error]) return false;
    for (NSString *name in [songs.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
        if (suffix && ![suffix isEqual:name]) continue;
        @autoreleasepool {
            short song = [songs[name] shortValue];
            if (!LoadSIDMusic(player, song, &error)) return false;
            [player setVolumePercent:(NSInteger)volume];
            if (![player restart:&error]) return false;
            NSURL *output = directory ? [directory URLByAppendingPathComponent:
                [NSString stringWithFormat:@"SID_%@.wav", name]] : nil;
            if (renderPath) output = [NSURL fileURLWithPath:[NSString stringWithUTF8String:renderPath]];
            AVAudioFrameCount frames = (AVAudioFrameCount)((output || renderSeconds) ?
                llround(seconds * player.sampleRate) : player.loopEndFrame + 11025);
            double rms = 0, peak = 0;
            if (![player renderFrames:frames toURL:output rms:&rms peak:&peak error:&error] ||
                (volume > 0 && rms < 0.00001) || peak >= 1.0 ||
                player.framesPlayed != frames || !player.isPlaying ||
                (!(output || renderSeconds) && player.completedLoops < 1)) {
                fprintf(stderr, "SID %s rendering failed: %s\n", name.UTF8String,
                    error ? error.localizedDescription.UTF8String : "invalid level, loop or timeline");
                return false;
            }
            fprintf(stderr, "SID %s: subtune=%d frames=%lld loop=%lld..%lld RMS=%.5f peak=%.5f\n",
                name.UTF8String, [tracks[@(song)][@"subtune"] intValue], player.frameCount,
                player.loopStartFrame, player.loopEndFrame, rms, peak);
        }
    }
    if (!LoadSIDMusic(player, 8, &error)) return false;
    double levels[3] = {0}, peaks[3] = {0};
    const NSInteger volumes[] = {100, 25, 0};
    for (int i = 0; i < 3; ++i) {
        [player setVolumePercent:volumes[i]];
        if (![player restart:&error] || ![player renderFrames:176400 toURL:nil
            rms:&levels[i] peak:&peaks[i] error:&error]) return false;
    }
    [player stop];
    BOOL passed = levels[0] > 0.00001 && fabs(levels[1] - levels[0] / 4) < 0.000001 &&
        peaks[0] < 1.0 && peaks[2] == 0 && !player.isPlaying;
    [player setVolumePercent:25];
    double restartRMS = 0, restartPeak = 0;
    passed = passed && [player restart:&error] &&
        [player renderFrames:176400 toURL:nil rms:&restartRMS peak:&restartPeak error:&error] &&
        fabs(restartRMS - levels[1]) < 0.000001 && restartPeak < 1.0;
    [player stop];
    fprintf(stderr, "Offline SID gain: 100%%=%.5f 25%%=%.5f 0%%=%.8f restart=%s\n",
        levels[0], levels[1], levels[2], passed ? "passed" : "FAILED");
    return passed;
}

void ErrorTone(void) {
    PlaySoundFile(CFSTR("Error1"), TRUE);
}

void PlaySoundFile(CFStringRef soundName, Boolean forceAsync) {
    if (!U3PlatformGetBooleanPreference(U3PreferenceSoundDisabled)) {
        Boolean async = (forceAsync || U3PlatformGetBooleanPreference(U3PreferenceAsyncSound));
        PlaySoundFileQT(soundName, async);
    }
}

static NSData *FadeToneData(int32_t pass) {
    const uint32_t sampleRate = 44100;
    const uint32_t frameCount = sampleRate * 3;
    const uint32_t dataSize = frameCount * sizeof(int16_t);
    (void)pass;
    NSMutableData *data = [NSMutableData dataWithLength:44 + dataSize];
    uint8_t *bytes = [data mutableBytes];
    memcpy(bytes, "RIFF", 4);
    uint32_t riffSize = 36 + dataSize;
    memcpy(bytes + 4, &riffSize, sizeof(riffSize));
    memcpy(bytes + 8, "WAVEfmt ", 8);
    uint32_t fmtSize = 16;
    uint16_t format = 1, channels = 1, bits = 16;
    uint32_t byteRate = sampleRate * channels * sizeof(int16_t);
    uint16_t blockAlign = channels * sizeof(int16_t);
    memcpy(bytes + 16, &fmtSize, sizeof(fmtSize));
    memcpy(bytes + 20, &format, sizeof(format));
    memcpy(bytes + 22, &channels, sizeof(channels));
    memcpy(bytes + 24, &sampleRate, sizeof(sampleRate));
    memcpy(bytes + 28, &byteRate, sizeof(byteRate));
    memcpy(bytes + 32, &blockAlign, sizeof(blockAlign));
    memcpy(bytes + 34, &bits, sizeof(bits));
    memcpy(bytes + 36, "data", 4);
    memcpy(bytes + 40, &dataSize, sizeof(dataSize));

    int16_t *samples = (int16_t *)(bytes + 44);
    uint32_t lfsr = 0x1FFFF;
    uint32_t noiseClock = 0;
    int noiseBit = 0;
    for (uint32_t i = 0; i < frameCount; ++i) {
        if (++noiseClock >= 7) {
            noiseClock = 0;
            uint32_t feedback = (lfsr ^ (lfsr >> 3)) & 1;
            noiseBit = lfsr & 1;
            lfsr = (lfsr >> 1) | (feedback << 16);
        }
        double attack = MIN(1.0, (double)i / (sampleRate * 0.04));
        double release = MIN(1.0, (double)(frameCount - i) / (sampleRate * 0.08));
        double envelope = attack * release;
        double sample = (noiseBit ? 0.58 : -0.58) * envelope;
        samples[i] = (int16_t)(MAX(-1.0, MIN(1.0, sample)) * 28000.0);
    }
    return data;
}

void PlayLegacyFadeTone(int32_t pass) {
    if (U3CocoaIsHeadlessDiagnostic() || U3PlatformGetBooleanPreference(U3PreferenceSoundDisabled))
        return;

    if (sFadeTonePlayer)
        return;

    NSError *error = nil;
    AVAudioPlayer *player = [[AVAudioPlayer alloc] initWithData:FadeToneData(pass) error:&error];
    if (player && [player prepareToPlay]) {
        [player setVolume:0.75f];
        sFadeTonePlayer = player;
        [player play];
    } else if (error) {
        NSLog(@"Cannot play Exodus fade tone: %@", [error localizedDescription]);
    }
    if (player != sFadeTonePlayer)
        [player release];
}

void StopLegacyFadeTone(void) {
    [sFadeTonePlayer stop];
    [sFadeTonePlayer release];
    sFadeTonePlayer = nil;
}
/*
void PlaySound(unsigned short what,Boolean async) // $4705
{
    Boolean             clearChan;
    long                err;
    SCStatus            status;
    short               saveMouseState;
    if (gDone) return;
    if (!U3PlatformGetBooleanPreference(U3PreferenceSoundDisabled))
        {
        clearChan=FALSE;
        saveMouseState = gMouseState;
        if (!async)
            async = U3PlatformGetBooleanPreference(U3PreferenceAsyncSound);
        if (!async) { gMouseState = 0; CursorUpdate(); }
        while (clearChan==FALSE)
            {
            err = SndChannelStatus(gSampChan[gCurChan],sizeof(status),&status);
            if (status.scChannelBusy)
                {
                gCurChan++;
                if (gCurChan>gMaxChan) gCurChan=1;
                }
            else
                {
                clearChan=TRUE;
                }
            }
        gSoundHandle[gCurChan] = GetResource ('snd ', BASERES+(0xFF-what));
        if (gSoundHandle[gCurChan] == nil) HandleError(ResError(), 49, BASERES+(0xFF-what));
        err = SndPlay (gSampChan[gCurChan], (SndListHandle)gSoundHandle[gCurChan], async);
        if (err != 0) HandleError(err, 50, BASERES+(0xFF-what));
        gMouseState = saveMouseState;
        CursorUpdate();
        }
}
*/
void OpenChannel(void) {
    gCurChan = 1;
    gMaxChan = 2;
/*  err = SndNewChannel (&gSampChan[3], chanType, 0, nil);
    if (err != 0)
        {
    //  HandleError(err, 51, 3);
        gMaxChan=2;
        return;
        }
    err = SndNewChannel (&gSampChan[4], chanType, 0, nil);
    if (err != 0)
        {
    //  HandleError(err, 51, 4);
        gMaxChan=3;
        return;
        }
    err = SndNewChannel (&gSampChan[5], chanType, 0, nil);
    if (err != 0)
        {
    //  HandleError(err, 51, 4);
        gMaxChan=4;
        return;
        }
    gMaxChan=5;
*/}

void CloseChannel(void) {
}

void SetUpSpeech(void) {
    NumVersion versionNum;
    short button;
    long result;
    Str255 errorStr;

    Gestalt(gestaltSpeechAttr, &result);
    if (!(result & 0x01)) {
        DisableSpeech();
    } else {
        versionNum = SpeechManagerVersion();
        if (versionNum.majorRev != 0 && versionNum.majorRev < 2 && ((versionNum.minorAndBugRev & 0xF0) >> 4) < 3) {
            GetIndString(errorStr, BASERES + 9, 8);
            ParamText(errorStr, nil, nil, nil);
            button = Alert(BASERES + 6, NIL_PTR);
            if (button == 1)
                ExitToShell();
            DisableSpeech();
        } else {
            gSpeech = TRUE;
            myChannel = nil;
            SetUpVoiceList();
            GetVoice("\pFred");
        }
    }
}

void DisableSpeech(void) {
    gSpeech = FALSE;
    U3PlatformSetBooleanPreference(U3PreferenceSpeechDisabled, true);
    ReflectPrefs();
}

void SetUpVoiceList(void) {
    long error;
    short count;

    if (gSpeech == FALSE)
        return;
    error = CountVoices(&gCountVoices);
    if (error != 0)
        HandleError(error, 52, 1);
    if (gCountVoices > 63)
        gCountVoices = 63;
    for (count = 1; count <= gCountVoices; count++) {
        error = GetIndVoice(count, &myVoice);
        if (error != 0) {
            HandleError(error, 52, 2);
            DisableSpeech();
            return;
        }
        error = GetVoiceDescription(&myVoice, &myVoiceDesc, sizeof(VoiceDescription));
        if (error != 0) {
            HandleError(error, 52, 3);
            DisableSpeech();
            return;
        }
        if (myVoiceDesc.name[0] < 64)
            memcpy(gVoiceName[count], myVoiceDesc.name, myVoiceDesc.name[0] + 1);
    }
}

void GetVoice(Str255 voicename) {
    long error;
    Str255 secondname = "\pFred";
    short voicefirst, voicesecond, count;

    if (gSpeech == FALSE)
        return;
    CloseSpeech();
    voicesecond = 1;
    voicefirst = -1;
    for (count = 1; count <= gCountVoices; count++) {
        if (EqualString(gVoiceName[count], voicename, FALSE, FALSE)) {
            voicefirst = count;
            count = gCountVoices + 1;
        }
        if (EqualString(gVoiceName[count], secondname, FALSE, FALSE)) {
            voicesecond = count;
        }
    }
    if (voicefirst == -1) {
        gCurVoiceNum = voicesecond;
    } else {
        gCurVoiceNum = voicefirst;
    }
    error = GetIndVoice(gCurVoiceNum, &myVoice);
    if (error != 0) {
        HandleError(error, 52, 4);
        DisableSpeech();
        return;
    }
    error = GetVoiceDescription(&myVoice, &myVoiceDesc, sizeof(VoiceDescription));
    if (error != 0) {
        HandleError(error, 52, 5);
        DisableSpeech();
        return;
    }
    error = NewSpeechChannel(&myVoice, &myChannel);
    if (error != 0) {
        HandleError(error, 53, 0);
        DisableSpeech();
        return;
    }
}

void CloseSpeech(void) {
    if (myChannel)
        DisposeSpeechChannel(myChannel);
}

void Speech(Str255 string, short shnum) {
    short byte;
    static Str255 speakString;
    static short lastVoice;

    if (gSpeech == FALSE || gDone == TRUE)
        return;
    if (U3PlatformGetBooleanPreference(U3PreferenceSpeechDisabled))
        return;
    if (lastVoice != shnum) {
        GetPascalStringFromArrayByIndex(speakString, CFSTR("TilesVoices"), shnum);
        GetVoice(speakString);
    }
    lastVoice = shnum;
    byte = 1;
    while (byte < string[0]) {
        if (string[byte] == ':') {
            BlockMove(string + byte + 1, string + 1, string[0] - (byte - 1));
            string[0] -= byte;
        }
        byte++;
    }
    BlockMove(string, speakString, string[0] + 1);
    SpeakText(myChannel, speakString + 1, speakString[0]);
}

void SpeakMessages(int msg1, int msg2, int voiceNum) {
    Str255 theString = "\p";
    GetPascalStringFromArrayByIndex(theString, CFSTR("Messages"), msg1 - 1);

    // Concatenate the second string if asked for
    if (msg2) {
        Str255 addlString = "\p";
        GetPascalStringFromArrayByIndex(addlString, CFSTR("Messages"), msg2 - 1);
        theString[++theString[0]] = ' ';
        memcpy(theString + theString[0], addlString + 1, addlString[0]);
        theString[0] += addlString[0];
    }

    SearchReplace(theString, "\pgp", "\p gold");
    SearchReplace(theString, "\pEVOCARE", "\pee voh care");
    SearchReplace(theString, "\pSOSARIA", "\pso saria");
    SearchReplace(theString, "\pg.p.", "\p gold");
    SearchReplace(theString, "\p(Y/N)", "\p");

    // Replace any strange chars with spaces.
    int i;
    for (i = 1; i <= theString[0]; i++) {
        if (theString[i] < ' ' || theString[i] > 'z')
            theString[i] = ' ';
    }

    Speech(theString, voiceNum);
}

void SetUpMusic(void) {
    Boolean musicFailed;

    if (gMusicIncapable)
        return;

    musicFailed = false;
    // try to start music-playing code
    if (musicFailed) {
        HandleError(1, 56, 0);
        DisableMusic();
    } else {
        //LWEnableMenuItem((MenuHandle)gSpecialMenu, MUSICID);
    }
}

void CloseMusic(void) {
    EndSong();
    [songPlayer release];
    songPlayer = nil;
    sFailedSong = 0;
    [sSIDTracks release];
    sSIDTracks = nil;
}

void SetMusicPortAndDevice(CGrafPtr thePort, GDHandle theDevice) {
}

void MusicUpdate(void) {
    /* Dedicated diagnostics exercise this player offline, without an audio device. */
    if (U3CocoaIsHeadlessDiagnostic()) return;
    if (U3PlatformGetBooleanPreference(U3PreferenceMusicDisabled)) {
        if (gSongPlaying) EndSong();
        return;
    }
    if (Party[3] == 0x80) gSongCurrent = gSongNext = 5;
    if (gSongCurrent < 0 || gSongCurrent > 0x10) gSongCurrent = 0;
    if (!gSongCurrent) gSongCurrent = gSongNext;
    if (gSongPlaying == gSongCurrent && songPlayer.isPlaying) {
        /* The initial score also ends at loopStartFrame. Honor queued changes
         * there, then at each complete repeat. Scene changes remain immediate. */
        uint64_t boundaries = songPlayer.loopStartFrame > 0 ?
            (songPlayer.framesPlayed >= (uint64_t)songPlayer.loopStartFrame ? songPlayer.completedLoops + 1 : 0) :
            songPlayer.completedLoops;
        BOOL atBoundary = boundaries > sLastLoopCount;
        sLastLoopCount = boundaries;
        if (!atBoundary || gSongNext == gSongCurrent) return;
        gSongCurrent = gSongNext;
    }
    if (gSongCurrent <= 0 || gSongCurrent == 9 || gSongCurrent >= 12) {
        if (gSongPlaying) EndSong();
        return;
    }
    if (sFailedSong == gSongCurrent) return;
    @autoreleasepool {
        NSError *error = nil;
        @try {
            if (!songPlayer) songPlayer = [[U3SIDMusicPlayer alloc] initWithOffline:NO error:&error];
            BOOL ready = songPlayer && LoadSIDMusic(songPlayer, gSongCurrent, &error);
            if (ready) {
                [songPlayer setVolumePercent:gQTMusicVolume];
                ready = [songPlayer restart:&error];
            }
            if (ready) {
                gSongPlaying = gSongCurrent;
                strk = gSongPlaying;
                sFailedSong = 0;
                sLastLoopCount = 0;
                return;
            }
        } @catch (NSException *exception) {
            error = MusicError(exception.reason, -1);
        }
        EndSong();
        sFailedSong = gSongCurrent;
        fprintf(stderr, "C64 SID song %d unavailable: %s\n", gSongCurrent,
            error ? error.localizedDescription.UTF8String : "missing or invalid soundtrack assets");
    }
}

void EndSong(void) {
    [songPlayer stop];
    gSongPlaying = 0;
    sLastLoopCount = 0;
}
