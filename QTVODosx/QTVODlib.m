/*!
 *  @file QTVODlib.m
 *  QTVODosx
 *
 *  Created by René J.V. Bertin on 20130604.
 *  Copyright 2013 RJVB. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#import <libgen.h>
#include <string.h>
#import <math.h>
#include "QTVOD.h"
#include "QTVODcomm.h"
#include "QTVODlib.h"
#include "QTilities.h"

extern int QTils_Log(const char *fileName, int lineNr, NSString *format, ... );
extern BOOL QTils_LogSetActive(BOOL);

char *ipAddress = NULL;
char *assocDataFileName = NULL;
SOCK sServer = NULLSOCKET;
NSInputStream *nsReadServer = NULL;
NSOutputStream *nsWriteServer = NULL;

static void NSnoLog( NSString *format, ... )
{
	return;
}

static void doNSLog( NSString *format, ... )
{ va_list ap;
  extern int QTils_Log(const char *fileName, int lineNr, NSString *format, ... );
	va_start(ap, format);
	NSLogv( format, ap );
	QTils_Log( __FILE__, __LINE__, [[[NSString alloc] initWithFormat:format arguments:ap] autorelease] );
	va_end(ap);
	return;
}

#ifndef DEBUG
#	define NSLog	NSnoLog
#endif

@interface QTVODStreamDelegate : NSObject<NSStreamDelegate>{
}
- (void) stream:(NSInputStream*)theStream handleEvent:(NSStreamEvent)theEvent;
@end

@implementation QTVODStreamDelegate

- (void) stream:(NSInputStream*)theStream handleEvent:(NSStreamEvent)theEvent
{
	switch( theEvent ){
		case NSStreamEventEndEncountered:
			break;
		case NSStreamEventHasBytesAvailable:{
		  uint8_t *buffer;
		  NSUInteger count;
		  NetMessage Msg, *msg = NULL;
			if( [theStream getBuffer:&buffer length:&count] ){
					QTils_Log( __FILE__, __LINE__, @"stream %@ has %u bytes available", theStream, count );
				if( count == sizeof(NetMessage) ){
					msg = (NetMessage*) buffer;
				}
			}
			else{
			  NSInteger read;
				read = [theStream read:(uint8_t*)&Msg maxLength:sizeof(Msg)];
				if( read == sizeof(Msg) ){
					msg = &Msg;
				}
				else if( read < 0 ){
					if( ReceiveErrors > 3 ){
						read = 0;
					}
					else{
						ReceiveErrors += 1;
					}
				}
				if( !read ){
				  NSStreamStatus status = [theStream streamStatus];
				  NSError *error = [theStream streamError];
					QTils_Log( __FILE__, __LINE__,
							@"Stream closed remotely or too many (%d) errors (last '%@'; status=%d): closing channel",
							ReceiveErrors, error, status );
					CloseCommClient(&sServer);
					PostMessage( "QTVOD", lastSSLogMsg );
				}
			}
			if( msg ){
				if( msg->size != sizeof(NetMessage) || msg->protocol != NETMESSAGE_PROTOCOL ){
					QTils_LogMsgEx( "%s: ignoring NetMessage with size %hu!=%hu and/or protocol %hu!=%hu",
								__FUNCTION__,
								msg->size, sizeof(NetMessage), msg->protocol, NETMESSAGE_PROTOCOL );
				}
				else{
					NetMessageToLogMsg( __FUNCTION__, NULL, msg );
					msg->flags.category = qtvod_Confirmation;
					SendMessageToNet( sServer, msg, SENDTIMEOUT, FALSE, __FUNCTION__ );
					if( msg->flags.type == qtvod_Quit ){
						[[NSApplication sharedApplication] terminate:theStream];
					}
				}
			}
			break;
		}
		default:
			break;
	}
}
@end

void SendErrorHandler( size_t errors )
{
	if( errors > 5 ){
		CloseCommClient(&sServer);
		QTils_Log( __FILE__, __LINE__, @"%u message sending errors (last %d '%s'): closing communications channel",
				errors, errSock, errSockText(errSock) );
		PostMessage( "QTVOD", lastSSLogMsg );
	}
}

// called from the application's applicationShouldTerminate handler:
void commsCleanUp()
{ id delg;
	if( sServer != NULLSOCKET ){
//		SendNetCommandOrNotification( sServeur, qtvod_Quit, qtvod_Notification );
		CloseCommClient(&sServer);
	}
	[nsReadServer close];
	[nsReadServer removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
	delg = [nsReadServer delegate];
	[nsReadServer setDelegate:nil];
	[delg release];
	[nsReadServer release];
	[nsWriteServer close];
	[nsWriteServer release];
	if( ipAddress ){
		QTils_free(ipAddress);
	}
	if( assocDataFileName ){
		QTils_free(assocDataFileName);
	}
}

void commsInit()
{
	InitCommClient( &sServer, ipAddress, ServerPortNr, ClientPortNr, 50 );
	if( sServer != NULLSOCKET ){
		CFStreamCreatePairWithSocket( NULL, sServer, (void*) &nsReadServer, (void*) &nsWriteServer );
		[nsReadServer retain]; [nsWriteServer retain];
		[nsReadServer setDelegate:[[QTVODStreamDelegate alloc] init] ];
		[nsReadServer scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		[nsReadServer open]; [nsWriteServer open];
		QTils_Log( __FILE__, __LINE__, @"Created NSStream pair (%@,%@)", nsReadServer, nsWriteServer );
		errSock = 0;
		SendErrors = ReceiveErrors = 0;
		HandleSendErrors = SendErrorHandler;
	}
}

void ParseArgs( int argc, char *argv[] )
{ int arg;
  __block BOOL argError = NO;
  char *valueStr;
  BOOL (^CheckOptArg)(int, char*, char**) = ^ BOOL (int idx, char *pattern, char **value ){
    BOOL ret = NO;
    size_t len = (pattern)? strlen(pattern) : 0;
	  if( len && strncmp( argv[idx], pattern, len ) == 0 ){
		  if( argv[idx][len] == '=' ){
			  *value = &argv[idx][len+1];
			  ret = YES;
		  }
		  else{
			  argError = YES;
			  PostMessage( pattern, "Argument requires a value (arg=value)" );
		  }
	  }
	  return ret;
  };

	arg = 1;
	while( arg < argc ){
		argError = NO;
		if( strcmp( argv[arg], "-logwindow" ) == 0 ){
		  extern BOOL doLogging;
			doLogging = NO;
			[[NSApplication sharedApplication] toggleLogging:NULL];
			sleep(1);
		}
		else if( strncmp( argv[arg], "-client", 7 ) == 0 ){
			if( argv[arg][7] == '=' ){
				ipAddress = QTils_strdup(&argv[arg][8]);
			}
			else{
				ipAddress = QTils_strdup("127.0.0.1");
			}
			commsInit();
		}
		else if( CheckOptArg( arg, "-assocData", &valueStr ) ){
			if( *valueStr ){
				assocDataFileName = QTils_strdup(valueStr);
			}
			else{
				assocDataFileName = QTils_strdup("*FromVODFile*");
			}
		}
		else if( strcmp( argv[arg], "-attendVODDescription" ) == 0 ){
			if( sServer == NULLSOCKET ){
				PostMessage( "QTVOD", "No server connected to point us to a video to open!" );
			}
			return;
		}
		else if( CheckOptArg( arg, "-freq", &valueStr ) ){
			sscanf( valueStr, "%lf", &globalVD.preferences.frequency );
			QTils_LogMsgEx( "-freq=%s -> %g", valueStr, globalVD.preferences.frequency );
		}
		else if( CheckOptArg( arg, "-timeZone", &valueStr ) ){
			sscanf( valueStr, "%lf", &globalVD.preferences.timeZone );
			QTils_LogMsgEx( "-timeZone=%s -> %g", valueStr, globalVD.preferences.timeZone );
		}
		else if( CheckOptArg( arg, "-DST", &valueStr ) ){
			globalVD.preferences.DST = (strcasecmp( valueStr, "true" ) == 0);
			QTils_LogMsgEx( "-DST=%s -> %d", valueStr, globalVD.preferences.DST );
		}
		else if( CheckOptArg( arg, "-hFlip", &valueStr ) ){
			globalVD.preferences.flipLeftRight = (strcasecmp( valueStr, "true" ) == 0);
			QTils_LogMsgEx( "-hFlip=%s -> %d", valueStr, globalVD.preferences.flipLeftRight );
		}
		else if( CheckOptArg( arg, "-VMGI", &valueStr ) ){
			globalVD.preferences.useVMGI = (strcasecmp( valueStr, "true" ) == 0);
			QTils_LogMsgEx( "-VMGI=%s -> %d", valueStr, globalVD.preferences.useVMGI );
		}
		else if( CheckOptArg( arg, "-DST", &valueStr ) ){
			globalVD.preferences.DST = (strcasecmp( valueStr, "true" ) == 0);
			QTils_LogMsgEx( "-DST=%s -> %d", valueStr, globalVD.preferences.DST );
		}
		else if( CheckOptArg( arg, "-log", &valueStr ) ){
			globalVD.preferences.log = (strcasecmp( valueStr, "true" ) == 0);
			QTils_LogMsgEx( "-log=%s -> %d", valueStr, globalVD.preferences.log );
		}
		else if( CheckOptArg( arg, "-scale", &valueStr ) ){
			sscanf( valueStr, "%lf", &globalVD.preferences.scale );
			QTils_LogMsgEx( "-scale=%s -> %g", valueStr, globalVD.preferences.scale );
		}
		else if( CheckOptArg( arg, "-chForward", &valueStr ) ){
			globalVD.preferences.channels.forward = atoi(valueStr);
			QTils_LogMsgEx( "-chForward=%s -> %d", valueStr, globalVD.preferences.channels.forward );
		}
		else if( CheckOptArg( arg, "-chPilot", &valueStr ) ){
			globalVD.preferences.channels.pilot = atoi(valueStr);
			QTils_LogMsgEx( "-chPilot=%s -> %d", valueStr, globalVD.preferences.channels.pilot );
		}
		else if( CheckOptArg( arg, "-chLeft", &valueStr ) ){
			globalVD.preferences.channels.left = atoi(valueStr);
			QTils_LogMsgEx( "-chLeft=%s -> %d", valueStr, globalVD.preferences.channels.left );
		}
		else if( CheckOptArg( arg, "-chRight", &valueStr ) ){
			globalVD.preferences.channels.right = atoi(valueStr);
			QTils_LogMsgEx( "-chRight=%s -> %d", valueStr, globalVD.preferences.channels.right );
		}
		else if( CheckOptArg( arg, "-fcodec", &valueStr ) ){
			QTils_free(globalVD.preferences.codec);
			globalVD.preferences.codec = QTils_strdup(valueStr);
			QTils_LogMsgEx( "-fcodec=%s -> %s", valueStr, globalVD.preferences.codec );
		}
		else if( CheckOptArg( arg, "-fbitrate", &valueStr ) ){
			QTils_free(globalVD.preferences.bitRate);
			globalVD.preferences.bitRate = QTils_strdup(valueStr);
			QTils_LogMsgEx( "-fbitrate=%s -> %s", valueStr, globalVD.preferences.bitRate );
		}
		else if( CheckOptArg( arg, "-fsplit", &valueStr ) ){
			globalVD.preferences.splitQuad = (strcasecmp( valueStr, "true" ) == 0);
			QTils_LogMsgEx( "-fsplit=%s -> %d", valueStr, globalVD.preferences.splitQuad );
		}
		else if( !argError ){
			NSLog( @"We'll open %s", argv[arg] );
		}
		arg++;
	}
}