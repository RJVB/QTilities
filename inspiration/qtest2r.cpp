#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#if ! defined(_WINDOWS) && !defined(WIN32) && !defined(_MSC_VER)
#	include <unistd.h>
#	include <libgen.h>
#	include <sys/param.h>
#else
#	include "winixdefs.h"
#	include "winutils.h"
#endif


#ifdef __MACH__
#	include <mach/mach.h>
#	include <mach/mach_time.h>
#	include <mach/mach_init.h>
#	include <mach/thread_policy.h>
#	include <sys/sysctl.h>
#	include <pthread.h>
#	include <sched.h>
#	include <time.h>
#endif

#if __APPLE_CC__
#	include <Carbon/Carbon.h>
#	include <QuickTime/QuickTime.h>
#	define BYTE unsigned char
#	define MAX_PATH	MAXPATHLEN
#else
#	include <qtml.h>
#	include <movies.h>
#	include <tchar.h>
#	include <Gestalt.h>
#	include <wtypes.h>
#	include <FixMath.h>
#	include <Script.h>
#	include <stdlib.h>
#	include <TextUtils.h>
#	include <NumberFormatting.h>
#	include <MacErrors.h>
#	include <CVPixelBuffer.h>
#endif

#include <String>

#include "timing.h"
#include "QTMovieSink.h"
#ifdef HAS_PERFORMER
#	include "QTpfuSaveImage.h"
#endif

#include "QTilities.h"

#include <iostream>
#include <strstream>
#include <vector>

#define SET_REALTIME_MODE

#ifdef __MACH__

static mach_timebase_info_data_t sTimebaseInfo;
static double calibrator= 0;

void rtsched_thread_policy_set( thread_act_t thread=0, thread_policy_flavor_t policy=THREAD_STANDARD_POLICY,
						 unsigned long period=0, unsigned long computation=0, unsigned long constraint=0, long importance=1 )
{
	union{
		struct thread_standard_policy standard;
		struct thread_time_constraint_policy ttc;
		struct thread_precedence_policy precedence;
	} poldat;
	mach_msg_type_number_t count;

	if( !thread ){
		thread= mach_thread_self();
	}
	if( !calibrator ){
//	  int ret, bus_speed, mib[2] = { CTL_HW, HW_BUS_FREQ };
//	  size_t len;
		mach_timebase_info(&sTimebaseInfo);
		  /* go from microseconds to absolute time units (the timebase is calibrated in nanoseconds): */
		calibrator= 1e3 * sTimebaseInfo.denom / sTimebaseInfo.numer;
//		len = sizeof( bus_speed);
//		ret = sysctl (mib, 2, &bus_speed, &len, NULL, 0);
//		if (ret < 0) {
//			fprintf( stderr, "sysctl query bus speed failed (%s)\n", strerror(errno) );
//		}
//		else{
//			fprintf( stderr, "System cpu/bus_speed==%d\n", bus_speed );
//			calibrator = bus_speed / 1000.0;
//		}
	}
	switch( policy ){
		default:
		case THREAD_STANDARD_POLICY:
			count= THREAD_STANDARD_POLICY_COUNT;
			break;
		case THREAD_TIME_CONSTRAINT_POLICY:
			poldat.ttc.period= period * calibrator;
			poldat.ttc.computation= computation * calibrator;
			poldat.ttc.constraint= constraint * calibrator;
			poldat.ttc.preemptible= 1;
			count= THREAD_TIME_CONSTRAINT_POLICY_COUNT;
			break;
		case THREAD_PRECEDENCE_POLICY:
			poldat.precedence.importance= importance;
			count= THREAD_PRECEDENCE_POLICY_COUNT;
			break;
	}
	if( thread_policy_set( thread, policy, (thread_policy_t)&poldat, count ) != KERN_SUCCESS ){
	  char errmsg[512];
		switch( policy ){
			case THREAD_TIME_CONSTRAINT_POLICY:
				snprintf( errmsg, sizeof(errmsg), "thread_policy_set(period=%lu,computation=%lu,constraint=%lu) failed",
					poldat.ttc.period, poldat.ttc.computation, poldat.ttc.constraint
				);
				break;
			case THREAD_PRECEDENCE_POLICY:
				snprintf( errmsg, sizeof(errmsg), "thread_policy_set(importance=%ld) failed",
					poldat.precedence.importance
				);
				break;
		}
		fprintf( stderr, "%s\n", errmsg );
	}
	return;
}

#	ifdef SET_REALTIME_MODE
#		define SET_REALTIME()	rtsched_thread_policy_set( 0, THREAD_TIME_CONSTRAINT_POLICY, 3000, 2500, 3000, 1 )
#	else
#		define SET_REALTIME()	/**/
#	endif
#	define EXIT_REALTIME()	/**/

#else

#	ifdef SET_REALTIME_MODE
#		define SET_REALTIME()	set_realtime()
#		define EXIT_REALTIME()	exit_realtime()
#	else
#		define SET_REALTIME()	/**/
#		define EXIT_REALTIME()	/**/
#	endif

#endif

#if __APPLE_CC__
char *_fullpath( char path[MAX_PATH], char *name, int namelen )
{ bool hasFullPath;

	if( name[0] == '.' ){
		if( name[1] == '.' ){
			name++;
		}
		if(
#if TARGET_OS_WIN32
		   name[1] == '\\'
#else
		   name[1] == '/'
#endif
		){
			name += 2;
		}
	}
#if TARGET_OS_WIN32
	hasFullPath = (name[1] == ':');
#else
	hasFullPath = (name[0] == '/');
#endif
	if( !hasFullPath ){
	  char *c, cwd[1024];
#ifdef _MSC_VER
		c = _getcwd( cwd, sizeof(cwd) );
#else
		c = getwd( cwd );
#endif
		if( c && (strlen(cwd) + strlen(name) + 2) <= MAX_PATH ){
			strcpy( path, cwd );
#if TARGET_OS_WIN32
			strcat( path, "\\" );
#else
			strcat( path, "/" );
#endif
			strcat( path, name );
		}
		else{
			path = NULL;
		}
	}
	return path;
}
#endif

