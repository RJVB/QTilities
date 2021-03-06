/*
 *  AskFileName.m
 *  QTilities
 *
 *  Created by René J.V. Bertin on 20110125.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 * QuickTime player toolkit; this file contains the MSWin32-specific routines and those that depend
 * on them directly.
 *
 */


#include "copyright.h"
IDENTIFY("AskFileName: Mac OS X/Cocoa file dialog");

#include <errno.h>
#include <stdlib.h>
#include <errno.h>

#import <Cocoa/Cocoa.h>

#ifndef TARGET_OS_MAC
#	define TARGET_OS_MAC
#endif

char *QTils_strdup( const char *txt );
void *QTils_realloc( void* mem, size_t size );


int FreeAskedFileName()
{
	return 0;
}

char *AskFileName( char *title )
{ int result;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *exts[16] = { @"mov", @"qi2m", @"VOD", @"jpgs", @"mpg",
		@"mp4", @"mpeg", @"avi", @"wmv", @"mp3", @"aif", @"wav", @"mid", @"jpg", @"jpeg", nil };
  NSArray *fileTypes = [NSArray arrayWithObjects:exts count:15];
  NSOpenPanel *oPanel = [NSOpenPanel openPanel];
  static char *nameBuf = NULL;
  char *fName = NULL;

	[oPanel setAllowsMultipleSelection:NO];
	[oPanel setCanChooseDirectories:NO];
	[oPanel setAllowedFileTypes:fileTypes];
	[oPanel setAllowsOtherFileTypes:YES];
	[oPanel setTreatsFilePackagesAsDirectories:YES];
	[oPanel setMessage:[NSString stringWithCString:title encoding:NSUTF8StringEncoding]];
	result = [oPanel runModal];
	if( result == NSOKButton ){
	  NSArray *filesToOpen = [oPanel URLs];
	  int count = [filesToOpen count];
		if( count > 0 ){
		  NSURL *aFile = [filesToOpen objectAtIndex:0];
// 		  const char *c = [[aFile absoluteString] cStringUsingEncoding:NSUTF8StringEncoding]; // or defaultCStringEncoding ?
		  const char *c = [[[aFile path] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding]
					    cStringUsingEncoding:NSUTF8StringEncoding];
			nameBuf = QTils_realloc( nameBuf, (1 + strlen(c)) * sizeof(char) );
			strcpy( nameBuf, c ), fName = nameBuf;
			if( strncasecmp( fName, "file://", 7 ) == 0 ){
				fName = strchr( &fName[7], '/' );
			}
		}
    }
    if( pool ){
	    [pool drain];
    }
    return fName;
}

char *AskSaveFileName( char *title )
{ NSInteger result;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *exts[16] = { @"mov", @"qi2m", @"VOD", @"jpgs", @"mpg",
		@"mp4", @"mpeg", @"avi", @"wmv", @"mp3", @"aif", @"wav", @"mid", @"jpg", @"jpeg", nil };
  NSArray *fileTypes = [NSArray arrayWithObjects:exts count:15];
  NSSavePanel *sPanel = [NSSavePanel savePanel];
  static char *nameBuf = NULL;
  char *fName = NULL;

	[sPanel setAllowedFileTypes:fileTypes];
	[sPanel setAllowsOtherFileTypes:YES];
	[sPanel setCanCreateDirectories:YES];
	[sPanel setCanSelectHiddenExtension:YES];
	[sPanel setExtensionHidden:NO];
	[sPanel setTreatsFilePackagesAsDirectories:YES];
	[sPanel setMessage:[NSString stringWithCString:title encoding:NSUTF8StringEncoding]];
	result = [sPanel runModal];
	if( result == NSFileHandlingPanelOKButton ){
	  NSURL *aFile = [sPanel URL];
// 	  const char *c = [[aFile absoluteString] cStringUsingEncoding:NSUTF8StringEncoding]; // or defaultCStringEncoding ?
	  const char *c = [[[aFile path] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding]
				    cStringUsingEncoding:NSUTF8StringEncoding];
		nameBuf = QTils_realloc( nameBuf, (1 + strlen(c)) * sizeof(char) );
		strcpy( nameBuf, c ), fName = nameBuf;
		if( strncasecmp( fName, "file://", 7 ) == 0 ){
			fName = strchr( &fName[7], '/' );
		}
    }
    if( pool ){
	    [pool drain];
    }
    return fName;
}

int PostYesNoDialog( const char *title, const char *message )
{ NSAlert* alert = [NSAlert
			alertWithMessageText:[NSString stringWithCString:title encoding:NSUTF8StringEncoding]
#ifdef LOG_FRANCAIS
			defaultButton:@"Oui" alternateButton:@"Non" otherButton:NULL
#else
			defaultButton:@"Yes" alternateButton:@"No" otherButton:NULL
#endif
			informativeTextWithFormat:[NSString stringWithCString:message encoding:NSUTF8StringEncoding]
		];
	return NSAlertDefaultReturn == [alert runModal];
}

int PostMessageBox( const char *title, const char *message )
{ NSAlert* alert = [[[NSAlert alloc] init] autorelease];
  NSString *msg, *tit;
	@synchronized([NSAlert class]){
		[alert setAlertStyle:NSInformationalAlertStyle];
		[alert setMessageText:@"" ];
		if( !(msg = [NSString stringWithCString:message encoding:NSUTF8StringEncoding]) ){
			msg = [NSString stringWithCString:message encoding:NSASCIIStringEncoding];
		}
		if( !(tit = [NSString stringWithCString:title encoding:NSUTF8StringEncoding]) ){
			tit = [NSString stringWithCString:title encoding:NSASCIIStringEncoding];
		}
		if( msg ){
			[alert setInformativeText:msg];
		}
		else{
			NSLog( @"msg=%@ tit=%@", msg, tit );
		}
		[[alert window] setTitle:tit];
		return NSAlertDefaultReturn == [alert runModal];
	}
	return 0;
}
