//
//  QTVODWindow.m
//  QTAmateur -> QTVOD
//
//  Created by Michael Ash on 5/22/05.
//  Copyright __MyCompanyName__ 2005 . All rights reserved.
//

#import "QTVODWindow.h"

#import <QTKit/QTKit.h>

#import "ExportManager.h"
#import "MAMovieExport.h"

#include <pthread.h>

#ifdef DEBUG
#	include <stdio.h>
#endif

#import "../QTils/macosx/NSQTWindow.h"
#import "../QTils/QTilities.h"
#import "QTVOD.h"
#import "QTVODlib.h"

extern void register_QTMovieWindowH( QTMovieWindowH, NativeWindow );
extern void unregister_QTMovieWindowH_from_NativeWindow(NativeWindow);

static pascal Boolean QTActionCallBack( MovieController mc, short action, void* params, long refCon );

typedef struct MyDocList{
	QTVODWindow *docwin;
	int id, playing;
	struct MyDocList *car, *cdr;
} MyDocList;

MyDocList *DocList= NULL;
int MyDocs= 0;

QTVOD *qtVOD = NULL;

static void NSnoLog( NSString *format, ... )
{
	return;
}

static void doNSLog( NSString *format, ... )
{ va_list ap;
  extern int QTils_Log(const char *fileName, int lineNr, NSString *format, ... );
	va_start(ap, format);
	NSLogv( format, ap );
	QTils_Log( __FILE__, __LINE__, [[[NSString alloc] initWithFormat:format arguments:ap] autorelease] );
	va_end(ap);
	return;
}

#ifndef DEBUG
#	define NSLog	NSnoLog
#endif

@interface QTVODWindow (Private)

- (NSSize)movieSizeForWindowSize:(NSSize)s;
- (NSSize)windowSizeForMovieSize:(NSSize)s;
- (NSSize)movieUnsizedSize;
- (NSSize)moviePreferredSize;
- (NSSize)movieCurrentSize;
- (void)setMovieSize:(NSSize)size;
- (void)setMovieSize:(NSSize)size caller:(const char*) caller;
- (void)setMovieSize:(NSSize)size callerNS:(NSString*) callerNS;
- (NSSize)sizeWithMovieAspectFromSize:(NSSize)s;

@end


@implementation QTVODWindow (Private)

- (NSSize)movieSizeForWindowSize:(NSSize)s
{
	NSWindow *window = [movieView window];
	NSRect windowRect = [window frame];
	windowRect.size = s;
	NSSize newContentSize = [window contentRectForFrameRect:windowRect].size;

	NSSize viewSize = [movieView bounds].size;
	if( [movieView isControllerVisible] ){
		viewSize.height -= [movieView controllerBarHeight];
	}
	NSSize curWindowSize = [[window contentView] bounds].size;

	float dx = curWindowSize.width - viewSize.width;
	float dy = curWindowSize.height - viewSize.height;

	return NSMakeSize(newContentSize.width - dx, newContentSize.height - dy);
}

- (NSSize)windowSizeForMovieSize:(NSSize)s
{
	NSWindow *window = [movieView window];
	NSSize viewSize = [movieView bounds].size;
	if( [movieView isControllerVisible] ){
		viewSize.height -= [movieView controllerBarHeight];
	}
	NSRect contentRect = [[window contentView] bounds];
	NSSize curWindowSize = contentRect.size;

	float dx = curWindowSize.width - viewSize.width;
	float dy = curWindowSize.height - viewSize.height;

#ifdef SHADOW
	contentRect.size = NSMakeSize(s.width + dx, 2 * (s.height + dy));
#else
	contentRect.size = NSMakeSize(s.width + dx, s.height + dy);
#endif

	return [window frameRectForContentRect:contentRect].size;
}

- (NSSize)windowContentSizeForMovieSize:(NSSize)s
{ NSWindow *window = [movieView window];
  NSSize viewSize = [movieView bounds].size;
	if( [movieView isControllerVisible] ){
		viewSize.height -= [movieView controllerBarHeight];
	}
	NSRect contentRect = [[window contentView] bounds];
	NSSize curWindowSize = contentRect.size;

	float dx = curWindowSize.width - viewSize.width;
	float dy = curWindowSize.height - viewSize.height;

#ifdef SHADOW
	contentRect.size = NSMakeSize(s.width + dx, 2 * (s.height + dy));
#else
	contentRect.size = NSMakeSize(s.width + dx, s.height + dy);
#endif

	return contentRect.size;
}

- (NSSize)movieUnsizedSize
{
	return NSMakeSize(300, 0);
}

- (NSSize)moviePreferredSize
{ QTMovie *m= (inFullscreen)? [fullscreenMovieView movie] : (movieView)? [movieView movie] : movie;
	NSSize size;
	NSValue *sizeObj = [[m movieAttributes] objectForKey:QTMovieNaturalSizeAttribute];
	if(sizeObj)
		size = [sizeObj sizeValue];
	if(!sizeObj || size.height < 2){
		if( m != movie && movie ){
			sizeObj = [[movie movieAttributes] objectForKey:QTMovieNaturalSizeAttribute];
			if(sizeObj){
				size = [sizeObj sizeValue];
			}
			else{
				size = [self movieUnsizedSize];
			}
		}
		else{
			size = [self movieUnsizedSize];
		}
	}
//	NSLog( @"%@ preferred size: %gx%g", theURL, size.width, size.height );
	return size;
}


- (NSSize)movieCurrentSize
{
#if 0
	QTMovie *m= (inFullscreen)? [fullscreenMovieView movie] : [movieView movie];
	NSSize size;
	NSValue *sizeObj = [[m movieAttributes] objectForKey:QTMovieNaturalSizeAttribute];
	if(sizeObj)
		size = [sizeObj sizeValue];
	if(!sizeObj || size.height < 2)
		size = [self movieUnsizedSize];
	return size;
#else
#	if 0
	NSWindow *w = [movieView window];
	NSRect cbounds = [w contentRectForFrameRect:[w frame]];
#	endif
//	NSLog( @"%@ current size: %gx%g / %gx%g", [theURL path],
//		 movieSize.width, movieSize.height,
//		 cbounds.size.width, cbounds.size.height );
	return movieSize;
#endif
}

- (void)setMovieSize:(NSSize)size
{ QTMovie *m= (inFullscreen)? [fullscreenMovieView movie] : [movieView movie];
	NSLog( @"setMovieSize %@ to %fx%f", [theURL path], size.width, size.height );
	movieSize = size;
	// this probably has no effect:
	[m setMovieAttributes:[NSDictionary dictionaryWithObject:[NSValue valueWithSize:size] forKey:QTMovieNaturalSizeAttribute]];
}

- (void)setMovieSize:(NSSize)size caller:(const char*)caller
{ QTMovie *m= (inFullscreen)? [fullscreenMovieView movie] : [movieView movie];
	NSLog( @"setMovieSize(%s) %@ to %fx%f", caller, [theURL path], size.width, size.height );
	movieSize = size;
	// this probably has no effect:
	[m setMovieAttributes:[NSDictionary dictionaryWithObject:[NSValue valueWithSize:size] forKey:QTMovieNaturalSizeAttribute]];
}

- (void)setMovieSize:(NSSize)size callerNS:(NSString*)callerNS
{ QTMovie *m= (inFullscreen)? [fullscreenMovieView movie] : [movieView movie];
	NSLog( @"setMovieSize(%@) %@ to %fx%f", callerNS, [theURL path], size.width, size.height );
	movieSize = size;
	// this probably has no effect:
	[m setMovieAttributes:[NSDictionary dictionaryWithObject:[NSValue valueWithSize:size] forKey:QTMovieNaturalSizeAttribute]];
}

- (NSSize)sizeWithMovieAspectFromSize:(NSSize)s
{
	NSSize preferredSize = [self moviePreferredSize];
	float aspect = preferredSize.width / preferredSize.height;
	float xx = s.width;
	float yx = s.height * aspect;
	float x = MIN(xx, yx);
	return NSMakeSize(x, x / aspect);
}

@end

@implementation QTVODWindow

- (id)init
{
	self = [super init];
	if (self) {
		// Add your subclass-specific initialization here.
		// If an error occurs here, send a [self release] message and return nil.
		addToRecentMenu = -1;
		vodView = -1;
	}
	return self;
}

- (void)dealloc
{ QTVOD *qv;
	qv = [self getQTVOD];
	[self setQTVOD:nil];
	if( qv && qv.sysOwned == self ){
		// don't let QTVOD refer to us anymore:
		qv->sysOwned = nil;
		[qv closeAndRelease];
	}
	if( movie ){
		[self removeMovieCallBack];
		[self setMovie:nil];
	}
	[theURL release];
	[exportManager release];

	[super dealloc];
}

- (id)drawer
{
	return( mDrawer );
}

- (NSString*) thePath
{
	return [theURL path];
}

- (QTAMovieView*)getView
{
	return movieView;
}

// return movieView as a QTMovieView (the class parent)
// This gives selector compatibility with the NSQTMovieWindow class used in QTils.framework
- (QTMovieView*) theMovieView
{
	return movieView;
}

- (QTMovie*)getMovie
{
	return( movie );
}

- (QTMovie*)theQTMovie
{
	return( movie );
}

