//
//  BatchExportController.m
//  QTAmateur -> QTVOD
//
//  Created by Michael Ash on 5/24/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "BatchExportController.h"

#import "ExportManager.h"
#import "MAMovieExport.h"


@implementation BatchExportController

- init
{
//	NSLog( @"[%@ %@] - [super initWithWindowNibName...]", NSStringFromClass([self class]), NSStringFromSelector(_cmd) );
	if((self = [super initWithWindowNibName:@"BatchExport"]))
	{
		tableContents = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[exportManager release];
	[tableContents release];

	[super dealloc];
}

- (void)windowDidLoad
{
	[super windowDidLoad];

	if(!exportManager)
		exportManager = [[ExportManager alloc] init];

	NSArray *components = [[MAMovieExport sharedInstance] componentList];

	[typePopup removeAllItems];
	for(id loopItem in components)
	{
		NSString *name = [loopItem objectForKey:kMAMovieExportComponentName];
		if(!name) name = [loopItem objectForKey:kMAMovieExportComponentFileExtension];
		if(!name) name = @"(null)";
		[[typePopup menu] addItemWithTitle:name action:NULL keyEquivalent:@""];
	}

	int index = [exportManager defaultIndex];
	[typePopup selectItemAtIndex:index];
	[self typePopupChanged:self];

	[tableView registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil]];

	[self setExportButtonEnabled];
}

- (void)setExportButtonEnabled
{
	[exportButton setEnabled:[tableContents count] > 0];
}

- (void)typePopupChanged:sender
{
//	NSLog( @"[%@ %@%@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd), sender );
	int index = [typePopup indexOfSelectedItem];
	NSArray *components = [[MAMovieExport sharedInstance] componentList];
	NSString *info = [[components objectAtIndex:index] objectForKey:kMAMovieExportComponentInfo];
	[infoField setStringValue:info];
	if(!info) info = NSLocalizedString(@"No info", @"");
	[exportManager setDefaultIndex:index];
}

- (void)doSettings:sender
{
	int componentIndex = [typePopup indexOfSelectedItem];
	[exportManager showSettingsAtIndex:componentIndex];
}

- (void)doExport:sender
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setPrompt:NSLocalizedString(@"Export", @"")];
	[panel setTitle:NSLocalizedString(@"Export", @"")];
	[panel setNameFieldLabel:NSLocalizedString(@"Export To:", @"")];

	[panel setCanChooseFiles:NO];
	[panel setCanChooseDirectories:YES];

	[panel beginSheetForDirectory:nil file:nil modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(exportPanelDidEnd:returnCode:contextInfo:) contextInfo:NULL];
}

- (BOOL)doExportPreflightToDirectory:(NSString *)directory withExtension:(NSString *)fileExtension
{
	NSFileManager *manager = [NSFileManager defaultManager];

	if(![manager isWritableFileAtPath:directory])
	{
		NSAlert *alert = [[NSAlert alloc] init];
		[alert setMessageText:[NSString stringWithFormat:NSLocalizedString(@"The directory %@ is not writable.", @""), [directory lastPathComponent]]];
		[alert setInformativeText:NSLocalizedString(@"Please choose another directory.", @"")];
		[alert setAlertStyle:NSCriticalAlertStyle];

		[alert runModal];

		[alert release];

		return NO;
	}

	NSMutableArray *badFilesArray = [NSMutableArray array];
	NSDictionary *dict;
	for(dict in tableContents)
	{
		NSString *origName = [dict objectForKey:@"filename"];
		NSString *nameNoExt = [origName stringByDeletingPathExtension];
		NSString *newName = [nameNoExt stringByAppendingPathExtension:fileExtension];
		NSString *fullpath = [directory stringByAppendingPathComponent:newName];

		if([manager fileExistsAtPath:fullpath])
			[badFilesArray addObject:newName];
	}

	if([badFilesArray count] > 0)
	{
		NSAlert *alert = [[NSAlert alloc] init];
		[alert setMessageText:NSLocalizedString(@"Some files in the destination directory will be overwritten. Are you sure you want to continue?", @"")];
		[alert setInformativeText:[NSLocalizedString(@"The following files will be overwritten:\n", @"") stringByAppendingString:[badFilesArray componentsJoinedByString:@"\n\t"]]];

		[alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"")];
		[alert addButtonWithTitle:NSLocalizedString(@"Replace", @"")];

		int result = [alert runModal];

		[alert release];

		return (result == NSAlertSecondButtonReturn);
	}

	return YES;
}

