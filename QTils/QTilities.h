/*!
 *  @file QTilities.h
 *  QTilities
 *
 *  Created by René J.V. Bertin on 20101122.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 */

#ifndef _QTILITIES_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(__DOXYGEN__)
// we're being scanned by doxygen: define some tokens that would get defined by headers
// that doxygen won't find, on this platform:
#	ifdef __APPLE_CC__
#		define TARGET_OS_MAC	1
#		define _QTMW_M
#	else
#		define WIN32
#		define __WIN32__
#		define _WINDOWS_
#		define TARGET_OS_WIN32	1
#	endif
#endif

#if defined(_WIN32) || defined(__WIN32__)
#	ifdef _QTILS_C
#		define QTLSext __declspec(dllexport)
#	else
#		define QTLSext __declspec(dllimport)
#	endif
#else
#	define QTLSext /**/
#endif

typedef struct QTils_Allocators {
	void* (*malloc)( size_t s );
	void* (*calloc)( size_t n, size_t s );
	void* (*realloc)( void* mem, size_t size );
	void (*free)( void **mem );
} QTils_Allocators;

QTLSext QTils_Allocators *init_QTils_Allocator( void* (*mallocPtr)(size_t), void* (*callocPtr)(size_t,size_t),
							    void* (*reallocPtr)(void*, size_t), void (*free)(void **) );
QTLSext extern QTils_Allocators *QTils_Allocator;
QTLSext void *QTils_malloc( size_t s );
QTLSext void *QTils_calloc( size_t n, size_t s );
QTLSext void *QTils_realloc( void* mem, size_t size );
QTLSext void QTils_freep( void **mem );
#define QTils_free(x)	QTils_freep((void**)(x))
QTLSext char *QTils_strdup( const char *txt );

/*!
	When USE_QTHANDLES is set, the QTMovieWindows structures are allocated through QuickTime's NewHandleClear,
	otherwise, through the default calloc() routine (and in that case, a handle is created from the the address
	of the newly allocated structure's self member.
 */
#define USE_QTHANDLES

#ifndef _QTMOVIESINK_H
typedef enum AnnotationKeys { akAuthor='auth', akComment='cmmt', akCopyRight='cprt', akDisplayName='name',
	akInfo='info', akKeywords='keyw', akDescr='desc', akFormat='orif', akSource='oris',
	akSoftware='soft', akWriter='wrtr', akYear='year', akCreationDate='@day', akTrack='@trk'
} AnnotationKeys;
#endif

/*!
	a point in 2-D Cartesian space (i.e. a screen)
 */
typedef struct Cartesian {
	short vertical, horizontal;
} Cartesian;

#ifdef __GNUC__
#	include <stdio.h>
#	include <sys/types.h>
#	define GCC_PACKED	__attribute__ ((packed))
#else
#	define GCC_PACKED	/**/
#endif

/*!
	201011116: TimeCode track management: the data used to define a TC track are stored in a separate table
	that can be used to check if a given media source (theURL) already has TC track for a given period.
	If so, one only needs to add a reference for that TC track to the track in which the media is being imported.
 */
typedef struct TCTrackInfo {
	char *theURL;
	void *theTCTrack;
	int N;
	double *StartTimes, *durations;
	size_t *frames;
	void* (*dealloc)(void *);
	struct TCTrackInfo *cdr;
} TCTrackInfo;

#ifndef _QTMOVIESINK_H
	typedef int				ErrCode;
#endif

#if !defined(__QUICKTIME__) && !defined(__MOVIES__)
	// QuickTime headers not included:
	typedef unsigned int		OSType;
	typedef long				TimeValue;
	typedef unsigned char		Boolean;
	typedef struct MovieType**	Movie;
	typedef struct TrackType**	Track;
	typedef char**				Handle;
	typedef struct QTCallBackHeader*	QTCallBack;
	typedef void				(*QTCallBackUPP)(QTCallBack cb, long refCon);
#	define noErr				0
	typedef struct TimeCodeTime {
		unsigned char hours;
		unsigned char minutes;
		unsigned char seconds;
		unsigned char frames;
	}						MovieFrameTime;

	// from Movies.h:
	enum kMovieLoadStates {
	  kMovieLoadStateError          = -1L,
	  kMovieLoadStateLoading        = 1000,
	  kMovieLoadStateLoaded         = 2000,
	  kMovieLoadStatePlayable       = 10000,
	  kMovieLoadStatePlaythroughOK  = 20000,
	  kMovieLoadStateComplete       = 100000L
	};

	typedef unsigned char		Str255[256];

	typedef unsigned char		UInt8;
	typedef unsigned short		UInt16;
	typedef short				SInt16;
#if __LP64__
	typedef unsigned int		UInt32;
	typedef int				SInt32;
#else
	typedef unsigned long		UInt32;
	typedef long				SInt32;
#endif

	enum ErrCodes {
		paramErr	=	-50
	};

#ifdef _MSC_VER
// QuickTime defines the EventRecord structure in such a way that its members are aligned
// without padding. We need to ensure we do the same here.
// NB: on GCC, this would be done by adding __attribute__ ((packed)) after the type's closing brace
#	pragma pack(show)
#	pragma pack(push,1)
#endif

	// from MacTypes.h:
	typedef struct Rect {
	  short               top;
	  short               left;
	  short               bottom;
	  short               right;
	}						Rect;

	typedef UInt16				EventKind;
	typedef unsigned short		EventModifiers;
/*!
	the QuickTime event record (actually the old QuickDraw engine...)
 */
	typedef struct EventRecord {
		EventKind what;
		UInt32 message;
		UInt32 when;
		Cartesian where;
		EventModifiers modifiers;
	}		GCC_PACKED		EventRecord;
#ifdef _MSC_VER
#	pragma pack(pop)
#	pragma pack(show)
#endif

#else // QuickTime Headers included:

// include a few additional QuickTime headers:
#	ifdef __APPLE_CC__
#		include <QuickTime/QuickTimeComponents.h>
#	else
#		include <QuickTimeComponents.h>
#	endif
#endif

#ifndef _QTKITDEFINES_H
	typedef void				QTMovie;
#endif // _QTKITDEFINES_H

/*!
	convert a Pascal string to a C string using a static buffer internal to the function
 */
QTLSext char *P2Cstr( Str255 pstr );
/*!
	returns the textual representation of a Mac FOURCHARCODE (e.g. 'JPEG')
	in a static string variable that gets changed on each invocation.
 */
QTLSext char *OSTStr( OSType type );

/*!
	presents an 'Open File' dialog
 */
QTLSext char *AskFileName(char *title);

/*!
	presents a 'Save File' dialog
 */
QTLSext char *AskSaveFileName(char *title);

#if TARGET_OS_MAC || defined(__APPLE_CC__) || defined(__MACH__)

	/*!
		presents a 'Yes/No' dialog with the specified title and message texts
	 */
	QTLSext int PostYesNoDialog( const char *title, const char *message );

	/*!
		presents a message window with the specified title and message texts
	 */
	QTLSext int PostMessage( const char *title, const char *message );
#endif

/*!
	returns the error string and optionally the error comment/description for the
	given error code. Works for Macintosh and POSIX/Unix error codes.
 */
QTLSext const char *MacErrorString( ErrCode err, const char **errComment );

