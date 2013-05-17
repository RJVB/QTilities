//
//  VDPreferences.h
//
//  Created by René J.V. Bertin on 20110915.
//  Copyright 2011 INRETS/LCPC — LEPSIS. All rights reserved.
//

#ifndef _VDPREFERENCES_H

#import <Cocoa/Cocoa.h>

typedef struct VODChannels {
	int forward, pilot, left, right;
} VODChannels;

typedef struct VODDescription {
	double frequency, scale, timeZone;
	BOOL DST, useVMGI, log, flipLeftRight;
	VODChannels channels;
	BOOL changed;
	char *codec, *bitRate;
	BOOL splitQuad;
} VODDescription;

extern VODDescription globalVDPreferences;

@interface NSAMatrix : NSMatrix {
	NSButton *previousCell;
}
@property (retain) NSButton *previousCell;
@end

@interface VDPreferencesController : NSWindowController {
	IBOutlet NSComboBox			*freqPopup,
							*tzPopup;
	IBOutlet NSButton			*dstButton, *flLRButton, *splitButton;
	IBOutlet NSTextField		*scaleTextField, *codecTextField, *bitRateTextField;
	IBOutlet NSNumberFormatter	*scaleNFormatter;
	IBOutlet NSStepperCell		*scaleStepperCell;

	IBOutlet NSAMatrix			*ch1Matrix, *ch2Matrix, *ch3Matrix, *ch4Matrix;
}

- (void)update:(BOOL)updateChannelDisplay;

- (void)freqPopupChanged:sender;
- (void)tzPopupChanged:sender;
- (void)dstButtonChanged:sender;
- (void)flLRButtonChanged:sender;
- (void)splitButtonChanged:sender;
- (void)scaleTextFieldChanged:sender;
- (void)codecTextFieldChanged:sender;
- (void)bitRateTextFieldChanged:sender;
- (void)scaleNFormatterChanged:sender;
- (void)scaleStepperCellChanged:sender;

- (void)ch1MatrixChanged:sender;
- (void)ch2MatrixChanged:sender;
- (void)ch3MatrixChanged:sender;
- (void)ch4MatrixChanged:sender;

- (BOOL) savePreferences:(const char*)prefsFileName;

@property (retain,readonly) NSButton		*dstButton;
@property (retain,readonly) NSButton		*flLRButton;
@property (retain,readonly) NSButton		*splitButton;
@property (retain,readonly) NSTextField	*scaleTextField;
@property (retain,readonly) NSTextField	*codecTextField;
@property (retain,readonly) NSTextField	*bitRateTextField;
@property (retain,readonly) NSNumberFormatter	*scaleNFormatter;
@property (retain,readonly) NSStepperCell	*scaleStepperCell;
@end

extern void UpdateVDPrefsWin(BOOL updateChannelDisplay);


#define _VDPREFERENCES_H
#endif
