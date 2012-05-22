//
//  VDPreferences.m
//
//  Created by René J.V. Bertin on 20110915.
//  Copyright 2011 INRETS/LCPC — LEPSIS. All rights reserved.
//

#import "QTVOD.h"

static void NSnoLog( NSString *format, ... )
{
	return;
}

static void doNSLog( NSString *format, ... )
{ va_list ap;
	va_start(ap, format);
	NSLogv( format, ap );
	va_end(ap);
	return;
}

#ifndef DEBUG
#	define NSLog	NSnoLog
#endif

VODDescription globalVDPreferences = {
	12.5, 1.0, 1.0,
	FALSE, TRUE, FALSE, TRUE,
	{ 1, 2, 3, 4}
};

static VDPreferencesController *VDPrefsWin = NULL;

static char *channelName(int channel)
{ char *chName = "illegal";
	switch( channel ){
		case fwWin:
			chName = "Forward";
			break;
		case pilotWin:
			chName = "Pilot";
			break;
		case leftWin:
			chName = "Left";
			break;
		case rightWin:
			chName = "Right";
			break;
		case tcWin:
			chName = "TC";
			break;
	}
	return chName;
}

// intercept the "shouldClose" notification, so that we can handle any changes to the settings:
@implementation NSWindow (VPWinShouldClose)
- (BOOL) windowShouldClose:(id)sender
{

	if( VDPrefsWin && ([VDPrefsWin window] == self) ){
		if( globalVDPreferences.changed ){
			[VDPrefsWin savePreferences:NULL];
		}
		globalVDPreferences.changed = NO;
		VDPrefsWin = NULL;
		NSLog( @"[%@ %@%@] : closing prefs window",
			 NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender
		);
	}
	else{
		NSLog( @"[%@ %@%@]",
			 NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender
		);
	}

	return YES;
}
@end

@implementation NSAMatrix
@synthesize previousCell;
@end

@implementation VDPreferencesController

- (id) init
{
//	NSLog( @"[%@ %@] - [super initWithWindowNibName...]", NSStringFromClass([self class]), NSStringFromSelector(_cmd) );
	self = [super initWithWindowNibName:@"VDPreferences"];
	return self;
}

- (void)dealloc
{
//	NSLog( @"[%@ %@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd) );
	[super dealloc];
}

- (IBAction) showWindow:sender
{
	[super showWindow:sender];
//	NSLog( @"[%@ %@%@] -> window=%@",
//		 NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender,
//		 [self window]
//	);
	[[self window] setWindowController:self];
	[[self window] orderFront:sender];
	[self update:YES];
	VDPrefsWin = self;
}

- (void)windowDidLoad
{
//	NSLog( @"[%@ %@] -> window=%@",
//		 NSStringFromClass([self class]), NSStringFromSelector(_cmd),
//		 [self window]
//	);
	[super windowDidLoad];
	[self update:YES];
}

- (void)windowDidUpdate:(NSNotification *)dummy
{
	NSLog( @"windowDidUpdate");
	[self update:YES];
}

// 20110916: we ought to be using a string representation instead of setting/getting
// double NSNumbers directly...
- (void) setComboBox:(NSComboBox*)it toDoubleVal:(double)val
{ NSNumber *nval;
  NSInteger idx;
	nval = [NSNumber numberWithDouble:val];
	idx = [it indexOfItemWithObjectValue:nval];
	if( idx == NSNotFound ){
		[it addItemWithObjectValue:nval];
		idx = [it indexOfItemWithObjectValue:nval];
	}
	if( idx != NSNotFound ){
		[it selectItemAtIndex:idx];
	}
}

- (void) setMatrixForQuadrant:(int)qd toChannel:(int)channel
{
	switch( qd ){
		case 1:
			[ch1Matrix selectCellWithTag:channel];
			NSLog( @"setMatrixForQuadrant:%d toChannel:%s]", qd, channelName(channel) );
			break;
		case 2:
			[ch2Matrix selectCellWithTag:channel];
			NSLog( @"setMatrixForQuadrant:%d toChannel:%s]", qd, channelName(channel) );
			break;
		case 3:
			[ch3Matrix selectCellWithTag:channel];
			NSLog( @"setMatrixForQuadrant:%d toChannel:%s]", qd, channelName(channel) );
			break;
		case 4:
			[ch4Matrix selectCellWithTag:channel];
			NSLog( @"setMatrixForQuadrant:%d toChannel:%s]", qd, channelName(channel) );
			break;
		default:
			doNSLog( @"Illegal quadrant #%d in [%@ %@%d]", qd,
				 NSStringFromClass([self class]), NSStringFromSelector(_cmd), channel
			);
			break;
	}
}