- (void)exportPanelDidEnd:(NSSavePanel *)panel returnCode:(int)code contextInfo:(void *)ctx
{
	if(code == NSOKButton)
	{
		[self performSelector:@selector(doExportIntoDirectory:) withObject:[panel filename] afterDelay:0.0];
	}
}

- (void)doExportIntoDirectory:(NSString *)directory
{
	NSArray *components = [[MAMovieExport sharedInstance] componentList];
	int componentIndex = [typePopup indexOfSelectedItem];
	NSString *extension = [[components objectAtIndex:componentIndex] objectForKey:kMAMovieExportComponentFileExtension];
	if([self doExportPreflightToDirectory:directory withExtension:extension])
	{
		while([tableContents count] > 0)
		{
			NSDictionary *dict = [tableContents objectAtIndex:0];
			NSURL *url = [dict objectForKey:@"URL"];
			NSString *filename = [dict objectForKey:@"filename"];

			NSString *nameNoExt = [filename stringByDeletingPathExtension];
			NSString *newName = [nameNoExt stringByAppendingPathExtension:extension];
			NSString *fullPathDest = [directory stringByAppendingPathComponent:newName];

			NSError *error;
			QTMovie *movie = [QTMovie movieWithURL:url error:&error];
			if(!movie)
			{
				NSAlert *alert = [NSAlert alertWithError:error];
				[alert runModal];
				break;
			}

			BOOL result = [exportManager exportMovie:movie toFile:fullPathDest named:filename atIndex:componentIndex];

			if(!result)
				break;

			[tableContents removeObjectAtIndex:0];
			[tableView reloadData];
		}
	}
}

@synthesize tableView;
@synthesize typePopup;
@synthesize infoField;
@synthesize exportButton;
@synthesize exportManager;
@synthesize tableContents;
@end

@implementation BatchExportController (TableDataSource)

- (int)numberOfRowsInTableView:(NSTableView *)tableView
{
	return [tableContents count];
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(int)row
{
	if(row < 0 || row >= [tableContents count])
		return nil;

	return [[tableContents objectAtIndex:row] objectForKey:[tableColumn identifier]];
}

- (NSDragOperation)tableView:(NSTableView*)tv validateDrop:(id <NSDraggingInfo>)info proposedRow:(int)row proposedDropOperation:(NSTableViewDropOperation)op
{
	if([[[info draggingPasteboard] types] containsObject:NSFilenamesPboardType])
	{
		if(op == NSTableViewDropOn)
			[tableView setDropRow:row dropOperation:NSTableViewDropAbove];
		return NSDragOperationCopy;
	}
	else
		return NSDragOperationNone;
}

- (BOOL)tableView:(NSTableView*)tv acceptDrop:(id <NSDraggingInfo>)info row:(int)row dropOperation:(NSTableViewDropOperation)op
{
	NSPasteboard *pboard = [info draggingPasteboard];
	if([[pboard types] containsObject:NSFilenamesPboardType])
	{
		if(row < 0)
			row = 0;

		id array = [pboard propertyListForType:NSFilenamesPboardType];
		if(![array isKindOfClass:[NSArray class]])
			array = [NSArray arrayWithObject:array];
		NSEnumerator *enumerator = [array reverseObjectEnumerator];
		id obj;
		while((obj = [enumerator nextObject]))
		{
			if(![obj isKindOfClass:[NSString class]])
				continue;

			NSString *filename = [obj lastPathComponent];
			if(![QTMovie canInitWithFile:obj])
			{
				NSAlert *alert = [[NSAlert alloc] init];

				[alert setMessageText:[NSString stringWithFormat:NSLocalizedString(@"QuickTime doesn't recognize the file %@.", @""), filename]];
				[alert setInformativeText:NSLocalizedString(@"The file may be corrupt or of a type that QuickTime doesn't understand.", @"")];

				[alert runModal];

				[alert release];

				continue;
			}

			NSURL *url = [NSURL fileURLWithPath:obj];
			NSDictionary *dictionary = [NSDictionary dictionaryWithObjectsAndKeys:
				url, @"URL",
				filename, @"filename",
				nil];
			[tableContents insertObject:dictionary atIndex:row];
		}
		[tableView reloadData];
		return YES;
	}
	return NO;
}

- (void)tableViewDeleteSelectedRows:(NSTableView *)tv
{
	NSIndexSet *indexes = [tableView selectedRowIndexes];
	[tableContents removeObjectsAtIndexes:indexes];
	[tableView reloadData];
}

@end
