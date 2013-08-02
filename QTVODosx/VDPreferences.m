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

GlobalPreferencesStruct globalVD = {
	{
		12.5, 1.0, 1.0,
		FALSE, TRUE, FALSE, TRUE,
		{1, 2, 3, 4},
		"copy", "2000k"
	},
	FALSE
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
		if( globalVD.changed ){
			[VDPrefsWin savePreferences:NULL];
		}
		globalVD.changed = NO;
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

static VDPreferencesController *prefsController = NULL;

@implementation VDPreferencesController

- (id) init
{
//	NSLog( @"[%@ %@] - [super initWithWindowNibName...]", NSStringFromClass([self class]), NSStringFromSelector(_cmd) );
	self = [super initWithWindowNibName:@"VDPreferences"];
	if( !prefsController ){
		prefsController = self;
	}
	return self;
}

- (void)dealloc
{
//	NSLog( @"[%@ %@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd) );
	if( windowTitle ){
		[windowTitle release];
	}
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
	if( sender ){
		[self setTitle:@"VODdesign"];
	}
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
{ NSNumber *scale = [NSNumber numberWithDouble:globalVD.preferences.scale];
	[dstButton setState:(globalVD.preferences.DST)? NSOnState : NSOffState];
	[flLRButton setState:(globalVD.preferences.flipLeftRight)? NSOnState : NSOffState];
	[splitButton setState:(globalVD.preferences.splitQuad)? NSOnState : NSOffState];
	[self setComboBox:freqPopup toDoubleVal:globalVD.preferences.frequency];
	[self setComboBox:tzPopup toDoubleVal:globalVD.preferences.timeZone];
	[scaleTextField setObjectValue:scale];
	if( globalVD.preferences.codec ){
		[codecTextField setObjectValue:[NSString stringWithUTF8String:globalVD.preferences.codec]];
	}
	else{
		[codecTextField setObjectValue:@""];
	}
	if( globalVD.preferences.bitRate ){
		[bitRateTextField setObjectValue:[NSString stringWithUTF8String:globalVD.preferences.bitRate]];
	}
	else{
		[bitRateTextField setObjectValue:@""];
	}
	// figure out how to @#$# keep the stepper synchronised with the text field if the latter is changed!
	[scaleStepperCell takeDoubleValueFrom:scale];
	if( updateChannelDisplay ){
		[self setMatrixForQuadrant:globalVD.preferences.channels.forward toChannel:fwWin];
		[self setMatrixForQuadrant:globalVD.preferences.channels.pilot toChannel:pilotWin];
		[self setMatrixForQuadrant:globalVD.preferences.channels.left toChannel:leftWin];
		[self setMatrixForQuadrant:globalVD.preferences.channels.right toChannel:rightWin];
	}
	[[self window] setDocumentEdited:globalVD.changed];
}

- (void)freqPopupChanged:sender
{ NSComboBox *it = (NSComboBox*)sender;
  NSNumber *nsval = [it objectValueOfSelectedItem];
	if( !nsval ){
		nsval = [it objectValue];
	}
	NSLog( @"[%@ %@%@] -> (%@)%@", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender,
		 NSStringFromClass([[it objectValueOfSelectedItem] class]), nsval );
	globalVD.preferences.frequency = [nsval doubleValue];
	globalVD.changed = YES;
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
	globalVD.preferences.timeZone = [nsval doubleValue];
	globalVD.changed = YES;
	[self update:YES];
}

- (void)dstButtonChanged:sender
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
	globalVD.preferences.DST = ([dstButton state] == NSOnState);
	globalVD.changed = YES;
	[self update:YES];
}

- (void)flLRButtonChanged:sender
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
	globalVD.preferences.flipLeftRight = ([flLRButton state] == NSOnState);
	globalVD.changed = YES;
	[self update:YES];
}

- (void)splitButtonChanged:sender
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
	globalVD.preferences.splitQuad = ([splitButton state] == NSOnState);
	globalVD.changed = YES;
	[self update:YES];
}

- (void)codecTextFieldChanged:sender
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
	if( globalVD.preferences.codec ){
		QTils_free(globalVD.preferences.codec);
	}
	globalVD.preferences.codec = QTils_strdup([[codecTextField stringValue] UTF8String]);
	globalVD.changed = YES;
	[self update:YES];
}

- (void)bitRateTextFieldChanged:sender
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
	if( globalVD.preferences.bitRate ){
		QTils_free(globalVD.preferences.bitRate);
	}
	globalVD.preferences.bitRate = QTils_strdup([[bitRateTextField stringValue] UTF8String]);
	globalVD.changed = YES;
	[self update:YES];
}

- (void)scaleTextFieldChanged:sender
{
	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
	globalVD.preferences.scale = [scaleTextField doubleValue];
	globalVD.changed = YES;
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
	globalVD.preferences.scale = [scaleTextField doubleValue];
	globalVD.changed = YES;
	[self update:YES];
}

