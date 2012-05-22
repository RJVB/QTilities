/*
 * QTReferenceGlue.m
 * /Volumes/Debian/Users/bertin/work/src/MacOSX/QTilities/build/Deployment/QTVOD.app
 * osaglue 0.5.4
 *
 */

#import "QTReferenceGlue.h"

@implementation QTReference

/* +app, +con, +its methods can be used in place of QTApp, QTCon, QTIts macros */

+ (QTReference *)app {
    return [self referenceWithAppData: nil aemReference: AEMApp];
}

+ (QTReference *)con {
    return [self referenceWithAppData: nil aemReference: AEMCon];
}

+ (QTReference *)its {
    return [self referenceWithAppData: nil aemReference: AEMIts];
}


/* ********************************* */

- (NSString *)description {
    return [QTReferenceRenderer formatObject: AS_aemReference appData: AS_appData];
}


/* Commands */

- (QTActivateCommand *)activate {
    return [QTActivateCommand commandWithAppData: AS_appData
                         eventClass: 'misc'
                            eventID: 'actv'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTActivateCommand *)activate:(id)directParameter {
    return [QTActivateCommand commandWithAppData: AS_appData
                         eventClass: 'misc'
                            eventID: 'actv'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTAddChapterCommand *)addChapter {
    return [QTAddChapterCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'nchp'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTAddChapterCommand *)addChapter:(id)directParameter {
    return [QTAddChapterCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'nchp'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTCloseCommand *)close {
    return [QTCloseCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'clos'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTCloseCommand *)close:(id)directParameter {
    return [QTCloseCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'clos'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTCountCommand *)count {
    return [QTCountCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'cnte'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTCountCommand *)count:(id)directParameter {
    return [QTCountCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'cnte'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTDeleteCommand *)delete {
    return [QTDeleteCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'delo'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTDeleteCommand *)delete:(id)directParameter {
    return [QTDeleteCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'delo'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTDuplicateCommand *)duplicate {
    return [QTDuplicateCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'clon'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTDuplicateCommand *)duplicate:(id)directParameter {
    return [QTDuplicateCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'clon'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTExistsCommand *)exists {
    return [QTExistsCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'doex'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTExistsCommand *)exists:(id)directParameter {
    return [QTExistsCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'doex'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTGetCommand *)get {
    return [QTGetCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'getd'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTGetCommand *)get:(id)directParameter {
    return [QTGetCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'getd'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTLaunchCommand *)launch {
    return [QTLaunchCommand commandWithAppData: AS_appData
                         eventClass: 'ascr'
                            eventID: 'noop'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTLaunchCommand *)launch:(id)directParameter {
    return [QTLaunchCommand commandWithAppData: AS_appData
                         eventClass: 'ascr'
                            eventID: 'noop'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTMakeCommand *)make {
    return [QTMakeCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'crel'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTMakeCommand *)make:(id)directParameter {
    return [QTMakeCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'crel'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTMarkTimeIntervalCommand *)markTimeInterval {
    return [QTMarkTimeIntervalCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'tINT'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTMarkTimeIntervalCommand *)markTimeInterval:(id)directParameter {
    return [QTMarkTimeIntervalCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'tINT'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTMoveCommand *)move {
    return [QTMoveCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'move'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTMoveCommand *)move:(id)directParameter {
    return [QTMoveCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'move'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTOpenCommand *)open {
    return [QTOpenCommand commandWithAppData: AS_appData
                         eventClass: 'aevt'
                            eventID: 'odoc'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTOpenCommand *)open:(id)directParameter {
    return [QTOpenCommand commandWithAppData: AS_appData
                         eventClass: 'aevt'
                            eventID: 'odoc'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTOpenLocationCommand *)openLocation {
    return [QTOpenLocationCommand commandWithAppData: AS_appData
                         eventClass: 'GURL'
                            eventID: 'GURL'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTOpenLocationCommand *)openLocation:(id)directParameter {
    return [QTOpenLocationCommand commandWithAppData: AS_appData
                         eventClass: 'GURL'
                            eventID: 'GURL'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTPlayCommand *)play {
    return [QTPlayCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'strt'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTPlayCommand *)play:(id)directParameter {
    return [QTPlayCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'strt'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTPrintCommand *)print {
    return [QTPrintCommand commandWithAppData: AS_appData
                         eventClass: 'aevt'
                            eventID: 'pdoc'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTPrintCommand *)print:(id)directParameter {
    return [QTPrintCommand commandWithAppData: AS_appData
                         eventClass: 'aevt'
                            eventID: 'pdoc'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTQuitCommand *)quit {
    return [QTQuitCommand commandWithAppData: AS_appData
                         eventClass: 'aevt'
                            eventID: 'quit'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTQuitCommand *)quit:(id)directParameter {
    return [QTQuitCommand commandWithAppData: AS_appData
                         eventClass: 'aevt'
                            eventID: 'quit'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTReopenCommand *)reopen {
    return [QTReopenCommand commandWithAppData: AS_appData
                         eventClass: 'aevt'
                            eventID: 'rapp'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTReopenCommand *)reopen:(id)directParameter {
    return [QTReopenCommand commandWithAppData: AS_appData
                         eventClass: 'aevt'
                            eventID: 'rapp'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTResetVideoCommand *)resetVideo {
    return [QTResetVideoCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'rset'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTResetVideoCommand *)resetVideo:(id)directParameter {
    return [QTResetVideoCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'rset'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTRunCommand *)run {
    return [QTRunCommand commandWithAppData: AS_appData
                         eventClass: 'aevt'
                            eventID: 'oapp'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTRunCommand *)run:(id)directParameter {
    return [QTRunCommand commandWithAppData: AS_appData
                         eventClass: 'aevt'
                            eventID: 'oapp'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTSaveCommand *)save {
    return [QTSaveCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'save'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTSaveCommand *)save:(id)directParameter {
    return [QTSaveCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'save'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTSetCommand *)set {
    return [QTSetCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'setd'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTSetCommand *)set:(id)directParameter {
    return [QTSetCommand commandWithAppData: AS_appData
                         eventClass: 'core'
                            eventID: 'setd'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTStepBackwardCommand *)stepBackward {
    return [QTStepBackwardCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'pfrm'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTStepBackwardCommand *)stepBackward:(id)directParameter {
    return [QTStepBackwardCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'pfrm'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTStepForwardCommand *)stepForward {
    return [QTStepForwardCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'nfrm'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTStepForwardCommand *)stepForward:(id)directParameter {
    return [QTStepForwardCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'nfrm'
                    directParameter: directParameter
                    parentReference: self];
}

- (QTStopCommand *)stop {
    return [QTStopCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'stop'
                    directParameter: kASNoDirectParameter
                    parentReference: self];
}

- (QTStopCommand *)stop:(id)directParameter {
    return [QTStopCommand commandWithAppData: AS_appData
                         eventClass: 'QVOD'
                            eventID: 'stop'
                    directParameter: directParameter
                    parentReference: self];
}


/* Elements */

- (QTReference *)QTChapter {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference elements: 'QTCH']];
}

- (QTReference *)applications {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference elements: 'capp']];
}

- (QTReference *)documentOrListOfDocument {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference elements: 'cct0']];
}

- (QTReference *)documents {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference elements: 'docu']];
}

- (QTReference *)fileOrListOfFile {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference elements: 'cct1']];
}

- (QTReference *)items {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference elements: 'cobj']];
}

- (QTReference *)listOfFileOrSpecifier {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference elements: 'cct2']];
}

- (QTReference *)printSettings {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference elements: 'pset']];
}

- (QTReference *)qtMovieViews {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference elements: 'QTMV']];
}

- (QTReference *)windows {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference elements: 'cwin']];
}


/* Properties */

- (QTReference *)TCframeRate {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'TFRT']];
}

- (QTReference *)absCurrentTime {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'ACRT']];
}

- (QTReference *)bounds {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'pbnd']];
}

- (QTReference *)chapterNames {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'CHNS']];
}

- (QTReference *)chapterTimes {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'CHTS']];
}

- (QTReference *)chapters {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'CHPS']];
}

- (QTReference *)class_ {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'pcls']];
}

- (QTReference *)closeable {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'hclb']];
}

- (QTReference *)collating {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'lwcl']];
}

- (QTReference *)copies {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'lwcp']];
}

- (QTReference *)currentTime {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'CURT']];
}

- (QTReference *)document {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'docu']];
}

- (QTReference *)duration {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'DUTN']];
}

- (QTReference *)endingPage {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'lwlp']];
}