- (void)setMovie:(QTMovie *)m
{
	if(m != movie)
	{ NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
		if(movie){
			[center removeObserver:self name:QTMovieDidEndNotification object:movie];
		}
		if( qtmwH && QTMovieWindowH_Check(qtmwH) ){
		  QTMovieWindows *wi = *qtmwH;
			// remove pointers and reset values that reflect our internal state that
			// shouldn't be touched by or influence DisposeQTMovieWindow() !
			wi->theMovieView = NULL;
			wi->theMC = NULL;
			wi->theView = NULL;
			wi->theViewPtr = NULL;
			wi->theMovie = NULL;
			wi->performingClose = FALSE;
			wi->handlingEvent = FALSE;
			wi->theNSQTMovieWindow = NULL;
			DisposeQTMovieWindow(qtmwH);
		}
		if( isProgrammatic ){
		  Movie qm = [movie quickTimeMovie];
			// take action to ensure that the movie file will be released. This is done automagically
			// when the movie was opened through the GUI.
			[movie invalidate];
			DisposeMovie(qm);
		}
		[movie release];
		if( m ){
		  ErrCode wihErr;
		  extern QTMovieWindowH InitQTMovieWindowHFromMovie( QTMovieWindowH wih, const char *theURL, Movie theMovie,
								  Handle dataRef, OSType dataRefType, DataHandler dh, short resId, ErrCode *err );
		  extern QTMovieWindowH AllocQTMovieWindowH();
			movie = [m retain];
			if(movie){
				[center addObserver:self selector:@selector(movieEnded:) name:QTMovieDidEndNotification object:movie];
				[self installMovieCallBack];
			}
			ACSCcount = 0;
			qtmwH = InitQTMovieWindowHFromMovie( AllocQTMovieWindowH(),
					[[m attributeForKey:QTMovieFileNameAttribute] cStringUsingEncoding:NSUTF8StringEncoding],
					[m quickTimeMovie], NULL, 'prox', NULL, -1, &wihErr
			);
		}
		else{
			movie = nil;
			qtmwH = NULL;
		}
		[movieView setMovie:movie];
		[mTimeDisplay setMovie:movie];
		[mRateDisplay setMovie:movie];
#ifdef SHADOW
		if(movieShadow)
			[center removeObserver:self name:QTMovieDidEndNotification object:movieShadow];
		[movieShadow release];
		movieShadow = [m retain];
		if(movieShadow){
			[center addObserver:self selector:@selector(movieEnded:) name:QTMovieDidEndNotification object:movieShadow];
//			[self installMovieCallBack];
		}
		[movieShadowView setMovie:movieShadow];
#endif
		[self setMovieSize:[self moviePreferredSize]];
		[self setOneToOne:self];
		resizesVertically = ([self moviePreferredSize].height >= 2);
		playAllFrames= ( [[movie attributeForKey:QTMoviePlaysAllFramesAttribute] boolValue] )? NSOnState : NSOffState;
		Loop= ( [[movie attributeForKey:QTMovieLoopsAttribute] boolValue] )? NSOnState : NSOffState;

		if( m!= nil ){
			MyDocList *new= (MyDocList*) calloc( 1, sizeof(MyDocList) );
			if( new ){
				new->docwin= self;
				new->cdr= DocList;
				new->id= MyDocs;
				new->playing= 0;
				if( DocList ){
					DocList->car= new;
				}
				DocList= new;
				MyDocs+= 1;
			}
		}
		else{
		  MyDocList *list= DocList, *next;
			while( list ){
				next= list->cdr;
				if( list->docwin== self ){
					if( list== DocList ){
						DocList->car= NULL;
						DocList= DocList->cdr;
					}
					else{
						if( list->cdr ){
							list->cdr->car= list->car;
						}
						if( list->car ){
							list->car->cdr = list->cdr;
						}
					}
					list->docwin= NULL;
					list->car= list->cdr= NULL;
					free(list);
					MyDocs-= 1;
				}
				list= next;
			}
		}
		if( qtmwH ){
		  QTMovieWindows *wi = *qtmwH;
			// bogus ID: will be set to a meaningful value for relevant windows only
			wi->idx = -1;
			wi->theView = (NativeWindow) [movieView window];
			wi->theViewPtr = &wi->theView;
			if( wi->theView ){
				register_QTMovieWindowH( qtmwH, wi->theView );
			}
			wi->theMovieView = movieView;
			wi->theMC = [m quickTimeMovieController];
			// the NSQTMovieWindow interface in the QTils framework is an elaboration of our own
			// QTMovieWindow interface: it offers the same selectors so we can initialise wi->theNSQTMovieWindow
			// as follows.
			wi->theNSQTMovieWindow = (void*) self;
		}
		else{
			qtmwH = NULL;
		}
	}
}

-(void) ToggleMCController
{ QTAMovieView *qv = [self getView];
  NSRect winRect = [[qv window] frame];
  NSSize winSize;
  float dy;
  BOOL vis = ![qv isControllerVisible];
	[qv setControllerVisible:vis];
	winSize = [self windowSizeForMovieSize:[self movieCurrentSize]];
	dy = winSize.height - winRect.size.height;
	winRect.origin.y -= dy;
	winRect.size = winSize;
	[[qv window] setFrame:winRect display:YES animate:NO];
	if( qtmwH && *qtmwH && (*qtmwH)->user_data ){
	  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
		[qtVOD UpdateGeometryForWindow:qtmwH];
	}
}

static NSMenuItem *NormalMI=NULL, *HalfMI=NULL, *DoubleMI= NULL, *MaxMI=NULL,
	*PlayAllFramesMI= NULL, *FullScreenMI=NULL, *SpanningMI=NULL, *PlayAllFramesAllMI= NULL,
	*LoopMI= NULL, *LoopAllMI= NULL;
static int playAllFramesAll= 0, LoopAll= 0;

- (void)updateMenus
{
	if( HalfMI ){
		[HalfMI setState:halfSize];
	}
	if( NormalMI ){
		[NormalMI setState:normalSize];
	}
	if( DoubleMI ){
		[DoubleMI setState:doubleSize];
	}
	if( MaxMI ){
		[MaxMI setState:maxSize];
	}
	if( FullScreenMI ){
		[FullScreenMI setState:((inFullscreen)?NSOnState : NSOffState)];
	}
	if( SpanningMI ){
		[SpanningMI setState:((inFullscreen)?NSOnState : NSOffState)];
	}
	playAllFrames= ( [[movie attributeForKey:QTMoviePlaysAllFramesAttribute] boolValue] )? NSOnState : NSOffState;
	Loop= ( [[movie attributeForKey:QTMovieLoopsAttribute] boolValue] )? NSOnState : NSOffState;
	if( PlayAllFramesMI ){
		[PlayAllFramesMI setState:playAllFrames];
	}
	if( PlayAllFramesAllMI ){
		[PlayAllFramesAllMI setState:playAllFramesAll];
	}
	if( LoopMI ){
		[LoopMI setState:Loop];
	}
	if( LoopAllMI ){
		[LoopAllMI setState:LoopAll];
	}
	[self setCurrentSizeDisplay];
	if( (InfoDrawer= [mDrawer state])== NSDrawerOpeningState ){
		InfoDrawer= NSDrawerOpenState;
	}
	else if( InfoDrawer== NSDrawerClosingState ){
		InfoDrawer= NSDrawerClosedState;
	}
}

BOOL doingAll = NO;

- (void)setOneToOne:sender
{
	NSSize size = [self moviePreferredSize];

	if( !doingAll ){
		if( qtmwH && *qtmwH && (*qtmwH)->user_data ){
		  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
			[qtVOD UpdateGeometryForWindow:qtmwH];
			if( self == qtVOD->TC || self == qtVOD.sysOwned ){
				[qtVOD PlaceWindows:nil withScale:1.0 withNSSize:[qtVOD->forward moviePreferredSize]];
			}
			else{
				[qtVOD PlaceWindows:nil withScale:1.0 withNSSize:size];
				[self updateMenus];
				return;
			}
		}
	}

	if( inFullscreen ){
		[self setMovieSize:size];
	}
	else{
		NSWindow *window = [movieView window];
		NSSize winSize = [self windowSizeForMovieSize:size];
		NSRect winRect = [window frame];
		float dy = winSize.height - winRect.size.height;
		winRect.origin.y -= dy;
		winRect.size = winSize;
		[window setFrame:winRect display:YES animate:NO];
		[self setMovieSize:size caller:"setOneToOne"];
	}

	if( sender!= self ){
		NormalMI= sender;
	}
	normalSize= NSOnState;
	halfSize= doubleSize= maxSize= NSOffState;
	[self updateMenus];
}

- (void)setHalfSize:sender
{
	NSSize size = [self moviePreferredSize];
	size.width /= 2.0;
	size.height /= 2.0;

	if( !doingAll ){
		if( qtmwH && *qtmwH && (*qtmwH)->user_data ){
		  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
			[qtVOD UpdateGeometryForWindow:qtmwH];
			if( self == qtVOD->TC || self == qtVOD.sysOwned ){
			  NSSize hsize = [qtVOD->forward moviePreferredSize];
				hsize.width /= 2.0;
				hsize.height /= 2.0;
				[qtVOD PlaceWindows:nil withScale:1.0 withNSSize:hsize];
			}
			else{
				[qtVOD PlaceWindows:nil withScale:1.0 withNSSize:size];
				[self updateMenus];
				return;
			}
		}
	}

	if( inFullscreen ){
		[self setMovieSize:size];
	}
	else{
		NSSize winSize = [self windowSizeForMovieSize:size];
		NSWindow *window = [movieView window];
		NSRect winRect = [window frame];
		float dy = winSize.height - winRect.size.height;
		winRect.origin.y -= dy;
		winRect.size = winSize;
		[self setMovieSize:size];
		[window setFrame:winRect display:YES animate:NO];
	}

	if( sender!= self ){
		HalfMI= sender;
	}
	halfSize= NSOnState;
	normalSize= doubleSize= maxSize= NSOffState;
	if( doingAll && qtmwH && *qtmwH && (*qtmwH)->user_data ){
	  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
		[qtVOD UpdateGeometryForWindow:qtmwH];
	}
	[self updateMenus];
}

- (void)setDoubleSize:sender
{
	NSSize size = [self moviePreferredSize];
	size.width *= 2.0;
	size.height *= 2.0;

	if( !doingAll ){
		if( qtmwH && *qtmwH && (*qtmwH)->user_data ){
		  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
			[qtVOD UpdateGeometryForWindow:qtmwH];
			if( self == qtVOD->TC || self == qtVOD.sysOwned ){
			  NSSize hsize = [qtVOD->forward moviePreferredSize];
				hsize.width *= 2.0;
				hsize.height *= 2.0;
				[qtVOD PlaceWindows:nil withScale:1.0 withNSSize:hsize];
			}
			else{
				[qtVOD PlaceWindows:nil withScale:1.0 withNSSize:size];
				[self updateMenus];
				return;
			}
		}
	}

	if( inFullscreen ){
		[self setMovieSize:size];
	}
	else{
		NSSize winSize = [self windowSizeForMovieSize:size];
		NSWindow *window = [movieView window];
		NSRect winRect = [window frame];
		float dy = winSize.height - winRect.size.height;
		winRect.origin.y -= dy;
		winRect.size = winSize;
		[self setMovieSize:size];
		[window setFrame:winRect display:YES animate:NO];
	}

	if( sender!= self ){
		DoubleMI= sender;
	}
	doubleSize= NSOnState;
	normalSize= halfSize= maxSize= NSOffState;
	if( doingAll && qtmwH && *qtmwH && (*qtmwH)->user_data ){
	  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
		[qtVOD UpdateGeometryForWindow:qtmwH];
	}
	[self updateMenus];
}

- (void)setFullscreenSize:sender
{
	NSWindow *window = [movieView window];
	NSRect newFrame = [[window screen] visibleFrame];
	NSSize mSize = [self movieSizeForWindowSize:newFrame.size];
	NSSize idealSize = [self sizeWithMovieAspectFromSize:mSize];
	NSSize idealWindowSize = [self windowSizeForMovieSize:idealSize];

	float dx = newFrame.size.width - idealWindowSize.width;
	float dy = newFrame.size.height - idealWindowSize.height;

	if( inFullscreen ){
		[self setMovieSize:idealSize];
	}
	else{
		newFrame.origin.x += dx / 2.0;
		newFrame.origin.y += dy / 2.0;
		newFrame.size = idealWindowSize;

		[self setMovieSize:idealSize];
		[window setFrame:newFrame display:YES animate:NO];
	}

	if( sender!= self ){
		MaxMI= sender;
	}
	maxSize= NSOnState;
	normalSize= halfSize= doubleSize= NSOffState;
	[self updateMenus];
}

