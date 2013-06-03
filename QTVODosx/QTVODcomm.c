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

typedef struct VDCommon1 {
	VODDESCRIPTIONCOMMON1
} VDCommon1;
typedef struct VDCommon2 {
	VODDESCRIPTIONCOMMON1
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
			target->codec = strdup(descr->codec);
			target->bitRate = strdup(descr->bitRate);
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
			strncpy( target->codec, descr->codec, sizeof(target->codec) );
			strncpy( target->bitRate, descr->bitRate, sizeof(target->bitRate) );
			memcpy( &target->splitQuad, &descr->splitQuad, sizeof(VDCommon2) );
			return target;
		}
	}
	return NULL;
}
