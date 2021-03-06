//!
//!  @file NSQTMovieWindow.m
//!  QTMovieWindow
//!
//!  Created by René J.V. Bertin on 20110224
//!  Copyright 2011 RJVB. All rights reserved.
//!

#import "NSQTMovieWindow.h"
#import "QTilities.h"
#import "Lists.h"
#import "QTMovieWin.h"
#import "Logging.h"
#import <crt_externs.h>
// #import <pthread.h>

static QTilsApplicationDelegate *QTilsDelegate = NULL;

static QTMovieWindowH (*OpenMenuHandler)(const char *) = NULL;
static void (*InfoMenuHandler)(QTMovieWindowH wih) = NULL;

void* SetOpenMenuHandler( QTMovieWindowH (*handler)(const char*) )
{ void *prev = OpenMenuHandler;
	OpenMenuHandler = handler;
	return prev;
}

void* SetInfoMenuHandler( void (*handler)(QTMovieWindowH wih) )
{ void *prev = InfoMenuHandler;
	InfoMenuHandler = handler;
	return prev;
}

@implementation NSQTMovieWindow

- (id) init
{ BOOL ok;
	// 20130618:
	self = [super init];
	if( self && !nibLoaded ){
		@try{
			ok = [NSBundle loadNibNamed:@"NSQTMovieWindow" owner:self];
		}
		@catch(NSException *e){
			NSLog( @"[%@ %@] caught exception %@",
				 NSStringFromClass([self class]), NSStringFromSelector(_cmd), e );
			ok = NO;
		}
		if( !ok ){
			NSLog( @"%@ failed to load 'NSQTMovieWindow' nib!", self );
		}
		else{
			nibLoaded = YES;
		}
	}
	if( nibLoaded ){
		[self setTitleWithCString:"NSQTMovieWindow"];
		return self;
	}
	else{
		return nil;
	}
}

- (void) dealloc
{
	Log( qtLogPtr, "[%@ dealloc]", self );
	[self setQTMovie:nil];
	[URL release];
	[super dealloc];
}

- (void) close
{
	Log( qtLogPtr, "[%@ %@]\n", NSStringFromClass([self class]), NSStringFromSelector(_cmd) );
	[super close];
}

- (BOOL)readFromURL:(NSURL *)url ofType:(NSString *)typeName error:(NSError **)outError
{ BOOL success = NO;
	if( outError ){
		*outError = NULL;
	}
	NSLog( @"[%@ %@] URL=%@",
		 NSStringFromClass([self class]), NSStringFromSelector(_cmd), url );
	if( [QTMovie canInitWithURL:url] ){
		[self setQTMovie:((QTMovie *)[QTMovie movieWithURL:url error:outError])];
		success = (*outError == nil && qtMovie != nil);
	}

	return success;
}

- (BOOL)readFromURL:(NSURL *)url ofType:(NSString *)type
{ BOOL success = NO;

	// read the movie
	NSLog( @"[%@ %@] URL=%@",
		 NSStringFromClass([self class]), NSStringFromSelector(_cmd), url );
	if( [QTMovie canInitWithURL:url] ){
		[self setQTMovie:((QTMovie *)[QTMovie movieWithURL:url error:nil])];
		success = (qtMovie != nil);
	}

	return success;
}

- (BOOL)readFromFile:(NSString *)fileName ofType:(NSString *)docType
{ NSURL *url = [NSURL fileURLWithPath:fileName];
	return [self readFromURL:url ofType:docType];
}

- (BOOL)setQTMovieFromMovie:(Movie)theMovie
{ BOOL success = NO;

	// read the movie
	if( theMovie ){
	  NSError *nsErr = nil;
		[self setQTMovie:[[QTMovie alloc]
					    initWithQuickTimeMovie:theMovie
					    disposeWhenDone:FALSE error:&nsErr]
		];
		success = (nsErr == nil && qtMovie != nil);
		// release NSErr?
	}

	return success;
}

- (id)initWithContentsOfURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError
{
	return [self initWithContentsOfURL:absoluteURL ofType:typeName];
}

/*!
	stub function that is likely to be called through Cocoa when NSQTMovieWindow is registered as
	an application's main class. QTils is not intended to open windows through the regular Mac OS X
	mechanism so we only use this function to record the arguments we're called with when launched
	through the Finder.
 */