- (void)setHalvedSize:sender
{
	NSSize size = [self movieCurrentSize];
	size.width /= 2.0;
	size.height /= 2.0;

	if( !doingAll ){
		if( qtmwH && *qtmwH && (*qtmwH)->user_data ){
		  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
			[qtVOD UpdateGeometryForWindow:qtmwH];
			[qtVOD PlaceWindows:nil withScale:1.0 withNSSize:size];
			[self updateMenus];
			return;
		}
	}

	if( inFullscreen ){
		[self setMovieSize:size];
	}
	else{
		NSSize winSize = [self windowSizeForMovieSize:size];
		NSWindow *window = [movieView window];
		NSRect winRect = [window frame];
		float dy = winSize.height - winRect.size.height;
		winRect.origin.y -= dy;
		winRect.size = winSize;
		[self setMovieSize:size];
		[window setFrame:winRect display:YES animate:NO];
	}

	if( normalSize== NSOnState ){
		halfSize= NSOnState;
	}
	else{
		halfSize= NSOffState;
	}
	normalSize= doubleSize= maxSize= NSOffState;
	if( doingAll && qtmwH && *qtmwH && (*qtmwH)->user_data ){
	  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
		[qtVOD UpdateGeometryForWindow:qtmwH];
	}
	[self updateMenus];
}

- (void)setHalvedSizeAll:sender
{ MyDocList *list= DocList;
	doingAll = YES;
	while( list ){
		[list->docwin setHalvedSize:sender];
		list= list->cdr;
	}
	doingAll = NO;
	if( qtmwH && *qtmwH && (*qtmwH)->user_data ){
	  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
		[qtVOD PlaceWindows:nil withScale:1.0];
	}
}

- (void)setDoubledSize:sender
{
	NSSize size = [self movieCurrentSize];
	size.width *= 2.0;
	size.height *= 2.0;

	if( !doingAll ){
		if( qtmwH && *qtmwH && (*qtmwH)->user_data ){
		  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
			[qtVOD UpdateGeometryForWindow:qtmwH];
			[qtVOD PlaceWindows:nil withScale:1.0 withNSSize:size];
			[self updateMenus];
			return;
		}
	}

	if( inFullscreen ){
		[self setMovieSize:size];
	}
	else{
		NSSize winSize = [self windowSizeForMovieSize:size];
		NSWindow *window = [movieView window];
		NSRect winRect = [window frame];
		float dy = winSize.height - winRect.size.height;
		winRect.origin.y -= dy;
		winRect.size = winSize;
		[self setMovieSize:size];
		[window setFrame:winRect display:YES animate:NO];
	}

	if( halfSize== NSOnState ){
		normalSize= NSOnState;
		doubleSize= NSOffState;
	}
	else if( normalSize== NSOnState ){
		doubleSize= NSOnState;
		normalSize= NSOffState;
	}
	else{
		doubleSize= NSOffState;
		normalSize= NSOffState;
	}
	halfSize= maxSize= NSOffState;
	if( doingAll && qtmwH && *qtmwH && (*qtmwH)->user_data ){
	  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
		[qtVOD UpdateGeometryForWindow:qtmwH];
	}
	[self updateMenus];
}

- (void)setDoubledSizeAll:sender
{ MyDocList *list= DocList;
	doingAll = YES;
	while( list ){
		[list->docwin setDoubledSize:sender];
		list= list->cdr;
	}
	doingAll = NO;
	if( qtmwH && *qtmwH && (*qtmwH)->user_data ){
	  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
		[qtVOD PlaceWindows:nil withScale:1.0];
	}
}

- (void)setFullscreen:sender
{
	if( sender!= self ){
		FullScreenMI= sender;
	}
	if( inFullscreen ){
		[self endFullscreen];
	}
	else{
		[self beginFullscreen:FSRegular];
	}
	[self updateMenus];
}

- (void)setSpanning:sender
{
	if( sender!= self ){
		SpanningMI= sender;
	}
	if( inFullscreen ){
		[self endFullscreen];
	}
	else{
		[self beginFullscreen:FSSpanning];
	}
	[self updateMenus];
}

- (void)goPosterFrame:sender
{
	if( inFullscreen ){
		[fullscreenMovieView gotoPosterFrame:self];
	}
	else{
		[movieView gotoPosterFrame:self];
	}
}

- (void)goBeginning:sender
{
	if( inFullscreen ){
		[fullscreenMovieView gotoBeginning:self];
	}
	else{
		[movieView gotoBeginning:self];
	}
}

- (void)setPBAllToState:(BOOL)state
{ QTMovie *m= (inFullscreen)? [fullscreenMovieView movie] : [movieView movie];
	playAllFrames= (state)? NSOnState : NSOffState;
	[m setAttribute:[NSNumber numberWithBool:state] forKey:QTMoviePlaysAllFramesAttribute];
	[self updateMenus];
}

- (void)setPBAll:sender
{ QTMovie *m= (inFullscreen)? [fullscreenMovieView movie] : [movieView movie];
	playAllFrames= ( [[m attributeForKey:QTMoviePlaysAllFramesAttribute] boolValue] )? NSOnState : NSOffState;
	if( playAllFrames!= NSOffState ){
		[m setAttribute:[NSNumber numberWithBool:NO] forKey:QTMoviePlaysAllFramesAttribute];
		playAllFrames= NSOffState;
		playAllFramesAll= (playAllFramesAll!=NSOffState && MyDocs>1)? NSMixedState : NSOffState;
	}
	else{
		[m setAttribute:[NSNumber numberWithBool:YES] forKey:QTMoviePlaysAllFramesAttribute];
		playAllFrames= NSOnState;
	}
	if( sender!= self ){
		PlayAllFramesMI= sender;
	}
	[self updateMenus];
}

- (void)setAllPBAll:sender
{ MyDocList *list= DocList;
	while( list ){
		QTMovie *m= (list->docwin->inFullscreen)? [list->docwin->fullscreenMovieView movie] : [list->docwin->movieView movie];
		if( playAllFramesAll!= NSOffState ){
			[m setAttribute:[NSNumber numberWithBool:NO] forKey:QTMoviePlaysAllFramesAttribute];
			list->docwin->playAllFrames= NSOffState;
//			playAllFramesAll= (playAllFramesAll!=NSOffState && MyDocs>1)? NSMixedState : NSOffState;
		}
		else{
			[m setAttribute:[NSNumber numberWithBool:YES] forKey:QTMoviePlaysAllFramesAttribute];
			list->docwin->playAllFrames= NSOnState;
//			playAllFrames= NSOnState;
		}
		list= list->cdr;
	}
	if( playAllFramesAll!= NSOffState ){
		playAllFramesAll= NSOffState;
	}
	else{
		playAllFramesAll= NSOnState;
	}
	if( sender!= self ){
		PlayAllFramesAllMI= sender;
	}
	[self updateMenus];
}

- (void)setLoop:sender
{ QTMovie *m= (inFullscreen)? [fullscreenMovieView movie] : [movieView movie];
	Loop= ( [[m attributeForKey:QTMovieLoopsAttribute] boolValue] )? NSOnState : NSOffState;
	if( Loop!= NSOffState ){
		[m setAttribute:[NSNumber numberWithBool:NO] forKey:QTMovieLoopsAttribute];
		Loop= NSOffState;
		LoopAll= (LoopAll!=NSOffState && MyDocs>1)? NSMixedState : NSOffState;
	}
	else{
		[m setAttribute:[NSNumber numberWithBool:YES] forKey:QTMovieLoopsAttribute];
		Loop= NSOnState;
	}
	if( sender!= self ){
		LoopMI= sender;
	}
	[self updateMenus];
}

- (void)setLoopAll:sender
{ MyDocList *list= DocList;
	while( list ){
		QTMovie *m= (list->docwin->inFullscreen)? [list->docwin->fullscreenMovieView movie] : [list->docwin->movieView movie];
		if( LoopAll!= NSOffState ){
			[m setAttribute:[NSNumber numberWithBool:NO] forKey:QTMovieLoopsAttribute];
			list->docwin->Loop= NSOffState;
		}
		else{
			[m setAttribute:[NSNumber numberWithBool:YES] forKey:QTMovieLoopsAttribute];
			list->docwin->Loop= NSOnState;
		}
		list= list->cdr;
	}
	if( LoopAll!= NSOffState ){
		LoopAll= NSOffState;
	}
	else{
		LoopAll= NSOnState;
	}
	if( sender!= self ){
		LoopAllMI= sender;
	}
	[self updateMenus];
}

- (void)goPosterFrameAll:sender
{ MyDocList *list= DocList;
	while( list ){
		if( inFullscreen ){
			[list->docwin->fullscreenMovieView gotoPosterFrame:self];
		}
		else{
			[list->docwin->movieView gotoPosterFrame:self];
		}
		list= list->cdr;
	}
}

- (void)goBeginningAll:sender
{ MyDocList *list= DocList;
	while( list ){
		if( inFullscreen ){
			[list->docwin->fullscreenMovieView gotoBeginning:self];
		}
		else{
			[list->docwin->movieView gotoBeginning:self];
		}
		list= list->cdr;
	}
}

- (void)playAll:sender
{ MyDocList *list= DocList;
  int state= 0;
	while( list ){
		if( list->docwin ){
			if( list->playing ){
				list->playing= 0;
				[ list->docwin->movieView pause:list->docwin ];
				state+= NSOffState;
			}
			else{
				list->playing= 1;
				[ list->docwin->movieView play:list->docwin ];
				state+= NSOnState;
			}
		}
		list= list->cdr;
	}
	if( MyDocs ){
		state/= MyDocs;
	}
	if( sender!= self ){
		if( state== NSOnState || state== NSOffState ){
			[sender setState:state];
		}
		else{
			[sender setState:NSMixedState];
		}
	}
}

- (void)playAllFullScreen:sender
{ MyDocList *list= DocList;
  int n= 0;
	while( list ){
		if( list->docwin ){
			if( list->playing ){
				list->playing= 0;
				[ list->docwin endFullscreen ];
			}
			else{
				list->playing= 1;
				[ list->docwin makeFullscreenView:FSMosaic ];
			}
		}
		list= list->cdr;
	}
	list= DocList;
	while( list ){
		if( list->docwin ){
			if( list->playing ){
				[ list->docwin->fullscreenMovieView play:list->docwin];
				n+= 1;
			}
		}
		list= list->cdr;
	}

	if( n ){
		[self disableSleep];
	}
	if( sender!= self ){
		if( n== MyDocs ){
			[sender setState:NSOnState];
		}
		else{
			[sender setState:((n)?NSMixedState:NSOffState)];
		}
	}
}

- (void)stepForwardAll:sender
{ MyDocList *list= DocList;
	while( list ){
		if( inFullscreen ){
			[list->docwin->fullscreenMovieView stepForward:self];
		}
		else{
			[list->docwin->movieView stepForward:self];
		}
		list= list->cdr;
	}
}

- (void)stepBackwardAll:sender
{ MyDocList *list= DocList;
	while( list ){
		if( inFullscreen ){
			[list->docwin->fullscreenMovieView stepBackward:self];
		}
		else{
			[list->docwin->movieView stepBackward:self];
		}
		list= list->cdr;
	}
}

- (void)toggleInfoDrawer:sender
{
	[mDrawer toggle:sender];
	if( (InfoDrawer= [mDrawer state])== NSDrawerOpeningState ){
		InfoDrawer= NSDrawerOpenState;
		[self UpdateDrawer];
	}
	else if( InfoDrawer== NSDrawerClosingState ){
		InfoDrawer= NSDrawerClosedState;
	}
}

