//
//  QTVOD.m
//  QTVODosx
//
//  Created by René J.V. Bertin on 20110906.
//  Copyright 2011 INRETS/LCPC — LEPSIS. All rights reserved.
//

#import "QTVOD.h"

#import <stdio.h>
#import <stdlib.h>
#import <stddef.h>
#import <libgen.h>
#import <string.h>
#import <math.h>

extern BOOL doLogging;

static void NSnoLog( NSString *format, ... )
{
	return;
}

static void doNSLog( NSString *format, ... )
{ va_list ap;
	va_start(ap, format);
	NSLogv( format, ap );
	va_end(ap);
	return;
}

#ifndef DEBUG
extern int QTils_Log(const char *fileName, int lineNr, NSString *format, ... );
#define NSLog(f,...)	QTils_Log(__FILE__, __LINE__, f, ##__VA_ARGS__)
#endif

@implementation NSString (hasSuffix)
- (BOOL) hasSuffix:(NSString*)aString caseSensitive:(BOOL)cs
{
	if( cs ){
	  NSRange range;
		range = [self rangeOfString:aString options:(NSAnchoredSearch|NSCaseInsensitiveSearch|NSBackwardsSearch)];
		return (range.location != NSNotFound);
	}
	else{
		return [self hasSuffix:aString];
	}
}
@end

extern char lastSSLogMsg[];

#pragma mark ---- QTVODlib functions

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
	attr_flLR = 13,
	attr_usevmgi = 14,
	attr_log = 15 };

VODDescription xmlVD;

@interface NSVODDescription : NSObject {
@public
	VODDescription *vodDescription;
}
+ (NSVODDescription*) createWithDescription:(VODDescription*) descr;
@property VODDescription *vodDescription;
@end

@implementation NSVODDescription

//- (void) dealloc
//{
//	[super dealloc];
//	fprintf( stderr, "[NSVODDescription dealloc(0x%p)]\n", self ); fflush(stderr);
//}

+ (NSVODDescription*) createWithDescription:(VODDescription*) descr
{
	self = [[self alloc] init];
	if( self ){
	  NSVODDescription *vd = (NSVODDescription*) self;
		vd->vodDescription = descr;
	}
	return [self autorelease];
}
@synthesize vodDescription;
@end

NSVODDescription *nsXMLVD = NULL;

