#ifndef _NAN_H
#define _NAN_H
/* RJVB: header file with the definitions of NaN and Inf for IEEE double floating point variables.
 */

/* since math.h contains a MAXDOUBLE definition, undef any
 * made in limits.h
 */
#ifdef MAXDOUBLE
#	undef MAXDOUBLE
#endif

  /* conditional inclusion of header files would be so great... Now you will have
   \ to add the symbol defining your environment when necessary....
   */
#if defined (linux) || defined(sgi) || defined(__GNUC__) || defined(_AIX)
#	include "math.h"
#endif

/* Handling of Not_a_Number's (only in IEEE floating-point standard) */

#ifdef linux
#	include <bits/nan.h>
/* #elif _AIX	*/
  /* Doesn't have a nan.h - we don't really need it neither.	*/
/* #else	*/
/* #	include <nan.h>	*/
#endif

#if defined(i386) || defined(__LITTLE_ENDIAN__) || (defined(BYTE_ORDER) && BYTE_ORDER==LITTLE_ENDIAN)
/* definitions for byte-reversed-ordered machines:	*/
typedef union IEEEsfp {
	struct {
		short low, high;
	} l;
	struct {
		unsigned f:23;
		unsigned e:8;
		unsigned s:1;
	} s;
	float f;
} IEEEsfp;

typedef union IEEEfp {
	struct {
		long low, high;
	} l;
	struct {
/* 		unsigned f1:32,f2:20;	*/
		unsigned f2:32,f1:20;
		unsigned e:11;
		unsigned s:1;
	} s;
	double d;
} IEEEfp;
#else
typedef union IEEEsfp {
	struct {
		short high, low;
	} l;
	struct {
		unsigned s:1;
		unsigned e:8;
		unsigned f:23;
	} s;
	float f;
} IEEEsfp;

typedef union IEEEfp {
	struct {
		long high, low;
	} l;
	struct {
		unsigned s:1;
		unsigned e:11;
		unsigned f1:20, f2:32;
	} s;
	double d;
} IEEEfp;
#endif

#define NAN_VAL	0x7ff
#define POS_INF	0x7ff00000
#define NEG_INF	0xfff00000

#if defined (linux) || defined(sgi) || defined(_AIX)
	/* RJVB: these are the machines I currently know for sure of that the definitions
	 \ in this file work. That is, they use IEEE 8-byte double floating point numbers.
	 \ Probably all unix machines do, but I do not have all the cpp machine identifiers
	 \ at hand.
	 */
#else
	/* RJVB: You will have to verify whether or not your machine has IEEE 8-byte floating point numbers!	*/
#endif

#if __GNUC__==4 && __GNUC_MINOR__==0 && (defined(__APPLE_CC__) || defined(__MACH__))
/* 20050808: on this host/compiler combination, it appears to be necessary to use the
 \ system calls isnan() and/or isinf() for reliable detection. This reeks of a compiler error.
 */
#	define USE_HOST_NANFS
#endif

#undef NaN

/* NaN or Inf: (s.e==NAN_VAL and ((s.f1!=0 and s.f2!=0) or (s.f1==0 and s.f2==0)))
 * i.e. s.e==NAN_VAL and (!(s.f1 xor s.f2))
 */
#ifdef USE_HOST_NANFS
#	define NaNorInf(X)	(isnan(X) || isinf(X))
#else
#	define NaNorInf(X)	(((IEEEfp *)&(X))->s.e==NAN_VAL)
#endif

#define I3Ed(X)	((IEEEfp*)&(X))

/* isNaN() only tests for NaN-ness, without returning the relevant bits. It should be marginally faster */
#ifdef USE_HOST_NANFS
#	define isNaN(X)	isnan(X)
#else
#	define isNaN(X)	( (I3Ed(X)->s.e==NAN_VAL) && ((I3Ed(X)->s.f1) || (I3Ed(X)->s.f2)) )
#endif

#ifdef USE_HOST_NANFS
#	define NaN(X)	( (isnan(X))? \
	((I3Ed(X)->s.f1!= 0 )? I3Ed(X)->s.f1 : \
		((I3Ed(X)->s.f2!= 0)? I3Ed(X)->s.f2 : 0)) : 0)
#else
#	define NaN(X)	( (I3Ed(X)->s.e==NAN_VAL)? \
	((I3Ed(X)->s.f1!= 0 )? I3Ed(X)->s.f1 : \
		((I3Ed(X)->s.f2!= 0)? I3Ed(X)->s.f2 : 0)) : 0)
#endif

#ifdef USE_HOST_NANFS
#	define INF(X)	isinf(X)
#else
#	define INF(X)	(((IEEEfp *)&(X))->s.e==NAN_VAL &&\
	((IEEEfp *)&(X))->s.f1== 0 &&\
	((IEEEfp *)&(X))->s.f2== 0)
#endif

#define set_NaN(X)	{IEEEfp *local_IEEEfp=(IEEEfp*)(&(X));\
	local_IEEEfp->s.s= 0; local_IEEEfp->s.e=NAN_VAL ;\
	local_IEEEfp->s.f1= 0xfffff ;\
	local_IEEEfp->s.f2= 0xffffffff;}

#define Inf(X)	((INF(X))?((((IEEEfp *)&(X))->l.high==POS_INF)?1:-1):0)

#define set_Inf(X,S)	{IEEEfp *local_IEEEfp=(IEEEfp*)(&(X));\
	local_IEEEfp->l.high=(S>0)?POS_INF:NEG_INF ;\
	local_IEEEfp->l.low= 0x00000000 ;}

#define set_INF(X)	{IEEEfp *local_IEEEfp=(IEEEfp*)(&(X));\
	local_IEEEfp->s.e=NAN_VAL ;\
	local_IEEEfp->s.f1= 0x00000 ;\
	local_IEEEfp->s.f2= 0x00000000 ;}

#endif	/* !_NAN_H	*/