- (QTReference *)errorHandling {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'lweh']];
}

- (QTReference *)faxNumber {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'faxn']];
}

- (QTReference *)file {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'file']];
}

- (QTReference *)frameRate {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'FMRT']];
}

- (QTReference *)frontmost {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'pisf']];
}

- (QTReference *)id_ {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'ID  ']];
}

- (QTReference *)index {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'pidx']];
}

- (QTReference *)lastInterval {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'tINT']];
}

- (QTReference *)miniaturizable {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'ismn']];
}

- (QTReference *)miniaturized {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'pmnd']];
}

- (QTReference *)modified {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'imod']];
}

- (QTReference *)movieView {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'QTmv']];
}

- (QTReference *)name {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'pnam']];
}

- (QTReference *)pagesAcross {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'lwla']];
}

- (QTReference *)pagesDown {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'lwld']];
}

- (QTReference *)path {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'FTPc']];
}

- (QTReference *)properties {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'pALL']];
}

- (QTReference *)requestedPrintTime {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'lwqt']];
}

- (QTReference *)resizable {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'prsz']];
}

- (QTReference *)startTime {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'STTM']];
}

- (QTReference *)startingPage {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'lwfp']];
}

- (QTReference *)targetPrinter {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'trpr']];
}

- (QTReference *)version_ {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'vers']];
}

