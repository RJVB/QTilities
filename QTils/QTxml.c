/*!
 *  @file QTxml.c
 *  QTilities
 *
 *  Created by René J.V. Bertin on 20110131.
 *  Copyright 2011 IFSTTAR / RJVB. All rights reserved.
 *  This file contains a number of useful QuickTime routines.
 *
 */

#include "winixdefs.h"
#include "copyright.h"
IDENTIFY("QTxml: interface to the QT XMLParser (mostly for Modula-2)" );

#include "Logging.h"

#define _QTILS_C

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if ! defined(_WINDOWS) && !defined(WIN32) && !defined(_MSC_VER)
#	include <unistd.h>
#endif


#if __APPLE_CC__
#	include <Carbon/Carbon.h>
#	include <QuickTime/QuickTime.h>
#else
#	include <ImageCodec.h>
#	include <TextUtils.h>
#	include <string.h>
#	include <Files.h>
#	include <Movies.h>
#	include <MediaHandlers.h>
#	include <QuickTimeComponents.h>
#	include <direct.h>
#endif

#include "NaN.h"
#include "QTilities.h"
#include "QTMovieWin.h"

#ifndef MIN
#	define	MIN(a,b) (((a)<(b))?(a):(b))
#endif /* MIN */

#undef POSIX_PATHNAMES

#if !(defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32) || defined(QTMOVIESINK)
extern char lastSSLogMsg[2048];
#endif // not on MSWin

ErrCode Check4XMLError( ComponentInstance xmlParser, ErrCode err, const char *theURL, Str255 descr )
{
	if( descr ){
		if( err == couldNotResolveDataRef ){
			snprintf( (char*)&descr[1], 254, "XML file not found" );
			descr[0] = (char) strlen((const char*)&descr[1]);
			Log( qtLogPtr, "Could not find/open XML file \"%s\" (error code %d)\n",
				theURL, (int) err
			);
		}
		else if( err != noErr ){
		  ComponentResult err2;
		  long line;
		  unsigned char len;
			descr[0] = 0;
			err2 = XMLParseGetDetailedParseError( xmlParser, &line, (StringPtr) descr );
			// desc will be a Pascal string; make it a C string:
			len = (unsigned char) descr[0];
			if( len > 0 ){
				memmove( descr, &descr[1], len );
				descr[len] = '\0';
			}
			Log( qtLogPtr, "Error parsing XML file \"%s\" at line %ld: \"%s\" (error code %d)\n",
				theURL, line, descr, (int) err
			);
		}
		else{
			descr[0] = '\0';
		}
	}
	return err;
}

ErrCode Check4XMLError_Mod2( ComponentInstance xmlParser, ErrCode err, const char *theURL,
				   int ulen, unsigned char *descr, int dlen )
{ Str255 ldescr;
	Check4XMLError( xmlParser, err, theURL, ldescr );
	strcpy( (char*)descr, (const char*)ldescr );
	descr[255] = '\0';
	return err;
}

// xmlParseFlagAllowUppercase|xmlParseFlagAllowUnquotedAttributeValues|elementFlagPreserveWhiteSpace

ErrCode ParseXMLFile( const char *theURL, ComponentInstance xmlParser, long flags, XMLDoc *document )
{ Handle dataRef = NULL;
  OSType dataRefType;
  ErrCode err;
  char *orgURL = (char*) theURL;
  Str255 errdescr;

	if( !xmlParser || !document ){
		return paramErr;
	}
	err = (ComponentResult) DataRefFromURL( (const char**) &orgURL, &dataRef, &dataRefType );
	if( err == noErr ){
		err = XMLParseDataRef(xmlParser, dataRef, dataRefType, flags, document );
		Check4XMLError( xmlParser, err, theURL, errdescr );
	}
	if( orgURL && theURL != orgURL ){
		QTils_free(&orgURL);
	}
	return err;
}

ErrCode ParseXMLFile_Mod2( const char *theURL, int ulen, ComponentInstance xmlParser, long flags, XMLDoc *document )
{
	return ParseXMLFile( theURL, xmlParser, flags, document );
}

