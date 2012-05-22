//
//  BatchExportController.h
//  QTAmateur -> QTVOD
//
//  Created by Michael Ash on 5/24/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@class ExportManager;

@interface BatchExportController : NSWindowController {
	IBOutlet NSTableView *tableView;
	IBOutlet NSPopUpButton *typePopup;
	IBOutlet NSTextField *infoField;
	IBOutlet NSButton *exportButton;

	ExportManager *exportManager;

	NSMutableArray *tableContents; // NSDictionary containing @"URL" and @"filename"
}

- (void)setExportButtonEnabled;

- (void)typePopupChanged:sender;
- (void)doSettings:sender;
- (void)doExport:sender;

@property (retain) NSTableView *tableView;
@property (retain) NSPopUpButton *typePopup;
@property (retain) NSTextField *infoField;
@property (retain) NSButton *exportButton;
@property (retain) ExportManager *exportManager;
@property (retain) NSMutableArray *tableContents;
@end
