#include "copyright.h"
IDENTIFY("Unit tests for QTils (QTMovieWindows) and QTMovieSink");

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>

#ifndef _MSC_VER
#	include <unistd.h>
#else
#	include "winixdefs.h"
#endif

#include "QTilities.h"

#ifndef TRUE
#	define TRUE		1
#endif
#ifndef FALSE
#	define FALSE	0
#endif

enum xmlItems { element_vodDesign = 1,
	element_frequency = 2,
	element_frequence = 3,
	element_scale = 4,
	element_utc = 5,
	element_echelle = 6,
	element_channels = 7,
	element_canaux = 8,
	element_parsing = 9,
	element_lecture = 10,
	attr_freq = 1,
	attr_scale = 2,
	attr_forward = 4,
	attr_pilot = 6,
	attr_left = 8,
	attr_right = 10,
	attr_zone = 11,
	attr_dst = 12,
	attr_usevmgi = 13,
	attr_log = 14 };

typedef struct VODChannels {
	int forward, pilot, left, right;
} VODChannels;

typedef struct VODDescription {
	double frequency, scale, timeZone;
	Boolean DST, useVMGI, log;
	VODChannels channels;
} VODDescription;

VODDescription xmlVD;

XML_Record xml_design_parser[28] = {
		{xml_element, "vod.design", element_vodDesign},
		{xml_element, "frequency", element_frequency},
			{xml_attribute, "fps", attr_freq, recordAttributeValueTypeDouble, &xmlVD.frequency},
		{xml_element, "frequence", element_frequence},
			{xml_attribute, "tps", attr_freq, recordAttributeValueTypeDouble, &xmlVD.frequency},
		{xml_element, "utc", element_utc},
			{xml_attribute, "zone", attr_zone, recordAttributeValueTypeDouble, &xmlVD.timeZone},
			{xml_attribute, "dst", attr_dst, recordAttributeValueTypeBoolean, &xmlVD.DST},
		{xml_element, "scale", element_scale},
			{xml_attribute, "factor", attr_scale, recordAttributeValueTypeDouble, &xmlVD.scale},
		{xml_element, "echelle", element_echelle},
			{xml_attribute, "facteur", attr_scale, recordAttributeValueTypeDouble, &xmlVD.scale},
		{xml_element, "channels", element_channels},
			{xml_attribute, "forward", attr_forward, recordAttributeValueTypeInteger, &xmlVD.channels.forward},
			{xml_attribute, "pilot", attr_pilot, recordAttributeValueTypeInteger, &xmlVD.channels.pilot},
			{xml_attribute, "left", attr_left, recordAttributeValueTypeInteger, &xmlVD.channels.left},
			{xml_attribute, "right", attr_right, recordAttributeValueTypeInteger, &xmlVD.channels.right},
		{xml_element, "canaux", element_canaux},
			{xml_attribute, "avant", attr_forward, recordAttributeValueTypeInteger, &xmlVD.channels.forward},
			{xml_attribute, "pilote", attr_pilot, recordAttributeValueTypeInteger, &xmlVD.channels.pilot},
			{xml_attribute, "gauche", attr_left, recordAttributeValueTypeInteger, &xmlVD.channels.left},
			{xml_attribute, "droite", attr_right, recordAttributeValueTypeInteger, &xmlVD.channels.right},
		{xml_element, "parsing", element_parsing},
			{xml_attribute, "usevmgi", attr_usevmgi, recordAttributeValueTypeBoolean, &xmlVD.useVMGI},
			{xml_attribute, "log", attr_log, recordAttributeValueTypeBoolean, &xmlVD.log},
		{xml_element, "lecture", element_lecture},
			{xml_attribute, "avecvmgi", attr_usevmgi, recordAttributeValueTypeBoolean, &xmlVD.useVMGI},
			{xml_attribute, "journal", attr_log, recordAttributeValueTypeBoolean, &xmlVD.log}
};