//NTSC and digital 525 video have exactly 30/1.001 frames per second or 60/1.001 fields per second
#define     TIME_SCALE      60000
#define     SAMPLE_DURATION 1001

#ifndef     VIDEO_CODEC_QUALITY
#define     VIDEO_CODEC_QUALITY codecNormalQuality
#endif

typedef struct tag2VUYQUAD {
        BYTE    rgbCb;
        BYTE    rgbY0;
        BYTE    rgbCr;
        BYTE    rgbY1;
} _2VUYQUAD;

    Rect m_trackFrame;
    int _x,_y; unsigned _w,_h; 
//    std::string _filename;
    std::vector<_2VUYQUAD> _imageD;

static OSStatus EncodedFrameOutputCallback (void *encodedFrameOutputRefCon, ICMCompressionSessionRef session, OSStatus error, ICMEncodedFrameRef frame, void *reserved)
{
    OSStatus rc = noErr; 
#if 1
    Media media = static_cast<Media>(encodedFrameOutputRefCon);
    if (noErr != (rc=AddMediaSampleFromEncodedFrame(media, frame, NULL)))
        std::cerr << "EncodedFrameOutputCallback - AddMediaSampleFromEncodedFrame failed: " << rc << std::endl;
#endif

#ifdef DEBUG1
    std::cerr << "EncodedFrameOutputCallback - DataPtr:" << (void*)ICMEncodedFrameGetDataPtr(frame) << " encodedFrameOutputRefCon:" << encodedFrameOutputRefCon << std::endl;

    ImageDescriptionHandle imageDesc;
    if (noErr != (rc=ICMEncodedFrameGetImageDescription(frame, &imageDesc))) {
        std::cerr << "EncodedFrameOutputCallback - ICMEncodedFrameGetImageDescription failed: " << rc << std::endl;
        return rc;
    }

    MediaSampleFlags flags =  ICMEncodedFrameGetMediaSampleFlags(frame);
    if (flags & mediaSampleNotSync)                     std::cerr << "sample is not a sync sample (eg. is frame differenced" << std::endl;
    if (flags & mediaSampleShadowSync)                  std::cerr << "sample is a shadow sync" << std::endl;
    if (flags & mediaSampleDroppable)                   std::cerr << "sample is not required to be decoded for later samples to be decoded properly" << std::endl;
    if (flags & mediaSamplePartialSync)                 std::cerr << "sample is a partial sync (e.g., I frame after open GOP)" << std::endl;
    if (flags & mediaSampleHasRedundantCoding)          std::cerr << "sample is known to contain redundant coding" << std::endl;
    if (flags & mediaSampleHasNoRedundantCoding)        std::cerr << "sample is known not to contain redundant coding" << std::endl;
    if (flags & mediaSampleIsDependedOnByOthers)        std::cerr << "one or more other samples depend upon the decode of this sample" << std::endl;
    if (flags & mediaSampleIsNotDependedOnByOthers)     std::cerr << "synonym for mediaSampleDroppable" << std::endl;
    if (flags & mediaSampleDependsOnOthers)             std::cerr << "sample's decode depends upon decode of other samples" << std::endl;
    if (flags & mediaSampleDoesNotDependOnOthers)       std::cerr << "sample's decode does not depend upon decode of other samples" << std::endl;
    if (flags & mediaSampleEarlierDisplayTimesAllowed)  std::cerr << "samples later in decode order may have earlier display times" << std::endl;
#endif

    return rc;
}

static void SourceTrackingCallback (void *sourceTrackingRefCon, ICMSourceTrackingFlags sourceTrackingFlags, void *sourceFrameRefCon, void *reserved)
{
#ifdef DEBUG1
    std::cerr << "SourceTrackingCallback - sourceTrackingRefCon: " << sourceTrackingRefCon << " sourceFrameRefCon: " << sourceFrameRefCon << std::endl;

    if (sourceTrackingFlags & kICMSourceTracking_LastCall)            std::cerr << "this is the last call for this sourceFrameRefCon." << std::endl;
    if (sourceTrackingFlags & kICMSourceTracking_ReleasedPixelBuffer) std::cerr << "the session is done with the source pixel buffer" << std::endl;
    if (sourceTrackingFlags & kICMSourceTracking_FrameWasEncoded)     std::cerr << "this frame was encoded." << std::endl;
    if (sourceTrackingFlags & kICMSourceTracking_CopiedPixelBuffer)   std::cerr << "the source pixel buffer was not compatible with the compressor's required pixel buffer attributes" << std::endl;
#endif
    if (sourceTrackingFlags & kICMSourceTracking_FrameWasDropped)     std::cerr << "this frame was dropped." << std::endl;
    if (sourceTrackingFlags & kICMSourceTracking_FrameWasMerged)      std::cerr << "this frame was merged into other frames." << std::endl;
    if (sourceTrackingFlags & kICMSourceTracking_FrameTimeWasChanged) std::cerr << "the time stamp of this frame was modified." << std::endl;
}

static void ReleaseBytesCallback( void *releaseRefCon, const void *baseAddress )
{
    return;

    std::cerr << "ReleaseBytesCallback -  baseAddress:" << baseAddress << " releaseRefCon:" << releaseRefCon << std::endl;
}


