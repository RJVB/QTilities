/*!
 *  @file QTMovieSink.h
 *
 *  (C) RenE J.V. Bertin on 20100727.
 *
 */

#ifndef _QTMOVIESINK_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32) || defined(__WIN32__)
#	ifdef _QTMOVIESINK_C
#		define QTMSext _declspec(dllexport)
#	else
#		define QTMSext _declspec(dllimport)
#	endif
#else
#	define QTMSext /**/
#endif

#ifndef _QTILITIES_H
	typedef int ErrCode;
#endif

#ifdef _MSC_VER
	// We want to ensure that the following structs are defined without padding, for ease of (ex)portability
	// to Modula-2.
	// NB: on GCC, this would be done by adding __attribute__ ((packed)) after the type's closing brace
#	pragma pack(show)
#	pragma pack(push,1)
#endif

typedef union QTAPixel {
	struct {
		unsigned char
#if TARGET_OS_MAC
			alpha, red, green, blue;
#else
			red, green, blue, alpha;
#endif
	} ciChannel;
	struct {
		unsigned char
#if TARGET_OS_MAC
			alpha, red, green, blue;
#else
			blue, green, red, alpha;
#endif
	} icmChannel;
	unsigned long value;
} QTAPixel;

/*!
	The public descriptor structure for a QuickTime Movie sink. This is an object
	that allows to 'dump' a stream of raster images with identical dimensions to a QuickTime movie, compressing
	on the fly with one of several codecs.
	Two methods are supported: the legacy approach using CompressImage, and the more modern
	approach using ICM Compression Sessions.
 */
typedef struct QTMovieSinks {
	struct QTMovieSinkQTStuff *privQT;		//!< a pointer to a structure holding QuickTime entities,
									//!<encapsulated to be opaque to the user
									//!<
	const char *theURL;					//!< the (full) pathname where to save the video
	unsigned short useICM;				//!< whether or not the more modern ICM Compression Sessions are used
	unsigned short Width, Height;			//!< width and height of the images
	unsigned short hasAlpha;				//!< whether the images have an alpha channel
	short dealloc_qms, dealloc_imageFrame;
	short AddFrame_RT;					//!< Win32 only: whether to switch to "realtime" priority in the
									//!< QTMovieSink_AddFrame functions
									//!<
	QTAPixel **imageFrame;				//!< pointer to the image data, allocated at initialisation.
									//!< This is where the user code has to copy or generate the image.
									//!< imageFrame contains frameBuffers matrices of Width * Height * 4
									//!< bytes.
									//!<
	unsigned short currentFrame,			//!< current ICMEncodingFrame
		frameBuffers;					//!< number of ICMEncodingFrames configured
	ErrCode lastErr;					//!< last error that occurred.
} QTMovieSinks;

/*!
	public structure containing the encoding statistics of a compression session
 */
typedef struct QTMSEncodingStats{
	double Duration,					//!< current duration
		frameRate;					//!< current instantaneous frame rate
	long Total,						//!< total number of encoded frames until now
		Dropped,						//!< ICM: number of dropped frames
		Merged,						//!< ICM: number of frames that were merged (with preceding frame(s))
		timeChanged,					//!< ICM: number of frames for which the frametime was modified
		bufferCopied;					//!< ICM: number of frames that had to be copied to allow encoding
} QTMSEncodingStats;

#ifdef _MSC_VER
#	pragma pack(pop)
#	pragma pack(show)
#endif
	
#pragma mark ---QTMovieSink using CompressImage or ICMCompressionSession----
#define __QTMOVIESINK_CI__

