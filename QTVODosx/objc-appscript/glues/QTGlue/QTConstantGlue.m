/*
 * QTConstantGlue.m
 * /Volumes/Debian/Users/bertin/work/src/MacOSX/QTilities/build/Deployment/QTVOD.app
 * osaglue 0.5.4
 *
 */

#import "QTConstantGlue.h"

@implementation QTConstant
+ (id)constantWithCode:(OSType)code_ {
    switch (code_) {
        case 'apr ': return [self April];
        case 'aug ': return [self August];
        case 'dec ': return [self December];
        case 'EPS ': return [self EPSPicture];
        case 'feb ': return [self February];
        case 'fri ': return [self Friday];
        case 'GIFf': return [self GIFPicture];
        case 'JPEG': return [self JPEGPicture];
        case 'jan ': return [self January];
        case 'jul ': return [self July];
        case 'jun ': return [self June];
        case 'mar ': return [self March];
        case 'may ': return [self May];
        case 'mon ': return [self Monday];
        case 'nov ': return [self November];
        case 'oct ': return [self October];
        case 'PICT': return [self PICTPicture];
        case 'QTCH': return [self QTChapter];
        case 'tr16': return [self RGB16Color];
        case 'tr96': return [self RGB96Color];
        case 'cRGB': return [self RGBColor];
        case 'sat ': return [self Saturday];
        case 'sep ': return [self September];
        case 'sun ': return [self Sunday];
        case 'TFRT': return [self TCframeRate];
        case 'TIFF': return [self TIFFPicture];
        case 'thu ': return [self Thursday];
        case 'tue ': return [self Tuesday];
        case 'wed ': return [self Wednesday];
        case 'ACRT': return [self absCurrentTime];
        case 'alis': return [self alias];
        case '****': return [self anything];
        case 'capp': return [self application];
        case 'bund': return [self applicationBundleID];
        case 'rmte': return [self applicationResponses];
        case 'sign': return [self applicationSignature];
        case 'aprl': return [self applicationURL];
        case 'ask ': return [self ask];
        case 'best': return [self best];
        case 'bool': return [self boolean];
        case 'qdrt': return [self boundingRectangle];
        case 'pbnd': return [self bounds];
        case 'case': return [self case_];
        case 'cmtr': return [self centimeters];
        case 'CHNS': return [self chapterNames];
        case 'CHTS': return [self chapterTimes];
        case 'CHPS': return [self chapters];
        case 'gcli': return [self classInfo];
        case 'pcls': return [self class_];
        case 'hclb': return [self closeable];
        case 'lwcl': return [self collating];
        case 'clrt': return [self colorTable];
        case 'lwcp': return [self copies];
        case 'ccmt': return [self cubicCentimeters];
        case 'cfet': return [self cubicFeet];
        case 'cuin': return [self cubicInches];
        case 'cmet': return [self cubicMeters];
        case 'cyrd': return [self cubicYards];
        case 'CURT': return [self currentTime];
        case 'tdas': return [self dashStyle];
        case 'rdat': return [self data];
        case 'ldt ': return [self date];
        case 'decm': return [self decimalStruct];
        case 'degc': return [self degreesCelsius];
        case 'degf': return [self degreesFahrenheit];
        case 'degk': return [self degreesKelvin];
        case 'lwdt': return [self detailed];
        case 'diac': return [self diacriticals];
        case 'docu': return [self document];
        case 'cct0': return [self documentOrListOfDocument];
        case 'comp': return [self doubleInteger];
        case 'DUTN': return [self duration];
        case 'elin': return [self elementInfo];
        case 'encs': return [self encodedString];
        case 'lwlp': return [self endingPage];
        case 'enum': return [self enumerator];
        case 'lweh': return [self errorHandling];
        case 'evin': return [self eventInfo];
        case 'expa': return [self expansion];
        case 'exte': return [self extendedFloat];
        case 'faxn': return [self faxNumber];
        case 'feet': return [self feet];
        case 'file': return [self file];
        case 'cct1': return [self fileOrListOfFile];
        case 'fsrf': return [self fileRef];
        case 'fss ': return [self fileSpecification];
        case 'furl': return [self fileURL];
        case 'fixd': return [self fixed];
        case 'fpnt': return [self fixedPoint];
        case 'frct': return [self fixedRectangle];
        case 'ldbl': return [self float128bit];
        case 'doub': return [self float_];
        case 'FMRT': return [self frameRate];
        case 'pisf': return [self frontmost];
        case 'galn': return [self gallons];
        case 'gram': return [self grams];
        case 'cgtx': return [self graphicText];
        case 'hyph': return [self hyphens];
        case 'ID  ': return [self id_];
        case 'inch': return [self inches];
        case 'pidx': return [self index];
        case 'long': return [self integer];
        case 'itxt': return [self internationalText];
        case 'intl': return [self internationalWritingCode];
        case 'cobj': return [self item];
        case 'kpid': return [self kernelProcessID];
        case 'kgrm': return [self kilograms];
        case 'kmtr': return [self kilometers];
        case 'tINT': return [self lastInterval];
        case 'list': return [self list];
        case 'cct2': return [self listOfFileOrSpecifier];
        case 'litr': return [self liters];
        case 'insl': return [self locationReference];
        case 'lfxd': return [self longFixed];
        case 'lfpt': return [self longFixedPoint];
        case 'lfrc': return [self longFixedRectangle];
        case 'lpnt': return [self longPoint];
        case 'lrct': return [self longRectangle];
        case 'port': return [self machPort];
        case 'mach': return [self machine];
        case 'mLoc': return [self machineLocation];
        case 'metr': return [self meters];
        case 'mile': return [self miles];
        case 'ismn': return [self miniaturizable];
        case 'pmnd': return [self miniaturized];
        case 'msng': return [self missingValue];
        case 'imod': return [self modified];
        case 'QTmv': return [self movieView];
        case 'pnam': return [self name];
        case 'no  ': return [self no];
        case 'null': return [self null];
        case 'nume': return [self numericStrings];
        case 'ozs ': return [self ounces];
        case 'lwla': return [self pagesAcross];
        case 'lwld': return [self pagesDown];
        case 'pmin': return [self parameterInfo];
        case 'FTPc': return [self path];
        case 'tpmm': return [self pixelMapRecord];
        case 'QDpt': return [self point];
        case 'lbs ': return [self pounds];
        case 'pset': return [self printSettings];
        case 'psn ': return [self processSerialNumber];
        case 'pALL': return [self properties];
        case 'prop': return [self property];
        case 'pinf': return [self propertyInfo];
        case 'punc': return [self punctuation];
        case 'QTMV': return [self qtMovieView];
        case 'qrts': return [self quarts];
        case 'reco': return [self record];
        case 'obj ': return [self reference];
        case 'lwqt': return [self requestedPrintTime];
        case 'prsz': return [self resizable];
        case 'trot': return [self rotation];
        case 'scpt': return [self script];
        case 'sing': return [self shortFloat];
        case 'shor': return [self shortInteger];
        case 'sqft': return [self squareFeet];
        case 'sqkm': return [self squareKilometers];
        case 'sqrm': return [self squareMeters];
        case 'sqmi': return [self squareMiles];
        case 'sqyd': return [self squareYards];
        case 'lwst': return [self standard];
        case 'STTM': return [self startTime];
        case 'QCTM': return [self startTime];
        case 'lwfp': return [self startingPage];
        case 'TEXT': return [self string];
        case 'styl': return [self styledClipboardText];
        case 'STXT': return [self styledText];
        case 'suin': return [self suiteInfo];
        case 'trpr': return [self targetPrinter];
        case 'tsty': return [self textStyleInfo];
        case 'type': return [self typeClass];
        case 'utxt': return [self unicodeText];
        case 'magn': return [self unsignedInteger];
        case 'ut16': return [self utf16Text];
        case 'utf8': return [self utf8Text];
        case 'vers': return [self version_];
        case 'pvis': return [self visible];
        case 'whit': return [self whitespace];
        case 'cwin': return [self window];
        case 'psct': return [self writingCode];
        case 'yard': return [self yards];
        case 'yes ': return [self yes];
        case 'iszm': return [self zoomable];
        case 'pzum': return [self zoomed];
        default: return [[self superclass] constantWithCode: code_];
    }
}


