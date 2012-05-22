/*!
 *  @file QTMovieSinkQTStuff.h
 *  QTMovieSink-106
 *
 *  Created by René J.V. Bertin on 20110308.
 *  Copyright 2011 INFSTTAR. All rights reserved.
 *
 */

#ifndef _QTMOVIESINKQTSTUFF_H

#pragma mark ----typedefs----

/*!
	an individual ICM Compression Sessions "encoding frame". The data being encoded is elsewhere...
 */
typedef struct ICMEncodingFrame {
	struct QTMovieSinkQTStuff *qtPriv;
	short inUse, OK;
} ICMEncodingFrame;

typedef struct CSBackupHook {
#if TARGET_OS_MAC
	CFURLRef				urlRef;			//!< another reference to the open file
	Boolean				bakExcl,			//!< CoreBackup exclude state before opening
						bakExclPath;		//!< CoreBackup excludeByPath state before opening
#else
	size_t				dum;				//!< relevant only on Mac OS X
#endif
} CSBackupHook;

/*!
	The internal, private descriptor structure for a QuickTime Movie sink.
 */
typedef struct QTMovieSinkQTStuff {
	Handle				dataRef;			//!< the data reference to the Movie output file
	OSType				dataRefType;		//!< the data ref. type, typically 'alis'
	Movie				theMovie;			//!< the destination Movie object
	Track				theTrack;			//!< the destination track in theMovie
	Media				theMedia;			//!< the destination Media in theTrack
	long					Frames;			//!< number of encoded frames
	DataHandler			dataHandler;		//!< the destination movie's DataHandler
										//!< (takes care of the actual writing of the encoded bytes)
										//!<
	CSBackupHook			cbState;			//!< hook to exclude the open file from the CoreBackup service
	const char			*saved_theURL;		//!< a copy of the destination filename
	Rect					trackFrame;		//!< the dimensions of theTrack
	CodecType				codecType;		//!< the codec being used
	CodecQ				quality;			//!< the encoding quality
	OSType				pixelFormat;		//!< the input pixel format
	unsigned long			numPixels;		//!< Width * Height
	// these are for use with CompressImage:
	struct {
		CGrafPtr				savedPort;
		GDHandle				savedGD;
		GWorldPtr				theWorld;
		PixMapHandle			thePixMap;
		Handle				compressedData;
		Ptr					compressedDataPtr;
		ImageDescriptionHandle	imgDesc;
	} CI;								//!< parameters for use with CompressImage
	// these are for use with ICMCompressionSession:
	// stats:
	struct {
#define BUFFERCOPIED_STATS				//!< should bufferCopied be updated?
		long							Dropped, Merged, timeChanged, bufferCopied;	//!<
										//!< Dropped, Merged, timeChanged and bufferCopied are
										//!< counters for establishing some encoding statistics.
										//!<
//		long							cycles;
		CVPixelBufferPoolRef			pixel_buffer_pool;
		CVPixelBufferRef				*pixel_buffer_ref;		//!< the encoding process uses <frameBuffers> pixel buffers
		ICMEncodingFrame				*pixel_buffer_frames;
		ICMCompressionSessionRef			compression_session_ref;	//!< identifies our compression session
		ICMSourceTrackingCallbackRecord	source_tracking_callback_record;	//!< defines the callback that updates the statistics counters
	} ICM;								//!< parameters for use with ICMCompressionSession
} QTMovieSinkQTStuff;

#define _QTMOVIESINKQTSTUFF_H
#endif // !_QTMOVIESINKQTSTUFF_H
