#import <Cocoa/Cocoa.h>
#import <stdlib.h>

#import "CocoaBridge.h"
#import "UltimaMain.h"

int main(int argc, char *argv[]) {
    int result = Ultima3_main();
    if (!getenv("U3_SKIP_APP_RUN"))
        U3CocoaRunApplication();
    return result;
}
