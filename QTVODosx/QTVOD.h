//
//  QTVOD.h
//  QTVODosx
//
//  Created by René J.V. Bertin on 20110906.
//  Copyright 2011 INRETS/LCPC — LEPSIS. All rights reserved.
//

#ifndef _QTVOD_H

#import <Cocoa/Cocoa.h>
#import <QTKit/QTKit.h>
#import "QTVODWindow.h"
#import "QTVODController.h"
#import "VDPreferences.h"
#import "../QTils/QTilities.h"

extern BOOL addToRecentDocs;

typedef struct TimeInterval {
	double timeA, timeB, dt,
		fps, wallTimeLapse;
	char timeStampA[256], timeStampB[256];
	BOOL benchMarking, wasBenchMarking, AllF[maxQTWM];
} TimeInterval;

#define WINLIST(idx)	((winlist[(idx)])?(*winlist[(idx)]):NULL)
#define QTMWH(idx)		((WINLIST(idx))?((*winlist[(idx)])->qtmwH):NULL)
#define QTVDOC(wih)		((wih && (*wih) && (*wih)->user_data)?((QTVOD*)(*wih)->user_data):NULL)

@interface QTVOD : NSDocument {
	BOOL hasBeenClosed, beingClosed;
	BOOL handlingPlayAction, handlingTimeAction, handlingCloseAction;
@public
	QTVODWindow *forward, *pilot, *left, *right, *TC;
	QTVODWindow **winlist[maxQTWM];
	ComponentInstance initialClock[maxQTWM];
	// typically, one of our windows will have been openened by the OS, and thus "owned"
	// by the system. This ought to be the only window which does not have the isProgrammatic
	// flag set.
	QTVODWindow *sysOwned;
	QTMovieWindowH timeBaseMaster;
	Cartesian Wpos[maxQTWM], Wsize[maxQTWM];
	short numQTMW;
	Movie fullMovie;

	NSURL *theURL;
	NSString *theDirPath, *assocDataFile;
	ComponentInstance xmlParser;
	VODDescription theDescription;
	Cartesian ULCorner;
	TimeInterval theTimeInterval;
	BOOL fullMovieChanged, finalCloseVideo, shouldBeClosed;
}

+ (QTVOD*) createWithAbsoluteURL:(NSURL*)aURL ofType:(NSString*)typeName
	forDocument:(QTVODWindow*)sO withAssocDataFile:(NSString*)aDFile
	error:(NSError **)outError;

- (void) closeAndRelease;

- (void) reregister_window:(QTMovieWindowH)wih;
- (BOOL) readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError;
- (void) ShowMetaData;
- (void) SetWindowLayer:(NSWindowOrderingMode)pos relativeTo:(NSInteger)relTo;
- (void) UpdateGeometryForWindow:(QTMovieWindowH)wih;
- (double) frameRate:(BOOL)forTC;
- (double) duration;
- (double) startTime;
- (double) getTime:(BOOL)absolute;
- (void) SetTimes:(double)t withRefWindow:(QTMovieWindowH)ref absolute:(BOOL)absolute;
- (void) SetTimes:(double)t rates:(float)rate withRefWindow:(QTMovieWindowH)ref absolute:(BOOL)absolute;
- (void) StartVideoExceptFor:(QTVODWindow*)excl;
- (void) StopVideoExceptFor:(QTVODWindow*)excl;
- (void) StepForwardExceptFor:(QTMovieWindowH)excl;
- (void) StepBackwardExceptFor:(QTMovieWindowH)excl;
- (BOOL) BenchmarkPlaybackRate;
- (BOOL) CalcTimeInterval:(BOOL)display reset:(BOOL)reset;
- (void) PlaceWindows:(Cartesian*)ulCorner withScale:(double)scale;
- (void) PlaceWindows:(Cartesian*)ulCorner withScale:(double)scale withNSSize:(NSSize)nsSize;
- (void) CreateQI2MFromDesign;
- (ErrCode) OpenVideo:(NSString *)typeName error:(NSError **)outError;
- (void) CloseVideo:(BOOL)final;
- (void) SlaveWindowsToMovie:(Movie)theMasterMovie storeCurrent:(Boolean)store;
- (ErrCode) ResetVideo:(BOOL)complete;
- (ErrCode) ResetVideo:(BOOL)complete closeSysWin:(BOOL)closeSysWin;
- (ErrCode) ReadDefaultVODDescription:(const char*)fName toDescription:(VODDescription*)descr;
- (ErrCode) nsReadDefaultVODDescription:(NSString*)fName toDescription:(VODDescription*)descr;
- (ErrCode) urlReadDefaultVODDescription:(NSURL*)url toDescription:(VODDescription*)descr;
- (ErrCode) ScanForDefaultVODDescription:(const char*)fName toDescription:(VODDescription*)descr;
- (ErrCode) nsScanForDefaultVODDescription:(NSString*)fName toDescription:(VODDescription*)descr;

@property (readonly) QTVODWindow *sysOwned;
@property (readonly) QTVODWindow *forward, *pilot, *left, *right, *TC;
@property (readonly) short numQTMW;
@property (readonly) Movie fullMovie;
@property (readonly) ComponentInstance xmlParser;
@property (readonly) NSURL *theURL;
@property (readonly) BOOL fullMovieChanged;
@property BOOL shouldBeClosed;
@property (readonly) NSString *assocDataFile;
@property (readonly) TimeInterval theTimeInterval;
@end

@interface NSString (hasSuffix)
- (BOOL) hasSuffix:(NSString*)aString caseSensitive:(BOOL)cs;
@end

extern NSMutableArray *QTVODList;

#define _QTVOD_H
#endif