- (void) update:(BOOL)updateChannelDisplay
{ NSNumber *scale = [NSNumber numberWithDouble:globalVDPreferences.scale];
	[dstButton setState:(globalVDPreferences.DST)? NSOnState : NSOffState];
	[flLRButton setState:(globalVDPreferences.flipLeftRight)? NSOnState : NSOffState];
	[self setComboBox:freqPopup toDoubleVal:globalVDPreferences.frequency];
	[self setComboBox:tzPopup toDoubleVal:globalVDPreferences.timeZone];
	[scaleTextField setObjectValue:scale];
	// figure out how to @#$# keep the stepper synchronised with the text field if the latter is changed!
	[scaleStepperCell takeDoubleValueFrom:scale];
	if( updateChannelDisplay ){
		[self setMatrixForQuadrant:globalVDPreferences.channels.forward toChannel:fwWin];
		[self setMatrixForQuadrant:globalVDPreferences.channels.pilot toChannel:pilotWin];
		[self setMatrixForQuadrant:globalVDPreferences.channels.left toChannel:leftWin];
		[self setMatrixForQuadrant:globalVDPreferences.channels.right toChannel:rightWin];
	}
	[[self window] setDocumentEdited:globalVDPreferences.changed];
}

- (void)freqPopupChanged:sender
{ NSComboBox *it = (NSComboBox*)sender;
  NSNumber *nsval = [it objectValueOfSelectedItem];
	if( !nsval ){
		nsval = [it objectValue];
	}
	NSLog( @"[%@ %@%@] -> (%@)%@", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender,
		 NSStringFromClass([[it objectValueOfSelectedItem] class]), nsval );
	globalVDPreferences.frequency = [nsval doubleValue];
	globalVDPreferences.changed = YES;
	[self update:YES];
}

- (void)tzPopupChanged:sender
{ NSComboBox *it = (NSComboBox*)sender;
  NSNumber *nsval = [it objectValueOfSelectedItem];
	if( !nsval ){
		nsval = [it objectValue];
	}
	NSLog( @"[%@ %@%@] -> (%@)%@", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender,
		 NSStringFromClass([[it objectValueOfSelectedItem] class]), nsval );
	globalVDPreferences.timeZone = [nsval doubleValue];
	globalVDPreferences.changed = YES;
	[self update:YES];
}

- (void)dstButtonChanged:sender
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
	globalVDPreferences.DST = ([dstButton state] == NSOnState);
	globalVDPreferences.changed = YES;
	[self update:YES];
}

- (void)flLRButtonChanged:sender
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
	globalVDPreferences.flipLeftRight = ([flLRButton state] == NSOnState);
	globalVDPreferences.changed = YES;
	[self update:YES];
}

- (void)scaleTextFieldChanged:sender
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
	globalVDPreferences.scale = [scaleTextField doubleValue];
	globalVDPreferences.changed = YES;
	[self update:YES];
}

- (void)scaleNFormatterChanged:sender
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
}

- (void)scaleStepperCellChanged:sender
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
	// synchronise the text field to the stepper. We do that here, because if done in the NIB
	// we do not receive a notification (actions go to a single receiver), and hence we cannot
	// update the global variable (we'd have to use a function to update a static struct and return
	// a pointer to that).
	[scaleTextField takeDoubleValueFrom:sender];
	globalVDPreferences.scale = [scaleTextField doubleValue];
	globalVDPreferences.changed = YES;
	[self update:YES];
}

- (void) setChannel:(int)channel fromQuadrant:(int)qd
{
	switch( channel ){
		case fwWin:
			globalVDPreferences.channels.forward = qd;
			NSLog( @"setChannel:Forward fromQuadrant:%d]", qd );
			break;
		case pilotWin:
			globalVDPreferences.channels.pilot = qd;
			NSLog( @"setChannel:Pilot fromQuadrant:%d]", qd );
			break;
		case leftWin:
			globalVDPreferences.channels.left = qd;
			NSLog( @"setChannel:Left fromQuadrant:%d]", qd );
			break;
		case rightWin:
			globalVDPreferences.channels.right = qd;
			NSLog( @"setChannel:Right fromQuadrant:%d]", qd );
			break;
		default:
			doNSLog( @"Illegal channel #%d in [%@ %@%d]", channel,
				 NSStringFromClass([self class]), NSStringFromSelector(_cmd), qd
			);
			break;
	}
}

- (void) getChMatrixSettings
{
	[self setChannel:[[ch1Matrix selectedCell] tag] fromQuadrant:1];
	[self setChannel:[[ch2Matrix selectedCell] tag] fromQuadrant:2];
	[self setChannel:[[ch3Matrix selectedCell] tag] fromQuadrant:3];
	[self setChannel:[[ch4Matrix selectedCell] tag] fromQuadrant:4];
}

- (void)ch1MatrixChanged:sender
{ NSButton *sel = [ch1Matrix selectedCell];
	if( sel != ch1Matrix.previousCell ){
		NSLog( @"[%@ %@%@]: selected cell %@ #%d",
			 NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender,
			 sel, [sel tag]
		);
//		[self setChannel:[sel tag] fromQuadrant:1];
//		[self getChMatrixSettings];
		[self setChannel:[sel tag] fromQuadrant:1];
		globalVDPreferences.changed = YES;
		[self update:NO];
		ch1Matrix.previousCell = sel;
	}
}

