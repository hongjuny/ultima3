//
//  U3AudioLegacy.m
//  Ultima3
//
//  Legacy implementation of the portable audio boundary.
//

#import "U3Audio.h"

#import "CocoaBridge.h"
#import "UltimaIncludes.h"

extern void ApplyVolumePreferences(void);
extern void PlaySoundFile(CFStringRef soundName, Boolean forceAsync);
extern void OpenChannel(void);
extern void CloseChannel(void);
extern void SetUpSpeech(void);
extern void CloseSpeech(void);
extern void Speech(Str255 string, short shnum);
extern void SpeakMessages(int msg1, int msg2, int voiceNum);
extern void SetUpMusic(void);
extern void CloseMusic(void);
extern void MusicUpdate(void);
extern void EndSong(void);
extern short gSongCurrent, gSongNext;

extern short gCurChan, gMaxChan;

static CFStringRef U3LegacySoundNameForEffect(U3SoundEffect effect) {
    switch (effect) {
        case U3SoundEffectAlarm: return CFSTR("Alarm");
        case U3SoundEffectAttack: return CFSTR("Attack");
        case U3SoundEffectBigDeath: return CFSTR("BigDeath");
        case U3SoundEffectBump: return CFSTR("Bump");
        case U3SoundEffectCombatStart: return CFSTR("CombatStart");
        case U3SoundEffectCombatVictory: return CFSTR("CombatVictory");
        case U3SoundEffectCreak: return CFSTR("Creak");
        case U3SoundEffectDeathFemale: return CFSTR("DeathFemale");
        case U3SoundEffectDeathMale: return CFSTR("DeathMale");
        case U3SoundEffectDownwards: return CFSTR("Downwards");
        case U3SoundEffectError1: return CFSTR("Error1");
        case U3SoundEffectError2: return CFSTR("Error2");
        case U3SoundEffectFailedSpell: return CFSTR("FailedSpell");
        case U3SoundEffectForceField: return CFSTR("ForceField");
        case U3SoundEffectHeal: return CFSTR("Heal");
        case U3SoundEffectHit: return CFSTR("Hit");
        case U3SoundEffectHorseWalk: return CFSTR("HorseWalk");
        case U3SoundEffectImmolate: return CFSTR("Immolate");
        case U3SoundEffectInvocation: return CFSTR("Invocation");
        case U3SoundEffectLBLevelRise: return CFSTR("LBLevelRise");
        case U3SoundEffectLevelUp: return CFSTR("ExpLevelUp");
        case U3SoundEffectMiscSpell: return CFSTR("MiscSpell");
        case U3SoundEffectMonsterSpell: return CFSTR("MonsterSpell");
        case U3SoundEffectMoongate: return CFSTR("Moongate");
        case U3SoundEffectMountHorse: return CFSTR("MountHorse");
        case U3SoundEffectOuch: return CFSTR("Ouch");
        case U3SoundEffectShoot: return CFSTR("Shoot");
        case U3SoundEffectShrine: return CFSTR("Shrine");
        case U3SoundEffectSink: return CFSTR("Sink");
        case U3SoundEffectStep: return CFSTR("Step");
        case U3SoundEffectSwish1: return CFSTR("Swish1");
        case U3SoundEffectSwish2: return CFSTR("Swish2");
        case U3SoundEffectSwish3: return CFSTR("Swish3");
        case U3SoundEffectSwish4: return CFSTR("Swish4");
        case U3SoundEffectTorchIgnite: return CFSTR("TorchIgnite");
        case U3SoundEffectUpwards: return CFSTR("Upwards");
    }
    return NULL;
}

void U3AudioApplyPreferences(void) {
    ApplyVolumePreferences();
}

void U3AudioOpenEffects(void) {
    OpenChannel();
}

void U3AudioCloseEffects(void) {
    CloseChannel();
}

void U3AudioSetUpMusic(void) {
    SetUpMusic();
}

void U3AudioCloseMusic(void) {
    CloseMusic();
}

void U3AudioSetUpSpeech(void) {
    SetUpSpeech();
}

void U3AudioCloseSpeech(void) {
    CloseSpeech();
}

void U3AudioPlaySound(U3SoundEffect effect, bool forceAsync) {
    CFStringRef soundName = U3LegacySoundNameForEffect(effect);
    if (soundName)
        PlaySoundFile(soundName, forceAsync);
}

void U3AudioStartMusic(int16_t songID) {
    gSongCurrent = gSongNext = songID;
    MusicUpdate();
}

void U3AudioStopMusic(void) {
    EndSong();
}

void U3AudioUpdateMusic(void) {
    MusicUpdate();
}

void U3AudioSetSoundVolumePercent(int16_t volumePercent) {
    SetSoundVolumePercent(volumePercent);
}

void U3AudioSetMusicVolumePercent(int16_t volumePercent) {
    (void)volumePercent;
    ApplyVolumePreferences();
}

void U3AudioSpeakText(const char *text, int16_t voiceID) {
    (void)text;
    (void)voiceID;
}

void U3AudioSpeakMessages(int16_t messageID, int16_t additionalMessageID, int16_t voiceID) {
    SpeakMessages(messageID, additionalMessageID, voiceID);
}

void U3AudioSpeakPascalString(uint8_t *pascalString, int16_t voiceID) {
    Speech(pascalString, voiceID);
}

void U3AudioPrimeLegacySample(const uint8_t *sampleData) {
    (void)sampleData;
}

void U3AudioPlayLegacyFadeTone(int32_t pass) {
    (void)pass;
    gCurChan++;
    if (gCurChan > gMaxChan)
        gCurChan = 1;
}