- (id)initWithContentsOfURL:(NSURL *)absoluteURL ofType:(NSString *)typeName
{ NSWindow *nswin;
// 	self = [super initWithType:@"MovieDocument" error:outError];
	self = [super init];
	if( movieView && (nswin = [movieView window]) ){
		[nswin performClose:nswin];
	}
	[self close];
	if( [QTilsDelegate isLaunched] ){
	 QTMovieWindowH wih;
		NSLog( @"[%@ %@] URL=%@; not modifying ArgArray of already launched application",
			 NSStringFromClass([self class]), NSStringFromSelector(_cmd), absoluteURL );
		if( OpenMenuHandler ){
			wih = (*OpenMenuHandler)([[[absoluteURL path] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding]
								 cStringUsingEncoding:NSUTF8StringEncoding]);
			if( QTMovieWindowH_Check(wih) ){
				// ensure that our new window is the active one, as this isn't necessarily the case:
				[(*wih)->theView makeKeyWindow];
			}
		}
	}
	else{
		if( absoluteURL && QTilsDelegate ){
			[[QTilsDelegate ArgArray] addObject:[[absoluteURL path] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
		}
	}
	return self;
}

/*!
	The functional initWithContentsOfURL method, only called when opening a movie through one of QTils'
	open functions
 */
- (id)internalInitWithContentsOfURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError
{
//	[super initWithContentsOfURL:absoluteURL ofType:typeName error:outError];
	self = [super initWithType:@"MovieDocument" error:outError];
	if( self ){
		URL = [absoluteURL retain];
		// the actual setting of the movie is already done explicitly by our caller
// 		[self readFromURL:absoluteURL ofType:typeName];
	}
	return self;
}

- (id) initWithQTWM:(struct QTMovieWindows**)wih
{ QTMovieWindows *wi = *wih;
  NSRect mrect;
  NSString *URLString;
  NSURL *theURL;
  NSError *err = nil;
	// load up the nib - done for us by [self init] called through initWithContentsOfURL -> initWithType !
//	if( self /* && !movieView */ ){
//		if( ![NSBundle loadNibNamed:@"NSQTMovieWindow" owner:self] ){
//			NSLog( @"%@ failed to load 'NSQTMovieWindow' nib!", self );
//		}
//	}
	URLString = [NSString stringWithCString:wi->theURL encoding:NSUTF8StringEncoding];
	if( !URLString ){
		URLString = [NSString stringWithCString:wi->theURL
						encoding:CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingDOSLatin1)];
	}
	if( !URLString ){
		NSLog( @"[%@ %@] couldn't create NSString from C string \"%s\"\n",
			 NSStringFromClass([self class]), NSStringFromSelector(_cmd), wi->theURL );
		return nil;
	}
	if( strncasecmp( wi->theURL, "file://", 7 ) && strncasecmp( wi->theURL, "http://", 7) ){
	  NSString *fileURL = @"file://";
		URLString = [fileURL stringByAppendingString:URLString];
	}
	theURL = [NSURL URLWithString:URLString];
	self = [self internalInitWithContentsOfURL:theURL ofType:@"MovieDocument" error:&err];
	// make the WindowControllers and show the windows before installing the Movie into
	// the movieView (setQTMovieFromMovie). If done afterwards, one can easily end up in
	// endless loops of mcActionControllerSizeChanged MC Actions.
	if( self ){
		[self makeWindowControllers];
		[self showWindows];
		[self setQTMovieFromMovie:wi->theMovie];
		[[movieView window] setReleasedWhenClosed:YES];
	}
// not necessary; one can just define the appropriate selector functions (windowWillClose etc)...
//	[[movieView window] setDelegate:[[[QTMovieWindowDelegate alloc] init] autorelease]];
	if( self && movieView ){
		mrect.size = [self windowContentSizeForMovieSize:[self moviePreferredSize]];
		[[movieView window] setContentSize:mrect.size];
		qtmwH = wih;
		[[movieView window] makeMainWindow];
		return self;
	}
	else{
		return nil;
	}
}

#define MCHEIGHT(mv)	[mv controllerBarHeight]

- (NSSize)movieSizeForWindowSize:(NSSize)s
{ NSWindow *window = [movieView window];
  NSRect windowRect = [window frame];

	windowRect.size = s;
	return [window contentRectForFrameRect:windowRect].size;
}

- (NSSize)windowSizeForMovieSize:(NSSize)s
{ NSWindow *window = [movieView window];
	if( [movieView isControllerVisible] ){
		s.height += MCHEIGHT(movieView);
	}

	NSRect contentRect = [[window contentView] bounds];

	contentRect.size = NSMakeSize(s.width, s.height);

	return [window frameRectForContentRect:contentRect].size;
}

- (NSRect)windowRectForMovieRect:(NSRect)r
{ NSWindow *window = [movieView window];
  NSRect viewRect = [movieView bounds];
  NSSize viewSize = viewRect.size;
	if( [movieView isControllerVisible] ){
		viewSize.height += MCHEIGHT(movieView);
	}

	NSRect contentRect = [[window contentView] bounds];
	NSSize curWindowSize = contentRect.size;

	CGFloat dx = curWindowSize.width - viewSize.width;
	CGFloat dy = curWindowSize.height - viewSize.height;

	contentRect.origin = r.origin;
	contentRect.size.width = r.size.width + dx;
	contentRect.size.height = r.size.height + dy;

	return [window frameRectForContentRect:contentRect];
}

- (NSSize) windowContentSizeForMovieSize:(NSSize)s
{ NSSize contentSize = s;
	if( [movieView isControllerVisible] ){
		contentSize.height += MCHEIGHT(movieView);
	}
	if( !contentSize.width ){
		contentSize.width = [[movieView window] frame].size.width;
	}
	return contentSize;
}

- (void) resizeWindowOnSpotWithFrameRect:(NSRect)aRect
{ NSWindow *nswin = [movieView window];
  NSRect r, curRect = [nswin frame];
	r = NSMakeRect(
		curRect.origin.x - (aRect.size.width - curRect.size.width),
		curRect.origin.y - (aRect.size.height - curRect.size.height),
		aRect.size.width, aRect.size.height );
    [nswin setFrame:r display:YES animate:NO];
}

- (void) resizeWindowOnSpotWithIFrameRect:(NSRect)aRect
{ NSWindow *nswin = [movieView window];
  NSRect r, curRect = [nswin frame];
	r = NSMakeRect(
		curRect.origin.x - (aRect.size.width - curRect.size.width),
		curRect.origin.y,
		aRect.size.width, aRect.size.height );
    [nswin setIFrame:r display:YES animate:NO];
}

- (NSSize)movieUnsizedSize
{
	return NSMakeSize(300, 0);
}

- (NSSize) moviePreferredSize
{ NSSize size;
  NSValue *sizeObj;
	if( qtMovie ){
		sizeObj = [[qtMovie movieAttributes] objectForKey:QTMovieNaturalSizeAttribute];
	}
	else{
		sizeObj = nil;
	}
	if( sizeObj ){
		size = [sizeObj sizeValue];
	}
	if( !sizeObj || size.height < 2 ){
		size = [self movieUnsizedSize];
	}
	return size;
}

- (NSSize)movieCurrentSize
{
#if 0
	QTMovie *m = [movieView movie];
	NSSize size;
	NSValue *sizeObj = [[m movieAttributes] objectForKey:QTMovieNaturalSizeAttribute];
	if( sizeObj ){
		size = [sizeObj sizeValue];
	}
	if( !sizeObj || size.height < 2 ){
		size = [self movieUnsizedSize];
	}
	return size;
#else
#	ifdef DEBUG
	NSWindow *w = [movieView window];
	NSRect cbounds = [w contentRectForFrameRect:[w iFrame]];
	Log( qtLogPtr, "%@ current size: %gx%g / %gx%g", [[URL path] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding],
		 movieSize.width, movieSize.height,
		 cbounds.size.width, cbounds.size.height );
#	endif
	return movieSize;
#endif
}

- (void)setMovieSize:(NSSize)size
{
	if( qtMovie ){
//		NSLog( @"setMovieSize %@ to %fx%f", qtMovie, size.width, size.height );
		movieSize = size;
		[qtMovie setMovieAttributes:[NSDictionary dictionaryWithObject:[NSValue valueWithSize:size]
				forKey:QTMovieNaturalSizeAttribute]
		];
	}
}

- (void)setMovieSize:(NSSize)size caller:(const char*)caller
{
	if( qtMovie ){
		Log( qtLogPtr, "setMovieSize(%s) %@ to %fx%f", caller, [[URL path] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding], size.width, size.height );
		movieSize = size;
		[qtMovie setMovieAttributes:[NSDictionary dictionaryWithObject:[NSValue valueWithSize:size]
				forKey:QTMovieNaturalSizeAttribute]
		];
	}
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

- (void) ToggleMCController
{
#if 0
  NSRect winRect = [[movieView window] frame];
  NSSize winSize;
  float dy;
  BOOL vis = ![movieView isControllerVisible];
	[movieView setControllerVisible:vis];
	winSize = [self windowSizeForMovieSize:[self movieCurrentSize]];
	dy = winSize.height - winRect.size.height;
	winRect.origin.y -= dy;
	winRect.size = winSize;
	[[movieView window] setFrame:winRect display:YES animate:NO];
#else
  NSRect winRect = [[movieView window] iFrame];
  BOOL vis = ![movieView isControllerVisible];
	[movieView setControllerVisible:vis];
	winRect.size = [self windowSizeForMovieSize:[self movieCurrentSize]];
	[[movieView window] setIFrame:winRect display:YES animate:NO];
#endif
}

- (void) setOneToOne:sender
{ NSSize size = [self moviePreferredSize];
  NSSize winSize = [self windowContentSizeForMovieSize:size];
  NSWindow *window = [movieView window];
  NSRect winRect = [window frame];
  float dy = winSize.height - winRect.size.height;
	winRect.origin.y -= dy;
	winRect.size = winSize;
	[window setFrame:winRect display:NO animate:NO];
	[window setContentSize:winSize];
	[self setMovieSize:size caller:"setOneToOne"];
}

- (void) setQTMovie:(QTMovie*) m
{ QTMovie *old = qtMovie;
  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
  Boolean nilMovie = FALSE;
	if( old ){
		[center removeObserver:self name:QTMovieDidEndNotification object:old];
		[center removeObserver:self name:QTMovieSizeDidChangeNotification object:old];
	}
	if( m == nil ){
		m = [QTMovie movieNamed:@"empty.mov" error:nil];
		nilMovie = TRUE;
	}
	else{
		// temporarily set the view's movie to nil so the current movie has more chances
		// of being closed properly
		[movieView setMovie:nil];
	}
	if( m != nil ){
		qtMovie = [m retain];
		[qtMovie setAttribute:[NSNumber numberWithBool:NO] forKey:QTMovieEditableAttribute];
	}
	else{
		qtMovie = nil;
	}
	[movieView setMovie:qtMovie];
	if( !nilMovie ){
		[self setOneToOne:self];
		resizesVertically = ([self moviePreferredSize].height >= 2);
		[[movieView window] setShowsResizeIndicator:NO];
		[movieView setShowsResizeIndicator:YES];
		[center addObserver:self selector:@selector(movieEnded:)
					name:QTMovieDidEndNotification object:qtMovie];
		[center addObserver:self selector:@selector(boundsDidChange:)
					name:QTMovieSizeDidChangeNotification object:qtMovie];
	}
	if( old ){
		[old release];
	}
}

- (QTMovie*) theQTMovie
{
	return qtMovie;
}

- (QTMovieView*) theMovieView
{
	return movieView;
}

- (NSWindow*) theWindow
{
	return [movieView window];
}

- (struct QTMovieWindows**) theQTMovieWindowH
{
	return qtmwH;
}

- (void) setTitleWithCString:(const char*)title
{ NSString *t = [NSString stringWithCString:title encoding:NSUTF8StringEncoding];
	if( !t ){
		t = [NSString stringWithCString:title
					encoding:CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingDOSLatin1)];
	}
	if( t ){
		[[movieView window] setTitle:t];
	}
	else{
		NSLog( @"[%@ %@] couldn't create NSString from C string \"%s\"\n",
			 NSStringFromClass([self class]), NSStringFromSelector(_cmd), title );
	}
}

- (void) movieEnded:(NSNotification *)notification
{
	Log( qtLogPtr, "[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), notification );
}

- (void) boundsDidChange:(NSQTMovieWindow*)sender
{
//	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
}

- (void) prepareForRelease
{
	qtmwH = NULL;
}

- (NSString*) description
{
	return [[NSString alloc] initWithFormat:@"<%@ with movie %@>", NSStringFromClass([self class]), qtMovie];
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

- (BOOL) windowShouldClose:(id)sender
{
	if( QTMovieWindowH_Check(qtmwH) ){
	  int (*fun)(QTMovieWindowH wi, void *params) = NULL;
	  id target;
	  SEL selector;
	  NSMCActionCallback nsfun = NULL;
		Log( qtLogPtr, "Close QT window %@ = #%u\n", (NSWindow*) sender, (*qtmwH)->idx );
		(*qtmwH)->shouldClose = TRUE;
		(*qtmwH)->performingClose = TRUE;
		// 20110310: stop the idling, that is the movie should no longer be tasked!
		[[(*qtmwH)->theMovieView movie] setIdling:NO];
		if( (nsfun = get_NSMCAction( qtmwH, _MCAction_.Close, &target, &selector )) ){
			(*nsfun)( target, selector, qtmwH, NULL );
		}
		else if( (fun = get_MCAction( qtmwH, _MCAction_.Close )) ){
			(*fun)( qtmwH, NULL );
		}
		// we shouldn't call CloseQTMovieWindow() with a valid NSWindow in wi->theView
		// as it would call the performClose method ... which is already being executed.
		CloseQTMovieWindow(qtmwH);
		(*qtmwH)->performingClose = FALSE;
	}
	return YES;
}

- (void) windowWillClose:(NSNotification*)notification
{ NSWindow *nswin = [notification object];
	Log( qtLogPtr, "[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), nswin );
	if( QTMovieWindowH_Check(qtmwH) ){
		unregister_QTMovieWindowH_from_NativeWindow((*qtmwH)->theView);
		(*qtmwH)->theView = NULL;
	}
}

// added automatically by "Convert to Objective-C 2.0"
@synthesize movieView;
@synthesize URL;
@synthesize qtMovie;
@synthesize qtmwH;
@synthesize resizesVertically;
@end

@implementation QTMovieWindowDelegate

extern QTMovieWindowH QTMovieWindowHFromNativeWindow( NativeWindow hWnd );

- (BOOL) windowShouldClose:(id)sender
{ QTMovieWindowH wi;
	wi = QTMovieWindowHFromNativeWindow((NativeWindow)sender);
	if( QTMovieWindowH_Check(wi) ){
	  int (*fun)(QTMovieWindowH wi, void *params) = NULL;
	  id target;
	  SEL selector;
	  NSMCActionCallback nsfun = NULL;
		Log( qtLogPtr, "Close QT window %@ = #%u\n", (NSWindow*) sender, (*wi)->idx );
		(*wi)->shouldClose = TRUE;
		(*wi)->performingClose = TRUE;
		// 20110310: stop the idling, that is the movie should no longer be tasked!
		[[(*wi)->theMovieView movie] setIdling:NO];
		if( (nsfun = get_NSMCAction( wi, _MCAction_.Close, &target, &selector )) ){
			(*nsfun)( target, selector, wi, NULL );
		}
		else if( (fun = get_MCAction( wi, _MCAction_.Close )) ){
			(*fun)( wi, NULL );
		}
		// we shouldn't call CloseQTMovieWindow() with a valid NSWindow in wi->theView
		// as it would call the performClose method ... which is already being executed.
		CloseQTMovieWindow(wi);
		(*wi)->performingClose = FALSE;
	}
	return YES;
}

- (void) windowWillClose:(NSNotification*)notification
{ NSWindow *nswin = [notification object];
  QTMovieWindowH wi;
	wi = QTMovieWindowHFromNativeWindow((NativeWindow)nswin);
	Log( qtLogPtr, "[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), nswin );
	if( QTMovieWindowH_Check(wi) ){
		unregister_QTMovieWindowH_from_NativeWindow((*wi)->theView);
		(*wi)->theView = NULL;
	}
}

@end

@implementation NSWindow (description)

- (NSString*) description
{ NSRect frame = [self frame];
  QTMovieWindowH wih = QTMovieWindowHFromNativeWindow(self);
	if( wih ){
	  char *valid = (QTMovieWindowH_Check(wih))? "valid" : "invalid";
		return [[NSString alloc]
			   initWithFormat:@"<%@ %p %s QTMovieWindow %p '%@' %gx%g+%g+%g>",
			   NSStringFromClass([self class]), self,
			   valid, wih,
			   [self title], frame.size.width, frame.size.height,
			   frame.origin.x, frame.origin.y];
	}
	else{
		return [[NSString alloc]
			   initWithFormat:@"<%@ %p '%@' %gx%g+%g+%g>", NSStringFromClass([self class]), self,
			   [self title], frame.size.width, frame.size.height,
			   frame.origin.x, frame.origin.y];
	}
}

- (void) showInfo:(id) sender
{
	Log( qtLogPtr, "[%@ %@] window=%@",
		 NSStringFromClass([self class]), NSStringFromSelector(_cmd), self );
	if( InfoMenuHandler ){
		(*InfoMenuHandler)(QTMovieWindowHFromNativeWindow(self));
	}
}

@end

int QTils_GetArgc()
{
	if( QTilsDelegate ){
		return [[QTilsDelegate ArgArray] count] + 1;
	}
	else{
		return 1;
	}
}

char **QTils_GetArgv( char **org_argv[] )
{ char **argv = NULL;
  int i;

	if( !org_argv ){
		return NULL;
	}

	if( QTilsDelegate && [[QTilsDelegate ArgArray] count] > 0 ){
		argv = (char **) calloc( [[QTilsDelegate ArgArray] count] + 2, sizeof(char*) );
		argv[0] = (*org_argv)[0];
		*org_argv = argv;
		for( i = 0 ; i < [[QTilsDelegate ArgArray] count] ; ++i ){
		  NSString *path = [[QTilsDelegate ArgArray] objectAtIndex:i];
			argv[i+1] = (char*) [path cStringUsingEncoding:NSUTF8StringEncoding];
		}
		// add the terminating NULL pointer
		argv[i+1] = NULL;
	}
	else{
		argv = (char **) calloc( 2, sizeof(char*) );
		argv[0] = (*org_argv)[0];
		// add the terminating NULL pointer
		argv[1] = NULL;
	}
	return argv;
}

@implementation QTilsApplicationDelegate
- (id) init
{
	self = [super init];
	if( self ){
		relaunch = NO;
		ArgArray = [[NSMutableArray alloc] initWithCapacity:1];
	}
	return self;
}

- (void) dealloc
{
	[ArgArray release];
	[super dealloc];
}

- (BOOL) application:(NSApplication *)theApplication openFile:(NSString *)filename
{
// 	NSLog( @"[%@ %@] URL=%@",
// 		 NSStringFromClass([self class]), NSStringFromSelector(_cmd), filename );
	if( !ArgArray ){
		ArgArray = [[[NSMutableArray alloc] initWithCapacity:1] retain];
	}
	[ArgArray addObject:filename];
	return YES;
}

- (BOOL) application:(NSApplication *)theApplication openFileWithoutUI:(NSString *)filename
{
	NSLog( @"[%@ %@] URL=%@",
		 NSStringFromClass([self class]), NSStringFromSelector(_cmd), filename );
	if( !ArgArray ){
		ArgArray = [[[NSMutableArray alloc] initWithCapacity:1] retain];
	}
	[ArgArray addObject:filename];
	return YES;
}

- (void) application:(NSApplication *)theApplication openFiles:(NSArray *)filenames
{
// 	NSLog( @"[%@ %@] URLs=%@",
// 		 NSStringFromClass([self class]), NSStringFromSelector(_cmd), filenames );
	if( !ArgArray ){
		ArgArray = [[[NSMutableArray alloc] initWithCapacity:[filenames count]] retain];
	}
	[ArgArray addObjectsFromArray:filenames];
}

- (BOOL) isState:(NSString*)key
{ NSThread *theThread = [NSThread mainThread];
  NSNumber *val = [[theThread threadDictionary] valueForKey:key];
	if( val && [val boolValue] ){
		return YES;
	}
	else{
		return NO;
	}
}

- (BOOL) setState:(BOOL)state forKey:(NSString*)key
{ NSThread *theThread = [NSThread mainThread];
  BOOL prev = [[[theThread threadDictionary] valueForKey:key] boolValue];
	[[theThread threadDictionary] setValue:[NSNumber numberWithBool:state] forKey:key];
	return prev;
}

- (BOOL) isRestarted
{
	return [self isState:@"isRestarted"];
}

- (BOOL) setRestarted:(BOOL)state
{
	return [self setState:state forKey:@"isRestarted"];
}

- (BOOL) isLaunched
// { NSThread *theThread = [NSThread mainThread];
//   NSNumber *val = [[theThread threadDictionary] valueForKey:@"isLaunched"];
// 	if( val && [val boolValue] ){
// 		return YES;
// 	}
// 	else{
// 		return NO;
// 	}
// }
{
	return [self isState:@"isLaunched"];
}

- (BOOL) setLaunched:(BOOL)state
// { NSThread *theThread = [NSThread mainThread];
//   BOOL prev = [self isLaunched];
// 	[[theThread threadDictionary] setValue:[NSNumber numberWithBool:state] forKey:@"isLaunched"];
// 	return prev;
// }
{
	return [self setState:state forKey:@"isLaunched"];
}

/*!
	a pointer to the owning process's main function - required since apparently we cannot
	obtain its address via dlsym?!
 */
static int (*mainFunction)(int, char**) = NULL;

- (void) applicationDidFinishLaunching:(NSNotification *)aNotification
{
	[self setLaunched:YES];
	if( relaunch && [ArgArray count] >= 0 ){
	  char **org_argv = *_NSGetArgv(), **argv;
	  int ret;
		argv = QTils_GetArgv(&org_argv);
		[self setRestarted:YES];
		if( mainFunction ){
			// we simply invoke main() once again, this time with the arguments we determined
// 			NSLog( @"[%@ %@] relaunching main with %s and %@",
// 				 NSStringFromClass([self class]), NSStringFromSelector(_cmd),
// 				 argv[0], ArgArray );
			// call the main function specified by the caller, and pass its return
			// value to exit() to ensure that we exit after main returns.
			ret = (*mainFunction)( QTils_GetArgc(), argv );
		}
		else{
			NSLog( @"[%@ %@] re-execv with %s and %@",
				 NSStringFromClass([self class]), NSStringFromSelector(_cmd),
				 argv[0], ArgArray );
			ret = execv( argv[0], argv );
			NSLog( @"execv returned %d, this shouldn't happen!" );
		}
		exit(ret);
	}
}

@synthesize relaunch;
@synthesize ArgArray;
@end

@implementation QTilsMenuDelegate
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	NSLog( @"[%@ %@] URL=%@",
		 NSStringFromClass([self class]), NSStringFromSelector(_cmd), filename );
	[[QTilsDelegate ArgArray] addObject:filename];
	return YES;
}

- (void) application:(NSApplication *)theApplication openFiles:(NSArray *)filenames
{
	NSLog( @"[%@ %@] URLs=%@",
		 NSStringFromClass([self class]), NSStringFromSelector(_cmd), filenames );
	[[QTilsDelegate ArgArray] addObjectsFromArray:filenames];
}

@end

int QTils_ApplicationLoad()
{ BOOL ret;
	if( ![QTilsDelegate isRestarted] ){
		[QTilsDelegate setLaunched:NO];
		ret = NSApplicationLoad();
		if( ret ){
			QTilsDelegate = [[[QTilsApplicationDelegate alloc] init] autorelease];
			[[NSApplication sharedApplication] setDelegate:QTilsDelegate ];
			ret = [NSBundle loadNibNamed:@"QTils-MainMenu" owner:[NSApplication sharedApplication]];
		}
		if( ret ){
			[[[NSApplication sharedApplication] mainMenu] setDelegate:[[[QTilsMenuDelegate alloc] init] autorelease] ];
			PumpMessages(0);
		}
	}
	else{
// 		NSLog( @"QTils_ApplicationLoad(): restarted=%@, skipping NSApplicationLoad()", restarted );
		ret = YES;
	}
	return ret;
}

int _QTils_ApplicationMain_( int *argc, char **argv[], int (*main)(int, char**) )
{ //NSThread *theThread = [NSThread mainThread];
// 	NSLog( @"QTils_ApplicationMain(%d): mainThread=%@/%p, dict=%@",
// 		 argc, theThread, pthread_self(), [theThread threadDictionary] );
	if( ![[[NSThread mainThread] threadDictionary] valueForKey:@"isRestarted"] ){
		[QTilsDelegate setRestarted:NO];
	}
	if( *argc <= 0 || !*argv || (*argc > 1 && strncasecmp( (*argv)[1], "-psn_", 5 ) == 0) ){
	  // use the cocoa approach
		QTilsDelegate = [[[QTilsApplicationDelegate alloc] init] autorelease];
		[QTilsDelegate setRelaunch:YES];
		[[NSApplication sharedApplication] setDelegate:QTilsDelegate ];
		[[[NSApplication sharedApplication] mainMenu] setDelegate:[[[QTilsMenuDelegate alloc] init] autorelease] ];
		mainFunction = main;
		[QTilsDelegate setLaunched:NO];
		return NSApplicationMain( *argc, (const char**) *argv );
	}
	else{
		return QTils_ApplicationLoad();
	}
}

__attribute__((destructor))
static void finaliser()
{
}