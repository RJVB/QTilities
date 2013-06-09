/*!
 *  @file QTVODlib.h
 *  QTVODosx
 *
 *  Created by René J.V. Bertin on 20130604.
 *  Copyright 2013 RJVB. All rights reserved.
 *
 */

#ifndef _QTVODLIB_H

#include "Chaussette2.h"

extern SOCK sServer;
extern char *assocDataFileName;

#ifdef __cplusplus
extern "C" {
#endif

extern void commsCleanUp();
extern void commsInit(const char *ipAddress);
extern void ParseArgs( int argc, char *argv[] );

extern QTVOD *getActiveQTVOD();
extern ErrCode ReplyNetMsg(NetMessage *msg);

#ifdef __cplusplus
}
#endif

#define _QTVODLIB_H
#endif