//
//  LWSoundPlayer.h
//  Ultima3
//
//  Created by Leon McNeill on 12/2/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface LWSoundPlayer : NSObject
{
	BOOL mIsPlaying;
	BOOL mDisposeAfterPlaying;
}

+ (LWSoundPlayer *) play:(NSSound *)aSound disposeAfterPlaying:(BOOL)dispose;
- (BOOL) isPlaying;
@end