/*!
	initialise and return a sink of the specified dimensions, with the given name, using the specified codec
	and quality. If <openQT> is True, QuickTime will be initialised. Optionally, a new QTMovieSinks object
	will be allocated, otherwise the specified object will be allocated. The encoder uses the newer
	ICM Compression Session algorithms, or the older, QuickTime/QuickDraw CompressImage
	approach. This function allocates the necessary frame buffer(s) into which the input image must be copied
	or generated.
	When using ICM, one can request up to a maximum of 255 source frame buffers, which will be cycled
	after an image (frame) has been added to the sink. This might have a performance benefit when compressing in
	a separate thread.
	@n
	Modula-2 name: OpenQTMovieSink
	@param	qms		pointer to the QTMovieSinks object to initialise, or NULL to allocate a new one
	@param	theURL	destination movie file name, or NULL to create an in-memory movie object
	@param	Width	image pixel width
	@param	Height	image pixel height
	@param	hasAlpha	image has an alpha channel. Best set to True even if there is none...
	@param	frameBuffers	the number of source frame buffers to use in ICM mode; limited to 1 in CompressImage mode!
	@param	codec	the compression codec to use. This can be any QuickTime codec that accepts RGB or RGBA images,
					some of which are proxied by the QTCompressionCodec() function
	@param	quality	the quality to use; the values are proxied through the QTCompressionQuality() function.
	@param	useICM	whether to use ICM Compression Sessions
	@param	openQT	whether to initialise QuickTime first.
	@param	errReturn	contains an error code hinting at the reason of failure (open_QTMovieSink returns NULL in that case).
 */
QTMSext QTMovieSinks *open_QTMovieSink( QTMovieSinks *qms, const char *theURL,
							    unsigned short Width, unsigned short Height, int hasAlpha,
							    unsigned short frameBuffers,
							    unsigned long codec, unsigned long quality, int useICM,
							    int openQT, ErrCode *errReturn );
/*!
	initialise and return a sink as described for open_QTMovieSink(), but using preallocated image frame memory.
	@n
	Modula-2 name: OpenQTMovieSinkWithData
 */
QTMSext QTMovieSinks *open_QTMovieSinkWithData( QTMovieSinks *qms, const char *theURL,
						  QTAPixel **imageFrame,
						  unsigned short Width, unsigned short Height, int hasAlpha,
						  unsigned short frameBuffers,
						  unsigned long codec, unsigned long quality, int useICM,
						  int openQT, ErrCode *errReturn );

/*!
	close a sink. This routine takes care of finalising the resulting QuickTime movie, and
	deallocates everything that has to be deallocated, including *qms if allocated by the library.
	Adds a TimeCode track if addTCTrack=TRUE
	QuickTime is closed if <closeQT> is True.
	@n
	Modula-2 name: CloseQTMovieSink
	@param	qms	pointer to a QTMovieSinks object; de-allocated and set to NULL if it was auto-allocated.
	@param	addTCTrack	whether to create and add a TimeCode track to the movie. NB: this track only makes (perfect)
						sens for movies with a (perfectly) fixed frame rate; no attempt is made to construct
						a complex TimeCode track that takes fluctuating frame presentation times into account!
	@param	stats	optional; passed to get_QTMovieSink_EncodingStats just before the movie is closed.
	@param	closeMovie	whether to close the movie or leave it open for future use.
	@param	closeQT	whether to close down QuickTime
 */
QTMSext ErrCode close_QTMovieSink( QTMovieSinks **qms, int addTCTrack, QTMSEncodingStats *stats,
						    int closeMovie, int closeQT );

/*!
	add a frame to the current sink, to be displayed with the specified duration. The frame added
	must be of the dimensions specified when creating the QTMovieSink, and
	is expected in qms->imageFrame[qms->currentFrame] .
	@n
	Modula-2 name: AddFrameToQTMovieSink
	@n
	Generating a movie takes the following form at its simplest:
	@code
QTMovieSinks *qms;
if( (qms = open_QTMovieSink( NULL, fileName, width, height, hasAlpha, ICMFrames, codec, quality, useICM, TRUE, &err )) && !err ){
	while( someCondition ){
		// generate an image, storing it in the appropriate buffer:
		generateAnImage( qms->imageFrame[qms->currentFrame], width, height );
		// or obtain an OpenGL screendump:
		glReadBuffer(GL_FRONT);
		glReadPixels( 0, 0, screen->w, screen->h,
#ifdef i386
			GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, qms->imageFrame[qms->currentFrame]
#else
			GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, qms->imageFrame[qms->currentFrame]
#endif
		);	
		QTMovieSink_AddFrame( qms, duration );
	}
	err = close_QTMovieSink( &qms, addTCTrack, NULL, TRUE );
}
	@endcode
 */
