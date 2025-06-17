//
//  UltimaGraphics.h
//  Ultima3
//

#ifndef UltimaGraphics_h
#define UltimaGraphics_h

void DrawFramePiece(short which, short x, short y);
void DrawTiles(void);
void PlotSig(short x,short y);
void WideAreaUpdate(void);
void SaveWideArea(void);
void TextScrollAreaUpdate(void);
void DrawIntro(unsigned char shape, short offset);
void CheckInterrupted(void);
void DrawIntroPix(void);
void DrawExodusPict(void);
void DrawMenu(void);
void DrawDemoScreen(void);
void DngPiece(unsigned char shape);
void SetupTileToGame(void);
Boolean SetupTileToDisplay(void);
void SetupOffset(void);
void RectCopy (Rect tSourceRect, Rect tDestRect, Boolean hideCurs);
void HideMonsters(void);
void ShowMonsters(void);
void GetFont(void);
void GetGraphics(void);
Rect GetTileRectForIndex(short index);
void ClearBottom(void);
void DrawFrame(short which);
void FadeOnExodusUltima(void);
void WriteLordBritish(void);
void FightScene(void);
void DemoUpdate(short ptr);
char CursorKey(Boolean usePenLoc);
void ClearScreen(void);
void CreateIntroData(void);
void DisposeIntroData(void);
void GetDungeonGraphics(void);
void GetPortraits(void);
Boolean DrawNamedImage(CFStringRef imageName, CGrafPtr destWorld, const Rect *destRect);
void DrawMap(unsigned char x, unsigned char y);
void DrawMiniMap(void);
void ClearTiles(void);
void DrawPortrait(char charNum, CGrafPtr destWorld);
void ExodusLights(void);
void ScrollShape(short shape, short amount);
void SwapShape(unsigned short shape);
void HandleUpdate(void);
void DrawFramePieceScroll(short which, short x, short y);
void SetUpMouseRect(void);
void DrawMasked(unsigned short shape, unsigned short x, unsigned short y);
void DrawDemo(void);
void SaveScrollArea(void);
void Stalagtites(void);
void DrawMiniDng(unsigned char mode);
Boolean SetupGameToDisplay(void);
Boolean SetupFrameToDisplay(void);
void DrawGamePortToMain(Boolean);

#ifdef __APPLE__
#ifdef __OBJC__
@class MTKView;
#else
typedef void MTKView;
#endif
void U3InitMetalRenderer(MTKView *view);
void U3MetalRenderFrame(void);
#endif

#endif /* UltimaGraphics_h */