/*!
	open a movie from a file; pass NULL for dum1 & id, 0 or 1 for flags
	@n
	Given the file <URL>, open it as a QuickTime movie. The function returns the Movie object (in-memory "handle")
	and the reference/type pair if requested. flags should be 0 or 1. <id> refers to the file's movie resource(s)
	and should be 0 or NULL. Internally, an association is made with a newly allocated and initialised QTMovieWindowH
	object - evidently windowless. This object can be retrieved via the QTMovieWindowH_from_Movie function.
 */
QTLSext ErrCode OpenMovieFromURL( Movie *newMovie, short flags, short *id,
						   const char *URL, Handle *dum1, OSType *type );
/*!
	 saves a movie in its file on disk.
 */
QTLSext ErrCode SaveMovie( Movie theMovie );
/*!
	 save a movie as a reference movie, without posting any dialogs.
	 @n
	A reference movie is a sort of linked copy that can have its own metadata and edits, but
	obtains its media from the original file. NB: if <theMovie> is itself a reference movie,
	the new copy will refer to the original source material, not to <theMovie>!
 */
QTLSext ErrCode SaveMovieAsRefMov( const char *dstURL, Movie theMovie );
/*!
	save a movie. If URL points to a valid filename, that name is used - as the default filename
	in the Save-As dialog that will be posted when noDialog=FALSE. If URL is not NULL, it might contain
	a copy of the selected filename upon success (to be freed by the user).
 */
QTLSext ErrCode SaveMovieAs( char **URL, Movie theMovie, int noDialog );
/*!
	Flattens a movie, that is, save it as a self-contained movie to the file dstURL .
	If theNewMovie is not NULL, the new Movie object is returned therein.
 */
QTLSext ErrCode FlattenMovieToURL( const char *dstURL, Movie theMovie, Movie *theNewMovie );
/*!
	closes a Movie object, and de-allocates all associated resources (including the possible QTMovieWindowH).
	theMovie will be NULL upon a successful exit.
 */
QTLSext ErrCode CloseMovie( Movie *theMovie );

typedef struct MemoryDataRef {
#if defined(__QUICKTIME__) || defined(__MOVIES__)
	Handle dataRef;		//!< dataRef pointing to in-memory content, e.g. a QI2M file
	OSType dataRefType;		//!< the dataRef type ('hndl')
	Handle memory;			//!< the actual in-memory content
#else
	void **dataRef;
	OSType dataRefType;
	void **memory;
#endif
	const char *virtURL;	//!< a description/reference/tag, a virtual URL for the dataRef
} MemoryDataRef;

QTLSext void DisposeMemoryDataRef(MemoryDataRef *memRef);
QTLSext ErrCode MemoryDataRefFromMemory( const char *string, size_t len, const char *virtURL, MemoryDataRef *memRef );
QTLSext ErrCode MemoryDataRefFromString( const char *string, const char *virtURL, MemoryDataRef *memRef );
QTLSext ErrCode OpenMovieFromMemoryDataRef( Movie *newMovie, MemoryDataRef *memRef, OSType contentType );

#if defined(__QUICKTIME__) || defined(__MOVIES__)
	QTLSext void DisposeDataRef(Handle dataRef);
	QTLSext ErrCode DataRefFromURL( const char **URL, Handle *dataRef, OSType *dataRefType );

	QTLSext ErrCode AnchorMovie2TopLeft( Movie theMovie );
	QTLSext void GetMovieDimensions( Movie theMovie, Fixed *width, Fixed *height );
	QTLSext void ClipFlipCurrentTrackToQuadrant( Movie theMovie, Track theTrack,
								 short quadrant, short hflip, short withMetaData
	);
	QTLSext ErrCode GetUserDataFromMovie( Movie theMovie, void **data, size_t *dataSize, OSType udType );
	QTLSext ErrCode AddUserDataToMovie( Movie theMovie, void *data, size_t dataSize, OSType udType, int replace );
	QTLSext ErrCode AddMetaDataStringToTrack( Movie theMovie, Track theTrack,
							    AnnotationKeys key, const char *value, const char *lang );
	QTLSext ErrCode AddMetaDataStringToMovie( Movie theMovie, AnnotationKeys key, const char *value, const char *lang );
	QTLSext ErrCode GetMetaDataStringFromTrack( Movie theMovie, Track theTrack,
							    AnnotationKeys key, char **value, char **lang );
	QTLSext ErrCode GetMetaDataStringFromMovie( Movie theMovie, AnnotationKeys key, char **value, char **lang );
	QTLSext int CopyMovieMetaData( Movie dstMovie, Movie srcMovie );
	QTLSext int CopyTrackMetaData( Movie dstMovie, Track dstTrack, Movie srcMovie, Track srcTrack );
	QTLSext ErrCode CopyMovieTracks( Movie dstMovie, Movie srcMovie );
	QTLSext ErrCode GetMediaStaticFrameRate( Media inMovieMedia, double *outFPS );
	QTLSext ErrCode GetMovieStaticFrameRate( Movie theMovie, double *outFPS, double *fpsTC, double *fpsMedia );
	QTLSext ErrCode GetMovieStartTime( Movie theMovie, Track TCTrack, TimeRecord *tr, long *startFrameNr );
	QTLSext void SetMovieStartTime( Movie theMovie, TimeValue startTime, int changeTimeBase );
	QTLSext ErrCode SlaveMovieToMasterMovie( Movie slave, Movie master );
	QTLSext void qtSetMoviePlayHints( Movie theMovie, unsigned long hints, int exclusive );
	QTLSext ErrCode NewTimedCallBackRegisterForMovie( Movie movie, QTCallBack *callbackRegister, int allowAtInterrupt );
	QTLSext ErrCode TimedCallBackRegisterFunctionInTime( Movie movie, QTCallBack cbRegister,
											  double time, QTCallBackUPP function, long data, int allowAtInterrupt);	
	QTLSext ErrCode DisposeCallBackRegister( QTCallBack cbRegister );

	typedef struct MediaRefs {
		Media theMedia;
		Handle dataRef;
		OSType dataRefType;
	} MediaRefs;

	QTLSext TCTrackInfo *dispose_TCTrackInfoList( TCTrackInfo *list );
	QTLSext ErrCode AddTimeCodeTrack( Movie theMovie, Track theTrack, double trackStart, TCTrackInfo **TCInfoList,
				   int N, double *StartTimes, double *durations, size_t *frames,
				   Cartesian *translate, const char *theURL );

	typedef TimeCodeTime MovieFrameTime;
#else // !QuickTime
	QTLSext unsigned short HasMovieChanged_Mod2( Movie theMovie );
#	ifndef _QTILS_C
#		define HasMovieChanged(m)	HasMovieChanged_Mod2(m)
#	endif //!_QTILS_C
#endif

/*!
	tries to find the given text in the given movie, and returns information concerning the hit.
	@param	theMovie	movie in which to search. All tracks are searched, but the search (might) start
					with the input value of inoutTrack
	@param	text		text pattern to find. Search is case INsensitive.
	@param	displayResult	when TRUE, the movie is spooled to the time found, and the pattern is highlighted.
	@param	inoutTrack	input: track to start with, or NULL. Output: the Track in which a hit was found.
	@param	foundTime		the time at which the hit was found, relative to the movie start
	@param	foundOffset	the offset in the movie text string where the search pattern was found.
	@param	foundText		if pointing to a char* initialised to NULL, a _copy_ of the string containing the hit
						is returned via this parameter.
 */
QTLSext ErrCode FindTextInMovie( Movie theMovie, char *text, int displayResult,
				    Track *inoutTrack, double *foundTime, long *foundOffset, char **foundText );
/*!
	returns a movie's associated chapter track, or NULL if there's none. If the movie has an
	associated QTMovieWindowH object, the information is obtained from there (as long as it's not NULL).
 */
