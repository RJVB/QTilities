/*
 * QTReferenceGlue.h
 * /Volumes/Debian/Users/bertin/work/src/MacOSX/QTilities/build/Deployment/QTVOD.app
 * osaglue 0.5.4
 *
 */

#import <Foundation/Foundation.h>
#import "Appscript/Appscript.h"
#import "QTCommandGlue.h"
#import "QTReferenceRendererGlue.h"
#define QTApp ((QTReference *)[QTReference referenceWithAppData: nil aemReference: AEMApp])
#define QTCon ((QTReference *)[QTReference referenceWithAppData: nil aemReference: AEMCon])
#define QTIts ((QTReference *)[QTReference referenceWithAppData: nil aemReference: AEMIts])

@interface QTReference : ASReference

/* +app, +con, +its methods can be used in place of QTApp, QTCon, QTIts macros */

+ (QTReference *)app;
+ (QTReference *)con;
+ (QTReference *)its;

/* ********************************* */

- (NSString *)description;

/* Commands */

- (QTActivateCommand *)activate;
- (QTActivateCommand *)activate:(id)directParameter;
- (QTAddChapterCommand *)addChapter;
- (QTAddChapterCommand *)addChapter:(id)directParameter;
- (QTCloseCommand *)close;
- (QTCloseCommand *)close:(id)directParameter;
- (QTCountCommand *)count;
- (QTCountCommand *)count:(id)directParameter;
- (QTDeleteCommand *)delete;
- (QTDeleteCommand *)delete:(id)directParameter;
- (QTDuplicateCommand *)duplicate;
- (QTDuplicateCommand *)duplicate:(id)directParameter;
- (QTExistsCommand *)exists;
- (QTExistsCommand *)exists:(id)directParameter;
- (QTGetCommand *)get;
- (QTGetCommand *)get:(id)directParameter;
- (QTLaunchCommand *)launch;
- (QTLaunchCommand *)launch:(id)directParameter;
- (QTMakeCommand *)make;
- (QTMakeCommand *)make:(id)directParameter;
- (QTMarkTimeIntervalCommand *)markTimeInterval;
- (QTMarkTimeIntervalCommand *)markTimeInterval:(id)directParameter;
- (QTMoveCommand *)move;
- (QTMoveCommand *)move:(id)directParameter;
- (QTOpenCommand *)open;
- (QTOpenCommand *)open:(id)directParameter;
- (QTOpenLocationCommand *)openLocation;
- (QTOpenLocationCommand *)openLocation:(id)directParameter;
- (QTPlayCommand *)play;
- (QTPlayCommand *)play:(id)directParameter;
- (QTPrintCommand *)print;
- (QTPrintCommand *)print:(id)directParameter;
- (QTQuitCommand *)quit;
- (QTQuitCommand *)quit:(id)directParameter;
- (QTReopenCommand *)reopen;
- (QTReopenCommand *)reopen:(id)directParameter;
- (QTResetVideoCommand *)resetVideo;
- (QTResetVideoCommand *)resetVideo:(id)directParameter;
- (QTRunCommand *)run;
- (QTRunCommand *)run:(id)directParameter;
- (QTSaveCommand *)save;
- (QTSaveCommand *)save:(id)directParameter;
- (QTSetCommand *)set;
- (QTSetCommand *)set:(id)directParameter;
- (QTStepBackwardCommand *)stepBackward;
- (QTStepBackwardCommand *)stepBackward:(id)directParameter;
- (QTStepForwardCommand *)stepForward;
- (QTStepForwardCommand *)stepForward:(id)directParameter;
- (QTStopCommand *)stop;
- (QTStopCommand *)stop:(id)directParameter;

/* Elements */

- (QTReference *)QTChapter;
- (QTReference *)applications;
- (QTReference *)documentOrListOfDocument;
- (QTReference *)documents;
- (QTReference *)fileOrListOfFile;
- (QTReference *)items;
- (QTReference *)listOfFileOrSpecifier;
- (QTReference *)printSettings;
- (QTReference *)qtMovieViews;
- (QTReference *)windows;

/* Properties */

- (QTReference *)TCframeRate;
- (QTReference *)absCurrentTime;
- (QTReference *)bounds;
- (QTReference *)chapterNames;
- (QTReference *)chapterTimes;
- (QTReference *)chapters;
- (QTReference *)class_;
- (QTReference *)closeable;
- (QTReference *)collating;
- (QTReference *)copies;
- (QTReference *)currentTime;
- (QTReference *)document;
- (QTReference *)duration;
- (QTReference *)endingPage;
- (QTReference *)errorHandling;
- (QTReference *)faxNumber;
- (QTReference *)file;
- (QTReference *)frameRate;
- (QTReference *)frontmost;
- (QTReference *)id_;
- (QTReference *)index;
- (QTReference *)lastInterval;
- (QTReference *)miniaturizable;
- (QTReference *)miniaturized;
- (QTReference *)modified;
- (QTReference *)movieView;
- (QTReference *)name;
- (QTReference *)pagesAcross;
- (QTReference *)pagesDown;
- (QTReference *)path;
- (QTReference *)properties;
- (QTReference *)requestedPrintTime;
- (QTReference *)resizable;
- (QTReference *)startTime;
- (QTReference *)startingPage;
- (QTReference *)targetPrinter;
- (QTReference *)version_;
- (QTReference *)visible;
- (QTReference *)zoomable;
- (QTReference *)zoomed;

/* ********************************* */


/* ordinal selectors */

- (QTReference *)first;
- (QTReference *)middle;
- (QTReference *)last;
- (QTReference *)any;

/* by-index, by-name, by-id selectors */

- (QTReference *)at:(int)index;
- (QTReference *)byIndex:(id)index;
- (QTReference *)byName:(id)name;
- (QTReference *)byID:(id)id_;

/* by-relative-position selectors */

- (QTReference *)previous:(ASConstant *)class_;
- (QTReference *)next:(ASConstant *)class_;

/* by-range selector */

- (QTReference *)at:(int)fromIndex to:(int)toIndex;
- (QTReference *)byRange:(id)fromObject to:(id)toObject;

/* by-test selector */

- (QTReference *)byTest:(QTReference *)testReference;

/* insertion location selectors */

- (QTReference *)beginning;
- (QTReference *)end;
- (QTReference *)before;
- (QTReference *)after;

/* Comparison and logic tests */

- (QTReference *)greaterThan:(id)object;
- (QTReference *)greaterOrEquals:(id)object;
- (QTReference *)equals:(id)object;
- (QTReference *)notEquals:(id)object;
- (QTReference *)lessThan:(id)object;
- (QTReference *)lessOrEquals:(id)object;
- (QTReference *)beginsWith:(id)object;
- (QTReference *)endsWith:(id)object;
- (QTReference *)contains:(id)object;
- (QTReference *)isIn:(id)object;
- (QTReference *)AND:(id)remainingOperands;
- (QTReference *)OR:(id)remainingOperands;
- (QTReference *)NOT;
@end

