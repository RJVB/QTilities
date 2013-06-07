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

extern "C" int QTils_Log(const char *fileName, int lineNr, NSString *format, ... );
extern "C" BOOL QTils_LogSetActive(BOOL);

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

inline QTVOD *getActiveQTVOD()
{ NSWindow *active = NULL;
	// obtain the app's key window, or else the frontmost window
	if( !(active = [NSApp keyWindow]) && [[NSApp orderedWindows] count] ){
		active = [[NSApp orderedWindows] objectAtIndex:0];
	}
	// if an eligible window was obtained, retrieve the corresponding document (a QTVODWindow)
	if( active ){
	  QTVODWindow *activeDocument = (QTVODWindow*) [[active windowController] document];
//		QTils_Log( __FILE__, __LINE__, @"Active document: %@", activeDocument );
		if( activeDocument ){
			// the actual document we're interested in here is the QTVOD collection:
			return [activeDocument getQTVOD];
		}
	}
	return NULL;
}

#pragma mark input stream delegate

@interface QTVODStreamDelegate : NSObject<NSStreamDelegate>{
}
- (void) stream:(NSInputStream*)theStream handleEvent:(NSStreamEvent)theEvent;
@end

@implementation QTVODStreamDelegate

// this is the function that gets invoked when there is data available on the input socket
- (void) stream:(NSInputStream*)theStream handleEvent:(NSStreamEvent)theEvent
{
	switch( theEvent ){
		case NSStreamEventEndEncountered:
			commsCleanUp();
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
					commsCleanUp();
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
//					NetMessageToLogMsg( __FUNCTION__, NULL, msg );
					ReplyNetMsg(msg);
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
		QTils_Log( __FILE__, __LINE__, @"%u message sending errors (last %d '%s'): closing communications channel",
				errors, errSock, errSockText(errSock) );
		CloseCommClient(&sServer);
		[nsWriteServer close];
		[nsWriteServer release];
		nsWriteServer = NULL;
		PostMessage( "QTVOD", lastSSLogMsg );
	}
}

#ifdef COMMTIMING
#	define MSGINIT(msg,t,c)	msg->flags.type=qtvod_##t , msg->flags.category=qtvod##c , msg->sentTime=-1
#	define REPLYINIT(msg,t,c)	msg->flags.type=qtvod_##t , msg->flags.category=c , msg->sentTime=-1
#else
#	define MSGINIT(msg,t,c)	msg->flags.type=qtvod_##t , msg->flags.category=qtvod_##c
#	define REPLYINIT(msg,t,c)	msg->flags.type=qtvod_##t , msg->flags.category=c
#endif

BOOL SendMessageToNet( NSOutputStream *ss, NetMessage *msg, const char *caller )
{ BOOL ret = FALSE;
	if( msg && ss && [ss hasSpaceAvailable] ){
		@synchronized(ss){
			errSock = 0;
			msg->size = sizeof(*msg);
			msg->protocol = NETMESSAGE_PROTOCOL;
#ifdef COMMTIMING
			msg->sentTime = HRTime_Time();
#endif
			ret = ([ss write:(const uint8_t*)msg maxLength:(NSUInteger)sizeof(NetMessage)] == sizeof(NetMessage));
			if( ret ){
				NetMessageToLogMsg( caller, "(sent)", msg );
			}
			else{
				NetMessageToLogMsg( caller, "(failure)", msg );
				if( errSock ){
					SendErrors += 1;
					if( HandleSendErrors ){
						HandleSendErrors(SendErrors);
					}
				}
			}
		}
	}
	return ret;
}

void SendNetCommandOrNotification( NSOutputStream *ss, NetMessageType type, NetMessageCategory cat )
{ NetMessage msg;
	if( ss ){
		msg.flags.type = type;
		msg.flags.category = cat;
		SendMessageToNet( ss, &msg, "SendNetCommandOrNotification" );
	}
}

void SendNetErrorNotification( NSOutputStream *ss, const char *txt, ErrCode err )
{ NetMessage msg;
	if( ss ){
		MSGINIT( (&msg), Err, Notification );
		if( *txt ){
			strncpy( msg.data.URN, txt, sizeof(msg.data.URN) );
			msg.data.URN[sizeof(msg.data.URN)-1] = '\0';
		}
		msg.data.error = err;
		SendMessageToNet( ss, &msg, "SendNetErrorNotification" );
	}
}