XML_Record xml_design_parser[30] = {
		{xml_element, "vod.design", element_vodDesign},
		{xml_element, "frequency", element_frequency},
			{xml_attribute, "fps", attr_freq, recordAttributeValueTypeDouble, &xmlVD.frequency},
		{xml_element, "frequence", element_frequence},
			{xml_attribute, "tps", attr_freq, recordAttributeValueTypeDouble, &xmlVD.frequency},
		{xml_element, "utc", element_utc},
			{xml_attribute, "zone", attr_zone, recordAttributeValueTypeDouble, &xmlVD.timeZone},
			{xml_attribute, "dst", attr_dst, recordAttributeValueTypeBoolean, &xmlVD.DST},
			{xml_attribute, "flipleftright", attr_flLR, recordAttributeValueTypeBoolean, &xmlVD.flipLeftRight},
			{xml_attribute, "flipgauchedroite", attr_flLR, recordAttributeValueTypeBoolean, &xmlVD.flipLeftRight},
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

static BOOL recreateChannelViews;

#define ClipInt(x,min,max)	if((x)<(min)){ (x)=(min); } else if((x)>(max)){ (x)=(max); }

#pragma mark ---- XML prefs parsing

#if 0
ErrCode qtvodReadXMLElementAttributes( XMLElement *theElement, VODDescription *descr, const char *fName )
{ unsigned short idx;
  ErrCode xmlErr = noErr;
  UInt8 bval;
	switch( theElement->identifier ){
		case element_frequency:
		case element_frequence:
			xmlErr = GetDoubleAttribute( theElement, attr_freq, &descr->frequency );
			if( xmlErr != attributeNotFound ){
				QTils_LogMsgEx( "attr #%d freq=%g (%d)", (int) attr_freq, descr->frequency, xmlErr );
			}
			break;
		case element_utc:
			xmlErr = GetDoubleAttribute( theElement, attr_zone, &descr->timeZone );
			if( xmlErr != attributeNotFound ){
				QTils_LogMsgEx( "attr #%d timeZone=%g (%d)", attr_zone, descr->timeZone, xmlErr );
			}
			xmlErr = GetBooleanAttribute( theElement, attr_dst, &bval );
			if( xmlErr != attributeNotFound ){
				descr->DST = (bval != 0);
				QTils_LogMsgEx( "attr #%d DST=%hd (%d)", attr_dst, (short) bval, xmlErr );
			}
			xmlErr = GetBooleanAttribute( theElement, attr_flLR, &bval );
			if( xmlErr != attributeNotFound ){
				descr->flipLeftRight = (bval != 0);
				QTils_LogMsgEx( "attr #%d flipLeftRight=%hd (%d)", attr_flLR, (short) bval, xmlErr );
			}
			break;
		case element_parsing:
		case element_lecture:
			xmlErr = GetBooleanAttribute( theElement, attr_usevmgi, &bval );
			if( xmlErr != attributeNotFound ){
				descr->useVMGI = (bval != 0);
				QTils_LogMsgEx( "attr #%d useVMGI=%hd (%d)", attr_usevmgi, (short) bval, xmlErr );
			}
			break;
		case element_scale:
		case element_echelle:
			xmlErr = GetDoubleAttribute( theElement, attr_scale, &descr->scale );
			if( xmlErr != attributeNotFound ){
				QTils_LogMsgEx( "attr #%d scale=%g (%d)", (int) attr_scale, descr->scale, xmlErr );
			}
			break;
		case element_channels:
		case element_canaux:
			xmlErr = GetIntegerAttribute( theElement, attr_forward, (SInt32*) &descr->channels.forward );
			if( xmlErr != attributeNotFound ){
				ClipInt( descr->channels.forward, 1, 4 );
				QTils_LogMsgEx( "attr #%d chForward=%d (%d)", attr_forward, descr->channels.forward, xmlErr );
			}
			xmlErr = GetIntegerAttribute( theElement, attr_pilot, (SInt32*) &descr->channels.pilot );
			if( xmlErr != attributeNotFound ){
				ClipInt( descr->channels.pilot, 1, 4 );
				QTils_LogMsgEx( "attr #%d chPilot=%d (%d)", attr_pilot, descr->channels.pilot, xmlErr );
			}
			xmlErr = GetIntegerAttribute( theElement, attr_left, (SInt32*) &descr->channels.left );
			if( xmlErr != attributeNotFound ){
				ClipInt( descr->channels.left, 1, 4 );
				QTils_LogMsgEx( "attr #%d chLeft=%d (%d)", attr_left, descr->channels.left, xmlErr );
			}
			xmlErr = GetIntegerAttribute( theElement, attr_right, (SInt32*) &descr->channels.right );
			if( xmlErr != attributeNotFound ){
				ClipInt( descr->channels.right, 1, 4 );
				QTils_LogMsgEx( "attr #%d chRight=%d (%d)", attr_right, descr->channels.right, xmlErr );
			}
			break;
		case element_vodDesign:{
		  XMLContent *element_content = NULL;
		  void qtvodReadXMLContent( const char *fName, XMLContent *theContent, VODDescription *descr, unsigned short *elm );
			if( XMLContentOfElement( theElement, &element_content ) ){
				idx = 0;
				qtvodReadXMLContent( fName, element_content, descr, &idx );
			}
			break;
		}
		case xmlIdentifierUnrecognized:
			if( theElement->name && theElement->attributes->name ){
				QTils_LogMsgEx( "unknown element <%s %s /> found in '%s'",
						theElement->name, theElement->attributes->name, fName );
			}
			else{
				QTils_LogMsgEx( "unknown element found in '%s'", fName );
			}
			break;
		default:
			xmlErr = paramErr;
			break;
	}
	return xmlErr;
}

void qtvodReadXMLContent( const char *fName, XMLContent *theContent, VODDescription *descr, unsigned short *elm )
{ XMLElement theElement;
	while( XMLContentKind( theContent, *elm ) != xmlContentTypeInvalid ){
		if( XMLElementOfContent( theContent, *elm, &theElement ) ){
			QTils_LogMsgEx( "Scanning attributes and/or elements of element #%d (entry %d)",
						theElement.identifier, *elm );
			qtvodReadXMLElementAttributes( &theElement, descr, fName );
		}
		*elm += 1;
	}
}
#endif

void ReadXMLDoc( const char *fName, XMLDoc xmldoc, VODDescription *descr )
{ XMLContent *theContent;
  size_t elm;
	elm = 0;
	if( !nsXMLVD ){
		nsXMLVD = [[NSVODDescription createWithDescription:&xmlVD] retain];
	}
	@synchronized(nsXMLVD){
		xmlVD = globalVDPreferences;
		while( XMLRootElementContentKind( xmldoc, elm ) != xmlContentTypeInvalid ){
			if( XMLContentOfElementOfRootElement( xmldoc, elm, &theContent ) ){
				ReadXMLContent( fName, theContent, xml_design_parser, sizeof(xml_design_parser)/sizeof(XML_Record), &elm );
				*descr = xmlVD;
			}
		}
	}
}

#pragma mark ---- MCAction handlers

static NSString *metaDataNSStr( Movie theMovie, int trackNr, AnnotationKeys key, char* keyDescr )
{ NSString *ret = nil;
  Track theTrack = NULL;
  char *value = NULL;
	if( trackNr > 0 && trackNr <= GetMovieTrackCount(theMovie) ){
		theTrack = GetMovieIndTrack(theMovie, trackNr);
		GetMetaDataStringFromTrack( theMovie, theTrack, key, &value, NULL );
	}
	else{
		GetMetaDataStringFromMovie( theMovie, key, &value, NULL );
	}
	if( value ){
		ret = [NSString stringWithFormat:@"%s%s\n", keyDescr, value];
		free(value);
	}
	else{
		ret = @"";
	}
	return ret;
}

//static BOOL handlingPlayAction, handlingTimeAction, handlingCloseAction;

//static int movieStep( QTMovieWindowH wih, void *params )
//{ double t;
//  short steps = (short) params;
//	if( !handlingTimeAction ){
//		handlingTimeAction = YES;
//		QTMovieWindowGetTime( wih, &t, 0 );
//		if( steps == 0 ){
//			[QTVDOC(wih) SetTimes:t withRefWindow:wih absolute:NO];
//		}
//		handlingTimeAction = NO;
//	}
//	return 0;
//}

//static int movieScan( QTMovieWindowH wih, void *params )
//{ double t, *tNew = (double*) params;
//
//	if( !handlingTimeAction ){
//		handlingTimeAction = YES;
//		QTMovieWindowGetTime( wih, &t, 0 );
//		if( t != *tNew ){
//			[QTVDOC(wih) SetTimes:*tNew withRefWindow:wih absolute:NO];
//		}
//		handlingTimeAction = NO;
//	}
//	return 0;
//}

//static int moviePlay( QTMovieWindowH wih, void *params )
//{ double t;
//	if( !handlingPlayAction ){
//		if( (*wih)->wasScanned > 0 ){
//			handlingPlayAction = YES;
//			QTMovieWindowGetTime( wih, &t, 0 );
//			[QTVDOC(wih) SetTimes:t withRefWindow:wih absolute:NO];
//			if( QTVDOC(wih) ){
//				[QTVDOC(wih) StopVideoExceptFor:QTVDOC(wih)->sysOwned];
//			}
//			handlingPlayAction = NO;
//		}
//	}
//	return 0;
//}

//static int movieStart( QTMovieWindowH wih, void *params )
//{
//	if( !handlingPlayAction ){
//		handlingPlayAction = YES;
//		if( QTVDOC(wih) ){
//			[QTVDOC(wih) StartVideoExceptFor:QTVDOC(wih)->sysOwned];
//		}
//		handlingPlayAction = NO;
//	}
//	return 0;
//}

//static int movieStop( QTMovieWindowH wih, void *params )
//{
//	if( !handlingPlayAction ){
//		handlingPlayAction = YES;
//		if( QTVDOC(wih) ){
//			[QTVDOC(wih) StopVideoExceptFor:QTVDOC(wih)->sysOwned];
//		}
//		handlingPlayAction = NO;
//	}
//	return 0;
//}

#if 0
static int movieClose0( QTMovieWindowH wih, void *params )
{ QTVODWindow *sO = (QTVDOC(wih))? QTVDOC(wih).sysOwned : NULL;
  BOOL addRecent = addToRecentDocs;
	if( sO && sO.qtmwH == wih && !handlingCloseAction ){
		QTils_LogMsgEx( "'system window' '%s'#%d: closing all", (*wih)->theURL, (*wih)->idx );
		handlingCloseAction = YES;
//		[QTVDOC(wih) CloseVideo:YES];
		handlingCloseAction = NO;
		return 1;
	}
	else{
		if( QTVDOC(wih) ){
			QTils_LogMsgEx( "Closing movie '%s'#%d in window %d", (*wih)->theURL, (*wih)->idx, QTVDOC(wih)->numQTMW );
		}
		else{
			QTils_LogMsgEx( "Closing movie '%s'#%d in remnant window", (*wih)->theURL, (*wih)->idx );
		}
		if( QTVDOC(wih) && (!sO || sO.qtmwH != wih) ){
			QTVDOC(wih)->numQTMW -= 1;
		}
		if( !handlingCloseAction && QTVDOC(wih) ){
			switch( (*wih)->idx ){
				case fwWin:
				case pilotWin:
				case leftWin:
				case rightWin:
				case tcWin:
					addToRecentDocs = NO;
					(*(QTVDOC(wih)->winlist[(*wih)->idx])).addToRecentMenu = 0;
					break;
				default:
					addToRecentDocs = YES;
					(*(QTVDOC(wih)->winlist[(*wih)->idx])).addToRecentMenu = 1;
					break;
			}
			if( (*wih)->idx >= fwWin && (*wih)->idx < maxQTWM ){
				*(QTVDOC(wih)->winlist[(*wih)->idx]) = NULL;
				QTVDOC(wih)->winlist[(*wih)->idx] = NULL;
			}
		}
		if( QTVDOC(wih) && QTVDOC(wih)->numQTMW == 0 && QTVDOC(wih)->finalCloseVideo ){
			if( sO ){
				QTVDOC(wih).sysOwned = nil;
				sO.addToRecentMenu = 1;
				if( sO.qtmwH == wih ){
					QTils_LogMsgEx( "(This was the 'system window' %s)", [[sO displayName] cStringUsingEncoding:NSUTF8StringEncoding] );
				}
				else{
					QTils_LogMsgEx( "(Closing '%s' and the 'system window' %s)",
								(*wih)->theURL, [[sO displayName] cStringUsingEncoding:NSUTF8StringEncoding] );
				}
				if( sO.qtmwH ){
					(*sO.qtmwH)->performingClose = YES;
				}
				{ NSWindow *nsSysWin = [[sO getView] window];
					[nsSysWin performClose:nsSysWin];
				}
				[sO close];
				addToRecentDocs = addRecent;
				return 1;
			}
			if( !handlingCloseAction && !(*wih)->performingClose ){
				addToRecentDocs = NO;
				(*(QTVDOC(wih)->winlist[(*wih)->idx])).addToRecentMenu = 0;
				(*wih)->performingClose = YES;
				[QTVDOC(wih) closeAndRelease];
				addToRecentDocs = addRecent;
				return 1;
			}
		}
	}
	return 0;
}
#endif

//static int movieClose( QTMovieWindowH wih, void *params )
//{ QTVODWindow *sO = (QTVDOC(wih))? QTVDOC(wih)->sysOwned : NULL;
//  int i;
//	if( sO && sO->qtmwH == wih ){
//		QTils_LogMsgEx( "'system window' '%s'#%d: marking all for closing", (*wih)->theURL, (*wih)->idx );
//		// mark for closing: this flag is processed in the window's didUpdate callback
//		sO->shouldBeClosed = YES;
//		QTVDOC(wih)->shouldBeClosed = YES;
//		for( i = 0 ; i < maxQTWM ; i++ ){
//			if( (QTVDOC(wih)->winlist[i]) && (*(QTVDOC(wih)->winlist[i])) ){
//				(*(QTVDOC(wih)->winlist[i]))->shouldBeClosed = YES;
//			}
//		}
//	}
//	else{
//		if( QTVDOC(wih) ){
//			QTils_LogMsgEx( "Closing movie '%s'#%d in window %d", (*wih)->theURL, (*wih)->idx, QTVDOC(wih)->numQTMW );
//		}
//		else{
//			QTils_LogMsgEx( "Closing movie '%s'#%d in remnant window", (*wih)->theURL, (*wih)->idx );
//		}
//		if( QTVDOC(wih) ){
//			(*(QTVDOC(wih)->winlist[(*wih)->idx]))->shouldBeClosed = YES;
//			if( (!sO || sO->qtmwH != wih) ){
//				QTVDOC(wih)->numQTMW -= 1;
//				if( (*wih)->idx >= fwWin && (*wih)->idx < maxQTWM ){
//					*(QTVDOC(wih)->winlist[(*wih)->idx]) = NULL;
//					QTVDOC(wih)->winlist[(*wih)->idx] = NULL;
//				}
//			}
//		}
//		if( QTVDOC(wih) && QTVDOC(wih)->numQTMW == 0 ){
//			if( sO ){
//			// ???
////				QTVDOC(wih)->sysOwned = nil;
//				sO->addToRecentMenu = 1;
//				if( sO->qtmwH == wih ){
//					QTils_LogMsgEx( "(This was the 'system window' %s)", [[sO displayName] cStringUsingEncoding:NSUTF8StringEncoding] );
//				}
//				else{
//					QTils_LogMsgEx( "(Closing '%s' and the 'system window' %s)",
//								(*wih)->theURL, [[sO displayName] cStringUsingEncoding:NSUTF8StringEncoding] );
//				}
//				// in order to get the close 'action' to the remaining, "system" window, it will need to be mapped:
//				[[[sO getView] window] orderBack:[[sO getView] window]];
//				// set the flags AFTER the orderBack command, in case that command executes inline (it usually does).
//				// We do not want the window to start closing while we're still here...
//				QTVDOC(wih)->shouldBeClosed = YES;
//				sO->shouldBeClosed = YES;
//			}
//		}
//	}
//	return 0;
//}

//static int movieKeyUp( QTMovieWindowH wih, void *params )
//{ EventRecord *evt = (EventRecord*) params;
//  QTVOD *qtVOD = QTVDOC(wih);
//  int ret = 0;
//	if( qtVOD ){
//		evt->message &= charCodeMask;
//		switch( evt->message ){
//			case 'i':
//			case 'I':
//				[qtVOD ShowMetaData];
//				break;
//			// key 'c'/'C' handled by the parent MCAction controller
//			case 'f':
//			case 'F':
//				[qtVOD SetWindowLayer:NSWindowAbove relativeTo:0];
//				break;
//			case 'b':
//			case 'B':
//				[qtVOD SetWindowLayer:NSWindowBelow relativeTo:0];
//				break;
//			case '1':
//				qtVOD->theDescription.scale /= sqrt(2.0);
//				[qtVOD PlaceWindows:nil withScale:1.0/sqrt(2.0)];
//				break;
//			case '2':
//				qtVOD->theDescription.scale *= sqrt(2.0);
//				[qtVOD PlaceWindows:nil withScale:sqrt(2.0)];
//				break;
//			case 't':
//			case 'T':
//				[qtVOD CalcTimeInterval:YES reset:NO];
//				break;
//			case 'p':
//			case 'P':
//				if( qtVOD->pilot && QTMovieWindowH_isOpen(qtVOD->pilot->qtmwH) ){
//				  Cartesian ulCorner;
//					QTMovieWindowGetGeometry(qtVOD->pilot->qtmwH, &ulCorner, NULL, 1 );
//					ulCorner.horizontal -= qtVOD->Wsize[leftWin].horizontal;
//					[qtVOD PlaceWindows:&ulCorner withScale:1.0];
//				}
//				break;
//			case 'q':
//			case 'Q':
//#if 0
//				handlingCloseAction = YES;
//				if( qtVOD->sysOwned ){
//					[qtVOD->sysOwned performClose:[[qtVOD->sysOwned getView] window]];
//				}
//				else{
//					[qtVOD closeAndRelease];
//				}
//				handlingCloseAction = NO;
//#else
//				{ int i;
//					qtVOD->shouldBeClosed = YES;
//					for( i = 0 ; i < maxQTWM ; i++ ){
//						if( (qtVOD->winlist[i]) && (*(qtVOD->winlist[i])) ){
//							(*(qtVOD->winlist[i]))->shouldBeClosed = YES;
//						}
//					}
//				}
//#endif
//				ret = 1;
//				break;
//		}
//	}
//	return ret;
//}

#pragma mark ---- URL functions

NSURL *pruneExtension( NSURL *URL, NSString *ext )
{ NSString *url = [[[NSString alloc] initWithString:[URL path]] autorelease];
  NSRange range;
	range = [url rangeOfString:ext options:(NSAnchoredSearch|NSCaseInsensitiveSearch|NSBackwardsSearch)];
	if( range.location != NSNotFound ){
		return [NSURL URLWithString:[url substringToIndex:range.location]];
	}
	else{
		return URL;
	}
}

NSURL *pruneExtensions( NSURL *URL )
{ NSURL *url;
	// the Modula-2 MSWin implementation removes a single leading and/or trailing quote (")
	// before attacking the list of extensions. That is (probably) not necessary here.
	url = pruneExtension(URL, @".mov");
	url = pruneExtension(url, @".vod");
	url = pruneExtension( url, @".qi2m" );
	url = pruneExtension( url, @".xml" );
	url = pruneExtension( url, @".ief" );
	url = pruneExtension( url, @"-design" );
	url = pruneExtension( url, @"-forward" );
	url = pruneExtension( url, @"-pilot" );
	url = pruneExtension( url, @"-left" );
	url = pruneExtension( url, @"-right" );
	url = pruneExtension( url, @"-TC" );
	return [NSURL fileURLWithPath:[url path]];
}

NSMutableArray *QTVODList = NULL;

#pragma mark ---- QTVOD class functions

BOOL addToRecentDocs = YES;

// for some reason, our own QTVODController subclass doesn't work as expected,
// but strangely we are allowed to override [NSDocumentController noteNewRecentDocument:] directly:
@implementation NSDocumentController (overridden)
- (void) noteNewRecentDocument:(QTVODWindow*)theDoc
{
	if( addToRecentDocs && theDoc.addToRecentMenu != 0 ){
#ifdef DEBUG
	  NSURL *fU = [theDoc fileURL];
		[self noteNewRecentDocumentURL:fU];
#else
		[self noteNewRecentDocumentURL:[theDoc fileURL]];
#endif
	}
}

@end

@interface ChannelViewSpec : NSObject {
@public
	NSURL *src, *movieCache;
	QTVODwinIDs channel;
	int channelNr;
	const char *channelName;
	char eMsg[2048];
	VODDescription *description;
	NSString *typeName;
	NSError **outError;
	BOOL done;
}

+ (ChannelViewSpec*) createWithURL:(NSURL*)s forChannel:(QTVODwinIDs)ch withName:(const char*)name
	withDescription:(VODDescription*)descr ofType:(NSString*)tName error:(NSError**)error;
@property const char *channelName;
@property VODDescription *description;
@property (retain) NSString *typeName;
@property NSError **outError;
@property int channelNr;
@property BOOL done;
@end

@implementation ChannelViewSpec

- (void) dealloc
{
	if( movieCache ){
		[movieCache release];
	}
	[super dealloc];
}

+ (ChannelViewSpec*) createWithURL:(NSURL*)s forChannel:(QTVODwinIDs)ch withName:(const char*)name
			   withDescription:(VODDescription*)descr ofType:(NSString*)tName error:(NSError**)error
{
	self = [[self alloc] init];
	if( self ){
	  ChannelViewSpec *cv = (ChannelViewSpec*) self;
		cv->src = s;
		cv->channel = ch;
		cv->channelName = name;
		cv->description = descr;
		cv->typeName = tName;
		cv->outError = error;
		cv->movieCache = nil;
		cv->eMsg[0] = '\0';
		cv->done = NO;
	}
	return [self autorelease];
}

@synthesize channelName;
@synthesize description;
@synthesize typeName;
@synthesize outError;
@synthesize channelNr;
@synthesize done;
@end


@interface QTVOD (Private)
- (void) register_window:(QTMovieWindowH)wih withIndex:(int)idx;
- (void) register_windows;
- (ErrCode) ImportMovie:(NSURL*)src withDescription:(VODDescription*)description;
- (QTVODWindow*) OpenChannelView:(NSURL*)cachedMovieFile forChannelNr:(int)channel withName:(const char*) chName;
- (id) CreateChannelView:(NSURL*)src withDescription:(VODDescription*)description
	channel:(int)channel channelName:(const char*)chName scale:(double)scale
	display:(BOOL)openNow ofType:(NSString *)typeName error:(NSError **)outError eMsg:(char*)eMsg;
- (void) CreateChannelViewInBackground:(ChannelViewSpec*)spec;
@end

@implementation QTVOD (Private)

- (int) movieStep:(QTMovieWindowH)wih withParams:(void*) params
{ double t;
  short steps = (short) params;
	if( !handlingTimeAction ){
		handlingTimeAction = YES;
		QTMovieWindowGetTime( wih, &t, 0 );
		if( steps == 0 ){
			[self SetTimes:t withRefWindow:wih absolute:NO];
		}
		handlingTimeAction = NO;
	}
	return 0;
}

- (int) movieScan:(QTMovieWindowH)wih withParams:(void*) params
{ double t, *tNew = (double*) params;

	if( !handlingTimeAction ){
		handlingTimeAction = YES;
		QTMovieWindowGetTime( wih, &t, 0 );
		if( t != *tNew ){
			[self SetTimes:*tNew withRefWindow:wih absolute:NO];
		}
		handlingTimeAction = NO;
	}
	return 0;
}

- (int) moviePlay:(QTMovieWindowH)wih withParams:(void*)params
{ double t;
	if( !handlingPlayAction ){
		if( (*wih)->wasScanned > 0 ){
			handlingPlayAction = YES;
			QTMovieWindowGetTime( wih, &t, 0 );
			[self SetTimes:t withRefWindow:wih absolute:NO];
			[self StopVideoExceptFor:sysOwned];
			handlingPlayAction = NO;
		}
	}
	return 0;
}

- (int) movieStart:(QTMovieWindowH)wih withParams:(void*)params
{
	if( !handlingPlayAction ){
		handlingPlayAction = YES;
		[self StartVideoExceptFor:sysOwned];
		handlingPlayAction = NO;
	}
	return 0;
}

- (int) movieStop:(QTMovieWindowH)wih withParams:(void*) params
{
	if( !handlingPlayAction ){
		handlingPlayAction = YES;
		[self StopVideoExceptFor:sysOwned];
		handlingPlayAction = NO;
	}
	return 0;
}

- (int) movieClose:(QTMovieWindowH)wih withParams:(void*)params
{ int i;
	if( sysOwned && sysOwned.qtmwH == wih ){
		QTils_LogMsgEx( "'system window' '%s'#%d: marking all for closing", (*wih)->theURL, (*wih)->idx );
		// mark for closing: this flag is processed in the window's didUpdate callback
		sysOwned.shouldBeClosed = YES;
		shouldBeClosed = YES;
		for( i = 0 ; i < maxQTWM ; i++ ){
			if( (winlist[i]) && (*(winlist[i])) ){
				(*(winlist[i])).shouldBeClosed = YES;
			}
		}
	}
	else{
		if( self ){
			QTils_LogMsgEx( "Closing movie '%s'#%d in window %d", (*wih)->theURL, (*wih)->idx, numQTMW );
		}
		else{
			QTils_LogMsgEx( "Closing movie '%s'#%d in remnant window", (*wih)->theURL, (*wih)->idx );
		}
		(*(winlist[(*wih)->idx])).shouldBeClosed = YES;
		if( (!sysOwned || sysOwned.qtmwH != wih) ){
			numQTMW -= 1;
			if( (*wih)->idx >= fwWin && (*wih)->idx < maxQTWM ){
				*(winlist[(*wih)->idx]) = NULL;
				winlist[(*wih)->idx] = NULL;
			}
		}
		if( numQTMW == 0 ){
			if( sysOwned ){
				sysOwned.addToRecentMenu = 1;
				if( sysOwned.qtmwH == wih ){
					QTils_LogMsgEx( "(This was the 'system window' %s)", [[sysOwned displayName] cStringUsingEncoding:NSUTF8StringEncoding] );
				}
				else{
					QTils_LogMsgEx( "(Closing '%s' and the 'system window' %s)",
								(*wih)->theURL, [[sysOwned displayName] cStringUsingEncoding:NSUTF8StringEncoding] );
				}
				// in order to get the close 'action' to the remaining, "system" window, it will need to be mapped
				// and we don't want it to start closing during the call to orderBack: !
				shouldBeClosed = NO;
				sysOwned.shouldBeClosed = NO;
				[[[sysOwned getView] window] orderBack:[[sysOwned getView] window]];
				// set the flags AFTER the orderBack command, in case that command executes inline (it usually does).
				// We do not want the window to start closing while we're still here...
				shouldBeClosed = YES;
				if( sysOwned ){
					sysOwned.shouldBeClosed = YES;
				}
			}
		}
	}
	return 0;
}

- (int) movieKeyUp:(QTMovieWindowH)wih withParams:(void*)params
{ EventRecord *evt = (EventRecord*) params;
  int ret = 0;
	evt->message &= charCodeMask;
	switch( evt->message ){
		case 'i':
		case 'I':
			[self ShowMetaData];
			break;
		// key 'c'/'C' handled by the parent MCAction controller
		case 'f':
		case 'F':
			[self SetWindowLayer:NSWindowAbove relativeTo:0];
			break;
		case 'b':
		case 'B':
			[self SetWindowLayer:NSWindowBelow relativeTo:0];
			break;
		case '1':
			self->theDescription.scale /= sqrt(2.0);
			[self PlaceWindows:nil withScale:1.0/sqrt(2.0)];
			break;
		case '2':
			self->theDescription.scale *= sqrt(2.0);
			[self PlaceWindows:nil withScale:sqrt(2.0)];
			break;
		case 't':
		case 'T':
			[self CalcTimeInterval:YES reset:NO];
			break;
		case 'p':
		case 'P':
			if( self->pilot && QTMovieWindowH_isOpen(self->pilot.qtmwH) ){
			  Cartesian ulCorner;
				QTMovieWindowGetGeometry(self->pilot.qtmwH, &ulCorner, NULL, 1 );
				ulCorner.horizontal -= self->Wsize[leftWin].horizontal;
				[self PlaceWindows:&ulCorner withScale:1.0];
			}
			break;
		case 'q':
		case 'Q':
			{ int i;
				shouldBeClosed = YES;
				for( i = 0 ; i < maxQTWM ; i++ ){
					if( (winlist[i]) && (*(winlist[i])) ){
						(*(winlist[i])).shouldBeClosed = YES;
					}
				}
			}
			ret = 1;
			break;
	}
	return ret;
}

#define REGISTER_NSMCACTION(wih,action,sel)	register_NSMCAction(self,(wih),(action),@selector(sel:withParams:),(NSMCActionCallback) [self methodForSelector:@selector(sel:withParams:)])

- (void) register_window:(QTMovieWindowH)wih withIndex:(int)idx
{
	if( wih && (*wih) && (*wih)->self == *wih && (*wih)->theView ){
		handlingPlayAction = NO;
		handlingTimeAction = NO;
		handlingCloseAction = NO;
		(*wih)->idx = idx;
		REGISTER_NSMCACTION( wih, MCAction()->Step, movieStep );
		REGISTER_NSMCACTION( wih, MCAction()->Play, moviePlay );
		REGISTER_NSMCACTION( wih, MCAction()->GoToTime, movieScan );
		REGISTER_NSMCACTION( wih, MCAction()->Start, movieStart );
		REGISTER_NSMCACTION( wih, MCAction()->Stop, movieStop );
		REGISTER_NSMCACTION( wih, MCAction()->Close, movieClose );
		REGISTER_NSMCACTION( wih, MCAction()->KeyUp, movieKeyUp );
		QTMovieWindowGetGeometry( wih, &Wpos[idx], &Wsize[idx], 1 );
	}
}

- (void) register_windows
{ int i;
	for( i = 0 ; i < maxQTWM ; i++ ){
		if( QTMWH(i) ){
//			register_window( QTMWH(i), i, &Wpos[i], &Wsize[i] );
			[self register_window:QTMWH(i) withIndex:i ];
			if( i != sysWin ){
				numQTMW += 1;
			}
			if( *QTMWH(i) ){
				(*QTMWH(i))->user_data = (void*) self;
			}
		}
	}
}

- (ErrCode) ImportMovie:(NSURL*)src withDescription:(VODDescription*)description;
{ ErrCode err = noErr, err2;
  NSString *fn = [NSString stringWithFormat:@"%@.qi2m", [src path] ];
  NSString *vodsource = [NSString stringWithFormat:@"%@.VOD", [src path] ];
  const char *fName = [fn UTF8String];
  Movie theMovie = NULL;
  FILE *fp;
	if( (fp = fopen(fName, "w")) ){
		fputs( "<?xml version=\"1.0\"?>\n", fp );
		fputs( "<?quicktime type=\"video/x-qt-img2mov\"?>\n", fp );
		// autosave will take care of generating the cache movie:
		fputs( "<import autoSave=True >\n", fp );
		if( assocDataFile ){
			fprintf( fp, "\t<description txt=\"UTC timeZone=%g, DST=%hd, assoc.data:%s\" />\n",
				   description->timeZone, (short) description->DST, [assocDataFile UTF8String]
			);
		}
		else{
			fprintf( fp, "\t<description txt=\"UTC timeZone=%g, DST=%hd\" />\n",
				   description->timeZone, (short) description->DST
			);
		}
		fprintf( fp, "\t<sequence src=\"%s\" channel=-1 freq=%g", [vodsource UTF8String], description->frequency );
		fprintf( fp, " hidetc=False timepad=False hflip=False vmgi=%s", (description->useVMGI)? "True" : "False" );
		fprintf( fp, " newchapter=True log=%s", (description->log)? "True" : "False" );
		fputs( " />\n", fp );
		fputs( "</import>\n", fp );
		fclose(fp);
		err = OpenMovieFromURL( &theMovie, 1, NULL, fName, NULL, NULL );
		if( err != noErr ){
		  const char *es, *ec;
			es = MacErrorString( LastQTError(), &ec );
			if( es ){
				QTils_LogMsgEx( "Import failure for '%s': %s (%s)", fName, es, ec );
			}
			else{
				QTils_LogMsgEx( "Import failure for '%s': %d (%d)", fName, LastQTError(), err );
			}
			PostMessage( fName, lastSSLogMsg );
			if( !(fp = fopen( [vodsource UTF8String], "r" )) ){
				QTils_LogMsgEx( "'%s' does not exist, so '%s' will be deleted",
							[vodsource UTF8String], fName
				);
				unlink(fName);
				err = couldNotResolveDataRef;
			}
			else{
				fclose(fp);
			}
		}
		else{
			CloseMovie(&theMovie);
			unlink(fName);
			QTils_LogMsgEx( "'%s' imported and deleted", fName );
			// now open the newly created cache movie:
			fName = [[NSString stringWithFormat:@"%@.mov", [src path] ] UTF8String];
			fullMovie = NULL;
			err = OpenMovieFromURL( &fullMovie, 1, NULL, fName, NULL, NULL );
			if( fullMovie && assocDataFile ){
				err2 = AddMetaDataStringToMovie( fullMovie, akSource, [assocDataFile UTF8String], NULL );
				if( err2 == noErr ){
					SaveMovie(fullMovie);
				}
			}
		}
	}
	else{
		PostMessage( fName, "Creation/opening error" );
		err = 1;
	}
	return err;
}

- (QTVODWindow*) OpenChannelView:(NSURL*)cachedMovieFile forChannelNr:(int)channel withName:(const char*) chName
{ QTVODWindow *wi = nil;
	if( cachedMovieFile ){
	  NSError *error = nil;
//		  QTVODController *ctrl = [[[QTVODController alloc] init] autorelease];
	  BOOL addRecent = addToRecentDocs;
		addToRecentDocs = NO;
//			wi = [ctrl openDocumentWithURLContents:cachedMovieFile error:&error addToRecent:NO];
		// we use the sharedDocumentController to tell ourselves to open 'cachedMovieFile' as
		// if the user requested that document via the Open menu or the Finder:
		wi = [[NSDocumentController sharedDocumentController] openDocumentWithContentsOfURL:cachedMovieFile display:YES error:&error];
		addToRecentDocs = addRecent;
// have faith ... if openDocumentWithContentsOfURL failed, opening a QTVODWindow manually will also fail...
//			if( !wi ){
//				wi = [QTVODWindow alloc];
//				if( wi ){
//					wi->isProgrammatic = YES;
//					[wi setQTVOD:(struct QTVOD*)self];
//					[NSBundle loadNibNamed:@"QTVODWindow" owner:wi];
//					[wi initWithContentsOfURL:cachedMovieFile ofType:typeName error:outError];
//				}
//			}
		if( !wi || error ){
			if( wi ){
				[wi close];
			}
			QTils_LogMsgEx( "Error creating window for %s channel, \"%s\" (%s)", chName,
						[[cachedMovieFile path] UTF8String],
						(error)? [[error localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding] : "unknown error" );
		}
		else{
			if( channel != 5 && channel != 6 ){
				if( [[wi getView] isControllerVisible] ){
					[wi ToggleMCController];
				}
			}
			wi->vodView = channel;
			// none of the channel views are to register in the recent documents menu:
			wi.addToRecentMenu = 0;
		}
	}
	return wi;
}

// creates a channel view for the VOD video in <src> based on the other arguments. Returns either
// a pointer to the opened window (display:YES) or the URL containing the channel view cache movie.
- (id) CreateChannelView:(NSURL*)src withDescription:(VODDescription*)description
	channel:(int)channel channelName:(const char*)chName scale:(double)scale
	display:(BOOL)openNow ofType:(NSString *)typeName error:(NSError **)outError eMsg:(char*)eMsg
{ NSURL *cachedMovieFile = nil;
  NSString *fn, *dn = [NSString stringWithFormat:@"%@vid", [src path] ];
  const char *fName;
  Movie theMovie = NULL;
  ErrCode err;
  NSError *derr = NULL;
  BOOL usingCacheDir;

	if( [[[[NSFileManager alloc] init] autorelease]
			createDirectoryAtPath:dn withIntermediateDirectories:YES attributes:nil error:&derr]
	){
		fn = [NSString stringWithFormat:@"%@vid/%s.mov", [src path], chName ];
		usingCacheDir = YES;
	}
	else{
		// fall back on the old approach of creating the cache files alongside the input
		fn = [NSString stringWithFormat:@"%@-%s.mov", [src path], chName ];
		usingCacheDir = NO;
	}
	fName = [fn UTF8String];
	// create a cache movie for the requested view, and return an NSURL to its location

	if( !recreateChannelViews ){
		err = OpenMovieFromURL( &theMovie, 1, NULL, fName, NULL, NULL );
	}
	if( !theMovie ){
#ifdef USECHANNELVIEWIMPORTFILES
	  FILE *fp;
	  NSString *importFile;
		if( usingCacheDir ){
			importFile = [NSString stringWithFormat:@"%@vid/%s.qi2m", [src path], chName ];
		}
		else{
			importFile = [NSString stringWithFormat:@"%@-%s.qi2m", [src path], chName ];
		}
		fName = [importFile UTF8String];
		if( doLogging ){
			// LogMsgEx uses a static buffer (lastSSLogMsg) to construct the formatted message, so we
			// need to avoid concurrent execution of the call (we use the class instance URL variable,
			// which is shared among all potential concurrent threads).
			// 20111128: QTils now has its own locking mechanism
			QTils_LogMsgEx( "Creating '%s' for channel %d", fName, channel );
		}
		if( (fp = fopen( fName, "w" )) ){
			fputs( "<?xml version=\"1.0\"?>\n", fp );
			fputs( "<?quicktime type=\"video/x-qt-img2mov\"?>\n", fp );
			// autosave will take care of generating the cache movie:
			fputs( "<import autoSave=False askSave=False >\n", fp );
			if( assocDataFile ){
				fprintf( fp, "\t<description txt=\"UTC timeZone=%g, DST=%hd, assoc.data:%s\" />\n",
					description->timeZone, (short) description->DST, [assocDataFile UTF8String]
				);
			}
			else{
				fprintf( fp, "\t<description txt=\"UTC timeZone=%g, DST=%hd\" />\n",
					description->timeZone, (short) description->DST
				);
			}
			fprintf( fp, "\t<sequence src=\"%s.mov\" channel=%d", [[src path] UTF8String], channel );
			if( channel == 5 || channel == 6 ){
				// ch.5: TimeCode track; ch.6: extended/text timecode track
				fputs( " hidetc=False timepad=False asmovie=True newchapter=False", fp );
			}
			else{
				// we hide the timecode display of the 4 camera views
				fputs( " hidetc=True timepad=False asmovie=True", fp );
				if( description->flipLeftRight
				   && (channel == description->channels.left || channel == description->channels.right)
				){
					fputs( " hflip=True", fp );
				}
				else{
					fputs( " hflip=False", fp );
				}
			}
			fputs( " />\n", fp );
			fputs( "</import>\n", fp );
			fclose(fp);
			err = OpenMovieFromURL( &theMovie, 1, NULL, fName, NULL, NULL );
			if( !theMovie ){
			  const char *es, *ec;
				es = MacErrorString( LastQTError(), &ec );
				if( es ){
					QTils_LogMsgEx( "Import failure for '%s': %s (%s)", fName, es, ec );
				}
				else{
					QTils_LogMsgEx( "Import failure for '%s': %d (%d)", fName, LastQTError(), err );
				}
				if( openNow ){
					PostMessage( fName, lastSSLogMsg );
				}
				else if( eMsg ){
					strcpy( eMsg, lastSSLogMsg );
				}
			}
			else{
			  Track track = NULL;
				unlink(fName);
				if( doLogging ){
					QTils_LogMsgEx( "'%s' imported and unlinked", fName );
				}
				if( channel != 5 && channel != 6 ){
					if( GetTrackWithName( theMovie, "timeStamp Track", TextMediaType, 0, &track, NULL ) == noErr && track ){
						SetTrackEnabled( track, NO );
					}
				}
				track = NULL;
				if( GetTrackWithName( theMovie, "TimeCode Track", TimeCodeMediaType, 0, &track, NULL ) == noErr && track ){
					SetTrackEnabled( track, YES );
				}
				cachedMovieFile = [NSURL fileURLWithPath:fn];
				fName = [[cachedMovieFile path] cStringUsingEncoding:NSUTF8StringEncoding];
				err = SaveMovieAsRefMov( fName, theMovie );
				if( err != noErr ){
					QTils_LogMsgEx( "Channel view creation failure saving '%s': %d", fName, err );
				}
				else if( doLogging ){
					QTils_LogMsgEx( "channel view '%s' created", fName );
				}
			}
		}
		else{
			if( openNow ){
				PostMessage( fName, "Creation/opening error" );
			}
			else if( eMsg ){
				strcpy( eMsg, "Creation/opening error" );
			}
		}
#else
	  MemoryDataRef memRef;
	  NSMutableString *qi2mString;
	  const char *fName;
		cachedMovieFile = [NSURL fileURLWithPath:fn];
		fName = [[cachedMovieFile path] cStringUsingEncoding:NSUTF8StringEncoding];
		qi2mString = [NSMutableString stringWithFormat:@"<?xml version=\"1.0\"?>\n"
							   "<?quicktime type=\"video/x-qt-img2mov\"?>\n"
							   "<import autoSave=True autoSaveName=\"%s\" >\n", fName];
		if( doLogging ){
			QTils_LogMsgEx( "Creating in-memory view for channel %d", channel );
		}
		if( assocDataFile ){
			[qi2mString appendFormat:@"\t<description txt=\"UTC timeZone=%g, DST=%hd, assoc.data:%s\" />\n",
				   description->timeZone, (short) description->DST, [assocDataFile UTF8String] ];
		}
		else{
			[qi2mString appendFormat:@"\t<description txt=\"UTC timeZone=%g, DST=%hd\" />\n",
				   description->timeZone, (short) description->DST ];
		}
		[qi2mString appendFormat:@"\t<sequence src=\"%s.mov\" channel=%d", [[src path] UTF8String], channel ];
		if( channel == 5 || channel == 6 ){
			// ch.5: TimeCode track; ch.6: extended/text timecode track
			[qi2mString appendString:@" hidetc=False timepad=False asmovie=True newchapter=False" ];
		}
		else{
			// we hide the timecode display of the 4 camera views
			[qi2mString appendString:@" hidetc=True timepad=False asmovie=True" ];
			if( description->flipLeftRight
			   && (channel == description->channels.left || channel == description->channels.right)
			){
				[qi2mString appendString:@" hflip=True" ];
			}
			else{
				[qi2mString appendString:@" hflip=False" ];
			}
		}
		[qi2mString appendString:@" />\n"
				"</import>\n" ];
		if( (err = MemoryDataRefFromString( [qi2mString cStringUsingEncoding:NSUTF8StringEncoding], [[src path] UTF8String], &memRef )) == noErr ){
			@synchronized(self){
				theMovie = NULL;
				err = OpenMovieFromMemoryDataRef( &theMovie, &memRef, 'QI2M' );
			}
			if( err != noErr || !theMovie ){
			  const char *es, *ec;
			  ErrCode last = LastQTError();
				es = MacErrorString( last, &ec );
				if( es ){
					QTils_LogMsgEx( "Channel view creation failure: %s (%s)", es, ec );
				}
				else{
					QTils_LogMsgEx( "Channel view creation failure: %d (%d)", last, err );
				}
				if( openNow ){
					PostMessage( fName, lastSSLogMsg );
				}
				else if( eMsg ){
					strcpy( eMsg, lastSSLogMsg );
				}
			}
			else{
			  Track track = NULL;
				if( channel != 5 && channel != 6 ){
					if( GetTrackWithName( theMovie, "timeStamp Track", TextMediaType, 0, &track, NULL ) == noErr && track ){
						SetTrackEnabled( track, NO );
					}
				}
				track = NULL;
				if( GetTrackWithName( theMovie, "TimeCode Track", TimeCodeMediaType, 0, &track, NULL ) == noErr && track ){
					SetTrackEnabled( track, YES );
				}
				if( HasMovieChanged(theMovie) ){
					cachedMovieFile = [NSURL fileURLWithPath:fn];
					fName = [[cachedMovieFile path] cStringUsingEncoding:NSUTF8StringEncoding];
					err = SaveMovieAsRefMov( fName, theMovie );
					if( err != noErr ){
						QTils_LogMsgEx( "Channel view creation failure saving '%s': %d", fName, err );
					}
					else if( doLogging ){
						QTils_LogMsgEx( "channel view '%s' created", fName );
					}
				}
				else if( doLogging ){
					QTils_LogMsgEx( "channel view '%s' created", fName );
				}
			}
			// we shouldn't dispose the dataRef here, it's still associated with theMovie and will be disposed
			// when the movie is closed.
			memRef.dataRef = NULL;
			DisposeMemoryDataRef(&memRef);
		}
#endif
	}
	if( theMovie && err == noErr ){
		CloseMovie(&theMovie);
		cachedMovieFile = [NSURL fileURLWithPath:fn];
		if( openNow ){
			// if we're to open the channelview cache movie, do it now and return the QTVODWindow instance
			return [self OpenChannelView:cachedMovieFile forChannelNr:channel withName:chName];
		}
		else{
			// just return the retained file URL
			return [cachedMovieFile retain];
		}
	}
	return nil;
}

// create a channel's movie cache file on a background thread, to gain a little time. When finished
// we have to hand off the cache file URL to the main thread as QTMovie instances can only be opened
// properly from the main thread (see http://developer.apple.com/library/mac/#technotes/tn2138/_index.html)
// Note that CreateChannelView: returns an NSURL (retained!) when display:NO !
- (void) CreateChannelViewInBackground:(ChannelViewSpec*)spec
{ NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	if( !spec ){
		return;
	}
	@synchronized(spec){
		if( !spec->description ){
			spec->description = &theDescription;
		}
		switch( spec->channel ){
			case fwWin:
				spec->channelNr = spec->description->channels.forward;
				[[NSThread currentThread] setName:@"forward"];
				break;
			case pilotWin:
				spec->channelNr = spec->description->channels.pilot;
				[[NSThread currentThread] setName:@"pilot"];
				break;
			case leftWin:
				spec->channelNr = spec->description->channels.left;
				[[NSThread currentThread] setName:@"left"];
				break;
			case rightWin:
				spec->channelNr = spec->description->channels.right;
				[[NSThread currentThread] setName:@"right"];
				break;
			case tcWin:
				spec->channelNr = 6;
				[[NSThread currentThread] setName:@"TC"];
				break;
		}
		if( spec->channelNr > 0 ){
			spec->movieCache = [self CreateChannelView:spec->src withDescription:spec->description
							    channel:spec->channelNr channelName:spec->channelName
							    scale:spec->description->scale display:NO ofType:spec->typeName
							    error:spec->outError eMsg:&spec->eMsg[0]];
			spec->done = YES;
			NSLog( @"Created channelView %s in %@ for %@, %@", spec->channelName, spec->movieCache, spec->src, [NSThread currentThread] );
		}
	}
	[pool drain];
}

@end

@implementation QTVOD

+ (QTVOD*) createWithAbsoluteURL:(NSURL*)aURL ofType:(NSString*)typeName
	forDocument:(QTVODWindow*)sO withAssocDataFile:(NSString*)aDFile
	error:(NSError **)outError
{
	self = [[self alloc] init];
	if( self ){
	  QTVOD *qv = (QTVOD*) self;
		// avoid warnings about accessing instance variables by using a local instance of ourselves...
		qv->sysOwned = sO;
		qv->assocDataFile = [aDFile retain];
		if( ![qv readFromURL:aURL ofType:typeName error:outError] ){
			[self release];
			return nil;
		}
		else{
			[QTVODList addObject:self];
		}
	}
	return [self autorelease];
}

- (id) init
{
    self = [super init];
    if (self) {
        // If an error occurs here, send a [self release] message and return nil.
		forward = pilot = left = right = TC = NULL;
		winlist[fwWin] = &forward;
		winlist[pilotWin] = &pilot;
		winlist[leftWin] = &left;
		winlist[rightWin] = &right;
		winlist[tcWin] = &TC;
		winlist[sysWin] = &sysOwned;

		xmlParser = nil;
		theDescription = globalVDPreferences;
		theDescription.changed = NO;
		fullMovieChanged = NO;
		ULCorner.horizontal = ULCorner.vertical = 0;
		theURL = NULL;
		theDirPath = NULL;
		assocDataFile = NULL;
		theTimeInterval.timeA = -1.0;
		theTimeInterval.dt = 0.0;
		numQTMW = 0;
		hasBeenClosed = NO;
		beingClosed = NO;
    }
    // Allocate QTVODList when we're going to need it
    if( !QTVODList ){
	    QTVODList = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void) dealloc
{ int i;
	for( i = 0 ; i < maxQTWM ; i++ ){
		if( WINLIST(i) ){
			[[[WINLIST(i) getView] window] performClose:[[WINLIST(i) getView] window]];
			// winlist[i] can have changed handling the performClose action, so check again:
			if( WINLIST(i) ){
				[WINLIST(i) close];
				*winlist[i] = NULL;
			}
		}
	}
	if( xmlParser ){
		DisposeXMLParser( &xmlParser, NULL, 1);
	}
	if( fullMovie ){
		if( fullMovieChanged || HasMovieChanged(fullMovie) ){
			if( PostYesNoDialog( [[self displayName] cStringUsingEncoding:NSUTF8StringEncoding],
							"This recording's cache movie has changed, do you want to save it?")
			){
				SaveMovie(fullMovie);
			}
		}
		CloseMovie(&fullMovie);
	}
	if( theURL ){
		[theURL release];
	}
	if( theDirPath ){
		[theDirPath release];
	}
	if( assocDataFile ){
		[assocDataFile release];
	}
	if( sysOwned ){
		[sysOwned setQTVOD:nil];
		sysOwned = nil;
	}
	if( QTVODList && [QTVODList containsObject:self] ){
		[QTVODList removeObject:self];
	}
	[super dealloc];
}

- (void) close
{
	if( !hasBeenClosed ){
		[self CloseVideo:YES];
		[super close];
		hasBeenClosed = YES;
	}
}

- (void) closeAndRelease
{
	if( !beingClosed ){
		beingClosed = YES;
		if( QTVODList && [QTVODList containsObject:self] ){
			[QTVODList removeObject:self];
		}
		[self close];
		[self release];
		beingClosed = NO;
	}
}

- (void) reregister_window:(QTMovieWindowH)wih
{ int i;
	for( i = 0 ; i < maxQTWM ; i++ ){
		if( QTMWH(i) == wih ){
//			register_window( QTMWH(i), i, &Wpos[i], &Wsize[i] );
			[self register_window:QTMWH(i) withIndex:i ];
		}
	}
}

- (void) ShowMetaData
{ static NSMutableString *MetaDataDisplayStr = NULL;
  QTMovieWindowH wih;
  extern QTMovieWindowH QTMovieWindowH_from_Movie(Movie);
  char header[1024];
	if( fullMovie && !MetaDataDisplayStr && (wih = QTMovieWindowH_from_Movie(fullMovie)) ){
	  MovieFrameTime ft;
		secondsToFrameTime( (*wih)->info->startTime, (*wih)->info->frameRate, &ft );
		snprintf( header, sizeof(header), "Recording %s duration %gs; starts at %02d:%02d:%02d;%03d\n",
				    [[theURL lastPathComponent] fileSystemRepresentation], (*wih)->info->duration,
				    (int) ft.hours, (int) ft.minutes, (int) ft.seconds, (int) ft.frames );
		if( assocDataFile ){
		  size_t len = strlen(header);
			snprintf( &header[len], sizeof(header)-len, "associated data file: %s\n\n",
				    [[[NSURL fileURLWithPath:assocDataFile] relativeString] fileSystemRepresentation] );
		}
		MetaDataDisplayStr = [[[NSMutableString alloc] init] autorelease];
		[MetaDataDisplayStr appendString:metaDataNSStr( fullMovie, 1, akDisplayName, "Name: " ) ];
		[MetaDataDisplayStr appendString:metaDataNSStr( fullMovie, 1, akSource, "Original file: " ) ];
		[MetaDataDisplayStr appendString:metaDataNSStr( fullMovie, 1, akCreationDate, "Creation date: " ) ];
		[MetaDataDisplayStr appendString:metaDataNSStr( fullMovie, 1, akDescr, "Description: " ) ];
		[MetaDataDisplayStr appendString:metaDataNSStr( fullMovie, 1, akInfo, "Information: " ) ];
		[MetaDataDisplayStr appendString:metaDataNSStr( fullMovie, 1, akComment, "Comments: " ) ];
		[MetaDataDisplayStr appendString:metaDataNSStr( fullMovie, -1, akDescr, "Description: " ) ];
		[MetaDataDisplayStr appendString:metaDataNSStr( fullMovie, -1, akInfo, "Information: " ) ];
		[MetaDataDisplayStr appendString:metaDataNSStr( fullMovie, -1, akComment, "Comments: " ) ];
		[MetaDataDisplayStr appendString:metaDataNSStr( fullMovie, -1, akCreationDate, "Creation date cache/.mov file: " ) ];
	}
	if( MetaDataDisplayStr ){
	  NSAlert* alert = [[[NSAlert alloc] init] autorelease];
		[alert setAlertStyle:NSInformationalAlertStyle];
		[alert setMessageText:[NSString stringWithUTF8String:header]];
		[alert setInformativeText:MetaDataDisplayStr];
		[[alert window] setTitle:@"Meta data"];
		[[alert window] setShowsResizeIndicator:YES];
//		[alert runModal];
		[alert beginSheetModalForWindow:(NSWindow*)(*wih)->theView modalDelegate:self
					  didEndSelector:nil contextInfo:nil];
		MetaDataDisplayStr = nil;
	}
}

- (void) SetWindowLayer:(NSWindowOrderingMode)pos relativeTo:(NSInteger)relTo
{ int i;
	for( i = 0 ; i < maxQTWM ; i++ ){
		if( i != sysWin && QTMWH(i) ){
			[ (NSWindow*)(*QTMWH(i))->theView orderWindow:pos relativeTo:relTo];
		}
	}
}

- (void) UpdateGeometryForWindow:(QTMovieWindowH)wih
{
	if( (*wih)->idx >= fwWin && (*wih)->idx < maxQTWM ){
		QTMovieWindowGetGeometry( wih, &Wpos[(*wih)->idx], &Wsize[(*wih)->idx], 1 );
	}
}

- (double) frameRate:(BOOL)forTC
{ int w;
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && QTMWH(w) ){
		  double f = (forTC)? (*QTMWH(w))->info->TCframeRate : (*QTMWH(w))->info->frameRate;
			return f;
		}
	}
	return 0.0/0.0;
}

- (double) duration
{ int w;
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && QTMWH(w) ){
			return (*QTMWH(w))->info->duration;
		}
	}
	return 0.0/0.0;
}

