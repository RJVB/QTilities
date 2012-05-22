/*
 * Copyright 1993, 1994, 1995, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * This source code ("Source Code") was originally derived from a
 * code base owned by Silicon Graphics, Inc. ("SGI")
 * 
 * LICENSE: SGI grants the user ("Licensee") permission to reproduce,
 * distribute, and create derivative works from this Source Code,
 * provided that: (1) the user reproduces this entire notice within
 * both source and binary format redistributions and any accompanying
 * materials such as documentation in printed or electronic format;
 * (2) the Source Code is not to be used, or ported or modified for
 * use, except in conjunction with OpenGL Performer; and (3) the
 * names of Silicon Graphics, Inc.  and SGI may not be used in any
 * advertising or publicity relating to the Source Code without the
 * prior written permission of SGI.  No further license or permission
 * may be inferred or deemed or construed to exist with regard to the
 * Source Code or the code base of which it forms a part. All rights
 * not expressly granted are reserved.
 * 
 * This Source Code is provided to Licensee AS IS, without any
 * warranty of any kind, either express, implied, or statutory,
 * including, but not limited to, any warranty that the Source Code
 * will conform to specifications, any implied warranties of
 * merchantability, fitness for a particular purpose, and freedom
 * from infringement, and any warranty that the documentation will
 * conform to the program, or any warranty that the Source Code will
 * be error free.
 * 
 * IN NO EVENT WILL SGI BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT
 * LIMITED TO DIRECT, INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES,
 * ARISING OUT OF, RESULTING FROM, OR IN ANY WAY CONNECTED WITH THE
 * SOURCE CODE, WHETHER OR NOT BASED UPON WARRANTY, CONTRACT, TORT OR
 * OTHERWISE, WHETHER OR NOT INJURY WAS SUSTAINED BY PERSONS OR
 * PROPERTY OR OTHERWISE, AND WHETHER OR NOT LOSS WAS SUSTAINED FROM,
 * OR AROSE OUT OF USE OR RESULTS FROM USE OF, OR LACK OF ABILITY TO
 * USE, THE SOURCE CODE.
 * 
 * Contact information:  Silicon Graphics, Inc., 
 * 1600 Amphitheatre Pkwy, Mountain View, CA  94043, 
 * or:  http://www.sgi.com
 */

#ifdef HAS_PERFORMER

/*
 * file: snapwin.c
 * ---------------
 * These routines are internal utility routines for use in test programs
 * 
 * $Revision: 1.1 $
 * $Date: 2000/11/21 21:39:36 $
 */
#include "winixdefs.h"
#include "copyright.h"
IDENTIFY("QTpfuSaveImage: optimised version of SGI Performer's pfuSaveImage with bridge to QuickTime");

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef _MSC_VER
#	include <unistd.h>
#else
#endif

#include <Performer/image.h>
// #include <Performer/pfutil.h>

#define _QTPFUSAVEIMAGE_C
#include "QTpfuSaveImage.h"

#ifdef __LITTLE_ENDIAN
#define PFU_CPACKTORGB(l,r,g,b)		\
	do {				\
	  int val = (int)(l);			\
		(r) = (val >>  0) & 0xff;		\
		(g) = (val >>  8) & 0xff;		\
		(b) = (val >> 16) & 0xff;		\
	} while(0)
#define PFU_CPACKTORGBA(l,r,g,b,a)	\
	do {				\
	  int val = (int)(l);			\
		(r) = (val >>  0) & 0xff;		\
		(g) = (val >>  8) & 0xff;		\
		(b) = (val >> 16) & 0xff;		\
		(a) = (val >> 24) & 0xff;		\
	} while(0)
#else /* OPENGL */
#define PFU_CPACKTORGB(l,r,g,b)		\
	do {				\
	  int val = (int)(l);			\
		(r) = (val >>  24) & 0xff;		\
		(g) = (val >>  16) & 0xff;		\
		(b) = (val >>  8) & 0xff;		\
	} while(0)