//QTLSext Track GetMovieChapterTrack( Movie theMovie, QTMovieWindowH *rwih );
/*!
	returns the number of chapters in a movie's chapter track, or -1 if there is no such track
	or an error occurred.
 */
QTLSext long GetMovieChapterCount( Movie theMovie );
/*!
	Returns time in seconds and/or text of a movie's specific chapter number. Either time and/or text can be NULL
	to ignore that information.
 */
QTLSext ErrCode GetMovieIndChapter( Movie theMovie, long ind, double *time, char **text );
QTLSext ErrCode MovieAddChapter( Movie theMovie, Track refTrack, const char *name,
				    double time, double duration );
/*!
	Return a track's name
 */
QTLSext ErrCode GetTrackName( Movie theMovie, Track theTrack, char **trackName );
/*!
	Return a movie's track that has the given name. Specify a type to search only
	for tracks of the given type, or type=0 to search among all tracks. If trackNr!=NULL
	it will contain the index of the found track upon return.
 */
QTLSext ErrCode GetTrackWithName( Movie theMovie, char *trackName, OSType type, long flags, Track *theTrack, long *trackNr );

/*!
	Get a track's content type (for instance, 'vide' for video) and subtype (e.g. 'mp4v' for MPEG4)
	Both trackType and trackSubType can be NULL if the information is not required.
 */
QTLSext ErrCode GetMovieTrackTypes( Movie theMovie, Track theTrack, OSType *trackType, OSType *trackSubType );

/*!
	As GetMovieTrackTypes, but takes a track number instead of a track pointer.
 */
QTLSext ErrCode GetMovieTrackNrTypes( Movie theMovie, long trackNr, OSType *trackType, OSType *trackSubType );

/*!
	Obtains descriptive information of a movie track's content (sub) type and the name and manufacturer
	of the component used for decompression. componentName and/or componentManufacturer can be NULL pointers.
 */
QTLSext ErrCode GetMovieTrackDecompressorInfo( Movie theMovie, Track theTrack, OSType *trackSubType,
							   char **componentName, OSType *componentManufacturer );

/*!
	As GetMovieTrackDecompressorInfo but takes a track number instead of a track pointer.
 */
QTLSext ErrCode GetMovieTrackNrDecompressorInfo( Movie theMovie, long trackNr, OSType *trackSubType,
							   char **componentName, OSType *componentManufacturer );

/*!
	a movie's duration in seconds
 */
QTLSext double GetMovieDurationSeconds(Movie theMovie);
/*!
 smallest time lapse that can be represented on this movie's timescale
 */
QTLSext double GetMovieTimeResolution(Movie theMovie);

QTLSext MovieFrameTime *secondsToFrameTime( double Time, double MovieFrameRate, MovieFrameTime *ft );
#define FTTS(ft,rate)	((ft)->hours * 3600.0 + (ft)->minutes * 60.0 + (ft)->seconds + ((double) (ft)->frames / rate))

#if TARGET_OS_MAC
	extern Boolean QTils_MessagePumpIsInActive, QTils_MessagePumpIsSecondary;
#endif
/*!
	a minimal event message handler that can be used to 'pump' the message queue. That is, it empties
	the message queue of all currently queued messages. If <force> is True, the function waits for a
	message to appear before emptying the queue; if False, it exists immediately if no messages are waiting.
	The simplest event loop (to be used when nothing is to be done except for handling messages to open windows)
	that doesn't hog the processor with a busy-wait:
	@code
while( canContinue ){
	PumpMessages(TRUE);
}
	@endcode
	This is possible without playback saccades because QuickTime handles that aspect on a separate thread.
 */
QTLSext unsigned int PumpMessages(int force);

/*!
	initialise the QuickTime engine
 */
QTLSext ErrCode OpenQT();
QTLSext void CloseQT();
QTLSext ErrCode LastQTError();

/*!
	define the type of the native window object
 */
#ifdef _WINDOWS_
	typedef HWND NativeWindow;
	typedef DWORD NativeError;
#else
	typedef struct NSWindow* NativeWindow;
	typedef long NativeError;
#endif
typedef NativeWindow*	NativeWindowPtr;
typedef NativeWindowPtr* NativeWindowH;

/*!
	informational variables that (may) have more restrictive alignment requirements,
	to avoid padding issues.
	NB: doubles are aligned on 8-byte boundaries using padding ... take this into account
	when interfacing from e.g. Modula-2! Listing them all together at the beginning of this
	structure should avoid any alignment issues.
 */
#ifdef _MSC_VER
#	pragma pack(push,1)
#endif
typedef struct QTMovieInfo {
	double frameRate, TCframeRate,
		duration,
		startTime;
	TimeValue timeScale;
	long startFrameNr;
	Rect naturalBounds;
	short controllerHeight;
} GCC_PACKED QTMovieInfo;
#ifdef _MSC_VER
#	pragma pack(pop)
#endif

/*!
	internal representation of a QTMovieWindow
	The members from self through user_data are 'public' and exported to Modula-2;
	the remaining members are of internal interest only.
 */
typedef struct QTMovieWindows {
	struct QTMovieWindows *self;		//!< a pointer to the structure itself
	NativeWindow theView,			//!< a handle on the native window, or NULL if there is no (more) window
		*theViewPtr;
	const char *theURL;				//!< the movie's filename or http URL
	int idx;						//!< a identifier of the movie window; initialised with a counter, but can be given any value

	int isPlaying,					//!< is the movie playing?
		isActive;					//!< is the movie window the active window?
	long loadState;				//!< a load progress indicator for streamed movies
	int wasStepped,				//!< was this movie just stepped (step = 1 frame jump)?
		wasScanned;				//!< was this movie just scanned (any jump)?
	int handlingEvent,				//!< an event is being handled for this window
		shouldClose;				//!< the window should be closed - once it's out of its event processing loop!
	Movie theMovie;				//!< the open QuickTime movie object
	QTMovieInfo *info;				//!< a pointer to the theInfo structure in the private section
	void *user_data;				//!< a hook for optional user data.
#if (defined(__QUICKTIME__) || defined(__MOVIES__))
	// --- opaque part!
	MovieController theMC;
	Track theTCTrack, theTimeStampTrack, theChapterTrack, theChapterRefTrack;
	Media theChapterMedia;
	MediaHandler theTCMediaH;
	TimeCodeDef tcdef;
	Rect theMovieRect;
	Handle dataRef;
	OSType dataRefType;
	DataHandler dataHandler;
	MemoryDataRef *memRef;			//!< set if the movie was opened from an in-memory dataRef
	short resId;
#ifdef _WINDOWS_
	WINDOWPOS gOldWindowPos;			//!< to keep track of the window's previous position
#endif // _WINDOWS_
#if TARGET_OS_MAC
//	struct NSAutoreleasePool *pool;	//!< Objective-C memory allocation pool
	int performingClose;			//!< window is being closed
								//!< this state is set while the windowShouldClose window delegate executes
								//!<
#	ifdef _QTMW_M
	IBOutlet struct NSQTMovieWindow *theNSQTMovieWindow;	//!<  receives an instance of our NSDocument class after loading the NIB
#	else
	struct NSQTMovieWindow *theNSQTMovieWindow;
#	endif
	struct QTMovieView *theMovieView;	//!< receives a pointer to the QTMovieView showing our Movie
	void *NSMCActionList;			//!< pointer to a C++ map associating MCAction values with user-specified callbacks into
								//!< an ObjC class "owning" the QTMovieWindowH
								//!<
#endif // TARGET_OS_MAC
	void *MCActionList;				//!< pointer to a C++ map associating MCAction values with user-specified callbacks
	float lastActionTime;
	short lastAction, stepPending;
	int hasAnyMCAction;
	SInt64 stepStartTime;
	QTMovieInfo theInfo;			//!< source for the info member in the public section.
#endif //__QUICKTIME__
} QTMovieWindows;

