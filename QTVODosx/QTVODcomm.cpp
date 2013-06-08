/*!
 *  @file QTVODcomm.c
 *  QTVODosx
 *
 *  Created by René J.V. Bertin on 20130603.
 *  Copyright 2013 RJVB. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "QTVODcomm.h"
#include "QTVOD.h"
#include "QTilities.h"
#include "NaN.h"
#include "CritSectEx/CritSectEx.h"

typedef struct VDCommon1 {
	VODDESCRIPTIONCOMMON1
} VDCommon1;
typedef struct VDCommon2 {
	VODDESCRIPTIONCOMMON2
} VDCommon2;

static CritSectEx *SendMutex = NULL;
static CritSectEx *ReceiveMutex = NULL;

size_t SendErrors = 0, ReceiveErrors = 0;
CommErrorHandler HandleSendErrors = NULL, HandleReceiveErrors = NULL;

VODDescription *VODDescriptionFromStatic( VODDescription *target, StaticVODDescription *descr )
{
	if( descr ){
		if( target ){
			memcpy( target, descr, sizeof(VDCommon1) );
			if( target->codec ){
				free(target->codec);
			}
			if( target->bitRate ){
				free(target->bitRate);
			}
			target->codec = (descr->codec[0])? strdup(descr->codec) : NULL;
			target->bitRate = (descr->bitRate[0])? strdup(descr->bitRate) : NULL;
			memcpy( &target->splitQuad, &descr->splitQuad, sizeof(VDCommon2) );
			return target;
		}
	}
	return NULL;
}

StaticVODDescription *VODDescriptionToStatic( VODDescription *descr, StaticVODDescription *target )
{
	if( descr ){
		if( target ){
			memcpy( target, descr, sizeof(VDCommon1) );
			if( descr->codec ){
				strncpy( target->codec, descr->codec, sizeof(target->codec) );
				target->codec[sizeof(target->codec) - 1] = '\0';
			}
			if( descr->bitRate ){
				strncpy( target->bitRate, descr->bitRate, sizeof(target->bitRate) );
				target->bitRate[sizeof(target->bitRate) - 1] = '\0';
			}
			memcpy( &target->splitQuad, &descr->splitQuad, sizeof(VDCommon2) );
			return target;
		}
	}
	return NULL;
}

BOOL InitCommClient( SOCK *clnt, char *address, unsigned short serverPortNr, unsigned short clientPortNr, int timeOutMS )
{ BOOL fatal, ret = FALSE;
  int err;
	if( InitIP() && CreateClient( clnt, clientPortNr, TRUE ) ){
		QTils_LogMsgEx( "Client socket opened on port %hu", clientPortNr );
		if( ConnectToServer( *clnt, serverPortNr, "", address, timeOutMS, &fatal ) ){
			QTils_LogMsgEx( "Connected to server \"%s:%hu\"", address, serverPortNr );
			if( !SendMutex ){
				SendMutex = new CritSectEx(4000);
			}
			if( !ReceiveMutex ){
				ReceiveMutex = new CritSectEx(4000);
			}
			ret = TRUE;
		}
		else{
			err = errSock;
			QTils_LogMsgEx( "Failure connecting to server \"%s:%hu\" (err=%d \"%s\")",
						address, serverPortNr, err, errSockText(err) );
			CloseClient(clnt);
		}
	}
	else{
		QTils_LogMsgEx( "Failure opening client socket on port %hu (err=%d \"%s\")",
					clientPortNr, errSock, errSockText(errSock) );
	}
	return ret;
}

void CloseCommClient( SOCK *clnt )
{
	if( *clnt != NULLSOCKET ){
		CloseConnectionToServer(clnt);
		CloseClient(clnt);
		delete SendMutex; SendMutex = NULL;
		delete ReceiveMutex; ReceiveMutex = NULL;
	}
}

BOOL InitCommServer( SOCK *srv, unsigned short portNr )
{ BOOL ret;
	if( srv && InitIP() && CreateServer( srv, portNr, TRUE ) ){
		QTils_LogMsgEx( "Server socket opened on port %hu", portNr );
		if( !SendMutex ){
			SendMutex = new CritSectEx(4000);
		}
		if( !ReceiveMutex ){
			ReceiveMutex = new CritSectEx(4000);
		}
		ret = TRUE;
	}
	else{
		QTils_LogMsgEx( "Failure opening server socket on port %hu (err=%d \"%s\")",
					portNr, errSock, errSockText(errSock) );
		ret = FALSE;
	}
	return ret;
}

BOOL ServerCheckForClient( SOCK srv, SOCK *clnt, int timeOutMs, BOOL block )
{ BOOL ret = FALSE;
	if( srv != NULLSOCKET && clnt && *clnt == NULLSOCKET ){
		if( WaitForClientConnection( srv, timeOutMs, block, clnt ) ){
			QTils_LogMsg( "A client has connected" );
			ret = TRUE;
		}
	}
	return ret;
}

void CloseCommServer( SOCK *srv, SOCK *clnt )
{
	CloseServer(clnt);
	CloseServer(srv);
	delete SendMutex; SendMutex = NULL;
	delete ReceiveMutex; ReceiveMutex = NULL;
}

BOOL SendMessageToNet( SOCK ss, NetMessage *msg, int timeOutMs, BOOL block, const char *caller )
{ BOOL ret = FALSE;
	if( msg && ss != NULLSOCKET ){
	  CritSectEx::Scope scope(SendMutex);
		errSock = 0;
		msg->size = sizeof(*msg);
		msg->protocol = NETMESSAGE_PROTOCOL;
#ifdef COMMTIMING
		msg->sentTime = HRTime_Time();
#endif
		ret = BasicSendNetMessage( ss, msg, sizeof(NetMessage), timeOutMs, block );
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
	return ret;
}

static inline void receiveError()
{
	ReceiveErrors += 1;
	if( HandleReceiveErrors ){
		HandleReceiveErrors(ReceiveErrors);
	}
}

BOOL ReceiveMessageFromNet( SOCK ss, NetMessage *msg, int timeOutMs, BOOL block, const char *caller )
{ BOOL ret = FALSE;
	if( msg && ss != NULLSOCKET ){
	  CritSectEx::Scope scope(ReceiveMutex);
		ret = BasicReceiveNetMessage( ss, msg, sizeof(NetMessage), timeOutMs, block );
		if( ret ){
			if( msg->size != sizeof(NetMessage) || msg->protocol != NETMESSAGE_PROTOCOL ){
				QTils_LogMsgEx(
							"ReceiveMessageFromNet: ignoring NetMessage with size %hu!=%hu and/or protocol %hu!=%hu",
							msg->size, sizeof(NetMessage), msg->protocol, NETMESSAGE_PROTOCOL );
				ret = FALSE;
				receiveError();
			}
#ifdef COMMTIMING
			msg->recdTime = HRTime_Time();
#endif
		}
		else{
			if( errSock ){
				NetMessageToLogMsg( caller, "Receive error)", msg );
				receiveError();
				errSock = 0;
			}
		}
	}
	return ret;
}

char *NetMessageToString(NetMessage *msg)
{ static String256 str;
  const char *c;
	if( !msg ){
		return NULL;
	}
	switch( msg->flags.type ){
		case qtvod_Open :
			c = "Open";
			break;
		case qtvod_Start :
			c = "Start";
			break;
		case qtvod_Stop :
			c = "Stop";
			break;
		case qtvod_Close :
			c = "Close";
			break;
		case qtvod_Reset :
			c = "Reset";
			break;
		case qtvod_Quit :
			c = "Quit";
			break;
		case qtvod_GotoTime :
			c = "GotoTime";
			break;
		case qtvod_GetTime :
			c = "GetTime";
			break;
		case qtvod_GetStartTime :
			c = "GetStartTime";
			break;
		case qtvod_GetDuration :
			c = "GetDuration";
			break;
		case qtvod_OK :
			c = "OK";
			break;
		case qtvod_Err :
			c = "Error";
			break;
		case qtvod_CurrentTime :
			c = "CurrentTime";
			break;
		case qtvod_StartTime :
			c = "StartTime";
			break;
		case qtvod_Duration :
			c = "Duration";
			break;
		case qtvod_NewChapter :
			c = "NewChapter";
			break;
		case qtvod_GetChapter :
			c = "GetChapter";
			break;
		case qtvod_Chapter :
			c = "Chapter";
			break;
		case qtvod_MarkIntervalTime :
			c = "MarkIntervalTime";
			break;
		case qtvod_GetLastInterval :
			c = "GetLastInterval";
			break;
		case qtvod_LastInterval :
			c = "LastInterval";
			break;
		case qtvod_MovieFinished :
			c = "MovieFinished";
			break;
		case qtvod_GetTimeSubscription :
			c = "GetTimeSubscription";
			break;
		default:
			c = "<??>";
			break;
	}
	switch( msg->flags.category ){
		case qtvod_Command:
			snprintf( str, sizeof(str), "%s (command)", c );
			break;
		case qtvod_Notification:
			snprintf( str, sizeof(str), "%s (notification)", c );
			break;
		case qtvod_Confirmation:
			snprintf( str, sizeof(str), "%s (confirmation)", c );
			break;
		case qtvod_Subscription:
			snprintf( str, sizeof(str), "%s (subscription)", c );
			break;
	}
#ifdef COMMTIMING
	if( msg->sentTime >= 0 && msg->recdTime >= 0 ){
	  size_t len = strlen(str);
		snprintf( &str[len], sizeof(str) - len, " [S@%gs R@%gs dt=%gs]",
			    msg->sentTime, msg->recdTime, msg->recdTime - msg->sentTime )
	}
#endif
	return str;
}

void NetMessageToLogMsg( const char *title, const char *caption, NetMessage *msg )
{ const char *tType;
	if( msg ){
		tType = (msg->data.boolean)? "(abs)" : "(rel)";
	}
	msg->data.URN[sizeof(msg->data.URN)-1] = '\0';
	switch( msg->flags.type ){
		case qtvod_Open:
			if( caption && *caption ){
				QTils_LogMsgEx(
					"%s %s: %s fichier \"%s\", freq=%gHz scale=%g Tzone=%g DST=%hd flipGaucheDroite=%hd canal FW=%d Dr=%d L=%d R=%d",
					title, caption, NetMessageToString(msg),
					msg->data.URN, msg->data.description.frequency, msg->data.description.scale,
					msg->data.description.timeZone, msg->data.description.DST, msg->data.description.flipLeftRight,
					msg->data.description.channels.forward, msg->data.description.channels.pilot,
					msg->data.description.channels.left, msg->data.description.channels.right );
			}
			else{
				QTils_LogMsgEx(
					"%s: %s fichier \"%s\", freq=%gHz scale=%g Tzone=%g DST=%hd flipGaucheDroite=%hd canal FW=%d Dr=%d L=%d R=%d",
					title, NetMessageToString(msg),
					msg->data.URN, msg->data.description.frequency, msg->data.description.scale,
					msg->data.description.timeZone, msg->data.description.DST, msg->data.description.flipLeftRight,
					msg->data.description.channels.forward, msg->data.description.channels.pilot,
					msg->data.description.channels.left, msg->data.description.channels.right );
			}
			break;
		case qtvod_Start:
		case qtvod_Stop:
		case qtvod_Close:
		case qtvod_Reset:
		case qtvod_Quit:
		case qtvod_GetTime:
		case qtvod_GetStartTime:
		case qtvod_GetDuration:
		case qtvod_GetLastInterval:
		case qtvod_OK:
			if( caption && *caption ){
				QTils_LogMsgEx( "%s %s: %s", title, caption, NetMessageToString(msg) );
			}
			else{
				QTils_LogMsgEx( "%s: %s", title, NetMessageToString(msg) );
			}
			break;
		case qtvod_MovieFinished:
			if( caption && *caption ){
				QTils_LogMsgEx( "%s %s: %s \"%s\" canal #%d",
							title, caption, NetMessageToString(msg), msg->data.URN, msg->data.iVal1 );
			}
			else{
				QTils_LogMsgEx( "%s: %s \"%s\" canal #%d",
							title, NetMessageToString(msg), msg->data.URN, msg->data.iVal1 );
			}
			break;
		case qtvod_Err:
			if( caption && *caption ){
				QTils_LogMsgEx( "%s %s: %s \"%s\" error=%d",
							title, caption, NetMessageToString(msg), msg->data.URN, msg->data.error );
			}
			else{
				QTils_LogMsgEx( "%s: %s \"%s\" error=%d",
							title, NetMessageToString(msg), msg->data.URN, msg->data.error );
			}
			break;
		case qtvod_GotoTime:
		case qtvod_CurrentTime:
		case qtvod_StartTime:
		case qtvod_Duration:
		case qtvod_MarkIntervalTime:
		case qtvod_LastInterval:
			if( caption && *caption ){
				QTils_LogMsgEx( "%s %s: %s t=%gs %s", title, caption, NetMessageToString(msg), msg->data.val1, tType );
			}
			else{
				QTils_LogMsgEx( "%s: %s t=%gs %s", title, NetMessageToString(msg), msg->data.val1, tType );
			}
			break;
		case qtvod_GetTimeSubscription:
			if( caption && *caption ){
				QTils_LogMsgEx( "%s %s: %s interval=%gs %s", title, caption, NetMessageToString(msg), msg->data.val1, tType );
			}
			else{
				QTils_LogMsgEx( "%s: %s interval=%gs %s", title, NetMessageToString(msg), msg->data.val1, tType );
			}
			break;
		case qtvod_NewChapter:
		case qtvod_GetChapter:
		case qtvod_Chapter:
			if( caption && *caption ){
				QTils_LogMsgEx( "%s %s: %s title=\"%s\" #%d start=%gs d=%gs %s",
							title, caption, NetMessageToString(msg),
							msg->data.URN, msg->data.iVal1, msg->data.val1, msg->data.val2, tType );
			}
			else{
				QTils_LogMsgEx( "%s: %s title=\"%s\" #%d start=%gs d=%gs %s",
							title, NetMessageToString(msg),
							msg->data.URN, msg->data.iVal1, msg->data.val1, msg->data.val2, tType );
			}
			break;
		default:
			QTils_LogMsgEx( "%s: <??>", (title)? title : "<??>" );
			break;
	}
}

#pragma mark Message construction functions

#ifdef COMMTIMING
#	define MSGINIT(msg,t,c)	msg->flags.type=qtvod_##t , msg->flags.category=qtvod##c , msg->sentTime=-1
#else
#	define MSGINIT(msg,t,c)	msg->flags.type=qtvod_##t , msg->flags.category=qtvod_##c
#endif

void msgOpenFile( NetMessage *msg, const char *URL, VODDescription *descr )
{
	if( msg && URL && descr ){
		MSGINIT( msg, Open, Command );
		strncpy( msg->data.URN, URL, sizeof(msg->data.URN) );
		msg->data.URN[sizeof(msg->data.URN)-1] = '\0';
		VODDescriptionToStatic( descr, &msg->data.description );
	}
}

void msgPlayMovie(NetMessage *msg)
{
	MSGINIT( msg, Start, Command );
}

void msgStopMovie(NetMessage *msg)
{
	MSGINIT( msg, Stop, Command );
}

void msgCloseMovie(NetMessage *msg)
{
	MSGINIT( msg, Close, Command );
}

void msgResetQTVOD( NetMessage *msg, BOOL complete )
{
	MSGINIT( msg, Reset, Command );
	msg->data.boolean = complete;
}

void msgQuitQTVOD(NetMessage *msg)
{
	MSGINIT( msg, Quit, Command );
}

void msgGotoTime( NetMessage *msg, double t, BOOL absolute )
{
	MSGINIT( msg, GotoTime, Command );
	msg->data.val1 = t;
	msg->data.boolean = absolute;
}

void msgGetTime( NetMessage *msg, BOOL absolute )
{
	MSGINIT( msg, GetTime, Command );
	msg->data.boolean = absolute;
}

void msgGetTimeSubscription( NetMessage *msg, double interval, BOOL absolute )
{
	MSGINIT( msg, GetTimeSubscription, Command );
	msg->data.val1 = interval;
	msg->data.boolean = absolute;
}

void msgGetStartTime(NetMessage *msg)
{
	MSGINIT( msg, GetStartTime, Command );
}

void msgGetDuration(NetMessage *msg)
{
	MSGINIT( msg, GetDuration, Command );
}

void msgGetChapter( NetMessage *msg, int32_t idx )
{
	MSGINIT( msg, GetChapter, Command );
	if( idx < 0 ){
		strcpy( msg->data.URN, "<list all>" );
	}
	else{
		strcpy( msg->data.URN, "<in retrieval>" );
	}
	msg->data.iVal1 = idx;
	set_NaN(msg->data.val1);
	set_NaN(msg->data.val2);
	msg->data.boolean = FALSE;
}

void msgNewChapter( NetMessage *msg, const char *title, double startTime, double duration, BOOL absolute )
{
	MSGINIT( msg, NewChapter, Command );
	strncpy( msg->data.URN, title, sizeof(msg->data.URN) );
	msg->data.URN[sizeof(msg->data.URN)-1] = '\0';
	msg->data.iVal1 = -999;
	msg->data.val1 = startTime;
	msg->data.val2 = duration;
	msg->data.boolean = absolute;
}

void msgMarkIntervalTime( NetMessage *msg, BOOL reset )
{
	MSGINIT( msg, MarkIntervalTime, Command );
	msg->data.boolean = reset;
}

void msgGetLastInterval(NetMessage *msg)
{
	MSGINIT( msg, GetLastInterval, Command );
}

#pragma mark Reply message construction functions

#ifdef COMMTIMING
#	define REPLYINIT(msg,t,c)	msg->flags.type=qtvod_##t , msg->flags.category=c , msg->sentTime=-1
#else
#	define REPLYINIT(msg,t,c)	msg->flags.type=qtvod_##t , msg->flags.category=c
#endif

BOOL replyCurrentTime( NetMessage *reply, NetMessageCategory cat, QTMovieWindowH wih, BOOL absolute )
{ double t;
	if( QTMovieWindowGetTime( wih, &t, absolute ) == noErr ){
		REPLYINIT( reply, CurrentTime, cat );
		reply->data.val1 = t;
		if( absolute ){
			reply->data.val2 = (*wih)->info->TCframeRate;
		}
		else{
			reply->data.val2 = (*wih)->info->frameRate;
		}
		reply->data.boolean = absolute;
		return TRUE;
	}
	return FALSE;
}

BOOL replyStartTime( NetMessage *reply, NetMessageCategory cat, QTMovieWindowH wih )
{
	if( QTMovieWindowH_isOpen(wih) ){
		REPLYINIT( reply, StartTime, cat );
		reply->data.val1 = (*wih)->info->startTime;
		reply->data.val2 = (*wih)->info->TCframeRate;
		reply->data.boolean = TRUE;
		return TRUE;
	}
	return FALSE;
}

BOOL replyDuration( NetMessage *reply, NetMessageCategory cat, QTMovieWindowH wih )
{
	if( QTMovieWindowH_isOpen(wih) ){
		REPLYINIT( reply, Duration, cat );
		reply->data.val1 = (*wih)->info->duration;
		reply->data.val2 = (*wih)->info->frameRate;
		// duration is always a relative time (i.e. w.r.t. the movie onset)
		reply->data.boolean = FALSE;
		return TRUE;
	}
	return FALSE;
}

BOOL replyChapter( NetMessage *reply, NetMessageCategory cat, const char *title, int32_t idx,
			   double startTime, double duration )
{
	if( title && *title && startTime >= 0 && duration >= 0 ){
		REPLYINIT( reply, Chapter, cat );
		strncpy( reply->data.URN, title, sizeof(reply->data.URN) );
		reply->data.URN[sizeof(reply->data.URN) - 1] = '\0';
		reply->data.iVal1 = idx;
		reply->data.val1 = startTime;
		reply->data.val2 = duration;
		// durations are always relative:
		reply->data.boolean = FALSE;
		return TRUE;
	}
	return FALSE;
}

BOOL replyLastInterval( NetMessage *reply, NetMessageCategory cat, double dt )
{
	REPLYINIT( reply, LastInterval, cat );
	reply->data.val1 = dt;
	// an interval is always relative:
	reply->data.boolean = FALSE;
	return TRUE;
}

#pragma mark Transceiving functions

void SendNetCommandOrNotification( SOCK ss, NetMessageType type, NetMessageCategory cat )
{ NetMessage msg;
	if( ss != NULLSOCKET ){
		msg.flags.type = type;
		msg.flags.category = cat;
		SendMessageToNet( ss, &msg, SENDTIMEOUT, FALSE, "SendNetCommandOrNotification" );
	}
}

void SendNetErrorNotification( SOCK ss, const char *txt, ErrCode err )
{ NetMessage msg;
	if( ss != NULLSOCKET ){
		MSGINIT( (&msg), Err, Notification );
		if( *txt ){
			strncpy( msg.data.URN, txt, sizeof(msg.data.URN) );
			msg.data.URN[sizeof(msg.data.URN)-1] = '\0';
		}
		msg.data.error = err;
		SendMessageToNet( ss, &msg, SENDTIMEOUT, FALSE, "SendNetErrorNotification" );
	}
}