- (QTReference *)visible {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'pvis']];
}

- (QTReference *)zoomable {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'iszm']];
}

- (QTReference *)zoomed {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference property: 'pzum']];
}


/* ********************************* */


/* ordinal selectors */

- (QTReference *)first {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference first]];
}

- (QTReference *)middle {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference middle]];
}

- (QTReference *)last {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference last]];
}

- (QTReference *)any {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference any]];
}


/* by-index, by-name, by-id selectors */

- (QTReference *)at:(int)index {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference at: index]];
}

- (QTReference *)byIndex:(id)index {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference byIndex: index]];
}

- (QTReference *)byName:(id)name {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference byName: name]];
}

- (QTReference *)byID:(id)id_ {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference byID: id_]];
}


/* by-relative-position selectors */

- (QTReference *)previous:(ASConstant *)class_ {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference previous: [class_ AS_code]]];
}

- (QTReference *)next:(ASConstant *)class_ {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference next: [class_ AS_code]]];
}


/* by-range selector */

- (QTReference *)at:(int)fromIndex to:(int)toIndex {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference at: fromIndex to: toIndex]];
}

- (QTReference *)byRange:(id)fromObject to:(id)toObject {
    // takes two con-based references, with other values being expanded as necessary
    if ([fromObject isKindOfClass: [QTReference class]])
        fromObject = [fromObject AS_aemReference];
    if ([toObject isKindOfClass: [QTReference class]])
        toObject = [toObject AS_aemReference];
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference byRange: fromObject to: toObject]];
}


/* by-test selector */

- (QTReference *)byTest:(QTReference *)testReference {
    return [QTReference referenceWithAppData: AS_appData
                    aemReference: [AS_aemReference byTest: [testReference AS_aemReference]]];
}


/* insertion location selectors */

- (QTReference *)beginning {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference beginning]];
}

- (QTReference *)end {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference end]];
}

- (QTReference *)before {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference before]];
}

- (QTReference *)after {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference after]];
}


/* Comparison and logic tests */

- (QTReference *)greaterThan:(id)object {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference greaterThan: object]];
}

- (QTReference *)greaterOrEquals:(id)object {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference greaterOrEquals: object]];
}

- (QTReference *)equals:(id)object {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference equals: object]];
}

- (QTReference *)notEquals:(id)object {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference notEquals: object]];
}

- (QTReference *)lessThan:(id)object {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference lessThan: object]];
}

- (QTReference *)lessOrEquals:(id)object {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference lessOrEquals: object]];
}

- (QTReference *)beginsWith:(id)object {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference beginsWith: object]];
}

- (QTReference *)endsWith:(id)object {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference endsWith: object]];
}

- (QTReference *)contains:(id)object {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference contains: object]];
}

- (QTReference *)isIn:(id)object {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference isIn: object]];
}

- (QTReference *)AND:(id)remainingOperands {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference AND: remainingOperands]];
}

- (QTReference *)OR:(id)remainingOperands {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference OR: remainingOperands]];
}

- (QTReference *)NOT {
    return [QTReference referenceWithAppData: AS_appData
                                 aemReference: [AS_aemReference NOT]];
}

@end

