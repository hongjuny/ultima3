//
//  UltimaSpellCombat.h
//  Ultima3
//

#ifndef UltimaSpellCombat_h
#define UltimaSpellCombat_h

Boolean Cast(short mode, short chnum);
void ProcessMagic(short chnum);
short EitherChoose(void);
short ClericChoose(void);
short WizardChoose(void);
void Incap(void);
void Spell(short chnum);
void Projectile(short chnum, short damage);
void BigDeath(short damage, short chnum);
void Necorp(void);
void Heal(short damage);
void Failed(void);
void DownLevel(void);
void UpLevel(void);
unsigned char CombatValidMove(short value);
unsigned char ValidMove(short value);
void GetScreen(short resid);
unsigned char ExodusCastle(void);
unsigned char CombatMonsterHere(short x, short y);
short Shoot(short x, short y);
void DamageMonster(short which, short damage, short chnum);
unsigned char GetXYDng(short x, short y);
void PutXYDng(unsigned char value, short x, short y);
short BackGround(short montype);
unsigned char HowMany(void);
void HandleMove(short chnum);
void Victory(void);
void Combat(void);
unsigned char DetermineShape(short type);
void CombatAttack(short chnum);
void Missed(void);
short FigureNewMonPosition(short mon);
void Poison(short chnum);
void ShowHit(short x, short y, unsigned char hitType, unsigned char tileUnder);

#endif /* UltimaSpellCombat_h */