const char *qi2mStringMask =
"<?xml version=\"1.0\"?>\n"
"<?quicktime type=\"video/x-qt-img2mov\"?>\n"
"<import autoSave=False askSave=False >\n"
"	<description txt=\"UTC timeZone=1, DST=0, assoc.data:*FromVODFile*\" />\n"
"	<sequence src=\"%s\" channel=-1 freq=-1 hidetc=False timepad=False hflip=False vmgi=True newchapter=False log=False />\n"
"</import>\n";

QTMovieWindowH *winlist = NULL;
int numQTMW = 0, MaxnumQTMW = 0;

#define xfree(x)	if((x)){ free((x)); (x)=NULL; }

int movieStep(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
	  double t, t2;
	  short steps = (short) params;
		if( QTMovieWindowGetTime(wi,&t,1) == noErr ){
			if( !steps ){
				fprintf( stderr, "movie #%d STEPPED to t=%gs (abs)\n", (*wi)->idx, t );
			}
			else{
#ifdef DEBUG
			  MovieFrameTime ft, ft2;
#endif
				t2 = t + ((double) steps/(*wi)->info->TCframeRate);
#ifdef DEBUG
				secondsToFrameTime( t, (*wi)->info->TCframeRate, &ft );
				secondsToFrameTime( t2, (*wi)->info->TCframeRate, &ft2 );
				fprintf( stderr, "Current #%d abs movieTime=%gs=%02d:%02d:%02d;%d; stepping to %gs=%02d:%02d:%02d;%d\n",
					(*wi)->idx,
					t, ft.hours, ft.minutes, ft.seconds, ft.frames,
					t2, ft2.hours, ft2.minutes, ft2.seconds, ft2.frames
				);
#else
				fprintf( stderr, "Current #%d abs movieTime=%gs; stepping to %gs\n", (*wi)->idx, t, t2 );
#endif
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
		if( QTMovieWindowGetTime(wi,&t,1) == noErr ){
			if( t2 ){
				fprintf( stderr, "Scanning movie #%d from %gs to %gs\n", (*wi)->idx, t,
					+ (*wi)->info->startTime + *t2
				);
			}
			else{
				fprintf( stderr, "Scanning movie #%d from %gs to ??s\n", (*wi)->idx, t );
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
		if( QTMovieWindowGetTime(wi,&t,1) == noErr ){
			if( (*wi)->wasScanned > 0 ){
				fprintf( stderr, "movie #%d scanned to t=%gs\n", (*wi)->idx, t );
			}
		}
	}
	return FALSE;
}

int movieStart(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
	  double t;
		if( QTMovieWindowGetTime(wi,&t,1) == noErr ){
			fprintf( stderr, "movie #%d starting to play at t=%gs\n", (*wi)->idx, t );
		}
	}
	return FALSE;
}

int movieStop(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
	  double t;
		if( QTMovieWindowGetTime(wi,&t,1) == noErr ){
			fprintf( stderr, "movie #%d stops playing at t=%gs\n", (*wi)->idx, t );
		}
	}
	return FALSE;
}

int movieFinished(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
	  double tr,ta,tt;
	  MovieFrameTime ft, ft2, ft3;
		QTMovieWindowStop( wi );

		QTMovieWindowGetTime(wi,&tr, 0);
		QTMovieWindowGetTime(wi,&ta, 1);
		QTMovieWindowGetFrameTime(wi, &ft, 1);
		secondsToFrameTime( ta, (*wi)->info->TCframeRate, &ft2 );
		fprintf( stderr, "Movie #%d finished at t %gs(rel) %gs=%02d:%02d:%02d;%d(abs;%gHz) - effective duration=%gs(abs) theoretical %gs\n",
			(*wi)->idx, tr,
			ta, ft.hours, ft.minutes, ft.seconds, ft.frames, (*wi)->info->TCframeRate,
			ta - (*wi)->info->startTime, (*wi)->info->duration
		);
		secondsToFrameTime( ta, (*wi)->info->frameRate, &ft3 );
		fprintf( stderr, "t (abs) %gs=%02d:%02d:%02d;%d (freq=%gHz)\n",
			ta, ft3.hours, ft3.minutes, ft3.seconds, ft3.frames, (*wi)->info->frameRate
		);

		secondsToFrameTime( (*wi)->info->startTime + (*wi)->info->duration, (*wi)->info->TCframeRate, &ft );
		QTMovieWindowSetFrameTime( wi, &ft, 1 );

		QTMovieWindowSetTime( wi, (tt=(*wi)->info->startTime + (*wi)->info->duration/2), 1 );
		QTMovieWindowGetTime(wi,&tr, 0);
		QTMovieWindowGetTime(wi,&ta, 1);

		secondsToFrameTime( tt, (*wi)->info->TCframeRate, &ft );
		secondsToFrameTime( ta, (*wi)->info->TCframeRate, &ft2 );
		fprintf( stderr, "Sent movie to %gs from start: %gs=%02d:%02d:%02d;%d, current time now t=%gs (rel) t=%gs=%02d:%02d:%02d;%d (abs)\n",
			(*wi)->info->duration/2, tt,
			ft.hours, ft.minutes, ft.seconds, ft.frames,
			tr, ta,
			ft2.hours, ft2.minutes, ft2.seconds, ft2.frames
		);
		secondsToFrameTime( ta, (*wi)->info->frameRate, &ft3 );
		fprintf( stderr, "t (abs) %gs=%02d:%02d:%02d;%d (freq=%gHz)\n",
			ta, ft3.hours, ft3.minutes, ft3.seconds, ft3.frames, (*wi)->info->frameRate
		);
	}
	return FALSE;
}