#define PFU_CPACKTORGBA(l,r,g,b,a)	\
	do {				\
	  int val = (int)(l);			\
		(r) = (val >>  24) & 0xff;		\
		(g) = (val >>  16) & 0xff;		\
		(b) = (val >>  8) & 0xff;		\
		(a) = (val >>  0) & 0xff;		\
	} while(0)
#endif /* GL type */

#define xfree(x)	if((x)){ free((x)); (x)=NULL; }



void pfuCpackToRGB(unsigned int *l, unsigned short *r, unsigned short *g, unsigned short *b, int n)
{
	while( n >= 8 ){
		PFU_CPACKTORGB(l[0], r[0], g[0], b[0]);
		PFU_CPACKTORGB(l[1], r[1], g[1], b[1]);
		PFU_CPACKTORGB(l[2], r[2], g[2], b[2]);
		PFU_CPACKTORGB(l[3], r[3], g[3], b[3]);
		PFU_CPACKTORGB(l[4], r[4], g[4], b[4]);
		PFU_CPACKTORGB(l[5], r[5], g[5], b[5]);
		PFU_CPACKTORGB(l[6], r[6], g[6], b[6]);
		PFU_CPACKTORGB(l[7], r[7], g[7], b[7]);
		l += 8, r += 8, g += 8, b += 8, n -= 8;
	}

	while( n-- ){
		PFU_CPACKTORGB(l[0], r[0], g[0], b[0]);
		l++, r++, g++, b++;
	}
}

void pfuCpackToRGBA(unsigned int *l, unsigned short *r, unsigned short *g, unsigned short *b, unsigned short *a, int n)
{
	while( n >= 8 ){
		PFU_CPACKTORGBA(l[0], r[0], g[0], b[0], a[0]);
		PFU_CPACKTORGBA(l[1], r[1], g[1], b[1], a[1]);
		PFU_CPACKTORGBA(l[2], r[2], g[2], b[2], a[2]);
		PFU_CPACKTORGBA(l[3], r[3], g[3], b[3], a[3]);
		PFU_CPACKTORGBA(l[4], r[4], g[4], b[4], a[4]);
		PFU_CPACKTORGBA(l[5], r[5], g[5], b[5], a[5]);
		PFU_CPACKTORGBA(l[6], r[6], g[6], b[6], a[6]);
		PFU_CPACKTORGBA(l[7], r[7], g[7], b[7], a[7]);
		l += 8, r += 8, g += 8, b += 8, a += 8, n -= 8;
	}

	while( n-- ){
		PFU_CPACKTORGBA(l[0], r[0], g[0], b[0], a[0]);
		l++, r++, g++, b++, a++;
	}
}

/* the standard error handler calls exit(). we don't want to quit out of 
 * the application */
void my_iopenerror(char *str)
{
    fprintf( stderr, "QTpfuSaveImage: %s", str );
}

extern void i_seterror(void (*)(char *)); /* from libimage.a */

QTpfuImageSaveData *init_QTpfuImageSaveData( QTpfuImageSaveData *ISD, const char *theURL,
								    int xsize, int ysize, int alpha )
{ QTpfuImageSaveData *isd = ISD;
	if( !isd ){
		isd = (QTpfuImageSaveData*) calloc( 1, sizeof(QTpfuImageSaveData) );
		if( !isd ){
			return NULL;
		}
		isd->freeISD = 1;
	}
	else{
		memset( isd, 0, sizeof(*isd) );
	}
	/* malloc buffers */
	isd->rs = (unsigned short *)malloc((unsigned int)(xsize*sizeof(short)) );
	isd->gs = (unsigned short *)malloc((unsigned int)(xsize*sizeof(short)) );
	isd->bs = (unsigned short *)malloc((unsigned int)(xsize*sizeof(short)) );
	if( alpha ){
		isd->as = (unsigned short *)malloc((unsigned int)(xsize*sizeof(short)) );
	}
	isd->theURL = strdup(theURL);
	isd->fp = fopen(theURL, "w");
	if( isd->rs && isd->gs && isd->bs && (!alpha || isd->as) && isd->fp ){
		isd->xsize = xsize;
		isd->ysize = ysize;
		isd->alpha = alpha;
		fputs( "<?xml version=\"1.0\"?>\n", isd->fp );
		fputs( "<?quicktime type=\"video/x-qt-img2mov\"?>\n", isd->fp );
		fputs( "<import autoSave=True >\n", isd->fp );
	}
	else{
		xfree(isd->rs);
		xfree(isd->gs);
		xfree(isd->bs);
		xfree(isd->as);
		xfree(isd->theURL);
		if( isd->fp ){
			fclose(isd->fp);
		}
		if( isd != ISD ){
			xfree(isd);
		}
		isd = NULL;
	}
	return isd;
}

