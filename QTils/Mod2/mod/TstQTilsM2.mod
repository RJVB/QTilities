MODULE TstQTilsM2;

<*/VERSION:QTILS_DEBUG*>

%IF StonyBrook %AND %NOT(LINUX) %THEN
	FROM SYSTEM IMPORT
		AttachDebugger, AttachAll;
%END

FROM SYSTEM IMPORT
	CAST, ADR, ADDRESS, TSIZE;

FROM Strings IMPORT
	Assign, Concat, Capitalize;

FROM WholeStr IMPORT
	IntToStr;

FROM RealStr IMPORT
	RealToStr;

FROM WIN32 IMPORT
	Sleep;

FROM ElapsedTime IMPORT
	StartTimeEx, GetTimeEx;

%IF AVEC_EXCEPTIONS %THEN
	FROM EXCEPTIONS IMPORT *;
%END

FROM QTilsM2 IMPORT *;

FROM POSIXm2 IMPORT *;
(*	jmp_buf, POSIX, MarkLog; *)

CONST
	(*movie = "d:\demos\motos\Perfect_Day.mp4"; *)
	(*movie = "Sample.mov"; *)
	(*movie = "c:/Documents and Settings/MSIS/Bureau/Sample.mov";*)
	movie = "d:\video\P3_Parcours3-S-full.mov";
	streamer = "http://rjvbertin.free.fr/Images/Sample.mov";
	(* streamer = "http://trailers.apple.com/movies/dreamworks/kung_fu_panda/kung_fu_panda-tlr1_h.320.mov"; *)
	(* 48Mo: *)
	(* streamer = "http://trailers.apple.com/movies/dreamworks/kung_fu_panda/kung_fu_panda-tlr1_h720p.mov"; *)
	(* 84Mb: *)
	(* streamer = "http://trailers.apple.com/movies/dreamworks/kung_fu_panda/kung_fu_panda-tlr1_h1080p.mov"; *)

VAR
	otype : OSType;
	ostr : OSTypeStr;
	numQTWM : UInt32;
	qtwm : QTMovieWindows;
	qtwmSize : CARDINAL;
	qtwmPtr : QTMovieWindowPtr;
	qtwmH, qtwmH2, qtwmH3 : QTMovieWindowH;
	err : ErrCode;
	n, ljumps, dumInt : Int32;
	theMovie : Movie;
	nTracks : Int32;
	t : Real64;
	ft : MovieFrameTime;
	testStr : ARRAY[0..16] OF CHAR;
	Title1 : ARRAY[0..1024] OF CHAR = "Title1";
	title1Len : UInt32;
	lang : MDLangStr = "";
	dst : ARRAY[0..1024] OF CHAR = "kk.mov";
	wpos, wsize : Cartesian;
	entreeBouclePrincipale : jmp_buf;
	jmpVal : INTEGER;
	lastKeyUp : CHAR;
	quitRequest : BOOLEAN;
	stringPtr : String1kPtr;
%IF AVEC_EXCEPTIONS %THEN
	exc : ExceptionSource;
%END

