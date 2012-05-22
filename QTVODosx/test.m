/*
 *  test.m
 *  QTVODosx
 *
 *  Created by René J.V. Bertin on 20120426.
 *  Copyright 2012 RJVB. All rights reserved.
 *
 */

#import <stdio.h>
#import <Cocoa/Cocoa.h>
#import <QTKit/QTKit.h>

/*!
	QuickTime uses a number of proprietary ways to access files, one of which consists of a "data handle"
	(not to be confounded with handler!), the data references, and its associated type.
	URLFromDataRef() takes such a reference/type pair, and attempts to determine the full path (URL) and
	name of the referenced file. The file name is returned as a Pascal string...
 */
static OSStatus URLFromDataRef( Handle dataRef, OSType dataRefType, char *theURL, int maxLen, Str255 fileName )
{ DataHandler dh;
  OSStatus err = invalidDataRef;
  ComponentResult result;
	if( dataRef ){
		if( theURL ){
		  CFStringRef outPath = NULL;
			// obtain the full path as a CFString, which is an abstract "text" type with associated encoding (etc).
			err = QTGetDataReferenceFullPathCFString(dataRef, dataRefType, (QTPathStyle)kQTNativeDefaultPathStyle, &outPath);
			if( err == noErr && outPath ){
				// convert the CFString to a C string:
#if __APPLE_CC__
				CFStringGetFileSystemRepresentation( outPath, theURL, maxLen );
#else
				CFStringGetCString( outPath, theURL, maxLen, CFStringGetSystemEncoding() );
#endif
				CFAllocatorDeallocate( kCFAllocatorDefault, (void*) outPath );
			}
		}
		if( fileName ){
			// if the user requested to know the Pascal string fileName, we try to determine that name
			// via a complementary procedure, via the data handler for the file in question.
			fileName[0] = 0;
			result = OpenAComponent( GetDataHandler( dataRef, dataRefType, kDataHCanRead), &dh );
			if( result == noErr ){
				result = DataHSetDataRef( dh, dataRef );
				if( result == noErr ){
					result = DataHGetFileName( dh, fileName );
				}
			}
		}
	}
	return err;
}

#if TARGET_OS_MAC
static OSStatus CFURLFromDataRef( Handle dataRef, OSType dataRefType, CFURLRef *theURL )
{ char path[PATH_MAX];
  OSStatus err = paramErr;
	if( theURL && (err = URLFromDataRef( dataRef, dataRefType, path, sizeof(path), NULL ) ) == noErr ){
		*theURL = CFURLCreateFromFileSystemRepresentation( NULL, (const UInt8 *) path, strlen(path), false );
	}
	return err;
}
#endif

void testOpen( NSURL *url )
{ NSDictionary * initAttributes = [NSDictionary dictionaryWithObjectsAndKeys:
							   url, QTMovieURLAttribute,
							   [NSNumber numberWithBool:NO], QTMovieOpenAsyncOKAttribute,
							   [NSNumber numberWithBool:YES], QTMovieEditableAttribute,
							   nil];
  NSError *error = NULL;
  QTMovie * movie = [[QTMovie alloc] initWithAttributes:initAttributes error:&error];
  QTTrack * destTrack = [[movie tracksOfMediaType:QTMediaTypeSound] lastObject];
  QTTimeRange range;

	range.time = QTMakeTimeWithTimeInterval(0.0);
	range.duration = QTMakeTimeWithTimeInterval(1.0);

	[destTrack insertEmptySegmentAt:range];
	fprintf( stderr, "insertEmptySegment: error=%d\n", GetMoviesError() );

	if( ![movie updateMovieFile] ){
		NSBeep();
		NSLog(@"Updating the movie file failed.");
		fprintf( stderr, "updateMovieFile: error=%d\n", GetMoviesError() );
		{ Handle odataRef = NULL;
		  OSType odataRefType;
		  DataHandler odataHandler = NULL;
		  OSStatus err;
		  Boolean closeDH;
			err = GetMovieDefaultDataRef( [movie quickTimeMovie], &odataRef, &odataRefType );
			if( err == noErr ){
#if TARGET_OS_MAC
			  Boolean excl, exclPath;
			  CFURLRef theURL = NULL;
				if( CFURLFromDataRef( odataRef, odataRefType, &theURL ) == noErr && theURL ){
					excl = CSBackupIsItemExcluded( theURL, &exclPath );
					CSBackupSetItemExcluded( theURL, false, true );
				}
#endif
				if( !odataHandler ){
					err = OpenMovieStorage( odataRef, odataRefType, kDataHCanRead|kDataHCanWrite, &odataHandler );
					closeDH = TRUE;
				}
				if( err == noErr ){
					err = UpdateMovieInStorage( [movie quickTimeMovie], odataHandler );
					fprintf( stderr, "UpdateMovieInStorage: error=%d/%d\n", (int) err, GetMoviesError() );
#if TARGET_OS_MAC
					if( theURL ){
						CSBackupSetItemExcluded( theURL, excl, exclPath );
						CFRelease(theURL);
					}
#endif
					if( closeDH ){
						if( err == noErr ){
							err = CloseMovieStorage( odataHandler );
						}
						else{
							CloseMovieStorage(odataHandler);
						}
					}
				}
				else{
					NSLog( @"OpenMovieStorage failed with err==%d", err );
				}
			}
			else{
				NSLog( @"GetMovieDefaultDataRef failed with err==%d", err );
			}
		}
	}
}

main( int argc, char *argv[] )
{ NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *exts[16] = { @"mov", @"qi2m", @"VOD", @"jpgs", @"mpg",
		@"mp4", @"mpeg", @"avi", @"wmv", @"mp3", @"aif", @"wav", @"mid", @"jpg", @"jpeg", nil };
  NSArray *fileTypes = [NSArray arrayWithObjects:exts count:15];
  NSOpenPanel *oPanel = [NSOpenPanel openPanel];
  int result;
	
	[oPanel setAllowsMultipleSelection:NO];
	[oPanel setCanChooseDirectories:NO];
	[oPanel setAllowedFileTypes:fileTypes];
	[oPanel setAllowsOtherFileTypes:YES];
	[oPanel setTreatsFilePackagesAsDirectories:YES];
	result = [oPanel runModal];
	if( result == NSOKButton ){
	  NSArray *filesToOpen = [oPanel URLs];
	  int count = [filesToOpen count];
		if( count > 0 ){
			EnterMovies();
			testOpen( [filesToOpen objectAtIndex:0] );
			ExitMovies();
		}
	}
	if( pool ){
		[pool drain];
	}
	exit(0);
}