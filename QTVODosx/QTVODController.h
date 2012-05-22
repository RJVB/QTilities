//
//  QTVODController.h
//  QTVODosx
//
//  Created by René J.V. Bertin on 20110920.
//  Copyright 2011 INRETS/LCPC — LEPSIS. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface QTVODController : NSDocumentController {
	@public
	BOOL addToRecentDocs;
}
- (void) noteNewRecentDocument:(NSDocument*)theDoc;
- (void) noteNewRecentDocumentURL:(NSURL*)theURL;
- (id) openDocumentWithURLContents:(NSURL*)URL error:(NSError**)outerror addToRecent:(BOOL)add;

@property BOOL addToRecentDocs;
@end
