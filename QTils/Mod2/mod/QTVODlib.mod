IMPLEMENTATION MODULE QTVODlib;

(* USECHANNELVIEWIMPORTFILES : correspond à l'ancien comportement où pour créer une vidéo cache
	pour les 4+1 canaux on créait d'abord un fichier .qi2m d'importation *)
<*/VALIDVERSION:USECHANNELVIEWIMPORTFILES,COMMTIMING,NOTOUTCOMMENTED*>

FROM SYSTEM IMPORT
	CAST, ADR, ADDRESS;

FROM WholeStr IMPORT
	IntToStr;

FROM Strings IMPORT
	Concat, Append, Delete, FindPrev, Assign, Equal, Length, FindNext;

FROM ExStrings IMPORT
	Utf8ToAnsi;

FROM Storage IMPORT
	ALLOCATE, DEALLOCATE;

FROM LongMath IMPORT
	sqrt;

FROM ChanConsts IMPORT *;
FROM StreamFile IMPORT
	Open, Close, ChanId;
FROM TextIO IMPORT
	WriteString, WriteChar, WriteLn;
FROM FileFunc IMPORT
	DeleteFile, RenameFile, CreateDirTree;

%IF WIN32 %THEN
	FROM WIN32 IMPORT
		HWND;
%ELSE
	FROM Windows IMPORT
		HWND;
%END
FROM WINUSER IMPORT
	SetWindowPos, HWND_BOTTOM, HWND_TOP, SWP_NOACTIVATE, SWP_NOMOVE, SWP_NOSIZE,
	ShowWindow, SW_HIDE, SW_MINIMIZE;
FROM COMMDLG IMPORT
	OPENFILENAME, GetOpenFileName, OFN_FILEMUSTEXIST, OFN_NOCHANGEDIR, OFN_EXPLORER, OFN_ENABLETEMPLATE;

FROM WIN32 IMPORT
	LARGE_INTEGER, QueryPerformanceFrequency, QueryPerformanceCounter;

%IF CHAUSSETTE2 %THEN
FROM Chaussette2 IMPORT
%ELSE
FROM Chaussette IMPORT
%END
	sock_nulle, SOCK;

(* importons tout de QTilsM2 et de QTVODcomm; on en aura besoin *)
FROM QTilsM2 IMPORT *;
FROM QTVODcomm IMPORT *;
FROM POSIXm2 IMPORT POSIX;

TYPE

	VODDESIGN_PARSER = ARRAY[0..37] OF XML_RECORD;

VAR

	xmlVD : VODDescription;
	cbPT : Real64;