typedef struct QTMovieWindows* QTMovieWindowPtr;
/*!
	most user manipulations will be on a QTMovieWindow handle, a pointer to a pointer to a QTMovieWindows structure
 */
typedef QTMovieWindowPtr* QTMovieWindowH;

/*!
	check if a QTMovieWindowH references a valid QTMovieWindow
 */
#ifdef _QTILS_C
#	define QTMovieWindowH_Check(wih)	((wih) && NativeWindow_from_QTMovieWindowH((wih)) && (*wih)->self == (*wih))
#else
#	define QTMovieWindowH_Check(wih)	_QTMovieWindowH_Check_((wih))
#endif // _QTILS_C
/*!
	check if a QTMovieWindowH references a valid QTMovieWindow with an open window
 */
#define QTMovieWindowH_isOpen(wih)	(QTMovieWindowH_Check(wih) && (*wih)->theView)

/*!
	public, exported names for the supported QuickTime MovieController actions
	A number of (sorely missing) non-QuickTime actions are defined as well, corresponding
	to starting and stopping a movie, as well as closing one.
 */
typedef struct MCActions {
	short
		Step,		//!< Step: the action sent when the user 'steps' the movie with the cursor keys. The action
					//!< is received *before* the step is executed!
					//!< NB: we know in which direction the movie is being stepped:
					//!< params==1 -> forward; params==-1 -> backward
					//!< The action is not yet finished when the next Play action is received, but during a subsequent
					//!< Idle action. That Idle action will call the Step MCActionCallback with param==NULL
					//!< Play: received when the movie is about to be played, even if for only a single frame
					//!< (i.e. to display a new current frame).
					//!<
		Play,
		Activate,		//!< Activate: sent each time the movie is activated
		Deactivate,
		GoToTime,		//!< GoToTime: sent when the current movie time is about to be changed, including when the
					//!< user steps or scans the movie. The params argument passed to the MCActionCallback points
					//!< to a double containing the movie's new time, in seconds.
					//!< The action is finished when the next Play action is received.
					//!<
		MouseDown,
		MovieClick,
		KeyUp,
		Suspend,
		Resume,
		LoadState,	//!< LoadState: actions received during loading of streamed movies. params==kMovieLoadStateComplete
					//!< when the movie has been loaded fully. The loadState variable in the QTMovieWindow will NOT
					//!< yet have been updated! (MCActions arrive before the action is executed)
					//!< Finished: sent when the movie has finished
					//!<
		Finished,
		Idle,		//!< For completeness sake: the Idle action which can be received *very* often
		Start,		//!< non-QT action
		Stop,		//!< non-QT action
		Close,		//!< non-QT action
		AnyAction;	//!< non-QT action: used to install an MCActionCallback that is called at the reception of any
					//!< MCAction event, before the dedicated handler. The params argument will point to an Int16
					//!< containing the action identifier, and the return value is IGNORED.
} MCActions;

QTLSext int _QTMovieWindowH_Check_( QTMovieWindowH wih );
/*!
	a priori no need to call InitQTMovieWindows() - done in dllmain (Win32) or OpenQT (OS X) !
 */
#if defined(__APPLE_CC__) || defined(__MACH__)
QTLSext short InitQTMovieWindows();
#else
QTLSext short InitQTMovieWindows( void *Win32ApplicationInstance );
#endif
QTLSext void DisposeQTMovieWindow( QTMovieWindowH WI );
QTLSext ErrCode CloseQTMovieWindow( QTMovieWindowH WI );
/*!
	open the movie with the specified URL in a movie window.
	@param	theURL	filename or http URL
	@param	visibleController	whether the movie controller is visible or not
 */
QTLSext QTMovieWindowH OpenQTMovieInWindow( const char *theURL, int visibleController );
QTLSext NativeWindowH NativeWindowHFromQTMovieWindowH( QTMovieWindowH wi );
QTLSext QTMovieWindowH OpenQTMovieFromMemoryDataRefInWindow( MemoryDataRef *memRef, OSType contentType, int controllerVisible );
QTLSext QTMovieWindowH OpenQTMovieWindowWithMovie( Movie theMovie, const char *theURL, short resId,
								    Handle dataRef, OSType dataRefType, int controllerVisible );

QTLSext ErrCode ActivateQTMovieWindow( QTMovieWindowH wih );
/*!
	sets the flags that signal that the movie should play all frames, regardless of whether the
	hardware can keep up. Note that these appear to be just hints, QTMovieWindowPlay and other
	start-playing calls do not seem to heed this setting.
 */
QTLSext ErrCode QTMovieWindowSetPlayAllFrames( QTMovieWindowH WI, int onoff, int *curState );
/*!
	requests a playback speed rate: 1 is normal speed
 */
QTLSext ErrCode QTMovieWindowSetPreferredRate( QTMovieWindowH WI, int rate, int *curRate );
QTLSext ErrCode QTMovieWindowPlay( QTMovieWindowH wih );
QTLSext ErrCode QTMovieWindowStop( QTMovieWindowH wih );
QTLSext ErrCode QTMovieWindowToggleMCController( QTMovieWindowH wih );

QTLSext ErrCode QTMovieWindowSetGeometry( QTMovieWindowH wih, Cartesian *pos, Cartesian *size, double sizeScale, int setEnvelope );
QTLSext ErrCode QTMovieWindowGetGeometry( QTMovieWindowH wih, Cartesian *pos, Cartesian *size, int getEnvelope );

QTLSext ErrCode QTMovieWindowGetTime( QTMovieWindowH wih, double *t, int absolute );
QTLSext ErrCode QTMovieWindowGetFrameTime( QTMovieWindowH wih, MovieFrameTime *ft, int absolute );
QTLSext ErrCode QTMovieWindowSetTime( QTMovieWindowH wih, double t, int absolute );
QTLSext ErrCode QTMovieWindowSetFrameTime( QTMovieWindowH wih, MovieFrameTime *ft, int absolute );
/*!
	Step the movie <steps> over frames; negative values mean backward stepping.
 */
QTLSext ErrCode QTMovieWindowStepNext( QTMovieWindowH wih, int steps );

/*!
	attempts to find the specified text in the movie's text track called 'timeStamp Track'
	if foundTime != NULL, it will contain the actual time the text was found.
	On success, foundText will point to a new string that will have to be freed by the user.
 */
QTLSext ErrCode FindTimeStampInMovieAtTime( Movie theMovie, double Time, char **foundText, double *foundTime );

QTLSext ErrCode SampleNumberAtMovieTime( Movie theMovie, Track theTrack, double t, long *sampleNum );

/*!
	returns a movie's associated chapter track, or NULL if there's none. If the movie has an
	associated QTMovieWindowH object, the information is obtained from there (as long as it's not NULL).
 */
QTLSext Track GetMovieChapterTrack( Movie theMovie, QTMovieWindowH *rwih );

// NB: on MSVisualC, one has to store the MCActionCallback value into a variable of type
// Boolean (*fun)(QTMovieWindowH)
// before calling, for some obscure reason!
/*!
	the type of a callback function for QuickTime moviecontroller actions
	@param	wi	a QTMovieWindow handle
	@param	params	to be interpreted as a function of the action received; see the discussion of the MCActions structure
 */