/* Enumerators */

+ (QTConstant *)applicationResponses {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"applicationResponses" type: typeEnumerated code: 'rmte'];
    return constantObj;
}

+ (QTConstant *)ask {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"ask" type: typeEnumerated code: 'ask '];
    return constantObj;
}

+ (QTConstant *)case_ {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"case_" type: typeEnumerated code: 'case'];
    return constantObj;
}

+ (QTConstant *)detailed {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"detailed" type: typeEnumerated code: 'lwdt'];
    return constantObj;
}

+ (QTConstant *)diacriticals {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"diacriticals" type: typeEnumerated code: 'diac'];
    return constantObj;
}

+ (QTConstant *)expansion {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"expansion" type: typeEnumerated code: 'expa'];
    return constantObj;
}

+ (QTConstant *)hyphens {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"hyphens" type: typeEnumerated code: 'hyph'];
    return constantObj;
}

+ (QTConstant *)no {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"no" type: typeEnumerated code: 'no  '];
    return constantObj;
}

+ (QTConstant *)numericStrings {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"numericStrings" type: typeEnumerated code: 'nume'];
    return constantObj;
}

+ (QTConstant *)punctuation {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"punctuation" type: typeEnumerated code: 'punc'];
    return constantObj;
}

+ (QTConstant *)standard {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"standard" type: typeEnumerated code: 'lwst'];
    return constantObj;
}

