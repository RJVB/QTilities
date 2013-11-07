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

/*!
	the main document class associated with the window NIB resource
 */
@interface NSQTMovieWindow : NSDocument
{
	IBOutlet NSQTMovieView *movieView;
	NSURL *URL;
	QTMovie *qtMovie;
	NSSize movieSize;
	BOOL resizesVertically,			//!< some windows, containing a movie with height<=2 only resize horizontally
		nibLoaded;
	struct QTMovieWindows **qtmwH;
}

- (id) init;
- (void) close;

- (BOOL)readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError;
- (BOOL)readFromURL:(NSURL *)url ofType:(NSString *)type;
- (BOOL)readFromFile:(NSString *)fileName ofType:(NSString *)docType;

- (BOOL)setQTMovieFromMovie:(Movie)theMovie;
- (id)initWithContentsOfURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError;
- (id)initWithContentsOfURL:(NSURL *)absoluteURL ofType:(NSString *)typeName;
- (id)internalInitWithContentsOfURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError;
- (id) initWithQTWM:(struct QTMovieWindows**)wih;
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

@interface QTilsApplicationDelegate : NSObject<NSApplicationDelegate> {
	BOOL relaunch;				//!< state variable used by applicationDidFinishLaunching to know whether or not
							//!< the main function (or argv[0]) must be relaunched with the new argc,argv.
							//!< setter: setRelaunch
	NSMutableArray *ArgArray;	//!< array receiving the file arguments passed to the executable by Apple's Launch Services
							//!< set by the delegate application:openFile methods
}
- (id) init;
- (void) dealloc;
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
- (void) application:(NSApplication *)theApplication openFiles:(NSArray *)filenames;
- (BOOL) application:(NSApplication *)theApplication openFileWithoutUI:(NSString *)filename;
- (void) applicationDidFinishLaunching:(NSNotification *)aNotification;
- (BOOL) isRestarted;
- (BOOL) setRestarted:(BOOL)state;
- (BOOL) isLaunched;
- (BOOL) setLaunched:(BOOL)state;
@property BOOL relaunch;
@property (retain) NSMutableArray *ArgArray;
@end

@interface QTilsMenuDelegate : NSObject<NSMenuDelegate> {
}
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
- (void) application:(NSApplication *)theApplication openFiles:(NSArray *)filenames;
@end

@interface NSWindow (description)
- (NSString*) description;
- (void) showInfo:(id)sender;		//!< receiver for the Get Info menu item.
@end