typedef int (*MCActionCallback)(QTMovieWindowH wi, void *params);

#if TARGET_OS_MAC
	/*!
		the type of a callback function for QuickTime moviecontroller actions calling the 'selector' of an
		Objective C class object "owning" the QTMovieWindowH
		@param	target	the class object, e.g. an NSDocument-descendant showing the movie
		@param	selector	the selector to call (e.g. moviePlay:withParams:)
		@param	wi	a QTMovieWindow handle
		@param	params	to be interpreted as a function of the action received; see the discussion of the MCActions structure
	 */
#	ifdef __OBJC__
		typedef int (*NSMCActionCallback)(id target, SEL selector, QTMovieWindowH wih, void *params);
#	else
		typedef int (*NSMCActionCallback)(void *target, void *selector, QTMovieWindowH wih, void *params);
#	endif
#endif

#ifdef __cplusplus
	extern "C"
	{
#endif
		/*!
			returns a pointer to the initialised MCAction structure
		 */
		QTLSext const MCActions *MCAction();
		/*!
			MovieController callback actions: functions receiving a QTMovieWindow handle
			and can return TRUE if the action should NOT be handled any further (i.e. cancelled),
			FALSE otherwise.
		 */
		QTLSext void register_MCAction( QTMovieWindowH wi, short action, MCActionCallback callback );
		QTLSext MCActionCallback get_MCAction( QTMovieWindowH wi, short action );
		QTLSext void unregister_MCAction( QTMovieWindowH wi, short action );
#	if TARGET_OS_MAC
			/*!
				 MovieController callback actions: functions receiving a QTMovieWindow handle
				 and can return TRUE if the action should NOT be handled any further (i.e. cancelled),
				 FALSE otherwise. These call into an Objective-C's selector
			 */
#		ifdef __OBJC__
			QTLSext void register_NSMCAction( id target, QTMovieWindowH wi, short action, SEL selector, NSMCActionCallback callback );
			QTLSext NSMCActionCallback get_NSMCAction( QTMovieWindowH wi, short action, id *target_return, SEL *selector_return );
#		else
			QTLSext void register_NSMCAction( void *target, QTMovieWindowH wi, short action, void *selector, NSMCActionCallback callback );
			QTLSext NSMCActionCallback get_NSMCAction( QTMovieWindowH wi, short action, void **target_return, void **selector_return );
#		endif
		QTLSext void unregister_NSMCAction( QTMovieWindowH wi, short action );
#	endif
#ifdef __cplusplus
	}
#endif

QTLSext void QTils_LogInit();
QTLSext void QTils_LogFinish();

/*!
	log a fixed-format message onto the QTils log .
 */
QTLSext size_t QTils_LogMsg( const char *msg );
/*!
	log a free-format message onto the QTils log .
 */
QTLSext size_t QTils_vLogMsgEx( const char *msg, va_list ap );
QTLSext size_t QTils_LogMsgEx( const char *msg, ... );

/*!
	a version of sprintf() that allocates the necessary buffer to hold the result, and
	returns the number of characters in the resulting buffer. Uses CFStringCreateWithFormatAndArguments
	to do the actual work and then (re)allocates a buffer that can be released with free() when done.
 */
QTLSext int ssprintf( char **buffer, const char *format, ... );
/*!
	Like ssprintf(), but appends the formated string to the existing buffer, expanding it as necessary.
	It returns the number of characters added.
 */
QTLSext int ssprintfAppend( char **buffer, const char *format, ... );

#pragma mark ----XML----

#if (!defined(__QUICKTIME__) && !defined(__MOVIES__)) || defined(__DOXYGEN__)
	typedef struct ComponentInstanceRecord* ComponentInstance;
	typedef struct RGBColor {
		unsigned short red;
		unsigned short green;
		unsigned short blue;
	} RGBColor;

	/*!
		from QuickTimeComponents.h, (c) Apple 1990-2006
	 */
	enum xmlParseComponentTypes {
		xmlParseComponentType		= 'pars',	//!< OpenDefaultComponent( xmlParseComponentType, xmlParseComponentSubType )
		xmlParseComponentSubType		= 'xml '	//!< OpenDefaultComponent( xmlParseComponentType, xmlParseComponentSubType )
	};

	enum xmlIdentifiersAndContentTypes {
		xmlIdentifierInvalid		= 0,
		xmlIdentifierUnrecognized	= (long)0xFFFFFFFF,
		xmlContentTypeInvalid		= 0,
		xmlContentTypeElement		= 1,
		xmlContentTypeCharData		= 2
	};

	enum xmlParsingFlags {
		elementFlagAlwaysSelfContained	= 1L << 0,		//!< Element doesn't have contents or closing tag even if it doesn't end with />, as in the HTML <img> tag
		elementFlagPreserveWhiteSpace		= 1L << 1,		//!< Preserve whitespace in content, default is to remove it
		xmlParseFlagAllowUppercase		= 1L << 0,		//!< Entities and attributes do not have to be lowercase (strict XML), but can be upper or mixed case as in HTML
		xmlParseFlagAllowUnquotedAttributeValues = 1L << 1,	//!< Attributes values do not have to be enclosed in quotes (strict XML), but can be left unquoted if they contain no spaces
		xmlParseFlagEventParseOnly		= 1L << 2,		//!< Do event parsing only
		xmlParseFlagPreserveWhiteSpace	= 1L << 3			//!< Preserve whitespace throughout the document
	};

	/*!
		the types of attributes supported. QTils adds support for a ValueKindDouble, but
		that is in fact simply a ValueKindCharString with a specific getter function
	 */
	enum xmlAttributeKinds {
		attributeValueKindCharString		= 0,
		attributeValueKindInteger		= 1L << 0, //!<    Number
		attributeValueKindPercent		= 1L << 1, //!<    Number or percent
		attributeValueKindBoolean		= 1L << 2, //!<    "true" or "false"
		attributeValueKindOnOff			= 1L << 3, //!<    "on" or "off"
		attributeValueKindColor			= 1L << 4, //!<    Either "#rrggbb" or a color name
		attributeValueKindEnum			= 1L << 5, //!<    one of a number of strings; the enum strings are passed as a zero-separated, double-zero-terminated C string in the attributeKindValueInfo param
		attributeValueKindCaseSensEnum	= 1L << 6, //!<    one of a number of strings; the enum strings are passed as for attributeValueKindEnum, but the values are case-sensitive
		MAX_ATTRIBUTE_VALUE_KIND			= attributeValueKindCaseSensEnum
	};

	enum {
		nameSpaceIDNone               = 0
	};

	/*!
		A Parsed XML attribute value, one of number/percent, boolean/on-off, color, or enumerated type
	 */
	union XMLAttributeValue {
		int				number;                 //!<    The value when valueKind is attributeValueKindInteger or attributeValueKindPercent
		unsigned char		boolean;                //!<    The value when valueKind is attributeValueKindBoolean or attributeValueKindOnOff
		RGBColor			color;                  //!<    The value when valueKind is attributeValueKindColor
		unsigned int		enumType;               //!<    The value when valueKind is attributeValueKindEnum
	};
	typedef union XMLAttributeValue         XMLAttributeValue;
	/*!
		An XML attribute-value pair
	 */
	struct XMLAttribute {
		unsigned int		identifier;             //!<    Tokenized identifier, if the attribute name was recognized by the parser
		char				*name;                  //!<    Attribute name, Only present if identifier == xmlIdentifierUnrecognized
		long				valueKind;              //!<    Type of parsed value, if the value was recognized and parsed; otherwise, attributeValueKindCharString
		XMLAttributeValue	value;                  //!<    Parsed attribute value
		char				*valueStr;              //!<    Always present
	};
	typedef struct XMLAttribute			XMLAttribute;
	typedef XMLAttribute				*XMLAttributePtr;
	/*!
		Forward struct declarations for recursively-defined tree structure
	 */
	typedef struct XMLContent			XMLContent;
	typedef XMLContent					*XMLContentPtr;

	/*!
		an XML element with its attributes and contents
	 */
	struct XMLElement {
		unsigned int		identifier;		//!<    Tokenized identifier, if the element name was recognized by the parser
		char				*name;			//!<    Element name, only present if identifier == xmlIdentifierUnrecognized
		XMLAttributePtr	attributes;		//!<    Array of attributes, terminated with an attribute with identifier == xmlIdentifierInvalid
		XMLContentPtr		contents;			//!<    Array of contents, terminated with a content with kind == xmlIdentifierInvalid
	};
	typedef struct XMLElement		XMLElement;
	typedef XMLElement				*XMLElementPtr;

	/*!
		 The content of an XML element is a series of parts, each of which may be either another element
		 or simply character data.
	 */
	union XMLElementContent {
		XMLElement		element;			//!<    The contents when the content kind is xmlContentTypeElement
		char				*charData;		//!<    The contents when the content kind is xmlContentTypeCharData
	};
	typedef union XMLElementContent	XMLElementContent;
	struct XMLContent {
		unsigned int		kind;
		XMLElementContent	actualContent;
	};

	/*!
		the internal representation of an XML document after import/parsing
	 */
	typedef struct XMLDocRecord {
		void			*xmlDataStorage;		//!<    opaque storage
		XMLElement	rootElement;			//!<    all elements with their attributes and content are to be found under the root element.
	} XMLDocRecord, *XMLDoc;

