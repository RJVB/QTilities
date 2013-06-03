/*!
 *  @file QTVODcomm.h
 *  QTVODosx
 *
 *  Created by René J.V. Bertin on 20130603.
 *  Copyright 2013 RJVB. All rights reserved.
 *
 */

#ifndef _QTVODCOMM_H

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
	VODChannels channels; \
	BOOL changed;

#define VODDESCRIPTIONCOMMON2	BOOL splitQuad;

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
	char codec[64], bitRate[64];
	VODDESCRIPTIONCOMMON2
} GCC_PACKED StaticVODDescription;

extern VODDescription *VODDescriptionFromStatic( StaticVODDescription *descr, VODDescription *target );
extern StaticVODDescription *VODDescriptionToStatic( VODDescription *descr, StaticVODDescription *target );

#ifdef _MSC_VER
#	pragma pack(pop)
#endif

#define _QTVODCOMM_H
#endif