void
CalcCompressionPerformance(OSType pfmt, CompressorComponent c, CodecType type, Handle name_h, Boolean skip)
{
//    char *name_c = &(*name_h)[1];
    std::string name_c = std::string( &(*name_h)[1], 
    (*name_h)[0] ); 
    std::ostrstream s;
    if( pfmt > 40 ){
#if TARGET_OS_WIN32
	    s << name_c << "-" << *((char*)&pfmt+3) << *((char*)&pfmt+2) << *((char*)&pfmt+1) << *((char*)&pfmt+0) << std::ends;
#else
	    s << name_c << "-" << *((char*)&pfmt+0) << *((char*)&pfmt+1) << *((char*)&pfmt+2) << *((char*)&pfmt+3) << std::ends;
#endif
    }
    else{
	    s << name_c << "-RGB" << std::ends;
    }
    if( skip && name_c != "Xiph Theora Encoder" ){
	    fprintf( stderr, "Skipping '%s'\n", s.str() );
	    return;
    }

    OSStatus rc;
    CVReturn cv; 
    OSType data_ref_type;
    Ptr ptr = (Ptr)&_imageD[0];

    CFStringRef out_path = NULL;
    CVPixelBufferRef pixel_buffer_ref=NULL;
    ICMCompressionSessionOptionsRef options_ref=NULL;
    ICMCompressionSessionRef compression_session_ref=NULL;
    Handle data_ref = NULL;
    DataHandler data_handler = NULL;
    Movie movie = NULL;
    Track track = NULL;
    Media media = NULL;
    double elapsed;

 // convert the file name into a data reference
    char tmp[255];
#if TARGET_OS_WIN32
    strcat(strcpy(strcpy(tmp,".\\")+2,s.str()),".mov");
#else
    strcat(strcpy(strcpy(tmp,"./")+2,s.str()),".mov");
#endif
    char file[MAX_PATH]; if (NULL == _fullpath (file,tmp,sizeof(tmp))) {
        std::cerr << "QTMovieFile::Initialize(), _fullpath() failed - " << strerror(errno) << std::endl;
        exit (-1);
    }
    if (nil == (out_path=CFStringCreateWithCString (kCFAllocatorDefault, file, kCFStringEncodingISOLatin1))) {
        std::cerr << "CFStringCreateWithCString(\"" << file <<"\") failed" << std::endl;
        goto done;
    }

    if (noErr != (rc=QTNewDataReferenceFromFullPathCFString(out_path, kQTNativeDefaultPathStyle, 0, &data_ref, &data_ref_type))) {
        std::cerr << "QTNewDataReferenceFromFullPathCFString(\"" << file <<"\") failed" << std::endl;
        goto done;
    }

#ifdef DEBUG
    std::cerr << file << ' ';
#else
    std::cerr << s.str() << ' ';
#endif

    CFRelease(out_path),out_path=NULL;

    if (noErr != (rc=EnterMovies())) {
        std::cerr << "EnterMovies() failed: " << rc << std::endl;
        goto done;
    }

    if (kCVReturnSuccess != (cv=CVPixelBufferCreateWithBytes(kCFAllocatorDefault,
                                _w, _h, pfmt,
                                ptr,_w*4/*8*/, ReleaseBytesCallback, (void*)0xDEADBABE,
                                NULL, // CFDictionaryRef pixelBufferAttributes,
                                &pixel_buffer_ref)))
    {
        std::cerr << " QTMovieFile::Initialize(), CVPixelBufferCreateWithBytes() failed - " << cv << std::endl;
        goto done;
    }

    if (noErr != (rc=ICMCompressionSessionOptionsCreate( kCFAllocatorDefault,&options_ref))) {
        std::cerr << " QTMovieFile::Initialize(), ICMCompressionSessionOptionsCreate() failed - " << rc << std::endl;
        goto done;
    }
 
    if (noErr != (rc=ICMCompressionSessionOptionsSetDurationsNeeded (options_ref, true))) {
        std::cerr << "ICMCompressionSessionOptionsSetDurationsNeeded() failed - " << rc << std::endl;
        goto done;
    }
    ICMCompressionSessionOptionsSetAllowFrameTimeChanges( options_ref, FALSE );
    ICMCompressionSessionOptionsSetAllowTemporalCompression( options_ref, TRUE );

    // create a new movie on disk
    if (noErr != (rc=CreateMovieStorage (data_ref, data_ref_type, 'TVOD', smCurrentScript, createMovieFileDeleteCurFile | createMovieFileDontCreateResFile, &data_handler, &movie))) {
    }

    // create a track with video
    track = NewMovieTrack (movie, FixRatio(_w, 1), FixRatio(_h, 1), kNoVolume);
    if (noErr != (rc=GetMoviesError())) {
    }

    media = NewTrackMedia (track, VideoMediaType, TIME_SCALE, nil, 0);
    if (noErr != (rc=GetMoviesError())) {
    }

    { ICMEncodedFrameOutputRecord encoded_frame_output_record;
	    encoded_frame_output_record.encodedFrameOutputCallback = EncodedFrameOutputCallback;
	    encoded_frame_output_record.encodedFrameOutputRefCon = media;
	    encoded_frame_output_record.frameDataAllocator = NULL;
	    
	    if (noErr != (rc=ICMCompressionSessionCreate( kCFAllocatorDefault, 
					    _w,_h, 
					    type, //kJPEGCodecType, 
					    TIME_SCALE, 
					    options_ref,
					    NULL /*source_pixel_buffer_attributes*/, 
					    &encoded_frame_output_record,
					    &compression_session_ref ))) 
	    { 
		   std::cerr << " QTMovieFile::Initialize(), ICMCompressionSessionCreate() failed - " << rc << std::endl;
		   goto done;
	    }
    }
    ICMCompressionSessionOptionsRelease(options_ref) , options_ref=NULL;
    
    if (noErr != (rc=BeginMediaEdits (media))) {
    }

    ICMSourceTrackingCallbackRecord source_tracking_callback_record;
    source_tracking_callback_record.sourceTrackingCallback = SourceTrackingCallback;
    source_tracking_callback_record.sourceTrackingRefCon = NULL;

    init_HRTime();
    SET_REALTIME();
    HRTime_tic();

    int frames; for (frames=0; (HRTime_toc() < 3); ++frames)
    {
        if (noErr != (rc=ICMCompressionSessionEncodeFrame(
                            compression_session_ref,
                            pixel_buffer_ref,
                            0, //displayTimeStamp
                            SAMPLE_DURATION,
                            kICMValidTime_DisplayDurationIsValid,
                            NULL,
                            &source_tracking_callback_record,
                            (void*)/*rand()*/0xF0ADB0AD)))
        {
            std::cerr << " QTMovieFile::Initialize(), ICMCompressionSessionEncodeFrame() failed: " << rc << std::endl;
            goto done;
        }
#ifdef DEBUG1
        std::cerr << " QTMovieFile::Initialize(), ICMCompressionSessionEncodeFrame() succeeded!" << std::endl;
#endif
    }

    if (noErr != (rc=ICMCompressionSessionCompleteFrames(compression_session_ref, TRUE, 0, 0))) {
        std::cerr << " QTMovieFile::Initialize(), ICMCompressionSessionCompleteFrames() failed: " << rc << std::endl;
        goto done;
    }

    elapsed = HRTime_toc();
    EXIT_REALTIME();
    std::cerr << " : " << frames << " frames in " << elapsed << " seconds - " << frames/elapsed << std::endl;

    if (noErr != (rc=EndMediaEdits (media))) {
        std::cerr << "EndMediaEdits() failed: " << rc << std::endl;
    }

    if (noErr != (rc=InsertMediaIntoTrack(track,0,0,GetMediaDuration(media),fixed1))) {
        std::cerr << "InsertMediaIntoTrack() failed: " << rc << std::endl;
    }

    if (noErr != (rc=UpdateMovieInStorage (movie, data_handler))) {
        std::cerr << "UpdateMovieInStorage() failed: " << rc << std::endl;
    }

done:
    if (data_handler) CloseMovieStorage (data_handler), data_handler=NULL;
    if (movie)        DisposeMovie (movie), movie=NULL;
    if (data_ref)     /*DeleteMovieStorage (data_ref, data_ref_type),*/ DisposeHandle (data_ref), data_ref=NULL;
    if (out_path)     CFRelease(out_path),out_path=NULL;

    if (compression_session_ref) ICMCompressionSessionRelease (compression_session_ref), compression_session_ref=NULL;
    if (options_ref)			 ICMCompressionSessionOptionsRelease (options_ref),      options_ref=NULL;
    if (pixel_buffer_ref)        CVPixelBufferRelease (pixel_buffer_ref),                pixel_buffer_ref=NULL;

    ExitMovies();
    sleep(1);
}

