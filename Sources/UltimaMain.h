//
//  UltimaMain.h
//  Ultima3
//

#ifndef UltimaMain_h
#define UltimaMain_h

int Ultima3_main(void);

void DrawOrganizeMenu(void);
void AttackCode(short whichMon);
void BombTrap(void);
void GetChest(short spell, short chnum);
void HandEquip(short chnum1, short chnum2, short item, char what2, short value);
void Ignite(void);
void JoinGold(short chnum);
void ModifyOrder(void);
void NegateTime(short mode, short chnum);
Boolean QuitSave(short mode);
void ReadyWeapon(short chnum, char weapon);
void CheckAllDead(void);
Boolean CheckAlive(short member);
void ShowChars(Boolean force);
void Volume(void);
void WearArmour(short chnum, char armour);
void Yell(short mode);
void Stats(short mode, short chnum);
void What2(void);
void NotHere(void);
void NoGo(void);
void GoWhirlPool(void);
void InverseTiles(void);
void ShowWind(void);
void ScrollThings(void);
void AnimateTiles(void);
void TwiddleFlags(void);
void InverseChnum(char which);
void UnInverseChnum(char which);

void HPAdd(short member, short amount);
Boolean HPSubtract(short rosNum, short amount);

// 0 = return true for keypresses only
// 1 = also return true for mousedowns
// 2 = as 1, but no speed constraints.
Boolean GetKeyMouse(unsigned char mode);
short WaitKeyMouse(void);

void IdleUntil(long endTime);
void DrawMapPause(void);
void InverseChar(char which);
unsigned short RandNum(unsigned short lowrnd, unsigned short highrnd);

#endif /* UltimaMain_h */
