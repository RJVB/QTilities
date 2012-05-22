/*!
 *  @file QTMovieWin.h
 *  QTilities
 *
 *  Created by René J.V. Bertin on 20101130.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 *  This file provides a "hidden public" interface to a certain number of functions and typedefs
 *  that are not usually required when calling into libQTils/QTils.dll/QTils.framework from C/C++
 *  Its primary reason of existence is the Modula-2 interface on MS Windows.
 *  DO NOT INCLUDE this headerfile in C or C++ code unless you know what you are doing!
 *
 */

#ifndef _QTMOVIEWIN_H

#define AllowQTMLDoubleBuffering

#ifdef __cplusplus
extern "C"
{
#endif

extern int UseQTMLDoubleBuffering;

/*!
	the QuickTime actions we support with callback routines. An instance of this structure
	(_MCAction_) is initialised at startup, and can be queried via MCAction().
 */
typedef struct AnnotationKeyList {
	unsigned long akAuthor, akComment, akCopyRight, akDisplayName,
	akInfo, akKeywords, akDescr, akFormat, akSource,
	akSoftware, akWriter, akYear, akCreationDate, akTrack;
} AnnotationKeyList;

/*!
	initialises dmbase with the appropriate Modula-2 wrapper functions
	and returns the size of the structure
 */
QTLSext size_t initDMBaseQTils_Mod2( LibQTilsBase *dmbase );
/*!
	exports the various metadata annotation keys to Modula-2
 */
QTLSext size_t ExportAnnotationKeys( AnnotationKeyList *list );

//extern void GetMaxBounds(Rect *maxRect);
extern void CreateNewMovieController( QTMovieWindowH wih, int controllerVisible );

extern QTMovieWindowH lastQTWMH;
extern const MCActions _MCAction_;

QTLSext void QTWMflush();

QTLSext long GetMovieTrackCount_Mod2(Movie theMovie);
QTLSext ErrCode AddMetaDataStringToTrack_Mod2( Movie theMovie, long theTrack,
							AnnotationKeys key,
							const char *value, int vlen, const char *lang, int llen );
QTLSext ErrCode AddMetaDataStringToMovie_Mod2( Movie theMovie, AnnotationKeys key,
							const char *value, int vlen, const char *lang, int llen );
QTLSext ErrCode GetMetaDataStringLengthFromTrack_Mod2( Movie theMovie, long theTrack,
							AnnotationKeys key, size_t *len );
QTLSext ErrCode GetMetaDataStringLengthFromMovie_Mod2( Movie theMovie, AnnotationKeys key,
							size_t *len );
QTLSext ErrCode GetMetaDataStringFromTrack_Mod2( Movie theMovie, long theTrack,
							AnnotationKeys key,
							char *value, int vlen, char *lang, int llen );
QTLSext ErrCode GetMetaDataStringFromMovie_Mod2( Movie theMovie, AnnotationKeys key,
							char *value, int vlen, char *lang, int llen );

QTLSext ErrCode OpenMovieFromURL_Mod2( Movie *newMovie, short flags, char *URL, int ulen );
QTLSext ErrCode SaveMovieAsRefMov_Mod2( const char *dstURL, int ulen, Movie theMovie );
QTLSext ErrCode SaveMovieAs_Mod2( char *fname, int flen, Movie theMovie, int noDialog );
QTLSext size_t QTils_LogMsg_Mod2( const char *msg, int mlen );
QTLSext size_t QTils_LogMsgEx_Mod2( const char *msg, int mlen, va_list ap );
QTLSext QTMovieWindowH NewQTMovieWindowH();
QTLSext QTMovieWindowH InitQTMovieWindowHFromMovie( QTMovieWindowH wih, const char *theURL, Movie theMovie,
								  Handle dataRef, OSType dataRefType, DataHandler dh, short resId, ErrCode *err );
QTLSext void DisposeQTMovieWindow( QTMovieWindowH WI );
QTLSext void DisposeQTMovieWindow_Mod2( QTMovieWindowH *WI );
QTLSext QTMovieWindowH OpenQTMovieInWindow_Mod2( const char *theURL, int ulen, int withController );

QTLSext ErrCode Check4XMLError_Mod2( ComponentInstance xmlParser, ErrCode err, const char *theURL,
						int ulen, unsigned char *descr, int dlen );
QTLSext ErrCode ParseXMLFile_Mod2( const char *theURL, int ulen, ComponentInstance xmlParser,
						long flags, XMLDoc *document );
QTLSext ErrCode XMLParserAddElement_Mod2( ComponentInstance *xmlParser, const char *elementName, int elen,
						unsigned int elementID, long elementFlags );
QTLSext ErrCode XMLParserAddElementAttribute_Mod2( ComponentInstance *xmlParser, unsigned int elementID,
						const char *attrName, int alen, unsigned int attrID, unsigned int attrType );
QTLSext ErrCode GetAttributeIndex_Mod2( XMLAttributePtr attributes, UInt32 attributeType, SInt32 *idx );
QTLSext ErrCode GetStringAttribute_Mod2( XMLElement *element, UInt32 attributeType,
						char *theString, int slen );

QTLSext int vsscanf_Mod2( const char *source, int slen, const char *format, int flen, va_list ap );
QTLSext int vsnprintf_Mod2( char *dest, int slen, const char *format, int flen, va_list ap );

#ifdef __cplusplus
}
#endif

#define _QTMOVIEWIN_H
#endif
