IMPLEMENTATION MODULE QTVODlibext;

(* USECHANNELVIEWIMPORTFILES : correspond à l'ancien comportement où pour créer une vidéo cache
	pour les 4+1 canaux on créait d'abord un fichier .qi2m d'importation *)
<*/VALIDVERSION:USECHANNELVIEWIMPORTFILES,COMMTIMING,NOTOUTCOMMENTED,USE_TIMEDCALLBACK,CCV_IN_BACKGROUND*>

FROM SYSTEM IMPORT
	CAST, ADR, ADDRESS;

FROM WholeStr IMPORT
	IntToStr;

FROM Strings IMPORT
	Concat, Append,
%IF USECHANNELVIEWIMPORTFILES %THEN
	Delete,
%END
	Assign, Equal, FindNext;

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
	WriteString,
%IF USECHANNELVIEWIMPORTFILES %THEN
	WriteChar,
%END
	WriteLn;
FROM FileFunc IMPORT
	DeleteFile, CreateDirTree;

%IF WIN32 %THEN
	FROM WIN32 IMPORT
		HWND;
%ELSE
	FROM Windows IMPORT
		HWND;
%END
FROM WINUSER IMPORT
	SetWindowPos, HWND_BOTTOM, HWND_TOP, SWP_NOACTIVATE, SWP_NOMOVE, SWP_NOSIZE;

FROM WIN32 IMPORT
	LARGE_INTEGER, QueryPerformanceFrequency, QueryPerformanceCounter;

%IF CCV_IN_BACKGROUND %THEN
FROM WIN32 IMPORT
	CRITICAL_SECTION, InitializeCriticalSectionAndSpinCount, DeleteCriticalSection,
	EnterCriticalSection, LeaveCriticalSection;
FROM Threads IMPORT
	Thread, CreateThread, WaitForThreadTermination, KillThread, WaitResult;
%END

%IF CHAUSSETTE2 %THEN
FROM Chaussette2 IMPORT
%ELSE
FROM Chaussette IMPORT
%END
	sock_nulle, SOCK;

(* importons tout de QTilsM2 et de QTVODcomm; on en aura besoin *)
FROM QTilsM2 IMPORT *;
FROM POSIXm2 IMPORT POSIX;
FROM QTVODcomm IMPORT *;
FROM QTVODlibbase IMPORT *;

TYPE

	ChannelDescription =
		RECORD
			source : URLString;
			description : VODDescription;
			channel : INTEGER;
			scale : Real64;
			chName : ARRAY[0..63] OF CHAR;
			fName : URLString;
			wih : QTMovieWindowH;
%IF CCV_IN_BACKGROUND %THEN
			done : BOOLEAN;
			thread : Thread;
			mutex : CRITICAL_SECTION;
%END
	END;
	ChannelDescriptionPtr = POINTER TO ChannelDescription;

VAR

	cbPT : Real64;
	channelDesc : ARRAY[0..maxQTWM] OF ChannelDescription;
	wIdx : CARDINAL;
%IF CCV_IN_BACKGROUND %THEN
	(* si TRUE, les 5 vues sont chargées dans des Threads individuels. Il se trouve
	 * que cela 1) n'accélère pas le processus d'ouverture et 2) que ce n'est pas une bonne
	 * idée car ni MSWin ni QuickTime sont conçus pour supporter à 100% des éléments IHM sur
	 * des threads autre que le principale.
	 *)
	CCV_IN_BACKGROUND : BOOLEAN;
%END

