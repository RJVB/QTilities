/*
 * QTAEMConstants.h
 * /Volumes/Debian/Users/bertin/work/src/MacOSX/QTilities/build/Deployment/QTVOD.app
 * osaglue 0.5.4
 *
 */

#import <Foundation/Foundation.h>
#import "Appscript/Appscript.h"

/* Types, enumerators, properties */

enum {
    kQTApplicationResponses = 'rmte',
    kQTAsk = 'ask ',
    kQTCase_ = 'case',
    kQTDetailed = 'lwdt',
    kQTDiacriticals = 'diac',
    kQTExpansion = 'expa',
    kQTHyphens = 'hyph',
    kQTNo = 'no  ',
    kQTNumericStrings = 'nume',
    kQTPunctuation = 'punc',
    kQTStandard = 'lwst',
    kQTWhitespace = 'whit',
    kQTYes = 'yes ',
    kQTApril = 'apr ',
    kQTAugust = 'aug ',
    kQTDecember = 'dec ',
    kQTEPSPicture = 'EPS ',
    kQTFebruary = 'feb ',
    kQTFriday = 'fri ',
    kQTGIFPicture = 'GIFf',
    kQTJPEGPicture = 'JPEG',
    kQTJanuary = 'jan ',
    kQTJuly = 'jul ',
    kQTJune = 'jun ',
    kQTMarch = 'mar ',
    kQTMay = 'may ',
    kQTMonday = 'mon ',
    kQTNovember = 'nov ',
    kQTOctober = 'oct ',
    kQTPICTPicture = 'PICT',
    kQTQTChapter = 'QTCH',
    kQTRGB16Color = 'tr16',
    kQTRGB96Color = 'tr96',
    kQTRGBColor = 'cRGB',
    kQTSaturday = 'sat ',
    kQTSeptember = 'sep ',
    kQTSunday = 'sun ',
    kQTTCframeRate = 'TFRT',
    kQTTIFFPicture = 'TIFF',
    kQTThursday = 'thu ',
    kQTTuesday = 'tue ',
    kQTWednesday = 'wed ',
    kQTAbsCurrentTime = 'ACRT',
    kQTAlias = 'alis',
    kQTAnything = '****',
    kQTApplication = 'capp',
    kQTApplicationBundleID = 'bund',
    kQTApplicationSignature = 'sign',
    kQTApplicationURL = 'aprl',
    kQTBest = 'best',
    kQTBoolean = 'bool',
    kQTBoundingRectangle = 'qdrt',
    kQTBounds = 'pbnd',
    kQTCentimeters = 'cmtr',
    kQTChapterNames = 'CHNS',
    kQTChapterTimes = 'CHTS',
    kQTChapters = 'CHPS',
    kQTClassInfo = 'gcli',
    kQTClass_ = 'pcls',
    kQTCloseable = 'hclb',
    kQTCollating = 'lwcl',
    kQTColorTable = 'clrt',
    kQTCopies = 'lwcp',
    kQTCubicCentimeters = 'ccmt',
    kQTCubicFeet = 'cfet',
    kQTCubicInches = 'cuin',
    kQTCubicMeters = 'cmet',
    kQTCubicYards = 'cyrd',
    kQTCurrentTime = 'CURT',
    kQTDashStyle = 'tdas',
    kQTData = 'rdat',
    kQTDate = 'ldt ',
    kQTDecimalStruct = 'decm',
    kQTDegreesCelsius = 'degc',
    kQTDegreesFahrenheit = 'degf',
    kQTDegreesKelvin = 'degk',
    kQTDocument = 'docu',
    kQTDocumentOrListOfDocument = 'cct0',
    kQTDoubleInteger = 'comp',
    kQTDuration = 'DUTN',
    kQTElementInfo = 'elin',
    kQTEncodedString = 'encs',
    kQTEndingPage = 'lwlp',
    kQTEnumerator = 'enum',
    kQTErrorHandling = 'lweh',
    kQTEventInfo = 'evin',
    kQTExtendedFloat = 'exte',
    kQTFaxNumber = 'faxn',
    kQTFeet = 'feet',
    kQTFile = 'file',
    kQTFileOrListOfFile = 'cct1',
    kQTFileRef = 'fsrf',
    kQTFileSpecification = 'fss ',
    kQTFileURL = 'furl',
    kQTFixed = 'fixd',
    kQTFixedPoint = 'fpnt',
    kQTFixedRectangle = 'frct',
    kQTFloat128bit = 'ldbl',
    kQTFloat_ = 'doub',
    kQTFrameRate = 'FMRT',
    kQTFrontmost = 'pisf',
    kQTGallons = 'galn',
    kQTGrams = 'gram',
    kQTGraphicText = 'cgtx',
    kQTId_ = 'ID  ',
    kQTInches = 'inch',
    kQTIndex = 'pidx',
    kQTInteger = 'long',
    kQTInternationalText = 'itxt',
    kQTInternationalWritingCode = 'intl',
    kQTItem = 'cobj',
    kQTKernelProcessID = 'kpid',
    kQTKilograms = 'kgrm',
    kQTKilometers = 'kmtr',
    kQTLastInterval = 'tINT',
    kQTList = 'list',
    kQTListOfFileOrSpecifier = 'cct2',
    kQTLiters = 'litr',
    kQTLocationReference = 'insl',
    kQTLongFixed = 'lfxd',
    kQTLongFixedPoint = 'lfpt',
    kQTLongFixedRectangle = 'lfrc',
    kQTLongPoint = 'lpnt',
    kQTLongRectangle = 'lrct',
    kQTMachPort = 'port',
    kQTMachine = 'mach',
    kQTMachineLocation = 'mLoc',
    kQTMeters = 'metr',
    kQTMiles = 'mile',
    kQTMiniaturizable = 'ismn',
    kQTMiniaturized = 'pmnd',
    kQTMissingValue = 'msng',
    kQTModified = 'imod',
    kQTMovieView = 'QTmv',
    kQTName = 'pnam',
    kQTNull = 'null',
    kQTOunces = 'ozs ',
    kQTPagesAcross = 'lwla',
    kQTPagesDown = 'lwld',
    kQTParameterInfo = 'pmin',
    kQTPath = 'FTPc',
    kQTPixelMapRecord = 'tpmm',
    kQTPoint = 'QDpt',
    kQTPounds = 'lbs ',
    kQTPrintSettings = 'pset',
    kQTProcessSerialNumber = 'psn ',
    kQTProperties = 'pALL',
    kQTProperty = 'prop',
    kQTPropertyInfo = 'pinf',
    kQTQtMovieView = 'QTMV',
    kQTQuarts = 'qrts',
    kQTRecord = 'reco',
    kQTReference = 'obj ',
    kQTRequestedPrintTime = 'lwqt',
    kQTResizable = 'prsz',
    kQTRotation = 'trot',
    kQTScript = 'scpt',
    kQTShortFloat = 'sing',
    kQTShortInteger = 'shor',
    kQTSquareFeet = 'sqft',
    kQTSquareKilometers = 'sqkm',
    kQTSquareMeters = 'sqrm',
    kQTSquareMiles = 'sqmi',
    kQTSquareYards = 'sqyd',
    kQTStartTime = 'STTM',
    kQTStartingPage = 'lwfp',
    kQTString = 'TEXT',
    kQTStyledClipboardText = 'styl',
    kQTStyledText = 'STXT',
    kQTSuiteInfo = 'suin',
    kQTTargetPrinter = 'trpr',
    kQTTextStyleInfo = 'tsty',
    kQTTypeClass = 'type',
    kQTUnicodeText = 'utxt',
    kQTUnsignedInteger = 'magn',
    kQTUtf16Text = 'ut16',
    kQTUtf8Text = 'utf8',
    kQTVersion = 'vers',
    kQTVersion_ = 'vers',
    kQTVisible = 'pvis',
    kQTWindow = 'cwin',
    kQTWritingCode = 'psct',
    kQTYards = 'yard',
    kQTZoomable = 'iszm',
    kQTZoomed = 'pzum',
};

