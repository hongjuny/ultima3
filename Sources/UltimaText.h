//
//  UltimaText.h
//  Ultima3
//

#ifndef UltimaText_h
#define UltimaText_h

void UPrint(Str255 gString, char x, char y);
void UPrint2(Str255 gString, float y);
void UPrintNum(short number, short x, short y);
void UPrintNumPad(short number, short pad);
void UPrintChar(char ch, short x, short y);
void UPrintMessage(short msgNum);
void UPrintMessageRewrapped(short msgNum);
void UPrintWin(Str255 gString);
void UPrintWinEncode(Str255 gString);

void PrintSpell(short spell);
void PrintWeapon(short weapon);
void PrintWeaponList(short weapon);
void PrintArmour(short armour);
void PrintArmourList(short armour);
void PrintTile(short tile, Boolean plural);
void PrintMonster(short which, Boolean plural, char variant);

short UInputNum(short x, short y);
long UInputBigNum(short x, short y);
void UInputText(short x, short y, Str255 dest, short maxChar, Boolean numOnly);

short GetChar(void);
short GetKey(void);

void UIncTx(void);
void UIncTy(void);
void CenterMessage(short which, short y);
void Message(short which, short x, short y);
short PixelsWideString(Str255 gString);
void SetNewFont(Boolean force);
void UCenterAt(Str255 gString, short x, short y);
void NewPrint(Str255 gString, short x, short y);
unsigned char StringLocation(Str255 source, Str255 search);
void RewrapString(Str255 str, Boolean withCursor);
void ShowClickMessage(void);
void ClearUpdatePort(void);
void UTextScroll(void);
void WinText(short grey);
void AddString(Str255 str1, Str255 str2);
void SearchReplace(Str255 source, Str255 search, Str255 replace);
void DrawPrompt(void);
void Speak(short perNum, short shnum);

OSStatus UDrawThemePascalString(ConstStr255Param inPString, ThemeFontID inFontID);
short UThemePascalStringWidth(ConstStr255Param inPString, ThemeFontID inFontID);

#endif /* UltimaText_h */