CONST

	(* si les vues des cameras ont un controleur sous l'image *)
	vues_avec_controleur = 0;


CONST
%IF USECHANNELVIEWIMPORTFILES %THEN
	tabChar = CHR(9);
%END
	TimeCodeChannel = 6 (*5*);

VAR

	Wpos, Wsize : ARRAY[0..maxQTWM] OF Cartesian;
	ULCorner : Cartesian;
	idx : CARDINAL;
	MetaDataDisplayStrPtr : StringPtr;
	sqrt2 : Real64;
	HPCcalibrator : Real64;
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

PROCEDURE SetTimes( t : Real64; ref : QTMovieWindowH; absolute : Int32 ) : BOOLEAN;
VAR
	w : CARDINAL;
	ret : BOOLEAN;
BEGIN
	FOR w := 0 TO maxQTWM DO
		IF qtwmH[w] <> ref
			THEN
				QTils.QTMovieWindowSetTime(qtwmH[w], t, absolute);
		END;
	END;
	ret := FALSE;
%IF USE_TIMEDCALLBACK %THEN
	IF (callbackRegister <> NIL) AND QTils.QTMovieWindowH_Check(qtwmH[fwWin])
		AND (currentTimeSubscription.lastForcedMovieTime <> t)
		THEN
			(* on doit remettre encore une pendule à l'heure: *)
			currentTimeSubscription.forcePump := TRUE;
			PumpSubscriptions( callbackRegister, CAST(Int32,qtwmH[fwWin]) );
			currentTimeSubscription.lastForcedMovieTime := t;
			currentTimeSubscription.forcePump := FALSE;
			ret := TRUE;
	END;
%END
	RETURN ret;
END SetTimes;

PROCEDURE SetTimesAndNotify( t : Real64; ref : QTMovieWindowH; absolute : Int32; ss : SOCK; class : NetMessageClass );
VAR
	msg : NetMessage;
	timeSent : BOOLEAN;
BEGIN
	timeSent := SetTimes( t, ref, absolute );
	IF (ss <> sock_nulle)
		AND ((NOT timeSent) OR (currentTimeSubscription.absolute <> VAL(BOOLEAN,absolute)))
		THEN
			replyCurrentTime( msg, class, ref, VAL(BOOLEAN,absolute) );
			IF ss <> sock_nulle
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
		QTils.LogMsgEx( "movieScan(): t=%gs tNew=%gs scanned=%d, stepped=%d", t, tNew^, wih^^.wasScanned, wih^^.wasStepped );
	IF ( t <> tNew^ )
		THEN
			IF wih^^.wasStepped < 0
				THEN
					(* 20130601: on a fait un pas (avec les touches curseur du clavier?), et on veut notifier le serveur! *)
					SetTimesAndNotify( tNew^, wih, 0, sServeur, qtvod_Notification );
				ELSE
					(* ici pas besoin de notifier le serveur - sauf s'il a fait une subscription ... un cas pris en compte
						par SetTimes *)
					SetTimes( tNew^, wih, 0 );
			END;
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

%IF USE_TIMEDCALLBACK %THEN
PROCEDURE PumpSubscriptions( cbRegister : QTCallBack; params : Int32 ) [CDECL];
%ELSE
PROCEDURE PumpSubscriptions(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
%END
VAR
	subscrTimer, dt : Real64;
	msg : NetMessage;
%IF USE_TIMEDCALLBACK %THEN
	wih : QTMovieWindowH;
%END
BEGIN
	subscrTimer := HRTime();
	(* on vérifie s'il faut envoyer le temps courant et s'il est temps de le faire *)
	WITH currentTimeSubscription DO
		dt := subscrTimer - lastSentTime;
%IF USE_TIMEDCALLBACK %THEN
		wih := CAST(QTMovieWindowH, params);
		IF (sendInterval > 0.0) OR forcePump
%ELSE
		IF ((sendInterval > 0.0) AND (dt >= sendInterval)) OR forcePump
%END
			THEN
				(* on obtient le temps courant dans la fenêtre *)
				replyCurrentTime( msg, qtvod_Subscription, wih, absolute );
				(* si ce temps est différent au temps précédemment envoyé, on envoie le message *)
				IF (msg.data.val1 <> lastMovieTime) OR forcePump
					THEN
(*
						QTils.LogMsgEx( "SUBS dt=%g-%g=%g movie#%d.dt=%g-%g=%g",
							subscrTimer, lastSentTime,
							dt, wih^^.idx,
							msg.data.val1, lastMovieTime,
							msg.data.val1 - lastMovieTime );
*)
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
%IF USE_TIMEDCALLBACK %THEN
	QTils.TimedCallBackRegisterFunctionInTime( wih^^.theMovie, cbRegister,
		currentTimeSubscription.sendInterval, PumpSubscriptions, params, qtCallBacksAllowedAtInterrupt );
%ELSE
	RETURN 0;
%END
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
				source, descr.frequency, VAL(Int16,6), "False", useVMGI, CAST(Real64,0.5), CAST(Real64,0.0), "timeStamps" );
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

PROCEDURE CCDWorker( args : ADDRESS ) : CARDINAL;
VAR
	CDp : ChannelDescriptionPtr;
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
%IF USECHANNELVIEWIMPORTFILES %THEN
	res : OpenResults;
%END
	wih : QTMovieWindowH;

	PROCEDURE logHere(lineNr : CARDINAL);
	BEGIN
%IF CCV_IN_BACKGROUND %THEN
		IF CCV_IN_BACKGROUND
			THEN
				QTils.LogMsgEx( "CCDWorker '%s'#%d @line %u", CDp^.source, CDp^.channel, lineNr );
		END;
%END
	END logHere;
	PROCEDURE lock();
	BEGIN
%IF CCV_IN_BACKGROUND %THEN
		IF CCV_IN_BACKGROUND
			THEN
				EnterCriticalSection(CDp^.mutex);
		END;
%END
	END lock;
	PROCEDURE unlock();
	BEGIN
%IF CCV_IN_BACKGROUND %THEN
		IF CCV_IN_BACKGROUND
			THEN
				LeaveCriticalSection(CDp^.mutex);
		END;
%END
	END unlock;

BEGIN

	CDp := args;

%IF CCV_IN_BACKGROUND %THEN
	IF CCV_IN_BACKGROUND
		THEN
			QTils.LogMsgEx( "CCDWorker started on '%s'#%d", CDp^.source, CDp^.channel );
			OpenQT();
	END;
%END

	IntToStr( CDp^.channel, chanNum );
	(*Concat( CDp^.source, CDp^.chName, CDp^.fName );
	Append( ".mov", CDp^.fName );*)
	(* 20121121: c'est plus propre de stocker les fichiers cache dans un répertoire spécifique;
		CreateDirTree() permet de faire ça facilement *)
	POSIX.sprintf( CDp^.fName, "%svid\%s.mov", CDp^.source, CDp^.chName );
	IF NOT CreateDirTree(CDp^.fName)
		THEN
			(* on retourne à l'ancien comportement *)
			POSIX.sprintf( CDp^.fName, "%s-%s.mov", CDp^.source, CDp^.chName );
	END;
	IF (NOT recreateChannelViews)
		AND (QTils.OpenMovieFromURLWithQTMovieWindowH( theMovie, 1, CDp^.fName, CDp^.wih ) = noErr)
		THEN
			(* si la vue sur <CDp^.channel> existe déjà en cache (fichier .mov), on l'ouvre directement dans
			 * une fenêtre *)
			IF ( (CDp^.channel = 5) OR (CDp^.channel = 6) )
				THEN
					(* wih := QTils.OpenQTMovieInWindow( CDp^.fName, 1 ); *)
					err := QTils.DisplayMovieInQTMovieWindowH( theMovie, CDp^.wih, CDp^.fName, 1 );
				ELSE
					(* wih := QTils.OpenQTMovieInWindow( CDp^.fName, vues_avec_controleur ); *)
					err := QTils.DisplayMovieInQTMovieWindowH( theMovie, CDp^.wih, CDp^.fName, vues_avec_controleur );
			END;
			wih := CDp^.wih;
		ELSE
			wih := NULL_QTMovieWindowH;
	END;
	IF ( wih = NULL_QTMovieWindowH )
		THEN
%IF USECHANNELVIEWIMPORTFILES %THEN
			(* on va créer le fichier .qi2m pour ouvrir <CDp^.channel> dans la vidéo <CDp^.source> *)
			Delete( CDp^.fName, LENGTH(CDp^.fName)-4, 4 );
			Append( ".qi2m", CDp^.fName );
			QTils.LogMsgEx( "création %s pour canal %d", CDp^.fName, CDp^.channel );
			Open( fp, CDp^.fName, write+old, res );
			IF ( res = opened )
				THEN
					Concat( CDp^.source, ".mov", source );
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
								CDp^.description.timeZone, VAL(Int16,CDp^.description.DST) );
						ELSE
							QTils.sprintf( tmpStr, '\t<description txt="UTC timeZone=%g, DST=%hd, assoc.data:%s" />',
								CDp^.description.timeZone, VAL(Int16,CDp^.description.DST), assocDataFileName );
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
					IF ( (CDp^.channel <> 5) AND (CDp^.channel <> 6) )
						THEN
							(* on cache la piste TimeCode des vues des 4 caméras (hidetc=True)
							 * et on force l'interprétation comme 'movie' *)
							WriteString( fp, ' hidetc=True timepad=False asmovie=True' );
							IF CDp^.description.flipLeftRight AND ((CDp^.channel = CDp^.description.channels.left)
								OR (CDp^.channel = CDp^.description.channels.right))
								THEN
									(* vues à inverser horizontalement *)
									WriteString( fp, ' hflip=True' );
								ELSE
									WriteString( fp, ' hflip=False' );
							END;
						ELSE
							(* CDp^.channel no. 5 = la piste TimeCode; no. 6 = la piste texte avec l'horodatage des trames *)
							WriteString( fp, ' hidetc=False timepad=False asmovie=True newchapter=False' );
					END;
					WriteString( fp, ' />' ); WriteLn(fp);
					WriteString( fp, '</import>' ); WriteLn(fp);
					Close(fp);
					(* fichier .qi2m créé, on l'ouvre maintenant, d'abord sans fenêtre *)
					err := QTils.OpenMovieFromURLWithQTMovieWindowH( theMovie, 1, CDp^.fName, CDp^.wih );
					IF err = noErr
						THEN
							(* on prépare le movie pour être affiché *)
							IF PrepareChannelCacheMovie( theMovie, CDp^.description.channels, CDp^.channel )
								THEN
									QTils.SaveMovie(theMovie);
							END;
							IF ( (CDp^.channel = 5) OR (CDp^.channel = 6) )
								THEN
									(* wih := QTils.OpenQTMovieWindowWithMovie( theMovie, "", 1 ); *)
									err := QTils.DisplayMovieInQTMovieWindowH( theMovie, CDp^.wih, "", 1 );
								ELSE
									(* wih := QTils.OpenQTMovieWindowWithMovie( theMovie, "", vues_avec_controleur ); *)
									err := QTils.DisplayMovieInQTMovieWindowH( theMovie, CDp^.wih, "", vues_avec_controleur );
							END;
							wih := CDp^.wih;
					END;
%IF NOTOUTCOMMENTED %THEN
					IF ( (CDp^.channel = 5) OR (CDp^.channel = 6) )
						THEN
							wih := QTils.OpenQTMovieInWindow( CDp^.fName, 1 );
						ELSE
							wih := QTils.OpenQTMovieInWindow( CDp^.fName, vues_avec_controleur );
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
							PostMessage( CDp^.fName, QTils.lastSSLogMsg^ );
						ELSE
							DeleteFile(CDp^.fName);
							QTils.LogMsgEx( "%s importé et supprimé", CDp^.fName );
					END;
				ELSE
					PostMessage( CDp^.fName, "Echec de création/ouverture" );
			END;
%ELSE (* NOT USECHANNELVIEWIMPORTFILES *)
			qi2mString := NIL;
			Concat( CDp^.source, ".mov", source );
			QTils.ssprintf( qi2mString,
					'<?xml version="1.0"?>\n<?quicktime type="video/x-qt-img2mov"?>\n<import autoSave=True autoSaveName=%s >\n',
					(* on active l'autoSave sur l'importation pour une création automatique du fichier cache *)
					CDp^.fName );
			IF LENGTH(assocDataFileName) = 0
				THEN
					QTils.ssprintfAppend( qi2mString, '\t<description txt="UTC timeZone=%g, DST=%hd" />\n',
						CDp^.description.timeZone, VAL(Int16,CDp^.description.DST) );
				ELSE
					QTils.ssprintfAppend( qi2mString, '\t<description txt="UTC timeZone=%g, DST=%hd, assoc.data:%s" />\n',
						CDp^.description.timeZone, VAL(Int16,CDp^.description.DST), assocDataFileName );
			END;
			(* début de la définition d'une séquence: *)
			IF fullMovieIsSplit
				THEN
					QTils.ssprintfAppend( qi2mString, '\t<sequence src="%s"', source );
				ELSE
					QTils.ssprintfAppend( qi2mString, '\t<sequence src="%s" channel=%s', source, chanNum );
			END;
			IF ( (CDp^.channel <> 5) AND (CDp^.channel <> 6) )
				THEN
					(* on cache la piste TimeCode des vues des 4 caméras (hidetc=True)
					 * et on force l'interprétation comme 'movie' *)
					QTils.ssprintfAppend( qi2mString, '%s', ' hidetc=True timepad=False asmovie=True' );
					IF CDp^.description.flipLeftRight AND ((CDp^.channel = CDp^.description.channels.left)
						OR (CDp^.channel = CDp^.description.channels.right))
						THEN
							(* vues à inverser horizontalement *)
							QTils.ssprintfAppend( qi2mString, '%s', ' hflip=True' );
						ELSE
							QTils.ssprintfAppend( qi2mString, '%s', ' hflip=False' );
					END;
				ELSE
					(* CDp^.channel no. 5 = la piste TimeCode; no. 6 = la piste texte avec l'horodatage des trames *)
					QTils.ssprintfAppend( qi2mString, '%s', ' hidetc=False timepad=False asmovie=True newchapter=False' );
			END;
			QTils.ssprintfAppend( qi2mString, '%s\n%s\n', ' />', '</import>' );
			IF qi2mString <> NIL
				THEN
					err := QTils.MemoryDataRefFromString( qi2mString^, CDp^.fName, memRef );
					IF err = noErr
						THEN
							(* le MemoryDataRef a été créé, on l'ouvre maintenant, d'abord dans un .mov *)
							err := QTils.OpenMovieFromMemoryDataRefWithQTMovieWindowH( theMovie, memRef, FOUR_CHAR_CODE('QI2M'), CDp^.wih );
							IF err = noErr
								THEN
									IF PrepareChannelCacheMovie( theMovie, CDp^.description.channels, CDp^.channel )
										THEN
											QTils.SaveMovieAsRefMov( CDp^.fName, theMovie );
									END;
									IF ( (CDp^.channel = 5) OR (CDp^.channel = 6) )
										THEN
											(* wih := QTils.OpenQTMovieWindowWithMovie( theMovie, "", 1 ); *)
											QTils.DisplayMovieInQTMovieWindowH( theMovie, CDp^.wih, "", 1 );
										ELSE
											(* wih := QTils.OpenQTMovieWindowWithMovie( theMovie, "", vues_avec_controleur ); *)
											QTils.DisplayMovieInQTMovieWindowH( theMovie, CDp^.wih, "", vues_avec_controleur );
									END;
									wih := CDp^.wih;
							END;
							IF ( wih = NULL_QTMovieWindowH )
								THEN
									IF QTils.MacErrorString( QTils.LastQTError(), errString, errComment ) <> 0
										THEN
											QTils.LogMsgEx( "Echec d'importation: %s (%s)\n%s\n", errString, errComment, qi2mString^ );
										ELSE
											QTils.LogMsgEx( "Echec d'importation %d\n%s\n", QTils.LastQTError(), qi2mString^ );
									END;
									PostMessage( CDp^.fName, QTils.lastSSLogMsg^ );
									QTils.DisposeMemoryDataRef(memRef);
								ELSE
									QTils.LogMsgEx( "%s importé de mémoire", CDp^.fName );
							END;
						ELSE
							QTils.LogMsgEx( "Echec de création de MemoryDataRef pour canal %s: %d", chanNum, err );
							PostMessage( CDp^.fName, QTils.lastSSLogMsg^ );
					END;
					QTils.free(qi2mString);
				ELSE
					QTils.LogMsgEx( "Echec de création de %s en mémoire", CDp^.fName );
					PostMessage( CDp^.fName, QTils.lastSSLogMsg^ );
			END;
%END
	END;
	IF QTils.QTMovieWindowH_isOpen(wih)
		THEN
		IF ( (CDp^.scale <> 1.0) & (CDp^.scale > 0.0) )
			THEN
				QTils.QTMovieWindowSetGeometry( wih, NIL, NIL, CDp^.scale, 0 );
		END;
		QTils.SetMoviePlayHints( wih^^.theMovie, hintsPlayingEveryFrame, 1 );
	END;

	(* CDp^.wih := wih; *)
%IF CCV_IN_BACKGROUND %THEN
	lock();
	CDp^.done := TRUE;
	unlock();
	IF CCV_IN_BACKGROUND
		THEN
			QTils.LogMsgEx( "CCDWorker finished with '%s'#%d", CDp^.source, CDp^.channel );
			(*unlock();*)
			(* windows created on a background thread need their own message pump ... *)
			WHILE ( (QTils.QTMovieWindowH_Check(wih)) AND (NOT quitRequest) ) DO
				QTils.PumpMessages(1);
			END;
			QTils.PumpMessages(0);
			CloseQT();
	END;
%END
	RETURN VAL(CARDINAL, (wih <> NULL_QTMovieWindowH) );
END CCDWorker;

PROCEDURE CreateChannelView( VAR CD : ChannelDescription ) : QTMovieWindowH;
VAR
BEGIN
	(*CD.wih := QTils.InitQTMovieWindowH( 360, 288 );*)
	CD.wih := NULL_QTMovieWindowH;
%IF CCV_IN_BACKGROUND %THEN
	CD.done := FALSE;
	IF CCV_IN_BACKGROUND
		THEN
			IF CreateThread( CD.thread, CCDWorker, ADR(CD), 0, TRUE )
				THEN
					RETURN NULL_QTMovieWindowH;
				ELSE
					CCDWorker( ADR(CD) );
					RETURN CD.wih;
			END;
		ELSE
			CCDWorker( ADR(CD) );
			RETURN CD.wih;
	END;
%ELSE
	CCDWorker( ADR(CD) );
	RETURN CD.wih;
%END
END CreateChannelView;

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

PROCEDURE DisplayVideo( VAR URL : URLString; VAR description : VODDescription ) : ErrCode;
VAR
	fName : URLString;
	w : CARDINAL;
	err : ErrCode;
	isVODFile : BOOLEAN;

%IF CCV_IN_BACKGROUND %THEN
	PROCEDURE CheckNotDone(w : CARDINAL) : BOOLEAN;
	VAR
		done : BOOLEAN;
	BEGIN
		IF CCV_IN_BACKGROUND
			THEN
				EnterCriticalSection(channelDesc[w].mutex);
				done := channelDesc[w].done;
				LeaveCriticalSection(channelDesc[w].mutex);
			ELSE
				done := channelDesc[w].done;
		END;
		IF done
			THEN
				QTils.LogMsgEx( "Channel #%d is ready", w );
		END;
		RETURN NOT done;
	END CheckNotDone;
%END

BEGIN

	err := OpenVideo( URL, description, ADR(isVODFile) );
	IF err <> noErr
		THEN
			RETURN err;
	END;
	Assign( fullMovieWMH^^.theURL^, fName );

	(* maintenant on peut tenter d'ouvrir les 5 fenêtres *)
	(* vue vers l'avant *)
	FOR w := 0 TO maxQTWM DO
		channelDesc[w].source := URL;
		channelDesc[w].description := description;
		channelDesc[w].scale := description.scale;
	END;

(*
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
*)
	(* vue vers l'avant *)
	channelDesc[fwWin].channel := description.channels.forward;
	channelDesc[fwWin].chName := "forward";
	CreateChannelView( channelDesc[fwWin] );
	(* vue conducteur *)
	channelDesc[pilotWin].channel := description.channels.pilot;
	channelDesc[pilotWin].chName := "pilot";
	CreateChannelView( channelDesc[pilotWin] );
	(* vue latérale-gauche *)
	channelDesc[leftWin].channel := description.channels.left;
	channelDesc[leftWin].chName := "left";
	CreateChannelView( channelDesc[leftWin] );
	(* vue latérale-droite *)
	channelDesc[rightWin].channel := description.channels.right;
	channelDesc[rightWin].chName := "right";
	CreateChannelView( channelDesc[rightWin] );
	(* la piste TimeCode qui donne le temps pour les 4 vues *)
	channelDesc[tcWin].channel := TimeCodeChannel;
	channelDesc[tcWin].chName := "TC";
	channelDesc[tcWin].scale := 1.0;
	CreateChannelView( channelDesc[tcWin] );

%IF CCV_IN_BACKGROUND %THEN
	IF CCV_IN_BACKGROUND
		THEN
			QTils.LogMsg("Waiting for windows to open");
			(* loop until all background threads are ready to roll on *)
(*
			FOR w := 0 TO maxQTWM DO
				EnterCriticalSection( channelDesc[w].mutex );
				LeaveCriticalSection( channelDesc[w].mutex );
				QTils.LogMsgEx( "Channel #%d is ready", w );
				QTils.PumpMessages(0);
			END;
*)
			WHILE CheckNotDone(fwWin) OR CheckNotDone(pilotWin) OR CheckNotDone(leftWin)
				OR CheckNotDone(rightWin) OR CheckNotDone(tcWin) DO
					QTils.PumpMessages(1);
			END;
			QTils.LogMsg("Done!");
	END;
%END

	qtwmH[fwWin] := channelDesc[fwWin].wih;
	qtwmH[pilotWin] := channelDesc[pilotWin].wih;
	qtwmH[leftWin] := channelDesc[leftWin].wih;
	qtwmH[rightWin] := channelDesc[rightWin].wih;
	qtwmH[tcWin] := channelDesc[tcWin].wih;

	(* enregistrement des fonctions de gestion d'actions et MAJ de numQTMW *)
	FOR w := 0 TO maxQTWM	DO
		register_window(qtwmH, w);
	END;
%IF %NOT USE_TIMEDCALLBACK %THEN
	QTils.register_MCAction( qtwmH[fwWin], MCAction.AnyAction, PumpSubscriptions );
%END

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
END DisplayVideo;

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
	QTils.CloseMovie(fullMovie);
	fullMovie := NIL;
	fullMovieWMH := NULL_QTMovieWindowH;
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
			err := DisplayVideo( baseFileName, baseDescription );
			IF ( (err = noErr) AND (NOT complete) AND (t >= 0.0) )
				THEN
					SetTimes( t, NIL, 0 );
					QTils.LogMsgEx( "QTVODm2::ResetVideo: temps restoré: %gs", t );
			END;
	END;
	RETURN err;
END ResetVideo;

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

(* ==================================== BEGIN ==================================== *)
BEGIN

	numQTWM := 0;
	MetaDataDisplayStrPtr := NIL;
	FOR idx := 0 TO maxQTWM	DO
		qtwmH[idx] := NULL_QTMovieWindowH;
	END;
	sServeur := sock_nulle;
	sqrt2 := sqrt(2.0);
	ULCorner.horizontal := 0;
	ULCorner.vertical := 0;
	fullMovieChanged := FALSE;
	theTimeInterVal.timeA := -1.0;
	theTimeInterVal.dt := 0.0;
	InitHRTime();
	splitCamTrack.forward := -1;
	splitCamTrack.pilot := -1;
	splitCamTrack.left := -1;
	splitCamTrack.right := -1;
	currentTimeSubscription.sendInterval := 0.0;
	currentTimeSubscription.lastSentTime := 0.0;
	currentTimeSubscription.lastMovieTime := -1.0;
	currentTimeSubscription.lastForcedMovieTime := -1.0;
	currentTimeSubscription.absolute := FALSE;
	callbackRegister := NIL;
%IF CCV_IN_BACKGROUND %THEN
	CCV_IN_BACKGROUND := FALSE;
%END
	FOR wIdx := 0 TO maxQTWM DO
		channelDesc[wIdx].wih := NULL_QTMovieWindowH;
		channelDesc[wIdx].fName := "";
%IF CCV_IN_BACKGROUND %THEN
		channelDesc[wIdx].thread := NIL;
		InitializeCriticalSectionAndSpinCount( channelDesc[wIdx].mutex, 4000 );
%END
	END;

FINALLY

%IF CCV_IN_BACKGROUND %THEN
	FOR wIdx := 0 TO maxQTWM DO
		DeleteCriticalSection(channelDesc[wIdx].mutex);
		IF channelDesc[wIdx].thread <> NIL
			THEN
				KillThread( channelDesc[wIdx].thread, 0 );
		END;
	END;
%END

	CloseCommClient(sServeur);

END QTVODlibext.