- (void)setTimeDisplay
{
	if( movie && InfoDrawer ){
	  QTTime currentPlayTime = [[movie attributeForKey:QTMovieCurrentTimeAttribute] QTTimeValue];
		[mTimeDisplay setStringValue:QTStringFromTime(currentPlayTime) /*+@"@"+[NSString stringWithFormat:@"%d",[movie rate]] */ ];
//		NSLog( @"Playing=%d@%g, TC=%@", Playing, [movie rate], QTStringFromTime(currentPlayTime) );
//		{ NSTimeInterval timeInterval;
//			if( QTGetTimeInterval(currentPlayTime, &timeInterval) ){
//				NSLog( @"Playing=%d@%g, TC=%@ = %g", Playing, [movie rate], QTStringFromTime(currentPlayTime), timeInterval );
//			}
//			else{
//				NSLog( @"Playing=%d@%g, TC=%@ can't determine timeinterval", Playing, [movie rate], QTStringFromTime(currentPlayTime) );
//			}
//		}
	}
}

- (void )gotoTime:sender
{
	if( movie && InfoDrawer!= NSDrawerClosedState ){
	  NSString *stime = [mTimeDisplay stringValue];
	  NSString *srate = [mRateDisplay stringValue];
	  QTTime ntime;
	  float rate;
		if( stime ){
			ntime = QTTimeFromString(stime);
			if( (ntime.flags & kQTTimeIsIndefinite) != kQTTimeIsIndefinite ){
			}
			else{
				doNSLog( @"Can't read QT time from \"%@\"", stime );
				ntime = [movie currentTime];
			}
		}
		else{
			doNSLog( @"Can't read string from %@", [mTimeDisplay objectValue] );
		}
		if( srate ){
			rate = [srate floatValue];
			if( rate <= 0 || rate >= HUGE_VAL ){
				NSLog( @"Can't read QT rate from \"%@\" (-> %g)", srate, (double) rate );
				rate = [[movie attributeForKey:QTMoviePreferredRateAttribute] floatValue];
			}
		}
		else{
			NSLog( @"Can't read string from %@", [mRateDisplay objectValue] );
		}
		if( !stime && !srate ){
			return;
		}

		if( qtmwH && *qtmwH && (*qtmwH)->user_data ){
		  QTVOD *qtVOD = (QTVOD*) (*qtmwH)->user_data;
			[qtVOD SetTimes:((double) ntime.timeValue / (double) ntime.timeScale) rates:rate withRefWindow:nil absolute:NO];
			return;
		}
		if( stime ){
			if( (ntime.flags & kQTTimeIsIndefinite) != kQTTimeIsIndefinite ){
				[movie setCurrentTime:ntime];
				[self setTimeDisplay];
			}
		}
		if( srate ){
		  BOOL p = Playing;
			if( p ){
				[movie setRate:rate];
			}
			[movie setAttribute:[NSNumber numberWithFloat:rate] forKey:QTMoviePreferredRateAttribute];
			[self setRateDisplay];
		}
	}
}

- (void)setDurationDisplay
{
	if( movie && InfoDrawer ){
		if( [movie attributeForKey:QTMovieHasDurationAttribute] ){
		  NSString *durStr = QTStringFromTime([[movie attributeForKey:QTMovieDurationAttribute] QTTimeValue]);
			if( durStr ){
				[mDuration setStringValue:durStr];
			}
		}
	}
}

- (void)setRateDisplay
{
	if( movie && InfoDrawer ){
	 NSMutableString *rateString = [NSMutableString string];
		if( Playing ){
			[rateString appendFormat:@"%g x", (double) [movie rate] ];
		}
		else{
			[rateString appendFormat:@"%@ x", [movie attributeForKey:QTMoviePreferredRateAttribute] ];
		}
		[mRateDisplay setStringValue:rateString];
	}
}

- (void)setNormalSizeDisplay
{
	if( movie && InfoDrawer ){
	  NSMutableString *sizeString = [NSMutableString string];
	  NSSize movSize = NSMakeSize(0,0);
	  movSize = [[movie attributeForKey:QTMovieNaturalSizeAttribute] sizeValue];

		[sizeString appendFormat:@"%.0f", movSize.width];
		[sizeString appendString:@" x "];
		[sizeString appendFormat:@"%.0f", movSize.height];

		[mNormalSize setStringValue:sizeString];
	}
}

- (void)setCurrentSizeDisplay
{
	if( movie && InfoDrawer ){
	  NSSize movCurrentSize = [self movieCurrentSize];
	  NSMutableString *sizeString = [NSMutableString string];

//		if( movie && [movieView isControllerVisible] ){
//			movCurrentSize.height -= [movieView controllerBarHeight];
//		}

		[sizeString appendFormat:@"%.0f", movCurrentSize.width];
		[sizeString appendString:@" x "];
		[sizeString appendFormat:@"%.0f", movCurrentSize.height];

		[mCurrentSize setStringValue:sizeString];
	}
}

- (void)setSource:(NSString *)name
{ NSArray *pathComponents = [[NSFileManager defaultManager] componentsToDisplayForPath:name];
  NSEnumerator *pathEnumerator = [pathComponents objectEnumerator];
  NSString *component = [pathEnumerator nextObject];
  NSMutableString *displayablePath = [NSMutableString string];

	while( component != nil ){
		if( [component length] > 0 ){
			[displayablePath appendString:component];

			component = [pathEnumerator nextObject];
			if( component != nil ){
				[displayablePath appendString:@":"];
			}
		}
		else{
			component = [pathEnumerator nextObject];
		}
	}

	[mSourceName setStringValue:displayablePath];
}

- (void)UpdateDrawer
{
	[self setTimeDisplay];
	[self setDurationDisplay];
	[self setRateDisplay];
	[self setNormalSizeDisplay];
	[self setCurrentSizeDisplay];
	[self setSource:[[self fileURL] path]];
}

- (void)doExportSettings:sender
{
	int componentIndex = [exportTypePopup indexOfSelectedItem];
	[exportManager showSettingsAtIndex:componentIndex];
}

- (void)exportPopupChanged:sender
{
	NSArray *components = [[MAMovieExport sharedInstance] componentList];
	NSString *info = [[components objectAtIndex:[exportTypePopup indexOfSelectedItem]] objectForKey:kMAMovieExportComponentInfo];
	if(!info) info = NSLocalizedString(@"No info", @"");
	[exportInfoField setStringValue:info];
}

static unsigned int fullScreenViews= 0;

- (void)makeFullscreenView:(FSStyle)style
{
	if(inFullscreen) return;

	NSWindow *window = [movieView window];
	NSScreen *screen = [window screen];
	NSRect screensRect;
	NSEnumerator *enumerator;
	id scr;
	float minHeight= -1, minX= -1, theY= 0;

	if( style == FSSpanning ){
		memset( &screensRect, 0, sizeof(screensRect) );
		if( [NSScreen screens] ){
			enumerator = [[NSScreen screens] objectEnumerator];
			while( scr = [enumerator nextObject] ){
			  NSRect r = [scr frame];
				screensRect.size.width += r.size.width;
//				NSLog( @"Screen %@ %gx%g+%g+%g", scr, r.size.width, r.size.height, r.origin.x, r.origin.y );
				if( minHeight< 0 || r.size.height< minHeight ){
					minHeight = r.size.height;
				}
				if( minX< 0 || r.origin.x< minX ){
					minX = r.origin.x;
					theY = r.origin.y;
				}
			}
			screensRect.size.height = minHeight;
			screensRect.origin.x = minX;
			screensRect.origin.y = theY;
			NSLog( @"Screen-spanning rect: %gx%g+%g+%g", screensRect.size.width, minHeight, screensRect.origin.x, screensRect.origin.y );
		}
	}

	[window orderOut:self];
	[movieView setMovie:nil];

	if( style == FSSpanning ){
		[fullscreenWindow setFrame:screensRect display:YES];
	}
	else{
		[fullscreenWindow setFrame:[screen frame] display:YES];
	}

	[fullscreenMovieView setMovie:movie];
	[fullscreenWindow makeKeyAndOrderFront:self];

	if([screen isEqual:[NSScreen mainScreen]])
		SetSystemUIMode(kUIModeAllHidden, 0);

	inFullscreen = YES;
	fullScreenViews += 1;
}

- (void)beginFullscreen:(FSStyle)style
{
	if( !inFullscreen ){
		[self makeFullscreenView:style];
	}

	NSLog( @"Starting fullscreen playback self=%p%@ fMV=%p%@",
		  self, self, fullscreenMovieView, fullscreenMovieView
	);
	[fullscreenMovieView pplay:self];

	[self disableSleep];
}

- (void)endFullscreen
{
	if(!inFullscreen) return;

	NSLog( @"Ending fullscreen playback self=%p%@ fMV=%p%@ views=%u",
		  self, self, fullscreenMovieView, fullscreenMovieView, fullScreenViews
	);

	if( fullScreenViews> 0 ){
		fullScreenViews -= 1;
	}
	if( fullScreenViews== 0 ){
		SetSystemUIMode(kUIModeNormal, 0);
	}

	[fullscreenMovieView pause:self];
	[fullscreenWindow orderOut:self];
	[fullscreenMovieView setMovie:nil];

	[movieView setMovie:movie];
	[movieView setControllerVisible:YES];
	[[movieView window] makeKeyAndOrderFront:self];

	[self enableSleep];

	inFullscreen = NO;
}

- (void)updateSystemActivity:(NSTimer *)timer
{
	UpdateSystemActivity(OverallAct);
}

- (void)disableSleep
{
	if(!dontSleepTimer)
		dontSleepTimer = [[NSTimer scheduledTimerWithTimeInterval:30.0 target:self selector:@selector(updateSystemActivity:) userInfo:nil repeats:YES] retain];
}

- (void)enableSleep
{
	[dontSleepTimer invalidate];
	[dontSleepTimer release];
	dontSleepTimer = nil;
}

- (void) showHelpFile:sender
{
	NSLog(@"clicked help..." );
}

// document stuff

-(void)awakeFromNib
{
	// initialise movie drawer items
	InfoDrawer= [mDrawer state];
	[super awakeFromNib];
	[self UpdateDrawer];
}

- (NSString *)windowNibName
{
    // Override returning the nib file name of the document
    // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this method and override -makeWindowControllers instead.
    return @"QTVODWindow";
}

- (NSString*) description
{
	if( theQTVOD ){
		return [[NSString alloc]
			   initWithFormat:@"<%@ showing movie %@, part of %@>", NSStringFromClass([self class]),
			   movie, theQTVOD];
	}
	else{
		return [[NSString alloc]
			   initWithFormat:@"<%@ showing movie %@>", NSStringFromClass([self class]),
			   movie];
	}
}

