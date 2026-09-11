//
//  U3Audio.h
//  Ultima3
//
//  Boundary for sound effects, music, and speech.
//

#ifndef U3Audio_h
#define U3Audio_h

#include "U3Types.h"

typedef enum U3SoundEffect {
    U3SoundEffectAlarm = 0,
    U3SoundEffectAttack,
    U3SoundEffectBigDeath,
    U3SoundEffectBump,
    U3SoundEffectCombatStart,
    U3SoundEffectCombatVictory,
    U3SoundEffectCreak,
    U3SoundEffectDeathFemale,
    U3SoundEffectDeathMale,
    U3SoundEffectDownwards,
    U3SoundEffectError1,
    U3SoundEffectError2,
    U3SoundEffectFailedSpell,
    U3SoundEffectForceField,
    U3SoundEffectHeal,
    U3SoundEffectHit,
    U3SoundEffectHorseWalk,
    U3SoundEffectImmolate,
    U3SoundEffectInvocation,
    U3SoundEffectLBLevelRise,
    U3SoundEffectLevelUp,
    U3SoundEffectMiscSpell,
    U3SoundEffectMonsterSpell,
    U3SoundEffectMoongate,
    U3SoundEffectMountHorse,
    U3SoundEffectOuch,
    U3SoundEffectShoot,
    U3SoundEffectShrine,
    U3SoundEffectSink,
    U3SoundEffectStep,
    U3SoundEffectSwish1,
    U3SoundEffectSwish2,
    U3SoundEffectSwish3,
    U3SoundEffectSwish4,
    U3SoundEffectTorchIgnite,
    U3SoundEffectUpwards
} U3SoundEffect;

void U3AudioApplyPreferences(void);
void U3AudioOpenEffects(void);
void U3AudioCloseEffects(void);
void U3AudioSetUpMusic(void);
void U3AudioCloseMusic(void);
void U3AudioSetUpSpeech(void);
void U3AudioCloseSpeech(void);
void U3AudioPlaySound(U3SoundEffect effect, bool forceAsync);
void U3AudioStartMusic(int16_t songID);
void U3AudioStopMusic(void);
void U3AudioUpdateMusic(void);
void U3AudioSetSoundVolumePercent(int16_t volumePercent);
void U3AudioSetMusicVolumePercent(int16_t volumePercent);
void U3AudioSpeakText(const char *text, int16_t voiceID);
void U3AudioSpeakMessages(int16_t messageID, int16_t additionalMessageID, int16_t voiceID);
void U3AudioSpeakPascalString(uint8_t *pascalString, int16_t voiceID);
void U3AudioPrimeLegacySample(const uint8_t *sampleData);
void U3AudioPlayLegacyFadeTone(int32_t pass);
void U3AudioStopLegacyFadeTone(void);
bool U3AudioMusicSelfTest(void);

#endif /* U3Audio_h */