- (double) startTime
{ int w;
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && QTMWH(w) ){
			return (*QTMWH(w))->info->startTime;
		}
	}
	return 0.0/0.0;
}

- (double) getTime:(BOOL)absolute
{ int w;
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && QTMWH(w) ){
		  double t;
			QTMovieWindowGetTime( QTMWH(w), &t, absolute );
			return t;
		}
	}
	return 0.0/0.0;
}

- (void) SetTimes:(double)t withRefWindow:(QTMovieWindowH)ref absolute:(BOOL)absolute
{ int w;
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && QTMWH(w) != ref ){
			QTMovieWindowSetTime( QTMWH(w), t, absolute );
			[WINLIST(w) UpdateDrawer];
		}
	}
}

- (void) SetTimes:(double)t rates:(float)rate withRefWindow:(QTMovieWindowH)ref absolute:(BOOL)absolute
{ int w;
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && QTMWH(w) != ref ){
		  QTMovie *movie = [WINLIST(w) getMovie];
			QTMovieWindowSetTime( QTMWH(w), t, absolute );
			if( [movie rate] > 1e-5 ){
				[movie setRate:rate];
			}
			[movie setAttribute:[NSNumber numberWithFloat:rate] forKey:QTMoviePreferredRateAttribute];
			[WINLIST(w) UpdateDrawer];
		}
	}
}

