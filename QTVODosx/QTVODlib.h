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

extern char *ipAddress;
extern SOCK sServer;

extern void commsCleanUp();
extern void ParseArgs( int argc, char *argv[] );

#define _QTVODLIB_H
#endif