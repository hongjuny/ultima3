#import <Cocoa/Cocoa.h>
#import <stdlib.h>
#include <unistd.h>

#import "CocoaBridge.h"
#import "UltimaMain.h"
#import "U3Audio.h"

extern Boolean U3LegacyBitmapSelfTest(void);
extern bool U3IOSelfTest(void);
extern Boolean U3PartySelectionSelfTest(void);
extern Boolean U3CharacterCreationSelfTest(void);

int main(int argc, char *argv[]) {
    if (getenv("U3_BOOT_CHECK") || getenv("U3_WORLD_RENDER_CHECK") ||
        getenv("U3_WORLD_INPUT_CHECK") || getenv("U3_WORLD_MOUSE_CHECK")) {
        if (!getenv("U3_SAVE_DIRECTORY")) {
            fprintf(stderr, "Boot check requires U3_SAVE_DIRECTORY for isolated storage.\n");
            return 2;
        }
        alarm(90);
    }
    if (getenv("U3_IO_SELF_TEST")) {
        @autoreleasepool {
            bool passed = U3IOSelfTest();
            fprintf(stderr, "Save container test: %s\n", passed ? "passed" : "FAILED");
            return passed ? 0 : 1;
        }
    }
    if (getenv("U3_AUDIO_SELF_TEST")) {
        @autoreleasepool {
            bool passed = U3AudioMusicSelfTest();
            fprintf(stderr, "Music playback test: %s\n", passed ? "passed" : "FAILED");
            return passed ? 0 : 1;
        }
    }
    if (getenv("U3_RENDER_SELF_TEST")) {
        @autoreleasepool {
            Boolean passed = U3LegacyBitmapSelfTest();
            fprintf(stderr, "Legacy bitmap test: %s\n", passed ? "passed" : "FAILED");
            Boolean partyPassed = U3PartySelectionSelfTest();
            fprintf(stderr, "Party selection test: %s\n", partyPassed ? "passed" : "FAILED");
            passed = partyPassed && passed;
            Boolean characterPassed = U3CharacterCreationSelfTest();
            fprintf(stderr, "Character creation test: %s\n", characterPassed ? "passed" : "FAILED");
            passed = characterPassed && passed;
            Boolean imagesPassed = U3CocoaImageSelfTest();
            fprintf(stderr, "Image decoding test: %s\n", imagesPassed ? "passed" : "FAILED");
            passed = passed && imagesPassed;
            return passed ? 0 : 1;
        }
    }
    @autoreleasepool {
        int result = Ultima3_main();
        if (!getenv("U3_SKIP_APP_RUN") && !getenv("U3_STARTUP_RENDER_CHECK") &&
            !getenv("U3_BOOT_CHECK") && !getenv("U3_WORLD_RENDER_CHECK") &&
            !getenv("U3_WORLD_INPUT_CHECK") && !getenv("U3_WORLD_MOUSE_CHECK"))
            U3CocoaRunApplication();
        return result;
    }
}