int movieStep(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
	  double t, t2;
	  short steps = (short) ( (unsigned long) params);
		if( QTMovieWindowGetTime(wi,&t, 0) == noErr ){
			if( !steps ){
				std::cerr << "movie STEPPED to t=" << t << "s" << std::endl;
			}
			else{
				t2 = t + ((double) steps/(*wi)->info->frameRate);
				std::cerr << "Current movieTime=" << t << "s; stepping to " << t2 << "s" << std::endl;
			}
		}
	}
	// if we return TRUE the action is discarded!
	return FALSE;
}

int movieScan(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
	  double t, *t2 = (double*) params;
		if( QTMovieWindowGetTime(wi,&t, 0) == noErr ){
			if( t2 ){
				std::cerr << "Scanning movie from " << t << "s to " << *t2 << "s" << std::endl;
			}
			else{
				std::cerr << "Scanning movie from " << t << "s to ??s" << std::endl;
			}
		}
	}
	// if we return TRUE the action is discarded!
	return FALSE;
}

int moviePlay(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
	  double t;
		if( QTMovieWindowGetTime(wi,&t, 0 ) == noErr ){
			if( (*wi)->wasScanned > 0 ){
				std::cerr << "movie scanned to t=" << t << "s" << std::endl;
			}
		}
	}
	return FALSE;
}

int movieFinished(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
		QTMovieWindowSetTime( wi, (*wi)->info->duration/2, 0 );
	}
	return FALSE;
}

#if 0
ErrCode XMLParserAddElement( ComponentInstance *xmlParser, const char *elementName,
					unsigned int elementID, long elementFlags, int *errors )
{ ErrCode err;
	err = XMLParserAddElement( xmlParser, elementName, NULL, elementID, elementFlags );
	if( err != noErr ){
		*errors += 1;
	}
	return err;
}

ErrCode XMLParserAddElementAttribute( ComponentInstance *xmlParser, unsigned int elementID,
					const char *attrName, unsigned int attrID, unsigned int attrType, int *errors )
{ ErrCode err;
	err = XMLParserAddElementAttribute( xmlParser, elementID, NULL, attrName, attrID, attrType );
	if( err != noErr ){
		*errors += 1;
	}
	return err;
}