BOOL replyCurrentTime( NetMessage *reply, NetMessageCategory cat, QTVOD *qv, BOOL absolute )
{
	if( reply && qv ){
		REPLYINIT( reply, CurrentTime, cat );
		reply->data.val1 = [qv getTime:absolute];
		reply->data.val2 = [qv frameRate:absolute];
		reply->data.boolean = absolute;
		return TRUE;
	}
	return FALSE;
}

BOOL replyStartTime( NetMessage *reply, NetMessageCategory cat, QTVOD *qv )
{
	if( reply && qv ){
		REPLYINIT( reply, StartTime, cat );
		reply->data.val1 = [qv startTime];
		reply->data.val2 = [qv frameRate:NO];
		reply->data.boolean = TRUE;
		return TRUE;
	}
	return FALSE;
}

BOOL replyDuration( NetMessage *reply, NetMessageCategory cat, QTVOD *qv )
{
	if( reply && qv ){
		REPLYINIT( reply, Duration, cat );
		reply->data.val1 = [qv duration];
		reply->data.val2 = [qv frameRate:NO];
		// duration is always a relative time (i.e. w.r.t. the movie onset)
		reply->data.boolean = FALSE;
		return TRUE;
	}
	return FALSE;
}

// called from the application's applicationShouldTerminate handler:
void commsCleanUp()
{ id delg;
	if( sServer != NULLSOCKET ){
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
//	CFStreamCreatePairWithSocketToHost( NULL, (CFStringRef) [NSString stringWithUTF8String:ipAddress], ServerPortNr,
//								(CFReadStreamRef*) &nsReadServer, (CFWriteStreamRef*) &nsWriteServer );
	if( sServer != NULLSOCKET ){
		CFStreamCreatePairWithSocket( NULL, sServer, (CFReadStreamRef*) &nsReadServer, (CFWriteStreamRef*) &nsWriteServer );
		[nsReadServer retain]; [nsWriteServer retain];
		[nsReadServer setDelegate:[[QTVODStreamDelegate alloc] init] ];
		[nsReadServer scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		[nsReadServer open]; [nsWriteServer open];
		QTils_Log( __FILE__, __LINE__, @"Created NSStream pair (%@,%@), status=(%d,%d)",
				nsReadServer, nsWriteServer, [nsReadServer streamStatus], [nsWriteServer streamStatus] );
		errSock = 0;
		SendErrors = ReceiveErrors = 0;
		HandleSendErrors = SendErrorHandler;
	}
}

void ParseArgs( int argc, char *argv[] )
{ int arg;
  __block BOOL argError = NO;
  char *valueStr;
  BOOL (^CheckOptArg)(int, const char*, char**) = ^ BOOL (int idx, const char *pattern, char **value ){
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

ErrCode ReplyNetMsg(NetMessage *msg)
{ ErrCode err;
  double t;
  int32_t idx, n;
  BOOL sendDefaultReply;
  NetMessage Netreply;
  QTVOD *qv;

	QTils_LogMsgEx( "Received NetMessage '%s'", NetMessageToString(msg) );

	if( msg->flags.category != qtvod_Command ){
		return noErr;
	}

	msg->data.error = noErr;
	Netreply = *msg;
	Netreply.flags.category = qtvod_Confirmation;
	sendDefaultReply = TRUE;

	switch( msg->flags.type ){
		case qtvod_Close:
			[getActiveQTVOD() CloseVideo:YES];
			break;
		case qtvod_Reset:
			if( (qv = getActiveQTVOD()) ){
				err = [qv ResetVideo:msg->data.boolean];
				if( err == noErr ){
					Netreply.data.val1 = [qv getTime:NO];
					Netreply.data.val2 = [qv frameRate:NO];
				}
				else{
					SendNetErrorNotification( nsWriteServer /*sServer*/, NetMessageToString(msg), err );
					msgCloseMovie(&Netreply);
					[qv close];
					Netreply.flags.category = qtvod_Notification;
				}
			}
			else{
				err = errWindowNotFound;
				SendNetErrorNotification( nsWriteServer /*sServer*/, NetMessageToString(msg), err );
			}
			msg->data.error = err;
			break;
		case qtvod_Quit:
			[[NSApplication sharedApplication] terminate:nsReadServer];
			break;
		case qtvod_Open:{
		  NSURL *absoluteURL = [NSURL URLWithString:[NSString stringWithUTF8String:msg->data.URN]];
			if( absoluteURL ){
				if( (qv = getActiveQTVOD()) ){
					[qv CloseVideo:YES];
					VODDescriptionFromStatic( &globalVD.preferences, &msg->data.description );
					globalVD.changed = YES;
					if( [qv readFromURL:absoluteURL ofType:@"" error:nil] ){
						Netreply.data.description.frequency = msg->data.description.frequency = [qv frameRate:NO];
					}
					else{
						err = [qv openErr];
						SendNetErrorNotification( nsWriteServer /*sServer*/, msg->data.URN, err );
						[qv close];
						return err;
					}
				}
				else{
				  NSError *outError = NULL;
					qv = [QTVOD createWithAbsoluteURL:absoluteURL ofType:@"" forDocument:nil error:&outError];
					if( qv && !outError ){
						Netreply.data.description.frequency = msg->data.description.frequency = [qv frameRate:NO];
					}
					else if( outError ){
						err = [outError code];
						SendNetErrorNotification( nsWriteServer /*sServer*/, msg->data.URN, err );
						[outError release];
						return err;
					}
					
				}
			}
			else{
				QTils_LogMsgEx( "Could not create NSURL from URN='%s'", msg->data.URN );
				err = fnfErr;
				SendNetErrorNotification( nsWriteServer /*sServer*/, msg->data.URN, err );
				return err;
			}
			msg->data.error = err;
			break;
		}
		case qtvod_Stop:
			if( (qv = getActiveQTVOD()) ){
				[qv StopVideoExceptFor:NULL];
				msg->data.val1 = Netreply.data.val1 = [qv getTime:NO];
				msg->data.boolean = Netreply.data.boolean = 0;
			}
			else{
				msg->data.error = err= errWindowNotFound;
			}
			break;
		case qtvod_Start:
			if( (qv = getActiveQTVOD()) ){
				[qv StartVideoExceptFor:NULL];
				msg->data.val1 = Netreply.data.val1 = [qv getTime:NO];
				msg->data.boolean = Netreply.data.boolean = 0;
			}
			else{
				msg->data.error = err= errWindowNotFound;
			}
			break;
		case qtvod_GetTime:
			if( (qv = getActiveQTVOD()) ){
				replyCurrentTime( &Netreply, qtvod_Confirmation, qv, msg->data.boolean );
			}
			else{
				SendNetErrorNotification( nsWriteServer, NetMessageToString(msg), errWindowNotFound );
				return errWindowNotFound;
			}
			break;
		case qtvod_GetTimeSubscription:
			if( (qv = getActiveQTVOD()) ){
				[qv setTimeSubscrInterval:msg->data.val1 absolute:msg->data.boolean];
			}
			else{
				msg->data.error = err= errWindowNotFound;
			}
			break;
		case qtvod_GotoTime:
			if( (qv = getActiveQTVOD()) ){
				[qv SetTimes:msg->data.val1 withRefWindow:NULL absolute:msg->data.boolean];
				if( [qv getTime:msg->data.boolean] != msg->data.val1 ){
					msg->data.error = Netreply.data.error = err = 1;
				}
			}
			else{
				msg->data.error = err= errWindowNotFound;
			}
			break;
		case qtvod_GetStartTime:
			if( (qv = getActiveQTVOD()) ){
				replyStartTime( &Netreply, qtvod_Confirmation, qv );
			}
			else{
				SendNetErrorNotification( nsWriteServer, NetMessageToString(msg), errWindowNotFound );
				return errWindowNotFound;
			}
			break;
		case qtvod_GetDuration:
			if( (qv = getActiveQTVOD()) ){
				replyDuration( &Netreply, qtvod_Confirmation, qv );
			}
			else{
				SendNetErrorNotification( nsWriteServer, NetMessageToString(msg), errWindowNotFound );
				return errWindowNotFound;
			}
			break;
		case qtvod_GetChapter:
			break;
		case qtvod_NewChapter:
			break;
		case qtvod_MarkIntervalTime:
			break;
		case qtvod_GetLastInterval:
			break;
	}
	if( sendDefaultReply ){
		SendMessageToNet( nsWriteServer /*sServer*/, &Netreply, "QTVOD::ReplyNetMsg - client" );
	}
	msg->flags.category = qtvod_Confirmation;
	return msg->data.error;
}