- (void)windowControllerDidLoadNib:(NSWindowController *) aController
{
    [super windowControllerDidLoadNib:aController];
	[movieView setMovie:movie];
	[self setOneToOne:self];
	InfoDrawer= [mDrawer state];

	[[movieView window] setShowsResizeIndicator:NO];
	[movieView setShowsResizeIndicator:YES];
	if( qtmwH && (*qtmwH)->theView != (NativeWindow) [movieView window] ){
	  QTMovieWindows *wi = *qtmwH;
	  QTVOD *qv = (QTVOD*) [self getQTVOD];
		wi->theView = (NativeWindow) [movieView window];
		wi->theViewPtr = &wi->theView;
		if( wi->theView ){
			register_QTMovieWindowH( qtmwH, wi->theView );
		}
		wi->theNSQTMovieWindow = (void*) self;
		if( qv ){
			NSLog( @"re-registering window #%d for %@ (%@)", (*qtmwH)->idx, qv, [qv displayName] );
			[qv reregister_window:qtmwH];
			[qv PlaceWindows:nil withScale:1.0];
			[qv PlaceWindows:nil withScale:1.0];
		}
	}
}

- (NSData *)dataRepresentationOfType:(NSString *)aType
{
    // Insert code here to write your document from the given data.  You can also choose to override -fileWrapperRepresentationOfType: or -writeToFile:ofType: instead.
    // For applications targeted for Tiger or later systems, you should use the new Tiger API -dataOfType:error:.  In this case you can also choose to override -writeToURL:ofType:error:, -fileWrapperOfType:error:, or -writeToURL:ofType:forSaveOperation:originalContentsURL:error: instead.
    return nil;
}

#include <unistd.h>

- (void) setQTVOD:(id)it
{ QTVOD *prev = theQTVOD;
	theQTVOD = [it retain];
	if( prev ){
		[prev release];
	}
}

- (id) getQTVOD
{
	return theQTVOD;
}

#pragma mark ---- AppleScript interface functions

- (NSString*) uniqueID
{
	return [[[NSString stringWithFormat:@"QTVOD-%lx-%@", getpid(), [self displayName]] retain] autorelease];
}

- (id) handlePlayScriptCommand:(NSScriptCommand*) command
{ QTVOD *qv = [self getQTVOD];
	SLOG( @"playScript %@", command );
	if( qv ){
		[qv StartVideoExceptFor:nil];
	}
	else{
		[[self theMovieView] play:[[self theMovieView] window]];
	}
	return nil;
}

- (id) handleStopScriptCommand:(NSScriptCommand*) command
{ QTVOD *qv = [self getQTVOD];
	SLOG( @"stopScript %@", command );
	if( qv ){
		[qv StopVideoExceptFor:nil];
	}
	else{
		[[self theMovieView] pause:[[self theMovieView] window]];
	}
	return nil;
}

- (id) handleStepForwardScriptCommand:(NSScriptCommand*) command
{
	// if we're a QTVOD "master document", the MCAction handlers will cause all displayed windows
	// to be stepped when our own movie (the full, source) is stepped
	[[self theMovieView] stepForward:self];
	return nil;
}

- (id) handleStepBackwardScriptCommand:(NSScriptCommand*) command
{
	[[self theMovieView] stepBackward:self];
	return nil;
}

// adding a new chapter requires passing in a name and a startTime
- (id) handleAddChapterScriptCommand:(NSScriptCommand*) command
{ NSDictionary *args = [command arguments];
  NSString *name;
  NSNumber *startTime, *duration;
  ErrCode err = noErr;
	NSLog(@"addChapter %@ (%@)", command, NSStringFromClass([command class]) );
	name = [args objectForKey:@"QTMovieChapterName"];
	startTime = [args objectForKey:@"QTMovieChapterStartTime"];
	if( (duration = [args objectForKey:@"QTMovieChapterDuration"]) ){
		NSLog( @"duration=%@ (%@)", duration, NSStringFromClass([duration class]) );
	}
	if( name && startTime ){
	  QTVOD *qv = [self getQTVOD];
		err = MovieAddChapter( [movie quickTimeMovie], 0,
						  [name cStringUsingEncoding:NSUTF8StringEncoding],
						  [startTime doubleValue], (duration)? [duration doubleValue] : 0.0 );
		if( qv ){
			MovieAddChapter( [[qv.TC getMovie] quickTimeMovie], 0,
				[name cStringUsingEncoding:NSUTF8StringEncoding],
				[startTime doubleValue], (duration)? [duration doubleValue] : 0.0 );
			// we replace err here:
			err = MovieAddChapter( qv.fullMovie, 0,
				[name cStringUsingEncoding:NSUTF8StringEncoding],
				[startTime doubleValue], (duration)? [duration doubleValue] : 0.0 );
			if( err != noErr || HasMovieChanged(qv.fullMovie) ){
#ifdef DEBUG
			  NSString *fu = [[self fileURL] path];
#endif
				qv->fullMovieChanged = YES;
//				[self updateChangeCount:NSChangeReadOtherContents];
			}
		}
	}
	if( err != noErr ){
		[command setScriptErrorNumber:err];
		[command setScriptErrorString:@"QTils::MovieAddChapter returned an error"];
	}
	return nil;
}

- (id) handleMarkIntervalTimeScriptCommand:(NSScriptCommand*) command
{ NSDictionary *args = [command arguments];
  NSNumber *reset = [args objectForKey:@"IntervalTimeReset"];
  NSNumber *display = [args objectForKey:@"IntervalTimeDisplay"];
  QTVOD *qv = [self getQTVOD];
	if( qv ){
		[qv CalcTimeInterval:((display)? [display boolValue] : NO) reset:((reset)?[reset boolValue] : NO)];
	}
	else{
		[command setScriptErrorNumber:paramErr];
		[command setScriptErrorString:@"this video does not support interval measurements"];
	}
	return nil;
}

- (id) handleResetScriptCommand:(NSScriptCommand*) command
{ NSDictionary *args = [command arguments];
  NSNumber *complete = [args objectForKey:@"CompleteReset"];
  QTVOD *qv = [self getQTVOD];
	if( qv ){
		[qv ResetVideo:((complete)? [complete boolValue] : NO)];
		return qv.sysOwned;
	}
	else{
		[command setScriptErrorNumber:paramErr];
		[command setScriptErrorString:@"this video does not support resets"];
	}
	return nil;
}

- (void) handleResetMenu:(id) sender
{ QTVOD *qv = [self getQTVOD];
	if( qv ){
		[qv ResetVideo:NO closeSysWin:YES];
	}
	else if( qtmwH && QTVDOC(qtmwH) ){
		[QTVDOC(qtmwH) ResetVideo:NO];
	}
}

- (id) handleReadDesignScriptCommand:(NSScriptCommand*) command
{ NSDictionary *args = [command arguments];
  NSString *fName = [args objectForKey:@"name"];
  QTVOD *qv = [self getQTVOD];
	doNSLog(@"readDesign %@[%@] (%@)", command, args, NSStringFromClass([command class]) );
	if( qv ){
		[qv nsReadDefaultVODDescription:fName toDescription:&qv->theDescription];
	}
	else{
		[command setScriptErrorNumber:paramErr];
		[command setScriptErrorString:@"this video does not support XML design files"];
	}
	return nil;
}

#pragma mark -------------------

- (NSNumber*) duration
{ double t;
  QTVOD *qv = [self getQTVOD];
	if( qv ){
		t = [qv duration];
	}
	else if( QTMovieWindowH_Check(qtmwH) ){
		t = (*qtmwH)->info->duration;
	}
	else{
	  QTTime duration = [[movie attributeForKey:QTMovieDurationAttribute] QTTimeValue];
		t = (double) duration.timeValue / (double) duration.timeScale;
	}
	return [[[NSNumber alloc] initWithDouble:t] autorelease];
}

- (NSNumber*) frameRate
{ double t;
  QTVOD *qv = [self getQTVOD];
	if( qv ){
		t = [qv frameRate:NO];
	}
	else if( QTMovieWindowH_Check(qtmwH) ){
		t = (*qtmwH)->info->frameRate;
	}
	else{
		SLOG( @"no known frameRate" );
		t = 0.0/0.0;
	}
	return [[[NSNumber alloc] initWithDouble:t] autorelease];
}

- (NSNumber*) TCframeRate
{ double t;
  QTVOD *qv = [self getQTVOD];
	if( qv ){
		t = [qv frameRate:YES];
	}
	else if( QTMovieWindowH_Check(qtmwH) ){
		t = (*qtmwH)->info->TCframeRate;
	}
	else{
	  NSArray *trackArray = [movie tracksOfMediaType:QTMediaTypeTimeCode];
	  MediaHandler tcMediaH;
	  TimeCodeRecord ft;
	  TimeCodeDef tcdef;
	  long frame;
	  ErrCode err;
		if( trackArray && [trackArray count] > 0 ){
			tcMediaH = GetMediaHandler([[[trackArray objectAtIndex:0] media] quickTimeMedia]);
			err = (ErrCode) TCGetCurrentTimeCode( tcMediaH, &frame, &tcdef, &ft, NULL );
			if( err == noErr ){
				SLOG( @"TCframeRate determined from 1st TimeCode track" );
				t = ((double)tcdef.fTimeScale)
									/ ((double)tcdef.frameDuration);
			}
			else{
				SLOG( @"no known TCframeRate" );
				t = 0.0/0.0;
			}
		}
		else{
			SLOG( @"nothing to determine TCframeRate from" );
			t = 0.0/0.0;
		}
	}
	return [[[NSNumber alloc] initWithDouble:t] autorelease];
}

- (NSNumber*) startTime
{ double t;
  QTVOD *qv = [self getQTVOD];
	if( qv ){
		t = [qv startTime];
	}
	else if( QTMovieWindowH_Check(qtmwH) ){
		t = (*qtmwH)->info->timeScale;
	}
	else{
	  NSArray *trackArray = [movie tracksOfMediaType:QTMediaTypeTimeCode];
	  long startFrameNr;
	  TimeRecord tr;
		if( trackArray && [trackArray count] > 0 ){
			GetMovieStartTime( [movie quickTimeMovie], [[trackArray objectAtIndex:0] quickTimeTrack], &tr, &startFrameNr );
			SLOG( @"startTime determined from 1st TimeCode track" );
			t = ((double) *((SInt64*)&tr.value))/((double)tr.scale);
		}
		else{
			SLOG( @"no known startTime" );
			t = 0.0/0.0;
		}
	}
	return [[[NSNumber alloc] initWithDouble:t] autorelease];
}

- (NSNumber*) currentTime
{ double t;
  QTVOD *qv = [self getQTVOD];
	if( qv ){
		t = [qv getTime:NO];
	}
	else	if( QTMovieWindowH_Check(qtmwH) ){
		QTMovieWindowGetTime( qtmwH, &t, 0 );
	}
	else{
	  QTTime currentPlayTime = [[movie attributeForKey:QTMovieCurrentTimeAttribute] QTTimeValue];
		t = (double) currentPlayTime.timeValue / (double) currentPlayTime.timeScale;
	}
	return [[[NSNumber alloc] initWithDouble:t] autorelease];
}

