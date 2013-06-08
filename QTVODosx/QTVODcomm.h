/*!
 *  @file QTVODcomm.h
 *  QTVODosx
 *
 *  Created by René J.V. Bertin on 20130603.
 *  Copyright 2013 RJVB. All rights reserved.
 *
 */

#ifndef _QTVODCOMM_H

#include "Chaussette2.h"
#include "QTilities.h"

#ifdef __GNUC__
#	include <stdio.h>
#	include <sys/types.h>
#	define GCC_PACKED	__attribute__ ((packed))
#else
#	define GCC_PACKED	/**/
#endif

#ifdef _MSC_VER
#	pragma pack(push,1)
#endif

#ifndef __OBJC__
#	define BOOL	unsigned char
#endif

typedef struct VODChannels {
	int forward, pilot, left, right;
} GCC_PACKED VODChannels;

#define VODDESCRIPTIONCOMMON1	double frequency, scale, timeZone; \
	BOOL DST, useVMGI, log, flipLeftRight; \
	VODChannels channels;

#define VODDESCRIPTIONCOMMON2	BOOL splitQuad;

typedef char	URLString[1025];
typedef char	Str64[65];
typedef char	String256[256];

/*!	the internal VODDescription structure, which uses strings that are allocated dynamically */
typedef struct VODDescription {
	VODDESCRIPTIONCOMMON1
	char *codec, *bitRate;
	VODDESCRIPTIONCOMMON2
} GCC_PACKED VODDescription;

/*! the VODDescription variant used for communicating with remote clients/servers
 */
typedef struct StaticVODDescription {
	VODDESCRIPTIONCOMMON1
	Str64 codec, bitRate;
	VODDESCRIPTIONCOMMON2
} GCC_PACKED StaticVODDescription;

typedef enum NetMessageCategory
#ifdef MSC_VER
	: char
#endif
	{ qtvod_NoClass = 0,
		qtvod_Command, qtvod_Confirmation, qtvod_Notification, qtvod_Subscription } GCC_PACKED NetMessageCategory;
typedef enum NetMessageType
#ifdef MSC_VER
	: char
#endif
	{ qtvod_NoType = 0,
		/* commands to QTVOD that are also sent back as confirmation: */
		qtvod_Open, qtvod_Start, qtvod_Stop,
		qtvod_Close, qtvod_Reset, qtvod_Quit, qtvod_GotoTime, qtvod_NewChapter,
		qtvod_MarkIntervalTime, qtvod_GetTimeSubscription,
		/* commands that are confirmed via a dedicated message: */
		qtvod_GetTime, qtvod_GetStartTime, qtvod_GetDuration, qtvod_GetChapter,
		qtvod_GetLastInterval,
		/* reply messages */
		qtvod_OK, qtvod_Err,
		qtvod_CurrentTime, qtvod_StartTime, qtvod_Duration, qtvod_Chapter,
		qtvod_LastInterval, qtvod_MovieFinished } GCC_PACKED NetMessageType;

typedef struct NetMessage {
	// message size and protocol version 
	uint16_t size, protocol;
	// message type and class
	// !NB! GCC_PACKED is NOT recursive, it should be specified on sub-structures too!!
	struct {
		NetMessageType		type;
		NetMessageCategory	category;
	} GCC_PACKED flags;
	struct {
		// a message can carry 2 double fl.point values;
		double	val1, val2;
		// a single 32bit integer;
		int32_t	iVal1;
		// a single boolean value;
		BOOL		boolean;
		// a single Universal Resource Name;
		URLString	URN;
		// a single static VODDescription;
		StaticVODDescription	description;
		// and a single error code
		ErrCode	error;
	} GCC_PACKED data;
#ifdef COMMTIMING
	double sentTime, recdTime;
#endif
} GCC_PACKED NetMessage;

typedef void	(*CommErrorHandler)(size_t);

#ifdef _MSC_VER
#	pragma pack(pop)
#endif

extern size_t ReceiveErrors, SendErrors;
extern CommErrorHandler HandleSendErrors, HandleReceiveErrors;

#ifdef __cplusplus
extern "C" {
#endif

extern VODDescription *VODDescriptionFromStatic( VODDescription *target, StaticVODDescription *descr );
extern StaticVODDescription *VODDescriptionToStatic( VODDescription *descr, StaticVODDescription *target );

extern BOOL InitCommClient( SOCK *s, char *address, unsigned short serverPortNr, unsigned short clientPortNr, int timeOutMS );
extern void CloseCommClient( SOCK *clnt );
extern BOOL InitCommServer( SOCK *srv, unsigned short portNr );
extern BOOL ServerCheckForClient( SOCK srv, SOCK *clnt, int timeOutMs, BOOL block );
extern void CloseCommServer( SOCK *srv, SOCK *clnt );

extern BOOL SendMessageToNet( SOCK ss, NetMessage *msg, int timeOutMs, BOOL block, const char *caller );

//! returns a pointer to a static String256
extern char *NetMessageToString(NetMessage *msg);
extern void NetMessageToLogMsg( const char *title, const char *caption, NetMessage *msg );

extern void msgOpenFile( NetMessage *msg, const char *URL, VODDescription *descr );
extern void msgPlayMovie(NetMessage *msg);
extern void msgStopMovie(NetMessage *msg);
extern void msgCloseMovie(NetMessage *msg);
extern void msgResetQTVOD( NetMessage *msg, BOOL complete );
extern void msgQuitQTVOD(NetMessage *msg);
extern void msgGotoTime( NetMessage *msg, double t, BOOL absolute );
extern void msgGetTime( NetMessage *msg, BOOL absolute );
extern void msgGetTimeSubscription( NetMessage *msg, double interval, BOOL absolute );
extern void msgGetStartTime(NetMessage *msg);
extern void msgGetDuration(NetMessage *msg);
extern void msgGetChapter( NetMessage *msg, int32_t idx );
extern void msgNewChapter( NetMessage *msg, const char *title, double startTime, double duration, BOOL absolute );
extern void msgMarkIntervalTime( NetMessage *msg, BOOL reset );
extern void msgGetLastInterval(NetMessage *msg);

extern BOOL replyCurrentTime( NetMessage *reply, NetMessageCategory cat, QTMovieWindowH wih, BOOL absolute );
extern BOOL replyStartTime( NetMessage *reply, NetMessageCategory cat, QTMovieWindowH wih );
extern BOOL replyDuration( NetMessage *reply, NetMessageCategory cat, QTMovieWindowH wih );
extern BOOL replyChapter( NetMessage *reply, NetMessageCategory cat, const char *title, int32_t idx,
			   double startTime, double duration );
extern BOOL replyLastInterval( NetMessage *reply, NetMessageCategory cat, double dt );

extern void SendNetCommandOrNotification( SOCK ss, NetMessageType type, NetMessageCategory cat );
extern void SendNetErrorNotification( SOCK ss, const char *txt, ErrCode err );

#ifdef __cplusplus
}
#endif

#define SENDTIMEOUT	2
#define ServerPortNr	5351
#define ClientPortNr	4351
#define NETMESSAGE_PROTOCOL	3

#define _QTVODCOMM_H
#endif