enum {
    eQTQTChapter = 'QTCH',
    eQTApplications = 'capp',
    eQTDocumentOrListOfDocument = 'cct0',
    eQTDocuments = 'docu',
    eQTFileOrListOfFile = 'cct1',
    eQTItems = 'cobj',
    eQTListOfFileOrSpecifier = 'cct2',
    eQTPrintSettings = 'pset',
    eQTQtMovieViews = 'QTMV',
    eQTWindows = 'cwin',
    pQTTCframeRate = 'TFRT',
    pQTAbsCurrentTime = 'ACRT',
    pQTBounds = 'pbnd',
    pQTChapterNames = 'CHNS',
    pQTChapterTimes = 'CHTS',
    pQTChapters = 'CHPS',
    pQTClass_ = 'pcls',
    pQTCloseable = 'hclb',
    pQTCollating = 'lwcl',
    pQTCopies = 'lwcp',
    pQTCurrentTime = 'CURT',
    pQTDocument = 'docu',
    pQTDuration = 'DUTN',
    pQTEndingPage = 'lwlp',
    pQTErrorHandling = 'lweh',
    pQTFaxNumber = 'faxn',
    pQTFile = 'file',
    pQTFrameRate = 'FMRT',
    pQTFrontmost = 'pisf',
    pQTId_ = 'ID  ',
    pQTIndex = 'pidx',
    pQTLastInterval = 'tINT',
    pQTMiniaturizable = 'ismn',
    pQTMiniaturized = 'pmnd',
    pQTModified = 'imod',
    pQTMovieView = 'QTmv',
    pQTName = 'pnam',
    pQTPagesAcross = 'lwla',
    pQTPagesDown = 'lwld',
    pQTPath = 'FTPc',
    pQTProperties = 'pALL',
    pQTRequestedPrintTime = 'lwqt',
    pQTResizable = 'prsz',
    pQTStartTime = 'STTM',
    pQTStartingPage = 'lwfp',
    pQTTargetPrinter = 'trpr',
    pQTVersion_ = 'vers',
    pQTVisible = 'pvis',
    pQTZoomable = 'iszm',
    pQTZoomed = 'pzum',
};