- (void) setCurrentTime:(NSNumber*)value
{ double t = [value doubleValue];
  QTVOD *qv = [self getQTVOD];
	if( qv ){
		[qv SetTimes:t withRefWindow:nil absolute:NO];
	}
	else if( QTMovieWindowH_Check(qtmwH) ){
		QTMovieWindowSetTime( qtmwH, t, 0 );
	}
	else{
	  QTTime currentPlayTime = [[movie attributeForKey:QTMovieCurrentTimeAttribute] QTTimeValue];
		currentPlayTime.timeValue = (long long)( t * (double) currentPlayTime.timeScale );
		[movie setCurrentTime:currentPlayTime];
	}
}

- (NSNumber*) absCurrentTime
{ double t;
  QTVOD *qv = [self getQTVOD];
	if( qv ){
		t = [qv getTime:YES];
	}
	else	if( QTMovieWindowH_Check(qtmwH) ){
		QTMovieWindowGetTime( qtmwH, &t, 1 );
	}
	else{
	  NSArray *trackArray = [movie tracksOfMediaType:QTMediaTypeTimeCode];
	  MediaHandler tcMediaH;
	  TimeCodeRecord ft;
	  TimeCodeDef tcdef;
	  long frame;
	  ErrCode err;
	  double TCframeRate;
		if( trackArray && [trackArray count] > 0 ){
			tcMediaH = GetMediaHandler([[[trackArray objectAtIndex:0] media] quickTimeMedia]);
			err = (ErrCode) TCGetCurrentTimeCode( tcMediaH, &frame, &tcdef, &ft, NULL );
			if( err == noErr ){
				TCframeRate = ((double)tcdef.fTimeScale)
									/ ((double)tcdef.frameDuration);
				t = FTTS(&ft.t, TCframeRate);
				SLOG( @"absolute current time determined from 1st TimeCode track" );
			}
			else{
				SLOG( @"no known absolute current time" );
				t = 0.0/0.0;
			}
		}
		else{
			return [self currentTime];
		}
	}
	return [[[NSNumber alloc] initWithDouble:t] autorelease];
}

- (void) setAbsCurrentTime:(NSNumber*)value
{ double t = [value doubleValue];
  QTVOD *qv = [self getQTVOD];
	if( qv ){
		[qv SetTimes:t withRefWindow:nil absolute:YES];
	}
	else if( QTMovieWindowH_Check(qtmwH) ){
		QTMovieWindowSetTime( qtmwH, t, 1 );
	}
	else{
	  QTTime currentPlayTime = [[movie attributeForKey:QTMovieCurrentTimeAttribute] QTTimeValue];
	  double startTime = [[self startTime] doubleValue];
		// lazy, fixme, absolute time is not necessarily exactly equal to relative time + startTime!
		currentPlayTime.timeValue = (long long)( (t - startTime) * (double) currentPlayTime.timeScale );
		[movie setCurrentTime:currentPlayTime];
	}
}

- (NSNumber*) lastInterval
{ double t;
  QTVOD *qv = [self getQTVOD];
	if( qv ){
		t = qv.theTimeInterval.dt;
	}
	else{
		SLOG( @"no known last interval" );
		t = 0.0/0.0;
	}
	return [[[NSNumber alloc] initWithDouble:t] autorelease];
}


- (NSArray*) chapterNames
{ Movie m = [movie quickTimeMovie];
  long N = GetMovieChapterCount(m);
  NSMutableArray *ret = nil;
	if( N > 0 ){
	  long i;
	  char *txt;
		ret = [[[NSMutableArray arrayWithCapacity:N] retain] autorelease];
		for( i = 0 ; i < N ; i++ ){
			txt = NULL;
			if( GetMovieIndChapter( m, i, NULL, &txt ) == noErr && txt ){
				[ret addObject:[NSString stringWithCString:txt encoding:NSUTF8StringEncoding]];
				free(txt);
				txt = NULL;
			}
		}
		NSLog( @"chapterNames: %@[0] = %@ (%@)", ret, [ret objectAtIndex:0], NSStringFromClass([[ret objectAtIndex:0] class]) );
	}
	return [[[NSArray arrayWithArray:ret] retain] autorelease];
}

- (NSArray*) chapterTimes
{ Movie m = [movie quickTimeMovie];
  long N = GetMovieChapterCount(m);
  NSMutableArray *ret = nil;
	if( N > 0 ){
	  long i;
	  double t;
		ret = [NSMutableArray arrayWithCapacity:N];
		for( i = 0 ; i < N ; i++ ){
			if( GetMovieIndChapter( m, i, &t, NULL ) == noErr ){
				[ret addObject:[NSNumber numberWithDouble:t]];
			}
		}
		NSLog( @"chapterTimes: %@", ret );
	}
	return [[[NSArray arrayWithArray:ret] retain] autorelease];
}

- (NSArray*) chapters
{ Movie m = [movie quickTimeMovie];
  long N = GetMovieChapterCount(m);
  NSMutableArray *ret = nil;
	if( N > 0 ){
	  long i;
	  double t;
	  char *txt;
		ret = [[[NSMutableArray arrayWithCapacity:N] retain] autorelease];
		for( i = 0 ; i < N ; i++ ){
			txt = NULL;
			if( GetMovieIndChapter( m, i, &t, &txt ) == noErr && txt ){
			  NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:
							    [NSString stringWithCString:txt encoding:NSUTF8StringEncoding], @"QTMovieChapterName",
							    [NSNumber numberWithDouble:t], @"QTMovieChapterStartTime", nil ];
				[ret addObject:dict];
				free(txt);
				txt = NULL;
			}
		}
		NSLog( @"chapters: %@[0] = %@ (%@)", ret, [ret objectAtIndex:0], NSStringFromClass([[ret objectAtIndex:0] class]) );
	}
	return [[[NSArray arrayWithArray:ret] retain] autorelease];
}

#pragma mark -------- open and read a requested file ----

- (BOOL)readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError
{ QTMovie *m;
  static char active = 0;
  NSString *absPath = [absoluteURL path];
  BOOL addRecent = addToRecentDocs;
//	doNSLog( @"URL=\"%@\", ofType \"%@\"", absoluteURL, typeName );
	if( [typeName isEqual:@"XMLPropertyList"] ){
		return NO;
	}
	if( !active && ([absPath hasSuffix:@".VOD" caseSensitive:YES]
				 || [absPath hasSuffix:@".avi" caseSensitive:YES]
				 || [absPath hasSuffix:@".IEF" caseSensitive:YES]
				 || [absPath hasSuffix:@"-design.qi2m" caseSensitive:YES])
	){
		active = 1;

		{ NSString *aDFile;
			if( [absPath hasSuffix:@".IEF" caseSensitive:YES] ){
				aDFile = absPath;
			}
			else if( assocDataFileName ){
				aDFile = [NSString stringWithUTF8String:assocDataFileName];
				QTils_free(assocDataFileName);
			}
			else{
				// createWithAbsoluteURL retains the assocDataFile for us
				aDFile = @"*FromVODFile*";
			}
			qtVOD = [QTVOD createWithAbsoluteURL:absoluteURL ofType:typeName forDocument:self withAssocDataFile:aDFile error:outError];
			if( qtVOD && !*outError ){
				shouldBeInvisible = YES;
				if( sServer != NULLSOCKET ){
				  NetMessage msg;
				  VODDescription descr = [qtVOD theDescription];
					msgOpenFile( &msg, [absPath UTF8String], &descr );
					msg.flags.category = qtvod_Notification;
					SendMessageToNet( sServer, &msg, SENDTIMEOUT, NO, __FUNCTION__ );
				}
			}
		}

		if( ([absPath hasSuffix:@".VOD" caseSensitive:YES]
			|| [absPath hasSuffix:@".IEF" caseSensitive:YES]
			|| [absPath hasSuffix:@".qi2m" caseSensitive:YES])
		){ extern NSURL *pruneExtensions( NSURL *URL );
		  NSURL *baseURL = pruneExtensions(absoluteURL);
		  NSFileManager *nfs = [[[NSFileManager alloc] init] autorelease];
		  NSString *cacheMov = [NSString stringWithFormat:@"%@.mov", [baseURL path]];
			if( [nfs isReadableFileAtPath:cacheMov] ){
				absoluteURL = [NSURL fileURLWithPath:cacheMov];
			}
			else if( ![absPath hasSuffix:@".VOD" caseSensitive:YES] ){
				cacheMov = [NSString stringWithFormat:@"%@.VOD", [baseURL path]];
				// don't test, let the system handle any errors:
				absoluteURL = [NSURL fileURLWithPath:cacheMov];
			}
		}

		addToRecentDocs = YES;
		// our own state variable:
		addToRecentMenu = 1;
		[self readFromURL:absoluteURL ofType:typeName error:outError];
		addToRecentDocs = addRecent;
		active = 0;
	}
	else{
		m = [QTMovie movieWithURL:absoluteURL error:outError];
		theURL = [absoluteURL retain];
		NSLog( @"Opened \"%@\" (process %u:%u) %d,%d\n", [absoluteURL path], getpid(), pthread_self(),
			 [[theURL path] hasSuffix:@".VOD" caseSensitive:YES], [[theURL path] hasSuffix:@".VOD" caseSensitive:NO] );
		if( isProgrammatic ){
			[self makeWindowControllers];
			[self showWindows];
		}
		[self setMovie:m];
	}
	return YES;
}

- (BOOL)readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName
{
	NSError *error = nil;
	return [self readFromURL:absoluteURL ofType:typeName error:&error];
}


- (BOOL)readFromFile:(NSString *)fileName ofType:(NSString *)docType
{
	NSURL *url = [NSURL fileURLWithPath:fileName];
	NSError *error = nil;
	return [self readFromURL:url ofType:docType error:&error];
}

// RJVB 20091214: override the default saveDocumentAs() method:
- (IBAction)saveDocumentAs:(id)sender
{
	NSSavePanel *panel = [NSSavePanel savePanel];
	[panel setPrompt:NSLocalizedString(@"Save (reference)", @"")];
	[panel setTitle:NSLocalizedString(@"Save (reference)", @"")];
	[panel setNameFieldLabel:NSLocalizedString(@"Save (reference) As:", @"")];
	// we'd really like to be sure we save files of the right type - QT is a little braindead about that!
	[panel setRequiredFileType:@"mov"];
	[panel setCanSelectHiddenExtension:YES];

	[panel beginSheetForDirectory:nil file:nil
				modalForWindow:[movieView window] modalDelegate:self
				didEndSelector:@selector(savePanelDidEnd:returnCode:contextInfo:) contextInfo:NULL ];
}

- (void)savePanelDidEnd:(NSSavePanel *)panel returnCode:(int)code contextInfo:(void *)ctx
{
	if(code == NSOKButton)
	{
		// save the movie with the proper filename, and flattening it
		[movie writeToFile:[panel filename]
		    withAttributes:nil
		];
	}
}

- (IBAction)saveDocument:(id)sender
{
	NSSavePanel *panel = [NSSavePanel savePanel];
	[panel setPrompt:NSLocalizedString(@"Save (contained)", @"")];
	[panel setTitle:NSLocalizedString(@"Save (contained)", @"")];
	[panel setNameFieldLabel:NSLocalizedString(@"Save (contained) As:", @"")];
	// we'd really like to be sure we save files of the right type - QT is a little braindead about that!
	[panel setRequiredFileType:@"mov"];
	[panel setCanSelectHiddenExtension:YES];

	[panel beginSheetForDirectory:nil file:nil
				modalForWindow:[movieView window] modalDelegate:self
				didEndSelector:@selector(savePanelDidEndFlat:returnCode:contextInfo:) contextInfo:NULL ];
}