- (void) StartVideoExceptFor:(QTVODWindow*)excl
{ int w;
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && WINLIST(w) != excl ){
			[[WINLIST(w) theMovieView] play:WINLIST(w)];
			WINLIST(w)->Playing = YES;
			[WINLIST(w) UpdateDrawer];
			[WINLIST(w) updateMenus];
		}
	}
}

- (void) StopVideoExceptFor:(QTVODWindow*)excl
{ int w;
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && WINLIST(w) != excl ){
			[[WINLIST(w) theMovieView] pause:WINLIST(w)];
			WINLIST(w)->Playing = NO;
			[WINLIST(w) UpdateDrawer];
			[WINLIST(w) updateMenus];
		}
	}
}

- (void) StepForwardExceptFor:(QTMovieWindowH)excl
{ int w;
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && QTMWH(w) != excl ){
			[[WINLIST(w) theMovieView] stepForward:WINLIST(w)];
			[WINLIST(w) UpdateDrawer];
		}
	}
}

- (void) StepBackwardExceptFor:(QTMovieWindowH)excl
{ int w;
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && QTMWH(w) != excl ){
			[[WINLIST(w) theMovieView] stepBackward:WINLIST(w)];
			[WINLIST(w) UpdateDrawer];
		}
	}
}

