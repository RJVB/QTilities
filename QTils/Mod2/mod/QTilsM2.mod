IMPLEMENTATION MODULE QTilsM2;

(* definir QTILS_DEV pour charger la librairie QTils-dev.dll au lieu de QTils.dll *)
<*/VALIDVERSION:QTILS_DEV,QTILS_DEBUG*>

FROM SYSTEM IMPORT
	CAST, ADR, ADDRESS, TSIZE, VA_START, ADDADR;

FROM Environment IMPORT
	GetCommandLine;

%IF WIN32 %THEN
	FROM WIN32 IMPORT
		FARPROC,
		(*IsBadCodePtr,*)
		GetProcAddress,
		HANDLE, FormatMessage,
		FORMAT_MESSAGE_FROM_SYSTEM, FORMAT_MESSAGE_IGNORE_INSERTS, FORMAT_MESSAGE_MAX_WIDTH_MASK,
		LoadLibrary, FreeLibrary;
	FROM WINX IMPORT
		NULL_HANDLE;
%END

FROM WINUSER IMPORT
	MessageBox, MB_OK, MB_ICONEXCLAMATION, MB_APPLMODAL, MB_ICONQUESTION, MB_YESNO, IDOK, IDYES;

FROM Strings IMPORT
	Assign, Length, FindNext, Equal, Concat;

(*
FROM FormatString IMPORT
	FormatString, FormatStringEx;
*)

