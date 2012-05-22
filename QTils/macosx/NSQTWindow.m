//!
//  @file NSQTWindow.m
//  QTilities
//
//  Created by René J.V. Bertin on 20110909.
//  Copyright 2011 INRETS/LCPC — LEPSIS. All rights reserved.
//

#import "NSQTMovieWindow.h"
#import "NSQTWindow.h"


@implementation NSWindow (convertFrameULtofromBL)

/*!
	convert a window frame rect between the Mac's bottom-left (BL) based co-ordinate scheme
	to the more usual upper-left (UL) co-ordinate system.
 */
- (NSRect) convertFrameULtofromBL:(NSRect)iframe useVisible:(BOOL)uv
{
	// we seem to obtain the window origin as if the menubar isn't there - but if it IS there,
	// the actual window position is different. Thus, we need to use [screen frame] and not [screen visibleFrame].
	// To calculate an actual new (BL) origin based on a given (TL) origin, we will use visibleFrame, such that
	// y==0 will be at the 1st visible screen pixel.
  NSRect sframe = (uv)? [[self screen] visibleFrame] : [[self screen] frame];
	iframe.origin.y = sframe.origin.y + sframe.size.height - (iframe.origin.y + iframe.size.height + [self titleBarHeight]);
#ifdef DEBUG0
  NSRect wframe = [self frame];
	NSLog( @"%@ origin (%g,%g) screen (%gx%g+%g+%g) -> (%g,%g)", self,
		 wframe.origin.x, wframe.origin.y,
		 sframe.size.width, sframe.size.height, sframe.origin.x, sframe.origin.y,
		 iframe.origin.x, iframe.origin.y );
#endif
	return iframe;
}

@end

@implementation NSWindow (convertContentULtofromBL)

/*!
	convert a window's content rect between the Mac's bottom-left (BL) based co-ordinate scheme
	to the more usual upper-left (UL) co-ordinate system.
 */
- (NSRect) convertContentULtofromBL:(NSRect)iframe
{
	iframe.origin.y = iframe.size.height - iframe.origin.y;
	return iframe;
}

@end

@implementation NSWindow (iFrame)
/*
	get a window's frame rect in the UL co-ordinate system
 */
- (NSRect) iFrame
{ NSRect iframe = [self convertFrameULtofromBL:[self frame] useVisible:NO];
	return iframe;
}

@end

@implementation NSWindow (setIFrame)

/*!
	change a window's frame rect for one in the UL co-ordinate system
 */
- (void)setIFrame:(NSRect)windowFrame display:(BOOL)displayViews
{
	return [self setFrame:[self convertFrameULtofromBL:windowFrame useVisible:NO] display:displayViews];
}

- (void)setIFrame:(NSRect)windowFrame display:(BOOL)displayViews animate:(BOOL)performAnimation
{
	return [self setFrame:[self convertFrameULtofromBL:windowFrame useVisible:NO] display:displayViews animate:performAnimation];
}

@end

@implementation NSWindow (iFrameRectForContentIRect)

/*!
	get the window's frame rect that holds or would hold the given content rect. Both
	rects are in the UL co-ordinate system.
 */
- (NSRect) iFrameRectForContentIRect:(NSRect)iRect
{ NSRect iframe = [self frameRectForContentRect:iRect];
	// iframe will have the same origin as cbounds (still in our flipped co-ordinate system). We'll
	// have to make sure the content's upper-left corner ends up at that location, and not the windows
	// UL corner:
	iframe.origin.x += (iRect.size.width - iframe.size.width);
	iframe.origin.y += (iRect.size.height - iframe.size.height);
	return iframe;
}

@end

@implementation NSWindow (titleBarHeight)

- (float) titleBarHeight
{ NSRect frame = NSMakeRect (0, 0, 100, 100);
  NSRect contentRect;
    contentRect = [self contentRectForFrameRect:frame];

    return (frame.size.height - contentRect.size.height);

} // titleBarHeight

@end