- (BOOL) CalcTimeInterval:(BOOL)display reset:(BOOL)reset
{ BOOL ret = NO;
  QTMovieWindowH wih = (TC)? TC.qtmwH : NULL;
  double t;
  char *foundText = NULL;
	if( wih && (*wih)->self == *wih && (*wih)->theView && QTMovieWindowGetTime(wih, &t, 0) == noErr ){
		if( theTimeInterval.timeA < 0.0 || reset ){
			theTimeInterval.timeA = t;
			FindTimeStampInMovieAtTime( (*wih)->theMovie, t, &foundText, NULL );
			if( foundText ){
				strncpy( theTimeInterval.timeStampA, foundText, sizeof(theTimeInterval.timeStampA) );
				free(foundText);
			}
			else{
				theTimeInterval.timeStampA[0] = '\0';
			}
			theTimeInterval.timeB = -1.0;
		}
		else{
			theTimeInterval.timeB = t;
			FindTimeStampInMovieAtTime( (*wih)->theMovie, t, &foundText, NULL );
			if( foundText ){
				strncpy( theTimeInterval.timeStampB, foundText, sizeof(theTimeInterval.timeStampB) );
				free(foundText);
			}
			else{
				theTimeInterval.timeStampB[0] = '\0';
			}
			theTimeInterval.dt = theTimeInterval.timeB - theTimeInterval.timeA;
			if( display ){
			  NSAlert* alert = [[[NSAlert alloc] init] autorelease];
			  NSString *msg = [NSString stringWithFormat:
						    @"A: t=%gs, '%s'\n"
						    "B: t=%gs, '%s'\n"
						    "\nB-A = %gs\n",
						    theTimeInterval.timeA, theTimeInterval.timeStampA,
						    theTimeInterval.timeB, theTimeInterval.timeStampB,
						    theTimeInterval.dt
				];
				[alert setAlertStyle:NSInformationalAlertStyle];
				[alert setMessageText:@""];
				[alert setInformativeText:msg];
				[[alert window] setTitle:@"Time interval"];
				[alert runModal];
			}
			theTimeInterval.timeA = theTimeInterval.timeB;
			strcpy( theTimeInterval.timeStampA, theTimeInterval.timeStampB );
			theTimeInterval.timeB = -1.0;
			ret = YES;
		}
	}
	return ret;
}

