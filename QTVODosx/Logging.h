/*
 *  Logging.h
 *
 *  Created by René J.V. Bertin on 20100713.
 *  Copyright 2010 INRETS / RJVB. All rights reserved.
 *
 */

#ifndef _LOGGING_H

#	include <libgen.h>
#	ifndef _PCLOGCONTROLLER_H
		extern int PCLogStatus( void*, void*, ... );
#	endif
	extern void *qtLogName();
	extern void *nsString(char *s);
#	define	cLog(d,f,...)	PCLogStatus((d),basename(__FILE__),__LINE__,nsString((char*)(f)), ##__VA_ARGS__)
#	define	Log(d,f,...)	PCLogStatus((d),basename(__FILE__),__LINE__,(f), ##__VA_ARGS__)
#	define	qtLogPtr		qtLogName()


#define _LOGGING_H
#endif