PROCEDURE movieStep(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
VAR
	t, t2 : Real64;
	steps : Int16;
	tstr : ARRAY[0..63] OF CHAR;
BEGIN
	steps := CAST(Int16, params);
	QTils.QTMovieWindowGetTime(wih, t, 0);
	IF steps = 0
		THEN
			RealToStr( t, tstr );
			PostMessage( "Movie stepped to new time:", tstr );
		ELSE
			t2 := t + CAST(Real64,steps) / wih^^.info^.frameRate;
		END;
	RETURN 0;
END movieStep;

PROCEDURE moviePlay(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
VAR
	t : Real64;
	tstr : ARRAY[0..63] OF CHAR;
BEGIN
	IF wih^^.wasScanned > 0
		THEN
			QTils.QTMovieWindowGetTime(wih, t, 0);
			RealToStr( t, tstr );
			PostMessage( "Movie scanned to new time:", tstr );
		ELSE
			QTils.LogMsg( "Play action received, wasScanned <= 0" );
		END;
	RETURN 0;
END moviePlay;

PROCEDURE movieLoadState(wih : QTMovieWindowH; params : MCActionParams ) : Int32 [CDECL];
VAR
	code : ARRAY[0..32] OF CHAR;
	loadState : Int32;
BEGIN
	loadState := CAST(Int32,params);
	IntToStr( loadState, code );
	PostMessage( "Changement d'état de chargement", code );
	IF (loadState >= kMovieLoadStatePlaythroughOK) & (wih^^.loadState <= kMovieLoadStateComplete)
			& (wih^^.isPlaying=0)
		THEN
			QTils.QTMovieWindowPlay(wih);
		END;
	RETURN 0;
END movieLoadState;

PROCEDURE movieFinished(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
VAR
	t : Real64;
	ft : MovieFrameTime;
BEGIN
	QTils.QTMovieWindowSetTime(wih, wih^^.info^.duration/2.0, 0 );
	err := QTils.QTMovieWindowGetTime(wih, t, 0);
	(* on obtient le temps résultant en tant que heures/minutes/secondes/trames *)
	QTils.secondsToFrameTime(t, wih^^.info^.frameRate, ft );
	RETURN 0;
END movieFinished;

PROCEDURE movieClose(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
VAR
	wiNum : ARRAY[0..16] OF CHAR;
BEGIN
	IntToStr( wih^^.idx, wiNum );
	PostMessage( "Fermeture du movie", wiNum );
	numQTWM := numQTWM - 1;
	RETURN 0;
END movieClose;

PROCEDURE movieKeyUp(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
VAR
	evt : EventRecordPtr;
	size : Cartesian;
BEGIN
	IF( params = NIL )
		THEN
			RETURN 0;
	END;
	evt := CAST(EventRecordPtr, params);
	lastKeyUp := VAL(CHAR,evt^.message);
	CASE lastKeyUp OF
		'q', 'Q' :
				(* Il vaut mieux ne pas fermer des movies/fenêtres dans une MCActionCallback! *)
				quitRequest := TRUE;
				(* ici on retourne TRUE! *)
				RETURN 1;
		| '1' :
				QTils.QTMovieWindowGetGeometry( wih, NULL_Cartesian, ADR(size), 0 );
				size.horizontal := wih^^.info^.naturalBounds.right;
				size.vertical := wih^^.info^.naturalBounds.bottom + wih^^.info^.controllerHeight;
				QTils.QTMovieWindowSetGeometry( wih, NULL_Cartesian, ADR(size), 1.0, 0 );
				QTils.QTMovieWindowGetGeometry( wih, NULL_Cartesian, ADR(size), 0 );
		| 'c', 'C' :
				QTils.LogMsgEx( "Toggling MC visibility returned %d",
					QTils.QTMovieWindowToggleMCController(wih) );
		ELSE
			(* noop *)
	END;
	RETURN 0;
END movieKeyUp;

PROCEDURE register_MCActions( qtwmH : QTMovieWindowH );
CONST
	moitie = "On est à la moitié!";
VAR
	theTrack, theOffset, Chapters, idx : Int32;
	theTime : Real64;
	chapText : ARRAY[0..1024] OF CHAR;
BEGIN
	IF QTils.QTMovieWindowH_Check(qtwmH)
		THEN
			(* void statement: *)
			(* QTils.register_MCAction( qtwmH, 0, moviePlay ); *)
			QTils.register_MCAction( qtwmH, MCAction.Step, movieStep );
			QTils.register_MCAction( qtwmH, MCAction.Play, moviePlay );
			QTils.register_MCAction( qtwmH, MCAction.LoadState, movieLoadState );
			QTils.register_MCAction( qtwmH, MCAction.Finished, movieFinished );
			QTils.register_MCAction( qtwmH, MCAction.Close, movieClose );
			QTils.register_MCAction( qtwmH, MCAction.KeyUp, movieKeyUp );
			numQTWM := numQTWM + 1;
(*
		quelques tests concernant les chapitres, rien à voir avec des MCActions!
*)
			theTrack := 0; theTime := 0.0; theOffset := 0;
			IF QTils.FindTextInMovie( qtwmH^^.theMovie, "mov", 1, theTrack, theTime, theOffset, chapText ) = noErr
				THEN
					QTils.LogMsgEx( 'Trouvé "mov" dans piste %ld à t=%gs, offset %ld dans le texte\n"%s"',
						theTrack, theTime, theOffset, chapText );
			END;
			theTrack := 0; theTime := 0.0; theOffset := 0;
			IF QTils.FindTextInMovie( qtwmH^^.theMovie, "qi2m", 1, theTrack, theTime, theOffset, chapText ) = noErr
				THEN
					QTils.LogMsgEx( 'Trouvé "qi2m" dans piste %ld à t=%gs, offset %ld dans le texte\n"%s"\n',
						theTrack, theTime, theOffset, chapText );
			END;
			theTime := qtwmH^^.info^.duration / 2.0;
			QTils.LogMsgEx( 'Ajout de chapitre "%s" à t=%gs err=%d\n',
				moitie, theTime,
				QTils.MovieAddChapter( qtwmH^^.theMovie, 0, moitie, theTime, 0.0 )
			);
			Chapters := QTils.GetMovieChapterCount(qtwmH^^.theMovie);
			FOR idx := 0 TO Chapters-1 DO
				IF QTils.GetMovieIndChapter( qtwmH^^.theMovie, idx, theTime, chapText ) = noErr
					THEN
						QTils.LogMsgEx( 'Chapitre #%ld "%s" à t=%gs\n',
							idx, chapText, theTime );
						IF QTils.FindTimeStampInMovieAtTime( qtwmH^^.theMovie, theTime, chapText ) = noErr
							THEN
									QTils.LogMsgEx( 'timeStamp à t=%gs : "%s"\n', theTime, chapText );
						END;
				END;
			END;
			IF CAST(BOOLEAN,QTils.HasMovieChanged(qtwmH^^.theMovie))
					AND PostYesNoDialog( qtwmH^^.theURL^,
							"ce movie a changé, voulez-vous le sauvegarder?" )
				THEN
					err := QTils.SaveMovie( qtwmH^^.theMovie );
					QTils.LogMsgEx( 'sauvegarde de "%s" : err=%d\n', qtwmH^^.theURL^, err );
					IF err <> noErr
						THEN
							PostMessage( "TstQTilsM2", QTils.lastSSLogMsg^ );
					END;
			END;
		END;
END register_MCActions;

PROCEDURE isPlayable(wih : QTMovieWindowH) : BOOLEAN;
BEGIN
	RETURN QTils.QTMovieWindowH_isOpen(wih) & (wih^^.loadState >= kMovieLoadStatePlaythroughOK);
END isPlayable;

PROCEDURE testXML() : ErrCode;
CONST
	element_root = 1;
	element_el1 = 2;
	element_el2 = 3;
	attr_atint = 1;
	attr_atbool = 2;
	attr_atstring = 3;
	attr_atReal64 = 4;
	attr_atshort = 5;
VAR
	xmlParser : ComponentInstance;
	xmldoc : XMLDoc;
	xmlErr : ErrCode;
	errDescr : Str255;
	element_id, idx : Int32;
	root_content, element_content : XMLContentArrayPtr;
	element_attrs : XMLAttributePtr;
	data : RECORD
			atint : Int32;
			atbool : UInt8;
			el1 : RECORD
					atstring : ARRAY[0..63] OF CHAR;
					atReal64 : Real64;
				END;
			el2 : RECORD
					atshort : Int16;
					atint : Int32;
				END;
		END;
BEGIN
	xmlParser := NIL;
	xmldoc := NIL;
	xmlErr := QTils.GetAttributeIndex( NIL, attr_atint, idx );
	QTils.LogMsgEx( "idx=%d err=%d\n", idx, xmlErr );
	xmlErr := QTils.XMLParserAddElement( xmlParser, "root", element_root, 0 );
	IF xmlErr = noErr THEN
		xmlErr := QTils.XMLParserAddElementAttribute( xmlParser, element_root, "atint", attr_atint, attributeValueKindInteger );
	END;
	IF xmlErr = noErr THEN
		xmlErr := QTils.XMLParserAddElementAttribute( xmlParser, element_root, "atbool", attr_atbool, attributeValueKindBoolean );
	END;
	IF xmlErr = noErr THEN
		xmlErr := QTils.XMLParserAddElement( xmlParser, "el1", element_el1, 0 );
	END;
	IF xmlErr = noErr THEN
		xmlErr := QTils.XMLParserAddElementAttribute( xmlParser, element_el1, "atstring", attr_atstring, attributeValueKindCharString );
	END;
	IF xmlErr = noErr THEN
		xmlErr := QTils.XMLParserAddElementAttribute( xmlParser, element_el1, "atdouble", attr_atReal64, attributeValueKindDouble );
	END;
	IF xmlErr = noErr THEN
		xmlErr := QTils.XMLParserAddElement( xmlParser, "el2", element_el2, 0 );
	END;
	IF xmlErr = noErr THEN
		xmlErr := QTils.XMLParserAddElementAttribute( xmlParser, element_el2, "atshort", attr_atshort, attributeValueKindInteger );
	END;
	IF xmlErr = noErr THEN
		xmlErr := QTils.XMLParserAddElementAttribute( xmlParser, element_el2, "atint", attr_atint, attributeValueKindInteger );
	END;
	IF xmlErr = noErr THEN
		xmlErr := QTils.ParseXMLFile( "test.xml", xmlParser,
				   xmlParseFlagAllowUppercase BOR xmlParseFlagAllowUnquotedAttributeValues
				   BOR elementFlagPreserveWhiteSpace,
				   xmldoc
		);
		xmlErr := QTils.Check4XMLError( xmlParser, xmlErr, "test.xml", errDescr );
	END;
	IF xmlErr = noErr THEN
		IF ( xmldoc^.rootElement.identifier = element_root )
			THEN
				xmlErr := QTils.GetAttributeIndex( xmldoc^.rootElement.attributes, attr_atint, idx );
				QTils.LogMsgEx( "idx=%d err=%d\n", idx, xmlErr );
				xmlErr := QTils.GetAttributeIndex( xmldoc^.rootElement.attributes, attr_atbool, idx );
				QTils.LogMsgEx( "idx=%d err=%d\n", idx, xmlErr );
				xmlErr := QTils.GetAttributeIndex( xmldoc^.rootElement.attributes, attr_atReal64, idx );
				QTils.LogMsgEx( "idx=%d err=%d\n", idx, xmlErr );
				xmlErr := QTils.GetIntegerAttribute( xmldoc^.rootElement, attr_atint, data.atint );
				xmlErr := QTils.GetBooleanAttribute( xmldoc^.rootElement, attr_atbool, data.atbool );
				idx := 0;
				root_content := CAST(XMLContentArrayPtr, xmldoc^.rootElement.contents);
				WHILE ( root_content^[idx].kind <> xmlContentTypeInvalid ) DO
					IF ( root_content^[idx].kind = xmlContentTypeElement )
						THEN
							element_content := CAST(XMLContentArrayPtr, xmldoc^.rootElement.contents);
							element_id := element_content^[idx].actualContent.element.identifier;
							element_attrs := element_content^[idx].actualContent.element.attributes;
							CASE element_id OF
								element_el2:
									xmlErr := QTils.GetIntegerAttribute( element_content^[idx].actualContent.element, attr_atint, data.el2.atint );
									xmlErr := QTils.GetShortAttribute( element_content^[idx].actualContent.element, attr_atshort, data.el2.atshort );
								| element_el1:
									xmlErr := QTils.GetDoubleAttribute( element_content^[idx].actualContent.element, attr_atReal64, data.el1.atReal64 );
									xmlErr := QTils.GetStringAttribute( element_content^[idx].actualContent.element, attr_atstring, data.el1.atstring );
								ELSE
									(* noop *)
							END;
					END;
					INC(idx);
				END;
		END;
		xmlErr := QTils.DisposeXMLParser( xmlParser, xmldoc, 1 );
	END;
	RETURN xmlErr;
END testXML;

PROCEDURE testQTMovieSinks() : ErrCode;
CONST
	fBuffers = 64;
	useConstImage = FALSE;
VAR
	qmsErr : ErrCode;
	qms : QTMovieSinks;
	colCntr : UInt8;
	pix : QTAPixel;

		PROCEDURE generateImage;
		VAR
			len, p : UInt32;
			f : UInt16;
			px : QTAPixelPtr;
			fb : QTFrameBuffer;
		BEGIN
			len := VAL(UInt32, qms.Width) * VAL(UInt32, qms.Height);
			f := 0;
			WITH QTils DO
				fb := QTMSCurrentFrameBuffer(qms);
				(* initialisation du pixel avec lequel on peindra l'image pour ce pas: *)
				IF qms.useICM = 0
					THEN
						CASE colCntr OF
							0 :
									pix.ciChannel.red := 255; pix.ciChannel.green := 0; pix.ciChannel.blue := 0;
									INC(colCntr);
							| 1 :
									pix.ciChannel.red := 0; pix.ciChannel.green := 255; pix.ciChannel.blue := 0;
									INC(colCntr);
							| 2 :
									pix.ciChannel.red := 0; pix.ciChannel.green := 0; pix.ciChannel.blue := 255;
									colCntr := 0;
							ELSE
									pix.value := 0ABBABABEh;
						END;
						pix.icmChannel.alpha := 255;
					ELSE
						CASE colCntr OF
							0 :
									pix.icmChannel.red := 255; pix.icmChannel.green := 0; pix.icmChannel.blue := 0;
									INC(colCntr);
							| 1 :
									pix.icmChannel.red := 0; pix.icmChannel.green := 255; pix.icmChannel.blue := 0;
									INC(colCntr);
							| 2 :
									pix.icmChannel.red := 0; pix.icmChannel.green := 0; pix.icmChannel.blue := 255;
									colCntr := 0;
							ELSE
								(*	une couleur uniforme donnerait l'encodage le plus rapide...
										pix.value := 0ABBABABEh;
								*)
									pix.value := 0ABBABABEh;
						END;
						pix.icmChannel.alpha := 255;
				END;
				p := 0;
				WHILE p < len DO
					(*
						si QTFrameBuffer est défini comme POINTER TO ARRAY OF QTAPixel (donc array 'ouvert'), on obtient
						un range error pour l'élément 2817? Pourquoi 2817??!
					*)
					fb^[p] := pix;
					(*
					px := QTMSPixelAddressInCurrentFrameBuffer(qms, p);
					px^ := pix;
					*)
					INC(p);
				END;
(*
				IF qms.useICM = 0
					THEN
						WHILE f < 1 qms.frameBuffers DO
							p := 0;
							WHILE p < len DO
								px := QTMSPixelAddress(qms, f, p);
								px^.value := 0ABBABABEh;
								INC(p);
							END;
							INC(f);
						END;
					ELSE
						WHILE f < qms.frameBuffers-2 DO
							p := 0;
							WHILE p < len DO
								px := QTMSPixelAddress(qms, f, p);
								px^.icmChannel.red := 255; px^.icmChannel.green := 0; px^.icmChannel.blue := 0;
								px^.icmChannel.alpha := 255;
								px := QTMSPixelAddress(qms, f+1, p);
								px^.icmChannel.red := 0; px^.icmChannel.green := 255; px^.icmChannel.blue := 0;
								px^.icmChannel.alpha := 255;
								px := QTMSPixelAddress(qms, f+2, p);
								px^.icmChannel.red := 0; px^.icmChannel.green := 0; px^.icmChannel.blue := 255;
								px^.icmChannel.alpha := 255;
								INC(p);
							END;
							f := f + 3;
						END;
				END;
*)
			END;
		END generateImage;

		PROCEDURE benchmark( width, height : UInt16; useICM, useRT, constantImage : BOOLEAN );
		VAR
			ext : ARRAY[0..31] OF CHAR;
			i : UInt32;
			timer, totTimer : CARDINAL32;
			t, pt, ptt, duration, totDuration : Real64;
			fName : ARRAY[0..255] OF CHAR;
			qmsCloseErr : ErrCode;
			stats : QTMSEncodingStats;
		BEGIN
			WITH QTils DO
				IF useRT
					THEN
						Assign( "-rt.mov", ext );
					ELSE
						Assign( ".mov", ext );
				END;
				IF useICM
					THEN
						Concat( "tstJPEGicm", ext, fName );
					ELSE
						Concat( "tstJPEG", ext, fName );
				END;
				qmsErr := OpenQTMovieSink( qms, fName, width, height, 1, fBuffers,
						QTCompressionCodec.JPEG, QTCompressionQuality.High, ORD(useICM), 0 );
				IF qmsErr = noErr
					THEN
						qms.AddFrame_RT := ORD(useRT);
						IF constantImage
							THEN
								colCntr := 3;
							ELSE
								colCntr := 0;
						END;
						i := 0;
						pt := 0.0;
						duration := 0.0;
						totTimer := StartTimeEx();
						timer := StartTimeEx();
						LOOP
							generateImage;
							t := VAL(Real64, GetTimeEx(timer))/1000.0;
							IF qms.useICM = 1
								THEN
									qmsErr := AddFrameToQTMovieSinkWithTime( qms, t );
								ELSE
									qmsErr := AddFrameToQTMovieSink( qms, t - ptt );
							END;
							ptt := t;
							pt := VAL(Real64, GetTimeEx(timer))/1000.0;
							duration := duration + (pt - t);
							IF qmsErr <> noErr
								THEN
									EXIT;
								ELSIF duration >= 3.0
									THEN
										INC(i);
										EXIT;
								ELSE
									INC(i);
							END;
						END;
						(*
							ici on pourrait ajouter quelques méta-données:
							AddMetaDataStringToMovie( GetQTMovieSinkMovie(qms), MetaData.akComment, "Movie généré avec QTMovieSinks pour Modula-2", "fr_FR" );
						*)
						qmsCloseErr := CloseQTMovieSink( qms, 1, ADR(stats), 0 );
						totDuration := VAL(Real64, GetTimeEx(totTimer))/1000.0;
						LogMsgEx( 'Enregistré %gs(%gs) et %hu(%ld) trames dans "%s" en %gs => %gfps(%gfps); err=%d/%d\n',
											duration, stats.Duration, i, stats.Total,
											qms.theURL, totDuration, VAL(Real64,i)/duration, stats.frameRate, qmsErr, qmsCloseErr );
						IF qmsErr = noErr
							THEN
								qmsErr := qmsCloseErr;
						END;
					ELSE
						LogMsgEx( "Erreur d'ouverture/création de '%s': %d\n", fName, qmsErr );
				END;
			END;
		END benchmark;

BEGIN
	benchmark( 1024, 768, FALSE, FALSE, useConstImage );
	benchmark( 1024, 768, FALSE, TRUE, useConstImage );
	benchmark( 1024, 768, TRUE, FALSE, useConstImage );
	benchmark( 1024, 768, TRUE, TRUE, useConstImage );
	RETURN qmsErr;
END testQTMovieSinks;

(* ==================================== BEGIN ==================================== *)
BEGIN
%IF StonyBrook %AND %NOT(LINUX) %THEN
	AttachDebugger(AttachAll);
%END
%IF AVEC_EXCEPTIONS %THEN
	AllocateSource(exc);
%END

	IF NOT ( QTilsAvailable() AND POSIXAvailable() )
		THEN
			HALT;
	END;
	QTils.QTils_Allocator^.malloc := POSIX.malloc;
	QTils.QTils_Allocator^.calloc := POSIX.calloc;
	QTils.QTils_Allocator^.realloc := POSIX.realloc;
	QTils.QTils_Allocator^.free := POSIX.free;

	FOR n := 0 TO INT(POSIX.argc) - 1 DO
		Capitalize( POSIX.argv^[n]^ );
		MarkLog; POSIX.LogMsgEx( "C-style argv[%d] = '%s' (%s)\n", n, POSIX.argv^[n]^, POSIX.Arg(n) );
	END;

	stringPtr := NIL;
	n := QTils.ssprintfAppend( stringPtr, "Ceci %s un test", "est" );
	PostMessage( "1e retour de ssprintfAppend", stringPtr^);
	n := n + QTils.ssprintfAppend( stringPtr, " Et ceci %s un test\n", "n'est pas" );
	IF stringPtr <> NIL
		THEN
			PosixLogMsgEx( "ssprintf returned %d, string='%s'\n", n, stringPtr );
			POSIX.free(stringPtr);
	END;

	OpenQT();

	(* pour info... *)
	qtwmSize := TSIZE(QTMovieWindows);
	qtwmSize := SIZE(QTMovieWindows);
	qtwmSize := SIZE(qtwm);

	(* la mauvaise maniere d'obtenir un QTMovieWindowH ... *)
	qtwmPtr := ADR(qtwm);
	qtwmH := ADR(qtwmPtr);
	qtwmH2 := NULL_QTMovieWindowH;
	qtwmH3 := NULL_QTMovieWindowH;
	numQTWM := 0;
	ljumps := 0;

	(* on obtient un 'jmp_buf', tampon de contexte pour setjmp/longjmp *)
	entreeBouclePrincipale := POSIX.new_jmp_buf();

	(* QTOpened est TRUE si QTilsM2 s'est initialisé correctement, sinon, QuickTime n'est pas disponible: *)
	IF QTOpened()
		THEN
			otype := FOUR_CHAR_CODE('TVOD');
			ostr := OSTStr(otype);
			QTils.LogMsgEx( "'TVOD' -> 0x%lx -> '%s'", otype, ostr );
			QTils.LogMsgEx( "TRUE=%s FALSE=%s\n", BoolStr(TRUE), BoolStr(FALSE) );

			t := 3.1415;
			nTracks := -1;
			n := QTils.sprintf( testStr, "Real64=%g\nhex=0x%lx", t, nTracks );
			QTils.LogMsgEx( "sprintf=%d, testStr='%s'", n, testStr );
			n := QTils.sscanf( testStr, "Real64=%lf\nhex=0x%lx", ADR(t), ADR(nTracks) );
			QTils.LogMsgEx( "after sscanf=%d: Real64=%g hex=0x%lx", n, t, nTracks );

			testXML();
			testQTMovieSinks();

			(* QTMovieWindowH_isOpen() doit retourner FALSE pour ce QTMovieWindowH: *)
			IF QTils.QTMovieWindowH_isOpen( qtwmH )
				THEN
%IF AVEC_EXCEPTIONS %THEN
					RAISE( exc, 0, "QTMovieWindowH_isOpen() accepted an invalid QTMovieWindowH" );
%ELSE
					PostMessage("Fatal", "QTMovieWindowH_isOpen() accepted an invalid QTMovieWindowH" );
%END
			END;

			(* La plupart des fonctions sont membre du record QTils (ce sont les fonctions qui qppellent QTils.dll
				directement)
			*)
			WITH QTils DO
				(* on ouvre <movie> sans la rendre visible dans une fenêtre, dans le but d'en faire
					une copie dans un "reference movie":
				*)
				theMovie := CAST(Movie, NIL);
				err := OpenMovieFromURL( theMovie, 1, movie );
				IF err = noErr
					THEN
						nTracks := GetMovieTrackCount(theMovie);
						err := AddMetaDataStringToTrack(theMovie, 1, MetaData.akDisplayName, "akDisplayName depuis Modula2", "fr_FR");
						err := GetMetaDataStringLengthFromTrack(theMovie, 1, MetaData.akDisplayName, title1Len);
						err := GetMetaDataStringFromTrack(theMovie, 1, MetaData.akDisplayName, Title1, lang);
						IF err = noErr
							THEN
								LogMsg( Title1 );
							END;
						IF err = noErr
							THEN
								err := AddMetaDataStringToMovie(theMovie, MetaData.akComment, Title1, lang);
							END;
						err := SaveMovieAsRefMov( dst, theMovie );
						IF err = noErr
							THEN
								LogMsg( "Reference movie saved OK" );
							END;
						err := SaveMovieAs( dst, theMovie, 0 );
						err := CloseMovie( theMovie );
				END;
				(* si movie = "" on doit recevoir le nom du fichier choisi par l'utilisateur via la
				 * fenetre dialogue postee par OpenMovieFromURL(). *)
				dst := "";
				theMovie := CAST(Movie, NIL);
				err := OpenMovieFromURL( theMovie, 1, dst );
				IF err = noErr
					THEN
						PostMessage( "Movie ouvert:", dst );
						err := CloseMovie( theMovie );
				END;

				(* on marque ce endroit comme cible pour un longjmp.
				 * ATTN: on doit déréférencier entreeBouclePrincipale !
				 *)
				jmpVal := POSIX.setjmp(entreeBouclePrincipale^);
				IF ( jmpVal <> 0 )
					THEN
						MarkLog; POSIX.LogMsgEx( "setjmp a retourné %d après un longjmp", jmpVal );
						(* on est arrivé ici depuis le lonjmp ci-dessous. On RAZ n ...!
						n := 0; *)
						INC(ljumps);
						numQTWM := 0;
				END;

				(* On ouvre <movie> dans une fenêtre: *)
				qtwmH := OpenQTMovieInWindow( movie, 1 );
				(* On ouvre la copie dans une autre fenêtre *)
				qtwmH2 := OpenQTMovieInWindow( dst, 1 );
				(* et on ouvre une vidéo qui est sur un serveur (streaming): *)
				qtwmH3 := OpenQTMovieInWindow( streamer, 1 );

				(* il nous faut au moins 1 fenêtre pour continuer *)
				IF NOT ( QTMovieWindowH_Check(qtwmH) OR QTMovieWindowH_Check(qtwmH2)
						OR QTMovieWindowH_Check(qtwmH3) )
					THEN
						err := LastQTError();
						(* nettoyage ... *)
						DisposeQTMovieWindow(qtwmH);
						DisposeQTMovieWindow(qtwmH2);
						DisposeQTMovieWindow(qtwmH3);
%IF AVEC_EXCEPTIONS %THEN
						RAISE( exc, 0 (*LastQTError()*), "échec dans OpenQTMovieInWindow()" );
%ELSE
						PostMessage("Fatal", "échec dans OpenQTMovieInWindow()" );
%END
					END;

				(* enregistrement des fonctions pour gérer les notifications d'actions envoyées
				   par les contrôleurs *)
				register_MCActions( qtwmH );
				register_MCActions( qtwmH2 );
				register_MCActions( qtwmH3);

				(* test de la fonctionnalité GetTime/SetTime *)
				err := QTMovieWindowGetTime(qtwmH2, t, 0);
				IF err = noErr
					THEN
						LogMsg( "Obtained movie starttime OK" );
						(* on envoie qtwmH2 à la moitié de sa durée: *)
						QTMovieWindowSetTime(qtwmH2, qtwmH2^^.info^.duration/2.0, 0 );
						err := QTMovieWindowGetTime(qtwmH2, t, 0);
						(* on obtient le temps résultant en tant que heures/minutes/secondes/trames *)
						secondsToFrameTime(t, qtwmH2^^.info^.frameRate, ft );
					END;

				(* test de la fonctionnalité de mise à l'échelle des fenêtres: *)
				IF QTMovieWindowH_isOpen(qtwmH3)
					THEN
						QTMovieWindowSetGeometry(qtwmH3, NULL_Cartesian, NULL_Cartesian, 0.5, 0);
						Sleep(500);
						QTMovieWindowSetGeometry(qtwmH3, NULL_Cartesian, NULL_Cartesian, 0.5, 0);
						Sleep(500);
						(* échelle 1/8e: *)
						QTMovieWindowSetGeometry(qtwmH3, NULL_Cartesian, NULL_Cartesian, 0.5, 0);
						Sleep(500);
						IF QTMovieWindowH_isOpen(qtwmH2)
							THEN
								(* position et taille de la fenêtre de qtwmH2 (l'envelope) : *)
								QTMovieWindowGetGeometry(qtwmH2, ADR(wpos), ADR(wsize), 1);
								(* le position de son coin bas/droite: *)
								wpos.horizontal := wpos.horizontal + wsize.horizontal;
								wpos.vertical := wpos.vertical + wsize.vertical;
								(* on met qtwmH3 à l'échelle 1/4e (opération sur le contenu),
								 * puis sa fenêtre à la position qu'on vient de calculer (opération sur
								 * l'envelope): cela nécessite donc 2 appels! *)
								QTMovieWindowSetGeometry(qtwmH3, NULL_Cartesian, NULL_Cartesian, 2.0, 0);
								QTMovieWindowSetGeometry(qtwmH3, ADR(wpos), NULL_Cartesian, 1.0, 1);
							ELSE
								QTMovieWindowSetGeometry(qtwmH3, NULL_Cartesian, NULL_Cartesian, 2.0, 0);
							END;
					END;

				(* la lecture est démarrée dans les fenêtres ouvertes et uniquement quand il y a assez de quoi lire *)
				IF isPlayable(qtwmH)
					THEN
						QTMovieWindowPlay(qtwmH);
				END;
				IF isPlayable(qtwmH2)
					THEN
						(*SetMoviePlayHints( qtwmH2^^.theMovie, hintsPlayingEveryFrame, 1 );*)
						QTMovieWindowSetPlayAllFrames( qtwmH2, 1, dumInt );
						QTMovieWindowSetPreferredRate( qtwmH2, 1000, dumInt );
						QTMovieWindowPlay(qtwmH2);
				END;
				IF isPlayable(qtwmH3)
					THEN
						QTMovieWindowPlay(qtwmH3);
				END;

				(* on traite les évenements tant qu'au moins 1 des fenêtres reste ouverte: *)
				WHILE ( (numQTWM > 0) AND (NOT quitRequest) )
					DO
						n := PumpMessages(1)
				END;

				DisposeQTMovieWindow(qtwmH);
				DisposeQTMovieWindow(qtwmH2);
				DisposeQTMovieWindow(qtwmH3);

				LogMsg( "We're done!" );
				IF ( (n <> 0) AND (NOT quitRequest) AND (ljumps < 3) )
					THEN
						POSIX.longjmp( entreeBouclePrincipale, ljumps+1 );
					ELSE
						(* soit il n'y a jamais eu de vidéo(s) ouverte(s), soit on a fait un longjmp
						 * après avoir fermé toutes les vidéos.
						 *)
				END;
			END;
	END;

FINALLY

	IF POSIXAvailable()
		THEN
			POSIX.dispose_jmp_buf(entreeBouclePrincipale);
	END;

END TstQTilsM2.