- (void)ch2MatrixChanged:sender
{ NSButton *sel = [ch2Matrix selectedCell];
	if( sel != ch2Matrix.previousCell ){
		NSLog( @"[%@ %@%@]: selected cell %@ #%d",
			 NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender,
			 sel, [sel tag]
		);
//		[self setChannel:[sel tag] fromQuadrant:2];
//		[self getChMatrixSettings];
		[self setChannel:[sel tag] fromQuadrant:2];
		globalVDPreferences.changed = YES;
		[self update:NO];
		ch2Matrix.previousCell = sel;
	}
}

- (void)ch3MatrixChanged:sender
{ NSButton *sel = [ch3Matrix selectedCell];
	if( sel != ch3Matrix.previousCell ){
		NSLog( @"[%@ %@%@]: selected cell %@ #%d",
			 NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender,
			 sel, [sel tag]
		);
//		[self setChannel:[sel tag] fromQuadrant:3];
//		[self getChMatrixSettings];
		[self setChannel:[sel tag] fromQuadrant:3];
		globalVDPreferences.changed = YES;
		[self update:NO];
		ch3Matrix.previousCell = sel;
	}
}

- (void)ch4MatrixChanged:sender
{ NSButton *sel = [ch4Matrix selectedCell];
	if( sel != ch4Matrix.previousCell ){
		NSLog( @"[%@ %@%@]: selected cell %@ #%d",
			 NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender,
			 sel, [sel tag]
		);
//		[self setChannel:[sel tag] fromQuadrant:4];
//		[self getChMatrixSettings];
		[self setChannel:[sel tag] fromQuadrant:4];
		globalVDPreferences.changed = YES;
		[self update:NO];
		ch4Matrix.previousCell = sel;
	}
}

- (BOOL) savePreferences:(const char*)prefsFileName
{ NSInteger result;
  NSString *exts[] = { @"xml", nil };
  NSArray *fileTypes = [NSArray arrayWithObjects:exts count:1];
  NSSavePanel *sPanel = [NSSavePanel savePanel];
  BOOL ret = NO;
	if( prefsFileName ){
		[sPanel setNameFieldStringValue:[NSString stringWithCString:prefsFileName encoding:NSUTF8StringEncoding]];
	}
	else{
		[sPanel setNameFieldStringValue:@"VODdesign.xml"];
	}
	[sPanel setAllowedFileTypes:fileTypes];
	[sPanel setAllowsOtherFileTypes:NO];
	[sPanel setCanCreateDirectories:YES];
	[sPanel setCanSelectHiddenExtension:YES];
	[sPanel setExtensionHidden:NO];
	[sPanel setTreatsFilePackagesAsDirectories:YES];
	[sPanel setMessage:@"Save settings to a VODdesign.xml or equivalent file"];
	result = [sPanel runModal];
	if( result == NSFileHandlingPanelOKButton ){
	  NSString *fName = [[sPanel URL] path];
	  NSMutableString *contents;
		if( (contents = [[[NSMutableString alloc] init] autorelease]) ){
			// construct the text to write:
			[contents setString:@"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
				"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
				"<vod.design>\n" ];
			[contents appendFormat:@"\t<frequency fps=%g />\n", globalVDPreferences.frequency];
			[contents appendFormat:@"\t<scale factor=%g />\n", globalVDPreferences.scale];
			[contents appendFormat:@"\t<UTC zone=%g DST=%s flipLeftRight=%s />\n", globalVDPreferences.timeZone,
								(globalVDPreferences.DST)? "True" : "False",
								(globalVDPreferences.flipLeftRight)? "True" : "False" ];
			[contents appendString:@"\t<channels\n"];
			[contents appendFormat:@"\t\tforward=%d\n", globalVDPreferences.channels.forward];
			[contents appendFormat:@"\t\tpilot=%d\n", globalVDPreferences.channels.pilot];
			[contents appendFormat:@"\t\tleft=%d\n", globalVDPreferences.channels.left];
			[contents appendFormat:@"\t\tright=%d />\n", globalVDPreferences.channels.right];
			[contents appendString:@"</vod.design>\n"];
			// write the contents to the requested file, creating/replacing it as necessary:
			ret = [contents writeToFile:fName atomically:NO encoding:NSUTF8StringEncoding error:NULL];
		}
	}
	if( ret ){
		globalVDPreferences.changed = NO;
	}
	return ret;
}
@synthesize dstButton;
@synthesize flLRButton;
@synthesize scaleTextField;
@synthesize scaleNFormatter;
@synthesize scaleStepperCell;
@end

void UpdateVDPrefsWin(BOOL updateChannelDisplay)
{
	if( VDPrefsWin ){
		[VDPrefsWin update:updateChannelDisplay];
	}
}
