/*
 *  UltimaNew.h
 *  Ultima3
 *
 */

#ifndef UltimaNew_h
#define UltimaNew_h

void ValidatePrefs(void);
Boolean TerminateCharacterDialog(void);
Boolean FormPartyDialog(void);
void GetButtons(void);
void DisposeButtons(void);
void DrawButton(short butNum, Boolean pushed, Boolean dim);
void SetButtonRect(Rect* rect, short butNum);
Boolean HandleButtonClick(Point point, short butNum);
void ConfigureFilter(short setOK, short setCancel);
void DoAutoHeal(void);
void RestoreDisplay(void);

#endif /* UltimaNew_h */
