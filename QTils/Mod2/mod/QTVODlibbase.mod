IMPLEMENTATION MODULE QTVODlibbase;

(* USECHANNELVIEWIMPORTFILES : correspond à l'ancien comportement où pour créer une vidéo cache
	pour les 4+1 canaux on créait d'abord un fichier .qi2m d'importation *)
<*/VALIDVERSION:USECHANNELVIEWIMPORTFILES,COMMTIMING,NOTOUTCOMMENTED,USE_TIMEDCALLBACK,CCV_IN_BACKGROUND*>

FROM SYSTEM IMPORT
	CAST, ADR;

FROM Strings IMPORT
	Concat, Append, Delete, FindPrev, Assign, Equal, Length, FindNext;

FROM ChanConsts IMPORT *;
FROM StreamFile IMPORT
	Open, Close, ChanId;
FROM TextIO IMPORT
	WriteString, WriteChar, WriteLn;
FROM FileFunc IMPORT
	DeleteFile;

FROM COMMDLG IMPORT
	OPENFILENAME, GetOpenFileName, OFN_FILEMUSTEXIST, OFN_NOCHANGEDIR, OFN_EXPLORER;

FROM QTilsM2 IMPORT *;
FROM POSIXm2 IMPORT POSIX;
FROM QTVODcomm IMPORT *;

TYPE

	VODDESIGN_PARSER = ARRAY[0..37] OF XML_RECORD;

VAR

	xmlVD : VODDescription;
	xmlParser : ComponentInstance;
	xmldoc : XMLDoc;
	inOpenVideo : BOOLEAN;


CONST

	tabChar = CHR(9);
	element_vodDesign = 1;
	element_frequency = 2;
	element_frequence = 3;
	element_scale = 4;
	element_utc = 5;
	element_echelle = 6;
	element_channels = 7;
	element_canaux = 8;
	element_parsing = 9;
	element_lecture = 10;
	element_transcoding = 11;
	element_transcodage = 12;
	attr_freq = 1;
	attr_scale = 2;
	attr_forward = 4;
	attr_pilot = 6;
	attr_left = 8;
	attr_right = 10;
	attr_zone = 11;
	attr_dst = 12;
	attr_flLR = 13;
	attr_usevmgi = 14;
	attr_log = 15;
	attr_codec = 16;
	attr_bitrate = 17;
	attr_split = 18;

	_d = FALSE;
	_fn = TRUE;

	xml_design_parser = VODDESIGN_PARSER{
			{xml_element, "vod.design", element_vodDesign},
			{xml_element, "frequency", element_frequency},
				{xml_attribute, "fps", attr_freq, recordAttributeValueTypeDouble, _d, ADR(xmlVD.frequency) },
			{xml_element, "frequence", element_frequence},
				{xml_attribute, "tps", attr_freq, recordAttributeValueTypeDouble, _d, ADR(xmlVD.frequency)},
			{xml_element, "utc", element_utc},
				{xml_attribute, "zone", attr_zone, recordAttributeValueTypeDouble, _d, ADR(xmlVD.timeZone)},
				{xml_attribute, "dst", attr_dst, recordAttributeValueTypeBoolean, _d, ADR(xmlVD.DST)},
			{xml_element, "scale", element_scale},
				{xml_attribute, "factor", attr_scale, recordAttributeValueTypeDouble, _d, ADR(xmlVD.scale)},
			{xml_element, "echelle", element_echelle},
				{xml_attribute, "facteur", attr_scale, recordAttributeValueTypeDouble, _d, ADR(xmlVD.scale)},
			{xml_element, "channels", element_channels},
				{xml_attribute, "forward", attr_forward, recordAttributeValueTypeInteger, _d, ADR(xmlVD.channels.forward)},
				{xml_attribute, "pilot", attr_pilot, recordAttributeValueTypeInteger, _d, ADR(xmlVD.channels.pilot)},
				{xml_attribute, "left", attr_left, recordAttributeValueTypeInteger, _d, ADR(xmlVD.channels.left)},
				{xml_attribute, "right", attr_right, recordAttributeValueTypeInteger, _d, ADR(xmlVD.channels.right)},
				{xml_attribute, "flipleftright", attr_flLR, recordAttributeValueTypeBoolean, _d, ADR(xmlVD.flipLeftRight)},
			{xml_element, "canaux", element_canaux},
				{xml_attribute, "avant", attr_forward, recordAttributeValueTypeInteger, _d, ADR(xmlVD.channels.forward)},
				{xml_attribute, "pilote", attr_pilot, recordAttributeValueTypeInteger, _d, ADR(xmlVD.channels.pilot)},
				{xml_attribute, "gauche", attr_left, recordAttributeValueTypeInteger, _d, ADR(xmlVD.channels.left)},
				{xml_attribute, "droite", attr_right, recordAttributeValueTypeInteger, _d, ADR(xmlVD.channels.right)},
				{xml_attribute, "flipgauchedroite", attr_flLR, recordAttributeValueTypeBoolean, _d, ADR(xmlVD.flipLeftRight)},
			{xml_element, "parsing", element_parsing},
				{xml_attribute, "usevmgi", attr_usevmgi, recordAttributeValueTypeBoolean, _d, ADR(xmlVD.useVMGI)},
				{xml_attribute, "log", attr_log, recordAttributeValueTypeBoolean, _d, ADR(xmlVD.log)},
			{xml_element, "lecture", element_lecture},
				{xml_attribute, "avecvmgi", attr_usevmgi, attributeValueKindBoolean, _d, ADR(xmlVD.useVMGI)},
				{xml_attribute, "journal", attr_log, recordAttributeValueTypeBoolean, _d, ADR(xmlVD.log)},
			{xml_element, "transcoding.mp4", element_transcoding},
				(* on doit utiliser un XMLAttributeParseCallback pour lire des chaines de caractères *)
				{xml_attribute, "codec", attr_codec, recordAttributeValueTypeCharString, _fn, GetXMLParamString },
				{xml_attribute, "bitrate", attr_bitrate, recordAttributeValueTypeCharString, _fn, GetXMLParamString},
				{xml_attribute, "split", attr_split, recordAttributeValueTypeBoolean, _d, ADR(xmlVD.splitQuad)},
			{xml_element, "transcodage.mp4", element_transcodage},
				{xml_attribute, "codec", attr_codec, recordAttributeValueTypeCharString, _fn, GetXMLParamString},
				{xml_attribute, "taux", attr_bitrate, recordAttributeValueTypeCharString, _fn, GetXMLParamString},
				{xml_attribute, "split", attr_split, recordAttributeValueTypeBoolean, _d, ADR(xmlVD.splitQuad)}
	};