ErrCode XMLParserAddElement( ComponentInstance *xmlParser, const char *elementName,
					   unsigned int elementID, unsigned int *namespaceID, long elementFlags )
{ UInt32 elID = elementID;
  ErrCode err;
	if( xmlParser && !*xmlParser ){
		*xmlParser = OpenDefaultComponent( xmlParseComponentType, xmlParseComponentSubType );
	}
	if( xmlParser && *xmlParser ){
		err = XMLParseAddElement( *xmlParser, (char*) elementName, (namespaceID)? *namespaceID : nameSpaceIDNone,
							&elID, elementFlags );
	}
	else{
		err = paramErr;
	}
	return err;
}

ErrCode XMLParserAddElement_Mod2( ComponentInstance *xmlParser, const char *elementName, int elen,
								unsigned int elementID, long elementFlags )
{ ErrCode err;
  extern LibQTilsBase *clientsDMBase;

	err = XMLParserAddElement( xmlParser, elementName, elementID,
						 (clientsDMBase)? (unsigned int*) &clientsDMBase->XMLnameSpaceID : NULL, elementFlags );
//	Log( qtLogPtr, "XMLParserAddElement(%p,\"%s\",%u,%u) -> %d",
//	    *xmlParser, elementName, elementID, elementFlags, err
//	);
	return err;
}

ErrCode XMLParserAddElementAttribute( ComponentInstance *xmlParser,
								unsigned int elementID, unsigned int *namespaceID,
								const char *attrName, unsigned int attrID, unsigned int attrType )
{ UInt32 atID = attrID;
  ErrCode err;
	if( xmlParser && *xmlParser ){
		err = XMLParseAddAttributeAndValue( *xmlParser, elementID, (namespaceID)? *namespaceID : nameSpaceIDNone,
									(char*) attrName, &atID, attrType, NULL
		);
	}
	else{
		err = paramErr;
	}
	return err;
}

ErrCode XMLParserAddElementAttribute_Mod2( ComponentInstance *xmlParser, unsigned int elementID,
					   const char *attrName, int alen, unsigned int attrID, unsigned int attrType )
{ ErrCode err;
  extern LibQTilsBase *clientsDMBase;
	err = XMLParserAddElementAttribute( xmlParser, elementID,
								(clientsDMBase)? (unsigned int*) &clientsDMBase->XMLnameSpaceID : NULL, attrName, attrID, attrType );
//	Log( qtLogPtr, "XMLParserAddElementAttribute(%p,%u,\"%s\",%u,%u) -> %d",
//	    *xmlParser, elementID, attrName, attrID, attrType, err
//	);
	return err;
}

ErrCode CreateXMLParser( ComponentInstance *xmlParser, XML_Record *xml_design, unsigned int N, int *errors )
{ ErrCode xmlErr = noErr;
  unsigned int i, lastElement, type;

	*errors = 0;
	lastElement = 0;

	for( i = 0 ; i < N ; i++ ){
		switch( xml_design[i].itemType ){
			case xml_element :
				xmlErr = XMLParserAddElement( xmlParser,
					xml_design[i].xml.element.Tag,
					xml_design[i].xml.element.ID, NULL, 0
				);
				if( xmlErr == noErr ){
					lastElement = xml_design[i].xml.element.ID;
				}
				else{
					*errors += 1;
				}
				break;
			case xml_attribute :
				switch( xml_design[i].xml.attribute.Type ){
					case recordAttributeValueTypeCharString:
						type = attributeValueKindCharString;
						break;
					case recordAttributeValueTypeDouble:
						type = attributeValueKindDouble;
						break;
					case recordAttributeValueTypeInteger:
						type = attributeValueKindInteger;
						break;
					case recordAttributeValueTypePercent:
						type = attributeValueKindPercent;
						break;
					case recordAttributeValueTypeBoolean:
						type = attributeValueKindBoolean;
						break;
					case recordAttributeValueTypeOnOff:
						type = attributeValueKindOnOff;
						break;
					default:
						type = xml_design[i].xml.attribute.Type;
						break;
				}
				xmlErr = XMLParserAddElementAttribute( xmlParser, lastElement, NULL,
					xml_design[i].xml.attribute.Tag, xml_design[i].xml.attribute.ID,
					type );
				if( xmlErr != noErr ){
					Log( qtLogPtr, "Definition error for attribute #%d '%s' type %d of element #%d: err=%d",
						xml_design[i].xml.attribute.ID, xml_design[i].xml.attribute.Tag,
						xml_design[i].xml.attribute.Type, lastElement, xmlErr );
					*errors += 1;
				}
				break;
		}
	}
	return xmlErr;
}

