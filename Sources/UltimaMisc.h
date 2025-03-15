//
//  UltimaMisc.h
//  Ultima3
//

#ifndef UltimaMisc_h
#define UltimaMisc_h

void GetDirection(short mode);

void GetMiscStuff(short id);
void PutMiscStuff(void);

unsigned char ValidMonsterDir(short tile, short montype);
void ShowMonsterList(void);
short MonsterHere(short x, short y);
char GetHeading(short value);
short Absolute(short value);
short MapConstrain(short value);
void LoadUltimaMap(short resid);
void BlockExodus(void);
void SpawnMonster(void);
void SpawnUntilFull(void);
void MoveMonsters(void);
void AgeChars(void);
void DrawMoonGateStuff(void);
void AddExp(short chnum, short amount);
void SafeExodus(void);
void NoEffect(void);
void GetParty(void);
void PutParty(void);
void GetRoster(void);
void PutRoster(void);
void OpenRstr(void);
void GetSosaria(void);
void PutSosaria(void);
void PushSosaria(void);
void PullSosaria(void);
void ResetSosaria(void);
void GetDemoRsrc(void);
void OtherCommand(short yell);
Boolean StealDisarmFail(short rosNum);
void AddItem(short rosNum, short item, short amount);
void Shop(short shopNum, short chnum);
void Routine6E35(void); // lovely that I never picked a name for these
void Routine6E6B(void);
void Shrine(short chnum);
Boolean AddGold(short rosNum, short gold, Boolean overflow);

short GetXY(short x, short y);
void PutXY(short a, short x, short y);

unsigned char GetXYVal(int x, int y);
void PutXYVal(unsigned char value, unsigned char x, unsigned char y);

unsigned char GetXYTile(short x, short y);
void PutXYTile(short value, short x, short y);

#endif /* UltimaMisc_h */