int movieClose(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) && (*wi)->idx >= 0 && (*wi)->idx < MaxnumQTMW ){
		numQTMW -= 1;
		fprintf( stderr, "Closing movie #%d; %d remaining\n", (*wi)->idx, numQTMW );
		// we can dispose of the window here, or leave it to the library.
		// If we do it here the disposing will probably be incomplete to avoid 'dangling events',
		// (esp. on Mac OS X)
		// so we cannot remove the wi from our local list via
		// winlist[ (*wi)->idx ] = NULL;
		DisposeQTMovieWindow(wi);
		return TRUE;
	}
	return FALSE;
}

void CloseMovies(int final)
{ int i;
	for( i = 0 ; i < MaxnumQTMW ; i++ ){
		if( final ){
			if( winlist[i] ){
				DisposeQTMovieWindow( winlist[i] );
				winlist[i] = NULL;
				numQTMW -= 1;
			}
		}
		else{
			if( QTMovieWindowH_Check(winlist[i]) ){
				CloseQTMovieWindow( winlist[i] );
				numQTMW -= 1;
			}
		}
	}
}

int quitRequest = FALSE;
int movieKeyUp(QTMovieWindowH wi, void *params )
{ EventRecord *evt = (EventRecord*) params;
	if( evt ){
		if( evt->message == 'q' || evt->message == 'Q' ){
			quitRequest = TRUE;
			return TRUE;
		}
		else if( evt->message == 'C' ){
			fprintf( stderr, "Toggling MC visibility\n" );
			QTMovieWindowToggleMCController(wi);
		}
		else if( evt->message == '1' ){
		  Cartesian size;
			QTMovieWindowGetGeometry( wi, NULL, &size, 0 );
			size.horizontal = (*wi)->info->naturalBounds.right;
			size.vertical = (*wi)->info->naturalBounds.bottom + (*wi)->info->controllerHeight;
			QTMovieWindowSetGeometry( wi, NULL, &size, 1.0, 0 );
			QTMovieWindowGetGeometry( wi, NULL, &size, 0 );
		}
	}
	return FALSE;
}

void doSigExit(int sig)
{
	fprintf( stderr, "Exit on signal #%d\n", sig );
	CloseMovies(TRUE);
	CloseQT();
	exit(sig);
}

