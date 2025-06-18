#include "numtostring.h"
#define NIL_PTR         0L
#define BASERES         400
#define BASECURSORID    500
#define GAMESCREENID    BASERES
#define FRAMETILE       BASERES+1
#define DEPTH           8
#define TILEON          32
#define XMON            64
#define YMON            96
#define HPMON           128
#define VARMON          160
#define APPLEMENU       128
#define ABOUTID         1
#define FILEMENU        129
#define  SAVEID         1
#define  PAUSEID        2
#define  ABORTID        4
#define  QUITID         5
#define EDITMENU        130
#define SPECIALMENU     131
#define  SOUNDID        1
#define  MUSICID        2
#define  SPEECHID       3
#define  CONSTRAINID    4
#define  AUTOCOMBATID   5
#define  DOUBLESIZEID   7
#define  FULLSCREENID   8
#define REFERENCEMENU   132
#define  COMMANDLIST    1
#define  SPELLLIST      2

#define DRAGTHRESH      16

// New propertylist-based prefs keys.  Mostly inverse meanings to 1.x prefs so that nil is default.
extern CFStringRef U3PrefCheckedResourcesDate;
extern CFStringRef U3PrefSoundInactive;
extern CFStringRef U3PrefMusicInactive;
extern CFStringRef U3PrefSoundVolume;
extern CFStringRef U3PrefMusicVolume;
extern CFStringRef U3PrefSpeechInactive;
extern CFStringRef U3PrefSpeedUnconstrain;
extern CFStringRef U3PrefOriginalSize;
extern CFStringRef U3PrefFullScreen;
extern CFStringRef U3PrefDontAskDisplayMode;
extern CFStringRef U3PrefNoEducateAboutFullScreen;
extern CFStringRef U3PrefFullScreenResChange;
extern CFStringRef U3PrefClassicAppearance;
extern CFStringRef U3PrefIncludeWind;
extern CFStringRef U3PrefNoDiagonals;
extern CFStringRef U3PrefAutoSave;
extern CFStringRef U3PrefManualCombat;
extern CFStringRef U3PrefNoAutoHeal;
extern CFStringRef U3PrefAsyncSound;
extern CFStringRef U3PrefHealThreshold;
extern CFStringRef U3PrefCurWindowX;
extern CFStringRef U3PrefCurWindowY;
extern CFStringRef U3PrefSaveWindowX;
extern CFStringRef U3PrefSaveWindowY;
extern CFStringRef U3PrefGameFont;
extern CFStringRef U3PrefTileSet;
extern CFStringRef U3PrefUserName;
extern CFStringRef U3PrefRegCode;
extern CFStringRef U3PrefLatestReleaseNumber;
extern CFStringRef U3PrefLatestReleaseNote;
extern CFStringRef U3PrefInformedNewVersionDate;
extern CFStringRef U3PrefInformDayInterval;

enum {
  initCmd                       = 1,
  freqDurationCmd               = 40
};