- (void)savePanelDidEndFlat:(NSSavePanel *)panel returnCode:(int)code contextInfo:(void *)ctx
{
	if(code == NSOKButton)
	{
		// save the movie with the proper filename, and flattening it
		[movie writeToFile:[panel filename]
		    withAttributes:[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:YES], QTMovieFlatten, nil]
		];
	}
}

- (IBAction)saveDocumentTo:(id)sender
{
	if(!exportManager)
		exportManager = [[ExportManager alloc] initWithQTMovie:movie];

	// first, prepare the accessory view
	NSArray *components = [[MAMovieExport sharedInstance] componentList];

	[exportTypePopup removeAllItems];
	for(id loopItem in components)
	{
		NSString *name = [loopItem objectForKey:kMAMovieExportComponentName];
		if(!name) name = [loopItem objectForKey:kMAMovieExportComponentFileExtension];
		if(!name) name = @"(null)";
		[[exportTypePopup menu] addItemWithTitle:name action:NULL keyEquivalent:@""];
	}

	int index = [exportManager defaultIndex];
	[exportTypePopup selectItemAtIndex:index];
	[self exportPopupChanged:self];

	NSSavePanel *panel = [NSSavePanel savePanel];
	[panel setAccessoryView:exportAccessoryView];
	[panel setPrompt:NSLocalizedString(@"Export", @"")];
	[panel setTitle:NSLocalizedString(@"Export", @"")];
	[panel setNameFieldLabel:NSLocalizedString(@"Export To:", @"")];

	[panel beginSheetForDirectory:nil file:nil modalForWindow:[movieView window] modalDelegate:self didEndSelector:@selector(exportPanelDidEnd:returnCode:contextInfo:) contextInfo:NULL];
}

- (void)exportPanelDidEnd:(NSSavePanel *)panel returnCode:(int)code contextInfo:(void *)ctx
{
	[exportManager setDefaultIndex:[exportTypePopup indexOfSelectedItem]];

	if(code == NSOKButton)
	{
		int componentIndex = [exportTypePopup indexOfSelectedItem];
		[exportManager exportToFile:[panel filename] named:[self displayName] atIndex:componentIndex];
	}
}

- (void)windowDidResize:(NSNotification *)dummy
{
	if( delayedClosing ){
		[self performClose:nil];
	}
	else{
		[self setCurrentSizeDisplay];
	}
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{ NSSize mSize = [self movieSizeForWindowSize:frameSize];
	if( !([[sender currentEvent] modifierFlags] & NSShiftKeyMask) || !resizesVertically ){
	  NSSize idealSize = [self sizeWithMovieAspectFromSize:mSize];
	  NSSize idealWindowSize = [self windowSizeForMovieSize:idealSize];
		[self setMovieSize:idealSize caller:"windowWillResize"];
		return idealWindowSize;
	}
	else{
		[self setMovieSize:mSize caller:"windowWillResize"];
		return frameSize;
	}
}

- (NSRect)windowWillUseStandardFrame:(NSWindow *)sender defaultFrame:(NSRect)newFrame
{ NSSize mSize = [self movieSizeForWindowSize:newFrame.size];
	if( !([[sender currentEvent] modifierFlags] & NSShiftKeyMask) || !resizesVertically ){
	  NSSize idealSize = [self sizeWithMovieAspectFromSize:mSize];
	  NSSize idealWindowSize = [self windowSizeForMovieSize:idealSize];

		float dx = newFrame.size.width - idealWindowSize.width;
		float dy = newFrame.size.height - idealWindowSize.height;

		newFrame.origin.x += dx / 2.0;
		newFrame.origin.y += dy / 2.0;
		newFrame.size = idealWindowSize;

		[self setMovieSize:idealSize caller:"windowWillUseStandardFrame"];
		return newFrame;
	}
	else{
		[self setMovieSize:mSize caller:"windowWillUseStandardFrame"];
		return newFrame;
	}
}

- (void)windowDidUpdate:(NSNotification *)dummy
{
	if( delayedClosing ){
		[self performClose:nil];
	}
	else{
	  QTVOD *qv = [self getQTVOD];
		if( shouldBeClosed && (!qv || qv.shouldBeClosed || (qv.sysOwned && qv.sysOwned.shouldBeClosed)) ){
			if( qv ){
				[qv closeAndRelease];
			}
			else{
				[self performClose:nil];
			}
			return;
		}
		else if( shouldBeInvisible ){
		  NSWindow *nswin = [[self getView] window];
			if( [nswin isVisible] ){
				NSLog( @"[%@ orderOut:%@]", [self displayName], nswin );
				[nswin orderOut:nswin];
				if( [nswin canBecomeMainWindow] ){
					[nswin makeMainWindow];
				}
				return;
			}
		}
		[self setCurrentSizeDisplay];
	}
}

- (void)windowDidLoad:(NSNotification *)dummy
{
	NSLog( @"[%@ windowDidLoad]", [theURL lastPathComponent] );
	[self setCurrentSizeDisplay];
}

- (void)windowDidExpose:(NSNotification *)dummy
{
	NSLog( @"[%@ windowDidExpose]", [theURL lastPathComponent] );
	[self setCurrentSizeDisplay];
}

- (void)movieEnded:(NSNotification *)notification
{
//	[self performClose:self];
}

//- (BOOL)validateMenuItem:(id <NSMenuItem>)item
- (BOOL)validateMenuItem:(NSMenuItem*)item
{
	SEL sel = [item action];
	return /* sel != @selector(saveDocument:)
		&& sel != @selector(saveDocumentAs:)
		&& */ sel != @selector(revertDocumentToSaved:)
		&& sel != @selector(runPageLayout:)
		&& sel != @selector(printDocument:);
}

- (void)dumpTypes
{
	/*NSMutableString *str = [NSMutableString string];

	NSString *format = @"\t\t<dict>\n"
		"\t\t\t<key>CFBundleTypeExtensions</key>\n"
		"\t\t\t<array>\n"
		"\t\t\t\t<string>%@</string>\n"
		"\t\t\t</array>\n"
		"\t\t\t<key>CFBundleTypeIconFile</key>\n"
		"\t\t\t<string></string>\n"
		"\t\t\t<key>CFBundleTypeName</key>\n"
		"\t\t\t<string>AllFiles</string>\n"
		"\t\t\t<key>CFBundleTypeOSTypes</key>\n"
		"\t\t\t<array>\n"
		"\t\t\t\t<string>%@</string>\n"
		"\t\t\t</array>\n"
		"\t\t\t<key>CFBundleTypeRole</key>\n"
		"\t\t\t<string>Viewer</string>\n"
		"\t\t\t<key>NSDocumentClass</key>\n"
		"\t\t\t<string>QTVODWindow</string>\n"
		"\t\t</dict>\n";*/

	NSMutableString *typeExtensionsString = [NSMutableString string];
	NSMutableString *osTypesString = [NSMutableString string];

	NSEnumerator *enumerator = [[QTMovie movieFileTypes:QTIncludeStillImageTypes] objectEnumerator];
	NSString *type;
	while((type = [enumerator nextObject]))
	{
		NSMutableString *toAppend = typeExtensionsString;
		if([type length] == 6 && [type characterAtIndex:0] == '\'' && [type characterAtIndex:5] == '\'')
		{
			toAppend = osTypesString;
			type = [type substringWithRange:NSMakeRange(1, 4)];
		}
		[toAppend appendFormat:@"\t\t\t\t<string>%@</string>\n", type];
	}

	NSLog( @"Types:\n\n\nextensions:\n%@\n\n\nostypes:\n%@\n\n\n", typeExtensionsString, osTypesString);
}

- (void) removeMovieCallBack
{
	if( movie ){
		NSLog( @"removeMovieCallBack %@", movie );
		MCSetActionFilterWithRefCon( [movie quickTimeMovieController], nil, (long) self );
	}
}

- (void) installMovieCallBack
{
	callbackResult= noErr;

	MovieController mc= [movie quickTimeMovieController];
	MCActionFilterWithRefConUPP upp= NewMCActionFilterWithRefConUPP(QTActionCallBack);

	if( mc && upp ){
		callbackResult= MCSetActionFilterWithRefCon( mc, upp, (long) self );
	}
	if( upp ){
		DisposeMCActionFilterWithRefConUPP(upp);
	}
}

- (void) performClose:(id)sender
{
	if(inFullscreen){
		[self endFullscreen];
	}
	else{
	  NSWindow *nswin = [[self getView] window];
		if( nswin ){
			if( sender ){
				[nswin performClose:sender];
			}
			else{
				[nswin performClose:nswin];
			}
		}
	}
}

- (void) performCloseAll:(id)sender
{ MyDocList *list;
	if( QTVODList ){
		for( QTVOD *qv in QTVODList ){
			[qv close];
		}
		[QTVODList release];
		QTVODList = nil;
	}
	list = DocList;
	while( list ){
		if( [list->docwin getQTVOD] ){
			// should no longer happen!
			[(QTVOD*)[list->docwin getQTVOD] closeAndRelease];
			list = DocList;
		}
		else{
			[list->docwin performClose:nil];
			list= list->cdr;
		}
	}
}

- (BOOL) windowShouldClose:(id)sender
{
	NSLog( @"[%@ windowShouldClose]", self );
	if( handlingMCAction > 0 ){
		delayedClosing = YES;
		return NO;
	}
	else{
		delayedClosing = NO;
	}
	if( QTMovieWindowH_Check(qtmwH) && !(*qtmwH)->performingClose ){
	  int (*fun)(QTMovieWindowH wi, void *params) = NULL;
	  id target;
	  SEL selector;
	  NSMCActionCallback nsfun = NULL;
		(*qtmwH)->shouldClose = TRUE;
		(*qtmwH)->performingClose = TRUE;
		// 20110310: stop the idling, that is the movie should no longer be tasked!
		[[(QTMovieView*)(*qtmwH)->theMovieView movie] setIdling:NO];
		if( (nsfun = get_NSMCAction( qtmwH, MCAction()->Close, &target, &selector )) ){
			(*nsfun)( target, selector, qtmwH, NULL );
		}
		else if( (fun = get_MCAction( qtmwH, MCAction()->Close )) ){
			(*fun)( qtmwH, NULL );
		}
		// qtmwH is a class variable and can thus have been changed ...
		if( qtmwH ){
			(*qtmwH)->performingClose = FALSE;
			if( QTVDOC(qtmwH) && qtmwH == QTVDOC(qtmwH)->timeBaseMaster ){
				[QTVDOC(qtmwH) SlaveWindowsToMovie:NULL storeCurrent:NO];
			}
		}
	}
	[self removeMovieCallBack];
	[self setMovie:nil];
	if( isProgrammatic ){
		[self autorelease];
	}
	// if all's well, the window will be closed for us if we return YES. We can thus
	// unset the flag whether it ought to be closed! (or, we could have a 3-level state,
	// closingState = {remainOpen=0, shouldBeClosed, willBeClosed}
	shouldBeClosed = NO;
	return YES;
}

- (void) windowWillClose:(NSNotification*)notification
{ NSWindow *nswin = [notification object];
  QTMovieWindowH wi;
  extern QTMovieWindowH QTMovieWindowHFromNativeWindow( NativeWindow hWnd );
  QTVOD *qv;

	if( QTMovieWindowH_Check(qtmwH) && !(*qtmwH)->shouldClose ){
		doNSLog( @"[%@ %@%@] : calling windowShouldClose method first", NSStringFromClass([self class]), NSStringFromSelector(_cmd), nswin );
		[self windowShouldClose:nswin];
	}
	else{
		NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), nswin );
	}
	if( (qv = [self getQTVOD]) ){
		[self setQTVOD:nil];
		if( qv.sysOwned == self || qv.sysOwned == nil ){
			// don't let QTVOD refer to us anymore:
			qv->sysOwned = nil;
			[qv closeAndRelease];
		}
	}
	wi = QTMovieWindowHFromNativeWindow((NativeWindow)nswin);
	if( QTMovieWindowH_Check(wi) ){
		unregister_QTMovieWindowH_from_NativeWindow((*wi)->theView);
		(*wi)->theView = NULL;
	}
	else{
		addToRecentDocs = YES;
	}
	if( addToRecentMenu >= 0 ){
		addToRecentDocs = (addToRecentMenu > 0);
	}
}