CONST

	(* si les vues des cameras ont un controleur sous l'image *)
	vues_avec_controleur = 0;

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


CONST

	tabChar = CHR(9);
	TimeCodeChannel = 6 (*5*);

VAR

	Wpos, Wsize : ARRAY[0..maxQTWM] OF Cartesian;
	ULCorner : Cartesian;
	idx : CARDINAL;
	MetaDataDisplayStrPtr : StringPtr;
	xmlParser : ComponentInstance;
	xmldoc : XMLDoc;
	sqrt2 : Real64;
	HPCcalibrator : Real64;
	TCwindowNr : Int32;
	recreateChannelViews : BOOLEAN;
	splitCamTrack : VODChannels;

PROCEDURE InitHRTime();
VAR
	HPCval : LARGE_INTEGER;
BEGIN
	IF QueryPerformanceFrequency(HPCval)
		THEN
			HPCcalibrator := 1.0 / VAL(Real64, HPCval);
		ELSE
			HPCcalibrator := 0.0;
	END;
END InitHRTime;

PROCEDURE HRTime() : Real64;
VAR
	HPCval : LARGE_INTEGER;
BEGIN
	IF QueryPerformanceCounter(HPCval)
		THEN
			RETURN VAL(Real64,HPCval) * HPCcalibrator;
		ELSE
			RETURN -1.0;
	END;
END HRTime;

PROCEDURE SetTimes( t : Real64; ref : QTMovieWindowH; absolute : Int32 );
VAR
	w : CARDINAL;
BEGIN
	FOR w := 0 TO maxQTWM DO
		IF qtwmH[w] <> ref
			THEN
				QTils.QTMovieWindowSetTime(qtwmH[w], t, absolute);
		END;
	END;
END SetTimes;

PROCEDURE SetTimesAndNotify( t : Real64; ref : QTMovieWindowH; absolute : Int32; ss : SOCK; class : NetMessageClass );
VAR
	msg : NetMessage;
BEGIN
	SetTimes( t, ref, absolute );
	IF ss <> sock_nulle
		THEN
			replyCurrentTime( msg, class, ref, VAL(BOOLEAN,absolute) );
			IF ( ss <> sock_nulle )
				THEN
					SendMessageToNet( ss, msg, SENDTIMEOUT, FALSE, "QTVODm2::SetTimesAndNotify" );
			END;
	END;
END SetTimesAndNotify;

(* ajoute <str> à la fin d'un pointer vers une chaine, desalloue la mémoire utilisée
 * précédemment et retourne le nouveau pointeur. *)
PROCEDURE appendString2StringPtr( VAR ptr : StringPtr; str : ARRAY OF CHAR ) : StringPtr;
VAR
	new : StringPtr;
	ptrLen, strLen : CARDINAL;
BEGIN
	strLen := LENGTH(str);
	IF ( ptr <> NIL )
		THEN
			ptrLen := LENGTH(ptr^);
			ALLOCATE( CAST(ADDRESS,new), ptrLen + strLen+ 1 );
			IF ( new <> NIL )
				THEN
					Concat( ptr^, str, new^ );
			END;
			DEALLOCATE( CAST(ADDRESS,ptr), ptrLen + 1 );
		ELSE
			ALLOCATE( CAST(ADDRESS,new), strLen + 1 );
			IF ( new <> NIL )
				THEN
					Assign( str, new^ );
			END;
	END;
	RETURN new;
END appendString2StringPtr;

(* récupère les méta-données pour la clef <key> et les ajoute à la chaine <allStrPtr>, précédées
 * par la description <keyDescr>. Les méta-données sont obtenus dans <theTrack> si positif, sinon
 * dans <theMovie> directement. *)
PROCEDURE appendMetaData( theMovie : Movie; theTrack : Int32;
						key : AnnotationKey; keyDescr : ARRAY OF CHAR;
						VAR allStrPtr : StringPtr );
VAR
	theMD : StringPtr;
	MDLen : UInt32;
	lang : ARRAY[0..15] OF CHAR;
	newline : ARRAY[0..1] OF CHAR;
	err : ErrCode;
BEGIN
	(* détermination de la longueur de chaine nécessaire: *)
	IF ( theTrack > 0 )
		THEN
			err := QTils.GetMetaDataStringLengthFromTrack(theMovie, theTrack, key, MDLen);
		ELSE
			err := QTils.GetMetaDataStringLengthFromMovie(theMovie, key, MDLen);
	END;
	IF ( (err = noErr) AND (MDLen > 0) )
		THEN
			ALLOCATE( CAST(ADDRESS,theMD), MDLen+1 );
			IF ( theMD <> NIL )
				THEN
					(* obtention des méta-données. *)
					IF ( theTrack > 0 )
						THEN
							err := QTils.GetMetaDataStringFromTrack(theMovie, theTrack, key, theMD^, lang);
						ELSE
							err := QTils.GetMetaDataStringFromMovie(theMovie, key, theMD^, lang);
					END;
					IF err = noErr
						THEN
							(* QuickTime MetaData est en format UTF8 *)
							Utf8ToAnsi( theMD^, theMD^, '*' );
							QTils.LogMsgEx( "appendMetaData(): %s%s", keyDescr, theMD^ );
							allStrPtr := appendString2StringPtr( allStrPtr, keyDescr );
							allStrPtr := appendString2StringPtr( allStrPtr, theMD^ );
							(* rajout d'un retour à la ligne: *)
							newline[0] := CHR(13);
							newline[1] := CHR(0);
							allStrPtr := appendString2StringPtr( allStrPtr, newline );
					END;
					DEALLOCATE( CAST(ADDRESS,theMD), MDLen+1 );
			END;
	END;
END appendMetaData;

(* Collectionne les méta-données enregistrées dans la 1e piste et le Movie lui-même affiché dans
 * <wih>, construit une chaine avec des préfixes descriptifs, et affiche le résultat dans un popup. *)
PROCEDURE showMetaData();
VAR
	header : URLString;
	wih : QTMovieWindowH;
	ft : MovieFrameTime;
	trackNr : Int32;
	trackType, trackSubType, creator : OSType;
	cName : ARRAY[0..256] OF CHAR;
BEGIN
	IF ( (fullMovie <> NIL) AND (MetaDataDisplayStrPtr = NIL) )
		THEN
			wih := QTils.QTMovieWindowHFromMovie(fullMovie);
			IF QTils.QTMovieWindowH_BasicCheck(wih)
				THEN
					QTils.secondsToFrameTime( wih^^.info^.startTime, wih^^.info^.frameRate, ft );
					QTils.sprintf( header, 'Enregistrement %s durée %gs; début à %02d:%02d:%02d;%03d\n',
						baseFileName, wih^^.info^.duration,
						VAL(Int32,ft.hours), VAL(Int32,ft.minutes), VAL(Int32,ft.seconds), VAL(Int32,ft.frames) );
				ELSE
					QTils.sprintf( header, 'Enregistrement %s durée %gs\n',
						baseFileName, QTils.GetMovieDuration(fullMovie) );
			END;
			IF LENGTH(assocDataFileName) > 0
				THEN
					QTils.sprintf( header, "%sfichier de données associées: %s\n\n", header, assocDataFileName );
				ELSE
					QTils.sprintf( header, "%s\n", header );
			END;
			MetaDataDisplayStrPtr := appendString2StringPtr( MetaDataDisplayStrPtr, header );
			appendMetaData( fullMovie, 1, MetaData.akDisplayName, "Nom: ", MetaDataDisplayStrPtr );
			appendMetaData( fullMovie, 1, MetaData.akSource, "Fichier original: ", MetaDataDisplayStrPtr );
			appendMetaData( fullMovie, 1, MetaData.akCreationDate, "Date de création: ", MetaDataDisplayStrPtr );
			appendMetaData( fullMovie, 1, MetaData.akDescr, "Description: ", MetaDataDisplayStrPtr );
			appendMetaData( fullMovie, 1, MetaData.akInfo, "Informations: ", MetaDataDisplayStrPtr );
			appendMetaData( fullMovie, 1, MetaData.akComment, "Commentaires: ", MetaDataDisplayStrPtr );
			appendMetaData( fullMovie, -1, MetaData.akDescr, "Description: ", MetaDataDisplayStrPtr );
			appendMetaData( fullMovie, -1, MetaData.akInfo, "Informations: ", MetaDataDisplayStrPtr );
			appendMetaData( fullMovie, -1, MetaData.akComment, "Commentaires: ", MetaDataDisplayStrPtr );
			appendMetaData( fullMovie, -1, MetaData.akCreationDate, "Date de création du fichier cache/.mov: ", MetaDataDisplayStrPtr );
			(* quelques informations techniques sur la vidéo: *)
			trackNr := 1;
			WHILE( QTils.GetMovieTrackTypes( fullMovie, trackNr, trackType, trackSubType ) = noErr ) DO
				IF (trackType = FOUR_CHAR_CODE("vide"))
						AND (QTils.GetMovieTrackDecompressorInfo( fullMovie, trackNr, trackSubType, cName, creator ) = noErr)
					THEN
						QTils.sprintf( header, "Piste #%ld, type '%s' décodeur '%s' par '%s'\n",
							trackNr, OSTStr(trackSubType), cName, OSTStr(creator) );
						MetaDataDisplayStrPtr := appendString2StringPtr( MetaDataDisplayStrPtr, header );
				END;
				INC(trackNr,1);
			END;
			IF ( MetaDataDisplayStrPtr <> NIL )
				THEN
					PostMessage( "Meta-données", MetaDataDisplayStrPtr^ );
					DEALLOCATE( CAST(ADDRESS,MetaDataDisplayStrPtr), LENGTH(MetaDataDisplayStrPtr^)+1 );
					MetaDataDisplayStrPtr := NIL;
			END;
	END;
END showMetaData;

PROCEDURE movieStep(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
VAR
	t : Real64;
	steps : Int16;
BEGIN
	steps := CAST(Int16, params);
	QTils.QTMovieWindowGetTime(wih, t, 0);
	IF steps = 0
		THEN
			SetTimesAndNotify( t, wih, 0, sServeur, qtvod_Notification );
	END;
	RETURN 0;
END movieStep;

(* la procedure qui est appelée quand on "scrub" le contrôleur sous la vidéo. Dans un premier temps
 * on n'envoie pas de notification vers le serveur (si on est connecté) car il peut y avoir beaucoup
 * d'appels à la suite. *)
PROCEDURE movieScan(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
TYPE
	Real64Ptr = POINTER TO Real64;
VAR
	t : Real64;
	tNew : Real64Ptr;
BEGIN
	tNew := CAST(Real64Ptr, params);
	QTils.QTMovieWindowGetTime(wih, t, 0);
	IF ( t <> tNew^ )
		THEN
			SetTimes( tNew^, wih, 0 );
	END;
	RETURN 0;
END movieScan;

VAR
	handlingPlayAction : BOOLEAN;

PROCEDURE moviePlay(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
VAR
	t : Real64;
	w : CARDINAL;
BEGIN
	IF handlingPlayAction
		THEN
			RETURN 0;
	END;
	IF wih^^.wasScanned > 0
		THEN
			handlingPlayAction := TRUE;
			QTils.QTMovieWindowGetTime(wih, t, 0);
			SetTimesAndNotify( t, wih, 0, sServeur, qtvod_Notification );
			FOR w := 0 TO maxQTWM DO
				QTils.QTMovieWindowStop(qtwmH[w]);
			END;
			handlingPlayAction := FALSE;
	END;
	RETURN 0;
END moviePlay;

PROCEDURE StartVideo(excl : QTMovieWindowH);
VAR
	w : CARDINAL;
BEGIN
	FOR w := 0 TO maxQTWM DO
		IF qtwmH[w] <> excl
			THEN
				QTils.QTMovieWindowPlay(qtwmH[w]);
		END;
	END;
END StartVideo;

PROCEDURE StepVideo(excl : QTMovieWindowH ; steps : Int32);
VAR
	w : CARDINAL;
BEGIN
	FOR w := 0 TO maxQTWM DO
		IF qtwmH[w] <> excl
			THEN
				QTils.QTMovieWindowStepNext(qtwmH[w], steps);
		END;
	END;
END StepVideo;

PROCEDURE movieStart(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
VAR
	msg : NetMessage;
BEGIN
	IF handlingPlayAction
		THEN
			RETURN 0;
	END;
	handlingPlayAction := TRUE;
	StartVideo(wih);
(*
	SendNetCommandOrNotification( sServeur, qtvod_Start, qtvod_Notification );
 *)
	(* on construit un message avec le temps actuel *)
	replyCurrentTime( msg, qtvod_Notification, wih, FALSE );
	(* on le converti en message qtvod_Start *)
	msg.flags.type := qtvod_Start;
	IF ( sServeur <> sock_nulle )
		THEN
			SendMessageToNet( sServeur, msg, SENDTIMEOUT, FALSE, "QTVODm2::movieStart" );
	END;
	handlingPlayAction := FALSE;
	RETURN 0;
END movieStart;

PROCEDURE StopVideo(excl : QTMovieWindowH);
VAR
	w : CARDINAL;
BEGIN
	FOR w := 0 TO maxQTWM DO
		IF qtwmH[w] <> excl
			THEN
				QTils.QTMovieWindowStop(qtwmH[w]);
		END;
	END;
END StopVideo;

PROCEDURE movieStop(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
VAR
	msg : NetMessage;
BEGIN
	IF handlingPlayAction
		THEN
			RETURN 0;
	END;
	handlingPlayAction := TRUE;
	StopVideo(wih);
(*
	SendNetCommandOrNotification( sServeur, qtvod_Stop, qtvod_Notification );
 *)
	replyCurrentTime( msg, qtvod_Notification, wih, FALSE );
	msg.flags.type := qtvod_Stop;
	IF ( sServeur <> sock_nulle )
		THEN
			SendMessageToNet( sServeur, msg, SENDTIMEOUT, FALSE, "QTVODm2::movieStop" );
	END;
	handlingPlayAction := FALSE;
	RETURN 0;
END movieStop;

PROCEDURE movieClose(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
BEGIN
	QTils.LogMsgEx( 'Fermeture du movie "%s"#%d dans fenêtre %d', wih^^.theURL^, wih^^.idx, numQTWM );
	numQTWM := numQTWM - 1;
	IF (numQTWM <= 0)
		THEN
			SendNetCommandOrNotification( sServeur, qtvod_Close, qtvod_Notification );
	END;
	RETURN 0;
END movieClose;

PROCEDURE replyMovieFinished( VAR msg : NetMessage; fName : String1kPtr; idx : Int32 );
BEGIN
	msg.flags.type := qtvod_MovieFinished;
	msg.flags.class := qtvod_Notification;
	msg.data.URN := fName^;
	msg.data.iVal1 := idx;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END replyMovieFinished;

PROCEDURE movieFinished(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
VAR
	msg : NetMessage;
BEGIN
	QTils.LogMsgEx( 'Fin du movie "%s"#%d dans fenêtre %d', wih^^.theURL^, wih^^.idx, numQTWM );
	IF ( sServeur <> sock_nulle )
		THEN
			replyMovieFinished( msg, wih^^.theURL, wih^^.idx );
			SendMessageToNet( sServeur, msg, SENDTIMEOUT, FALSE, "QTVODm2::movieFinished" );
	END;
	RETURN 0;
END movieFinished;

(*
PROCEDURE PumpSubscriptions(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
*)
PROCEDURE PumpSubscriptions( cbRegister : QTCallBack; params : Int32 ) [CDECL];
VAR
	subscrTimer, dt : Real64;
	msg : NetMessage;
	wih : QTMovieWindowH;
BEGIN
	wih := CAST(QTMovieWindowH, params);
	subscrTimer := HRTime();
	(* on vérifie s'il faut envoyer le temps courant et s'il est temps de le faire *)
	WITH currentTimeSubscription DO
		dt := subscrTimer - lastSentTime;
		IF (sendInterval > 0.0) (* AND (dt >= sendInterval) *)
			THEN
				(* on obtient le temps courant dans la fenêtre *)
				replyCurrentTime( msg, qtvod_Subscription, wih, absolute );
				(* si ce temps est différent au temps précédemment envoyé, on envoie le message *)
				IF msg.data.val1 <> lastMovieTime
					THEN
						QTils.LogMsgEx( "SUBS dt=%g-%g=%g movie#%d.dt=%g-%g=%g",
							subscrTimer, lastSentTime,
							dt, wih^^.idx,
							msg.data.val1, lastMovieTime,
							msg.data.val1 - lastMovieTime );
						lastMovieTime := msg.data.val1;
						SendMessageToNet( sServeur, msg, SENDTIMEOUT, FALSE, "QTVODm2 - currentTime subscription" );
						(* on veut envoyer le premier temps différent du temps précédent, donc ne modifie
							msgTime QUE quand il y a eu envoi. Cela entraine une petite surcharge (vérification
							du temps courant) quand la vidéo n'est pas en mode lecture, mais dans ce cas cela n'a
							pas d'importance. *)
						lastSentTime := subscrTimer;
				END;
		END;
	END;
	QTils.TimedCallBackRegisterFunctionInTime( wih^^.theMovie, cbRegister,
		currentTimeSubscription.sendInterval, PumpSubscriptions, params, qtCallBacksAllowedAtInterrupt );
END PumpSubscriptions;

PROCEDURE timeCallBack( cbRegister : QTCallBack; params : Int32 ) [CDECL];
VAR
	t : Real64;
	wih : QTMovieWindowH;
BEGIN
	wih := CAST(QTMovieWindowH, params);
	IF QTils.QTMovieWindowH_Check(wih)
		THEN
(*
			IF wih^^.isPlaying <> 0
				THEN
*)
					QTils.QTMovieWindowGetTime(wih, t, 0);
					QTils.LogMsgEx( "timeCallBack @t=%gs currentTime=%g dt=%g\n", HRTime(), t, t-cbPT );
					cbPT := t;
(*
			END;
*)
		QTils.TimedCallBackRegisterFunctionInTime( wih^^.theMovie, cbRegister, 1.5, timeCallBack,
									 params, qtCallBacksAllowedAtInterrupt);
	END;
END timeCallBack;

PROCEDURE SetWindowLayer( pos : HWND; ref : QTMovieWindowH );
VAR
	w : CARDINAL;
BEGIN
	FOR w := 0 TO maxQTWM DO
		IF qtwmH[w] <> ref
			THEN
				SetWindowPos( qtwmH[w]^^.theView, pos, 0,0,0,0, SWP_NOACTIVATE BOR SWP_NOMOVE BOR SWP_NOSIZE );
			ELSE
				SetWindowPos( qtwmH[w]^^.theView, pos, 0,0,0,0, SWP_NOMOVE BOR SWP_NOSIZE );
		END;
	END;
END SetWindowLayer;

PROCEDURE BenchmarkPlaybackRate;
BEGIN
	WITH theTimeInterVal DO
		IF (NOT benchMarking) AND (NOT wasBenchMarking)
			THEN
				StopVideo(NULL_QTMovieWindowH);
				benchMarking := TRUE;
				CalcTimeInterval( TRUE, FALSE );
				wallTimeLapse := HRTime();
			ELSIF (benchMarking) OR (wasBenchMarking)
				THEN
					wallTimeLapse := HRTime() - wallTimeLapse;
					benchMarking := TRUE;
					CalcTimeInterval( FALSE, TRUE );
					benchMarking := FALSE;
					wasBenchMarking := FALSE;
		END;
	END;
END BenchmarkPlaybackRate;

PROCEDURE CalcTimeInterval(reset, display : BOOLEAN);
VAR
	t : Real64;
	msgStr : String1kPtr;
	err : ErrCode;
	startSample, endSample : Int32;
	w : CARDINAL;
BEGIN
	IF QTils.QTMovieWindowH_isOpen(qtwmH[tcWin]) AND (QTils.QTMovieWindowGetTime(qtwmH[tcWin], t, 0) = noErr)
		THEN
			w := tcWin;
		ELSIF QTils.QTMovieWindowH_isOpen(qtwmH[fwWin]) AND (QTils.QTMovieWindowGetTime(qtwmH[fwWin], t, 0) = noErr)
			THEN
				w := fwWin;
		ELSE
			PostMessage( "Erreur", "la fenêtre 'TC' ou la fenêtre 'forward' doit être ouverte pour cette fonction!" );
			RETURN;
	END;
	WITH theTimeInterVal DO
		IF (timeA < 0.0) OR reset
			THEN
				timeA := t;
				Assign("",timeStampA);
				QTils.FindTimeStampInMovieAtTime( qtwmH[w]^^.theMovie, t, timeStampA );
				timeB := -1.0;
			ELSIF timeB < 0.0
				THEN
					timeB := t;
					Assign("",timeStampB);
					QTils.FindTimeStampInMovieAtTime( qtwmH[w]^^.theMovie, t, timeStampB );
					dt := timeB - timeA;
					IF display
						THEN
							msgStr := NIL;
							IF benchMarking
								THEN
									IF fullMovie <> NIL
										THEN
											err := QTils.SampleNumberAtMovieTime( fullMovie, 0, timeA, startSample );
											IF err <> noErr
												THEN
													QTils.LogMsgEx( "Erreur %d pour obtenir l'échantillon @ t=%gs",
														err, timeA );
													startSample := 0;
											END;
											err := QTils.SampleNumberAtMovieTime( fullMovie, 0, timeB, endSample );
											IF err <> noErr
												THEN
													QTils.LogMsgEx( "Erreur %d pour obtenir l'échantillon @ t=%gs",
														err, timeB );
													endSample := 0;
											END;
										ELSE
											startSample := 0; endSample := 0;
									END;
									QTils.ssprintf( msgStr, "Test de lecture à vitesse maximale:\n"+
											"début: t=%gs (vidéo), '%s'\n"+
											"fin: t=%gs (vidéo), '%s'\n"+
											"intervalle = %gs\n"+
											"durée réelle = %gs\n"+
											"\n%g fois temps réel\n",
												timeA, timeStampA,
												timeB, timeStampB,
												dt, wallTimeLapse, dt / wallTimeLapse
									);
									IF startSample < endSample
										THEN
											QTils.ssprintfAppend( msgStr, "taux observé = %gHz\n",
												VAL(Real64, (endSample - startSample)) / wallTimeLapse );
									END;
									PostMessage( "Résultat benchmark", msgStr^ );
								ELSE
									QTils.ssprintf( msgStr,
										"A: t=%gs, '%s'\n"+
										"B: t=%gs, '%s'\n"+
										"\nB-A = %gs\n",
											timeA, timeStampA, timeB, timeStampB, dt
									);
									PostMessage( "Intervalle temporel", msgStr^ );
							END;
							QTils.free(msgStr);
					END;
					timeA := timeB;
					Assign( timeStampB, timeStampA );
					timeB := -1.0;
		END;
	END;
END CalcTimeInterval;

PROCEDURE movieKeyUp(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
VAR
	evt : EventRecordPtr;
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
		| 'i', 'I' :
				showMetaData();
		| 'c', 'C' :
				QTils.QTMovieWindowToggleMCController(wih);
		| 'f', 'F':
				SetWindowLayer( HWND_TOP, wih );
		| 'b', 'B':
				SetWindowLayer( HWND_BOTTOM, wih );
		| '1':
				baseDescription.scale := baseDescription.scale / sqrt2;
				PlaceWindows( ULCorner, 1.0/sqrt2 );
		| '2':
				baseDescription.scale := baseDescription.scale * sqrt2;
				PlaceWindows( ULCorner, sqrt2 );
		| 't', 'T':
				CalcTimeInterval(FALSE, TRUE);
		| '0':
				BenchmarkPlaybackRate();
		| 'p', 'P':
				IF QTils.QTMovieWindowH_isOpen(qtwmH[pilotWin])
					THEN
						QTils.QTMovieWindowGetGeometry( qtwmH[pilotWin], ADR(ULCorner), NIL, 1 );
						ULCorner.horizontal := ULCorner.horizontal - Wsize[leftWin].horizontal;
						PlaceWindows( ULCorner, 1.0 );
				END;
		ELSE
			(* noop *)
	END;
	RETURN 0;
END movieKeyUp;

PROCEDURE register_window( AqtwmH : ARRAY OF QTMovieWindowH; w : CARDINAL );
VAR
	qtwmH : QTMovieWindowH;
	err, err2 : ErrCode;
BEGIN
	qtwmH := AqtwmH[w];
	IF QTils.QTMovieWindowH_Check(qtwmH)
		THEN
			handlingPlayAction := FALSE;
			(* le membre idx n'est signifiant que nous nous, pas pour QTils,
			 * donc on peut l'intialiser comme convenable
			 *)
			qtwmH^^.idx := w;
			(* void statement: *)
			(* QTils.register_MCAction( qtwmH, 0, moviePlay ); *)
			QTils.register_MCAction( qtwmH, MCAction.Step, movieStep );
			QTils.register_MCAction( qtwmH, MCAction.Play, moviePlay );
			QTils.register_MCAction( qtwmH, MCAction.GoToTime, movieScan );
			QTils.register_MCAction( qtwmH, MCAction.Start, movieStart );
			QTils.register_MCAction( qtwmH, MCAction.Stop, movieStop );
			QTils.register_MCAction( qtwmH, MCAction.Close, movieClose );
			QTils.register_MCAction( qtwmH, MCAction.Finished, movieFinished );
			QTils.register_MCAction( qtwmH, MCAction.KeyUp, movieKeyUp );
			err := QTils.QTMovieWindowGetGeometry( qtwmH, ADR(Wpos[w]), ADR(Wsize[w]), 1 );
			IF (AqtwmH[fwWin] <> NULL_QTMovieWindowH) AND (w <> fwWin)
				THEN
					err2 := QTils.SlaveMovieToMasterMovie( qtwmH^^.theMovie, AqtwmH[fwWin]^^.theMovie );
					IF err2 <> noErr
						THEN
							QTils.LogMsgEx( "register_window(%d): SlaveMovieToMasterMovie(fwWin) a retourné erreur %d",
								w, err2 );
					END;
			END;
			numQTWM := numQTWM + 1;
	END;
END register_window;

PROCEDURE PlaceWindows( ULCorner : Cartesian; scale : Real64 );
VAR
	pos : Cartesian;
	w : CARDINAL;
BEGIN
	QTils.LogMsgEx( "placement des fenêtres par rapport à (%hd,%hd)",
		ULCorner.horizontal, ULCorner.vertical );

	IF ( (scale <> 1.0) AND (scale > 0.0) )
		THEN
			FOR w := 0 TO maxQTWM DO
				IF ( (w <> tcWin) AND QTils.QTMovieWindowH_Check(qtwmH[w]) )
					THEN
						QTils.QTMovieWindowSetGeometry( qtwmH[w], NIL, NIL, scale, 0 );
						QTils.QTMovieWindowGetGeometry( qtwmH[w], ADR(Wpos[w]), ADR(Wsize[w]), 1 );
				END;
			END;
	END;

	(* vue pilote en haut et au milieu *)
	pos.horizontal := ULCorner.horizontal + Wsize[pilotWin (*1?!*)].horizontal;
	pos.vertical := ULCorner.vertical;
	QTils.QTMovieWindowSetGeometry( qtwmH[pilotWin], ADR(pos), NULL_Cartesian, 1.0, 1 );

	(* vue lat-gauche en 2e ligne, à gauche *)
	pos.horizontal := ULCorner.horizontal;
	pos.vertical := ULCorner.vertical + Wsize[pilotWin].vertical;
	QTils.QTMovieWindowSetGeometry( qtwmH[leftWin], ADR(pos), NULL_Cartesian, 1.0, 1 );

	(* vue vers l'avant en 2e ligne, au milieu, sous la vue pilote *)
	pos.horizontal := pos.horizontal + Wsize[leftWin].horizontal;
	QTils.QTMovieWindowSetGeometry( qtwmH[fwWin], ADR(pos), NULL_Cartesian, 1.0, 1 );

	(* vue lat-droite en 2e ligne, à droite *)
	pos.horizontal := pos.horizontal + Wsize[fwWin].horizontal;
	QTils.QTMovieWindowSetGeometry( qtwmH[rightWin], ADR(pos), NULL_Cartesian, 1.0, 1 );

	(* TimeCode en 3e ligne, centrée *)
	pos.horizontal := ULCorner.horizontal + (Wsize[leftWin].horizontal + Wsize[pilotWin].horizontal/2) - Wsize[tcWin].horizontal/2;
	pos.vertical := ULCorner.vertical + Wsize[pilotWin].vertical + Wsize[leftWin].vertical;
	IF ( pos.horizontal < 0 )
		THEN
			pos.horizontal := 0;
	END;
	QTils.QTMovieWindowSetGeometry( qtwmH[tcWin], ADR(pos), NULL_Cartesian, 1.0, 1 );
END PlaceWindows;

PROCEDURE isPlayable(wih : QTMovieWindowH) : BOOLEAN;
BEGIN
	RETURN QTils.QTMovieWindowH_isOpen(wih) & (wih^^.loadState >= kMovieLoadStatePlaythroughOK);
END isPlayable;

(* enregistre un fichier .qi2m qui reconstruit le "design" actuel dans un seul movie QuickTime *)
PROCEDURE CreateQI2MFromDesign( VAR URL : URLString; descr : VODDescription );
TYPE
  (*
	 * type d'array dont chaque élément (1..4) définit, pour une vue donnée, quelle translation
	 * est nécessaire pour avoir le canal correspondant à l'endroit indiqué
	 *)
	dxyArray = ARRAY[1..4] OF RECORD dx, dy : Real64 END;
CONST
(* cartographie pour les translations de # canal vers vue Pilote/Gauche/Avant/Droite;
 * "ok" veut dire vérifié.
 * 0,0       --> 0,0
 * | 1 | 2 |     |    | Pi |    |
 * | 3 | 4 |     | Ga | Av | Dr |
 * Par exemple:
 * chPiTrans[descr.channels.pilot].dx donne la translation horizontale pour mettre le canal
 * contenant la vue pilote à l'endroit "Pi" de la vue panoramique.
 *)
	chPiTrans = dxyArray{ {1.0,0.0}, (*ok*){0.0,0.0}, (*ok*){1.0,-1.0}, {0.0,-1.0} };
	chGaTrans = dxyArray{ (*ok*){0.0,1.0}, {-1.0,1.0}, (*ok*){0.0,0.0}, {-1.0,0.0} };
	chAvTrans = dxyArray{ (*ok*){1.0,1.0}, (*ok*){0.0,1.0}, {1.0,0.0}, {-1.0,0.0} };
	chDrTrans = dxyArray{ {2.0,1.0}, {1.0,1.0}, {2.0,0.0}, (*ok*){1.0,0.0} };
VAR
	fName, source, xmlStr : URLString;
	useVMGI, flLR : ARRAY[0..7] OF CHAR;
	fp : ChanId;
	res : OpenResults;
	channels : VODChannels;
BEGIN
	(*Concat( URL, "-design", fName );
	Append( ".qi2m", fName ); *)
	(* 20121121: le fichier design.qi2m est également créé dans le répertoire cache *)
	POSIX.sprintf( fName, "%svid\design.qi2m", URL );
	IF NOT CreateDirTree(fName)
		THEN
			(* on retourne sur l'ancien comportement *)
			POSIX.sprintf( fName, "%s-design.qi2m", URL );
	END;
	Concat( URL, ".VOD", source );

	Open( fp, fName, write, res );
	IF ( res = fileExists )
		THEN
			DeleteFile(fName);
		ELSE
			Close(fp);
	END;
	Open( fp, fName, write+old, res );

	IF ( res = opened )
		THEN
			WriteString( fp, '<?xml version="1.0"?>' ); WriteLn(fp);
			WriteString( fp, '<?quicktime type="video/x-qt-img2mov"?>' ); WriteLn(fp);
			(* on active l'autoSave sur l'importation pour une création automatique du fichier cache *)
			WriteString( fp, '<import autoSave=True >' ); WriteLn(fp);

			IF descr.useVMGI
				THEN
					Assign( "True", useVMGI );
				ELSE
					Assign( "False", useVMGI );
			END;
			IF descr.flipLeftRight
				THEN
					Assign( "True", flLR );
				ELSE
					Assign( "False", flLR );
			END;

			IF LENGTH(assocDataFileName) = 0
				THEN
					QTils.sprintf( xmlStr, '\t<description txt="UTC timeZone=%g, DST=%hd" />',
						descr.timeZone, VAL(Int16,descr.DST) );
				ELSE
					QTils.sprintf( xmlStr, '\t<description txt="UTC timeZone=%g, DST=%hd, assoc.data:%s" />',
						descr.timeZone, VAL(Int16,descr.DST), assocDataFileName );
			END;
			WriteString( fp, xmlStr ); WriteLn(fp);

(* on importe le fichier VOD d'origine, sans transcodage, donc pas besoin d'adapter l'algorithme
	si oui ou non l'importation se fait en pistes distinctes ...
			IF fullMovieIsSplit
				THEN
					(* une vidéo avec les vues des 4 caméras dans 4 pistes distinctes *)
					channels.forward := -1;
					channels.pilot := -1;
					channels.left := -1;
					channels.right := -1;
				ELSE
					channels := descr.channels;
			END;
*)
			channels := descr.channels;
			QTils.sprintf( xmlStr,
				'\t<sequence src="%s" freq=%g channel=%hd timepad=False hflip=%s vmgi=%s\n'
				+ '\t\trelTransH=%g relTransV=%g newchapter=True description="%s" />\n',
				source, descr.frequency, VAL(Int16,6), "False", useVMGI, 0.5, 0.0, "timeStamps" );
			WriteString( fp, xmlStr );

			QTils.sprintf( xmlStr,
				'\t<sequence src="%s" freq=%g channel=%d timepad=False hflip=%s vmgi=%s addtc=False hidets=True\n'
				+ '\t\trelTransH=%g relTransV=%g description="%s" />\n',
				source, descr.frequency, channels.pilot, "False", useVMGI,
				chPiTrans[descr.channels.pilot].dx, chPiTrans[descr.channels.pilot].dy, "pilot" );
			WriteString( fp, xmlStr );

			QTils.sprintf( xmlStr,
				'\t<sequence src="%s" freq=%g channel=%d timepad=False hflip=%s vmgi=%s addtc=False hidets=True\n'
				+ '\t\trelTransH=%g relTransV=%g description="%s" />\n',
				source, descr.frequency, channels.left, flLR, useVMGI,
				chGaTrans[descr.channels.left].dx, chGaTrans[descr.channels.left].dy, "left" );
			WriteString( fp, xmlStr );


			QTils.sprintf( xmlStr,
				'\t<sequence src="%s" freq=%g channel=%d timepad=False hflip=%s vmgi=%s addtc=False hidets=True\n'
				+ '\t\trelTransH=%g relTransV=%g description="%s" />\n',
				source, descr.frequency, channels.right, flLR, useVMGI,
				chDrTrans[descr.channels.right].dx, chDrTrans[descr.channels.right].dy, "right" );
			WriteString( fp, xmlStr );

			QTils.sprintf( xmlStr,
				'\t<sequence src="%s" freq=%g channel=%d timepad=False hflip=%s vmgi=%s addtc=False hidets=True\n'
				+ '\t\trelTransH=%g relTransV=%g description="%s" />\n',
				source, descr.frequency, channels.forward, "False", useVMGI,
				chAvTrans[descr.channels.forward].dx, chAvTrans[descr.channels.forward].dy, "forward" );
			WriteString( fp, xmlStr );

			WriteString( fp, '</import>' ); WriteLn(fp);
			Close(fp);
	END;
END CreateQI2MFromDesign;

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

PROCEDURE CreateChannelView( src : ARRAY OF CHAR; description : VODDescription; channel : INTEGER;
														chName : ARRAY OF CHAR; scale : Real64; VAR fName : ARRAY OF CHAR ) : QTMovieWindowH;
VAR
	chanNum : ARRAY[0..16] OF CHAR;
	source : ARRAY[0..1024] OF CHAR;
	errString, errComment : ARRAY[0..127] OF CHAR;
%IF USECHANNELVIEWIMPORTFILES %THEN
	tmpStr : ARRAY[0..255] OF CHAR;
	fp : ChanId;
%ELSE
	qi2mString : String1kPtr;
	memRef : MemoryDataRef;
	err : ErrCode;
	theMovie : Movie;
%END
	res : OpenResults;
	wih : QTMovieWindowH;
BEGIN
	IntToStr( channel, chanNum );
	(*Concat( src, chName, fName );
	Append( ".mov", fName );*)
	(* 20121121: c'est plus propre de stocker les fichiers cache dans un répertoire spécifique;
		CreateDirTree() permet de faire ça facilement *)
	POSIX.sprintf( fName, "%svid\%s.mov", src, chName );
	IF NOT CreateDirTree(fName)
		THEN
			(* on retourne à l'ancien comportement *)
			POSIX.sprintf( fName, "%s-%s.mov", src, chName );
	END;
	IF NOT recreateChannelViews
		THEN
			(* si la vue sur <channel> existe déjà en cache (fichier .mov), on l'ouvre directement dans
			 * une fenêtre *)
			IF ( (channel = 5) OR (channel = 6) )
				THEN
					wih := QTils.OpenQTMovieInWindow( fName, 1 );
				ELSE
					wih := QTils.OpenQTMovieInWindow( fName, vues_avec_controleur );
			END;
		ELSE
			wih := NULL_QTMovieWindowH;
	END;
	IF ( wih = NULL_QTMovieWindowH )
		THEN
%IF USECHANNELVIEWIMPORTFILES %THEN
			(* on va créer le fichier .qi2m pour ouvrir <channel> dans la vidéo <src> *)
			Delete( fName, LENGTH(fName)-4, 4 );
			Append( ".qi2m", fName );
			QTils.LogMsgEx( "création %s pour canal %d", fName, channel );
			Open( fp, fName, write+old, res );
			IF ( res = opened )
				THEN
					Concat( src, ".mov", source );
					WriteString( fp, '<?xml version="1.0"?>' ); WriteLn(fp);
					WriteString( fp, '<?quicktime type="video/x-qt-img2mov"?>' ); WriteLn(fp);
					(* on active l'autoSave sur l'importation pour une création automatique du fichier cache *)
					WriteString( fp, '<import autoSave=True >' ); WriteLn(fp);
					(*
							On doit ajouter la description voulue car il s'agit de méta-données au niveau du Movie
							qui ne sont pas importées à partir du Movie source.
					*)
					IF LENGTH(assocDataFileName) = 0
						THEN
							QTils.sprintf( tmpStr, '\t<description txt="UTC timeZone=%g, DST=%hd" />',
								description.timeZone, VAL(Int16,description.DST) );
						ELSE
							QTils.sprintf( tmpStr, '\t<description txt="UTC timeZone=%g, DST=%hd, assoc.data:%s" />',
								description.timeZone, VAL(Int16,description.DST), assocDataFileName );
					END;
					(* 20110913 - tmpStr was never printed?! *)
					WriteString( fp, tmpStr ); WriteLn(fp);
					WriteChar( fp, tabChar );
					(* début de la définition d'une séquence: *)
					WriteString( fp, '<sequence src="' );
					WriteString( fp, source );
					IF NOT fullMovieIsSplit
						THEN
							WriteString( fp, '" channel=' );
							WriteString( fp, chanNum );
					END;
					IF ( (channel <> 5) AND (channel <> 6) )
						THEN
							(* on cache la piste TimeCode des vues des 4 caméras (hidetc=True)
							 * et on force l'interprétation comme 'movie' *)
							WriteString( fp, ' hidetc=True timepad=False asmovie=True' );
							IF description.flipLeftRight AND ((channel = description.channels.left) OR (channel = description.channels.right))
								THEN
									(* vues à inverser horizontalement *)
									WriteString( fp, ' hflip=True' );
								ELSE
									WriteString( fp, ' hflip=False' );
							END;
						ELSE
							(* channel no. 5 = la piste TimeCode; no. 6 = la piste texte avec l'horodatage des trames *)
							WriteString( fp, ' hidetc=False timepad=False asmovie=True newchapter=False' );
					END;
					WriteString( fp, ' />' ); WriteLn(fp);
					WriteString( fp, '</import>' ); WriteLn(fp);
					Close(fp);
					(* fichier .qi2m créé, on l'ouvre maintenant, d'abord sans fenêtre *)
					err := QTils.OpenMovieFromURL( theMovie, 1, fName );
					IF err = noErr
						THEN
							(* on prépare le movie pour être affiché *)
							IF PrepareChannelCacheMovie( theMovie, description.channels, channel )
								THEN
									QTils.SaveMovie(theMovie);
							END;
							IF ( (channel = 5) OR (channel = 6) )
								THEN
									wih := QTils.OpenQTMovieWindowWithMovie( theMovie, "", 1 );
								ELSE
									wih := QTils.OpenQTMovieWindowWithMovie( theMovie, "", vues_avec_controleur );
							END;
					END;
%IF NOTOUTCOMMENTED %THEN
					IF ( (channel = 5) OR (channel = 6) )
						THEN
							wih := QTils.OpenQTMovieInWindow( fName, 1 );
						ELSE
							wih := QTils.OpenQTMovieInWindow( fName, vues_avec_controleur );
					END;
%END
					IF ( wih = NULL_QTMovieWindowH )
						THEN
							IF QTils.MacErrorString( QTils.LastQTError(), errString, errComment ) <> 0
								THEN
									QTils.LogMsgEx( "Echec d'importation: %s (%s)", errString, errComment );
								ELSE
									QTils.LogMsgEx( "Echec d'importation %d", QTils.LastQTError() );
							END;
							PostMessage( fName, QTils.lastSSLogMsg^ );
						ELSE
							DeleteFile(fName);
							QTils.LogMsgEx( "%s importé et supprimé", fName );
					END;
				ELSE
					PostMessage( fName, "Echec de création/ouverture" );
			END;
%ELSE (* NOT USECHANNELVIEWIMPORTFILES *)
			qi2mString := NIL;
			Concat( src, ".mov", source );
			QTils.ssprintf( qi2mString,
					'<?xml version="1.0"?>\n<?quicktime type="video/x-qt-img2mov"?>\n<import autoSave=True autoSaveName=%s >\n',
					(* on active l'autoSave sur l'importation pour une création automatique du fichier cache *)
					fName );
			IF LENGTH(assocDataFileName) = 0
				THEN
					QTils.ssprintfAppend( qi2mString, '\t<description txt="UTC timeZone=%g, DST=%hd" />\n',
						description.timeZone, VAL(Int16,description.DST) );
				ELSE
					QTils.ssprintfAppend( qi2mString, '\t<description txt="UTC timeZone=%g, DST=%hd, assoc.data:%s" />\n',
						description.timeZone, VAL(Int16,description.DST), assocDataFileName );
			END;
			(* début de la définition d'une séquence: *)
			IF fullMovieIsSplit
				THEN
					QTils.ssprintfAppend( qi2mString, '\t<sequence src="%s"', source );
				ELSE
					QTils.ssprintfAppend( qi2mString, '\t<sequence src="%s" channel=%s', source, chanNum );
			END;
			IF ( (channel <> 5) AND (channel <> 6) )
				THEN
					(* on cache la piste TimeCode des vues des 4 caméras (hidetc=True)
					 * et on force l'interprétation comme 'movie' *)
					QTils.ssprintfAppend( qi2mString, '%s', ' hidetc=True timepad=False asmovie=True' );
					IF description.flipLeftRight AND ((channel = description.channels.left) OR (channel = description.channels.right))
						THEN
							(* vues à inverser horizontalement *)
							QTils.ssprintfAppend( qi2mString, '%s', ' hflip=True' );
						ELSE
							QTils.ssprintfAppend( qi2mString, '%s', ' hflip=False' );
					END;
				ELSE
					(* channel no. 5 = la piste TimeCode; no. 6 = la piste texte avec l'horodatage des trames *)
					QTils.ssprintfAppend( qi2mString, '%s', ' hidetc=False timepad=False asmovie=True newchapter=False' );
			END;
			QTils.ssprintfAppend( qi2mString, '%s\n%s\n', ' />', '</import>' );
			IF qi2mString <> NIL
				THEN
					err := QTils.MemoryDataRefFromString( qi2mString^, fName, memRef );
					IF err = noErr
						THEN
							(* le MemoryDataRef a été créé, on l'ouvre maintenant, d'abord dans un .mov *)
							err := QTils.OpenMovieFromMemoryDataRef( theMovie, memRef, FOUR_CHAR_CODE('QI2M') );
							IF err = noErr
								THEN
									IF PrepareChannelCacheMovie( theMovie, description.channels, channel )
										THEN
											QTils.SaveMovieAsRefMov( fName, theMovie );
									END;
									IF ( (channel = 5) OR (channel = 6) )
										THEN
											wih := QTils.OpenQTMovieWindowWithMovie( theMovie, "", 1 );
										ELSE
											wih := QTils.OpenQTMovieWindowWithMovie( theMovie, "", vues_avec_controleur );
									END;
							END;
							IF ( wih = NULL_QTMovieWindowH )
								THEN
									IF QTils.MacErrorString( QTils.LastQTError(), errString, errComment ) <> 0
										THEN
											QTils.LogMsgEx( "Echec d'importation: %s (%s)\n%s\n", errString, errComment, qi2mString^ );
										ELSE
											QTils.LogMsgEx( "Echec d'importation %d\n%s\n", QTils.LastQTError(), qi2mString^ );
									END;
									PostMessage( fName, QTils.lastSSLogMsg^ );
									QTils.DisposeMemoryDataRef(memRef);
								ELSE
									QTils.LogMsgEx( "%s importé de mémoire", fName );
							END;
						ELSE
							QTils.LogMsgEx( "Echec de création de MemoryDataRef pour canal %s: %d", chanNum, err );
							PostMessage( fName, QTils.lastSSLogMsg^ );
					END;
					QTils.free(qi2mString);
				ELSE
					QTils.LogMsgEx( "Echec de création de %s en mémoire", fName );
					PostMessage( fName, QTils.lastSSLogMsg^ );
			END;
%END
	END;
	IF QTils.QTMovieWindowH_isOpen(wih)
		THEN
		IF ( (scale <> 1.0) & (scale > 0.0) )
			THEN
				QTils.QTMovieWindowSetGeometry( wih, NIL, NIL, scale, 0 );
		END;
		QTils.SetMoviePlayHints( wih^^.theMovie, hintsPlayingEveryFrame, 1 );
	END;
	RETURN wih;
END CreateChannelView;

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

VAR
	inOpenVideo : BOOLEAN;

PROCEDURE PrepareChannelCacheMovie( theMovie : Movie; channels : VODChannels; chanNum : INTEGER ) : BOOLEAN;
CONST
	camMask = "Camera ";
VAR
	trackNr, trackCount : Int32;
	camString, camTrackString : ARRAY[0..127] OF CHAR;
	lang : ARRAY[0..15] OF CHAR;
	found : BOOLEAN;
	pos : CARDINAL;
BEGIN
	IF (chanNum <> 5) AND (chanNum <> 6)
		THEN
			trackNr := -1;
			IF (QTils.GetTrackWithName( theMovie, "timeStamp Track", trackNr ) = noErr) AND (trackNr >= 0)
				THEN
					QTils.LogMsgEx( "Desactivation de la piste %ld, timeStamp Track", trackNr );
					QTils.DisableTrack( theMovie, trackNr );
			END;
	END;
	trackNr := -1;
	IF (QTils.GetTrackWithName( theMovie, "Timecode Track", trackNr ) = noErr) AND (trackNr >= 0)
		THEN
			QTils.LogMsgEx( "Activation de la piste %ld, Timecode Track", trackNr );
			QTils.EnableTrack( theMovie, trackNr );
	END;
	(* si on a une fullMovie avec 4 pistes distinctes pour les 4 caméras ET on prépare la vue
		d'une de ces caméras (et non pas la vue TimeCode), on désactive toutes les pistes caméra
		qui ne correspondent pas à la caméra demandée *)
	IF fullMovieIsSplit AND (chanNum > 0)
		THEN
			trackCount := QTils.GetMovieTrackCount(theMovie) + 1;
			(* la caméra qu'on traite ici: *)
			QTils.sprintf( camString, "Camera %d", chanNum );
			FOR trackNr := 0 TO trackCount DO;
				(* le numéro de la caméra est associé à la méta-donnée 'cam#' de la piste: *)
				IF QTils.GetMetaDataStringFromTrack( theMovie, trackNr, FOUR_CHAR_CODE("cam#"), camTrackString, lang ) = noErr
					THEN
						(* on a une chaine issue des méta-données, mais elle peut être vide ou correspondre à autre chose: *)
						FindNext( camMask, camTrackString, 0, found, pos );
						IF found
							THEN
								(* c'est bien une chaine de forme "Camera "... maintenant faisons la comparaison: *)
								IF Equal( camString, camTrackString )
									THEN
										(* c'est la bonne piste *)
										QTils.EnableTrack( theMovie, trackNr );
										(* on sauvegarde le numéro de la piste pour référence future *)
										IF channels.forward = chanNum
											THEN
												splitCamTrack.forward := trackNr;
											ELSIF channels.pilot = chanNum
												THEN
													splitCamTrack.pilot := trackNr;
											ELSIF channels.left = chanNum
												THEN
													splitCamTrack.left := trackNr;
											ELSIF channels.right = chanNum
												THEN
													splitCamTrack.right := trackNr;
										END;
									ELSE
										(* c'est la piste d'une autre caméra: on désactive *)
										QTils.DisableTrack( theMovie, trackNr );
								END;
						END;
				END;
			END;
	END;
	RETURN (QTils.HasMovieChanged(theMovie) <> 0);
END PrepareChannelCacheMovie;

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

PROCEDURE OpenVideo( VAR URL : URLString; VAR description : VODDescription ) : ErrCode;
VAR
	fName : URLString;
	errString, errComment, quadString : ARRAY[0..127] OF CHAR;
	lang : ARRAY[0..15] OF CHAR;
	w : CARDINAL;
	err : ErrCode;
	isVODFile : BOOLEAN;
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
					isVODFile := HasExtension( fName, ".VOD" );
					PruneExtensions(fName);
					URL := fName;
					Assign( fName, baseFileName );
				ELSE
					RETURN 1
			END;
		ELSE
			isVODFile := HasExtension( URL, ".VOD" );
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
					err := OpenVideo( URL, description );
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
(*
			fullMovieWMH := QTils.OpenQTMovieWindowWithMovie( fullMovie, fName, 1 );
			IF fullMovieWMH <> NULL_QTMovieWindowH
				THEN
					ShowWindow( fullMovieWMH^^.theView, SW_MINIMIZE );
			END;
*)
	END;

	(* maintenant on peut tenter d'ouvrir les 5 fenêtres *)
	(* vue vers l'avant *)
	qtwmH[fwWin] := CreateChannelView( URL, description, description.channels.forward, "forward",
			description.scale, fName );
	(* vue conducteur *)
	qtwmH[pilotWin] := CreateChannelView( URL, description, description.channels.pilot, "pilot",
			description.scale, fName );
	(* vue latérale-gauche *)
	qtwmH[leftWin] := CreateChannelView( URL, description, description.channels.left, "left",
			description.scale, fName );
	(* vue latérale-droite *)
	qtwmH[rightWin] := CreateChannelView( URL, description, description.channels.right, "right",
			description.scale, fName );
	(* la piste TimeCode qui donne le temps pour les 4 vues *)
	qtwmH[tcWin] := CreateChannelView( URL, description, TimeCodeChannel, "TC", 1.0, fName );

	(* enregistrement des fonctions de gestion d'actions et MAJ de numQTMW *)
	FOR w := 0 TO maxQTWM	DO
		register_window(qtwmH, w);
	END;
(*
	QTils.register_MCAction( qtwmH[fwWin], MCAction.AnyAction, PumpSubscriptions );
*)
	QTils.NewTimedCallBackRegisterForMovie( qtwmH[fwWin]^^.theMovie, callbackRegister, qtCallBacksAllowedAtInterrupt );
	QTils.TimedCallBackRegisterFunctionInTime( qtwmH[fwWin]^^.theMovie, callbackRegister,
		1.5, timeCallBack, CAST(Int32,qtwmH[fwWin]), qtCallBacksAllowedAtInterrupt );

	PlaceWindows( ULCorner, 1.0);

	IF err = noErr
		THEN
			err := QTils.LastQTError();
	END;

	IF (err = noErr) AND isVODFile
		THEN
			CreateQI2MFromDesign(URL, description);
	END;

	RETURN err;
END OpenVideo;

PROCEDURE FlushCaches;
VAR
	err : ErrCode;
	tfName, fName : URLString;
BEGIN
		Concat( baseFileName, ".mov", fName );
		err := QTils.SaveMovie( fullMovie );
		QTils.LogMsgEx( 'Sauvegarde du fichier "%s" : err=%d\n', fName, err );
		IF err <> noErr
			THEN
				PostMessage( "QTVODm2 - erreur", QTils.lastSSLogMsg^ );
			ELSE
				POSIX.sprintf( tfName, "%svid\forward.mov", baseFileName );
				IF CreateDirTree(tfName)
					THEN
						DeleteFile(tfName);
						POSIX.sprintf( tfName, "%svid\pilot.mov", baseFileName );
						DeleteFile(tfName);
						POSIX.sprintf( tfName, "%svid\left.mov", baseFileName );
						DeleteFile(tfName);
						POSIX.sprintf( tfName, "%svid\right.mov", baseFileName );
						DeleteFile(tfName);
						POSIX.sprintf( tfName, "%svid\TC.mov", baseFileName );
						DeleteFile(tfName);
					ELSE
						(* on retourne à l'ancien comportement *)
						Concat( baseFileName, "-forward.mov", tfName );
						DeleteFile(tfName);
						Concat( baseFileName, "-pilot.mov", tfName );
						DeleteFile(tfName);
						Concat( baseFileName, "-left.mov", tfName );
						DeleteFile(tfName);
						Concat( baseFileName, "-right.mov", tfName );
						DeleteFile(tfName);
						Concat( baseFileName, "-TC.mov", tfName );
						DeleteFile(tfName);
				END;
		END;
END FlushCaches;

PROCEDURE CloseVideo(final : BOOLEAN);
VAR
	w : CARDINAL;
BEGIN
	IF callbackRegister <> NIL
		THEN
			QTils.DisposeCallBackRegister(callbackRegister);
			callbackRegister := NIL;
	END;
	IF final
		THEN
			FOR w := 0 TO maxQTWM	DO
				IF( qtwmH[w] <> NULL_QTMovieWindowH )
					THEN
						QTils.DisposeQTMovieWindow(qtwmH[w]);
						numQTWM := numQTWM - 1;
				END;
			END;
		ELSE
			FOR w := 0 TO maxQTWM	DO
				IF( QTils.QTMovieWindowH_isOpen(qtwmH[w]) )
					THEN
						QTils.CloseQTMovieWindow(qtwmH[w]);
						numQTWM := numQTWM - 1;
				END;
			END;
	END;
	IF (fullMovieChanged OR CAST(BOOLEAN,QTils.HasMovieChanged(fullMovie)))
			AND PostYesNoDialog( baseFileName,
					"le movie (.mov) de cet enregistrement a changé, voulez-vous le sauvegarder?" )
		THEN
			FlushCaches();
	END;
	IF fullMovieWMH <> NULL_QTMovieWindowH
		THEN
			QTils.CloseQTMovieWindow(fullMovieWMH);
			fullMovie := NIL;
		ELSE
			QTils.CloseMovie(fullMovie);
	END;
END CloseVideo;

PROCEDURE ResetVideo(complete : BOOLEAN) : ErrCode;
VAR
	fName : URLString;
	t : Real64;
	w : CARDINAL;
	err : ErrCode;
BEGIN
	t := -1.0;
	IF ( complete AND (baseFileName[0] <> CHR(0)) )
		THEN
			Concat( baseFileName, ".mov", fName );
			DeleteFile(fName);
			QTils.LogMsgEx( "QTVODm2::ResetVideo: '%s' supprimée", fName );
	END;
	FOR w := 0 TO maxQTWM	DO
		IF( QTils.QTMovieWindowH_Check(qtwmH[w]) )
			THEN
				(* on essaie de savoir le temps actuel *)
				IF (t < 0.0)
					THEN
						IF ( QTils.QTMovieWindowGetTime( qtwmH[w], t, 0 ) = noErr )
							THEN
								QTils.LogMsgEx( "QTVODm2::ResetVideo: temps à restorer: %gs", t );
						END;
				END;
				DeleteFile( qtwmH[w]^^.theURL^ );
				(* NB ... il faut lire <theURL> maintenant, après DisposeQTMovieWindow() ce serait boom... *)
				QTils.LogMsgEx( "QTVODm2::ResetVideo: '%s' fermée et supprimée", qtwmH[w]^^.theURL^ );
				QTils.DisposeQTMovieWindow(qtwmH[w]);
				numQTWM := numQTWM - 1;
		END;
	END;
	err := paramErr;
	IF ( numQTWM = 0 )
		THEN
			err := OpenVideo( baseFileName, baseDescription );
			IF ( (err = noErr) AND (NOT complete) AND (t >= 0.0) )
				THEN
					SetTimes( t, NIL, 0 );
					QTils.LogMsgEx( "QTVODm2::ResetVideo: temps restoré: %gs", t );
			END;
	END;
	RETURN err;
END ResetVideo;

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

PROCEDURE ClipInt( VAR val : INTEGER; min, max : INTEGER );
BEGIN
	IF ( val < min )
		THEN
			val := min;
		ELSIF ( val > max )
			THEN
				val := max;
	END;
END ClipInt;

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
	elm : CARDINAL;
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

(* ==================================== BEGIN ==================================== *)
BEGIN

	inOpenVideo := FALSE;
	numQTWM := 0;
	baseFileName := "";
	assocDataFileName := "";
	MetaDataDisplayStrPtr := NIL;
	FOR idx := 0 TO maxQTWM	DO
		qtwmH[idx] := NULL_QTMovieWindowH;
	END;
	sServeur := sock_nulle;
	xmlParser := NIL;
	xmldoc := NIL;
	sqrt2 := sqrt(2.0);
	ULCorner.horizontal := 0;
	ULCorner.vertical := 0;
	fullMovie := NIL;
	fullMovieChanged := FALSE;
	theTimeInterVal.timeA := -1.0;
	theTimeInterVal.dt := 0.0;
	baseDescription.useVMGI := TRUE;
	baseDescription.log := FALSE;
	fullMovieWMH := NULL_QTMovieWindowH;
	fullMovieIsSplit := FALSE;
	InitHRTime();
	splitCamTrack.forward := -1;
	splitCamTrack.pilot := -1;
	splitCamTrack.left := -1;
	splitCamTrack.right := -1;
	currentTimeSubscription.sendInterval := 0.0;
	currentTimeSubscription.lastSentTime := 0.0;
	currentTimeSubscription.lastMovieTime := -1.0;
	currentTimeSubscription.absolute := FALSE;
	callbackRegister := NIL;

FINALLY

	CloseCommClient(sServeur);
	IF xmlParser <> NIL
		THEN
			QTils.DisposeXMLParser( xmlParser, xmldoc, 1 );
	END;

END QTVODlib.
