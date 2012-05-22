/*
 * QTCommandGlue.m
 * /Volumes/Debian/Users/bertin/work/src/MacOSX/QTilities/build/Deployment/QTVOD.app
 * osaglue 0.5.4
 *
 */

#import "QTCommandGlue.h"

@implementation QTCommand
- (NSString *)AS_formatObject:(id)obj appData:(id)appData {
    return [QTReferenceRenderer formatObject: obj appData: appData];
}

@end


@implementation QTActivateCommand
- (NSString *)AS_commandName {
    return @"activate";
}

@end


@implementation QTAddChapterCommand
- (QTAddChapterCommand *)duration:(id)value {
    [AS_event setParameter: value forKeyword: 'NCDU'];
    return self;

}

- (QTAddChapterCommand *)name:(id)value {
    [AS_event setParameter: value forKeyword: 'pnam'];
    return self;

}

- (QTAddChapterCommand *)startTime:(id)value {
    [AS_event setParameter: value forKeyword: 'NCST'];
    return self;

}

- (NSString *)AS_commandName {
    return @"addChapter";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'NCDU':
            return @"duration";
        case 'pnam':
            return @"name";
        case 'NCST':
            return @"startTime";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTCloseCommand
- (QTCloseCommand *)saving:(id)value {
    [AS_event setParameter: value forKeyword: 'savo'];
    return self;

}

- (QTCloseCommand *)savingIn:(id)value {
    [AS_event setParameter: value forKeyword: 'kfil'];
    return self;

}

- (NSString *)AS_commandName {
    return @"close";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'savo':
            return @"saving";
        case 'kfil':
            return @"savingIn";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTCountCommand
- (NSString *)AS_commandName {
    return @"count";
}

@end


@implementation QTDeleteCommand
- (NSString *)AS_commandName {
    return @"delete";
}

@end


@implementation QTDuplicateCommand
- (QTDuplicateCommand *)to:(id)value {
    [AS_event setParameter: value forKeyword: 'insh'];
    return self;

}

- (QTDuplicateCommand *)withProperties:(id)value {
    [AS_event setParameter: value forKeyword: 'prdt'];
    return self;

}

- (NSString *)AS_commandName {
    return @"duplicate";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'insh':
            return @"to";
        case 'prdt':
            return @"withProperties";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTExistsCommand
- (NSString *)AS_commandName {
    return @"exists";
}

@end


@implementation QTGetCommand
- (NSString *)AS_commandName {
    return @"get";
}

@end


@implementation QTLaunchCommand
- (NSString *)AS_commandName {
    return @"launch";
}

@end


@implementation QTMakeCommand
- (QTMakeCommand *)at:(id)value {
    [AS_event setParameter: value forKeyword: 'insh'];
    return self;

}

- (QTMakeCommand *)new_:(id)value {
    [AS_event setParameter: value forKeyword: 'kocl'];
    return self;

}

- (QTMakeCommand *)withData:(id)value {
    [AS_event setParameter: value forKeyword: 'data'];
    return self;

}

- (QTMakeCommand *)withProperties:(id)value {
    [AS_event setParameter: value forKeyword: 'prdt'];
    return self;

}

- (NSString *)AS_commandName {
    return @"make";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'insh':
            return @"at";
        case 'kocl':
            return @"new_";
        case 'data':
            return @"withData";
        case 'prdt':
            return @"withProperties";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTMarkTimeIntervalCommand
- (QTMarkTimeIntervalCommand *)display:(id)value {
    [AS_event setParameter: value forKeyword: 'dply'];
    return self;

}

- (QTMarkTimeIntervalCommand *)reset:(id)value {
    [AS_event setParameter: value forKeyword: 'rset'];
    return self;

}

- (NSString *)AS_commandName {
    return @"markTimeInterval";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'dply':
            return @"display";
        case 'rset':
            return @"reset";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTMoveCommand
- (QTMoveCommand *)to:(id)value {
    [AS_event setParameter: value forKeyword: 'insh'];
    return self;

}

- (NSString *)AS_commandName {
    return @"move";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'insh':
            return @"to";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTOpenCommand
- (NSString *)AS_commandName {
    return @"open";
}

@end


@implementation QTOpenLocationCommand
- (QTOpenLocationCommand *)window:(id)value {
    [AS_event setParameter: value forKeyword: 'WIND'];
    return self;

}

- (NSString *)AS_commandName {
    return @"openLocation";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'WIND':
            return @"window";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTPlayCommand
- (NSString *)AS_commandName {
    return @"play";
}

@end


@implementation QTPrintCommand
- (QTPrintCommand *)printDialog:(id)value {
    [AS_event setParameter: value forKeyword: 'pdlg'];
    return self;

}

- (QTPrintCommand *)withProperties:(id)value {
    [AS_event setParameter: value forKeyword: 'prdt'];
    return self;

}

- (NSString *)AS_commandName {
    return @"print";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'pdlg':
            return @"printDialog";
        case 'prdt':
            return @"withProperties";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTQuitCommand
- (QTQuitCommand *)saving:(id)value {
    [AS_event setParameter: value forKeyword: 'savo'];
    return self;

}

- (NSString *)AS_commandName {
    return @"quit";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'savo':
            return @"saving";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTReopenCommand
- (NSString *)AS_commandName {
    return @"reopen";
}

@end


@implementation QTResetVideoCommand
- (QTResetVideoCommand *)complete:(id)value {
    [AS_event setParameter: value forKeyword: 'cmpl'];
    return self;

}

- (NSString *)AS_commandName {
    return @"resetVideo";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'cmpl':
            return @"complete";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTRunCommand
- (NSString *)AS_commandName {
    return @"run";
}

@end


@implementation QTSaveCommand
- (QTSaveCommand *)as:(id)value {
    [AS_event setParameter: value forKeyword: 'fltp'];
    return self;

}

- (QTSaveCommand *)in:(id)value {
    [AS_event setParameter: value forKeyword: 'kfil'];
    return self;

}

- (NSString *)AS_commandName {
    return @"save";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'fltp':
            return @"as";
        case 'kfil':
            return @"in";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTSetCommand
- (QTSetCommand *)to:(id)value {
    [AS_event setParameter: value forKeyword: 'data'];
    return self;

}

- (NSString *)AS_commandName {
    return @"set";
}

- (NSString *)AS_parameterNameForCode:(DescType)code {
    switch (code) {
        case 'data':
            return @"to";
    }
    return [super AS_parameterNameForCode: code];
}

@end


@implementation QTStepBackwardCommand
- (NSString *)AS_commandName {
    return @"stepBackward";
}

@end


@implementation QTStepForwardCommand
- (NSString *)AS_commandName {
    return @"stepForward";
}

@end


@implementation QTStopCommand
- (NSString *)AS_commandName {
    return @"stop";
}

@end