ErrCode DisposeXMLParser( ComponentInstance *xmlParser, XMLDoc *xmldoc, int parserToo )
{ ErrCode err = paramErr;
	if( xmlParser && *xmlParser ){
		if( xmldoc && *xmldoc ){
			err = XMLParseDisposeXMLDoc( *xmlParser, *xmldoc );
			*xmldoc = NULL;
		}
		if( parserToo ){
			err = CloseComponent(*xmlParser);
			*xmlParser = NULL;
		}
	}
	return err;
}

/*!
	attempts to parse a single XML element from file fName based on the design table
 */
ErrCode ReadXMLElementAttributes( XMLElement *theElement, size_t elm, XML_Record *design, size_t designLength,
						   Boolean useHandler, const char *fName )
{ ErrCode xmlErr = noErr;
  size_t idx;
	if( theElement->identifier == xmlIdentifierUnrecognized ){
		if( theElement->name && theElement->attributes->name ){
			Log( qtLogPtr, ">> unknown element <%s %s /> found in '%s'",
						theElement->name, theElement->attributes->name, fName );
		}
		else{
			Log( qtLogPtr, ">> unknown element found in '%s'", fName );
		}
		return xmlErr;
	}
	for( idx = 0 ; idx < designLength ; ){
		if( design[idx].itemType == xml_element && theElement->identifier == design[idx].xml.element.ID ){
		  XMLContent *element_content = NULL;
		  SInt32 elem = 0;
			if( XMLContentOfElement( theElement, &element_content ) ){
			  size_t i = 0;
				ReadXMLContent( fName, element_content, design, designLength, &i );
			}
			idx += 1;
			if( design[idx].itemType == xml_attribute ){
				while( design[idx].itemType == xml_attribute && idx < designLength
					 && theElement->attributes[elem].identifier != xmlIdentifierInvalid
				){
					if( theElement->attributes[elem].identifier == design[idx].xml.attribute.ID ){
					  XML_ParsedValue val;
						if( !design[idx].reading.parsed ){
							Log( qtLogPtr, ">> attr #%d for tag %s has storage nor handler for parsed/ing result (entry %d)", (int) design[idx].xml.attribute.ID,
										design[idx].xml.attribute.Tag, idx );
							return paramErr;
						}
						if( useHandler ){
							xmlErr = (*design[idx].reading.handler)( theElement, elem, elm, design, idx, fName );
						}
						else{
							val.integerValue = design[idx].reading.parsed;
							// get the expected value kind from the design array, as it distinguishes between
							// attribteValueKindCharString and attributeValueKindDouble!
							switch( design[idx].xml.attribute.Type ){
								case recordAttributeValueTypeCharString:
									xmlErr = GetStringAttribute( theElement, design[idx].xml.attribute.ID, val.stringValue );
									if( xmlErr != attributeNotFound ){
										Log( qtLogPtr, "> attr #%d %s=%s (%d)", (int) design[idx].xml.attribute.ID,
													design[idx].xml.attribute.Tag,
													*val.stringValue, xmlErr );
									}
									break;
								case recordAttributeValueTypeInteger:
								case recordAttributeValueTypePercent:
									xmlErr = GetIntegerAttribute( theElement, design[idx].xml.attribute.ID, val.integerValue );
									if( xmlErr != attributeNotFound ){
										Log( qtLogPtr, "> attr #%d %s=%d (%d)", (int) design[idx].xml.attribute.ID,
													design[idx].xml.attribute.Tag,
													*val.integerValue, xmlErr );
									}
									break;
								case recordAttributeValueTypeBoolean:
								case recordAttributeValueTypeOnOff:
									xmlErr = GetBooleanAttribute( theElement, design[idx].xml.attribute.ID, val.booleanValue );
									if( xmlErr != attributeNotFound ){
										Log( qtLogPtr, "> attr #%d %s=%d (%d)", (int) design[idx].xml.attribute.ID,
													design[idx].xml.attribute.Tag,
													(int) *val.booleanValue, xmlErr );
									}
									break;
								case recordAttributeValueTypeDouble:
									xmlErr = GetDoubleAttribute( theElement, design[idx].xml.attribute.ID, val.doubleValue );
									if( xmlErr != attributeNotFound ){
										Log( qtLogPtr, "> attr #%d %s=%g (%d)", (int) design[idx].xml.attribute.ID,
													design[idx].xml.attribute.Tag,
													*val.doubleValue, xmlErr );
									}
									break;
								default:
									Log( qtLogPtr, ">> unknown attr #%d for tag %s", (int) design[idx].xml.attribute.ID,
												design[idx].xml.attribute.Tag );
									break;
							}
						}
					}
					idx += 1;
					elem += 1;
				}
			}
		}
		else{
			idx += 1;
		}
	}
	return xmlErr;
}