+ (QTConstant *)whitespace {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"whitespace" type: typeEnumerated code: 'whit'];
    return constantObj;
}

+ (QTConstant *)yes {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"yes" type: typeEnumerated code: 'yes '];
    return constantObj;
}


/* Types and properties */

+ (QTConstant *)April {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"April" type: typeType code: 'apr '];
    return constantObj;
}

+ (QTConstant *)August {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"August" type: typeType code: 'aug '];
    return constantObj;
}

+ (QTConstant *)December {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"December" type: typeType code: 'dec '];
    return constantObj;
}

+ (QTConstant *)EPSPicture {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"EPSPicture" type: typeType code: 'EPS '];
    return constantObj;
}

+ (QTConstant *)February {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"February" type: typeType code: 'feb '];
    return constantObj;
}

+ (QTConstant *)Friday {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"Friday" type: typeType code: 'fri '];
    return constantObj;
}

+ (QTConstant *)GIFPicture {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"GIFPicture" type: typeType code: 'GIFf'];
    return constantObj;
}

+ (QTConstant *)JPEGPicture {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"JPEGPicture" type: typeType code: 'JPEG'];
    return constantObj;
}

+ (QTConstant *)January {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"January" type: typeType code: 'jan '];
    return constantObj;
}

+ (QTConstant *)July {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"July" type: typeType code: 'jul '];
    return constantObj;
}

+ (QTConstant *)June {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"June" type: typeType code: 'jun '];
    return constantObj;
}

+ (QTConstant *)March {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"March" type: typeType code: 'mar '];
    return constantObj;
}

+ (QTConstant *)May {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"May" type: typeType code: 'may '];
    return constantObj;
}

+ (QTConstant *)Monday {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"Monday" type: typeType code: 'mon '];
    return constantObj;
}

+ (QTConstant *)November {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"November" type: typeType code: 'nov '];
    return constantObj;
}

+ (QTConstant *)October {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"October" type: typeType code: 'oct '];
    return constantObj;
}

+ (QTConstant *)PICTPicture {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"PICTPicture" type: typeType code: 'PICT'];
    return constantObj;
}

+ (QTConstant *)QTChapter {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"QTChapter" type: typeType code: 'QTCH'];
    return constantObj;
}

+ (QTConstant *)RGB16Color {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"RGB16Color" type: typeType code: 'tr16'];
    return constantObj;
}

+ (QTConstant *)RGB96Color {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"RGB96Color" type: typeType code: 'tr96'];
    return constantObj;
}

+ (QTConstant *)RGBColor {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"RGBColor" type: typeType code: 'cRGB'];
    return constantObj;
}

+ (QTConstant *)Saturday {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"Saturday" type: typeType code: 'sat '];
    return constantObj;
}

+ (QTConstant *)September {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"September" type: typeType code: 'sep '];
    return constantObj;
}

+ (QTConstant *)Sunday {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"Sunday" type: typeType code: 'sun '];
    return constantObj;
}

+ (QTConstant *)TCframeRate {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"TCframeRate" type: typeType code: 'TFRT'];
    return constantObj;
}

+ (QTConstant *)TIFFPicture {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"TIFFPicture" type: typeType code: 'TIFF'];
    return constantObj;
}