void TestXMLParsing()
{ ComponentInstance xmlParser = NULL;
  XMLDoc xmldoc = NULL;
  enum xmlElements { element_root = 1, element_frequency,
	element_frequence, element_scale,
	element_utc, element_echelle, element_channels, element_canaux,
  };
  enum xmlAttributes { attr_freq = 1,
	attr_scale, attr_forward, attr_pilot,
	attr_left, attr_right, attr_zone,
	attr_dst,
  };
  ErrCode xmlErr;
  SInt32 idx, element_id;
  int errors = 0;
  XMLContentPtr root_content;
  XMLElement *theElement;
  XMLAttributePtr element_attrs;
  struct {
		double t, frequency, scale;
		double timeZone;
		UInt8 DST;
		struct{
			SInt32 forward, pilot, left, right;
		} channels;
  } data;
	xmlErr = XMLParserAddElement( &xmlParser, "vod.design", element_root, 0, &errors );
	
	xmlErr = XMLParserAddElement( &xmlParser, "frequency", element_frequency, 0, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_frequency, "fps",
								    attr_freq, attributeValueKindDouble, &errors );
	
	xmlErr = XMLParserAddElement( &xmlParser, "frequence", element_frequence, 0, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_frequence, "tps",
								    attr_freq, attributeValueKindDouble, &errors );
	
	xmlErr = XMLParserAddElement( &xmlParser, "utc", element_utc, 0, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_utc, "zone",
								    attr_zone, attributeValueKindDouble, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_utc, "dst",
								    attr_dst, attributeValueKindBoolean, &errors );
	
	xmlErr = XMLParserAddElement( &xmlParser, "scale", element_scale, 0, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_scale, "factor",
								    attr_scale, attributeValueKindDouble, &errors );
	
	xmlErr = XMLParserAddElement( &xmlParser, "echelle", element_echelle, 0, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_echelle, "facteur",
								    attr_scale, attributeValueKindDouble, &errors );
	
	xmlErr = XMLParserAddElement( &xmlParser, "channels", element_channels, 0, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_channels, "forward",
								    attr_forward, attributeValueKindInteger, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_channels, "pilot",
								    attr_pilot, attributeValueKindInteger, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_channels, "left",
								    attr_left, attributeValueKindInteger, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_channels, "right",
								    attr_right, attributeValueKindInteger, &errors );
	
	xmlErr = XMLParserAddElement( &xmlParser, "cnaaux", element_canaux, 0, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_canaux, "avant",
								    attr_forward, attributeValueKindInteger, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_canaux, "pilote",
								    attr_pilot, attributeValueKindInteger, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_canaux, "gauche",
								    attr_left, attributeValueKindInteger, &errors );
	xmlErr = XMLParserAddElementAttribute( &xmlParser, element_canaux, "droite",
								    attr_right, attributeValueKindInteger, &errors );
	xmlErr = ParseXMLFile( "test.xml", xmlParser,
			   xmlParseFlagAllowUppercase|xmlParseFlagAllowUnquotedAttributeValues|elementFlagPreserveWhiteSpace,
			   &xmldoc
	);
	memset( &data, 0, sizeof(data) );
	if( xmlErr == noErr && xmldoc->rootElement.identifier == element_root ){
		idx = 0;
		root_content = xmldoc->rootElement.contents;
		while( root_content[idx].kind != xmlContentTypeInvalid ){
			if( root_content[idx].kind == xmlContentTypeElement ){
				theElement = XMLElementContents(xmldoc, idx);
//				element_attrs = element_content[idx].actualContent.element.attributes;
				switch( theElement->identifier ){
					case element_frequency:
					case element_frequence:
						xmlErr = GetDoubleAttribute( theElement, attr_freq, &data.frequency );
						if( xmlErr != attributeNotFound ){
							fprintf( stderr, "freq=%g (%d)\n", data.frequency, xmlErr );
						}
						break;
					case element_utc:{
					  UInt8 temp;
						xmlErr = GetDoubleAttribute( theElement, attr_zone, &data.timeZone );
						if( xmlErr != attributeNotFound ){
							fprintf( stderr, "zone=%g (%d)\n", data.timeZone, xmlErr );
						}
						xmlErr = GetBooleanAttribute( theElement, attr_dst, &temp );
						if( xmlErr != attributeNotFound ){
							data.DST = (temp != 0);
							fprintf( stderr, "DST=%s (%d)\n", (data.DST)? "TRUE" : "FALSE", xmlErr );
						}
						break;
					}
					case element_scale:
					case element_echelle:
						xmlErr = GetDoubleAttribute( theElement, attr_scale, &data.scale );
						if( xmlErr != attributeNotFound ){
							fprintf( stderr, "scale=%g (%d)\n", data.scale, xmlErr );
						}
						break;
					case element_channels:
					case element_canaux:
						xmlErr = GetIntegerAttribute( theElement, attr_forward, &data.channels.forward );
						if( xmlErr != attributeNotFound ){
							fprintf( stderr, "forward=%d (%d)\n", (int) data.channels.forward, xmlErr );
						}
						xmlErr = GetIntegerAttribute( theElement, attr_pilot, &data.channels.pilot );
						if( xmlErr != attributeNotFound ){
							fprintf( stderr, "pilot=%d (%d)\n", (int) data.channels.pilot, xmlErr );
						}
						xmlErr = GetIntegerAttribute( theElement, attr_left, &data.channels.left );
						if( xmlErr != attributeNotFound ){
							fprintf( stderr, "left=%d (%d)\n", (int) data.channels.left, xmlErr );
						}
						xmlErr = GetIntegerAttribute( theElement, attr_right, &data.channels.right );
						if( xmlErr != attributeNotFound ){
							fprintf( stderr, "right=%d (%d)\n", (int) data.channels.right, xmlErr );
						}
						break;
					default:
						xmlErr = paramErr;
				}
			}
			idx++;
		}
	}
	xmlErr = DisposeXMLParser( &xmlParser, &xmldoc, 1 );
}
#else
void TestXMLParsing()
{
}
#endif