#endif
/*!
	NB NB NB: a "DoubleAttribute" is based on attributeValueKindCharString !!
 */
#ifndef	attributeValueKindDouble
#	define attributeValueKindDouble	attributeValueKindCharString
#endif
enum {
	attributeNotFound = -1
};

/*!
	 the 2 types of XML_Record
 */
typedef enum xml_Item_Types { xml_element, xml_attribute } xml_Item_Types;

/*!
	value kinds supported for the XML_Record type: the difference with
	QuickTime's xmlAttributeKinds is that attributeValueKindDouble != attributeValueKindCharString
	which is important for automatic parsing.
	The translation to xmlAttributeKinds is made by CreateXMLParser();
 */
enum xmlRecordAttributeTypes {
	recordAttributeValueTypeCharString		= 0,
	recordAttributeValueTypeInteger		= 1L << 0, //!<    Number
	recordAttributeValueTypePercent		= 1L << 1, //!<    Number or percent
	recordAttributeValueTypeBoolean		= 1L << 2, //!<    "true" or "false"
	recordAttributeValueTypeOnOff			= 1L << 3, //!<    "on" or "off"
	recordAttributeValueTypeDouble		= (1+MAX_ATTRIBUTE_VALUE_KIND), //!<    floating point number
	MAX_RECORD_ATTRIBUTE_VALUE_TYPE		= recordAttributeValueTypeDouble
};

typedef union XML_ParsedValue {
	char **stringValue;
	SInt32 *integerValue;
	double *doubleValue;
	UInt8 *booleanValue;
} XML_ParsedValue;

struct XML_Record;

/*!
	Callback to handle the occurrence of a specific element/attribute in XML file fName
	@param	theElement	the XML element being considered
	@param	idx			the index of the element's attribute under consideration
	@param	elm			the element's number (largely informational)
	@param	designTable	the design table describing the XML file
	@param	designEntry	the index into the design table that matches the current element/attribute pair
	@param	fName		the XML file name
 */
typedef ErrCode (*XMLAttributeParseCallback)(XMLElement *theElement, SInt32 idx, size_t elm, struct XML_Record *designTable,
									size_t designEntry, const char *fName);

/*!
	table entry defition to build an xmlParser using CreateXMLParser, and attempt to parse an imported
	XML file further into actual values, using ReadXMLContent.
 */
typedef struct XML_Record {
	xml_Item_Types itemType;
	union content {
		struct xml_element_content {
			char Tag[64];
			UInt32 ID, unused;
		} element;
		struct xml_attribute_content {
			char Tag[64];
			UInt32 ID, Type;
		} attribute;
	} xml;
	union localread {
		void *parsed;
		XMLAttributeParseCallback handler;
	} reading;
} XML_Record;

/*!
	converts an XML parsing error code into something (more) humanly comprehensible
 */
QTLSext ErrCode Check4XMLError( ComponentInstance xmlParser, ErrCode err, const char *theURL, Str255 descr );
/*!
	parses a file given an XML "encoding key" and imports the contents into an XMLDoc
	@param	theURL	filename
	@param	xmlParser	an XML parser descriptor, a sort of internal DTD
	@param	document	a pointer to an XMLDoc structure that will hold the imported content upon success
 */
QTLSext ErrCode ParseXMLFile( const char *theURL, ComponentInstance xmlParser, long flags, XMLDoc *document );
/*!
	add an element to an xmlParser
	@param	xmlParser	the parser being created
	@param	elementName	the tag (string) expected in the XML file
	@param	elementID	the integer identifier this element will have in the XMLDoc structure
	@param	namespaceID	a namespace identifier or NULL to use the default namespace
	@param	elementFlags	should be 0
 */
QTLSext ErrCode XMLParserAddElement( ComponentInstance *xmlParser, const char *elementName,
								unsigned int elementID, unsigned int *namespaceID, long elementFlags );
/*!
	add an attribute to the given element
	@param	xmlParser	the parser being created
	@param	elementID	the numerical identifier of the target element
	@param	namespaceID	the namespace identifier or NULL to use the default namespace
	@param	attrName	the tag (string) expected in the XML file
	@param	attrID	the numerical identifier this attribute will have in the XMLDoc structure
	@param	attrType	the attribute kind (attributeValueKindCharString, etc)
 */
QTLSext ErrCode XMLParserAddElementAttribute( ComponentInstance *xmlParser,
								unsigned int elementID, unsigned int *namespaceID,
								const char *attrName, unsigned int attrID, unsigned int attrType );

/*!
	create an xmlParser from an XML_Record table that contains the 'gist' of the thing.
 */
QTLSext ErrCode CreateXMLParser( ComponentInstance *xmlParser, XML_Record *xml_design_parser, unsigned int N, int *errors );

/*!
	Dispose the xmlParser and/or imported XMLDoc structure.
	@param	parserToo	if False, only the XMLDoc is disposed of, which allows reuse of the parser
 */
QTLSext ErrCode DisposeXMLParser( ComponentInstance *xmlParser, XMLDoc *xmldoc, int parserToo );

/*!
	attempts to parse the XML file fName based on the design table. The parsed values are stored at the locations registered
	in the design table.
 */
QTLSext void ReadXMLContent( const char *fName, XMLContent *theContent, XML_Record *design,
					   size_t designLength, size_t *elm );
///*!
//	attempts to parse the XML file fName based on the design table. The actual parsing is left to the handler(s)
//	associated with each attribute specification in the design table.
// */
//QTLSext void ReadXMLContentWithHandlers( const char *fName, XMLContent *theContent, XML_Record *design,
//					   size_t designLength, size_t *elm );