+ (QTConstant *)Thursday {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"Thursday" type: typeType code: 'thu '];
    return constantObj;
}

+ (QTConstant *)Tuesday {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"Tuesday" type: typeType code: 'tue '];
    return constantObj;
}

+ (QTConstant *)Wednesday {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"Wednesday" type: typeType code: 'wed '];
    return constantObj;
}

+ (QTConstant *)absCurrentTime {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"absCurrentTime" type: typeType code: 'ACRT'];
    return constantObj;
}

+ (QTConstant *)alias {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"alias" type: typeType code: 'alis'];
    return constantObj;
}

+ (QTConstant *)anything {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"anything" type: typeType code: '****'];
    return constantObj;
}

+ (QTConstant *)application {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"application" type: typeType code: 'capp'];
    return constantObj;
}

+ (QTConstant *)applicationBundleID {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"applicationBundleID" type: typeType code: 'bund'];
    return constantObj;
}

+ (QTConstant *)applicationSignature {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"applicationSignature" type: typeType code: 'sign'];
    return constantObj;
}

+ (QTConstant *)applicationURL {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"applicationURL" type: typeType code: 'aprl'];
    return constantObj;
}

+ (QTConstant *)best {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"best" type: typeType code: 'best'];
    return constantObj;
}

+ (QTConstant *)boolean {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"boolean" type: typeType code: 'bool'];
    return constantObj;
}

+ (QTConstant *)boundingRectangle {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"boundingRectangle" type: typeType code: 'qdrt'];
    return constantObj;
}

+ (QTConstant *)bounds {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"bounds" type: typeType code: 'pbnd'];
    return constantObj;
}

+ (QTConstant *)centimeters {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"centimeters" type: typeType code: 'cmtr'];
    return constantObj;
}

+ (QTConstant *)chapterNames {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"chapterNames" type: typeType code: 'CHNS'];
    return constantObj;
}

+ (QTConstant *)chapterTimes {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"chapterTimes" type: typeType code: 'CHTS'];
    return constantObj;
}

+ (QTConstant *)chapters {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"chapters" type: typeType code: 'CHPS'];
    return constantObj;
}

+ (QTConstant *)classInfo {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"classInfo" type: typeType code: 'gcli'];
    return constantObj;
}

+ (QTConstant *)class_ {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"class_" type: typeType code: 'pcls'];
    return constantObj;
}

+ (QTConstant *)closeable {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"closeable" type: typeType code: 'hclb'];
    return constantObj;
}

+ (QTConstant *)collating {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"collating" type: typeType code: 'lwcl'];
    return constantObj;
}

+ (QTConstant *)colorTable {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"colorTable" type: typeType code: 'clrt'];
    return constantObj;
}

+ (QTConstant *)copies {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"copies" type: typeType code: 'lwcp'];
    return constantObj;
}

+ (QTConstant *)cubicCentimeters {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"cubicCentimeters" type: typeType code: 'ccmt'];
    return constantObj;
}

+ (QTConstant *)cubicFeet {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"cubicFeet" type: typeType code: 'cfet'];
    return constantObj;
}

+ (QTConstant *)cubicInches {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"cubicInches" type: typeType code: 'cuin'];
    return constantObj;
}

+ (QTConstant *)cubicMeters {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"cubicMeters" type: typeType code: 'cmet'];
    return constantObj;
}

+ (QTConstant *)cubicYards {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"cubicYards" type: typeType code: 'cyrd'];
    return constantObj;
}

+ (QTConstant *)currentTime {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"currentTime" type: typeType code: 'CURT'];
    return constantObj;
}

+ (QTConstant *)dashStyle {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"dashStyle" type: typeType code: 'tdas'];
    return constantObj;
}

+ (QTConstant *)data {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"data" type: typeType code: 'rdat'];
    return constantObj;
}

+ (QTConstant *)date {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"date" type: typeType code: 'ldt '];
    return constantObj;
}

+ (QTConstant *)decimalStruct {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"decimalStruct" type: typeType code: 'decm'];
    return constantObj;
}

+ (QTConstant *)degreesCelsius {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"degreesCelsius" type: typeType code: 'degc'];
    return constantObj;
}

+ (QTConstant *)degreesFahrenheit {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"degreesFahrenheit" type: typeType code: 'degf'];
    return constantObj;
}

+ (QTConstant *)degreesKelvin {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"degreesKelvin" type: typeType code: 'degk'];
    return constantObj;
}

