#include <Carbon/Carbon.r>

#define Reserved8   reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved
#define Reserved12  Reserved8, reserved, reserved, reserved, reserved
#define Reserved13  Reserved12, reserved
#define dp_none__   noParams, "", directParamOptional, singleItem, notEnumerated, Reserved13
#define reply_none__   noReply, "", replyOptional, singleItem, notEnumerated, Reserved13
#define synonym_verb__ reply_none__, dp_none__, { }
#define plural__    "", {"", kAESpecialClassProperties, cType, "", reserved, singleItem, notEnumerated, readOnly, Reserved8, noApostrophe, notFeminine, notMasculine, plural}, {}

resource 'aete' (0, "QTVOD Terminology") {
	0x1,  // major version
	0x0,  // minor version
	english,
	roman,
	{
		"Type Names Suite",
		"Hidden terms",
		kASTypeNamesSuite,
		1,
		1,
		{
			/* Events */

			"count",
			"Return the number of elements of a particular class within an object.",
			'core', 'cnte',
			'long',
			"The count.",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'obj ',
			"The objects to be counted.",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"each", 'kocl', 'type',
				"The class of objects to be counted.",
				optional,
				singleItem, notEnumerated, Reserved13
			}
		},
		{
			/* Classes */

			"document", 'docu',
			"A QTVOD document.",
			{
				"movieView", 'QTmv', 'QTMV',
				"The movie's QTMovieView",
				reserved, singleItem, notEnumerated, readOnly, Reserved12
			},
			{
			},

			"document or list of document", 't001', "", { }, { },

			"file or list of file", 't000', "", { }, { },

			"list of file or specifier", '****', "", { }, { }
		},
		{
			/* Comparisons */
		},
		{
			/* Enumerations */
		},

		"Standard Suite",
		"Common classes and commands for all applications.",
		'????',
		1,
		1,
		{
			/* Events */

			"open",
			"Open a document.",
			'aevt', 'odoc',
			't001',
			"The opened document(s).",
			replyRequired, singleItem, notEnumerated, Reserved13,
			't000',
			"The file(s) to be opened.",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"close",
			"Close a document.",
			'core', 'clos',
			reply_none__,
			'obj ',
			"the document(s) or window(s) to close.",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"saving", 'savo', 'savo',
				"Should changes be saved before closing?",
				optional,
				singleItem, enumerated, Reserved13,
				"saving in", 'kfil', 'file',
				"The file in which to save the document, if so.",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"save",
			"Save a document.",
			'core', 'save',
			reply_none__,
			'obj ',
			"The document(s) or window(s) to save.",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"in", 'kfil', 'file',
				"The file in which to save the document.",
				optional,
				singleItem, notEnumerated, Reserved13,
				"as", 'fltp', '****',
				"The file format to use.",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"print",
			"Print a document.",
			'aevt', 'pdoc',
			reply_none__,
			'****',
			"The file(s), document(s), or window(s) to be printed.",
			directParamRequired,
			listOfItems, notEnumerated, Reserved13,
			{
				"with properties", 'prdt', 'pset',
				"The print settings to use.",
				optional,
				singleItem, notEnumerated, Reserved13,
				"print dialog", 'pdlg', 'bool',
				"Should the application show the print dialog?",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"quit",
			"Quit the application.",
			'aevt', 'quit',
			reply_none__,
			dp_none__,
			{
				"saving", 'savo', 'savo',
				"Should changes be saved before quitting?",
				optional,
				singleItem, enumerated, Reserved13
			},

			"count",
			"Return the number of elements of a particular class within an object.",
			'core', 'cnte',
			'long',
			"The count.",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'obj ',
			"The objects to be counted.",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"delete",
			"Delete an object.",
			'core', 'delo',
			reply_none__,
			'obj ',
			"The object(s) to delete.",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"duplicate",
			"Copy an object.",
			'core', 'clon',
			reply_none__,
			'obj ',
			"The object(s) to copy.",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"to", 'insh', 'insl',
				"The location for the new copy or copies.",
				optional,
				singleItem, notEnumerated, Reserved13,
				"with properties", 'prdt', 'reco',
				"Properties to set in the new copy or copies right away.",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"exists",
			"Verify that an object exists.",
			'core', 'doex',
			'bool',
			"Did the object(s) exist?",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'****',
			"The object(s) to check.",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"make",
			"Create a new object.",
			'core', 'crel',
			'obj ',
			"The new object.",
			replyRequired, singleItem, notEnumerated, Reserved13,
			dp_none__,
			{
				"new", 'kocl', 'type',
				"The class of the new object.",
				required,
				singleItem, notEnumerated, Reserved13,
				"at", 'insh', 'insl',
				"The location at which to insert the object.",
				optional,
				singleItem, notEnumerated, Reserved13,
				"with data", 'data', '****',
				"The initial contents of the object.",
				optional,
				singleItem, notEnumerated, Reserved13,
				"with properties", 'prdt', 'reco',
				"The initial values for properties of the object.",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"move",
			"Move an object to a new location.",
			'core', 'move',
			reply_none__,
			'obj ',
			"The object(s) to move.",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"to", 'insh', 'insl',
				"The new location for the object(s).",
				required,
				singleItem, notEnumerated, Reserved13
			}
		},
		{
			/* Classes */

			"print settings", 'pset',
			"",
			{
				"copies", 'lwcp', 'long',
				"the number of copies of a document to be printed",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"collating", 'lwcl', 'bool',
				"Should printed copies be collated?",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"starting page", 'lwfp', 'long',
				"the first page of the document to be printed",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"ending page", 'lwlp', 'long',
				"the last page of the document to be printed",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"pages across", 'lwla', 'long',
				"number of logical pages laid across a physical page",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"pages down", 'lwld', 'long',
				"number of logical pages laid out down a physical page",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"requested print time", 'lwqt', 'ldt ',
				"the time at which the desktop printer should print the document",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"error handling", 'lweh', 'enum',
				"how errors are handled",
				reserved, singleItem, enumerated, readWrite, Reserved12,

				"fax number", 'faxn', 'TEXT',
				"for fax number",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"target printer", 'trpr', 'TEXT',
				"for target printer",
				reserved, singleItem, notEnumerated, readWrite, Reserved12
			},
			{
			},

			"application", 'capp',
			"The application's top-level scripting object.",
			{
				"name", 'pnam', 'TEXT',
				"The name of the application.",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"frontmost", 'pisf', 'bool',
				"Is this the active application?",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"version", 'vers', 'TEXT',
				"The version number of the application.",
				reserved, singleItem, notEnumerated, readOnly, Reserved12
			},
			{
				'docu', { },
				'cwin', { }
			},
			"applications", 'capp', plural__,

			"document", 'docu',
			"A document.",
			{
				"name", 'pnam', 'TEXT',
				"Its name.",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"modified", 'imod', 'bool',
				"Has it been modified since the last save?",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"file", 'file', 'file',
				"Its location on disk, if it has one.",
				reserved, singleItem, notEnumerated, readOnly, Reserved12
			},
			{
			},
			"documents", 'docu', plural__,

			"window", 'cwin',
			"A window.",
			{
				"name", 'pnam', 'TEXT',
				"The title of the window.",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"id", 'ID  ', 'long',
				"The unique identifier of the window.",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"index", 'pidx', 'long',
				"The index of the window, ordered front to back.",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"bounds", 'pbnd', 'qdrt',
				"The bounding rectangle of the window.",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"closeable", 'hclb', 'bool',
				"Does the window have a close button?",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"miniaturizable", 'ismn', 'bool',
				"Does the window have a minimize button?",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"miniaturized", 'pmnd', 'bool',
				"Is the window minimized right now?",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"resizable", 'prsz', 'bool',
				"Can the window be resized?",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"visible", 'pvis', 'bool',
				"Is the window visible right now?",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"zoomable", 'iszm', 'bool',
				"Does the window have a zoom button?",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"zoomed", 'pzum', 'bool',
				"Is the window zoomed right now?",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"document", 'docu', 'docu',
				"The document whose contents are displayed in the window.",
				reserved, singleItem, notEnumerated, readOnly, Reserved12
			},
			{
			},
			"windows", 'cwin', plural__
		},
		{
			/* Comparisons */
		},
		{
			/* Enumerations */
			'savo',
			{
				"yes", 'yes ', "Save the file.",
				"no", 'no  ', "Do not save the file.",
				"ask", 'ask ', "Ask the user whether or not to save the file."
			},

			'enum',
			{
				"standard", 'lwst', "Standard PostScript error handling",
				"detailed", 'lwdt', "print a detailed report of PostScript errors"
			}
		},

		"QTVOD Scripting Suite",
		"Class and properties for QTVOD.app",
		'QVOD',
		1,
		1,
		{
			/* Events */

			"close",
			"close the document",
			'QVOD', 'clse',
			reply_none__,
			'docu',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"play",
			"start playing",
			'QVOD', 'strt',
			reply_none__,
			'docu',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"stop",
			"stop playing",
			'QVOD', 'stop',
			reply_none__,
			'docu',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"stepForward",
			"step 1 frame forward",
			'QVOD', 'nfrm',
			reply_none__,
			'docu',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"stepBackward",
			"step 1 frame backward",
			'QVOD', 'pfrm',
			reply_none__,
			'docu',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"addChapter",
			"add a new movie chapter",
			'QVOD', 'nchp',
			reply_none__,
			'docu',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"name", 'pnam', 'TEXT',
				"new chapter title",
				required,
				singleItem, notEnumerated, Reserved13,
				"startTime", 'NCST', 'doub',
				"new chapter's starting time in seconds",
				required,
				singleItem, notEnumerated, Reserved13,
				"duration", 'NCDU', 'doub',
				"new chapter's duration",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"markTimeInterval",
			"marks a new reference frame for time interval measurement",
			'QVOD', 'tINT',
			reply_none__,
			'docu',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"reset", 'rset', 'bool',
				"start a new interval - otherwise the previous 2nd reference frame becomes the 1st",
				optional,
				singleItem, notEnumerated, Reserved13,
				"display", 'dply', 'bool',
				"whether they host (QTVOD) should display the measured interval duration in a popup window",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"resetVideo",
			"reloads the video, possible after trashing the main cache file (complete=True). This command is not very reliable and to be avoided: prefer instead to get the document's currentTime, close it and reopen the file and finally set the currentTime to the saved value.",
			'QVOD', 'rset',
			reply_none__,
			'docu',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"complete", 'cmpl', 'bool',
				"whether to trash the main movie cache file during a reset",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"readDesign",
			"reads a design from an XML design file",
			'QVOD', 'dsgn',
			reply_none__,
			'docu',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"name", 'pnam', 'TEXT',
				"the name of the file to parse",
				required,
				singleItem, notEnumerated, Reserved13
			},

			"connectToServer",
			"connects to the specified server",
			'QVOD', 'srvr',
			reply_none__,
			'SCRP',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"address", 'ADDR', 'TEXT',
				"server IP4 address",
				required,
				singleItem, notEnumerated, Reserved13
			},

			"toggleLogging",
			"toggles the logging facility",
			'QVOD', 'tlog',
			reply_none__,
			'SCRP',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			}
		},
		{
			/* Classes */

			"QTChapter", 'QTCH',
			"",
			{
				"name", 'pnam', 'TEXT',
				"chapter name",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"startTime", 'QCTM', 'nmbr',
				"the chapter's start time",
				reserved, singleItem, notEnumerated, readOnly, Reserved12
			},
			{
			},

			"document", 'docu',
			"A QTVOD document.",
			{
				"name", 'pnam', 'TEXT',
				"Its name.",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"file", 'file', 'file',
				"Its file on disk",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"id", 'ID  ', 'TEXT',
				"Its unique ID",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"path", 'FTPc', 'TEXT',
				"Its location on disk, if it has one.",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"currentTime", 'CURT', 'doub',
				"The current time in seconds from the movie start",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"absCurrentTime", 'ACRT', 'doub',
				"The current time in absolute seconds, i.e. 3600 is 1h am. Obtained from the TimeCode track",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"duration", 'DUTN', 'doub',
				"The movie's duration",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"startTime", 'STTM', 'doub',
				"The movie's starting time",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"frameRate", 'FMRT', 'doub',
				"The movie's frame rate",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"TCframeRate", 'TFRT', 'doub',
				"The movie's frame rate based on TimeCode info",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"lastInterval", 'tINT', 'doub',
				"the last measured time interval between 2 frames",
				reserved, singleItem, notEnumerated, readOnly, Reserved12,

				"chapterNames", 'CHNS', 'TEXT',
				"the movie's chapter names list",
				reserved, listOfItems, notEnumerated, readOnly, Reserved12,

				"chapterTimes", 'CHTS', 'doub',
				"the movie's chapter times list",
				reserved, listOfItems, notEnumerated, readOnly, Reserved12,

				"chapters", 'CHPS', 'QTCH',
				"the movie's chapter list",
				reserved, listOfItems, notEnumerated, readOnly, Reserved12
			},
			{
			},
			"documents", 'docu', plural__,

			"qtMovieView", 'QTMV',
			"The Cocoa QTMovieView class",
			{
			},
			{
			},
			"qtMovieViews", 'QTMV', plural__,

			"QTVODScripting", 'SCRP',
			"QTVOD top level scripting object",
			{
			},
			{
			},
			"QTVODScriptings", 'SCRP', plural__
		},
		{
			/* Comparisons */
		},
		{
			/* Enumerations */
		}
	}
};
