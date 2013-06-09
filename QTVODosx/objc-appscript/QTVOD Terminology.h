/*
 * QTVOD Terminology.h
 */

#import <AppKit/AppKit.h>
#import <ScriptingBridge/ScriptingBridge.h>


@class QTVODTerminologyApplication, QTVODTerminologyDocument, QTVODTerminologyWindow, QTVODTerminologyDocument, QTVODTerminologyQtMovieView, QTVODTerminologyQTVODScripting;

enum QTVODTerminologySaveOptions {
	QTVODTerminologySaveOptionsYes = 'yes ' /* Save the file. */,
	QTVODTerminologySaveOptionsNo = 'no  ' /* Do not save the file. */,
	QTVODTerminologySaveOptionsAsk = 'ask ' /* Ask the user whether or not to save the file. */
};
typedef enum QTVODTerminologySaveOptions QTVODTerminologySaveOptions;

enum QTVODTerminologyPrintingErrorHandling {
	QTVODTerminologyPrintingErrorHandlingStandard = 'lwst' /* Standard PostScript error handling */,
	QTVODTerminologyPrintingErrorHandlingDetailed = 'lwdt' /* print a detailed report of PostScript errors */
};
typedef enum QTVODTerminologyPrintingErrorHandling QTVODTerminologyPrintingErrorHandling;



/*
 * Standard Suite
 */

// The application's top-level scripting object.
@interface QTVODTerminologyApplication : SBApplication

- (SBElementArray *) documents;
- (SBElementArray *) windows;

@property (copy, readonly) NSString *name;  // The name of the application.
@property (readonly) BOOL frontmost;  // Is this the active application?
@property (copy, readonly) NSString *version;  // The version number of the application.

- (id) open:(id)x;  // Open a document.
- (void) print:(id)x withProperties:(NSDictionary *)withProperties printDialog:(BOOL)printDialog;  // Print a document.
- (void) quitSaving:(QTVODTerminologySaveOptions)saving;  // Quit the application.
- (BOOL) exists:(id)x;  // Verify that an object exists.

@end

// A document.
@interface QTVODTerminologyDocument : SBObject

@property (copy, readonly) NSString *name;  // Its name.
@property (readonly) BOOL modified;  // Has it been modified since the last save?
@property (copy, readonly) NSURL *file;  // Its location on disk, if it has one.

- (void) closeSaving:(QTVODTerminologySaveOptions)saving savingIn:(NSURL *)savingIn;  // Close a document.
- (void) saveIn:(NSURL *)in_ as:(id)as;  // Save a document.
- (void) printWithProperties:(NSDictionary *)withProperties printDialog:(BOOL)printDialog;  // Print a document.
- (void) delete;  // Delete an object.
- (void) duplicateTo:(SBObject *)to withProperties:(NSDictionary *)withProperties;  // Copy an object.
- (void) moveTo:(SBObject *)to;  // Move an object to a new location.
- (void) close;  // close the document
- (void) play;  // start playing
- (void) stop;  // stop playing
- (void) stepForward;  // step 1 frame forward
- (void) stepBackward;  // step 1 frame backward
- (void) addChapterName:(NSString *)name startTime:(double)startTime duration:(double)duration;  // add a new movie chapter
- (void) markTimeIntervalReset:(BOOL)reset display:(BOOL)display;  // marks a new reference frame for time interval measurement
- (void) resetVideoComplete:(BOOL)complete;  // reloads the video, possible after trashing the main cache file (complete=True). This command is not very reliable and to be avoided: prefer instead to get the document's currentTime, close it and reopen the file and finally set the currentTime to the saved value.
- (void) readDesignName:(NSString *)name;  // reads a design from an XML design file

@end

// A window.
@interface QTVODTerminologyWindow : SBObject

