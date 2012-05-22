/*
 * QTCommandGlue.h
 * /Volumes/Debian/Users/bertin/work/src/MacOSX/QTilities/build/Deployment/QTVOD.app
 * osaglue 0.5.4
 *
 */

#import <Foundation/Foundation.h>
#import "Appscript/Appscript.h"
#import "QTReferenceRendererGlue.h"

@interface QTCommand : ASCommand
- (NSString *)AS_formatObject:(id)obj appData:(id)appData;
@end


@interface QTActivateCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTAddChapterCommand : QTCommand
- (QTAddChapterCommand *)duration:(id)value;
- (QTAddChapterCommand *)name:(id)value;
- (QTAddChapterCommand *)startTime:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTCloseCommand : QTCommand
- (QTCloseCommand *)saving:(id)value;
- (QTCloseCommand *)savingIn:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTCountCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTDeleteCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTDuplicateCommand : QTCommand
- (QTDuplicateCommand *)to:(id)value;
- (QTDuplicateCommand *)withProperties:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTExistsCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTGetCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTLaunchCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTMakeCommand : QTCommand
- (QTMakeCommand *)at:(id)value;
- (QTMakeCommand *)new_:(id)value;
- (QTMakeCommand *)withData:(id)value;
- (QTMakeCommand *)withProperties:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTMarkTimeIntervalCommand : QTCommand
- (QTMarkTimeIntervalCommand *)display:(id)value;
- (QTMarkTimeIntervalCommand *)reset:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTMoveCommand : QTCommand
- (QTMoveCommand *)to:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTOpenCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTOpenLocationCommand : QTCommand
- (QTOpenLocationCommand *)window:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTPlayCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTPrintCommand : QTCommand
- (QTPrintCommand *)printDialog:(id)value;
- (QTPrintCommand *)withProperties:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTQuitCommand : QTCommand
- (QTQuitCommand *)saving:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTReopenCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTResetVideoCommand : QTCommand
- (QTResetVideoCommand *)complete:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTRunCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTSaveCommand : QTCommand
- (QTSaveCommand *)as:(id)value;
- (QTSaveCommand *)in:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTSetCommand : QTCommand
- (QTSetCommand *)to:(id)value;
- (NSString *)AS_commandName;
- (NSString *)AS_parameterNameForCode:(DescType)code;
@end


@interface QTStepBackwardCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTStepForwardCommand : QTCommand
- (NSString *)AS_commandName;
@end


@interface QTStopCommand : QTCommand
- (NSString *)AS_commandName;
@end