void ReadXMLContent( const char *fName, XMLContent *theContent, XML_Record *design, size_t designLength, size_t *elm )
{ XMLElement theElement;
	while( XMLContentKind( theContent, *elm ) != xmlContentTypeInvalid ){
		if( XMLElementOfContent( theContent, *elm, &theElement ) ){
			Log( qtLogPtr, "> Scanning attributes and/or elements of element #%d (entry %d)",
						theElement.identifier, *elm );
			ReadXMLElementAttributes( &theElement, *elm, design, designLength, FALSE, fName );
		}
		*elm += 1;
	}
}

XMLElement *XMLElementContents( XMLDoc xmldoc, UInt32 element )
{ XMLElement *elm;
	if( xmldoc && xmldoc->rootElement.contents[element].kind == xmlContentTypeElement ){
		elm = &xmldoc->rootElement.contents[element].actualContent.element;
	}
	else{
		elm = NULL;
	}
	return elm;
}

unsigned int XMLRootElementID( XMLDoc xmldoc )
{
	if( xmldoc ){
		return xmldoc->rootElement.identifier;
	}
	else{
		return 0;
	}
}

unsigned int XMLContentKind( XMLContent *theContent, unsigned short element )
{ unsigned int kind;
	if( theContent ){
		kind = theContent[element].kind;
	}
	else{
		kind = xmlIdentifierInvalid;
	}
	return kind;
}

unsigned int XMLRootElementContentKind( XMLDoc xmldoc, unsigned short element )
{ unsigned int kind;
  XMLContent *root_content;
	if( xmldoc ){
		root_content = xmldoc->rootElement.contents;
		kind = XMLContentKind( root_content, element );
	}
	else{
		kind = xmlIdentifierInvalid;
	}
	return kind;
}

Boolean XMLContentOfElementOfRootElement( XMLDoc xmldoc, unsigned short element, XMLContent **theContent )
{ Boolean ret;
	if( xmldoc && theContent ){
		*theContent = xmldoc->rootElement.contents;
		ret = TRUE;
	}
	else{
		ret = FALSE;
	}
	return ret;
}

Boolean XMLContentOfElement( XMLElement *parentElement, XMLContent **theElements )
{ Boolean ret;
	if( parentElement && theElements ){
		*theElements = parentElement->contents;
		ret = TRUE;
	}
	else{
		ret = FALSE;
	}
	return ret;
}

