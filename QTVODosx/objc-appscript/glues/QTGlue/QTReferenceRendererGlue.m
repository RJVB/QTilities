/*
 * QTReferenceRendererGlue.m
 * /Volumes/Debian/Users/bertin/work/src/MacOSX/QTilities/build/Deployment/QTVOD.app
 * osaglue 0.5.4
 *
 */

#import "QTReferenceRendererGlue.h"

@implementation QTReferenceRenderer
- (NSString *)propertyByCode:(OSType)code {
    switch (code) {
        case 'TFRT': return @"TCframeRate";
        case 'ACRT': return @"absCurrentTime";
        case 'pbnd': return @"bounds";
        case 'CHNS': return @"chapterNames";
        case 'CHTS': return @"chapterTimes";
        case 'CHPS': return @"chapters";
        case 'pcls': return @"class_";
        case 'hclb': return @"closeable";
        case 'lwcl': return @"collating";
        case 'lwcp': return @"copies";
        case 'CURT': return @"currentTime";
        case 'docu': return @"document";
        case 'DUTN': return @"duration";
        case 'lwlp': return @"endingPage";
        case 'lweh': return @"errorHandling";
        case 'faxn': return @"faxNumber";
        case 'file': return @"file";
        case 'FMRT': return @"frameRate";
        case 'pisf': return @"frontmost";
        case 'ID  ': return @"id_";
        case 'pidx': return @"index";
        case 'tINT': return @"lastInterval";
        case 'ismn': return @"miniaturizable";
        case 'pmnd': return @"miniaturized";
        case 'imod': return @"modified";
        case 'QTmv': return @"movieView";
        case 'pnam': return @"name";
        case 'lwla': return @"pagesAcross";
        case 'lwld': return @"pagesDown";
        case 'FTPc': return @"path";
        case 'pALL': return @"properties";
        case 'lwqt': return @"requestedPrintTime";
        case 'prsz': return @"resizable";
        case 'QCTM': return @"startTime";
        case 'STTM': return @"startTime";
        case 'lwfp': return @"startingPage";
        case 'trpr': return @"targetPrinter";
        case 'vers': return @"version_";
        case 'pvis': return @"visible";
        case 'iszm': return @"zoomable";
        case 'pzum': return @"zoomed";
        default: return nil;
    }
}

- (NSString *)elementByCode:(OSType)code {
    switch (code) {
        case 'QTCH': return @"QTChapter";
        case 'capp': return @"applications";
        case 'cct0': return @"documentOrListOfDocument";
        case 'docu': return @"documents";
        case 'cct1': return @"fileOrListOfFile";
        case 'cobj': return @"items";
        case 'cct2': return @"listOfFileOrSpecifier";
        case 'pset': return @"printSettings";
        case 'QTMV': return @"qtMovieViews";
        case 'cwin': return @"windows";
        default: return nil;
    }
}

- (NSString *)prefix {
    return @"QT";
}

@end

