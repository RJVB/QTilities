/*!
  @file NSQTMovieWindow.h
  QTMovieWindow

  Created by René J.V. Bertin on 20110224
  Copyright 2011 RJVB. All rights reserved.
 */

#import <Cocoa/Cocoa.h>
#import <QTKit/QTKit.h>
#import "NSQTWindow.h"
#import "NSQTMovieView.h"
#ifndef _QTKITDEFINES_H
#	define _QTKITDEFINES_H
#endif

@class QTMovieView;

@interface NSQTMovieWindow : NSDocument
{
	IBOutlet NSQTMovieView *movieView;
	NSURL *URL;
	QTMovie *qtMovie;
	NSSize movieSize;
	BOOL resizesVertically;
	struct QTMovieWindows **qtmwH;
}

- (void) close;

- (BOOL)readFromURL:(NSURL *)url ofType:(NSString *)type;
- (BOOL)setQTMovieFromMovie:(Movie)theMovie;
- (id)initWithContentsOfURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError;
- (id) initWithQTWMandPool:(struct QTMovieWindows**)wih;
- (NSSize)windowSizeForMovieSize:(NSSize)s;
- (NSSize) windowContentSizeForMovieSize:(NSSize)s;
- (NSRect)windowRectForMovieRect:(NSRect)r;
- (void) resizeWindowOnSpotWithFrameRect:(NSRect)aRect;
- (NSSize)moviePreferredSize;
- (NSSize)movieCurrentSize;
- (void)setMovieSize:(NSSize)size;
- (void)setMovieSize:(NSSize)size caller:(const char*)caller;
- (void) ToggleMCController;
- (void) setOneToOne:sender;
- (void) setQTMovie:(QTMovie*)m;
- (QTMovie*) theQTMovie;
- (QTMovieView*) theMovieView;
- (NSWindow*) theWindow;
- (struct QTMovieWindows**) theQTMovieWindowH;
- (void) setTitleWithCString:(const char*)title;
- (void) boundsDidChange:(NSQTMovieWindow*)sender;
- (void) prepareForRelease;
- (NSString*) description;
// delegate functions:
- (BOOL) windowShouldClose:(id)sender;
- (void) windowWillClose:(NSNotification*)notification;

// added automatically by "Convert to Objective-C 2.0"
@property (retain,getter=theMovieView) QTMovieView *movieView;
@property (retain) NSURL *URL;
@property (retain,getter=theQTMovie) QTMovie *qtMovie;
@property (getter=theQTMovieWindowH) struct QTMovieWindows **qtmwH;
@property BOOL resizesVertically;
@end

// delegate for our NSWindow so we can handle a close event properly
@interface QTMovieWindowDelegate : NSObject
- (BOOL) windowShouldClose:(id)sender;
- (void) windowWillClose:(NSNotification*)notification;
@end

@interface NSWindow (description)
- (NSString*) description;
@end