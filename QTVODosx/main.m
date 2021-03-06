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
#import "QTVODlib.h"
#import "VDPreferences.h"

#ifndef DEBUG
	extern int QTils_Log(const char *fileName, int lineNr, NSString *format, ... );
#	define NSLog(f,...)	QTils_Log(__FILE__, __LINE__, f, ##__VA_ARGS__)
#endif

@implementation NSApplication (QTVOD)

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
		if( [[NSWorkspace sharedWorkspace] launchApplication:@"NSLogger"] ){
			fprintf( stderr, "Waiting for NSLogger ..." ); fflush(stderr);
			while( ![[[[NSWorkspace sharedWorkspace] activeApplication] objectForKey:@"NSApplicationName"]
				    isEqualToString:@"NSLogger"]
			){
				PumpMessages(YES);
			}
			fputs( " NSLogger running\n", stderr );
		}
		// bring us back to the front:
		[NSApp activateIgnoringOtherApps:YES];
	}
	QTils_LogSetActive(doLogging);
	QTils_LogMsgEx( "toggleLogging: logging now %s\n", (doLogging)? "ON" : "OFF" );
	if( sender != self ){
		DoLoggingMI = sender;
	}
	[self updateMenus];
}

//- (id) handleConnectToServerScriptCommand:(NSScriptCommand*) command
//{ NSDictionary *args = [command arguments];
//  NSString *address;
//	NSLog(@"connectToServer %@ (%@)", command, NSStringFromClass([command class]) );
//	address = [args objectForKey:@"address"];
//	if( address ){
//		commsCleanUp();
//		commsInit([address UTF8String]);
//	}
//	if( errSock != 0 || sServer == NULLSOCKET ){
//		[command setScriptErrorNumber:errSock];
//		[command setScriptErrorString:@"InitCommClient returned an error"];
//	}
//	return nil;
//}
//
//- (id) handleToggleLoggingScriptCommand:(NSScriptCommand*) command
//{ 
//	NSLog(@"toggleLogging %@ (%@)", command, NSStringFromClass([command class]) );
//	[self toggleLogging:NULL];
//	return nil;
//}
//
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
	if( sServer != NULLSOCKET ){
	  NetMessage msg;
		msgQuitQTVOD(&msg);
		msg.flags.category = qtvod_Notification;
		SendMessageToNet( sServer, &msg, SENDTIMEOUT, NO, __FUNCTION__ );
	}
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
			if( [qv cbRegister] ){
				DisposeCallBackRegister([qv cbRegister]);
				[qv setCbRegister:NULL];
			}
			[qv closeAndRelease];
			NSLog( @"%@ retainCount=%u", qv, [qv retainCount] );
			while( [qv retainCount] > 1 ){
				[qv release];
			}
#ifdef DEBUG
			n += 1;
#endif
		}
		[QTVODList release];
		QTVODList = nil;
	}
	commsCleanUp();
	CloseQT();
	if( nsXMLVD ){
		[nsXMLVD release];
	}
	return NSTerminateNow;
}

- (void) applicationWillFinishLaunching:(NSNotification*)notice
{ extern int *_NSGetArgc();
  extern char ***_NSGetArgv();
	ParseArgs( *_NSGetArgc(), *_NSGetArgv() );
	UpdateVDPrefsWin(YES);
}

//- (id) handleConnectToServerScriptCommand:(NSScriptCommand*) command
//{ NSDictionary *args = [command arguments];
//  NSString *address;
//	NSLog(@"connectToServer %@ (%@)", command, NSStringFromClass([command class]) );
//	address = [args objectForKey:@"address"];
//	if( address ){
//		commsCleanUp();
//		commsInit([address UTF8String]);
//	}
//	if( errSock != 0 || sServer == NULLSOCKET ){
//		[command setScriptErrorNumber:errSock];
//		[command setScriptErrorString:@"InitCommClient returned an error"];
//	}
//	return nil;
//}
//
//- (id) handleToggleLoggingScriptCommand:(NSScriptCommand*) command
//{ 
//	NSLog(@"toggleLogging %@ (%@)", command, NSStringFromClass([command class]) );
//	[[NSApplication sharedApplication] toggleLogging:NULL];
//	return nil;
//}

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

__attribute__((constructor))
static void initialiser()
{
	init_QTils_Allocator( malloc, calloc, realloc, freep );
}

int main(int argc, char *argv[])
{ extern Boolean QTMWInitialised;
	QTils_MessagePumpIsInActive = TRUE;
	QTils_LogSetActive(doLogging);
	if( !QTMWInitialised ){
		InitQTMovieWindows();
	}
	RegisterMyHelpBook();
	{ QTVOD *qv = [QTVOD alloc];
	  VODDescription d;
		// 20130802: better initialise d to all 0 if not to something more appropriate!
		memset( &d, 0, sizeof(d) );
		if( [qv ReadDefaultVODDescription:"VODdesign.xml" toDescription:&d] == noErr ){
			NSLog( @"Read settings from VODdesign.xml" );
			globalVD.preferences = d;
		}
		[qv release];
	}
	[[NSApplication sharedApplication] setDelegate:[[[QTVODApplicationDelegate alloc] init] autorelease] ];

	return NSApplicationMain( argc, (const char **) argv );
}
