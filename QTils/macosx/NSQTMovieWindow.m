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


@implementation NSQTMovieWindow

- (void) dealloc
{
	Log( qtLogPtr, "[%@ dealloc]", self );
	[self setQTMovie:nil];
	[URL release];
	[super dealloc];
}

- (void) close
{
	NSLog( @"[%@ %@]\n", NSStringFromClass([self class]), NSStringFromSelector(_cmd) );
	[super close];
}

- (BOOL)readFromURL:(NSURL *)url ofType:(NSString *)type
{ BOOL success = NO;

	// read the movie
	if( [QTMovie canInitWithURL:url] ){
		[self setQTMovie:((QTMovie *)[QTMovie movieWithURL:url error:nil])];
		success = (qtMovie != nil);
	}

	return success;
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
	}

	return success;
}

- (id)initWithContentsOfURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError
{
//	[super initWithContentsOfURL:absoluteURL ofType:typeName error:outError];
	self = [super initWithType:@"MovieDocument" error:outError];
	if( self ){
		URL = [absoluteURL retain];
	}
	return self;
}

- (id) initWithQTWMandPool:(struct QTMovieWindows**)wih
{ QTMovieWindows *wi = *wih;
  NSRect mrect;
  NSString *URLString;
  NSURL *theURL;
  NSError *err = nil;
	// load up the nib
	if( self /* && !movieView */ ){
		if( ![NSBundle loadNibNamed:@"NSQTMovieWindow" owner:self] ){
			NSLog( @"%@ failed to load 'NSQTMovieWindow' nib!", self );
		}
	}
	URLString = [NSString stringWithCString:wi->theURL encoding:NSUTF8StringEncoding];
	if( strncasecmp( wi->theURL, "file://", 7 ) && strncasecmp( wi->theURL, "http://", 7) ){
	  NSString *fileURL = @"file://";
		URLString = [fileURL stringByAppendingString:URLString];
	}
	theURL = [NSURL URLWithString:URLString];
	self = [self initWithContentsOfURL:theURL ofType:@"MovieDocument" error:&err];
	// make the WindowControllers and show the windows before installing the Movie into
	// the movieView (setQTMovieFromMovie). If done afterwards, one can easily end up in
	// endless loops of mcActionControllerSizeChanged MC Actions.
	[self makeWindowControllers];
	[self showWindows];
	[self setQTMovieFromMovie:wi->theMovie];
	[[movieView window] setReleasedWhenClosed:YES];
// not necessary; one can just define the appropriate selector functions (windowWillClose etc)...
//	[[movieView window] setDelegate:[[[QTMovieWindowDelegate alloc] init] autorelease]];
	if( movieView ){
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
	Log( qtLogPtr, "%@ current size: %gx%g / %gx%g", [URL path],
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
		NSLog( @"setMovieSize(%s) %@ to %fx%f", caller, [URL path], size.width, size.height );
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
{
	[[movieView window] setTitle:[NSString stringWithCString:title encoding:NSUTF8StringEncoding]];
}

- (void) movieEnded:(NSNotification *)notification
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), notification );
}

- (void) boundsDidChange:(NSQTMovieWindow*)sender
{
//	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
}

- (void) prepareForRelease
{
	qtmwH = NULL;
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
		NSLog( @"Close QT window %@ = #%u\n", (NSWindow*) sender, (*qtmwH)->idx );
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
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), nswin );
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
		NSLog( @"Close QT window %@ = #%u\n", (NSWindow*) sender, (*wi)->idx );
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
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), nswin );
	if( QTMovieWindowH_Check(wi) ){
		unregister_QTMovieWindowH_from_NativeWindow((*wi)->theView);
		(*wi)->theView = NULL;
	}
}

@end