int main()
{
#if 0
    if (!SetThreadAffinityMask(GetCurrentThread(),0x2)) {
        std::cerr << "  SetThreadAffinityMask() failed - " << GetLastError() << std::endl;
        exit(-1);
    }
#endif
    OSErr rc;
#ifdef _QTPFUSAVEIMAGE_H
	QTpfuImageSaveData isd;
	unsigned int *dst;
#endif

    _x = 0; _y = 0; _w = 640; _h = 240;

    m_trackFrame.top    = (short)_x; 
    m_trackFrame.left   = (short)_y; 
    m_trackFrame.right  = (short)(_x+_w);
    m_trackFrame.bottom = (short)(_y+_h); 

    _imageD.resize(_w*_h*40);

	QTMovieSinks *qms;
	int err;
	unsigned long codec = /*cAnimationCodec*/ /*cJPEGCodec*/ QTCompressionCodec()->JPEG,
				quality = /*cNormalQuality*/ QTCompressionQuality()->Normal;
	unsigned char QMSframes = 32;

#if TARGET_OS_WIN32
	if( noErr != (rc=InitializeQTML(0)) ){
		std::cerr << "QTMovieFile::Initialize(), InitializeQTML(0) failed: " << rc << std::endl;
		exit(-1);
	}
#endif
	EnterMovies();

	TestXMLParsing();

	if( (qms = open_QTMovieSink( NULL, "ApplePhotoTIFF-QTMovieSinkICM.mov", _w, _h, TRUE, QMSframes, QTCompressionCodec()->TIFF, quality, TRUE, FALSE, &err )) ){
	  int frames;
	  std::ostrstream s;
	  QTMSEncodingStats stats;
	    std::cerr << qms->theURL << " : ";
	    for( frames = 0 ; frames < qms->frameBuffers ; frames++ ){
		    for( int i= 0; i < _w*_h; i++ ){
			    qms->imageFrame[frames][i].value = 0xDEADBABE;
		    }
	    }
	    init_HRTime();
	    SET_REALTIME();
	    HRTime_tic();
	    for( frames = 0; HRTime_toc() < 3; frames++ ){
		    QTMovieSink_AddFrame( qms, 0.016667 );
	    }
	    double elapsed = HRTime_toc();
	    EXIT_REALTIME();
	    s << frames << " frames in " << elapsed << " seconds - " << frames/elapsed << " fps" << std::ends;
	    std::cerr << s.str();
	    std::cerr << std::endl;
	    QTMovieSink_AddMovieMetaDataString( qms, akInfo, s.str(), NULL );
	    if( get_QTMovieSink_EncodingStats( qms, &stats ) ){
		    std::cerr << "Frames - Total: " << stats.Total << " Dropped: " << stats.Dropped << " Merged: " << stats.Merged
			<< " timeChanged: " << stats.timeChanged << " bufferCopied: " << stats.bufferCopied << std::endl;
	    }
	    close_QTMovieSink( &qms, TRUE, NULL, FALSE );
	    sleep(2);
	}
	else{
	    std::cerr << "open_QTMovieSinkICM() failed with error " << err << std::endl;
	}
	if( (qms = open_QTMovieSink( NULL, "ApplePhotoJPEG-QTMovieSink.mov",
						   _w, _h, TRUE, QMSframes, codec, quality, FALSE, FALSE, &err ))
	){
	  int frames;
	  std::ostrstream s;
	    std::cerr << qms->theURL << " : ";
	    for( frames = 0 ; frames < qms->frameBuffers ; frames++ ){
		    for( int i= 0; i < _w*_h; i++ ){
			    qms->imageFrame[frames][i].ciChannel.red = 0xDE;
			    qms->imageFrame[frames][i].ciChannel.green = 0xAD;
			    qms->imageFrame[frames][i].ciChannel.blue = 0xBA;
			    qms->imageFrame[frames][i].ciChannel.alpha = 0xBE;
		    }
	    }
	    init_HRTime();
	    SET_REALTIME();
	    HRTime_tic();
	    for( frames = 0; HRTime_toc() < 3; frames++ ){
		    QTMovieSink_AddFrame( qms, 0.016667 );
	    }
	    double elapsed = HRTime_toc();
	    EXIT_REALTIME();
	    s << frames << " frames in " << elapsed << " seconds - " << frames/elapsed << " fps" << std::ends;
	    std::cerr << s.str();
	    std::cerr << std::endl;
	    QTMovieSink_AddMovieMetaDataString( qms, akInfo, s.str(), NULL );
	    close_QTMovieSink( &qms, TRUE, NULL, FALSE );
	    sleep(2);
	    { QTMovieWindowH qwi;
		 QTMovieWindows qtbak;
		 NativeWindowH nwi;
#if TARGET_OS_MAC
		    InitQTMovieWindows();
#endif
			qwi = OpenQTMovieInWindow( "ApplePhotoJPEG-QTMovieSink.mov", TRUE );
			nwi = NativeWindowHFromQTMovieWindowH(qwi);
			if( nwi ){
				qtbak = **qwi;
				register_MCAction( qwi, MCAction()->Step, movieStep );
				register_MCAction( qwi, MCAction()->GoToTime, movieScan );
				register_MCAction( qwi, MCAction()->Play, moviePlay );
				register_MCAction( qwi, MCAction()->Finished, movieFinished );
				std::cerr << "Opened QT Movie window (native window=" << (void*) nwi << "->" << (void*) *nwi << "->" << **nwi << ")!" << std::endl;
			}
			while( QTMovieWindowH_isOpen(qwi) ){
//				std::cerr << "Handled " << PumpMessages(TRUE) << " messages" << std::endl;
//				ActivateQTMovieWindow(qwi);
				PumpMessages(TRUE);
				// movie playback seems to continue properly even when we sleep:
//				sleep(1);
			}
			if( qwi ){
				std::cerr << "QT Movie window closed (native window=" << (void*) nwi << "->" << (void*) *nwi << ")!" << std::endl;
				DisposeQTMovieWindow(qwi);
			}
	    }
		{ Movie theMovie;
			short id = 0;
			if( OpenMovieFromURL( &theMovie, 1, &id, "c:/Documents and Settings/MSIS/Bureau/LaneSplitting-full.qi2m", NULL, NULL ) == noErr ){
				SaveMovieAsRefMov( "LaneSplitting.mov", theMovie );
				CloseMovie( &theMovie );
			}
		}
	}
	else{
	    std::cerr << "open_QTMovieSink() failed with error " << err << std::endl;
	}
	if( (qms = open_QTMovieSink( NULL, "ApplePhotoJPEG-QTMovieSinkICM.mov", _w, _h, TRUE, QMSframes, codec, quality, TRUE, FALSE, &err )) ){
	  int frames;
	  std::ostrstream s;
	  QTMSEncodingStats stats;
	    std::cerr << qms->theURL << " : ";
	    for( frames = 0 ; frames < qms->frameBuffers ; frames++ ){
		    for( int i= 0; i < _w*_h; i++ ){
			    qms->imageFrame[frames][i].value = 0xDEADBABE;
		    }
	    }
	    init_HRTime();
	    SET_REALTIME();
	    HRTime_tic();
	    for( frames = 0; HRTime_toc() < 3; frames++ ){
		    QTMovieSink_AddFrame( qms, 0.016667 );
	    }
	    double elapsed = HRTime_toc();
	    EXIT_REALTIME();
	    s << frames << " frames in " << elapsed << " seconds - " << frames/elapsed << " fps" << std::ends;
	    std::cerr << s.str();
	    std::cerr << std::endl;
	    QTMovieSink_AddMovieMetaDataString( qms, akInfo, s.str(), NULL );
	    if( get_QTMovieSink_EncodingStats( qms, &stats ) ){
		    std::cerr << "Frames - Total: " << stats.Total << " Dropped: " << stats.Dropped << " Merged: " << stats.Merged
			<< " timeChanged: " << stats.timeChanged << " bufferCopied: " << stats.bufferCopied << std::endl;
	    }
	    close_QTMovieSink( &qms, TRUE, NULL, FALSE );
	    sleep(2);
	}
	else{
	    std::cerr << "open_QTMovieSinkICM() failed with error " << err << std::endl;
	}
#ifdef _QTPFUSAVEIMAGE_H
	if( init_QTpfuImageSaveData( &isd, "PFUSaveImage-opt.qi2m", _w, _h, TRUE )
	   && (dst = (unsigned int*) calloc( _w * _h, sizeof(unsigned int)))
	){
	  std::string frame = "";
	  std::string nrs="0123456789.rgb", fname = isd.theURL, ext = ".rgb";
	  std::ostrstream s;
	  char nr[32];
	  int frames;
		fname += "-opt-";
		std::cerr << isd.theURL << " : ";
		for( int i= 0; i < _w*_h; i++ ){
			dst[i] = 0xDEADBABE;
		}
		QTpfuImageAddDescription( &isd, "3 second QTpfuSaveImage benchmark", NULL );
		init_HRTime();
		SET_REALTIME();
		HRTime_tic();
		for( frames = 0; HRTime_toc() < 3; frames++ ){
			snprintf( nr, sizeof(nr), "%04u", frames );
			frame = fname + nr + ".rgb";
//			snprintf( (char*)nrs.c_str(), 13, "%04u.rgb", frames );
//			frame = fname + nrs;
			QTpfuSaveImage( &isd, frame.c_str(), dst, 0, 0, 0.016667 );
		}
		double elapsed = HRTime_toc();
		EXIT_REALTIME();
		s << frames << " frames in " << elapsed << " seconds - " << frames/elapsed << " fps" << std::ends;
		std::cerr << s.str();
		std::cerr << std::endl;
		QTpfuImageAddDescription( &isd, s.str(), NULL );
		free_QTpfuImageSaveData( &isd );
	}
#endif

    CodecNameSpecListPtr list;
    if (noErr != (rc=GetCodecNameList (&list, 1))) {
        std::cerr << "QTMovieFile::Initialize(), GetCodecNameList(0) failed: " << rc << std::endl;
        exit(-1);
    }
    for (int i=0; i<list->count; ++i) 
    {
        CodecNameSpec * p = &list->list[i];
        // 'cpix' resources are used by codecs to list their supported non-RGB pixel formats
        Handle cpix=NULL;
#if TARGET_OS_WIN32
	   OSType RGBpixelFormat = k32BGRAPixelFormat/*k32RGBAPixelFormat*/;
#else
	   OSType RGBpixelFormat = k32ARGBPixelFormat;
#endif
	    if( noErr == (rc = GetComponentPublicResource( p->codec, FOUR_CHAR_CODE('cpix'), 1, &cpix ))
			&& kJPEGCodecType == p->cType
		){
            int cpixFormatCount = GetHandleSize(cpix) / sizeof(OSType);
            for (int j = 0; j < cpixFormatCount; j++)
            {
                OSType* nextFmt = (OSType*)(cpix[0] + (j * sizeof(OSType)));
                CalcCompressionPerformance(*nextFmt, p->codec, p->cType, p->name, FALSE);
            }
            DisposeHandle(cpix);

		  { unsigned long *imageFrame = (unsigned long*) &_imageD[0];
			  for( int f= 0; f < _w*_h; f++ ){
				  imageFrame[f] = 0xDEADBABE;
			  }
		  }
		  CalcCompressionPerformance(RGBpixelFormat, p->codec, p->cType, p->name, FALSE);
//		  i += 1;
        }
	   else{
		   if( GetComponentPublicResource( p->codec, FOUR_CHAR_CODE('cpix'), 1, &cpix ) == noErr ){
			int cpixFormatCount = GetHandleSize(cpix) / sizeof(OSType);
			   for( int j = 0; j < cpixFormatCount; j++ ){
				OSType* nextFmt = (OSType*)(cpix[0] + (j * sizeof(OSType)));
				   CalcCompressionPerformance(*nextFmt, p->codec, p->cType, p->name, TRUE);
			   }
		   }
		   else{
			   CalcCompressionPerformance(RGBpixelFormat, p->codec, p->cType, p->name, TRUE);
		   }
	   }
    }
    if (noErr != (rc=DisposeCodecNameList(list))) {
        std::cerr << "QTMovieFile::Initialize(), DisposeCodecNameList(0) failed: " << rc << std::endl;
        exit(-1);
    }

	getchar();
    ExitMovies();

#if TARGET_OS_WIN32
    TerminateQTML();
#endif
    return (int) rc;
}