+ (QTConstant *)document {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"document" type: typeType code: 'docu'];
    return constantObj;
}

+ (QTConstant *)documentOrListOfDocument {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"documentOrListOfDocument" type: typeType code: 'cct0'];
    return constantObj;
}

+ (QTConstant *)doubleInteger {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"doubleInteger" type: typeType code: 'comp'];
    return constantObj;
}

+ (QTConstant *)duration {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"duration" type: typeType code: 'DUTN'];
    return constantObj;
}

+ (QTConstant *)elementInfo {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"elementInfo" type: typeType code: 'elin'];
    return constantObj;
}

+ (QTConstant *)encodedString {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"encodedString" type: typeType code: 'encs'];
    return constantObj;
}

+ (QTConstant *)endingPage {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"endingPage" type: typeType code: 'lwlp'];
    return constantObj;
}

+ (QTConstant *)enumerator {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"enumerator" type: typeType code: 'enum'];
    return constantObj;
}

+ (QTConstant *)errorHandling {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"errorHandling" type: typeType code: 'lweh'];
    return constantObj;
}

+ (QTConstant *)eventInfo {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"eventInfo" type: typeType code: 'evin'];
    return constantObj;
}

+ (QTConstant *)extendedFloat {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"extendedFloat" type: typeType code: 'exte'];
    return constantObj;
}

+ (QTConstant *)faxNumber {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"faxNumber" type: typeType code: 'faxn'];
    return constantObj;
}

+ (QTConstant *)feet {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"feet" type: typeType code: 'feet'];
    return constantObj;
}

+ (QTConstant *)file {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"file" type: typeType code: 'file'];
    return constantObj;
}

+ (QTConstant *)fileOrListOfFile {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"fileOrListOfFile" type: typeType code: 'cct1'];
    return constantObj;
}

+ (QTConstant *)fileRef {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"fileRef" type: typeType code: 'fsrf'];
    return constantObj;
}

+ (QTConstant *)fileSpecification {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"fileSpecification" type: typeType code: 'fss '];
    return constantObj;
}

+ (QTConstant *)fileURL {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"fileURL" type: typeType code: 'furl'];
    return constantObj;
}

+ (QTConstant *)fixed {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"fixed" type: typeType code: 'fixd'];
    return constantObj;
}

+ (QTConstant *)fixedPoint {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"fixedPoint" type: typeType code: 'fpnt'];
    return constantObj;
}

+ (QTConstant *)fixedRectangle {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"fixedRectangle" type: typeType code: 'frct'];
    return constantObj;
}

+ (QTConstant *)float128bit {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"float128bit" type: typeType code: 'ldbl'];
    return constantObj;
}

+ (QTConstant *)float_ {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"float_" type: typeType code: 'doub'];
    return constantObj;
}

+ (QTConstant *)frameRate {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"frameRate" type: typeType code: 'FMRT'];
    return constantObj;
}

+ (QTConstant *)frontmost {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"frontmost" type: typeType code: 'pisf'];
    return constantObj;
}

+ (QTConstant *)gallons {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"gallons" type: typeType code: 'galn'];
    return constantObj;
}

+ (QTConstant *)grams {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"grams" type: typeType code: 'gram'];
    return constantObj;
}

+ (QTConstant *)graphicText {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"graphicText" type: typeType code: 'cgtx'];
    return constantObj;
}

+ (QTConstant *)id_ {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"id_" type: typeType code: 'ID  '];
    return constantObj;
}

+ (QTConstant *)inches {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"inches" type: typeType code: 'inch'];
    return constantObj;
}

+ (QTConstant *)index {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"index" type: typeType code: 'pidx'];
    return constantObj;
}

+ (QTConstant *)integer {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"integer" type: typeType code: 'long'];
    return constantObj;
}

+ (QTConstant *)internationalText {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"internationalText" type: typeType code: 'itxt'];
    return constantObj;
}

+ (QTConstant *)internationalWritingCode {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"internationalWritingCode" type: typeType code: 'intl'];
    return constantObj;
}

+ (QTConstant *)item {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"item" type: typeType code: 'cobj'];
    return constantObj;
}

+ (QTConstant *)kernelProcessID {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"kernelProcessID" type: typeType code: 'kpid'];
    return constantObj;
}

