//!
//  @file NSQTWindow.h
//  QTilities
//
//  Created by René J.V. Bertin on 20110909.
//  Copyright 2011 INRETS/LCPC — LEPSIS. All rights reserved.

//  Adds a few methods to the NSWindow class
//

#import <Cocoa/Cocoa.h>
#import <QTKit/QTKit.h>


@interface NSWindow (convertFrameULtofromBL)
/*!
	convert a window frame rect between the Mac's bottom-left (BL) based co-ordinate scheme
	to the more usual upper-left (UL) co-ordinate system.
 */
- (NSRect) convertFrameULtofromBL:(NSRect)iframe useVisible:(BOOL)uv;
@end

@interface NSWindow (convertContentULtofromBL)
/*!
	convert a window's content rect between the Mac's bottom-left (BL) based co-ordinate scheme
	to the more usual upper-left (UL) co-ordinate system.
 */
- (NSRect) convertContentULtofromBL:(NSRect)iframe;
@end

@interface NSWindow (iFrame)
- (NSRect) iFrame;
@end

@interface NSWindow (setIFrame)
/*!
	change a window's frame rect for one in the UL co-ordinate system
 */
- (void)setIFrame:(NSRect)windowFrame display:(BOOL)displayViews;
- (void)setIFrame:(NSRect)windowFrame display:(BOOL)displayViews animate:(BOOL)performAnimation;
@end

@interface NSWindow (iFrameRectForContentIRect)
/*!
	get the window's frame rect that holds or would hold the given content rect. Both
	rects are in the UL co-ordinate system.
 */
- (NSRect) iFrameRectForContentIRect:(NSRect)iRect;
@end

@interface NSWindow (titleBarHeight)
- (float) titleBarHeight;
@end