#if 0
Dell Precision T5500, Xeon E5560 2.13Ghz:
Photo Apple - JPEG - 2vuy  : 1030 frames in 3.00243 seconds - 343.056
Photo Apple - JPEG - yuvu  QTMovieFile::Initialize(), CVPixelBufferCreateWithBytes() failed - -6680
Photo Apple - JPEG - yuvs  : 1033 frames in 3.0021 seconds - 344.092
Photo Apple - JPEG - r408  : 1014 frames in 3.00168 seconds - 337.811
Photo Apple - JPEG - v408  : 1007 frames in 3.00032 seconds - 335.631
ApplePhotoJPEG-QTMovieSink.mov : 616 frames in 3.00154 seconds - 205.228 fps

Toshiba Pentium-M 1.6Ghz:
Photo Apple - JPEG - 2vuy  : 719 frames in 3.00118 seconds - 239.573
Photo Apple - JPEG - yuvu  QTMovieFile::Initialize(), CVPixelBufferCreateWithBytes() failed - -6680
Photo Apple - JPEG - yuvs  : 687 frames in 3.00122 seconds - 228.907
Photo Apple - JPEG - r408  : 668 frames in 3.00077 seconds - 222.609
Photo Apple - JPEG - v408  : 693 frames in 3.00282 seconds - 230.783
Photo Apple - JPEG - BGRA  : 291 frames in 3.00897 seconds - 96.7108
ApplePhotoJPEG-QTMovieSink.mov : 390 frames in 3.00376 seconds - 129.837 fps
ApplePhotoJPEG-QTMovieSinkICM.mov : 309 frames in 3.00603 seconds - 102.793 fps
Frames - Dropped: 0 Merged: 0 timeChanged: 0 bufferCopied: 309

