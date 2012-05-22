/*
#### http://www.flipcode.net/archives/vsscanf_for_Win32.shtml ####

I don't how many time I've been irritated by the fact that Microsoft
chose to implement all the v*printf family of routines, but for some
odd reason decided to leave out all the v*scanf routines.

It is all the more frustrating that the functionality is actually
present inside their crt library (in a function called input, which is
not exported).

>From there your choices are as follows:

	- Write your own vsscanf
	- Rip the source out of crt and graft it into your app.
	- Use a third party implementation (trio is a good one, btw).

All these solutions suck: if you choose to write your own, you'll soon
discover how hard it is to write an ANSI conforming scanf, other than
hacking together Frankenstein code that is half yours, and half calling
the native system sscanf when things gets hard (float conversion springs
to mind).

If you choose a third party implementation (crt, trio, glibc, ...), there's
the usual problems: it's usually much bulkier than you'd like, doesn't compile
the way you have your tree setup, comes with hieroglyphic Makefiles, carries crual
and unusual licensing agreements, and basically adds a maintenance burden to your
already complicated work day.
	
Given all this, and the fact that irritation always ends up breeding code,
here's my solution to the problem: since sscanf is available in the system,
I've implemented vsscanf by hacking together a fake stack before hoping into sscanf.

Advantages:
	- It works
	- It is small
	- It has no strings attached
	- In time, it inherits all maintenance work done by MSFT on scanf

Disadvantages:
	- It adds a tiny overhead (browsing the fmt for % signs)
	- It is not portable (although the technique is)
	- But then again the rest of the universe *does* implement
	  vsscanf, so who cares.

Enjoy,

	- mgix

PS: This is written with Visual C++ inline assembly. If someone gets
around to writing the equivalent in the weird gcc inline assembly linguo,
please send it my way. Thanks.

*/

/*
 * vsscanf for Win32
 *
 * Written 5/2003 by <mgix@mgix.com>
 *
 * This code is in the Public Domain
 *
 */

#ifndef _VSSCANF_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdarg.h>

#ifdef _MSC_VER
__declspec(dllimport)
#endif
int vsscanf( const char  *buffer, const char  *format, va_list argPtr );

#ifdef __cplusplus
}
#endif

#define _VSSCANF_H
#endif