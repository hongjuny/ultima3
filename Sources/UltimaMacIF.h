//
//  UltimaMacIF.h
//  Ultima3
//

#ifndef UltimaMacIF_h
#define UltimaMacIF_h

void ReflectPrefs(void);
SInt16 DoStandardAlert(AlertType type, short id);
void SetCurWindowPrefPosn(short x, short y);
void SetSaveWindowPrefPosn(short x, short y);
void MyInvertRect(Rect* rect);
void MyInvertFrame(Rect* rect);
void HandleStatsClick(Point mouse);
pascal Boolean DialogFilter(DialogPtr theDlg, EventRecord *event, short *itemHit);
void SetUpDisplayDialog(void);
void SetUpDisplay(void);
void AdaptToWindow(Boolean forceOnScreen);
void ForceOnScreen(WindowPtr win);
void GetGlobalWindowRect(WindowPtr win, Rect *rect);
short GetWindowDevice(WindowPtr win);
void GetWindowDeviceRect(WindowPtr win, Rect *gr);
Boolean SetUpHelpWorld(void);
void DestroyHelpWorld(void);
void DrawRaceHelp(short race, short x, short y);
void DrawClassHelp(short clss, short x, short y);
void DoStats(short chnum);
void DrawFancyRecord(Boolean centered);
void RenderCharStats(short ch, const Rect* rect);
void ShowHideBackground(void);
Boolean ClearShroud(void);
Boolean ShouldSuppressMenuBarHiding(void);
void ShowMenuBarIfNecessary(void);
void MyHideMenuBar(void);
void MyShowMenuBar(void);
void ShowHideReference(void);
void SetUpFont(void);
void HandleMouseDown(void);
void HandleError(OSErr error, long desc, long idnum);
void ImageDisplay(short which, Boolean hidePause);
void ImageGoAway(void);
void DisableSound(void);
void DisableMusic(void);
void InitMacro(void);
void AddMacro(char whut);
void DecMacro(void);
short WidgetClick(Rect cRect, Boolean click, Boolean drag);
short MaxMana(char rosNum);
void SetUpDragRect(void);
void HandleMenuChoice(long menuChoice);
Boolean QuitDialog(void);
void ToolBoxInit(void);
void OpenGraphicsAndSound(void);
void MenuBarInit(void);
void WindowInit(short which);
void CheckSystemRequirements(void);
void Hibernate(void);
void WakeUp(void);
void SetUpGWorlds(void);
void TearDownGWorlds(void);
void DisableMenus(void);
void EnableMenus(void);
void ResetCursor(void);
void CursorUpdate(void);
short RosterSelect(void);
Boolean CharacterCreateDialog(short rosNum);

#endif /* UltimaMacIF_h */