Powerbook G4 1.5Ghz (quite busy with lots of other things); 32 ICM buffers:
[Session started at 2010-09-15 14:24:19 +0200.]
Apple Photo - JPEG - 2vuy  : 1298 frames in 3.0004 seconds - 432.609
Apple Photo - JPEG - yuvu  QTMovieFile::Initialize(), CVPixelBufferCreateWithBytes() failed - -6680
Apple Photo - JPEG - yuvs  : 636 frames in 3.00241 seconds - 211.83
Apple Photo - JPEG - r408  : 551 frames in 3.00469 seconds - 183.38
Apple Photo - JPEG - v408  : 591 frames in 3.00226 seconds - 196.851
Apple Photo - JPEG - RGB  : 348 frames in 3.00477 seconds - 115.816
ApplePhotoJPEG-QTMovieSink.mov : 592 frames in 3.00061 seconds - 197.293 fps
ApplePhotoJPEG-QTMovieSinkICM.mov : 517 frames in 3.00238 seconds - 172.197 fps
Frames - Dropped: 0 Merged: 0 timeChanged: 0 bufferCopied: 517

Powermac 2*G5 1.8Ghz; 32 ICM buffers:
[Session started at 2010-09-15 15:06:54 +0200.]
Apple Photo - JPEG - 2vuy  : 1472 frames in 3.00273 seconds - 490.221
Apple Photo - JPEG - yuvu  QTMovieFile::Initialize(), CVPixelBufferCreateWithBytes() failed - -6680
Apple Photo - JPEG - yuvs  : 1257 frames in 3.0004 seconds - 418.944
Apple Photo - JPEG - r408  : 1225 frames in 3.00037 seconds - 408.282
Apple Photo - JPEG - v408  : 1186 frames in 3.00183 seconds - 395.092
Apple Photo - JPEG - RGB  : 992 frames in 3.00167 seconds - 330.483
ApplePhotoJPEG-QTMovieSink.mov : 1234 frames in 3.00211 seconds - 411.044 fps
ApplePhotoJPEG-QTMovieSinkICM.mov : 963 frames in 3.00175 seconds - 320.813 fps
Frames - Dropped: 0 Merged: 0 timeChanged: 0 bufferCopied: 963

MacBookPro Core2Duo 2.4Ghz OSX 10.6.5; 32 ICM buffers:
Apple Photo - JPEG - yuv2  : 2230 frames in 3.00105 seconds - 743.074
Apple Photo - JPEG - uvuy  QTMovieFile::Initialize(), CVPixelBufferCreateWithBytes() failed - -6680
Apple Photo - JPEG - svuy  : 2242 frames in 3.00089 seconds - 747.111
Apple Photo - JPEG - 804r  : 2057 frames in 3.00042 seconds - 685.57
Apple Photo - JPEG - 804v  : 1986 frames in 3.00105 seconds - 661.769
Apple Photo - JPEG - RGB  : 1403 frames in 3.00048 seconds - 467.591
ApplePhotoJPEG-QTMovieSink.mov : 1559 frames in 3.00041 seconds - 519.595 fps
ApplePhotoJPEG-QTMovieSinkICM.mov : 1376 frames in 3.00086 seconds - 458.535 fps
Frames - Dropped: 0 Merged: 0 timeChanged: 0 bufferCopied: 1376

#endif