QTMSext ErrCode QTMovieSink_AddFrame( QTMovieSinks *qms, double frameDuration );

#pragma mark ---QTMovieSink using ICMCompressionSession----
#define __QTMOVIESINK_ICM__

/*!
	returns the QTMovieSink's Movie object
	@n
	Modula-2 name: GetQTMovieSinkMovie
 */
QTMSext Movie get_QTMovieSink_Movie( QTMovieSinks *qms );
	
/*!
	obtain some statistics about the ongoing ICM encoding process. <stats> is not touched if <qms>
	does not refer to a valid QTMovieSink.
	@n
	Modula-2 name: GetQTMovieSinkEncodingStats
 */
QTMSext int get_QTMovieSink_EncodingStats( QTMovieSinks *qms, QTMSEncodingStats *stats );

/*!
	Alternative to QTMovieSink_AddFrame() to add a frame to the current sink. It does the same things 
	except that it takes a (movie; relative) time at which the frame is to be displayed. This is more
	appropriate for an image generation process that runs at variable speed, but is possible only
	with ICM Compression Session!
	@n
	Modula-2 name: AddFrameToQTMovieSinkWithTime
 */
QTMSext ErrCode QTMovieSink_AddFrameWithTime( QTMovieSinks *qms, double frameTime );

QTMSext QTAPixel *QTMSFrameBuffer( QTMovieSinks *qms, unsigned short frameBuffer );
QTMSext QTAPixel *QTMSCurrentFrameBuffer( QTMovieSinks *qms );
QTMSext QTAPixel *QTMSPixelAddress( QTMovieSinks *qms, unsigned short frameBuffer, unsigned int pixnr );
QTMSext QTAPixel *QTMSPixelAddressInCurrentFrameBuffer( QTMovieSinks *qms, unsigned int pixnr );

#pragma mark ----VideoCodec constants----
#define __QTMOVIESINK_METADATA__

/*!
	The video codecs supported by default. These are just convenience variables allowing to avoid
	dependencies on the QuickTime headers (it should be possible to use QTMovieSink without having the
	QuickTime SDK installed). It is perfectly possible to use the FOURCHARCODE of another codec, as
	long as it works with RGB or RGBA data.
 */
QTMSext extern const unsigned long
	cVideoCodec,
	cJPEGCodec,
	cMJPEGACodec,
	cAnimationCodec,
	cHD720pCodec,
	cHD1080i60Codec,
	cMPEG4Codec,
	cH264Codec,
	cRawCodec,
	cCinepakCodec,
	cTIFFCodec;
#if defined(__APPLE_CC__) || defined(__MACH__)
	QTMSext extern const unsigned long cApplePixletCodec;
#endif

/*!
	another way of creating a shareable copy of the various supported codec identifiers,
	to export those codes to client software that does not have access to or cannot include
	the QuickTime headers.
 */
typedef struct QTCompressionCodecs {
	unsigned long
		Video		//!< the Video codec
		, JPEG		//!< the Motion JPEG codec
		, JPEGA		//!< the Motion JPEGA codec
		, Animation	//!< an animation codec, lossless and appropriate for computer-generated images
		, HD720p		//!< the DVCPro HD 720 p codec
		, HD1080i60	//!< the DVCPro HD 1080i 60Hz codec
		, MPEG4		//!< an MPEG4 encoder
		, H264		//!< an MPEG4 - H.264 encoder
		, Raw		//!< raw image stream output
		, Cinepak		//!< the old CinePak codec
		, TIFF		//!< "Motion TIFF" output
#if defined(__APPLE_CC__) || defined(__MACH__)
		, ApplePixlet	//!< Macintosh only: the Pixlet fractal codec.
#endif
		;
} QTCompressionCodecs;

