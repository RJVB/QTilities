/*!
 *  @file copyright.h
 *
 *  Created by René J.V. Bertin on 20100723.
 *  Copyright 2010 INRETS / RJVB. All rights reserved.
 *
 */


#ifndef _RIGHTS_
#define _RIGHTS_

#define COPYRIGHT "\015\013\tQTMovieSink: $Copyright (c) 2010 INRETS / RJVB. All rights reserved.$\015\013"
/* static char copyright[] = COPYRIGHT;	*/

#if !defined(IDENTIFY)

#define STR(name)	# name
#define STRING(name)	STR(name)

#define XG_IDENTIFY()	"QTMovieSink v1.0 '" __FILE__ "'-[" __DATE__ "," __TIME__ "]"

#ifndef SWITCHES
#	ifdef DEBUG
#		define _IDENTIFY(s,i)	static const char *xg_id_string= "$Id: @(#) "XG_IDENTIFY()"-(\015\013\t\t" s "\015\013\t) DEBUG version" i " $"
#	else
#		define _IDENTIFY(s,i)	static const char *xg_id_string= "$Id: @(#) "XG_IDENTIFY()"-(\015\013\t\t" s "\015\013\t)" i " $"
#	endif
#else
  /* SWITCHES contains the compiler name and the switches given to the compiler.	*/
#	define _IDENTIFY(s,i)	static const char *xg_id_string= "$Id: @(#) "XG_IDENTIFY()"-(\015\013\t\t" s "\015\013\t)["SWITCHES"]" " $"
#endif

#define __IDENTIFY(s,i)\
static const char *xg_id_string_stub(){ _IDENTIFY(s,i);\
	static char called=0;\
	if( !called){\
		called=1;\
		return(xg_id_string_stub());\
	}\
	else{\
		called= 0;\
		return(xg_id_string);\
	}}

#ifdef __GNUC__
#	define IDENTIFY(s)	__attribute__((used)) __IDENTIFY(s," (gcc-" STRING(__GNUC__) ")")
#elif defined(_MSC_VER)
#	define IDENTIFY(s)	__IDENTIFY(s," (MSVC-" STRING(_MSC_VER) ")")
#else
#	define IDENTIFY(s)	__IDENTIFY(s," (cc)")
#endif

#endif


#define C__IDENTIFY(s,i)	static char *cident= s

#if !defined(DEBUG)

#	define C___IDENTIFY(s,i)\
static const char *cident_stub(){ C__IDENTIFY(s,i);\
	static char called=0;\
	if( !called){\
		called=cident;\
		return(cident_stub());\
	}\
	else{\
		called= 0;\
		return(cident);\
	}}


#	else
#		define C___IDENTIFY(s,i) C__IDENTIFY(s,i);
#	endif

#ifdef __GNUC__
#	define C_IDENTIFY(s)	__attribute__((used)) C__IDENTIFY(s," (gcc-" STRING(__GNUC__) ")")
#elif defined(_MSC_VER)
#	define C_IDENTIFY(s)	C__IDENTIFY(s," (MSVC-" STRING(_MSC_VER) ")")
#else
#	define C_IDENTIFY(s)	C__IDENTIFY(s," (cc)")
#endif


#endif /* _RIGHTS_ */