void register_wi( QTMovieWindowH wi )
{
	if( wi ){
		if( (*wi)->idx != numQTMW ){
		 // the idx field is purely informational, we can change it at leisure:
			(*wi)->idx = numQTMW;
		}
		winlist[numQTMW] = wi;
		register_MCAction( wi, MCAction()->Step, movieStep );
		register_MCAction( wi, MCAction()->GoToTime, movieScan );
		register_MCAction( wi, MCAction()->Play, moviePlay );
		register_MCAction( wi, MCAction()->Start, movieStart );
		register_MCAction( wi, MCAction()->Stop, movieStop );
		register_MCAction( wi, MCAction()->Finished, movieFinished );
		register_MCAction( wi, MCAction()->Close, movieClose );
		register_MCAction( wi, MCAction()->KeyUp, movieKeyUp );
		numQTMW += 1;
		if( numQTMW > MaxnumQTMW ){
			MaxnumQTMW = numQTMW;
		}
	}
}

int vsprintfM2( char *dest, int dlen, const char *format, int flen, ... )
{ va_list ap;
  int n;
  extern int vsnprintf_Mod2( char *dest, int dlen, const char *format, int flen, va_list ap );
	va_start( ap, flen );
	if( !flen ){
		flen = strlen(format);
	}
	n = vsnprintf_Mod2( dest, dlen, format, flen, ap );
	va_end(ap);
	return n;
}

int vsscanfM2( char *src, int slen, const char *format, int flen, ... )
{ va_list ap;
  int n;
  extern int vsscanf_Mod2( const char *source, int slen, const char *format, int flen, va_list ap );
	va_start( ap, flen );
	if( !flen ){
		flen = strlen(format);
	}
	n = vsscanf_Mod2( src, slen, format, flen, ap );
	va_end(ap);
	return n;
}

#define ClipInt(x,min,max)	if((x)<(min)){ (x)=(min); } else if((x)>(max)){ (x)=(max); }

void ReadXMLDoc( char *fName, XMLDoc xmldoc, VODDescription *descr )
{ XMLContent *theContent;
  unsigned short elm;
	elm = 0;
	while( XMLRootElementContentKind( xmldoc, elm ) != xmlContentTypeInvalid ){
		if( XMLContentOfElementOfRootElement( xmldoc, elm, &theContent ) ){
			ReadXMLContent( fName, theContent, xml_design_parser,
						sizeof(xml_design_parser)/sizeof(xml_design_parser[0]), &elm );
			*descr = xmlVD;
		}
	}
}