/*!
	returns a pointer to an intialised QTCompressionCodecs structure
 */
QTMSext QTCompressionCodecs *QTCompressionCodec();

/*!
	Convenience variables encapsulating QuickTime's quality levels:
 */
QTMSext extern const unsigned long cLossless,
	cMaxQuality,
	cMinQuality,
	cLowQuality,
	cNormalQuality,
	cHighQuality;

/*!
	Convenience structure encapsulating QuickTime's quality levels
 */
typedef struct QTCompressionQualities {
	unsigned long
		Lossless,		//!< lossless quality; the animation codec uses RLE in this case
		Max,			//!< highest quality, slowest
		Min,			//!< lowest quality, fastest
		Low,			//!< low quality, faster
		Normal,		//!< normal quality; optimal quality/speed trade-off
		High;		//!< high quality: acceptable quality/speed trade-off
} QTCompressionQualities;

/*!
	returns a pointer to an initialised QTCompressionQualities structure
 */
QTMSext QTCompressionQualities *QTCompressionQuality();

#pragma mark ----Movie/Track MetaData----

#ifndef _QTILITIES_H
/*!
	supported QuickTime metadata classes:
 */
typedef enum AnnotationKeys {
	akAuthor='auth',
	akComment='cmmt',
	akCopyRight='cprt',
	akDisplayName='name',
	akInfo='info',
	akKeywords='keyw',
	akDescr='desc',
	akFormat='orif',
	akSource='oris',
	akSoftware='soft',
	akWriter='wrtr',
	akYear='year',
	akCreationDate='©day',
	akTrack = '@trk'
} AnnotationKeys;
#endif // _QTILITIES_H

/*!
	add the specied key/value pair to the metadata of the current track of the sink's movie
 */
QTMSext ErrCode QTMovieSink_AddTrackMetaDataString( QTMovieSinks *qms,
										AnnotationKeys key, const char *value, const char *lang );
/*!
	add the specied key/value pair to the sink's movie metadata
 */
QTMSext ErrCode QTMovieSink_AddMovieMetaDataString( QTMovieSinks *qms,
						AnnotationKeys key, const char *value, const char *lang );

QTMSext ErrCode open_QTMovieSink_Mod2( QTMovieSinks *qms, const char *theURL, int ulen,
						unsigned short Width, unsigned short Height, int hasAlpha,
						unsigned short frameBuffers,
						unsigned long codec, unsigned long quality, int useICM,
						int openQT );
QTMSext ErrCode open_QTMovieSinkWithData_Mod2( QTMovieSinks *qms, const char *theURL, int ulen,
						QTAPixel **imageFrame,
						unsigned short Width, unsigned short Height, int hasAlpha,
						unsigned short frameBuffers,
						unsigned long codec, unsigned long quality, int useICM,
						int openQT );
QTMSext ErrCode close_QTMovieSink_Mod2( QTMovieSinks *qms, int addTCTrack, QTMSEncodingStats *stats,
							    int closeMovie, int closeQT );
QTMSext ErrCode QTMovieSink_AddTrackMetaDataString_Mod2( QTMovieSinks *qms,
						AnnotationKeys key, const char *value, int vlen, const char *lang, int llen );
QTMSext ErrCode QTMovieSink_AddMovieMetaDataString_Mod2( QTMovieSinks *qms,
						AnnotationKeys key, const char *value, int vlen, const char *lang, int llen );

#ifdef __cplusplus
}
#endif

#define _QTMOVIESINK_H
#endif