- (void) setChannel:(int)channel fromQuadrant:(int)qd
{
	switch( channel ){
		case fwWin:
			globalVD.preferences.channels.forward = qd;
			NSLog( @"setChannel:Forward fromQuadrant:%d]", qd );
			break;
		case pilotWin:
			globalVD.preferences.channels.pilot = qd;
			NSLog( @"setChannel:Pilot fromQuadrant:%d]", qd );
			break;
		case leftWin:
			globalVD.preferences.channels.left = qd;
			NSLog( @"setChannel:Left fromQuadrant:%d]", qd );
			break;
		case rightWin:
			globalVD.preferences.channels.right = qd;
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
		globalVD.changed = YES;
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
		globalVD.changed = YES;
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
		globalVD.changed = YES;
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
		globalVD.changed = YES;
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
			[contents appendFormat:@"\t<frequency fps=%g />\n", globalVD.preferences.frequency];
			[contents appendFormat:@"\t<scale factor=%g />\n", globalVD.preferences.scale];
			[contents appendFormat:@"\t<UTC zone=%g DST=%s />\n", globalVD.preferences.timeZone,
								(globalVD.preferences.DST)? "True" : "False" ];
			[contents appendString:@"\t<channels\n"];
			[contents appendFormat:@"\t\tforward=%d\n", globalVD.preferences.channels.forward];
			[contents appendFormat:@"\t\tpilot=%d\n", globalVD.preferences.channels.pilot];
			[contents appendFormat:@"\t\tleft=%d\n", globalVD.preferences.channels.left];
			[contents appendFormat:@"\t\tright=%d flipLeftRight=%s />\n", globalVD.preferences.channels.right,
								(globalVD.preferences.flipLeftRight)? "True" : "False" ];
			[contents appendString:@"\t<transcoding.mp4\n"];
			if( globalVD.preferences.codec && *globalVD.preferences.codec ){
				[contents appendFormat:@"\t\tcodec=%s\n", globalVD.preferences.codec ];
			}
			if( globalVD.preferences.bitRate && *globalVD.preferences.bitRate ){
				[contents appendFormat:@"\t\tbitrate=%s\n", globalVD.preferences.bitRate ];
			}
			[contents appendFormat:@"\t\tsplit=%s />\n", (globalVD.preferences.splitQuad)? "True" : "False" ];
			[contents appendString:@"</vod.design>\n"];
			// write the contents to the requested file, creating/replacing it as necessary:
			ret = [contents writeToFile:fName atomically:NO encoding:NSUTF8StringEncoding error:NULL];
			if( ret ){
				[self setTitle:fName];
			}
		}
	}
	if( ret ){
		globalVD.changed = NO;
	}
	return ret;
}

- (void) setTitle:(NSString*)title
{
	if( windowTitle ){
		[windowTitle release];
	}
	windowTitle = [title retain];
	[[self window] setTitle:windowTitle];
}

- (NSString *)windowTitleForDocumentDisplayName:(NSString *)displayName
{
	return (windowTitle)? windowTitle : displayName;
}

@synthesize splitButton;
@synthesize bitRateTextField;
@synthesize codecTextField;
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

@implementation VDPreferencesLoader

// this is the callback invoked when we open an XML file.
- (BOOL)readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError
{ BOOL ret = NO;
	if( [typeName isEqual:@"XMLPropertyList"] ){
	  QTVOD *qv = [QTVOD alloc];
	  VODDescription d;
		if( prefsController ){
			[prefsController close];
		}
		if( [qv nsReadDefaultVODDescription:[absoluteURL path] toDescription:&d] == noErr ){
			doNSLog( @"Read settings from %@", [absoluteURL path] );
			globalVD.preferences = d;
			{ VDPreferencesController *vd;
				// either use the existing preferences window, or create a new instance:
				if( prefsController ){
					vd = prefsController;
				}
				else{
					vd = [[[VDPreferencesController alloc] init] retain];
				}
				[[vd window] display];
				[vd showWindow:NULL];
				// add preference window controller to the NSDocument instance we just opened.
				// It's this bit of magic that will close the document (and thus allow the file
				// to be reloaded) when we close the preference window.
				[self addWindowController:vd];
				[vd setDocument:self];
				[vd setTitle:[absoluteURL path]];
				ret = YES;
			}
		}
		[qv release];
	}
	return ret;
}

@synthesize addToRecentMenu;

@end

__attribute__((constructor))
static void initialiser( int argc, char** argv, char** envp )
{
	globalVD.preferences.codec = QTils_strdup(globalVD.preferences.codec);
	globalVD.preferences.bitRate = QTils_strdup(globalVD.preferences.bitRate);
}
