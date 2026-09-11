// Sound & Speech Routines

#import "UltimaSound.h"

#import "UltimaIncludes.h"
#import "CocoaBridge.h"
#import "U3Platform.h"
#import "UltimaMacIF.h"
#import "UltimaText.h"

#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>

extern Boolean          gDone;
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
static AVMIDIPlayer     *songPlayer = nil;
short                   gQTMusicVolume = 100;

static NSURL *MusicSoundBankURL(void) {
    return [[NSBundle mainBundle] URLForResource:@"GeneralUser-GS" withExtension:@"sf2"
                                   subdirectory:@"MusicMIDI"];
}

static Boolean MusicIsPlaying(void) {
    return songPlayer && [songPlayer isPlaying];
}

static void SetMusicVolume(float volume) {
    /* AVMIDIPlayer has no volume API. Keep the preference for a future mixer. */
    (void)volume;
}

void ApplyVolumePreferences(void) {
    short soundVolume = U3PlatformGetIntegerPreference(U3PreferenceSoundVolume);
    if (soundVolume < 1)
        soundVolume = 100;
    SetSoundVolumePercent(soundVolume);

    Boolean isPlayingMusic = (songPlayer && gSongPlaying != 0);
    Boolean shouldPlayMusic = !U3PlatformGetBooleanPreference(U3PreferenceMusicDisabled);
    if (isPlayingMusic != shouldPlayMusic) {
        if (shouldPlayMusic) {
            gSongPlaying = 0xFF;
            gSongCurrent = 0;
            MusicUpdate();
        } else {
            EndSong();
            gSongPlaying = 0;
        }
    }

    short musicVolume = U3PlatformGetIntegerPreference(U3PreferenceMusicVolume);
    if (musicVolume < 1)
        musicVolume = 100;
    gQTMusicVolume = musicVolume;
    SetMusicVolume((float)gQTMusicVolume / 100.0f);
}

bool U3AudioMusicSelfTest(void) {
    NSString *path = [[NSBundle mainBundle] pathForResource:@"Song_1" ofType:@"mid" inDirectory:@"MusicMIDI"];
    if (!path || !MusicSoundBankURL())
        return false;
    if (U3CocoaIsHeadlessDiagnostic() && !getenv("U3_VERIFY_MIDI_DEVICE")) {
        fprintf(stderr, "MIDI asset test: passed (playback deferred outside GUI)\n");
        return true;
    }
    NSError *error = nil;
    NSData *data = [NSData dataWithContentsOfFile:path];
    AVMIDIPlayer *player = nil;
    @try {
        player = [[[AVMIDIPlayer alloc] initWithData:data soundBankURL:MusicSoundBankURL() error:&error] autorelease];
        [player prepareToPlay];
        [player play:nil];
    } @catch (NSException *exception) {
        fprintf(stderr, "MIDI playback unavailable: %s\n", [[exception reason] UTF8String]);
        return false;
    }
    if (!player) {
        fprintf(stderr, "MIDI playback unavailable: %s\n", error ? [[error localizedDescription] UTF8String] : "unknown error");
        return false;
    }
    Boolean playing = [player isPlaying];
    fprintf(stderr, "Fixed-bank MIDI: duration=%.2f playing=%d\n", [player duration], playing);
    [player stop];
    if (!playing || [player duration] <= 0)
        return false;
    NSString *effectPath = [[NSBundle mainBundle] pathForResource:@"Step" ofType:@"wav" inDirectory:@"SoundsPCM"];
    if (!effectPath)
        return false;
    Boolean effectPassed = [[NSFileManager defaultManager] fileExistsAtPath:effectPath];
    return effectPassed;
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
}

void SetMusicPortAndDevice(CGrafPtr thePort, GDHandle theDevice) {
}

void MusicUpdate(void) {
    short songid;
    static Boolean last7;

    if (U3PlatformGetBooleanPreference(U3PreferenceMusicDisabled))
        return;
    if (!MusicIsPlaying() || (strk == 7 && last7)) {   // current time >= full time
        if (gSongNext == gSongCurrent) {
            if (songPlayer) {
                //printf("replaying (cur=%d, next=%d)\n", gSongCurrent, gSongNext);
                [songPlayer setCurrentPosition:0.0];
                [songPlayer play:nil];
            }
        } else {
            //printf("ending #1 (cur=%d, next=%d)\n", gSongCurrent, gSongNext);
            EndSong();
            gSongCurrent = gSongNext;
            gSongPlaying = 0;
        }
    }

    last7 = false;    //(gSongCurrent==7); // why is this in here??
    if ((gSongCurrent == gSongPlaying) && gSongPlaying != 0)
        return;
    if (gSongCurrent > 0x10)
        gSongCurrent = 0;
    if (gSongCurrent == 0)
        gSongCurrent = gSongNext;
    gSongPlaying = gSongCurrent;
    if (gSongCurrent == 0) {
        if (songPlayer && MusicIsPlaying()) {
            //printf("ending #2 (cur=%d, next=%d)\n", gSongCurrent, gSongNext);
            EndSong();
            [songPlayer release];
            songPlayer = nil;
        }
        return;
    }
    songid = '0' + gSongPlaying;
    if (songid > '9')
        songid += 7;
    if (songid == '9' || songid == 'C' || songid == 'D' || songid == 'E' || songid == 'F')
        return;
    //printf("ending #3 (cur=%d, next=%d)\n", gSongCurrent, gSongNext);
    EndSong();

    NSString *songName = [NSString stringWithFormat:@"Song_%c", songid];
    NSString *path = [[NSBundle mainBundle] pathForResource:songName ofType:@"mid" inDirectory:@"MusicMIDI"];
    NSURL *soundBank = MusicSoundBankURL();
    if (!path || !soundBank) {
        fprintf(stderr, "Music asset missing: %s or GeneralUser-GS.sf2\n", [songName UTF8String]);
        gSongPlaying = 0;
        HandleError(paramErr, 57, 1);
        return;
    }
    NSData *songData = [NSData dataWithContentsOfFile:path];
    NSError *error = nil;
    AVMIDIPlayer *newPlayer = nil;
    @try {
        newPlayer = [[AVMIDIPlayer alloc] initWithData:songData soundBankURL:soundBank error:&error];
    } @catch (NSException *exception) {
        fprintf(stderr, "MIDI playback unavailable: %s\n", [[exception reason] UTF8String]);
        gSongPlaying = 0;
        return;
    }
    if (!newPlayer) {
        fprintf(stderr, "Music unavailable: %s\n", [path UTF8String]);
        if (error)
            fprintf(stderr, "MIDI error: %s\n", [[error localizedDescription] UTF8String]);
        return;
    }
    [songPlayer release];
    songPlayer = newPlayer;
    [songPlayer prepareToPlay];
    [songPlayer play:nil];
    strk = gSongPlaying;
}

void EndSong(void) {
    long startTime;

    if (songPlayer) {
        startTime = U3PlatformTickCount();
        const int numTicks = 30;
        float scale = gQTMusicVolume / (float)numTicks;
        while (U3PlatformTickCount() < (startTime + numTicks)) {
            int newVolume = gQTMusicVolume - (U3PlatformTickCount() - startTime) * scale;
            SetMusicVolume((float)newVolume / 100.0f);
            U3PlatformWaitTicks(3);
        }
        [songPlayer stop];
        SetMusicVolume((float)gQTMusicVolume / 100.0f);
    }
}