+ (QTConstant *)kilograms {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"kilograms" type: typeType code: 'kgrm'];
    return constantObj;
}

+ (QTConstant *)kilometers {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"kilometers" type: typeType code: 'kmtr'];
    return constantObj;
}

+ (QTConstant *)lastInterval {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"lastInterval" type: typeType code: 'tINT'];
    return constantObj;
}

+ (QTConstant *)list {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"list" type: typeType code: 'list'];
    return constantObj;
}

+ (QTConstant *)listOfFileOrSpecifier {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"listOfFileOrSpecifier" type: typeType code: 'cct2'];
    return constantObj;
}

+ (QTConstant *)liters {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"liters" type: typeType code: 'litr'];
    return constantObj;
}

+ (QTConstant *)locationReference {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"locationReference" type: typeType code: 'insl'];
    return constantObj;
}

+ (QTConstant *)longFixed {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"longFixed" type: typeType code: 'lfxd'];
    return constantObj;
}

+ (QTConstant *)longFixedPoint {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"longFixedPoint" type: typeType code: 'lfpt'];
    return constantObj;
}

+ (QTConstant *)longFixedRectangle {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"longFixedRectangle" type: typeType code: 'lfrc'];
    return constantObj;
}

+ (QTConstant *)longPoint {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"longPoint" type: typeType code: 'lpnt'];
    return constantObj;
}

+ (QTConstant *)longRectangle {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"longRectangle" type: typeType code: 'lrct'];
    return constantObj;
}

+ (QTConstant *)machPort {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"machPort" type: typeType code: 'port'];
    return constantObj;
}

+ (QTConstant *)machine {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"machine" type: typeType code: 'mach'];
    return constantObj;
}

+ (QTConstant *)machineLocation {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"machineLocation" type: typeType code: 'mLoc'];
    return constantObj;
}

+ (QTConstant *)meters {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"meters" type: typeType code: 'metr'];
    return constantObj;
}

+ (QTConstant *)miles {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"miles" type: typeType code: 'mile'];
    return constantObj;
}

+ (QTConstant *)miniaturizable {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"miniaturizable" type: typeType code: 'ismn'];
    return constantObj;
}

+ (QTConstant *)miniaturized {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"miniaturized" type: typeType code: 'pmnd'];
    return constantObj;
}

+ (QTConstant *)missingValue {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"missingValue" type: typeType code: 'msng'];
    return constantObj;
}

+ (QTConstant *)modified {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"modified" type: typeType code: 'imod'];
    return constantObj;
}

+ (QTConstant *)movieView {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"movieView" type: typeType code: 'QTmv'];
    return constantObj;
}

+ (QTConstant *)name {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"name" type: typeType code: 'pnam'];
    return constantObj;
}

+ (QTConstant *)null {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"null" type: typeType code: 'null'];
    return constantObj;
}

+ (QTConstant *)ounces {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"ounces" type: typeType code: 'ozs '];
    return constantObj;
}

+ (QTConstant *)pagesAcross {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"pagesAcross" type: typeType code: 'lwla'];
    return constantObj;
}

+ (QTConstant *)pagesDown {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"pagesDown" type: typeType code: 'lwld'];
    return constantObj;
}

+ (QTConstant *)parameterInfo {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"parameterInfo" type: typeType code: 'pmin'];
    return constantObj;
}

+ (QTConstant *)path {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"path" type: typeType code: 'FTPc'];
    return constantObj;
}

+ (QTConstant *)pixelMapRecord {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"pixelMapRecord" type: typeType code: 'tpmm'];
    return constantObj;
}

+ (QTConstant *)point {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"point" type: typeType code: 'QDpt'];
    return constantObj;
}

+ (QTConstant *)pounds {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"pounds" type: typeType code: 'lbs '];
    return constantObj;
}

+ (QTConstant *)printSettings {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"printSettings" type: typeType code: 'pset'];
    return constantObj;
}

+ (QTConstant *)processSerialNumber {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"processSerialNumber" type: typeType code: 'psn '];
    return constantObj;
}

+ (QTConstant *)properties {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"properties" type: typeType code: 'pALL'];
    return constantObj;
}

+ (QTConstant *)property {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"property" type: typeType code: 'prop'];
    return constantObj;
}

+ (QTConstant *)propertyInfo {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"propertyInfo" type: typeType code: 'pinf'];
    return constantObj;
}

