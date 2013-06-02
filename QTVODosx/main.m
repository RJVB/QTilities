//
//  main.m
//  QTAmateur -> QTVOD
//
//  Created by Michael Ash on 5/22/05.
//  Copyright __MyCompanyName__ 2005 . All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <QTKit/QTKit.h>
#ifndef _QTKITDEFINES_H
#	define _QTKITDEFINES_H
#endif
#import "../QTils/QTilities.h"
#import "QTVODWindow.h"
#import "QTVOD.h"
#import "VDPreferences.h"

@implementation NSApplication (toggleLogging)

NSMenuItem *DoLoggingMI = NULL;
BOOL doLogging = NO;
extern BOOL QTils_LogSetActive(BOOL);

- (void)updateMenus
{
	if( DoLoggingMI ){
		[DoLoggingMI setState:doLogging];
	}
}

- (void)toggleLogging:sender
{
	doLogging = !doLogging;
	if( doLogging ){
		[[NSWorkspace sharedWorkspace] launchApplication:@"NSLogger"];
	}
	QTils_LogMsgEx( "toggleLogging: logging now %s\n", (doLogging)? "ON" : "OFF" );
	QTils_LogSetActive(doLogging);
	if( sender != self ){
		DoLoggingMI = sender;
	}
	[self updateMenus];
}
@end

//@implementation NSApplication (showPreferences)
//
//- (void)showPreferences:sender
//{ VDPreferences *vdPrefsWin = [[[VDPreferences alloc] init] autorelease];
//	return;
//}
//
//@end

@implementation QTVODApplicationDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{ extern NSObject *nsXMLVD;
	if( QTVODList ){
#ifdef DEBUG
	  int N = [QTVODList count], n = 0;
#endif
		// quitting the application via the Quit menu/command doesn't (always?) cause our windows
		// to be closed via the same path as when closing them manually/programmatically. In particular,
		// it can happen that changes to a QTVOD->fullMovie are discarded without opportunity to save them.
		// Bumping the ChangeCount on the QTVODWindow document stalls the quit process, but a bit too effectively
		// since those windows are hidden (i.e. the app never quits).
		// Thus, we maintain a list of all open QTVOD "master documents", and close them "our way", before
		// allowing the application to quit.
		for( QTVOD *qv in QTVODList ){
			[qv close];
#ifdef DEBUG
			n += 1;
#endif
		}
		[QTVODList release];
		QTVODList = nil;
		CloseQT();
	}
	if( nsXMLVD ){
		[nsXMLVD release];
	}
	return NSTerminateNow;
}
@end

// From http://developer.apple.com/library/mac/#documentation/Carbon/Conceptual/ProvidingUserAssitAppleHelp/registering_help/registering_help.html%23//apple_ref/doc/uid/TP30000903-CH207-CHDGHHDF :
OSStatus RegisterMyHelpBook(void)
{ CFBundleRef myApplicationBundle;
  CFURLRef myBundleURL;
  OSStatus err = noErr;

	myApplicationBundle = NULL;
	myBundleURL = NULL;

	myApplicationBundle = CFBundleGetMainBundle();// 1
	if( myApplicationBundle == NULL ){
		err = fnfErr;
		goto bail;
	}

	myBundleURL = CFBundleCopyBundleURL(myApplicationBundle);// 2
	if( myBundleURL == NULL ){
		err = fnfErr;
		goto bail;
	}

	if( err == noErr ){
		err = AHRegisterHelpBookWithURL(myBundleURL);// 3
	}
	CFRelease(myBundleURL);
bail:
	return err;
}

static void freep( void **p)
{
	if( p && *p ){
		free(*p);
		*p = NULL;
	}
}

int main(int argc, char *argv[])
{ extern Boolean QTMWInitialised;
	QTils_MessagePumpIsInActive = TRUE;
	QTils_LogSetActive(doLogging);
	if( !QTMWInitialised ){
		init_QTils_Allocator( malloc, calloc, realloc, freep );
		InitQTMovieWindows();
	}
	RegisterMyHelpBook();
	{ QTVOD *qv = [QTVOD alloc];
	  VODDescription d;
		if( [qv ReadDefaultVODDescription:"VODdesign.xml" toDescription:&d] == noErr ){
			NSLog( @"Read settings from VODdesign.xml" );
			globalVDPreferences = d;
		}
		[qv release];
	}
	[NSApp setDelegate:[[[QTVODApplicationDelegate alloc] init] autorelease] ];
	return NSApplicationMain( argc, (const char **) argv );
}