- (void) PlaceWindows:(Cartesian*)ulCorner withScale:(double)scale
{ int w;
  Cartesian pos;
	if( scale != 1.0 && scale > 0.0 ){
		for( w = 0 ; w <  maxQTWM ; w++ ){
			if( w != tcWin && w != sysWin && QTMovieWindowH_Check(QTMWH(w)) ){
				QTMovieWindowSetGeometry( QTMWH(w), NULL, NULL, scale, 0 );
				QTMovieWindowGetGeometry( QTMWH(w), &Wpos[w], &Wsize[w], 1 );
			}
		}
	}
	if( ulCorner ){
		ULCorner = *ulCorner;
	}

	/* vue pilote en haut et au milieu */
	pos.horizontal = ULCorner.horizontal + Wsize[pilotWin].horizontal;
	pos.vertical = ULCorner.vertical;
	if( pilot ){
		QTMovieWindowSetGeometry( pilot.qtmwH, &pos, NULL, 1.0, 1 );
	}

	/* vue lat-gauche en 2e ligne, à gauche */
	pos.horizontal = ULCorner.horizontal;
	pos.vertical = ULCorner.vertical + Wsize[pilotWin].vertical;
	if( left ){
		QTMovieWindowSetGeometry( left.qtmwH, &pos, NULL, 1.0, 1 );
	}

	/* vue vers l'avant en 2e ligne, au milieu, sous la vue pilote */
	pos.horizontal = pos.horizontal + Wsize[leftWin].horizontal;
	if( forward ){
		QTMovieWindowSetGeometry( forward.qtmwH, &pos, NULL, 1.0, 1 );
	}

	/* vue lat-droite en 2e ligne, à droite */
	pos.horizontal = pos.horizontal + Wsize[fwWin].horizontal;
	if( right ){
		QTMovieWindowSetGeometry( right.qtmwH, &pos, NULL, 1.0, 1 );
	}

	/* TimeCode en 3e ligne, centrée */
	pos.horizontal = ULCorner.horizontal + (Wsize[leftWin].horizontal + Wsize[pilotWin].horizontal/2) - Wsize[tcWin].horizontal/2;
	pos.vertical = ULCorner.vertical + Wsize[pilotWin].vertical + Wsize[leftWin].vertical;
	if( pos.horizontal < 0 ){
		pos.horizontal = 0;
	}
	if( TC ){
		QTMovieWindowSetGeometry( TC.qtmwH, &pos, NULL, 1.0, 1 );
	}
}

- (void) PlaceWindows:(Cartesian*)ulCorner withScale:(double)scale withNSSize:(NSSize)nsSize
{ Cartesian size;
  int w;
	size.horizontal = (short)(nsSize.width + 0.5);
	size.vertical = (short)(nsSize.height + 0.5);
	for( w = 0 ; w <  maxQTWM ; w++ ){
		if( w != tcWin && w != sysWin && QTMovieWindowH_Check(QTMWH(w)) ){
			QTMovieWindowSetGeometry( QTMWH(w), NULL, &size, scale, 0 );
			QTMovieWindowGetGeometry( QTMWH(w), &Wpos[w], &Wsize[w], 1 );
		}
	}
	return [self PlaceWindows:ulCorner withScale:1.0];
}