(* ===== création de l'interface avec QTils.dll ==== *)
TYPE

	QTCompressionCodecPtr = POINTER TO QTCompressionCodecs;
	QTCompressionQualityPtr = POINTER TO QTCompressionQualities;
	(* NB: il semble qu'il vaut mieux déclarer tout [CDECL] et non [StdCall]! *)
	LibQTilsBase =
		RECORD
			(* fonctions privées *)
			cOpenQT : PROCEDURE() :ErrCode [CDECL];
			cCloseQT : PROCEDURE [CDECL];

			QTCompressionCodec : PROCEDURE() :QTCompressionCodecPtr [CDECL];
			QTCompressionQuality : PROCEDURE() :QTCompressionQualityPtr [CDECL];

			cMCActionListProc : PROCEDURE() :MCActionsPtr [CDECL];
			(* vérifie si l'argument est un QTMovieWindowH valide *)
			cQTMovieWindowH_Check : PROCEDURE(QTMovieWindowH) :Int32 [CDECL];
			cOpenQTMovieInWindow : PROCEDURE(VAR ARRAY OF CHAR,Int32) :QTMovieWindowH [CDECL];
			(* ferme une QTMovieWindow mais garde l'objet. Cette fonction est
			 * appelée par la QTils.DLL, quand l'utilisateur ferme la fenêtre.
			 * Dans le code, on peut appeler DisposeQTMovieWindowH() directement.
			 * 20110112: sauf dans une MCActionCallback!!
			 *)
			cCloseQTMovieWindow : PROCEDURE(QTMovieWindowH) :ErrCode [CDECL];
			cLogMsgEx : PROCEDURE((*msg*) ARRAY OF CHAR, (*va_list ap*) ADDRESS) :UInt32 [CDECL];

			cvsscanf : PROCEDURE((*source*) ARRAY OF CHAR, (*format*) ARRAY OF CHAR,
														(*va_list ap*) ADDRESS) :Int32 [CDECL];
			cvsprintf : PROCEDURE(VAR (*dest*) ARRAY OF CHAR, (*format*) ARRAY OF CHAR,
														(*va_list ap*) ADDRESS) :Int32 [CDECL];
			cvssprintf : PROCEDURE(VAR (*dest*) String1kPtr, (*format*) ARRAY OF CHAR,
														(*va_list ap*) ADDRESS) :Int32 [CDECL];
			cvssprintfAppend : PROCEDURE(VAR (*dest*) String1kPtr, (*format*) ARRAY OF CHAR,
														(*va_list ap*) ADDRESS) :Int32 [CDECL];

			(* *** fonctions exportées *** *)
			cQTils : QTilsExports;
		END;
	LibBaseGetter = PROCEDURE(VAR LibQTilsBase) :UInt32 [CDECL];
	AnnotationKeysGetter = PROCEDURE(VAR AnnotationKeys) :UInt32 [CDECL];

CONST

	vraiStr = "VRAI";
	fauxStr = "FAUX";
	NUM_MOD_FUNCTIONS = 19;

TYPE

	XML_PARSEDVALUE_TYPES = ( parsedvalue_string, parsedvalue_integer, parsedvalue_boolean, parsedvalue_double );

	XML_PARSEDVALUE =
		RECORD
			CASE : XML_PARSEDVALUE_TYPES OF
				parsedvalue_string :
					stringValue : POINTER TO ARRAY OF CHAR;
				| parsedvalue_integer :
					integerValue : POINTER TO Int32;
				| parsedvalue_boolean :
					booleanValue : POINTER TO UInt8;
				| parsedvalue_double :
					doubleValue : POINTER TO Real64;
			END;
	END;

VAR

	lQTOpened : BOOLEAN;
	lQTOpenError : ErrCode;
	QTilsHandle : HANDLE;
	QTilsC : LibQTilsBase;
	DevMode : BOOLEAN;
	ligneDeCmde : BOOLEAN;
	QTilsDLL : Str255;

PROCEDURE QTilsAvailable() : BOOLEAN;
BEGIN
	RETURN QTilsHandle <> NULL_HANDLE;
END QTilsAvailable;

PROCEDURE QTOpened() : BOOLEAN;
BEGIN
	RETURN lQTOpened;
END QTOpened;

PROCEDURE QTOpenError() : ErrCode;
BEGIN
	RETURN lQTOpenError;
END QTOpenError;

PROCEDURE FOUR_CHAR_CODE_be(code : OSTypeStr) : OSType;
VAR
	ot : POINTER TO OSType;
BEGIN
	ot := ADR(code);
	RETURN ot^;
END FOUR_CHAR_CODE_be;

PROCEDURE FOUR_CHAR_CODE(code : OSTypeStr) : OSType;
BEGIN
	RETURN (VAL(OSType,code[0]) SHL 24) + (VAL(OSType,code[1]) SHL 16)
		+ (VAL(OSType,code[2]) SHL 8) + VAL(OSType,code[3]);
END FOUR_CHAR_CODE;

PROCEDURE OSTStr_be(otype : OSType) : OSTypeStr;
VAR
	code : POINTER TO OSTypeStr;
BEGIN
	code := ADR(otype);
	code^[4] := CHR(0);
	RETURN code^;
END OSTStr_be;

PROCEDURE OSTStr(otype : OSType) : OSTypeStr;
VAR
	c : POINTER TO OSTypeStr;
	code : OSTypeStr;
BEGIN
	c := ADR(otype);
	code[0] := c^[3];
	code[1] := c^[2];
	code[2] := c^[1];
	code[3] := c^[0];
	code[4] := CHR(0);
	RETURN code;
END OSTStr;

PROCEDURE BoolStr(state : BOOLEAN) : String5b;
BEGIN
	IF state
		THEN
			RETURN vraiStr;
		ELSE
			RETURN fauxStr;
	END;
END BoolStr;

(*
PROCEDURE StringTerminate(VAR str : ARRAY OF CHAR);
BEGIN
	str[ Length(str) ] := CHR(0);
END StringTerminate;
*)

(*
PROCEDURE PostMessage(title, message : ARRAY OF CHAR) : CARDINAL;
BEGIN
	IF NOT ligneDeCmde
		THEN
			RETURN MessageBox(NIL, message, title, MB_APPLMODAL BOR MB_OK BOR MB_ICONEXCLAMATION)
		ELSE
			RETURN 1
	END;
END PostMessage;
*)

PROCEDURE PostMessage(title, message : ARRAY OF CHAR) : CARDINAL;
BEGIN
	RETURN MessageBox(NIL, message, title, MB_APPLMODAL BOR MB_OK BOR MB_ICONEXCLAMATION)
END PostMessage;

PROCEDURE PostYesNoDialog(title, message : ARRAY OF CHAR) : BOOLEAN;
VAR
	mb : CARDINAL;
BEGIN
	mb := MessageBox(NIL, message, title, MB_APPLMODAL BOR MB_ICONQUESTION BOR MB_YESNO);
	RETURN ((mb = IDOK) OR (mb = IDYES))
END PostYesNoDialog;

(* obtient l'adresse de <symbol> dans la DLL <handle> *)
PROCEDURE dlsym(handle : HANDLE; symbol : ARRAY OF CHAR; VAR valid : BOOLEAN) : PROC;
%IF WIN32 %THEN
	VAR
		procAdr : FARPROC;
	BEGIN
		procAdr := GetProcAddress(handle, symbol);
		IF procAdr = CAST(FARPROC, NIL)
			THEN
				valid := FALSE;
				PostMessage("Fonction introuvable dans QTils.dll", symbol);
			ELSE
				(*valid := NOT(IsBadCodePtr(procAdr));*)
				(* IsBadCodePtr should no longer be used! *)
				valid := TRUE;
			END;
		RETURN CAST(PROC, procAdr)
%ELSE
	BEGIN
		RETURN CAST(PROC, NIL)
	%END
END dlsym;

(* ===== code exporté ===== *)

PROCEDURE OpenQT() : ErrCode;
BEGIN
	IF QTilsHandle <> NULL_HANDLE
		THEN
			lQTOpenError := QTilsC.cOpenQT();
		ELSE
			lQTOpenError := 1;
	END;
	IF lQTOpenError = noErr
		THEN
			lQTOpened := TRUE;
		ELSE
			lQTOpened := FALSE;
	END;
	RETURN lQTOpenError;
END OpenQT;

PROCEDURE CloseQT;
BEGIN
	IF lQTOpened
		THEN
			QTilsC.cCloseQT;
			lQTOpened := FALSE;
	END;
END CloseQT;

(*
		vérifie si l'argument est un QTMovieWindowH minimalement valide. Un QTMovieWindowH
		alloué en interne par OpenMovieFromURL ne passera pas le QTMovieWindowH_Check ...
*)
PROCEDURE QTMovieWindowH_BasicCheck(wih : QTMovieWindowH) : BOOLEAN;
BEGIN
	IF (wih <> NIL) AND (wih^ <> NIL) AND (QTils.QTMovieWindowHFromMovie(wih^^.theMovie) = wih)
		THEN
			RETURN TRUE;
		ELSE
			RETURN FALSE;
	END;
END QTMovieWindowH_BasicCheck;
(*
		vérifie si l'argument est un QTMovieWindowH valide qui est ou
		a été ouvert dans une fenêtre
*)
PROCEDURE QTMovieWindowH_Check(wih : QTMovieWindowH) : BOOLEAN;
BEGIN
	RETURN (QTilsC.cQTMovieWindowH_Check(wih) <> 0);
END QTMovieWindowH_Check;

(* vérifie si <wih> est un QTMovieWindowH valabe et avec la fenêtre ouverte *)
PROCEDURE QTMovieWindowH_isOpen(wih : QTMovieWindowH) : BOOLEAN;
BEGIN
	RETURN (QTMovieWindowH_Check(wih) & (wih^^.theView <> NULL_NativeWindow));
END QTMovieWindowH_isOpen;

(* ouvre le movie <theURL> dans une fenêtre en utilisant QuickTime (si ce dernier a été
 * initialisé) via la fonctionalité de QTils.DLL. Retourne un QTMovieWindow handle
 * (c-a-d un POINTER TO POINTER OF QTMovieWindow). Comme OpenQTMovieInWindow est la seule manière
 * d'obtenir un QTMovieWindowH valable, il suffit de tester l'initialisation réussie de QuickTime ici.
 *)
PROCEDURE OpenQTMovieInWindow(theURL : ARRAY OF CHAR; withController : Int32) : QTMovieWindowH;
BEGIN
	IF lQTOpened
		THEN
			RETURN QTilsC.cOpenQTMovieInWindow(theURL, withController);
			(* on doit stocker le resultat de l'appel dans une variable locale plutôt que
				de retourner directement si cOpenQTMovieInWindow a été déclaré [StdCall] au lieu
				de [CDECL]!
			wih := QTilsC.cOpenQTMovieInWindow(theURL, withController);
			RETURN wih;
			*)
		ELSE
			PostMessage(theURL, "QuickTime non disponible!");
			RETURN NULL_QTMovieWindowH;
		END;
END OpenQTMovieInWindow;

PROCEDURE CloseQTMovieWindow(wih : QTMovieWindowH) : ErrCode;
BEGIN
	RETURN QTilsC.cCloseQTMovieWindow(wih);
END CloseQTMovieWindow;

PROCEDURE FTTS(VAR ft : MovieFrameTime; frameRate : Real64) : Real64;
BEGIN
	RETURN VAL(Real64, ft.hours) * 3600.0 + VAL(Real64, ft.minutes) * 60.0 + VAL(Real64, ft.seconds)
		+ VAL(Real64, ft.frames) / frameRate;
END FTTS;

PROCEDURE MSWinErrorString(err : INTEGER; VAR errStr : ARRAY OF CHAR) : UInt32;
VAR
	ret : UInt32;
BEGIN
%IF WIN32 %THEN
	IF err = 0
		THEN
%END
			Assign("", errStr);
			ret := 0;
%IF WIN32 %THEN
		ELSE
			ret := FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
				BOR FORMAT_MESSAGE_IGNORE_INSERTS BOR FORMAT_MESSAGE_MAX_WIDTH_MASK,
				NIL, err, 0, errStr, HIGH(errStr), NIL);
	END;
%END
	RETURN ret;
END MSWinErrorString;

PROCEDURE lLogMsg(msg : ARRAY OF CHAR) : UInt32 [CDECL];
BEGIN
	IF DevMode
		THEN
			PostMessage("LogMsg", msg);
	END;
	RETURN Length(msg);
END lLogMsg;

PROCEDURE LogMsgEx(formatStr : ARRAY OF CHAR) : UInt32 [RightToLeft, LEAVES, VARIABLE];
VAR
	argsList : ADDRESS;
	ret : UInt32;
(*
	msg : ARRAY[0..2048] OF CHAR;
*)
BEGIN
	IF (QTilsHandle <> NULL_HANDLE)
		THEN
			VA_START(argsList); (* <= to get the variable arguments base address *)
(*
			IF FormatStringEx(formatStr, msg, argsList)
*)
			ret := QTilsC.cLogMsgEx(formatStr, argsList);
		ELSE
			(* on pourrait utiliser wvsprintf ici pour générer le message voulu... *)
			IF DevMode
				THEN
					PostMessage("LogMsgEx", formatStr);
			END;
			ret := Length(formatStr);
		END;
	RETURN ret;
END LogMsgEx;

PROCEDURE vsscanf(source, formatStr : ARRAY OF CHAR) : Int32 [RightToLeft, LEAVES, VARIABLE];
VAR
	argsList : ADDRESS;
	ret : Int32;
BEGIN
	IF (QTilsHandle <> NULL_HANDLE)
		THEN
			VA_START(argsList); (* <= to get the variable arguments base address *)
			ret := QTilsC.cvsscanf(source, formatStr, argsList);
		ELSE
			ret := -2;
		END;
	RETURN ret;
END vsscanf;

PROCEDURE vsprintf(VAR dest : ARRAY OF CHAR; formatStr : ARRAY OF CHAR) : Int32 [RightToLeft, LEAVES, VARIABLE];
VAR
	argsList : ADDRESS;
	ret : Int32;
BEGIN
	IF (QTilsHandle <> NULL_HANDLE)
		THEN
			VA_START(argsList); (* <= to get the variable arguments base address *)
			ret := QTilsC.cvsprintf(dest, formatStr, argsList);
		ELSE
			ret := -2;
		END;
	RETURN ret;
END vsprintf;

PROCEDURE vssprintf( VAR dest : String1kPtr; formatStr : ARRAY OF CHAR) : Int32 [RightToLeft, LEAVES, VARIABLE];
VAR
	argsList : ADDRESS;
	ret : Int32;
BEGIN
	IF (QTilsHandle <> NULL_HANDLE)
		THEN
			VA_START(argsList); (* <= to get the variable arguments base address *)
			ret := QTilsC.cvssprintf(dest, formatStr, argsList);
		ELSE
			ret := -2;
		END;
	RETURN ret;
END vssprintf;

PROCEDURE vssprintfAppend( VAR dest : String1kPtr; formatStr : ARRAY OF CHAR) : Int32 [RightToLeft, LEAVES, VARIABLE];
VAR
	argsList : ADDRESS;
	ret : Int32;
BEGIN
	IF (QTilsHandle <> NULL_HANDLE)
		THEN
			VA_START(argsList); (* <= to get the variable arguments base address *)
			ret := QTilsC.cvssprintfAppend(dest, formatStr, argsList);
		ELSE
			ret := -2;
		END;
	RETURN ret;
END vssprintfAppend;

PROCEDURE XMLRootElementID(xmldoc : XMLDoc) : UInt32;
BEGIN
	IF xmldoc <> NIL
		THEN
			RETURN xmldoc^.rootElement.identifier;
		ELSE
			RETURN 0;
	END;
END XMLRootElementID;

PROCEDURE XMLContentKind(theContent : XMLContentArrayPtr; element : CARDINAL) : UInt32;
VAR
	kind : UInt32;
BEGIN
	IF theContent <> NIL
		THEN
			kind := theContent^[element].kind;
		ELSE
			kind := xmlIdentifierInvalid; (* = xmlContentTypeInvalid *)
	END;
	RETURN kind;
END XMLContentKind;

PROCEDURE XMLRootElementContentKind(xmldoc : XMLDoc; element : CARDINAL) : UInt32;
VAR
	kind : UInt32;
	root_content : XMLContentArrayPtr;
BEGIN
	IF xmldoc <> NIL
		THEN
			root_content := CAST(XMLContentArrayPtr, xmldoc^.rootElement.contents);
			kind := XMLContentKind(root_content, element);
		ELSE
			kind := xmlIdentifierInvalid; (* = xmlContentTypeInvalid *)
	END;
	RETURN kind;
END XMLRootElementContentKind;

PROCEDURE XMLContentOfElementOfRootElement(xmldoc : XMLDoc; element : CARDINAL;
	VAR theContent : XMLContentArrayPtr) : BOOLEAN;
VAR
	ret : BOOLEAN;
BEGIN
	IF xmldoc <> NIL
		THEN
			theContent := CAST(XMLContentArrayPtr, xmldoc^.rootElement.contents);
			ret := TRUE;
		ELSE
			ret := FALSE;
	END;
	RETURN ret;
END XMLContentOfElementOfRootElement;

PROCEDURE XMLContentOfElement(VAR parentElement : XMLElement; VAR theElements : XMLContentArrayPtr) : BOOLEAN;
VAR
	ret : BOOLEAN;
BEGIN
	IF (parentElement.contents <> NIL) AND (parentElement.contents^.kind = xmlContentTypeElement)
		THEN
			theElements := CAST(XMLContentArrayPtr,parentElement.contents);
			ret := TRUE;
		ELSE
			ret := FALSE;
	END;
	RETURN ret;
END XMLContentOfElement;

PROCEDURE XMLElementOfContent(theContent : XMLContentArrayPtr;
	element : CARDINAL; VAR theElement : XMLElement) : BOOLEAN;
VAR
	ret : BOOLEAN;
BEGIN
	IF theContent^[element].kind = xmlContentTypeElement
		THEN
			theElement := theContent^[element].actualContent.element;
			ret := TRUE;
		ELSE
			ret := FALSE;
	END;
	RETURN ret;
END XMLElementOfContent;

PROCEDURE CreateXMLParser( VAR xmlParser : ComponentInstance; xml_design_parser : ARRAY OF XML_RECORD;  VAR errors :  Int32 ) : ErrCode;
VAR
	xmlErr : ErrCode;
	i, N : CARDINAL;
	lastElement : UInt32;
BEGIN

	(*
		Le nombre d'éléments dans le tableau d'initialisation:
	*)
	N := HIGH(xml_design_parser);

	errors := 0;
  lastElement := 0;

	QTils.XMLnameSpaceID := nameSpaceIDNone;
	FOR i := 0 TO N-1 DO
		CASE xml_design_parser[i].itemType OF
			xml_element :
					xmlErr := QTils.XMLParserAddElement( xmlParser,
						xml_design_parser[i].elementTag, xml_design_parser[i].elementID, 0 );
					IF xmlErr = noErr
						THEN
							lastElement := xml_design_parser[i].elementID;
						ELSE
							INC(errors,1);
					END;
			| xml_attribute :
					xmlErr := QTils.XMLParserAddElementAttribute( xmlParser, lastElement,
						xml_design_parser[i].attributeTag, xml_design_parser[i].attributeID,
						xml_design_parser[i].attributeType );
					IF xmlErr = noErr
						THEN
						ELSE
							INC(errors,1);
							QTils.LogMsgEx( "Erreur de définition d'attribut #%d '%s' type %d pour élément #%d: err=%d",
								xml_design_parser[i].attributeID, xml_design_parser[i].attributeTag,
								xml_design_parser[i].attributeType, lastElement, xmlErr );
					END;
		END;
	END;

	RETURN xmlErr;
END CreateXMLParser;

PROCEDURE ReadXMLContent( fName : ARRAY OF CHAR; theContent : XMLContentArrayPtr;
														design : ARRAY OF XML_RECORD; VAR elm : CARDINAL );
VAR
	xmlErr : ErrCode;
	bool : UInt8;
	element_content : XMLContentArrayPtr;
	theElement : XMLElement;

	(*
		la procédure qui lit chaque élément individuel; les attributs, ou les (sous) éléments.
	*)
	PROCEDURE ReadXMLElementAttributes( theElement : XMLElement ) : ErrCode;
	VAR
		idx, idx2, elem : CARDINAL;
		attrs : XMLAttributeArrayPtr;
		val : XML_PARSEDVALUE;
	BEGIN
		IF theElement.identifier = xmlIdentifierUnrecognized
			THEN
				IF ( (theElement.name <> NIL) AND (theElement.attributes^.name <> NIL) )
					THEN
						QTils.LogMsgEx( "élément inconnu <%s %s /> rencontré dans %s",
							theElement.name, theElement.attributes^.name, fName );
					ELSE
						QTils.LogMsgEx( "élément inconnu rencontré dans %s", fName );
				END;
				PostMessage( "QTVODm2", QTils.lastSSLogMsg^ );
				RETURN xmlErr;
		END;
		idx := 0;
		attrs := CAST(XMLAttributeArrayPtr, theElement.attributes);
		WHILE idx <= HIGH(design) DO
			IF (design[idx].itemType = xml_element) AND (theElement.identifier = design[idx].elementID)
				THEN
					IF QTils.XMLContentOfElement( theElement, element_content )
						THEN
							(*
								ceci est le 'root element', la racine, qui n'a que des éléments, pas d'attributs. Si le fichier
								contient l'élément racine ailleurs qu'à la racine, le membre 'contents' de theELement sera un pointer
								vers un array de XMLContent, juste comme xmldoc^.rootElement.contents .
								On peut donc appeler ReadXMLContent de façon récursive.
								Bien sur il n'y a aucune utilité à pouvoir écrire des fichiers avec une liste de paramètres de configuration
								de façon récursive...
							*)
							idx2 := 0;
							ReadXMLContent( fName, element_content, design, idx2 );
					END;
					INC(idx,1);
					IF design[idx].itemType = xml_attribute
						THEN
							elem := 0;
							WHILE (idx <= HIGH(design)) AND (attrs^[elem].identifier <> xmlIdentifierInvalid) AND (design[idx].itemType = xml_attribute) DO
								IF attrs^[elem].identifier = design[idx].attributeID
									THEN
										IF ((NOT design[idx].useHandler) AND (design[idx].parsed = NIL))
											THEN
												QTils.LogMsgEx( ">> attr #%d pour tag %s ne contient pas de pointeur valide pour recevoir le resultat (entrée %d)", design[idx].attributeID,
															design[idx].attributeTag, idx );
												RETURN paramErr;
										END;
										IF (design[idx].useHandler AND  (design[idx].handler = NULL_XMLAttributeParseCallback))
											THEN
												QTils.LogMsgEx( ">> attr #%d pour tag %s ne contient pas de callback valide pour lire le resultat (entrée %d)", design[idx].attributeID,
															design[idx].attributeTag, idx );
												RETURN paramErr;
										END;
										IF design[idx].useHandler
											THEN
												xmlErr := design[idx].handler( theElement, elem, elm, design, idx, fName );
											ELSE
												val.integerValue := design[idx].parsed;
												CASE design[idx].attributeType OF
													recordAttributeValueTypeCharString :
														xmlErr := QTils.GetStringAttribute( theElement, design[idx].attributeID, val.stringValue^ );
														IF xmlErr <> attributeNotFound
															THEN
																QTils.LogMsgEx( "> attr #%d %s=%s (%d)", design[idx].attributeID,
																			design[idx].attributeTag, val.stringValue^, xmlErr );
														END;
													| recordAttributeValueTypeInteger, recordAttributeValueTypePercent :
														xmlErr := QTils.GetIntegerAttribute( theElement, design[idx].attributeID, val.integerValue^ );
														IF xmlErr <> attributeNotFound
															THEN
																QTils.LogMsgEx( "> attr #%d %s=%d (%d)", design[idx].attributeID,
																			design[idx].attributeTag, val.integerValue^, xmlErr );
														END;
													| recordAttributeValueTypeBoolean, recordAttributeValueTypeOnOff :
														xmlErr := QTils.GetBooleanAttribute( theElement, design[idx].attributeID, val.booleanValue^ );
														IF xmlErr <> attributeNotFound
															THEN
																QTils.LogMsgEx( "> attr #%d %s=%hd (%d)", design[idx].attributeID,
																			design[idx].attributeTag, VAL(Int16,val.booleanValue^), xmlErr );
														END;
													| recordAttributeValueTypeDouble :
														xmlErr := QTils.GetDoubleAttribute( theElement, design[idx].attributeID, val.doubleValue^ );
														IF xmlErr <> attributeNotFound
															THEN
																QTils.LogMsgEx( "> attr #%d %s=%g (%d)", design[idx].attributeID,
																			design[idx].attributeTag, val.doubleValue^, xmlErr );
														END;
													ELSE
														QTils.LogMsgEx( ">> attr #%d pour tag %s inconnu", design[idx].attributeID,
																	design[idx].attributeTag );
												END;
										END;
								END;
								INC(idx,1);
								INC(elem,1);
							END;
					END;
				ELSE
					INC(idx,1);
			END;
		END;
		RETURN xmlErr;
	END ReadXMLElementAttributes;

BEGIN
	(* tant qu'on trouve du contenu valide... *)
	WHILE ( QTils.XMLContentKind( theContent, elm ) <> xmlContentTypeInvalid ) DO
		(* et pour chaque élément trouvé (on ne gère pas le cas xmlContentTypeCharData) *)
		IF QTils.XMLElementOfContent( theContent, elm, theElement )
			THEN
				(* on lit les attributs qu'on connait de chaque élément *)
				QTils.LogMsgEx( "Scanne attributs et/ou éléments de l'élément #%d (entrée %d)", theElement.identifier, elm );
				ReadXMLElementAttributes(theElement);
		END;
		INC(elm);
	END;
END ReadXMLContent;

(* initialisation du module QTilsM2 *)
TYPE

	CH_CMDE = ARRAY[0..1023] OF CHAR;
	P_CH_CMDE = POINTER TO CH_CMDE;

VAR

	qcod : QTCompressionCodecPtr;
	qqual : QTCompressionQualityPtr;
	mca : MCActionsPtr;
	valid : BOOLEAN;
	getQTilsC : LibBaseGetter;
	getAnnotationKeys : AnnotationKeysGetter;
	QTilsCSize, QTilsCObtained : CARDINAL;
	ch : CH_CMDE;
	ii : CARDINAL;
	pCh : P_CH_CMDE;

PROCEDURE LoadQTilsDLL() : BOOLEAN;
VAR
	name : Str255;
	msg : ARRAY[0..1024] OF CHAR;
BEGIN
%IF WIN32 %THEN
	(* on charge la dll QTils *)
%IF QTILS_DEBUG %THEN
	QTilsHandle := LoadLibrary("QTils-debug.dll");
	IF (QTilsHandle = NULL_HANDLE)
		THEN
			QTilsHandle := LoadLibrary("QTils-dev.dll");
			Assign( "QTils-dev.dll", name );
		ELSE
			Assign( "QTils-debug.dll", name );
	END;
	DevMode := TRUE;
%ELSIF QTILS_DEV %THEN
	(* mode "developer": on charge la version de QTils.dll qui génère des messages dans une fenêtre journal *)
	QTilsHandle := LoadLibrary("QTils-dev.dll");
	Assign( "QTils-dev.dll", name );
	DevMode := TRUE;
%ELSE
	(* on charge QTils-dev.dll si elle est présente *)
	QTilsHandle := LoadLibrary("QTils-dev.dll");
	IF (QTilsHandle = NULL_HANDLE)
		THEN
			(* mode "release": on charge la version de QTils.dll qui ignore les messages journal *)
			QTilsHandle := LoadLibrary("QTils.dll");
			Assign( "QTils.dll", name );
			DevMode := FALSE;
		ELSE
			Assign( "QTils-dev.dll", name );
			DevMode := TRUE;
	END;
%END
	IF (QTilsHandle <> NULL_HANDLE)
		THEN
			(* on obtient l'adresse de la fonction initDMBaseQTils() qui va nous donner les adresses
				des fonctions d'intérêt dans le record QTilsC :
			*)
			Assign( name, QTilsDLL );
			getQTilsC := CAST(LibBaseGetter, dlsym(QTilsHandle, "initDMBaseQTils_Mod2", valid));
			IF valid
				THEN
					(* la fonction qui donnera les codes des différents types de meta-données QuickTime: *)
					getAnnotationKeys := CAST(AnnotationKeysGetter, dlsym(QTilsHandle, "ExportAnnotationKeys", valid));
				END;
			IF valid
				THEN
					(* taille de la structure initialisée par initDMBaseQTils_Mod2() : la taille de LibQTilsBase moins
					 * les (NUM_MOD_FUNCTIONS) membres qui renvoient vers des fonctions Modula-2
					 *)
					QTilsCSize := TSIZE(LibQTilsBase) - NUM_MOD_FUNCTIONS * TSIZE(ADDRESS);
					QTilsCObtained := getQTilsC(QTilsC);
					(* getQTilsC() (= QTils.dll::initDMBaseQTils) retourne la taille de l'equivalent C du record
						LibQTilsBase. Évidemment elle doit être égale à QTilsCSize, ce qui fournit un
						test rapide d'échec et/ou de la compatibilité de l'initialisation:
					*)
					IF QTilsCSize <> QTilsCObtained
						THEN
							Concat( "Incompatibilité de version de ", QTilsDLL, msg );
							PostMessage("Pb d'initialisation de QTilsM2", msg);
							FreeLibrary(QTilsHandle);
							QTilsHandle := NULL_HANDLE;
							RETURN FALSE;
						ELSE
							RETURN TRUE;
					END;
				ELSE
					PostMessage("Alerte", "Échec d'initialisation de QTilsM2");
					FreeLibrary(QTilsHandle);
					QTilsHandle := NULL_HANDLE;
					RETURN FALSE;
				END;
		ELSE
%IF QTILS_DEV %THEN
			PostMessage("Alerte", "Échec charge QTils-dev.dll");
%ELSE
			PostMessage("Alerte", "Échec charge QTils.dll (et/ou de QTils-dev.dll)");
%END
			RETURN FALSE;
	END;
END LoadQTilsDLL;


BEGIN
	GetCommandLine(ch);
	IF NOT Equal(ch, "")
		THEN
(*
			pCh := ADR(ch[ii + 4]);
			WHILE (pCh^[0] # 0C) AND ((pCh^[0] = " ") OR (ORD(pCh^[0]) = 09H)) DO
				pCh := ADDADR(pCh, 1)
			END;
			ligneDeCmde := (LENGTH(pCh^) > 0)
*)
			ligneDeCmde := TRUE
		ELSE
			ligneDeCmde := FALSE
	END;
	IF LoadQTilsDLL()
		THEN
			(* the MetaData codes: *)
			getAnnotationKeys(MetaData);
			WITH QTilsC DO
				cQTils.QTMovieWindowH_BasicCheck := QTMovieWindowH_BasicCheck;
				cQTils.QTMovieWindowH_Check := QTMovieWindowH_Check;
				cQTils.QTMovieWindowH_isOpen := QTMovieWindowH_isOpen;
				cQTils.OpenQTMovieInWindow := OpenQTMovieInWindow;
				cQTils.CloseQTMovieWindow := CloseQTMovieWindow;
				cQTils.FTTS := FTTS;
				cQTils.LogMsgEx := LogMsgEx;
				cQTils.XMLRootElementID := XMLRootElementID;
				cQTils.XMLContentKind := XMLContentKind;
				cQTils.XMLRootElementContentKind := XMLRootElementContentKind;
				cQTils.XMLContentOfElementOfRootElement := XMLContentOfElementOfRootElement;
				cQTils.XMLContentOfElement := XMLContentOfElement;
				cQTils.XMLElementOfContent := XMLElementOfContent;
				cQTils.CreateXMLParser := CreateXMLParser;
				cQTils.ReadXMLContent := ReadXMLContent;
				cQTils.sscanf := vsscanf;
				cQTils.sprintf := vsprintf;
				cQTils.ssprintf := vssprintf;
				cQTils.ssprintfAppend := vssprintfAppend;
			END;
			(* on exporte la partie publique de QTilsC: *)
			QTils := QTilsC.cQTils;
			(* on obtient les valeurs correspondant aux actions des controlleurs QuickTime;
				ces valeurs sont celles définies par QuickTime
			*)
			mca := QTilsC.cMCActionListProc();
			MCAction := mca^;
			(* idem pour les codecs et niveaux de qualité de compression *)
			qcod := QTilsC.QTCompressionCodec();
			qqual := QTilsC.QTCompressionQuality();
			QTCompressionCodec := qcod^;
			QTCompressionQuality := qqual^;
			QTils.LogMsg("initialisation QTilsM2 OK");
	END;
	IF (QTilsHandle = NULL_HANDLE)
		THEN
			(* because there'll always be dudes who try to use LogMsg(Ex) to notify the user we failed initialising... *)
			QTilsC.cQTils.LogMsg := lLogMsg;
			QTilsC.cQTils.LogMsgEx := LogMsgEx;
			QTils := QTilsC.cQTils;
	END;
%ELSE
	QTilsHandle := NULL_HANDLE;
%END
	(* lQTOpened doit toujours être FALSE tant qu'on n'a pas activé ("ouvert") QuickTime! *)
	lQTOpened := FALSE;

FINALLY

	QTils.LogMsg("décharge de QTilsM2");
%IF WIN32 %THEN
	CloseQT();
	IF QTilsHandle # NULL_HANDLE
		THEN
			FreeLibrary(QTilsHandle);
			QTilsHandle := NULL_HANDLE
	END
%END

END QTilsM2.