Boolean XMLElementOfContent( XMLContent *theContent, unsigned short element, XMLElement *theElement )
{ Boolean ret;
	if( theContent && theContent[element].kind == xmlContentTypeElement ){
		*theElement = theContent[element].actualContent.element;
		ret = TRUE;
	}
	else{
		ret = FALSE;
	}
	return ret;
}

// ---------------------------------------------------------------------------------
//		• _GetAttributeIndex_ •
//
// Get the index of the attribute
// ---------------------------------------------------------------------------------
static SInt32 _GetAttributeIndex_( XMLAttributePtr attributes, UInt32 attributeID )
{ SInt32 idx = 0;

	if( !attributes ){
		idx = attributeNotFound;
	}
	else{
		while( (attributes[idx]).identifier != xmlIdentifierInvalid && (attributes[idx]).identifier != attributeID ){
			idx++;
		}

		if( (attributes[idx]).identifier == xmlIdentifierInvalid ){
			idx = attributeNotFound;
		}
	}
	return idx;
}

ErrCode GetAttributeIndex( XMLAttributePtr attributes, UInt32 attributeID, SInt32 *idx )
{ ErrCode err;
	if( idx ){
		if( (*idx = _GetAttributeIndex_( attributes, attributeID )) == attributeNotFound ){
			err = *idx;
		}
		else{
			err = noErr;
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

ErrCode GetAttributeIndex_Mod2( XMLAttributePtr attributes, UInt32 attributeID, SInt32 *idx )
{ ErrCode err = GetAttributeIndex( attributes, attributeID, idx );
//	Log( qtLogPtr, "GetAttributeIndex(%p,%u) = %d", attributes, attributeID, *idx );
	return err;
}

// ---------------------------------------------------------------------------------
//		• GetStringAttribute •
//
// Get a C string attribute
// ---------------------------------------------------------------------------------
ErrCode GetStringAttribute( XMLElement *element, UInt32 attributeID, char **theString )
{ ComponentResult err = noErr;
  XMLAttributePtr attributes;
  long attributeIndex, stringLength;

	if( element && element->attributes ){
		attributes = element->attributes;
	}
	else{
		return paramErr;
	}

	// get the attribute index in the array
	attributeIndex = _GetAttributeIndex_(attributes, attributeID);

	// get the value
	if( attributeIndex != attributeNotFound && (attributes[attributeIndex].valueKind == attributeValueKindCharString) ){
		// allocate the string
		stringLength = strlen((Ptr)(attributes[attributeIndex].valueStr));
		*theString = (char*)QTils_malloc(stringLength + 1);
		if( !*theString ){
			err = (errno)? errno : ENOMEM;
		}
		else{
			// copy the string value
			BlockMove(attributes[attributeIndex].valueStr, *theString, stringLength);
			(*theString)[stringLength] = '\0';
		}
	}
	else{
		err = attributeNotFound;
	}
	return (ErrCode) err;
}

ErrCode GetStringAttribute_Mod2( XMLElement *element, UInt32 attributeID, char *theString, int slen )
{ ErrCode err;
  char *str = NULL;

	err = GetStringAttribute( element, attributeID, &str );
	if( str ){
		strncpy( theString, str, slen );
		theString[slen-1] = '\0';
//		Log( qtLogPtr, "GetStringAttribute_Mod2(): got \"%s\" -> '%s'[%d]\n", str, theString, slen );
		QTils_free(&str);
	}
	return err;
}

// ---------------------------------------------------------------------------------
//		• GetIntegerAttribute •
//
// Get the integer attribute
// ---------------------------------------------------------------------------------
ErrCode GetIntegerAttribute( XMLElement *element, UInt32 attributeID, SInt32 *theNumber )
{ ComponentResult err = paramErr;
  XMLAttributePtr attributes;
  long attributeIndex;

	if( element && element->attributes ){
		attributes = element->attributes;
	}
	else{
		return paramErr;
	}

	// get the attribute index in the array
	attributeIndex = _GetAttributeIndex_(attributes, attributeID);

	// get the value
	if( attributeIndex != attributeNotFound && theNumber ){
		if( (attributes[attributeIndex].valueKind == attributeValueKindCharString)
		   && attributes[attributeIndex].valueStr
		){
			if( sscanf( attributes[attributeIndex].valueStr, "%d", &attributes[attributeIndex].value.number ) >= 1 ){
				attributes[attributeIndex].valueKind |= attributeValueKindInteger;
			}
		}
		if( attributes[attributeIndex].valueKind & attributeValueKindInteger ){
			*theNumber = (SInt32) attributes[attributeIndex].value.number;
//			Log( qtLogPtr, "GetIntegerAttribute(%p,%u) = %s -> %d", attributes, attributeID,
//			    attributes[attributeIndex].valueStr, *theNumber
//			);
			err = noErr;
		}
	}
	else{
		err = attributeNotFound;
	}

	return (ErrCode) err;
}

// ---------------------------------------------------------------------------------
//		• GetShortAttribute •
//
// Get the short attribute
// ---------------------------------------------------------------------------------
ErrCode GetShortAttribute( XMLElement *element, UInt32 attributeID, SInt16 *theNumber )
{ ComponentResult err = noErr;
  SInt32 val;

	// get the attribute index in the array
	err = GetIntegerAttribute(element, attributeID, &val);

	// get the value
	if( err != attributeNotFound && theNumber ){
		*theNumber = (SInt16) val;
	}
	else{
		err = attributeNotFound;
	}

	return (ErrCode) err;
}


void Convert_French_Numerals( char *rbuf )
{ char *c = &rbuf[1];
  int i, n = strlen(rbuf)-1;
	for( i = 1 ; i < n ; i++, c++ ){
		if( *c == ',' && (isdigit(c[-1]) || isdigit(c[1])) ){
			*c = '.';
		}
	}
}

// ---------------------------------------------------------------------------------
//		• GetDoubleAttribute •
//
// Get the double attribute
// ---------------------------------------------------------------------------------
ErrCode GetDoubleAttribute( XMLElement *element, UInt32 attributeID, double *theNumber )
{ ComponentResult err = noErr;
  XMLAttributePtr attributes;
  long attributeIndex;

	if( element && element->attributes ){
		attributes = element->attributes;
	}
	else{
		return paramErr;
	}

	// get the attribute index in the array
	attributeIndex = _GetAttributeIndex_(attributes, attributeID);

	// get the value
	if( attributeIndex != attributeNotFound
	   && (attributes[attributeIndex].valueKind == attributeValueKindDouble) && theNumber
	){
	  char *number = (char*) attributes[attributeIndex].valueStr;
		if( strncasecmp( number, "NaN", 3 ) == 0 ){
			set_NaN(*theNumber);
		}
		else if( strncasecmp( number, "Inf", 3 ) == 0 ){
			set_Inf(*theNumber,1);
		}
		else if( strncasecmp( number, "-Inf", 4 ) == 0 ){
			set_Inf(*theNumber,-1);
		}
		else{
			Convert_French_Numerals(number);
			sscanf( number, "%lf", theNumber );
		}
	}
	else{
		err = attributeNotFound;
	}

	return (ErrCode) err;
}

// ---------------------------------------------------------------------------------
//		• GetBooleanAttribute •
//
// Get the Boolean attribute
// ---------------------------------------------------------------------------------
ErrCode GetBooleanAttribute( XMLElement *element, UInt32 attributeID, UInt8 *theBool )
{ ComponentResult err = noErr;
  XMLAttributePtr attributes;
  long attributeIndex;

	if( element && element->attributes ){
		attributes = element->attributes;
	}
	else{
		return paramErr;
	}

	// get the attribute index in the array
	attributeIndex = _GetAttributeIndex_( attributes, attributeID );

	// get the value
	if( attributeIndex != attributeNotFound && (attributes[attributeIndex].valueKind & attributeValueKindBoolean) && theBool ){
		*theBool = (UInt8) attributes[attributeIndex].value.boolean;
	}
	else{
		err = attributeNotFound;
	}

	return (ErrCode) err;
}