- (void) CreateQI2MFromDesign
{ struct dxyArray {
	double dx, dy;
  } chPiTrans[5] = { {0,0}, {1.0,0.0}, /*ok*/{0.0,0.0}, /*ok*/{1.0,-1.0}, {0.0,-1.0} },
  chLeTrans[5] = { {0,0}, /*ok*/{0.0,1.0}, {-1.0,1.0}, /*ok*/{0.0,0.0}, {-1.0,0.0} },
  chFwTrans[5] = { {0,0}, /*ok*/{1.0,1.0}, /*ok*/{0.0,1.0}, {1.0,0.0}, {-1.0,0.0} },
  chRiTrans[5] = { {0,0}, {2.0,1.0}, {1.0,1.0}, {2.0,0.0}, /*ok*/{1.0,0.0} };
  NSString *fn, *dn = [NSString stringWithFormat:@"%@vid", [theURL path] ];
  NSString *vodsource = [NSString stringWithFormat:@"%@.VOD", [theURL path] ];
  NSError *derr = NULL;
  const char *fName, *source = [vodsource UTF8String], *useVMGI;
  FILE *fp;
	if( [[[[NSFileManager alloc] init] autorelease]
			createDirectoryAtPath:dn withIntermediateDirectories:YES attributes:nil error:&derr]
	){
		fn = [NSString stringWithFormat:@"%@vid/design.qi2m", [theURL path] ];
	}
	else{
		// fall back on the old approach of creating the cache files alongside the input
		fn = [NSString stringWithFormat:@"%@-design.qi2m", [theURL path] ];
	}
	fName = [fn UTF8String];
	if( (fp = fopen(fName, "w")) ){
	  char *flipLaterals = (theDescription.flipLeftRight)? "True" : "False";
		if( theDescription.useVMGI ){
			useVMGI = "True";
		}
		else{
			useVMGI = "False";
		}

		fputs( "<?xml version=\"1.0\"?>\n", fp );
		fputs( "<?quicktime type=\"video/x-qt-img2mov\"?>\n", fp );
		// autosave will take care of generating the cache movie:
		fputs( "<import autoSave=True >\n", fp );
		if( assocDataFile ){
			fprintf( fp, "\t<description txt=\"UTC timeZone=%g, DST=%hd, assoc.data:%s\" />\n",
				   theDescription.timeZone, (short) theDescription.DST, [assocDataFile UTF8String]
			);
		}
		else{
			fprintf( fp, "\t<description txt=\"UTC timeZone=%g, DST=%hd\" />\n",
				   theDescription.timeZone, (short) theDescription.DST
			);
		}
		fprintf( fp,
			"\t<sequence src=\"%s\" freq=%g channel=%d timepad=False hflip=%s vmgi=%s\n"
			"\t\trelTransH=%g relTransV=%g newchapter=True description=\"%s\" />\n",
			source, theDescription.frequency, 6, "False", useVMGI, 0.5, 0.0, "timeStamps"
		);
		fprintf( fp,
			"\t<sequence src=\"%s\" freq=%g channel=%d timepad=False hflip=%s vmgi=%s addtc=False hidets=True\n"
			"\t\trelTransH=%g relTransV=%g description=\"%s\" />\n",
			source, theDescription.frequency, theDescription.channels.pilot, "False", useVMGI,
			chPiTrans[theDescription.channels.pilot].dx, chPiTrans[theDescription.channels.pilot].dy, "pilot"
		);
		fprintf( fp,
			"\t<sequence src=\"%s\" freq=%g channel=%d timepad=False hflip=%s vmgi=%s addtc=False hidets=True\n"
			"\t\trelTransH=%g relTransV=%g description=\"%s\" />\n",
			source, theDescription.frequency, theDescription.channels.left, flipLaterals, useVMGI,
			chLeTrans[theDescription.channels.left].dx, chLeTrans[theDescription.channels.left].dy, "left"
		);
		fprintf( fp,
			"\t<sequence src=\"%s\" freq=%g channel=%d timepad=False hflip=%s vmgi=%s addtc=False hidets=True\n"
			"\t\trelTransH=%g relTransV=%g description=\"%s\" />\n",
			source, theDescription.frequency, theDescription.channels.right, flipLaterals, useVMGI,
			chRiTrans[theDescription.channels.right].dx, chRiTrans[theDescription.channels.right].dy, "right"
		);
		fprintf( fp,
			"\t<sequence src=\"%s\" freq=%g channel=%d timepad=False hflip=%s vmgi=%s addtc=False hidets=True\n"
			"\t\trelTransH=%g relTransV=%g description=\"%s\" />\n",
			source, theDescription.frequency, theDescription.channels.forward, "False", useVMGI,
			chFwTrans[theDescription.channels.forward].dx, chFwTrans[theDescription.channels.forward].dy, "forward"
		);
		fputs( "</import>\n", fp );
		fclose(fp);
	}
}

// the preferred initialisation method:
- (BOOL) readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError
{
	if( absoluteURL ){
		// sic:
		[self setFileURL:theURL];
		theURL = [ pruneExtensions(absoluteURL) retain];
	}
	return ([self OpenVideo:typeName error:outError] == noErr)? YES : NO;
}

- (ErrCode) OpenVideo:(NSString *)typeName error:(NSError **)outError
{ ErrCode err = noErr;
  char *fName = NULL;
  NSString *fn = nil;
  static BOOL inOpenVideo = NO;

	if( !theURL || ![[theURL path] length] ){
		// present an open file dialog
		fName = AskFileName( "Please choose a video or movie to open" );
		if( fName && *fName ){
			// store the filename
			if( theURL ){
				[theURL release];
			}
			theURL = [pruneExtensions([NSURL fileURLWithPath:[NSString stringWithUTF8String:fName]]) retain];
		}
	}

	if( globalVDPreferences.changed ){
		theDescription = globalVDPreferences;
		theDescription.changed = NO;
		recreateChannelViews = YES;
	}
	else{
	  ErrCode e1, e2, e3;
		// attempt to read the "global" prefs file
		e1 = [self ReadDefaultVODDescription:"VODdesign.xml" toDescription:&theDescription];
		// find a local prefs file applying to a collection of recordings, starting from the
		// directory the video lives in and working upwards
		e2 = [self ScanForDefaultVODDescription:"VODdesign.xml" toDescription:&theDescription];
		// check if there is a recording-specific prefs file:
		e3 = [self nsReadDefaultVODDescription:[NSString stringWithFormat:@"%@-design.xml", [theURL path]] toDescription:&theDescription];
		if( e1 == noErr || e2 == noErr || e3 == noErr ){
			globalVDPreferences = theDescription;
			UpdateVDPrefsWin(TRUE);
		}
		recreateChannelViews = NO;
	}
//	{ NSVODDescription *nsvd = [NSVODDescription createWithDescription:&theDescription];
//		fprintf( stderr, "Allocated NSVODDescription 0x%p; should be released immediately after this statement\n", nsvd );
//		// but typically it'll be upon the exit from this function...
//	}
	if( assocDataFile && [assocDataFile compare:@"*FromVODFile*"] == NSOrderedSame ){
	  NSString *adfn = [NSString stringWithFormat:@"%@.IEF", [theURL path]];
	  NSFileManager *nfs = [[[NSFileManager alloc] init] autorelease];
		if( [nfs isReadableFileAtPath:adfn] ){
			[assocDataFile release];
			assocDataFile = [adfn retain];
		}
	}
	fn = [NSString stringWithFormat:@"%@.mov", [theURL path]];
	fName = (char*) [fn UTF8String];
	QTils_LogMsgEx( "Trying video cache: OpenMovieFromURL(%s)", fName );
	if( !recreateChannelViews && (err = OpenMovieFromURL( &fullMovie, 1, NULL, fName, NULL, NULL )) == noErr ){
		QTils_LogMsg( "all-channel cache movie opened" );
	}
	else{
		// make movie cache and call ImportMovie
		fn = [NSString stringWithFormat:@"%@.VOD", [theURL path]];
		fName = (char*) [fn UTF8String];
		if( theDescription.channels.forward <= 0 && theDescription.channels.pilot <= 0
		   && theDescription.channels.left <= 0 && theDescription.channels.right <= 0
		){
			QTils_LogMsgEx( "Trying video source: OpenMovieFromURL(%s)", fName );
			if( (err = OpenMovieFromURL( &fullMovie, 1, NULL, fName, NULL, NULL )) == noErr ){
				// succeeded in importing the source .VOD file with the default parameters
				fn = [NSString stringWithFormat:@"%@.mov", [theURL path]];
				fName = (char*) [fn UTF8String];
				err = SaveMovieAsRefMov( fName, fullMovie );
				CloseMovie(&fullMovie);
				if( err == noErr ){
					QTils_LogMsgEx( "Created cache movie '%s'", fName );
					err = OpenMovieFromURL( &fullMovie, 1, NULL, fName, NULL, NULL );
				}
				else{
					QTils_LogMsgEx( "Error in SaveMovieAsRefMov(): %d", err );
					PostMessage( fName, lastSSLogMsg );
				}
			}
		}
		else{
			err = [self ImportMovie:theURL withDescription:&theDescription];
			if( err != noErr ){
			  const char *es, *ec;
				es = MacErrorString( LastQTError(), &ec );
				if( es ){
					QTils_LogMsgEx( "Error in ImportMovie: %s (%s)\n"
								"Maybe the recording exists neither in .mov nor in .VOD?!\n", es, ec );
				}
				else{
					QTils_LogMsgEx( "Error in ImportMovie: %d (%d)\n"
								"Maybe the recording exists neither in .mov nor in .VOD?!\n", LastQTError(), err );
				}
				PostMessage( fName, lastSSLogMsg );
			}
		}
		if( !fullMovie ){
			PostMessage( [[theURL path] UTF8String], "No access to the principal video cache file" );
		}
		else{
			recreateChannelViews = YES;
		}
	}
	if( err != noErr ){
		if( err == couldNotResolveDataRef && !inOpenVideo ){
			inOpenVideo = YES;
			[theURL release];
			theURL = nil;
			err = [self OpenVideo:typeName error:outError];
			inOpenVideo = NO;
			return err;
		}
		else{
			return err;
		}
	}

	// now, try to open the 5 views:
	if( doLogging ){
		// sequential opening on the main thread
		forward = [self CreateChannelView:theURL withDescription:&theDescription
						  channel:theDescription.channels.forward channelName:"forward"
						  scale:theDescription.scale display:YES ofType:typeName error:outError eMsg:NULL];
		pilot = [self CreateChannelView:theURL withDescription:&theDescription
						  channel:theDescription.channels.pilot channelName:"pilot"
						  scale:theDescription.scale display:YES ofType:typeName error:outError eMsg:NULL];
		left = [self CreateChannelView:theURL withDescription:&theDescription
						  channel:theDescription.channels.left channelName:"left"
						  scale:theDescription.scale display:YES ofType:typeName error:outError eMsg:NULL];
		right = [self CreateChannelView:theURL withDescription:&theDescription
						  channel:theDescription.channels.right channelName:"right"
						  scale:theDescription.scale display:YES ofType:typeName error:outError eMsg:NULL];
		TC = [self CreateChannelView:theURL withDescription:&theDescription
						  channel:6 channelName:"TC"
						  scale:1.0 display:YES ofType:typeName error:outError eMsg:NULL];
	}
	else{
		// partially parallel opening using background threads:
	  ChannelViewSpec *chSpec[5] = {
			[[ChannelViewSpec createWithURL:theURL forChannel:fwWin withName:"forward"
						 withDescription:NULL ofType:typeName error:outError ] retain],
			[[ChannelViewSpec createWithURL:theURL forChannel:pilotWin withName:"pilot"
						 withDescription:NULL ofType:typeName error:outError ] retain],
			[[ChannelViewSpec createWithURL:theURL forChannel:leftWin withName:"left"
						 withDescription:NULL ofType:typeName error:outError ] retain],
			[[ChannelViewSpec createWithURL:theURL forChannel:rightWin withName:"right"
						 withDescription:NULL ofType:typeName error:outError ] retain],
			[[ChannelViewSpec createWithURL:theURL forChannel:tcWin withName:"TC"
						 withDescription:NULL ofType:typeName error:outError ] retain] };
	  const char *teMsg[] = { "Forward view", "Pilot view", "Left view", "Right view", "TC view" };
		[NSThread detachNewThreadSelector:@selector(CreateChannelViewInBackground:) toTarget:self withObject:chSpec[0]];
		[NSThread detachNewThreadSelector:@selector(CreateChannelViewInBackground:) toTarget:self withObject:chSpec[1]];
		[NSThread detachNewThreadSelector:@selector(CreateChannelViewInBackground:) toTarget:self withObject:chSpec[2]];
		[NSThread detachNewThreadSelector:@selector(CreateChannelViewInBackground:) toTarget:self withObject:chSpec[3]];
		[NSThread detachNewThreadSelector:@selector(CreateChannelViewInBackground:) toTarget:self withObject:chSpec[4]];

		int loops = 0;
		do{
		  int cs;
			for( cs = 0 ; cs < 5 ; cs++ ){
				if( chSpec[cs] ){
				  BOOL done = NO;
					@synchronized(chSpec[cs]){
						if( chSpec[cs]->done ){
							if( chSpec[cs]->movieCache ){
								*winlist[cs] = [self OpenChannelView:chSpec[cs]->movieCache
											    forChannelNr:chSpec[cs]->channelNr withName:chSpec[cs]->channelName];
								NSLog( @"Opened window %@ for %@", *winlist[cs], chSpec[cs]->movieCache );
							}
							else if( chSpec[cs]->eMsg[0] ){
								PostMessage( teMsg[cs], chSpec[cs]->eMsg );
							}
							done = YES;
						}
					}
					if( done ){
						[chSpec[cs] release];
						chSpec[cs] = nil;
					}
				}
			}
			loops += 1;
		} while( chSpec[0] || chSpec[1] || chSpec[2] || chSpec[3] || chSpec[4] );
		if( loops > 1 ){
			doNSLog(@"%@(%@) Looped %d times to open all 5 windows", self, theURL, loops );
		}
	}

	[self register_windows];
	[self PlaceWindows:nil withScale:1.0];

	if( err == noErr ){
		err = LastQTError();
	}

	if( err == noErr ){
		[self CreateQI2MFromDesign];
	}
	return err;
}

