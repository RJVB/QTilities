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
#include "QTilities.h"

typedef struct VDCommon1 {
	VODDESCRIPTIONCOMMON1
} VDCommon1;
typedef struct VDCommon2 {
	VODDESCRIPTIONCOMMON2
} VDCommon2;

VODDescription *VODDescriptionFromStatic( StaticVODDescription *descr, VODDescription *target )
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
					clientPortNr, err, errSockText(err) );
	}
	return ret;
}

void CloseCommClient( SOCK *clnt )
{
	if( *clnt != NULLSOCKET ){
		CloseConnectionToServer(clnt);
		CloseClient(clnt);
	}
}

char *NetMessageToString(NetMessage *msg)
{ static String256 str;
  char *c;
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
	switch( msg->flags.class ){
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

void NetMessageToLogMsg( char *title, char *caption, NetMessage *msg )
{ char *tType;
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
				QTils_LogMsgEx( "%s %s: %s \"%s\" erreur=%d",
							title, caption, NetMessageToString(msg), msg->data.URN, msg->data.error );
			}
			else{
				QTils_LogMsgEx( "%s: %s \"%s\" erreur=%d",
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
