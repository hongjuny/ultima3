//
//  LWSoundPlayer.m
//  Ultima3
//
//  Created by Leon McNeill on 12/2/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "LWSoundPlayer.h"


@implementation LWSoundPlayer

- (void) setDisposeAfterPlaying:(BOOL)value
{
	mDisposeAfterPlaying = value;
}

+ (LWSoundPlayer *) play:(NSSound *)aSound disposeAfterPlaying:(BOOL)dispose
{
	LWSoundPlayer *obj = [[LWSoundPlayer alloc] init];
	[obj setDisposeAfterPlaying:dispose];
	[NSThread detachNewThreadSelector:@selector(play:) toTarget:obj withObject:aSound];
	return obj;
}

- (id) init
{
	if ((self = [super init]) != nil)
		{
		mIsPlaying = YES;
		mDisposeAfterPlaying = NO;
		}
	return self;
}

- (void) play:(NSSound *)aSound
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[aSound setDelegate:self];
	[aSound play];
	CFRunLoopRun();
	[pool release];
}

- (void)sound:(NSSound *)sound didFinishPlaying:(BOOL)aBool
{
	if (aBool)
		{
		mIsPlaying = NO;
		if (mDisposeAfterPlaying)
			[self release];
		}
}

- (BOOL) isPlaying { return mIsPlaying; }

@end