int main( int argc, char* argv[] )
{ int i, n;
  QTMovieWindowH wi;
  unsigned long nMsg = 0, nPumps = 0;
  OSType otype;
  char *ostr;
  LibQTilsBase QTils;
  Track searchTrack;
  double searchTime;
  long searchOffset;
  char *foundText = NULL;
  char *qi2mString = NULL;

	otype = 'TVOD';
	ostr = OSTStr(otype);

	wi = (QTMovieWindowH) 0xfdfdfdfd;
	if( QTMovieWindowH_Check(wi) ){
		fprintf( stderr, "QTMovieWindowH_Check() accepted an invalid handle\n" );
		exit(-1);
	}
	{ double t = 3.1415;
	  int n, nTracks = -1;
	  char buf[256];
		n = vsprintfM2( buf, sizeof(buf), "double=%g\\nhex=0x%lx", 0, t, nTracks );
		n = vsscanfM2( buf, sizeof(buf), "double=%lf\\nhex=0x%lx", 0, &t, &nTracks );
		fprintf( stderr, "double=%g hex=0x%lx\n", t, nTracks );
	}
	if( argc > 0 ){
		// we need at least 2 windows
		n = (argc == 1)? 2 : argc;
		winlist = (QTMovieWindowH*) calloc( n, sizeof(QTMovieWindowH*) );
		if( !winlist ){
			perror( "Error allocating windowlist" );
			return -1;
		}
		OpenQT();
		initDMBaseQTils( &QTils );
		{ ComponentInstance xmlParser = NULL;
		  XMLDoc xmldoc = NULL;
		  enum xmlElements { element_root=1, element_el1=2, element_el2 };
		  enum xmlAttributes { attr_atint=1, attr_atbool, attr_atstring, attr_atdouble, attr_atshort };
		  ErrCode xmlErr;
		  SInt32 idx, element_id;
		  XMLContentPtr root_content, element_content;
		  XMLElement *theElement;
		  XMLAttributePtr element_attrs;
		  struct {
				SInt32 atint;
				UInt8 atbool;
				struct{
					char *atstring;
					double atdouble;
				} el1;
				struct{
					SInt16 atshort;
					SInt32 atint;
				} el2;
		  } data;
			xmlErr = XMLParserAddElement( &xmlParser, "root", element_root, NULL, 0 );
			xmlErr = XMLParserAddElementAttribute( &xmlParser, element_root, NULL, "atint", attr_atint, attributeValueKindInteger );
			xmlErr = XMLParserAddElementAttribute( &xmlParser, element_root, NULL, "atbool", attr_atbool, attributeValueKindBoolean );
			xmlErr = XMLParserAddElement( &xmlParser, "el1", element_el1, NULL, 0 );
			xmlErr = XMLParserAddElementAttribute( &xmlParser, element_el1, NULL, "atstring", attr_atstring, attributeValueKindCharString );
			xmlErr = XMLParserAddElementAttribute( &xmlParser, element_el1, NULL, "atdouble", attr_atdouble, attributeValueKindDouble );
			xmlErr = XMLParserAddElement( &xmlParser, "el2", element_el2, NULL, 0 );
			xmlErr = XMLParserAddElementAttribute( &xmlParser, element_el2, NULL, "atshort", attr_atshort, attributeValueKindInteger );
			xmlErr = XMLParserAddElementAttribute( &xmlParser, element_el2, NULL, "atint", attr_atint, attributeValueKindInteger );
			xmlErr = ParseXMLFile( "test.xml", xmlParser,
					   xmlParseFlagAllowUppercase|xmlParseFlagAllowUnquotedAttributeValues|elementFlagPreserveWhiteSpace,
					   &xmldoc
			);
			memset( &data, 0, sizeof(data) );
			if( xmlErr == noErr && xmldoc->rootElement.identifier == element_root ){
				xmlErr = GetAttributeIndex( xmldoc->rootElement.attributes, attr_atint, &idx );
				xmlErr = QTils.GetAttributeIndex( xmldoc->rootElement.attributes, attr_atbool, &idx );
				xmlErr = GetAttributeIndex( xmldoc->rootElement.attributes, attr_atdouble, &idx );
				xmlErr = GetIntegerAttribute( &xmldoc->rootElement, attr_atint, &data.atint );
				xmlErr = QTils.GetBooleanAttribute( &xmldoc->rootElement, attr_atbool, &data.atbool );
				idx = 0;
				root_content = xmldoc->rootElement.contents;
				while( root_content[idx].kind != xmlContentTypeInvalid ){
					if( root_content[idx].kind == xmlContentTypeElement ){
						element_content = xmldoc->rootElement.contents;
						theElement = &element_content[idx].actualContent.element;
						element_id = theElement->identifier;
						element_attrs = theElement->attributes;
						switch( element_id ){
							case element_el2:
								xmlErr = QTils.GetIntegerAttribute( theElement, attr_atint, &data.el2.atint );
								xmlErr = GetShortAttribute( theElement, attr_atshort, &data.el2.atshort );
								break;
							case element_el1:
								xmlErr = GetDoubleAttribute( theElement, attr_atdouble, &data.el1.atdouble );
								xmlErr = QTils.GetStringAttribute( theElement, attr_atstring, &data.el1.atstring );
								break;
						}
					}
					idx++;
				}
			}
			if( data.el1.atstring ){
				free(data.el1.atstring);
			}
			xmlErr = DisposeXMLParser( &xmlParser, &xmldoc, 1 );
			{ int errors = 0;
				xmlErr = CreateXMLParser( &xmlParser,
							xml_design_parser, sizeof(xml_design_parser)/sizeof(XML_Record),
							&errors
				);
				if( errors == 0 ){
					xmlErr = ParseXMLFile( "VODdesign.xml", xmlParser,
							  xmlParseFlagAllowUppercase|xmlParseFlagAllowUnquotedAttributeValues|elementFlagPreserveWhiteSpace,
							  &xmldoc );
					if( xmlErr == noErr ){
						if( XMLRootElementID(xmldoc) == element_vodDesign ){
						  VODDescription descr;
							QTils_LogMsgEx( "Reading VOD parameters from '%s'", "VODdesign.xml" );
							ReadXMLDoc( "VODdesign.xml", xmldoc, &descr );
						}
						else{
							QTils_LogMsgEx( "'%s' is valid XML but lacking root element '%s'",
										"VODdesign.xml", xml_design_parser[0].xml.element.Tag
							);
						}
					}
				}
				xmlErr = DisposeXMLParser( &xmlParser, NULL, 1 );
			}
		}
		if( argc == 1 ){
			// OpeQTMovieInWindow() will present a dialog if a NULL URL is passed in
			wi = OpenQTMovieInWindow( NULL, 1 );
			// register the window in our local list:
			register_wi(wi);
		}
		else{
#if defined(DEBUG) && defined(_MSC_VER)
			{ Movie theMovie;
			  ErrCode err;
				if( (err = OpenMovieFromURL( &theMovie, 1, NULL, NULL, NULL, NULL )) == noErr ){
					CloseMovie(&theMovie);
				}
			}
#endif
			for( i = 1 ; i < argc ; i++ ){
				wi = OpenQTMovieInWindow( argv[i], (i> 0 && i==argc-1)? 0 : 1 );
				// register the window in our local list:
				register_wi(wi);
				if( wi ){
				  size_t qlen = strlen(qi2mStringMask) + strlen(argv[i]) + 1;
					if( (qi2mString = calloc( qlen, sizeof(char) )) ){
					  MemoryDataRef memRef;
					  ErrCode err;
					  Movie theMovie;
						snprintf( qi2mString, qlen, qi2mStringMask, argv[i] );
						err = MemoryDataRefFromString( qi2mString, qlen, &memRef );
						QTils_LogMsgEx( "Importing movie from (qi2m) dataRef %p\n", memRef.dataRef );
//						err = OpenMovieFromMemoryDataRef( &theMovie, &memRef, 'QI2M' );
//						QTils_LogMsgEx( "Imported movie with code %d\n", err );
						wi = OpenQTMovieFromMemoryDataRefInWindow( &memRef, 'QI2M', 1 );
						QTils_LogMsgEx( "Imported movie with wi=%p, code %d\n", wi, LastQTError() );
						if( wi ){
//							CloseMovie(&theMovie);
							CloseQTMovieWindow(wi);
						}
						free(qi2mString);
						DisposeMemoryDataRef(&memRef);
					}
				}
			}
		}
		signal( SIGABRT, doSigExit );
		signal( SIGINT, doSigExit );
		signal( SIGTERM, doSigExit );

		if( numQTMW ){
		  Movie theMovie;
		  ErrCode err;
#if !(defined(DEBUG) && defined(_MSC_VER))
			if( (err = OpenMovieFromURL( &theMovie, 1, NULL, NULL, NULL, NULL )) == noErr ){
				CloseMovie(&theMovie);
			}
#endif
			if( (err = OpenMovieFromURL( &theMovie, 1, NULL, (*(winlist[0]))->theURL, NULL, NULL )) == noErr ){
			  char *dst = "c:/TEMP/kk.mov", *odst;
				err = AddMetaDataStringToMovie( theMovie, akComment, xg_id_string_stub(), NULL );
				fprintf( stderr, "Saving %s... ", argv[1] ); fflush(stderr);
				if( (err = SaveMovie( theMovie )) == noErr ){
					fprintf( stderr, "done\n" );
				}
				else{
					fprintf( stderr, "error=%d\n", err );
				}
				fprintf( stderr, "Saving %s to reference movie %s... ", argv[1], dst ); fflush(stderr);
				if( (err = SaveMovieAsRefMov( dst, theMovie )) == noErr ){
					fprintf( stderr, "done\n" );
					if( (wi = OpenQTMovieInWindow( dst, 0 )) ){
						register_wi(wi);
					}
				}
				else{
					fprintf( stderr, "error=%d\n", err );
				}
				odst = dst;
				SaveMovieAs( &dst, theMovie, FALSE );
				if( dst != odst ){
					free(dst);
				}
				err = CloseMovie(&theMovie);
			}
		}

		for( i = 0 ; i < numQTMW ; i++ ){
			// play the movie
			QTMovieWindowPlay( winlist[i] );
		}

		if( numQTMW > 1 ){
		  Cartesian pos, size;
			i -= 1;
			QTMovieWindowGetGeometry( winlist[i], &pos, &size, 1 );
			pos.horizontal = pos.vertical = 10;
			// set position and scale of the content region:
			QTMovieWindowSetGeometry( winlist[i], &pos, NULL, 0.5, 0 );
			// get the resultant details for the envelope:
			QTMovieWindowGetGeometry( winlist[i], &pos, &size, 1 );
			if( i && winlist[i-1] ){
				pos.horizontal += size.horizontal;
				pos.vertical += size.vertical;
				QTMovieWindowSetGeometry( winlist[i-1], &pos, NULL, 1.0, 1 );
			}

			QTMovieWindowStop(winlist[1]);
			QTMovieWindowToggleMCController(winlist[1]);
			{ long i, N = GetMovieChapterCount((*(winlist[1]))->theMovie);
			  double cTime;
			  char *cTitle;
				for( i = 0 ; i < N ; i++ ){
					if( GetMovieIndChapter( (*(winlist[1]))->theMovie, i, &cTime, &cTitle ) == noErr ){
						fprintf( stderr, "Title #%ld @ %gs = '%s'\n", i, cTime, cTitle );
						free(cTitle);
					}
				}
			}
			searchTrack = NULL; searchTime = searchOffset = 0; xfree(foundText);
			if( FindTextInMovie( (*(winlist[1]))->theMovie, "bla", 1, &searchTrack, &searchTime, &searchOffset, &foundText ) == noErr ){
				ActivateQTMovieWindow(winlist[1]);
				fprintf( stderr, "Found 'bla' in track %p at time %g, offset in movie text string %ld\n'%s'\n",
					   searchTrack, searchTime, searchOffset,
					   (foundText)? foundText : "<error retrieving found text>"
				);
				sleep(1);
			}
			searchTrack = NULL; searchTime = searchOffset = 0; xfree(foundText);
			if( FindTextInMovie( (*(winlist[1]))->theMovie, "tertest", 1, &searchTrack, &searchTime, &searchOffset, &foundText ) == noErr ){
				ActivateQTMovieWindow(winlist[1]);
				fprintf( stderr, "Found 'tertest' in track %p at time %g, offset in movie text string %ld\n'%s'\n",
					   searchTrack, searchTime, searchOffset,
					   (foundText)? foundText : "<error retrieving found text>"
				);
				sleep(1);
			}
			searchTrack = NULL; searchTime = searchOffset = 0; xfree(foundText);
			if( FindTextInMovie( (*(winlist[1]))->theMovie, "half", 1, &searchTrack, &searchTime, &searchOffset, &foundText ) == noErr ){
				ActivateQTMovieWindow(winlist[1]);
				fprintf( stderr, "Found 'half' in track %p at time %g, offset in movie text string %ld\n'%s'\n",
					   searchTrack, searchTime, searchOffset,
					   (foundText)? foundText : "<error retrieving found text>"
				);
				MovieAddChapter( (*(winlist[1]))->theMovie, NULL, "a new chapter entry", searchTime, 0.0 );
				sleep(1);
			}
			searchTrack = NULL; searchTime = searchOffset = 0; xfree(foundText);
			if( FindTextInMovie( (*(winlist[1]))->theMovie, "end", 1, &searchTrack, &searchTime, &searchOffset, &foundText ) == noErr ){
				ActivateQTMovieWindow(winlist[1]);
				fprintf( stderr, "Found 'end' in track %p at time %g, offset in movie text string %ld\n'%s'\n",
					   searchTrack, searchTime, searchOffset,
					   (foundText)? foundText : "<error retrieving found text>"
				);
				sleep(1);
			}
			searchTrack = NULL; searchTime = searchOffset = 0; xfree(foundText);
			if( FindTextInMovie( (*(winlist[1]))->theMovie, "09:38:17;06", 1, &searchTrack, &searchTime, &searchOffset, &foundText ) == noErr ){
				ActivateQTMovieWindow(winlist[1]);
				fprintf( stderr, "Found '09:38:17;06' in track %p at time %g, offset in movie text string %ld\n'%s'\n",
					   searchTrack, searchTime, searchOffset,
					   (foundText)? foundText : "<error retrieving found text>"
				);
				sleep(1);
			}
			searchTrack = NULL; searchTime = searchOffset = 0; xfree(foundText);
			if( FindTextInMovie( (*(winlist[1]))->theMovie, "substr", 1, &searchTrack, &searchTime, &searchOffset, &foundText ) == noErr ){
			  double dt = GetMovieTimeResolution((*(winlist[1]))->theMovie);
				ActivateQTMovieWindow(winlist[1]);
				fprintf( stderr, "Found 'substr' in track %p at time %g, offset in movie text string %ld\n'%s'\n",
					   searchTrack, searchTime, searchOffset,
					   (foundText)? foundText : "<error retrieving found text>"
				);
				searchTime += dt;
				sleep(1);
				// MovieSearchText appears to ignore the current setting for searchTime
			}
			xfree(foundText);
			if( FindTimeStampInMovieAtTime( (*(winlist[1]))->theMovie, 0, &foundText, &searchTime ) == noErr ){
				fprintf( stderr, "Found text '%s' in timeStamp track at specified t=0, returned time t=%g\n",
					   (foundText)? foundText : "<null>", searchTime
				);
				xfree(foundText);
			}
			if( MovieAddChapter( (*(winlist[1]))->theMovie, NULL, "another new chapter entry",
						 (*(winlist[1]))->info->duration/3.0, 0.0 ) == noErr
			){
//				SaveMovieAsRefMov( NULL, (*(winlist[1]))->theMovie );
				SaveMovie( (*(winlist[1]))->theMovie );
			}
			{ long i, N = GetMovieChapterCount((*(winlist[1]))->theMovie);
			  double cTime;
			  char *cTitle;
				for( i = 0 ; i < N ; i++ ){
					if( GetMovieIndChapter( (*(winlist[1]))->theMovie, i, &cTime, &cTitle ) == noErr ){
						fprintf( stderr, "Title #%ld @ %gs = '%s'\n", i, cTime, cTitle );
						free(cTitle);
					}
				}
			}
		}

		while( numQTMW && !quitRequest ){
			nMsg += PumpMessages(1);
			nPumps += 1;
		}

		for( i = 0 ; i < MaxnumQTMW ; i++ ){
			DisposeQTMovieWindow( winlist[i] );
			winlist[i] = NULL;
		}

		free(winlist);

		fprintf( stderr, "Handled %lu messages in %lu pumpcycles\n", nMsg, nPumps );

		CloseQT();
	}
	return 0;
}

