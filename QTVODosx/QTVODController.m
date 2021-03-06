//
//  QTVODController.m
//  QTVODosx
//
//  Created by René J.V. Bertin on 20110920.
//  Copyright 2011 INRETS/LCPC — LEPSIS. All rights reserved.
//

#import "QTVODController.h"


@implementation QTVODController

- (id) init
{
	// 20130618:
	self = [super init];
	return self;
}

- (void) dealloc
{
	[super dealloc];
}

- (void) noteNewRecentDocument:(NSDocument*)theDoc
{
	if( addToRecentDocs ){
		[super noteNewRecentDocument:theDoc];
	}
}

- (void) noteNewRecentDocumentURL:(NSURL*)theURL
{
	if( addToRecentDocs ){
		[super noteNewRecentDocumentURL:theURL];
	}
}

- (id) openDocumentWithURLContents:(NSURL*)URL error:(NSError**)outerror addToRecent:(BOOL)add
{
	addToRecentDocs = add;
	NSLog( @"[%@ %@] URL=%@",
		 NSStringFromClass([self class]), NSStringFromSelector(_cmd), URL );
	return [super openDocumentWithContentsOfURL:URL display:YES error:outerror];
}

@synthesize addToRecentDocs;
@end