//- (id) handleCloseScriptCommand:(NSCloseCommand*)command
//{
//	NSLog( @"[%@ handleCloseScriptCommand:%@]", self, command );
//	return self;
//}

//@synthesize movieView;
@synthesize resizesVertically;
@synthesize fullscreenWindow;
@synthesize fullscreenMovieView;
@synthesize mDrawer;
@synthesize mCurrentSize;
@synthesize mDuration;
@synthesize mNormalSize;
@synthesize mSourceName;
@synthesize mTimeDisplay;
@synthesize mRateDisplay;
@synthesize exportAccessoryView;
@synthesize exportTypePopup;
@synthesize exportInfoField;
@synthesize exportManager;
@synthesize dontSleepTimer;
@synthesize callbackResult;
//@synthesize theQTVOD;
@synthesize theURL;
@synthesize inFullscreen, delayedClosing, shouldBeClosed;
@synthesize Playing;
@synthesize InfoDrawer;
@synthesize qtmwH;
@synthesize ACSCcount;
@synthesize movieView;
@synthesize addToRecentMenu;
@end

#ifdef DEBUG2

//	NSLog( @"%s: %@ %@ Stepped=%d Scanned=%d par=%p (process %u:%p)", actionStr, [mydoc getMovie], \
//		  QTStringFromTime([[[mydoc getMovie] attributeForKey:QTMovieCurrentTimeAttribute] QTTimeValue]), \
//		  mydoc->wasStepped, mydoc->wasScanned, params, \
//		  getpid(), pthread_self() \
//	); \

#	define mcFeedBack(mydoc,actionStr)	{ \
	fname = [ [[mydoc getMovie] attributeForKey:QTMovieFileNameAttribute] fileSystemRepresentation]; \
	timestr = [ QTStringFromTime([[[mydoc getMovie] attributeForKey:QTMovieCurrentTimeAttribute] QTTimeValue]) fileSystemRepresentation]; \
	fprintf( stderr, "%s: %s %s rate=%g Stepped=%d Scanned=%d par=%p", actionStr, fname, timestr, \
		  (double)[[mydoc getMovie] rate], mydoc->wasStepped, mydoc->wasScanned, params \
	); \
	if( action == mcActionKeyUp ){ \
		fprintf( stderr, " key=%c", (char) ((EventRecord*) params)->message & charCodeMask ); \
	} \
	fprintf( stderr, " (process %u:%p)\n", getpid(), pthread_self() ); \
}
#else
#	define mcFeedBack(mydoc,actionStr)	/**/
#endif

#ifdef DEBUG
OSErr MCSetMovieTime( Movie theMovie, MovieController theMC, TimeValue t )
{ OSErr err;
	if( theMovie ){
	  TimeRecord trec;
		GetMovieTime( theMovie, &trec );
		err = GetMoviesError();
		if( err == noErr ){
			// trec.value is a 'wide', a structure containing a lo and a high int32 variable.
			// set it by casting to an int64 because that's the underlying intention ...
			*( (SInt64*)&(trec.value) ) = (SInt64)( t );
			SetMovieTime( theMovie, &trec );
			err = GetMoviesError();
			UpdateMovie( theMovie );
			if( theMC ){
				MCMovieChanged( theMC, theMovie );
				MCIdle( theMC );
			}
		}
	}
	else{
		err = paramErr;
	}
	return err;
}
#endif //DEBUG

static Boolean nothing( DialogPtr dialog, EventRecord *event, DialogItemIndex *itemhit )
{
	doNSLog( @"MovieInfo callback dialog=%@, event=%@, index=%@", dialog, event, itemhit );
	return 0;
}

static pascal Boolean QTActionCallBack( MovieController mc, short action, void* params, long refCon )
{ QTVODWindow *mydoc= (QTVODWindow*) refCon;
 pascal Boolean ret;
#ifdef DEBUG
#	ifdef DEBUG2
  const char *fname;
  const char *timestr;
#	endif
  QTTime curTime = [[[mydoc getMovie] attributeForKey:QTMovieCurrentTimeAttribute] QTTimeValue];
#endif
  extern Boolean QTMovieWindow_MCActionHandler( MovieController mc, short action, void *params, long refCon);

	mydoc->handlingMCAction += 1;
	switch( action ){
			case mcActionControllerSizeChanged:
			case mcActionActivate:
			case mcActionDeactivate:
			case mcActionSetPlaySelection:
			case mcActionMouseDown:
			case mcActionMovieClick:
			case mcActionSuspend:
			case mcActionResume:
			case mcActionMovieFinished:
#ifdef DEBUG
				if( mydoc->wasStepped > 0 || mydoc->wasScanned > 0 ){
					fprintf( stderr, "resetting Stepped:%d Scanned:%d action=%d\n", mydoc->wasStepped, mydoc->wasScanned, action );
				}
#endif
				mydoc->wasStepped = FALSE;
				mydoc->wasScanned = FALSE;
				break;
	}
	switch( action ){
		case mcActionControllerSizeChanged:{
			mydoc->ACSCcount += 1;
			break;
		}
		case mcActionMovieLoadStateChanged:
			// params==kMovieLoadStateComplete when streaming video has been received completely?
			mcFeedBack( mydoc, "LoadStateChanged" );
			break;
		case mcActionIdle:
#if DEBUG == 2
			if( mydoc->wasScanned > 0 || mydoc->wasStepped > 0 )
			{
				mcFeedBack( mydoc, "Idle" );
			}
#endif
			if(  mydoc->InfoDrawer== NSDrawerOpenState ){
				if( mydoc->Playing== -1 ){
					mydoc->Playing= ([[mydoc getMovie] rate]> 1e-5)? 1 : NO;
				}
				if( mydoc->Playing ){
					[mydoc setTimeDisplay];
//					[mydoc setRateDisplay];
				}
			}
			break;
		case mcActionStep:
			mydoc->wasStepped = TRUE;
#ifdef DEBUG
			mcFeedBack( mydoc, "Step" );
#endif
			[mydoc setTimeDisplay];
//			[mydoc setRateDisplay];
			// RJVB 20070920: hook to finding the frame time in the EDF data here.
			// determine if a matching EDF filename exists when opening a movie
			// if so, activate the play all frames option
			break;
		case mcActionPlay:{
		  int rate= (int) ([[mydoc getMovie] rate]*1e5);
#ifdef DEBUG
			mcFeedBack( mydoc, "Play" );
#endif
			if( mydoc->wasStepped > 0 ){
				// this is to prevent the upcoming GoToTime action to be taken for a user-scan event.
				mydoc->wasStepped = -1;
			}
			mydoc->wasScanned = FALSE;
			[mydoc UpdateDrawer];
			mydoc->Playing= (rate==0)? -1 : NO;
			break;
		}
		case mcActionActivate:{
#ifdef DEBUG
			mcFeedBack( mydoc, "Activate" );
#endif
			[mydoc setRateDisplay];
			[mydoc updateMenus];
			break;
		}
#ifdef DEBUG
		case mcActionGoToTime:
			if( !mydoc->wasStepped ){
				mydoc->wasScanned = TRUE;
			}
			mcFeedBack( mydoc, "GoToTime" );
			[mydoc UpdateDrawer];
			if( mydoc->wasStepped < 0 ){
				mydoc->wasStepped = FALSE;
			}
			break;
		case mcActionSetPlaySelection:
			mcFeedBack( mydoc, "SetPlaySelection" );
			break;
#endif
		case mcActionMouseDown:
#ifdef DEBUG
			NSLog( @"MouseDown %@, rate=%g", [mydoc getMovie], (double)[[mydoc getMovie] rate] );
#endif
			if( [[mydoc getMovie] rate]< 1e-5 ){
				mydoc->Playing= NO;
			}
			break;
		case mcActionMovieClick:
#ifdef DEBUG
			NSLog( @"MovieClick %@, rate=%g", [mydoc getMovie], (double)[[mydoc getMovie] rate] );
#endif
			if( [[mydoc getMovie] rate]< 1e-5 ){
				mydoc->Playing= NO;
			}
			[mydoc setRateDisplay];
			break;
		case mcActionKeyUp:{
		  EventRecord *evt = (EventRecord*) params;
			evt->message &= charCodeMask;
			switch( evt->message ){
				case 'C':
					[mydoc ToggleMCController];
					break;
				case 'I':
					if( ![mydoc getQTVOD] && !(mydoc->qtmwH && *(mydoc->qtmwH) && (*(mydoc->qtmwH))->user_data) ){
						ShowMovieInformation( [[mydoc getMovie] quickTimeMovie], nothing, (long) mydoc );
					}
					break;
				case 'L':{
				 extern char lastSSLogMsg[];
					if( lastSSLogMsg[0] ){
						PostMessage( "Last Log message", lastSSLogMsg );
					}
					break;
				}
			}
			mcFeedBack( mydoc, "KeyUp" );
			break;
		}
		case mcActionResume:
			mcFeedBack( mydoc, "Resume" );
			[mydoc setRateDisplay];
			break;
		case mcActionMovieFinished:
			mcFeedBack( mydoc, "MovieFinished" );
			mydoc->Playing= NO;
			break;
		default:
#if DEBUG == 2
			if( action!= mcActionIdle ){
				NSLog( @"action #%hd refCon=%p", action, refCon );
			}
#endif
			break;
	}
	ret = QTMovieWindow_MCActionHandler( mc, action, params, (long) mydoc->qtmwH );
	mydoc->handlingMCAction -= 1;
	return ret;
}

__attribute__((constructor))
static void initialiser( int argc, char** argv, char** envp )
{ extern Boolean QTMWInitialised;
	if( !QTMWInitialised ){
		// 20130603: at this point, the connection to the window server hasn't yet been made,
		// so handle that first.
		[NSApplication sharedApplication];
		InitQTMovieWindows();
	}
}

