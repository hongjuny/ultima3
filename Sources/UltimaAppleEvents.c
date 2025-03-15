// Ultima III Apple Event Handlers

#import "UltimaAppleEvents.h"

#import "PrefsDialog.h"
#import "UltimaMacIF.h"
#import "UltimaMisc.h"

extern Boolean gDone;

static pascal OSErr AEOpenApp(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon);
static pascal OSErr AEOpenDocs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon);
static pascal OSErr AEPrintDocs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon);
static pascal OSErr AEQuit(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon);
static pascal OSErr AEPrefs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon);

Boolean InitializeAEHandlers(void) {
    OSErr error;
    AEEventHandlerUPP aehpp;

    aehpp = NewAEEventHandlerUPP(AEOpenApp);
    error = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, aehpp, 0, false);
    if (error)
        return false;

    aehpp = NewAEEventHandlerUPP(AEOpenDocs);
    error = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, aehpp, 0, false);
    if (error)
        return false;

    aehpp = NewAEEventHandlerUPP(AEPrintDocs);
    error = AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, aehpp, 0, false);
    if (error)
        return false;

    aehpp = NewAEEventHandlerUPP(AEQuit);
    error = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, aehpp, 0, false);
    if (error)
        return false;

    aehpp = NewAEEventHandlerUPP(AEPrefs);
    error = AEInstallEventHandler(kCoreEventClass, kAEShowPreferences, aehpp, 0, false);
    if (error)
        return false;

    return true;
}

static pascal OSErr AEOpenApp(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon) {
    return noErr;
}

static pascal OSErr AEOpenDocs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon) {
    return noErr;
}

static pascal OSErr AEPrintDocs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon) {
    return noErr;
}

static pascal OSErr AEQuit(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon) {
    if (QuitDialog())
        gDone = TRUE;
    return noErr;
}

static pascal OSErr AEPrefs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon) {
    GameOptionsDialog();
    BlockExodus();
    return noErr;
}