+ (QTConstant *)qtMovieView {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"qtMovieView" type: typeType code: 'QTMV'];
    return constantObj;
}

+ (QTConstant *)quarts {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"quarts" type: typeType code: 'qrts'];
    return constantObj;
}

+ (QTConstant *)record {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"record" type: typeType code: 'reco'];
    return constantObj;
}

+ (QTConstant *)reference {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"reference" type: typeType code: 'obj '];
    return constantObj;
}

+ (QTConstant *)requestedPrintTime {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"requestedPrintTime" type: typeType code: 'lwqt'];
    return constantObj;
}

+ (QTConstant *)resizable {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"resizable" type: typeType code: 'prsz'];
    return constantObj;
}

+ (QTConstant *)rotation {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"rotation" type: typeType code: 'trot'];
    return constantObj;
}

+ (QTConstant *)script {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"script" type: typeType code: 'scpt'];
    return constantObj;
}

+ (QTConstant *)shortFloat {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"shortFloat" type: typeType code: 'sing'];
    return constantObj;
}

+ (QTConstant *)shortInteger {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"shortInteger" type: typeType code: 'shor'];
    return constantObj;
}

+ (QTConstant *)squareFeet {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"squareFeet" type: typeType code: 'sqft'];
    return constantObj;
}

+ (QTConstant *)squareKilometers {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"squareKilometers" type: typeType code: 'sqkm'];
    return constantObj;
}

+ (QTConstant *)squareMeters {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"squareMeters" type: typeType code: 'sqrm'];
    return constantObj;
}

+ (QTConstant *)squareMiles {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"squareMiles" type: typeType code: 'sqmi'];
    return constantObj;
}

+ (QTConstant *)squareYards {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"squareYards" type: typeType code: 'sqyd'];
    return constantObj;
}

+ (QTConstant *)startTime {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"startTime" type: typeType code: 'STTM'];
    return constantObj;
}

+ (QTConstant *)startingPage {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"startingPage" type: typeType code: 'lwfp'];
    return constantObj;
}

+ (QTConstant *)string {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"string" type: typeType code: 'TEXT'];
    return constantObj;
}

+ (QTConstant *)styledClipboardText {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"styledClipboardText" type: typeType code: 'styl'];
    return constantObj;
}

+ (QTConstant *)styledText {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"styledText" type: typeType code: 'STXT'];
    return constantObj;
}

+ (QTConstant *)suiteInfo {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"suiteInfo" type: typeType code: 'suin'];
    return constantObj;
}

+ (QTConstant *)targetPrinter {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"targetPrinter" type: typeType code: 'trpr'];
    return constantObj;
}

+ (QTConstant *)textStyleInfo {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"textStyleInfo" type: typeType code: 'tsty'];
    return constantObj;
}

+ (QTConstant *)typeClass {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"typeClass" type: typeType code: 'type'];
    return constantObj;
}

+ (QTConstant *)unicodeText {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"unicodeText" type: typeType code: 'utxt'];
    return constantObj;
}

+ (QTConstant *)unsignedInteger {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"unsignedInteger" type: typeType code: 'magn'];
    return constantObj;
}

+ (QTConstant *)utf16Text {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"utf16Text" type: typeType code: 'ut16'];
    return constantObj;
}

+ (QTConstant *)utf8Text {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"utf8Text" type: typeType code: 'utf8'];
    return constantObj;
}

+ (QTConstant *)version {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"version" type: typeType code: 'vers'];
    return constantObj;
}

+ (QTConstant *)version_ {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"version_" type: typeType code: 'vers'];
    return constantObj;
}

+ (QTConstant *)visible {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"visible" type: typeType code: 'pvis'];
    return constantObj;
}

+ (QTConstant *)window {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"window" type: typeType code: 'cwin'];
    return constantObj;
}

+ (QTConstant *)writingCode {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"writingCode" type: typeType code: 'psct'];
    return constantObj;
}

+ (QTConstant *)yards {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"yards" type: typeType code: 'yard'];
    return constantObj;
}

+ (QTConstant *)zoomable {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"zoomable" type: typeType code: 'iszm'];
    return constantObj;
}

+ (QTConstant *)zoomed {
    static QTConstant *constantObj;
    if (!constantObj)
        constantObj = [QTConstant constantWithName: @"zoomed" type: typeType code: 'pzum'];
    return constantObj;
}

@end