void free_QTpfuImageSaveData( QTpfuImageSaveData *isd )
{
	if( isd->fp ){
		fputs( "</import>\n", isd->fp );
		fclose(isd->fp);
		isd->fp = NULL;
	}
	/* free buffers */
	xfree(isd->rs);
	xfree(isd->gs);
	xfree(isd->bs);
	if( isd->alpha ){
		xfree(isd->as);
	}
	xfree(isd->theURL);
	if( isd->freeISD ){
		xfree(isd);
	}
}

int QTpfuSaveImage( QTpfuImageSaveData *isd, const char *name, unsigned int *scrbuf,
			    int xorg, int yorg, double duration )
{ int writeerr, y;
  unsigned int *ss;
  int xsize, ysize, alpha;

	if( !isd || !scrbuf ){
		return -1;
	}

 	xsize = isd->xsize, ysize = isd->ysize, alpha = isd->alpha;

	i_seterror(my_iopenerror);
	/* open the image file */
	isd->oimage = iopen( name, "w", RLE(1), 3, xsize, ysize, ((alpha)? 4 : 3) );

	if( !isd->oimage ){
		return -1;
	}

	writeerr = 0;

	/* write the data to the image file */
	if( alpha == 0 ){
		ss = scrbuf;
		for( y=0; y<ysize; y++ ){
			pfuCpackToRGB(ss, isd->rs, isd->gs, isd->bs, xsize);

			if(putrow(isd->oimage, isd->rs, (unsigned int)y, 0)!=xsize){
				writeerr = 1;
			}
			if(putrow(isd->oimage, isd->gs, (unsigned int)y, 1)!=xsize){
				writeerr = 1;
			}
			if(putrow(isd->oimage, isd->bs, (unsigned int)y, 2)!=xsize){
				writeerr = 1;
			}

			ss += xsize;
		}
	}
	else{
		ss = scrbuf;
		for( y=0; y<ysize; y++ ){
			pfuCpackToRGBA(ss, isd->rs, isd->gs, isd->bs, isd->as, xsize);

			if(putrow(isd->oimage, isd->rs, (unsigned int)y, 0)!=xsize){
				writeerr = 1;
			}
			if(putrow(isd->oimage, isd->gs, (unsigned int)y, 1)!=xsize){
				writeerr = 1;
			}
			if(putrow(isd->oimage, isd->bs, (unsigned int)y, 2)!=xsize){
				writeerr = 1;
			}
			if(putrow(isd->oimage, isd->as, (unsigned int)y, 3)!=xsize){
				writeerr = 1;
			}

			ss += xsize;
		}
	}

	/* close the image file */
	if( iclose(isd->oimage) < 0 || writeerr ){
	 	return -1;
	}
	else{
		if( isd->fp ){
			fprintf( isd->fp, "\t<image src=\"%s\" dur=%g />\n", name, duration );
		}
		return 0;
	}
}

int QTpfuImageAddDescription( QTpfuImageSaveData *isd, const char *descr, const char *lang )
{ int ret = -1;
	if( isd && isd->fp ){
		if( descr ){
			fprintf( isd->fp, "\t<description txt=\"%s\"", descr );
			if( lang ){
				fprintf( isd->fp, " lang=\"%s\"", lang );
			}
			fputs( " />\n", isd->fp );
			fflush(isd->fp);
			ret = 0;
		}
		else{
			ret = 1;
		}
	}
	return ret;
}

#endif // HAS_PERFORMER