/*!
	retrieves a pointer to the given element number
 */
QTLSext XMLElement *XMLElementContents( XMLDoc xmldoc, UInt32 element );

QTLSext unsigned int XMLRootElementID( XMLDoc xmldoc );
QTLSext unsigned int XMLContentKind( XMLContent *theContent, unsigned short element );
QTLSext unsigned int XMLRootElementContentKind( XMLDoc xmldoc, unsigned short element );
QTLSext Boolean XMLContentOfElementOfRootElement( XMLDoc xmldoc, unsigned short element, XMLContent **theContent );
QTLSext Boolean XMLContentOfElement( XMLElement *parentElement, XMLContent **theElements );
QTLSext Boolean XMLElementOfContent( XMLContent *theContent, unsigned short element, XMLElement *theElement );

/*!
	given an array of attributes, return the index at the given attribute is found
 */
QTLSext ErrCode GetAttributeIndex( XMLAttributePtr attributes, UInt32 attributeID, SInt32 *idx );
/*!
	returns the string value of the given attribute of the given element
 */
QTLSext ErrCode GetStringAttribute( XMLElement *element, UInt32 attributeID, char **theString );
/*!
	returns the integer value of the given attribute of the given element
 */
QTLSext ErrCode GetIntegerAttribute( XMLElement *element, UInt32 attributeID, SInt32 *theNumber );
/*!
	returns the (16 bit) short value of the given attribute of the given element
 */
QTLSext ErrCode GetShortAttribute( XMLElement *element, UInt32 attributeID, SInt16 *theNumber );
/*!
	returns the double value of the given attribute of the given element
	the value in the file may have a decimal point or a decimal comma (French notation).
 */
QTLSext ErrCode GetDoubleAttribute( XMLElement *element, UInt32 attributeID, double *theNumber );
/*!
	returns the boolean value of the given attribute of the given element
	the value in the file can be True or False
 */
QTLSext ErrCode GetBooleanAttribute( XMLElement *element, UInt32 attributeID, UInt8 *theBool );

// _QTILITIES_H cannot be defined at the very end of this header, as I usually do...
#define _QTILITIES_H

#pragma mark ----LibQTilsBase----

union QTAPixel;
struct QTMSEncodingStats;

/*!
	Convenience facility for obtaining the basic function pointers when loading
	QTils.dll dynamically, via dlopen() or LoadLibrary().
	After obtaining a handle to the library, obtain the address of initDMBaseQTils() and
	then pass it a pointer to a LibQTilsBase structure. initDMBaseQTils() will copy the function addresses
	into the respective members of that structure, and returns the size of the LibQTilsBase structure.
	Used for interfacing from Modula-2.
	NB!! QTilsM2 (Modula-2) defines several additional members in its interface that are not initialised in C!
 */