- (void) CloseVideo:(BOOL)final
{ int w;
  BOOL addRecent = addToRecentDocs, fmSaved = NO;
  NSString *fNames[maxQTWM];
	finalCloseVideo = final;
	if( fullMovie && final ){
		if( fullMovieChanged || HasMovieChanged(fullMovie) ){
		  QTMovie *fm = [QTMovie movieWithQuickTimeMovie:fullMovie disposeWhenDone:YES error:nil];
			if( PostYesNoDialog( [[fm attributeForKey:QTMovieFileNameAttribute] cStringUsingEncoding:NSUTF8StringEncoding],
							"This recording's cache movie has changed, do you want to save it?")
			){
				if( SaveMovie(fullMovie) == noErr ){
					NSLog( @"Saved modified '%@'",
						 [fm attributeForKey:QTMovieFileNameAttribute]
					);
					fmSaved = YES;
					ClearMovieChanged(fullMovie);
					fullMovieChanged = NO;
				}
			}
			// we've transferred ownership of fullMovie to 'fm', so here we only trash the reference to it:
			fullMovie = NULL;
			[sysOwned updateChangeCount:NSChangeCleared];
		}
		else{
			CloseMovie(&fullMovie);
		}
	}
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && QTMWH(w) && (w != sysWin || final) ){
			if( w == sysWin ){
				addToRecentDocs = YES;
				WINLIST(w).addToRecentMenu = 1;
				fNames[w] = nil;
			}
			else{
				addToRecentDocs = NO;
				WINLIST(w).addToRecentMenu = 0;
				fNames[w] = [[NSString stringWithFormat:@"%@", [[WINLIST(w) theURL] path]] retain];
			}
			if( !(*QTMWH(w))->performingClose ){
				[[[WINLIST(w) getView] window] performClose:[[WINLIST(w) getView] window]];
			}
			if( WINLIST(w) ){
				if( QTMWH(w) ){
					if( WINLIST(w).delayedClosing ){
						(*QTMWH(w))->user_data = NULL;
					}
					else if( !(*QTMWH(w))->performingClose ){
						[WINLIST(w) close];
					}
				}
				*winlist[w] = NULL;
				winlist[w] = NULL;
			}
			addToRecentDocs = addRecent;
		}
		else{
			fNames[w] = nil;
		}
	}
	if( fmSaved ){
		for( w = 0 ; w < maxQTWM ; w++ ){
			if( fNames[w] && (!winlist[w] || !*winlist[w]) ){
				unlink( [fNames[w] UTF8String] );
				[fNames[w] release];
			}
		}
	}
	if( numQTMW <= 0 && final ){
		if( sysOwned ){
			[sysOwned setQTVOD:nil];
			sysOwned = nil;
		}
	}
}

- (ErrCode) ResetVideo:(BOOL)complete closeSysWin:(BOOL)closeSysWin
{ ErrCode err = noErr;
  NSURL *baseURL = [theURL retain];
  double currentTime = [self getTime:NO];
  int w;
  NSString *fNames[maxQTWM];
  QTVODWindow *sO = sysOwned;
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( WINLIST(w) && QTMWH(w) && w != sysWin ){
			fNames[w] = [NSString stringWithFormat:@"%@", [[WINLIST(w) theURL] path]];
		}
		else{
			fNames[w] = nil;
		}
	}
	// protect the sysWindow:
	sysOwned = nil;
	[self CloseVideo:(complete || closeSysWin)];
	for( w = 0 ; w < maxQTWM ; w++ ){
		if( fNames[w] ){
			unlink( [fNames[w] UTF8String] );
		}
	}
	if( fullMovie && (fullMovieChanged || HasMovieChanged(fullMovie)) ){
	  QTMovie *fm = [QTMovie movieWithQuickTimeMovie:fullMovie disposeWhenDone:NO error:nil];
		if( SaveMovie(fullMovie) == noErr ){
			NSLog( @"Saved modified '%@'",
				 [fm attributeForKey:QTMovieFileNameAttribute]
			);
			ClearMovieChanged(fullMovie);
			fullMovieChanged = NO;
		}
		else{
		  const char *es, *ec, *fName = [[fm attributeForKey:QTMovieFileNameAttribute] UTF8String];
			es = MacErrorString( LastQTError(), &ec );
			if( es ){
				QTils_LogMsgEx( "Save failure for '%s': %s (%s)", fName, es, ec );
			}
			else{
				QTils_LogMsgEx( "Save failure for '%s': %d (%d)", fName, LastQTError(), err );
			}
			PostMessage( fName, lastSSLogMsg );
		}
		[sO updateChangeCount:NSChangeCleared];
	}
	// fixme:
//	if( complete ){
//	  NSString *fName = [NSString stringWithFormat:@"%@.mov", [baseURL path]];
//		if( fullMovie ){
//			CloseMovie(&fullMovie);
//		}
//		unlink( [fName UTF8String] );
//	}
	[theURL release];
	winlist[fwWin] = &forward;
	winlist[pilotWin] = &pilot;
	winlist[leftWin] = &left;
	winlist[rightWin] = &right;
	winlist[tcWin] = &TC;
	winlist[sysWin] = &sysOwned;
	sysOwned = sO;
	theURL = baseURL;
	if( [self readFromURL:nil ofType:@"" error:nil] == NO ){
		err = 1;
	}
	else{
		[self SetTimes:currentTime withRefWindow:nil absolute:NO];
	}
	return err;
}

- (ErrCode) ResetVideo:(BOOL)complete
{
	return [self ResetVideo:complete closeSysWin:complete];
}

/*!
	QTVOD method to read a default VODDescription from the named file, storing it in the designated variable
 */
- (ErrCode) ReadDefaultVODDescription:(const char*)fName toDescription:(VODDescription*)descr
{ ErrCode xmlErr = noErr;
  int errors = 0;
  XMLDoc xmldoc = NULL;
  NSMutableString *errDescr = nil;

	if( !xmlParser ){
		xmlErr = CreateXMLParser( &xmlParser, xml_design_parser, sizeof(xml_design_parser)/sizeof(XML_Record), &errors );
	}
	if( errors == 0 ){
		lastSSLogMsg[0] = '\0';
		xmlErr = ParseXMLFile( fName, xmlParser,
						  xmlParseFlagAllowUppercase|xmlParseFlagAllowUnquotedAttributeValues|elementFlagPreserveWhiteSpace,
						  &xmldoc );
		if( xmlErr != noErr ){
			errDescr = [NSMutableString stringWithUTF8String:&lastSSLogMsg[0]];
			if( xmlErr == couldNotResolveDataRef ){
				[errDescr appendString:@" (file does not exist)"];
			}
			else if( lastSSLogMsg[0] ){
				PostMessage( "QTVODosx", [errDescr UTF8String] );
			}
		}
		else{
			if( XMLRootElementID(xmldoc) == element_vodDesign ){
				QTils_LogMsgEx( "Reading VOD parameters from '%s'", fName );
				ReadXMLDoc( fName, xmldoc, descr );
			}
			else{
				QTils_LogMsgEx( "'%s' is valid XML but lacking root element '%s'",
							fName, xml_design_parser[0].xml.element.Tag
				);
			}
		}
	}
	if( xmldoc ){
		DisposeXMLParser( &xmlParser, &xmldoc, 0 );
	}
	return xmlErr;
}

- (ErrCode) nsReadDefaultVODDescription:(NSString*)fName toDescription:(VODDescription*)descr
{
	return [self ReadDefaultVODDescription:[fName UTF8String] toDescription:descr];
}

- (ErrCode) urlReadDefaultVODDescription:(NSURL*)url toDescription:(VODDescription*)descr
{
	return [self nsReadDefaultVODDescription:[url path] toDescription:descr];
}

- (ErrCode) ScanForDefaultVODDescription:(const char*)fName toDescription:(VODDescription*)descr
{ ErrCode err = noErr;
  NSArray *dirs = [theURL pathComponents];
  NSRange searchpath;
  BOOL done = NO;
	// dirs contains all the elements of the path, i.e. including the base filename. We're
	// interested in the directory hierarchy:
	searchpath.location = 0;
	searchpath.length = [dirs count] - 1;
	theDirPath = [[NSString pathWithComponents:[dirs subarrayWithRange:searchpath]] retain];
	while( !done ){
	  NSMutableString *fn = [[[NSMutableString alloc] init] autorelease];
		[fn setString:[NSString pathWithComponents:[dirs subarrayWithRange:searchpath]]];
		[fn appendFormat:@"/%s", fName];
		err = [self ReadDefaultVODDescription:[fn UTF8String] toDescription:descr];
		if( err != couldNotResolveDataRef || searchpath.length == 1 ){
			done = YES;
		}
		else{
			searchpath.length -= 1;
		}
	}
	return err;
}

- (ErrCode) nsScanForDefaultVODDescription:(NSString*)fName toDescription:(VODDescription*)descr
{
	return [self ScanForDefaultVODDescription:[fName UTF8String] toDescription:descr];
}

@synthesize sysOwned, forward, pilot, left, right, TC;
@synthesize numQTMW;
@synthesize fullMovie;
@synthesize xmlParser;
@synthesize theURL;
@synthesize fullMovieChanged, shouldBeClosed;
@synthesize assocDataFile;
@synthesize theTimeInterval;
@end
