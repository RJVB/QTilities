/*
 *  NSLoggerClient.m
 *  FFusion-RJVB
 *
 *  Created by René J.V. Bertin on 20130210.
 *  Copyright 2013 RJVB. All rights reserved.
 
	C to ObjC bridge to NSLogger functionality
 *
 */

#import <stdarg.h>
#import <Foundation/Foundation.h>
#import "NSLoggerClient.h"
#import "LoggerClient.h"

#define LOGGING_OPTIONS	kLoggerOption_BrowseBonjour/*|kLoggerOption_CaptureSystemConsole*/

int NSLogvprintf( const char *fileName, int lineNumber, const char *functionName, int doLog,
				   const char *item_name, void *avc, int level, const char *format, va_list ap )
{ NSString *string;
  int ret = 0;
  static char inited = 0;
	if( doLog ){
	  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		if( !inited ){
			LoggerSetOptions( NULL, LOGGING_OPTIONS );
			inited = 1;
		}
		if( avc ){
			NSString *nfmt = [[NSString alloc] initWithFormat:@"[%s 0x%lx] %s", item_name, avc, format]; 
			string = [[NSString alloc] initWithFormat:nfmt arguments:ap];
			[nfmt release];
		}
		else{
			string = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:format] arguments:ap];
		}
		ret = [string length];
		LogMessageF( fileName, lineNumber, functionName, @"QTils", level, string );
		[string release];
		[pool drain];
	}
	return ret;
}

int NSLogvprintf2( const char *fileName, int lineNumber, const char *functionName, int doLog,
				   const char *item_name, void *avc, int level, NSString *format, va_list ap )
{ NSString *string;
  int ret = 0;
  static char inited = 0;
	if( doLog ){
	  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		if( !inited ){
			LoggerSetOptions( NULL, LOGGING_OPTIONS );
			inited = 1;
		}
		if( avc ){
			NSString *nfmt = [[NSString alloc] initWithFormat:@"[%s 0x%lx] %@", item_name, avc, format];
			string = [[NSString alloc] initWithFormat:nfmt arguments:ap];
			[nfmt release];
		}
		else{
			string = [[NSString alloc] initWithFormat:format arguments:ap];
		}
		ret = [string length];
		LogMessageF( fileName, lineNumber, functionName, @"QTils", level, string );
		[string release];
		[pool drain];
	}
	return ret;
}

int NSLogprintf( const char *fileName, int lineNumber, const char *functionName, int doLog, const char *format, ...)
{ va_list ap;
  int ret = 0;
	va_start(ap, format);
	ret = NSLogvprintf( fileName, lineNumber, functionName, doLog, NULL, NULL, 0, format, ap );
	va_end(ap);
	return ret;
}

void NSLogFlushLog()
{
	LoggerFlush( NULL, NO );
}

@implementation NSString (NSLoggerClient)
- (void) nsLog:(id)it
{ NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSLog(self);
	[pool drain];
}
@end

int SwitchCocoaToMultiThreadedMode()
{ NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  int ret;
	if( ![NSThread isMultiThreaded] ){
	  NSString *dum = @"Cocoa is now in multi-threaded mode";
		[NSThread detachNewThreadSelector:@selector(nsLog:) toTarget:dum withObject:nil];
		ret = 1;
	}
	else{
		ret = 0;
	}
	[pool drain];
	return ret;
}