/* Events */

enum {
    ecQTActivate = 'misc',
    eiQTActivate = 'actv',
};

enum {
    ecQTAddChapter = 'QVOD',
    eiQTAddChapter = 'nchp',
    epQTDuration = 'NCDU',
    epQTName = 'pnam',
    epQTStartTime = 'NCST',
};

enum {
    ecQTClose = 'core',
    eiQTClose = 'clos',
    epQTSaving = 'savo',
    epQTSavingIn = 'kfil',
};

enum {
    ecQTCount = 'core',
    eiQTCount = 'cnte',
};

enum {
    ecQTDelete = 'core',
    eiQTDelete = 'delo',
};

enum {
    ecQTDuplicate = 'core',
    eiQTDuplicate = 'clon',
    epQTTo = 'insh',
    epQTWithProperties = 'prdt',
};

enum {
    ecQTExists = 'core',
    eiQTExists = 'doex',
};

enum {
    ecQTGet = 'core',
    eiQTGet = 'getd',
};

enum {
    ecQTLaunch = 'ascr',
    eiQTLaunch = 'noop',
};

enum {
    ecQTMake = 'core',
    eiQTMake = 'crel',
    epQTAt = 'insh',
    epQTNew_ = 'kocl',
    epQTWithData = 'data',
//  epQTWithProperties = 'prdt',
};

enum {
    ecQTMarkTimeInterval = 'QVOD',
    eiQTMarkTimeInterval = 'tINT',
    epQTDisplay = 'dply',
    epQTReset = 'rset',
};

enum {
    ecQTMove = 'core',
    eiQTMove = 'move',
//  epQTTo = 'insh',
};

enum {
    ecQTOpen = 'aevt',
    eiQTOpen = 'odoc',
};

enum {
    ecQTOpenLocation = 'GURL',
    eiQTOpenLocation = 'GURL',
    epQTWindow = 'WIND',
};

enum {
    ecQTPlay = 'QVOD',
    eiQTPlay = 'strt',
};

enum {
    ecQTPrint = 'aevt',
    eiQTPrint = 'pdoc',
    epQTPrintDialog = 'pdlg',
//  epQTWithProperties = 'prdt',
};

enum {
    ecQTQuit = 'aevt',
    eiQTQuit = 'quit',
//  epQTSaving = 'savo',
};

enum {
    ecQTReopen = 'aevt',
    eiQTReopen = 'rapp',
};

enum {
    ecQTResetVideo = 'QVOD',
    eiQTResetVideo = 'rset',
    epQTComplete = 'cmpl',
};

enum {
    ecQTRun = 'aevt',
    eiQTRun = 'oapp',
};

enum {
    ecQTSave = 'core',
    eiQTSave = 'save',
    epQTAs = 'fltp',
    epQTIn = 'kfil',
};

enum {
    ecQTSet = 'core',
    eiQTSet = 'setd',
//  epQTTo = 'data',
};

enum {
    ecQTStepBackward = 'QVOD',
    eiQTStepBackward = 'pfrm',
};

enum {
    ecQTStepForward = 'QVOD',
    eiQTStepForward = 'nfrm',
};

enum {
    ecQTStop = 'QVOD',
    eiQTStop = 'stop',
};

