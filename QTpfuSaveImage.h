#ifdef HAS_PERFORMER

#ifndef _QTPFUSAVEIMAGE_H

#include <stdio.h>
#include <Performer/image.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__WIN32__)
#	ifdef _QTPFUSAVEIMAGE_C
#		define PSI2ext _declspec(dllexport)
#	else
#		define PSI2ext _declspec(dllimport)
#	endif
#else
#	define PSI2ext extern
#endif

/*!
	data structure used by QTpfuSaveImage to cache reusable items.
	QTpfuSaveImage is a version of the SGI Performer pfuSaveImage function
	optimised for saving series of screendumps e.g. during a simulation. It reuses
	buffers allocated in preflight instead of at each invocation. It also
	generates a QI2M file that can be read by QuickTime if the QTImage2Mov importer
	is installed.
	Initialised by init_QTpfuImageSaveData, released by free_QTpfuImageSaveData.
 */
typedef struct QTpfuImageSaveData {
    unsigned short *rs, *gs, *bs, *as;	//!< line buffers, one per colour channel
    int xsize, ysize, alpha;	//!< image dimensions, and whether or not there's an alpha channel
    IMAGE *oimage;	//!< internal image buffer 
    char *theURL;	//!< base file name
    FILE *fp;	//!< file pointer to the QI2M file
    char freeISD;	//!< whether free_QTpfuImageSaveData needs to release <self>
				//!< (if a NULL <isd> was passed to init_QTpfuImageSaveData)
				//!<
} QTpfuImageSaveData;

PSI2ext QTpfuImageSaveData *init_QTpfuImageSaveData( QTpfuImageSaveData *isd, const char *theURL,
						int xsize, int ysize, int alpha );
PSI2ext void free_QTpfuImageSaveData( QTpfuImageSaveData *isd );
	PSI2ext int QTpfuSaveImage( QTpfuImageSaveData *isd, const char *name, unsigned int *scrbuf, 
						int xorg, int yorg, double duration );
/*!
	add an option description tag to the generated QI2M file, optionally containing
	a language specification.
 */
PSI2ext int QTpfuImageAddDescription( QTpfuImageSaveData *isd, const char *descr, const char *lang );

#define _QTPFUSAVEIMAGE_H

#ifdef __cplusplus
}
#endif

#endif // !_QTPFUSAVEIMAGE_H
#endif // HAS_PERFORMER