//
//  UltimaSound.h
//  Ultima3
//

#ifndef UltimaSound_h
#define UltimaSound_h

void ApplyVolumePreferences(void);
void PlaySoundFile(CFStringRef soundName, Boolean forceAsync);
void OpenChannel(void);
void CloseChannel(void);
void SetUpSpeech(void);
void DisableSpeech(void);
void SetUpVoiceList(void);
void GetVoice(Str255 voicename);
void CloseSpeech(void);
void Speech(Str255 string, short shnum);
void SpeakMessages(int msg1, int msg2, int voiceNum);
void SetUpMusic(void);
void CloseMusic(void);
void SetMusicPortAndDevice(CGrafPtr thePort, GDHandle theDevice);
void MusicUpdate(void);
void EndSong(void);
void ErrorTone(void);

#endif /* UltimaSound_h */