@property (copy, readonly) NSString *name;  // The title of the window.
- (NSInteger) id;  // The unique identifier of the window.
@property NSInteger index;  // The index of the window, ordered front to back.
@property NSRect bounds;  // The bounding rectangle of the window.
@property (readonly) BOOL closeable;  // Does the window have a close button?
@property (readonly) BOOL miniaturizable;  // Does the window have a minimize button?
@property BOOL miniaturized;  // Is the window minimized right now?
@property (readonly) BOOL resizable;  // Can the window be resized?
@property BOOL visible;  // Is the window visible right now?
@property (readonly) BOOL zoomable;  // Does the window have a zoom button?
@property BOOL zoomed;  // Is the window zoomed right now?
@property (copy, readonly) QTVODTerminologyDocument *document;  // The document whose contents are displayed in the window.

- (void) closeSaving:(QTVODTerminologySaveOptions)saving savingIn:(NSURL *)savingIn;  // Close a document.
- (void) saveIn:(NSURL *)in_ as:(id)as;  // Save a document.
- (void) printWithProperties:(NSDictionary *)withProperties printDialog:(BOOL)printDialog;  // Print a document.
- (void) delete;  // Delete an object.
- (void) duplicateTo:(SBObject *)to withProperties:(NSDictionary *)withProperties;  // Copy an object.
- (void) moveTo:(SBObject *)to;  // Move an object to a new location.

@end



/*
 * QTVOD Scripting Suite
 */

// A QTVOD document.
@interface QTVODTerminologyDocument (QTVODScriptingSuite)

@property (copy, readonly) NSString *name;  // Its name.
@property (copy, readonly) NSURL *file;  // Its file on disk
- (NSString *) id;  // Its unique ID
@property (copy, readonly) NSString *path;  // Its location on disk, if it has one.
@property double currentTime;  // The current time in seconds from the movie start
@property double absCurrentTime;  // The current time in absolute seconds, i.e. 3600 is 1h am. Obtained from the TimeCode track
@property (readonly) double duration;  // The movie's duration
@property (readonly) double startTime;  // The movie's starting time
@property (readonly) double frameRate;  // The movie's frame rate
@property (readonly) double TCframeRate;  // The movie's frame rate based on TimeCode info
@property (readonly) double lastInterval;  // the last measured time interval between 2 frames
@property (copy, readonly) NSArray *chapterNames;  // the movie's chapter names list
@property (copy, readonly) NSArray *chapterTimes;  // the movie's chapter times list
@property (copy, readonly) NSArray *chapters;  // the movie's chapter list

@end

// The Cocoa QTMovieView class
@interface QTVODTerminologyQtMovieView : SBObject

- (void) closeSaving:(QTVODTerminologySaveOptions)saving savingIn:(NSURL *)savingIn;  // Close a document.
- (void) saveIn:(NSURL *)in_ as:(id)as;  // Save a document.
- (void) printWithProperties:(NSDictionary *)withProperties printDialog:(BOOL)printDialog;  // Print a document.
- (void) delete;  // Delete an object.
- (void) duplicateTo:(SBObject *)to withProperties:(NSDictionary *)withProperties;  // Copy an object.
- (void) moveTo:(SBObject *)to;  // Move an object to a new location.

@end

// QTVOD top level scripting object
@interface QTVODTerminologyQTVODScripting : SBObject

- (void) closeSaving:(QTVODTerminologySaveOptions)saving savingIn:(NSURL *)savingIn;  // Close a document.
- (void) saveIn:(NSURL *)in_ as:(id)as;  // Save a document.
- (void) printWithProperties:(NSDictionary *)withProperties printDialog:(BOOL)printDialog;  // Print a document.
- (void) delete;  // Delete an object.
- (void) duplicateTo:(SBObject *)to withProperties:(NSDictionary *)withProperties;  // Copy an object.
- (void) moveTo:(SBObject *)to;  // Move an object to a new location.
- (void) connectToServerAddress:(NSString *)address;  // connects to the specified server
- (void) toggleLogging;  // toggles the logging facility

@end

