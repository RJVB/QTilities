//
//  QTVODWindow.h
//  QTAmateur -> QTVOD
//
//  Created by Michael Ash on 5/22/05.
//  Copyright __MyCompanyName__ 2005 . All rights reserved.
//

#ifndef _QTVODWINDOW_H

#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import "QTAMovieView.h"
#import "NSATextField.h"
#ifndef _QTKITDEFINES_H
#	define _QTKITDEFINES_H
#endif
#import "../QTils/QTilities.h"

#define SLOG(format,...)	NSLog(@"SLOG: File=%s line=%d proc=%s " format, strrchr("/" __FILE__,'/')+1, __LINE__, __PRETTY_FUNCTION__, ## __VA_ARGS__)

typedef enum {FSRegular, FSSpanning, FSMosaic } FSStyle;

typedef enum QTVODwinIDs {
	fwWin = 0,
	pilotWin = 1,
	leftWin = 2,
	rightWin = 3,
	tcWin = 4,
	sysWin = 5, maxQTWM } QTVODwinIDs;

// no reason to use QTAMovieView ...
#define QTAMovieView	QTMovieView
#define pplay			play

//@class QTMovieView;
@class QTAMovieView;
@class ExportManager;

@interface QTVODWindow : NSDocument {
	IBOutlet QTAMovieView *movieView;

	QTMovie *movie;
	NSSize movieSize;
#ifdef SHADOW
	IBOutlet QTAMovieView *movieShadowView;
	QTMovie *movieShadow;
#endif

	BOOL resizesVertically;

	int halfSize, normalSize, doubleSize, maxSize,
		playAllFrames, Loop;

	IBOutlet NSWindow *fullscreenWindow;
	IBOutlet QTAMovieView *fullscreenMovieView;

	IBOutlet NSDrawer		*mDrawer;
	IBOutlet NSTextField	*mCurrentSize;
	IBOutlet NSTextField	*mDuration;
	IBOutlet NSTextField	*mNormalSize;
	IBOutlet NSTextField	*mSourceName;
	IBOutlet NSATextField	*mTimeDisplay;
	IBOutlet NSATextField	*mRateDisplay;

	IBOutlet NSView *exportAccessoryView;
	IBOutlet NSPopUpButton *exportTypePopup;
	IBOutlet NSTextField *exportInfoField;
	ExportManager *exportManager;

	NSTimer *dontSleepTimer;

	ComponentResult callbackResult;

//	struct QTVOD *theQTVOD;
	id theQTVOD;

@public
	NSURL *theURL;
	BOOL inFullscreen, delayedClosing, shouldBeClosed;
	int Playing;
	short wasStepped, wasScanned, handlingMCAction, isProgrammatic, shouldBeInvisible, addToRecentMenu;
	QTVODwinIDs vodView;
	NSDrawerState InfoDrawer;
	QTMovieWindowH qtmwH;
	unsigned long ACSCcount;
#ifdef DEBUG
	QTTime prevActionTime;
#endif
}

- (NSString*) description;
- (void)updateMenus;

- (NSString*) thePath;

- (QTMovie*)getMovie;
- (QTMovie*)theQTMovie;
- (QTAMovieView*)getView;
- (QTMovieView*)theMovieView;
- (id)drawer;

- (void)setMovie:(QTMovie *)m;

- (void)toggleInfoDrawer:sender;
- (void)setDurationDisplay;
- (void)setNormalSizeDisplay;
- (void)setCurrentSizeDisplay;
- (void)setSource:(NSString *)name;
- (void)setTimeDisplay;
- (void)setRateDisplay;
- (void)UpdateDrawer;
- (void)gotoTime:sender;
- (NSNumber*) currentTime;
- (void) setCurrentTime:(NSNumber*)value;

- (void) ToggleMCController;
- (void)setOneToOne:sender;
- (void)setHalfSize:sender;
- (void)setDoubleSize:sender;
- (void)setFullscreenSize:sender;
- (void)setHalvedSize:sender;
- (void)setHalvedSizeAll:sender;
- (void)setDoubledSize:sender;
- (void)setDoubledSizeAll:sender;

- (void)setFullscreen:sender;
- (void)doExportSettings:sender;
- (void)exportPopupChanged:sender;

- (void)goPosterFrame:sender;
- (void)goBeginning:sender;
- (void)goPosterFrameAll:sender;
- (void)goBeginningAll:sender;

- (void)setPBAllToState:(BOOL)state;
- (void)setPBAll:sender;
- (void)setAllPBAll:sender;
- (void)setLoop:sender;
- (void)setLoopAll:sender;

- (void)playAll:sender;
- (void)playAllFullScreen:sender;
- (void)stepForwardAll:sender;
- (void)stepBackwardAll:sender;
- (void) handleResetMenu:(id) sender;

- (void)makeFullscreenView:(FSStyle)style;
- (void)beginFullscreen:(FSStyle)style;
- (void)endFullscreen;

- (void)disableSleep;
- (void)enableSleep;
- (void) showHelpFile:sender;

- (void) removeMovieCallBack;
- (void) installMovieCallBack;

- (id) getQTVOD;
- (void) setQTVOD:(id)it;

- (void) performClose:(id)sender;
- (void) performCloseAll:(id)sender;
// delegate functions:
- (BOOL) windowShouldClose:(id)sender;
- (void) windowWillClose:(NSNotification*)notification;

//@property (retain,getter=getView) QTAMovieView *movieView;
@property BOOL resizesVertically;
@property (retain) NSWindow *fullscreenWindow;
@property (retain) QTAMovieView *fullscreenMovieView;
@property (retain) NSDrawer		*mDrawer;
@property (retain) NSTextField	*mCurrentSize;
@property (retain) NSTextField	*mDuration;
@property (retain) NSTextField	*mNormalSize;
@property (retain) NSTextField	*mSourceName;
@property (retain) NSATextField	*mTimeDisplay;
@property (retain) NSATextField	*mRateDisplay;
@property (retain) NSView *exportAccessoryView;
@property (retain) NSPopUpButton *exportTypePopup;
@property (retain) NSTextField *exportInfoField;
@property (retain) ExportManager *exportManager;
@property (retain) NSTimer *dontSleepTimer;
@property ComponentResult callbackResult;
//@property (setter=setQTVOD:) struct QTVOD *theQTVOD;
@property (retain) NSURL *theURL;
@property BOOL inFullscreen, delayedClosing, shouldBeClosed;
@property int Playing;
@property NSDrawerState InfoDrawer;
@property QTMovieWindowH qtmwH;
@property unsigned long ACSCcount;
@property (retain) QTAMovieView *movieView;
//@property (getter=getQTVOD,setter=setQTVOD:) struct QTVOD *theQTVOD;
@property (retain,getter=getQTVOD,setter=setQTVOD:) id theQTVOD;
@property short addToRecentMenu;
@end

@interface NSApplication (toggleLogging)
- (void)toggleLogging:sender;
@end
@interface NSApplication (showPreferences)
- (void)showPreferences:sender;
@end

@interface QTVODApplicationDelegate : NSObject {
}
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;

@end

#	define _QTVODWINDOW_H
#endif // _QTVODWINDOW_H
