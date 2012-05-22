//
//  QTAMovieView.m
//  QTAmateur -> QTVOD
//
//  Created by René J.V. Bertin on 20070404.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "QTAMovieView.h"


@implementation QTAMovieView

//- (QTMovieView*) theParent
//{
//	return super;
//}

- (IBAction) play:(id) sender
{
//	NSLog( @"%@: (Re)Starting movie %@", self, sender );
	return [super play:sender];
}

- (IBAction) pplay:(id) sender
{
//	NSLog( @"(Re)Starting movie" );
	return [self play:sender];
}

@end
