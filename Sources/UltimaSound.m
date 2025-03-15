// Sound & Speech Routines

#import "UltimaSound.h"

#import "UltimaIncludes.h"
#import "CocoaBridge.h"
#import "UltimaMacIF.h"
#import "UltimaText.h"

#import <QuickTime/QuickTime.h>

extern Boolean          gDone;
extern short            zp[255];

short                   gCurChan, gMaxChan;
short                   gSongCurrent, gSongNext, gSongPlaying;
CGrafPtr                gMoviesPort;
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
Movie                   songMovie = nil;
short                   gQTMusicVolume = 255;

void ApplyVolumePreferences(void) {
    short soundVolume = NumberForPrefsKey(U3PrefSoundVolume);
    if (soundVolume < 1)
        soundVolume = 100;
    SetSoundVolumePercent(soundVolume);

    Boolean isPlayingMusic = (songMovie && gSongPlaying != 0);
    Boolean shouldPlayMusic = !CFPreferencesGetAppBooleanValue(U3PrefMusicInactive, kCFPreferencesCurrentApplication, NULL);
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

    short musicVolume = NumberForPrefsKey(U3PrefMusicVolume);
    if (musicVolume < 1)
        musicVolume = 100;
    gQTMusicVolume = (short)((float)musicVolume * 2.55);
    if (songMovie) {
        SetMovieVolume(songMovie, gQTMusicVolume);
    }
}

void ErrorTone(void) {
    PlaySoundFile(CFSTR("Error1"), TRUE);
}

void PlaySoundFile(CFStringRef soundName, Boolean forceAsync) {
    if (!CFPreferencesGetAppBooleanValue(U3PrefSoundInactive, kCFPreferencesCurrentApplication, NULL)) {
        Boolean async = (forceAsync || CFPreferencesGetAppBooleanValue(U3PrefAsyncSound, kCFPreferencesCurrentApplication, NULL));
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
    if (!CFPreferencesGetAppBooleanValue(U3PrefSoundInactive, kCFPreferencesCurrentApplication, NULL))
        {
        clearChan=FALSE;
        saveMouseState = gMouseState;
        if (!async)
            async = CFPreferencesGetAppBooleanValue(U3PrefAsyncSound, kCFPreferencesCurrentApplication, NULL);
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
    short err, chanType;

    chanType = sampledSynth;
    gCurChan = 1;
    if (gSoundIncapable)
        return;
    err = SndNewChannel(&gSampChan[1], chanType, 0, nil);
    if (err != 0) {
        HandleError(err, 51, 1);
        DisableSound();
        return;
    }
    err = SndNewChannel(&gSampChan[2], chanType, 0, nil);
    if (err != 0) {
        //  HandleError(err, 51, 2);
        gMaxChan = 1;
        return;
    }
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
    if (gSoundIncapable) {
        return;
    }
    SndDisposeChannel(gSampChan[1], FALSE);
    SndDisposeChannel(gSampChan[2], FALSE);
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
    CFPreferencesSetAppValue(U3PrefSpeechInactive, kCFBooleanTrue, kCFPreferencesCurrentApplication);
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
    if (CFPreferencesGetAppBooleanValue(U3PrefSpeechInactive, kCFPreferencesCurrentApplication, NULL))
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
    OSErr err;
    Boolean musicFailed;

    if (gMusicIncapable)
        return;

    musicFailed = false;
    err = EnterMovies();
    if (err)
        gMusicIncapable = true;
    // try to start music-playing code
    if (musicFailed) {
        HandleError(1, 56, 0);
        DisableMusic();
    } else {
        //LWEnableMenuItem((MenuHandle)gSpecialMenu, MUSICID);
    }
}

void CloseMusic(void) {
}

void SetMusicPortAndDevice(CGrafPtr thePort, GDHandle theDevice) {
    SetMovieGWorld(songMovie, thePort, theDevice);
}

void MusicUpdate(void) {
    FSSpec songSpec;
    Str31 SongName = "\pSong_0.mov";
    short songid, movResFile, movResID;    //, vRefNum;
    OSErr err;
    Boolean wasChanged;
    static Boolean last7;

    if (CFPreferencesGetAppBooleanValue(U3PrefMusicInactive, kCFPreferencesCurrentApplication, NULL))
        return;
    if (!songMovie || IsMovieDone(songMovie) || (strk == 7 && last7)) {   // current time >= full time
        if (gSongNext == gSongCurrent) {
            if (songMovie) {
                //printf("replaying (cur=%d, next=%d)\n", gSongCurrent, gSongNext);
                StopMovie(songMovie);
                GoToBeginningOfMovie(songMovie);
                StartMovie(songMovie);
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
        if (songMovie && !IsMovieDone(songMovie)) {
            //printf("ending #2 (cur=%d, next=%d)\n", gSongCurrent, gSongNext);
            EndSong();
            DisposeMovie(songMovie);
            songMovie = nil;
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

    SongName[SongName[0] - 4] = songid;

    FSRef fsr;
    ProcessSerialNumber psn;
    GetCurrentProcess(&psn);
    GetProcessBundleLocation(&psn, &fsr);
    CFURLRef bundleBaseURL = CFURLCreateFromFSRef(nil, &fsr);
    CFURLRef musicBaseURL = CFURLCreateCopyAppendingPathComponent(nil, bundleBaseURL, CFSTR("Contents/Resources/Music"), false);
    CFRelease(bundleBaseURL);
    CFStringRef thisSongStrRef = CFStringCreateWithPascalString(nil, SongName, kCFStringEncodingMacRoman);
    CFURLRef fullSongURLRef = CFURLCreateCopyAppendingPathComponent(nil, musicBaseURL, thisSongStrRef, false);
    CFRelease(thisSongStrRef);
    err = (CFURLGetFSRef(fullSongURLRef, &fsr)) ? noErr : paramErr;
    CFRelease(fullSongURLRef);
    CFRelease(musicBaseURL);
    if (err)
        HandleError(err, 57, 1);
    if (!err)
        err = FSGetCatalogInfo(&fsr, kFSCatInfoNone, nil, nil, &songSpec, nil);
    if (err)
        HandleError(err, 57, 1);
    if (!err)
        err = OpenMovieFile(&songSpec, &movResFile, fsRdPerm);
    if (err)
        HandleError(err, 57, 2);
    movResID = 0;
    if (songMovie)
        DisposeMovie(songMovie);
    if (!err)
        err = NewMovieFromFile(&songMovie, movResFile, &movResID, SongName, newMovieActive, &wasChanged);
    if (err)
        HandleError(err, 57, 3);
    if (!err)
        err = CloseMovieFile(movResFile);
    if (err)
        HandleError(err, 57, 4);
    if (!err) {
        GoToBeginningOfMovie(songMovie);
        SetMovieGWorld(songMovie, gMoviesPort, nil);    // it's only audio!
        SetMovieVolume(songMovie, gQTMusicVolume);
        StartMovie(songMovie);
    }
    strk = gSongPlaying;
}

void EndSong(void) {
    long startTime;

    if (songMovie) {
        startTime = TickCount();
        const int numTicks = 30;
        float scale = gQTMusicVolume / (float)numTicks;
        while (TickCount() < (startTime + numTicks)) {
            int newVolume = gQTMusicVolume - (TickCount() - startTime) * scale;
            SetMovieVolume(songMovie, newVolume);
            ThreadSleepTicks(3);
        }
        StopMovie(songMovie);
        SetMovieVolume(songMovie, gQTMusicVolume);
    }
}