PROCEDURE dirname( VAR path : URLString ) : BOOLEAN;
VAR
	sepFound : BOOLEAN;
	sepPos : CARDINAL;
	len : CARDINAL;
BEGIN
	IF ( LENGTH(path) < 1 )
		THEN
			RETURN FALSE;
	END;
	len := LENGTH(path) - 1;
	FindPrev( "\", path, len, sepFound, sepPos );
	IF sepFound
		THEN
			Delete( path, sepPos, len-sepPos+1 );
			RETURN TRUE;
		ELSE
     RETURN FALSE;
	END;
END dirname;

PROCEDURE PruneExtensions( VAR URL : ARRAY OF CHAR );
BEGIN
	IF ( URL[LENGTH(URL)-1] = '"' )
		THEN
			Delete( URL, LENGTH(URL)-1, 1 );
	END;
	IF ( URL[0] = '"' )
		THEN
			Delete( URL, 0, 1 );
	END;
	PruneExtension( URL, ".mov" );
	PruneExtension( URL, ".vod" );
	PruneExtension( URL, ".qi2m" );
	PruneExtension( URL, ".xml" );
	PruneExtension( URL, ".ief" );
	PruneExtension( URL, "-design" );
	PruneExtension( URL, "-forward" );
	PruneExtension( URL, "-pilot" );
	PruneExtension( URL, "-left" );
	PruneExtension( URL, "-right" );
	PruneExtension( URL, "-TC" );
END PruneExtensions;

PROCEDURE GetFileName( title : ARRAY OF CHAR; VAR fileName : ARRAY OF CHAR ) : BOOLEAN;
VAR
	lp : OPENFILENAME;
BEGIN
	POSIX.memset( lp, 0, SIZE(lp) );
	lp.lStructSize := SIZE(OPENFILENAME);
	lp.lpstrFilter := ADR("Fichiers QuickTime" + CHR(0) + "*.mov" + CHR(0)
			+ "Media supportés" + CHR(0) + "*.mov;*.qi2m;*.VOD;*.jpgs;*.mpg;*.mp4;*.mpeg;*.avi;*.wmv;*.mp3;*.aif;*.wav;*.mid;*.jpg;*.jpeg" + CHR(0)
			+ "Tous les fichiers" + CHR(0) + "*.*" + CHR(0) + CHR(0));
	lp.nFilterIndex := 1;
	fileName[0] := CHR(0);
	lp.lpstrFile := ADR(fileName);
	lp.nMaxFile := SIZE(fileName);
	lp.lpstrTitle := ADR(title);
	(* un jour je l'aurai ... la customisation de la fenêtre OpenFile! *)
	lp.lpTemplateName := ADR("VODdesign");
	lp.Flags := OFN_FILEMUSTEXIST BOR OFN_NOCHANGEDIR BOR OFN_EXPLORER;
	IF GetOpenFileName(lp)
		THEN
			RETURN TRUE;
		ELSE
			RETURN FALSE;
	END;
END GetFileName;

PROCEDURE GetXMLParamString( theElement : XMLElement; idx : CARDINAL; elm : UInt32; designTable : ARRAY OF XML_RECORD;
	designEntry : UInt32; fName : ARRAY OF CHAR ) : ErrCode;
VAR
	attrs : XMLAttributeArrayPtr;
BEGIN
	attrs := CAST(XMLAttributeArrayPtr, theElement.attributes);
	CASE attrs^[idx].identifier OF
		attr_codec :
				Assign( attrs^[idx].valueStr^, xmlVD.codec );
		| attr_bitrate :
				Assign( attrs^[idx].valueStr^, xmlVD.bitRate );
		ELSE
			RETURN paramErr;
	END;
	QTils.LogMsgEx( "> attr #%d %s=%s",
		VAL(INTEGER, attrs^[idx].identifier), designTable[designEntry].attributeTag,
		attrs^[idx].valueStr^ );
	RETURN noErr;
END GetXMLParamString;

%IF DEBUG %THEN
PROCEDURE QTVODCreateXMLParser( VAR xmlParser : ComponentInstance; VAR errors :  Int32 ) : ErrCode;
VAR
	xmlErr : ErrCode;
	i, N : CARDINAL;
	lastElement : UInt32;
BEGIN

	(*
		Le nombre d'éléments dans le tableau d'initialisation:
	*)
	N := SIZE(xml_design_parser) / SIZE(xml_design_parser[0]);

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
END QTVODCreateXMLParser;
%END (* DEBUG *)

%IF DEBUG %THEN
PROCEDURE QTVODReadXMLContent( fName : URLString; theContent : XMLContentArrayPtr;
														VAR descr : VODDescription; VAR elm : CARDINAL );
VAR
	xmlErr : ErrCode;
	bool : UInt8;
	element_content : XMLContentArrayPtr;
	theElement : XMLElement;

	(*
		la procédure qui lit chaque élément individuel; les attributs, ou les (sous) éléments.
	*)
	PROCEDURE ReadXMLElementAttributes( theElement : XMLElement; VAR descr : VODDescription );
	VAR
		idx : CARDINAL;
	BEGIN
		CASE theElement.identifier OF

			element_frequency,
			element_frequence :
					xmlErr := QTils.GetDoubleAttribute( theElement, attr_freq, descr.frequency );
						IF (xmlErr <> attributeNotFound )
							THEN
								QTils.LogMsgEx( "attr #%d freq=%g (%d)", VAL(INTEGER,attr_freq), descr.frequency, xmlErr );
						END;

			| element_utc :
					xmlErr := QTils.GetDoubleAttribute( theElement, attr_zone, descr.timeZone );
						IF (xmlErr <> attributeNotFound )
							THEN
								QTils.LogMsgEx( "attr #%d timeZone=%g (%d)", VAL(INTEGER,attr_zone), descr.timeZone, xmlErr );
						END;
					xmlErr := QTils.GetBooleanAttribute( theElement, attr_dst, bool );
						IF (xmlErr <> attributeNotFound )
							THEN
								descr.DST := (bool <> 0);
								QTils.LogMsgEx( "attr #%d DST=%hd (%d)", VAL(INTEGER,attr_dst), VAL(Int16,bool), xmlErr );
						END;
					xmlErr := QTils.GetBooleanAttribute( theElement, attr_flLR, bool );
						IF (xmlErr <> attributeNotFound )
							THEN
								descr.flipLeftRight := (bool <> 0);
								QTils.LogMsgEx( "attr #%d flipGaucheDroite=%hd (%d)", VAL(INTEGER,attr_flLR), VAL(Int16,bool), xmlErr );
						END;

			| element_parsing,
			element_lecture :
					xmlErr := QTils.GetBooleanAttribute( theElement, attr_usevmgi, bool );
						IF (xmlErr <> attributeNotFound )
							THEN
								descr.useVMGI := (bool <> 0);
								QTils.LogMsgEx( "attr #%d useVMGI=%hd (%d)", VAL(INTEGER,attr_usevmgi), VAL(Int16,bool), xmlErr );
						END;
					xmlErr := QTils.GetBooleanAttribute( theElement, attr_log, bool );
						IF (xmlErr <> attributeNotFound )
							THEN
								descr.log := (bool <> 0);
								QTils.LogMsgEx( "attr #%d log=%hd (%d)", VAL(INTEGER,attr_log), VAL(Int16,bool), xmlErr );
						END;

			| element_scale,
			element_echelle :
					xmlErr := QTils.GetDoubleAttribute( theElement, attr_scale, descr.scale );
						IF (xmlErr <> attributeNotFound )
							THEN
								QTils.LogMsgEx( "attr #%d scale=%g (%d)", VAL(INTEGER,attr_scale), descr.scale, xmlErr );
						END;

			| element_channels,
			element_canaux :
					xmlErr := QTils.GetIntegerAttribute( theElement, attr_forward, descr.channels.forward );
						IF (xmlErr <> attributeNotFound )
							THEN
								ClipInt( descr.channels.forward, 1, 4 );
								QTils.LogMsgEx( "attr #%d chForward=%d (%d)", VAL(INTEGER,attr_forward), descr.channels.forward, xmlErr );
						END;
					xmlErr := QTils.GetIntegerAttribute( theElement, attr_pilot, descr.channels.pilot );
						IF (xmlErr <> attributeNotFound )
							THEN
								ClipInt( descr.channels.pilot, 1, 4 );
								QTils.LogMsgEx( "attr #%d chPilot=%d (%d)", VAL(INTEGER,attr_pilot), descr.channels.pilot, xmlErr );
						END;
					xmlErr := QTils.GetIntegerAttribute( theElement, attr_left, descr.channels.left );
						IF (xmlErr <> attributeNotFound )
							THEN
								ClipInt( descr.channels.left, 1, 4 );
								QTils.LogMsgEx( "attr #%d chLeft=%d (%d)", VAL(INTEGER,attr_left), descr.channels.left, xmlErr );
						END;
					xmlErr := QTils.GetIntegerAttribute( theElement, attr_right, descr.channels.right );
						IF (xmlErr <> attributeNotFound )
							THEN
								ClipInt( descr.channels.right, 1, 4 );
								QTils.LogMsgEx( "attr #%d chRight=%d (%d)", VAL(INTEGER,attr_right), descr.channels.right, xmlErr );
						END;
			| element_transcoding,
			element_transcodage :
					xmlErr := QTils.GetStringAttribute( theElement, attr_codec, descr.codec );
						IF (xmlErr <> attributeNotFound )
							THEN
								QTils.LogMsgEx( "attr #%d codec=%s (%d)", VAL(INTEGER,attr_codec), descr.codec, xmlErr );
						END;
					xmlErr := QTils.GetStringAttribute( theElement, attr_bitrate, descr.bitRate );
						IF (xmlErr <> attributeNotFound )
							THEN
								QTils.LogMsgEx( "attr #%d bitRate=%s (%d)", VAL(INTEGER,attr_bitrate), descr.bitRate, xmlErr );
						END;
					xmlErr := QTils.GetBooleanAttribute( theElement, attr_split, bool );
						IF (xmlErr <> attributeNotFound )
							THEN
								descr.splitQuad := (bool <> 0);
								QTils.LogMsgEx( "attr #%d split=%hd (%d)", VAL(INTEGER,attr_log), VAL(Int16,bool), xmlErr );
						END;

			| element_vodDesign :
					(*
						ceci est le 'root element', la racine, qui n'a que des éléments, pas d'attributs. Si le fichier
						contient l'élément racine ailleurs qu'à la racine, le membre 'contents' de theELement sera un pointer
						vers un array de XMLContent, juste comme xmldoc^.rootElement.contents .
						On peut donc appeler QTVODReadXMLContent de façon récursive.
						Bien sur il n'y a aucune utilité à pouvoir écrire des fichiers avec une liste de paramètres de configuration
						de façon récursive...
					*)
					IF QTils.XMLContentOfElement( theElement, element_content )
						THEN
							idx := 0;
							QTVODReadXMLContent( fName, element_content, descr, idx );
					END;

			| xmlIdentifierUnrecognized :
					IF ( (theElement.name <> NIL) AND (theElement.attributes^.name <> NIL) )
						THEN
							QTils.LogMsgEx( "élément inconnu <%s %s /> rencontré dans %s",
								theElement.name, theElement.attributes^.name, fName );
						ELSE
							QTils.LogMsgEx( "élément inconnu rencontré dans %s", fName );
					END;
					PostMessage( "QTVODm2", QTils.lastSSLogMsg^ );

			ELSE
				xmlErr := paramErr;
		END;
	END ReadXMLElementAttributes;

BEGIN
	(* tant qu'on trouve du contenu valide... *)
	WHILE ( QTils.XMLContentKind( theContent, elm ) <> xmlContentTypeInvalid ) DO
		(* et pour chaque élément trouvé (on ne gère pas le cas xmlContentTypeCharData) *)
		IF QTils.XMLElementOfContent( theContent, elm, theElement )
			THEN
				(* on lit les attributs qu'on connait de chaque élément *)
				QTils.LogMsgEx( "Scanne attributs et/ou éléments de l'élément #%d (entrée %d)", theElement.identifier, elm );
				ReadXMLElementAttributes( theElement, descr );
		END;
		INC(elm);
	END;
END QTVODReadXMLContent;
%END (*DEBUG*)

(* test:
PROCEDURE FrequencyFromXMLElementAttributes( theElement : XMLElement; elm : UInt32;
								design : ARRAY OF XML_RECORD; idx : UInt32; fName : ARRAY OF CHAR ) : ErrCode;
VAR
	xmlErr : ErrCode;
BEGIN
	IF design[idx].attributeID = attr_freq
		THEN
			CASE theElement.identifier OF

				element_frequency,
				element_frequence :
						xmlErr := QTils.GetDoubleAttribute( theElement, attr_freq, xmlVD.frequency );
							IF (xmlErr <> attributeNotFound )
								THEN
									QTils.LogMsgEx( "< attr #%d freq=%g (%d)", VAL(INTEGER,attr_freq), xmlVD.frequency, xmlErr );
							END;
				ELSE
					QTils.LogMsgEx( "FrequencyFromXMLElementAttributes appellé pour un mauvais élément, %d", theElement.identifier );
					xmlErr := paramErr;
			END;
		ELSE
			QTils.LogMsgEx( "FrequencyFromXMLElementAttributes appellé pour un mauvais attribut '%s' (%d) de l'élément %d",
						design[idx].attributeTag, design[idx].attributeID, theElement.identifier );
			xmlErr := paramErr;
	END;
	RETURN xmlErr;
END FrequencyFromXMLElementAttributes;
*)

(*
PROCEDURE GetBitRateString( theElement : XMLElement; idx : CARDINAL; elm : UInt32; designTable : ARRAY OF XML_RECORD;
	designEntry : UInt32; fName : ARRAY OF CHAR ) : ErrCode;
BEGIN
	IF theElement.attributes^.identifier = attr_bitrate
		THEN
			Assign( theElement.attributes^.valueStr^, xmlVD.bitRate );
			QTils.LogMsgEx( "> attr #%d %s=%s", VAL(INTEGER, attr_bitrate), designTable[designEntry].attributeTag,
				xmlVD.bitRate );
			RETURN noErr;
		ELSE
			RETURN paramErr;
	END;
END GetBitRateString;
*)

PROCEDURE ReadXMLDoc( fName : URLString; xmldoc : XMLDoc; VAR descr : VODDescription );
VAR
	theContent : XMLContentArrayPtr;
	elm : CARDINAL;
BEGIN
	elm := 0;
	(*
		on commence à lire à l'indice passée dans <elm>, à partir de 0,
		tant qu'on trouve du contenu valide dans la racine:
	*)
	WHILE ( QTils.XMLRootElementContentKind(xmldoc, elm) <> xmlContentTypeInvalid ) DO
		(* on obtient un pointeur vers le contenu *)
		IF QTils.XMLContentOfElementOfRootElement( xmldoc, elm, theContent )
			THEN
				(*
					on lit le contenu. ReadXMLContent augmentera elm;
					la plupart du temps ce sera elle qui fait toute la lecture
				*)
				xmlVD := descr;
				QTils.ReadXMLContent( fName, theContent, xml_design_parser, elm );
				descr := xmlVD;
		END;
	END;
END ReadXMLDoc;

PROCEDURE ReadDefaultVODDescription( fName : URLString; VAR descr : VODDescription ) : ErrCode;
VAR
	xmlErr : ErrCode;
	errDescr : URLString;
	errors : Int32;
BEGIN

	IF xmlParser = NIL
		THEN
			xmlErr := QTils.CreateXMLParser( xmlParser, xml_design_parser, errors );
		ELSE
			xmlErr := noErr;
			errors := 0;
	END;

	IF errors = 0
		THEN
			(* vidons lastSSLogMsg - ParseXMLFile y laissera un message en cas d'erreur *)
			QTils.lastSSLogMsg^[0] := CHR(0);
			(*
			 * On lit <fName> et on tente d'interpreter son contenu selon les spécifications stockées
			 * dans xmlParser.
			 *)
			xmlErr := QTils.ParseXMLFile( fName, xmlParser,
						 xmlParseFlagAllowUppercase BOR xmlParseFlagAllowUnquotedAttributeValues
							BOR elementFlagPreserveWhiteSpace,
						 xmldoc
			);
	END;
	IF xmlErr <> noErr THEN
		Assign( QTils.lastSSLogMsg^, errDescr );
		IF ( xmlErr = -2000 )
			THEN
				Append( " (fichier inexistant)", errDescr );
			ELSIF ( QTils.lastSSLogMsg^[0] <> CHR(0) )
				THEN
					PostMessage( "QTVODm2", errDescr );
		END;
	END;

	IF xmlErr = noErr
		THEN
			IF QTils.XMLRootElementID(xmldoc) = element_vodDesign
				THEN
					QTils.LogMsgEx( "Lectures des paramètres depuis '%s'", fName );
					ReadXMLDoc( fName, xmldoc, descr );
				ELSE
					QTils.LogMsgEx( "'%s' est du XML valide mais n'a pas l'élément racine '%s'\n",
						fName, xml_design_parser[0].elementTag );
			END;
	END;
	QTils.DisposeXMLParser( xmlParser, xmldoc, 0 );
	RETURN xmlErr;
END ReadDefaultVODDescription;

(*
 * cherche un fichier (XML) <fName> dans le repertoire de movie ainsi que touts les répertoires au dessus,
 * et essaie de les lire jusqu'à en trouver un qui se lit ou qui contient une erreur.
 * NB: un fichier vide et lu sans erreurs et sans effet, et peut donc servir de marquer d'arrêt de l'algorithme.
 * NB: QTVODm2 aura déjà appellé ReadDefaultVODDescription() une fois avant, dans son répertoire de départ.
 * Cet appel aura donc inclut le fichier XML gouvernant les movies spécifiés sans chemin.
 *)
PROCEDURE ScanForDefaultVODDescription( movie, fName : URLString; VAR descr : VODDescription ) : ErrCode;
VAR
	path : URLString;
	descFName : URLString;
	err : ErrCode;
BEGIN
	err := noErr;
	Assign( movie, path );
	WHILE dirname(path) DO
		Concat( path, "\", descFName );
		Append( fName, descFName );
		err := ReadDefaultVODDescription( descFName, descr );
		IF ( (err = -2000) )
			THEN
				(* 'couldNotResolveDataRef' : on continue avec le répertoire parent *)
			ELSE
				QTils.LogMsgEx( "Scan arrêté sur %s", descFName );
				(* fichier trouvé ou autre erreur: on arrête *)
				path := "";
		END;
	END;
	RETURN err;
END ScanForDefaultVODDescription;

PROCEDURE ImportMovie( src : ARRAY OF CHAR; description : VODDescription ) : ErrCode;
VAR
	source, fName : ARRAY[0..1024] OF CHAR;
	tmpStr : ARRAY[0..255] OF CHAR;
	useVMGI, doLog : ARRAY[0..7] OF CHAR;
	fp : ChanId;
	res : OpenResults;
	err, err2 : ErrCode;
	theMovie : Movie;
	errString, errComment : ARRAY[0..127] OF CHAR;
BEGIN
	Concat( src, ".qi2m", fName );
	Open( fp, fName, write+old, res );
	IF ( res = opened )
		THEN
			Concat( src, ".VOD", source );
			IF description.useVMGI
				THEN
					Assign( "True", useVMGI );
				ELSE
					Assign( "False", useVMGI );
			END;
			IF description.log
				THEN
					Assign( "True", doLog );
				ELSE
					Assign( "False", doLog );
			END;

			WriteString( fp, '<?xml version="1.0"?>' ); WriteLn(fp);
			WriteString( fp, '<?quicktime type="video/x-qt-img2mov"?>' ); WriteLn(fp);
			(* on active l'autoSave sur l'importation pour une création automatique du fichier cache *)
			WriteString( fp, '<import autoSave=True >' ); WriteLn(fp);
			IF LENGTH(assocDataFileName) = 0
				THEN
					QTils.sprintf( tmpStr, '\t<description txt="UTC timeZone=%g, DST=%hd" />',
						description.timeZone, VAL(Int16,description.DST) );
				ELSE
					QTils.sprintf( tmpStr, '\t<description txt="UTC timeZone=%g, DST=%hd, assoc.data:%s" />',
						description.timeZone, VAL(Int16,description.DST), assocDataFileName );
			END;
			WriteString( fp, tmpStr ); WriteLn(fp);
			WriteChar( fp, tabChar );
			(* début de la définition d'une séquence: *)
			WriteString( fp, '<sequence src="' );
			WriteString( fp, source );
			QTils.sprintf( tmpStr, '" channel=-1 freq=%g', description.frequency );
			WriteString( fp, tmpStr );
			(* on cache la piste TimeCode des vues des 4 caméras (hidetc=True)
			 * et on force l'interprétation comme 'movie' *)
			WriteString( fp, ' hidetc=False timepad=False hflip=False vmgi=' );
			WriteString( fp, useVMGI );
			WriteString( fp, ' newchapter=True log=' );
			WriteString( fp, doLog );
			IF Length(description.codec) > 0
				THEN
					WriteLn(fp); WriteChar( fp, tabChar );
					WriteString( fp, " fcodec=" );
					WriteString( fp, description.codec );
					IF Length(description.bitRate) > 0
						THEN
							WriteString( fp, " fbitrate=" );
							WriteString( fp, description.bitRate );
					END;
			END;
			WriteString( fp, " fsplit=" );
			IF description.splitQuad
				THEN
					WriteString( fp, "True" );
				ELSE
					WriteString( fp, "False" );
			END;
			WriteString( fp, ' />' ); WriteLn(fp);
			WriteString( fp, '</import>' ); WriteLn(fp);
			Close(fp);
			(* fichier .qi2m créé, on l'ouvre maintenant, pas encore dans une fenêtre *)
			err := QTils.OpenMovieFromURL( theMovie, 1, fName );
			IF ( err <> noErr )
				THEN
					IF QTils.MacErrorString( err, errString, errComment ) <> 0
						THEN
							QTils.LogMsgEx( "Echec d'importation: %s (%s)", errString, errComment );
						ELSE
							QTils.LogMsgEx( "Echec d'importation %d/%d", QTils.LastQTError(), err );
					END;
					PostMessage( fName, QTils.lastSSLogMsg^ );
					Open( fp, source, read, res );
					IF res <> opened
						THEN
							(* fichier .VOD n'existe pas; pas la peine de préserver le fichier qi2m! *)
							QTils.LogMsgEx( "'%s' n'existe pas, donc suppression de '%s'", source, fName );
							DeleteFile(fName);
							err := -2000;
					END;
				ELSE
					QTils.CloseMovie( theMovie );
					DeleteFile(fName);
					QTils.LogMsgEx( "%s importé et supprimé", fName );
					Concat( src, ".mov", fName );
					QTils.OpenMovieFromURL( fullMovie, 1, fName );
					IF (fullMovie <> NIL) AND (LENGTH(assocDataFileName) <> 0)
						THEN
							err2 := QTils.AddMetaDataStringToMovie( fullMovie, MetaData.akSource, assocDataFileName, "" );
							IF err2 = noErr
								THEN
									QTils.SaveMovie(fullMovie);
							END;
					END;
			END;
		ELSE
			PostMessage( fName, "Echec de création/ouverture" );
			err := 1;
	END;
	RETURN err;
END ImportMovie;

PROCEDURE OpenVideo( VAR URL : URLString; VAR description : VODDescription; isVODFile : BPTR ) : ErrCode;
VAR
	fName : URLString;
	errString, errComment, quadString : ARRAY[0..127] OF CHAR;
	lang : ARRAY[0..15] OF CHAR;
	err : ErrCode;

BEGIN

	err := noErr;

	IF ( LENGTH(URL) = 0 )
		THEN
			fName := "";
(* 20121121: on n'appelle plus OpenMovieFromURL pour obtenir un nom de fichier car cela risque
	de prendre beaucoup de temps si c'est un fichier VOD.
			IF ( QTils.OpenMovieFromURL( fullMovie, 1, fName ) = noErr )
				THEN
					QTils.CloseMovie(fullMovie);
			END;
*)
			(* on essaie d'obtenir un nom de fichier de l'utilisateur *)
			GetFileName( "Merci de choisir un fichier vidéo", fName );
			(* ce qui nous intéresse surtout est de savoir si on a reçu un nom de fichier *)
			IF ( LENGTH(fName) > 0 )
				THEN
					IF isVODFile <> NIL
						THEN
							isVODFile^ := HasExtension( fName, ".VOD" );
					END;
					PruneExtensions(fName);
					URL := fName;
					Assign( fName, baseFileName );
				ELSE
					RETURN 1
			END;
		ELSE
			IF isVODFile <> NIL
				THEN
					isVODFile^ := HasExtension( URL, ".VOD" );
			END;
			(* on enlève les extensions "qui fachent" pour pouvoir mettre une extension spécifique *)
			PruneExtensions(URL);
			Assign( URL, baseFileName );
	END;

	err := ScanForDefaultVODDescription( URL, "VODdesign.xml", description );
	Concat( baseFileName, "-design.xml", fName );
	err := ReadDefaultVODDescription( fName, description );

	err := noErr;

	Concat( baseFileName, ".mov", fName );
	baseDescription := description;
	IF Equal( assocDataFileName, "*FromVODFile*" )
		THEN
			Concat( baseFileName, ".IEF", assocDataFileName );
	END;

	QTils.LogMsgEx( "essai vidéo cache: OpenMovieFromURL(%s)", fName );
	(* OpenMovieFromURL() ouvre une vidéo sans l'afficher dans une fenêtre ... ici on l'utilise
	 * pour vérifier si <fName> existe et est bien une vidéo
	 *)
	recreateChannelViews := FALSE;
	IF ( QTils.OpenMovieFromURL( fullMovie, 1, fName ) = noErr )
		THEN
			(* le "cache" existe déjà; on le garde ouvert! *)
			QTils.LogMsg( "all-channel cache movie opened" );
		ELSE
			(* il faudra construire le cache à partir du fichier .VOD d'origine
			 * Pour cela, il suffit d'importer le fichier VOD avec les paramètres par défaut
			 * sauf si on a une fréquence spécifiée! *)
			Concat( URL, ".VOD", fName );
			WITH description DO
				IF ( (channels.forward <= 0) AND (channels.pilot <= 0)
						AND (channels.left <= 0) AND (channels.right <=0) )
					THEN
						QTils.LogMsgEx( "essai vidéo source: OpenMovieFromURL(%s)", fName );
						err := QTils.OpenMovieFromURL( fullMovie, 1, fName );
						IF ( err = noErr )
							THEN
								(* on a réussi à importer le fichier .VOD avec les paramètres par défaut *)
								Concat( URL, ".mov", fName );
								(* on enregistre le cache, une copie dite "séquence de référence" dans le
								 * jargon QuickTime.  *)
								err := QTils.SaveMovieAsRefMov( fName, fullMovie );
								QTils.CloseMovie( fullMovie );
								QTils.LogMsgEx( "vidéo cache créée: %s", fName );
								IF ( err <> noErr )
									THEN
										QTils.LogMsgEx( "Erreur dans SaveMovieAsRefMov() %d", err );
										PostMessage( fName, QTils.lastSSLogMsg^ );
									ELSE
										err := QTils.OpenMovieFromURL( fullMovie, 1, fName );
								END;
						END;
					ELSE
						err := ImportMovie( URL, description );
						IF ( err <> noErr )
							THEN
								IF QTils.MacErrorString( err, errString, errComment ) <> 0
									THEN
										QTils.LogMsgEx( "Erreur %d dans ImportMovie(): %s (%s)\n"+
											"L'enregistrement n'existe ni en .mov ni en .VOD?!",
											err, errString, errComment );
									ELSE
										QTils.LogMsgEx( "Erreur %d dans ImportMovie()\n"+
											"L'enregistrement n'existe ni en .mov ni en .VOD?!",
											err );
								END;
								PostMessage( fName, QTils.lastSSLogMsg^ );
						END;
				END;
				IF fullMovie = NIL
					THEN
						PostMessage( URL, "Pas d'accès à la vidéo cache principale" );
					ELSE
						(*
							fullMovie est une vidéo cache principale nouvelle ou renouvellée; il conviendrait
							de régénérer les 'ChannelView' également
						*)
						recreateChannelViews := TRUE;
				END;
			END;
	END;
	IF err <> noErr
		THEN
			IF ( (err = -2000) AND (NOT inOpenVideo) )
				THEN
					inOpenVideo := TRUE;
					Assign( "", URL );
					err := OpenVideo( URL, description, isVODFile );
					inOpenVideo := FALSE;
					RETURN err;
				ELSE
					RETURN err;
			END;
	END;

	IF fullMovie <> NIL
		THEN
			IF QTils.GetMetaDataStringFromMovie( fullMovie, FOUR_CHAR_CODE("quad"), quadString, lang ) = noErr
				THEN
					IF Equal(quadString, "MPG4 VOD imported as 4 tracks" )
						THEN
							fullMovieIsSplit := TRUE;
						ELSE
							fullMovieIsSplit := FALSE;
					END;
			END;
			fullMovieWMH := QTils.QTMovieWindowHFromMovie(fullMovie);
			IF fullMovieWMH = NULL_QTMovieWindowH
				THEN
					RETURN 2;
			END;
		ELSE
			RETURN 1;
	END;
	RETURN err;
END OpenVideo;

PROCEDURE GetGPSStartTime( descr : VODDescription; VAR ecartVideoTempsGPS : Real64 ) : Real64;
VAR
	tVid, tGPS : Real64;
	idx, n : Int32;
	h, m : Int16;
	err : ErrCode;
	title : URLString;
	b : BOOLEAN;
	i : CARDINAL;
BEGIN
	tGPS := -1.0;
	IF fullMovieWMH <> NULL_QTMovieWindowH
		THEN
			n := QTils.GetMovieChapterCount(fullMovie);
			FOR idx := 0 TO n-1 DO
				err := QTils.GetMovieIndChapter( fullMovie, idx, tVid, title );
				IF err = noErr
					THEN
					FindNext("startGPS", title, 0, b, i);
					IF b AND (QTils.sscanf(title, "startGPS %hd:%hd:%lf ", ADR(h), ADR(m), ADR(tGPS)) = 3)
						THEN
							IF descr.DST
								THEN
									tGPS := tGPS + 3600.0 * (VAL(Real64,h) + descr.timeZone + 1.0) + 60.0 * VAL(Real64,m);
								ELSE
									tGPS := tGPS + 3600.0 * (VAL(Real64,h) + descr.timeZone) + 60.0 * VAL(Real64,m);
							END;
							ecartVideoTempsGPS := tGPS - fullMovieWMH^^.info^.startTime - tVid;
					END;
				END;
			END;
	END;
	RETURN tGPS;
END GetGPSStartTime;

(* ==================================== BEGIN ==================================== *)
BEGIN

	fullMovie := NIL;
	fullMovieWMH := NULL_QTMovieWindowH;
	baseFileName := "";
	assocDataFileName := "";
	baseDescription.useVMGI := TRUE;
	baseDescription.log := FALSE;
	xmlParser := NIL;
	xmldoc := NIL;
	inOpenVideo := FALSE;
	fullMovieIsSplit := FALSE;

FINALLY

	IF xmlParser <> NIL
		THEN
			QTils.DisposeXMLParser( xmlParser, xmldoc, 1 );
	END;
	IF fullMovie <> NIL
		THEN
			QTils.CloseMovie(fullMovie);
			fullMovie := NIL;
			fullMovieWMH := NULL_QTMovieWindowH;
		ELSIF fullMovieWMH <> NULL_QTMovieWindowH
			THEN
				QTils.CloseQTMovieWindow(fullMovieWMH);
				fullMovieWMH := NULL_QTMovieWindowH;
	END;

END QTVODlibbase.