typedef struct LibQTilsBase {
	// members that are hidden in the public Modula-2 interface:
	ErrCode (*OpenQT)();
	void (*CloseQT)();

	struct QTCompressionCodecs* (*QTCompressionCodec)();
	struct QTCompressionQualities* (*QTCompressionQuality)();

	const MCActions* (*MCAction)();
	int (*_QTMovieWindowH_Check_)( QTMovieWindowH wih );
	QTMovieWindowH (*OpenQTMovieInWindow)( const char *theURL, int withController );
	ErrCode (*CloseQTMovieWindow)( QTMovieWindowH WI );
	size_t (*QTils_LogMsgEx)( const char *msg, va_list ap );

	int (*vsscanf)( const char *source, int slen, const char *format, int flen, va_list ap );
	int (*vsnprintf)( char *dest, int slen, const char *format, int flen, va_list ap );
	int (*vssprintf)( char **buffer, const char *format, int flen, va_list ap );
	int (*vssprintfAppend)( char **buffer, const char *format, int flen, va_list ap );
	
	/////////////////////////////
	// public Modula-2 interface:
	void (*DisposeQTMovieWindow)( QTMovieWindowH WI );
	QTMovieWindowH (*QTMovieWindowHFromMovie)( Movie theMovie );

	ErrCode (*ActivateQTMovieWindow)( QTMovieWindowH wih );
	ErrCode (*QTMovieWindowToggleMCController)( QTMovieWindowH wih );
	ErrCode (*QTMovieWindowSetPlayAllFrames)( QTMovieWindowH WI, int onoff, int *curState );
	ErrCode (*QTMovieWindowSetPreferredRate)( QTMovieWindowH WI, int rate, int *curRate );
	ErrCode (*QTMovieWindowPlay)( QTMovieWindowH wih );
	ErrCode (*QTMovieWindowStop)( QTMovieWindowH wih );

	ErrCode (*QTMovieWindowGetTime)( QTMovieWindowH wih, double *t, int absolute );
	ErrCode (*QTMovieWindowGetFrameTime)( QTMovieWindowH wih, MovieFrameTime *ft, int absolute );
	ErrCode (*QTMovieWindowSetTime)( QTMovieWindowH wih, double t, int absolute );
	ErrCode (*QTMovieWindowSetFrameTime)( QTMovieWindowH wih, MovieFrameTime *ft, int absolute );
	ErrCode (*QTMovieWindowStepNext)( QTMovieWindowH wih, int steps );
	MovieFrameTime* (*secondsToFrameTime)( double Time, double MovieFrameRate, MovieFrameTime *ft );
	ErrCode (*QTMovieWindowSetGeometry)( QTMovieWindowH wih, Cartesian *pos, Cartesian *size, double sizeScale, int setEnvelope );
	ErrCode (*QTMovieWindowGetGeometry)( QTMovieWindowH wih, Cartesian *pos, Cartesian *size, int getEnvelope );

	void (*register_MCAction)( QTMovieWindowH wi, short action, MCActionCallback callback );
	MCActionCallback (*get_MCAction)( QTMovieWindowH wi, short action );
	void (*unregister_MCAction)( QTMovieWindowH wi, short action );

	void (*DisposeMemoryDataRef)(MemoryDataRef *memRef);
	ErrCode (*MemoryDataRefFromString)( const char *string, const char *virtURL, MemoryDataRef *memRef );
//	ErrCode (*MemoryDataRefFromStringPtr)( const char *string, size_t len, MemoryDataRef *memRef );
	ErrCode (*OpenMovieFromMemoryDataRef)( Movie *newMovie, MemoryDataRef *memRef, OSType contentType );
	QTMovieWindowH (*OpenQTMovieFromMemoryDataRefInWindow)( MemoryDataRef *memRef, OSType contentType, int controllerVisible );
	QTMovieWindowH (*OpenQTMovieWindowWithMovie)( Movie theMovie, char *theURL, int ulen, int visibleController );

#if !defined(__QUICKTIME__) && !defined(__MOVIES__)
	ErrCode (*OpenMovieFromURL)( Movie *newMovie, short flags, short *id, const char *URL, void *dum1, OSType *type );
#else
	ErrCode (*OpenMovieFromURL)( Movie *newMovie, short flags, short *id, const char *URL, Handle *dataRef, OSType *type );
#endif
	unsigned short (*HasMovieChanged)(Movie theMovie);
	ErrCode (*SaveMovie)( Movie theMovie );
	ErrCode (*SaveMovieAsRefMov)( const char *dstURL, Movie theMovie );
	ErrCode (*SaveMovieAs)( char **fname, Movie theMovie, int noDialog );
	ErrCode (*CloseMovie)(Movie *theMovie);
	void (*SetMoviePlayHints)( Movie theMovie, unsigned long hints, int exclusive );
	long (*GetMovieTrackCount)(Movie theMovie);
	double (*GetMovieDuration)(Movie theMovie);
	double (*GetMovieTimeResolution)(Movie theMovie);

	ErrCode (*AddMetaDataStringToTrack)( Movie theMovie, long theTrack, AnnotationKeys key,
							const char *value, int vlen, const char *lang, int llen );
	ErrCode (*AddMetaDataStringToMovie)( Movie theMovie, AnnotationKeys key,
							const char *value, int vlen, const char *lang, int llen );
	ErrCode (*GetMetaDataStringLengthFromTrack)( Movie theMovie, long theTrack, AnnotationKeys key,
							size_t *len );
	ErrCode (*GetMetaDataStringLengthFromMovie)( Movie theMovie, AnnotationKeys key,
							size_t *len );
	ErrCode (*GetMetaDataStringFromTrack)( Movie theMovie, long theTrack, AnnotationKeys key,
							char *value, int vlen, char *lang, int llen );
	ErrCode (*GetMetaDataStringFromMovie)( Movie theMovie, AnnotationKeys key,
							char *value, int vlen, char *lang, int llen );
	ErrCode (*FindTextInMovie)( Movie theMovie, char *text, int displayResult,
							Track *inoutTrack, double *foundTime, long *foundOffset, char **foundText );
	ErrCode (*FindTimeStampInMovieAtTime)( Movie theMovie, double Time, char **foundText, double *foundTime );
	long (*GetMovieChapterCount)( Movie theMovie );
	ErrCode (*GetMovieIndChapter)( Movie theMovie, long ind, double *time, char **text );
	ErrCode (*MovieAddChapter)( Movie theMovie, Track refTrack, const char *name,
							double time, double duration );
	ErrCode (*GetTrackName)( Movie theMovie, Track track, char **trackName );
	ErrCode (*GetTrackWithName)( Movie theMovie, char *trackName, OSType type, long flags, Track *track, long *trackNr );
	ErrCode (*EnableTrackNr)( Movie theMovie, long trackNr );
	ErrCode (*DisableTrackNr)( Movie theMovie, long trackNr );
	ErrCode (*GetMovieTrackNrTypes)( Movie theMovie, long trackNr, OSType *trackType, OSType *trackSubType );
	ErrCode (*GetMovieTrackNrDecompressorInfo)( Movie theMovie, long trackNr, OSType *trackSubType,
							   char **componentName, OSType *componentManufacturer );
	ErrCode (*SlaveMovieToMasterMovie)( Movie slave, Movie master );
	ErrCode (*SampleNumberAtMovieTime)( Movie theMovie, Track theTrack, double t, long *sampleNum );
	ErrCode (*NewTimedCallBackRegisterForMovie)( Movie movie, QTCallBack *callbackRegister, int allowAtInterrupt );
	ErrCode (*TimedCallBackRegisterFunctionInTime)( Movie movie, QTCallBack cbRegister,
											  double time, QTCallBackUPP function, long data, int allowAtInterrupt);
	ErrCode (*DisposeCallBackRegister)(QTCallBack cb);
	
	// QTXML functions:

	ErrCode (*Check4XMLError)( ComponentInstance xmlParser, ErrCode err, const char *theURL, Str255 descr );
	ErrCode (*ParseXMLFile)( const char *theURL, ComponentInstance xmlParser, long flags, XMLDoc *document );
	UInt32 XMLnameSpaceID;
	// NB: the Modula-2 proxy functions don't take the namespaceID argument but use XMLnameSpaceID !!
	ErrCode (*XMLParserAddElement)( ComponentInstance *xmlParser, const char *elementName, unsigned int elementID,
									unsigned int *namespaceID, long elementFlags );
	ErrCode (*XMLParserAddElementAttribute)( ComponentInstance *xmlParser, unsigned int elementID,
									unsigned int *namespaceID, const char *attrName,
									unsigned int attrID, unsigned int attrType );
	ErrCode (*DisposeXMLParser)( ComponentInstance *xmlParser, XMLDoc *xmldoc, int parserToo );
	ErrCode (*GetAttributeIndex)( XMLAttributePtr attributes, UInt32 attributeID, SInt32 *idx );
	ErrCode (*GetStringAttribute)( XMLElement *element, UInt32 attributeID, char **theString );
	ErrCode (*GetIntegerAttribute)( XMLElement *element, UInt32 attributeID, SInt32 *theNumber );
	ErrCode (*GetShortAttribute)( XMLElement *element, UInt32 attributeID, SInt16 *theNumber );
	ErrCode (*GetDoubleAttribute)( XMLElement *element, UInt32 attributeID, double *theNumber );
	ErrCode (*GetBooleanAttribute)( XMLElement *element, UInt32 attributeID, UInt8 *theBool );

	// QTMovieSinks functions:
	struct QTMovieSinks* (*open_QTMovieSink)( struct QTMovieSinks *qms, const char *theURL,
							unsigned short Width, unsigned short Height, int hasAlpha,
							unsigned char frameBuffers,
							unsigned long codec, unsigned long quality, int useICM,
							int openQT, ErrCode *errReturn );
	struct QTMovieSinks* (*open_QTMovieSinkWithData)( struct QTMovieSinks *qms, const char *theURL,
							union QTAPixel **imageFrame,
							unsigned short Width, unsigned short Height, int hasAlpha,
							unsigned char frameBuffers,
							unsigned long codec, unsigned long quality, int useICM,
							int openQT, ErrCode *errReturn );
	ErrCode (*close_QTMovieSink)( struct QTMovieSinks **qms, int addTCTrack, struct QTMSEncodingStats *stats, int closeQT );
	ErrCode (*QTMovieSink_AddFrame)( struct QTMovieSinks *qms, double frameDuration );
	ErrCode (*QTMovieSink_AddFrameWithTime)( struct QTMovieSinks *qms, double frameTime );
	union QTAPixel* (*QTMSFrameBuffer)( struct QTMovieSinks *qms, unsigned short frameBuffer );
	union QTAPixel* (*QTMSCurrentFrameBuffer)( struct QTMovieSinks *qms );
	union QTAPixel* (*QTMSPixelAddress)( struct QTMovieSinks *qms, unsigned short frameBuffer, unsigned int pixnr );
	union QTAPixel* (*QTMSPixelAddressInCurrentFrameBuffer)( struct QTMovieSinks *qms, unsigned int pixnr );
	Movie (*get_QTMovieSink_Movie)( struct QTMovieSinks *qms );
	int (*get_QTMovieSink_EncodingStats)( struct QTMovieSinks *qms, struct QTMSEncodingStats *stats );

	ErrCode (*LastQTError)();

	unsigned int (*PumpMessages)(int force);
	size_t (*QTils_LogMsg)(const char *msg);
	char *lastSSLogMsg;
	UInt32 (*MacErrorString) ( ErrCode err, char *errString, int slen,
						 char *errComment, int clen );
	void (*free)( char **mem );
	QTils_Allocators *QTils_Allocator;

	// NB!! Modula-2 defines several additional members in its interface that are not initialised in C!
} LibQTilsBase;

/*!
	initialises dmbase and returns the size of the structure
 */
QTLSext size_t initDMBaseQTils( LibQTilsBase *dmbase );

/*!
	the class for our movie windows, under MSWin
 */
#define	QTMOVIEWINDOW	"QTMovieWindow by RJVB"

#ifdef __cplusplus
}
#endif

#endif
