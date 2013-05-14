IMPLEMENTATION MODULE QTVODcomm;

<*/VALIDVERSION:CHAUSSETTE2,DEBUG,COMMTIMING,USE_POSIX_DETACHPROCESS,USE_SHELLEXECUTE*>

FROM SYSTEM IMPORT
	CAST;

FROM Strings IMPORT
	Concat, Append, Assign, Capitalize, FindPrev, Delete;

FROM WINUSER IMPORT
	SW_SHOWDEFAULT, SW_SHOWNORMAL;
FROM WIN32 IMPORT
	CreateProcess, CREATE_NEW_PROCESS_GROUP, DETACHED_PROCESS,
	CREATE_DEFAULT_ERROR_MODE, CloseHandle,
	PROCESS_INFORMATION, STARTUPINFO, STARTF_USESHOWWINDOW, GetLastError, HANDLE;
FROM WINX IMPORT
	NULL_HANDLE, NIL_STR, NIL_SECURITY_ATTRIBUTES;
%IF USE_SHELLEXECUTE %THEN
	FROM SHELLAPI IMPORT
		ShellExecute;
%END

FROM WINSOCK2 IMPORT
	WSAEWOULDBLOCK;

FROM WIN32 IMPORT
	LARGE_INTEGER, QueryPerformanceFrequency, QueryPerformanceCounter;

FROM ElapsedTime IMPORT
	StartTimeEx, GetTimeEx;

%IF CHAUSSETTE2 %THEN
FROM Chaussette2 IMPORT
%ELSE
FROM Chaussette IMPORT
%END
	SOCK, sock_nulle, BasicSendNetMessage, BasicReceiveNetMessage,
	InitIP, FinIP, EtabliClientUDPouTCP, EtabliServeurUDPouTCP, FermeClient, FermeServeur,
	AttenteConnexionDunClient, ConnexionAuServeur, FermeConnexionAuServeur, errSock;

%IF USE_POSIX_DETACHPROCESS %THEN
	FROM POSIXm2 IMPORT *;
%END
FROM QTilsM2 IMPORT *;

CONST

	NULL_VODDescriptionPtr = CAST(VODDescriptionPtr,0);
	CREATE_BREAKAWAY_FROM_JOB = 001000000h;
	CREATE_PRESERVE_CODE_AUTHZ_LEVEL = 002000000h;

VAR

	SendErrors, ReceiveErrors : CARDINAL;
	QTVODm2Process : HANDLE;
	HPCcalibrator : Real64;
	HPCval : LARGE_INTEGER;

PROCEDURE InitCommClient(
	VAR clnt : SOCK; IP : ARRAY OF CHAR; portS, portC : UInt16;
	timeOutms : INTEGER )  : BOOLEAN;
VAR
	fatal : BOOLEAN;
	name : ARRAY[0..8] OF CHAR;
	errStr : ARRAY[0..1023] OF CHAR;
	err : INTEGER;
BEGIN
	IF ( InitIP() AND EtabliClientUDPouTCP(clnt, portC, TRUE) )
		THEN
			QTils.LogMsgEx( "Net sock %hu ouvert - client", portC );
			name := "";
%IF CHAUSSETTE2 %THEN
			IF ConnexionAuServeur( clnt, portS, name, IP, timeOutms, fatal )
%ELSE
			IF ConnexionAuServeur( clnt, portS, name, IP, fatal )
%END
				THEN
					QTils.LogMsgEx( 'connecté au serveur "%s:%hu"', IP, portS );
					RETURN TRUE;
				ELSE
					err := errSock;
					MSWinErrorString( err, errStr );
					QTils.LogMsgEx( 'pas de serveur "%s:%hu" (err=%d "%s")', IP, portS, err, errStr );
					FermeClient(clnt);
					clnt := sock_nulle;
(*
					IF err = WSAEWOULDBLOCK
						THEN
							QTils.LogMsgEx( 'Nouvel essai de connexion à %s:%hu\n', IP, portS );
							RETURN InitCommClient( clnt, IP, portS, portC, timeOutms );
						ELSE
							RETURN FALSE;
					END;
*)
				RETURN FALSE;
			END;
			RETURN TRUE;
		ELSE
			MSWinErrorString( errSock, errStr );
			QTils.LogMsgEx( "Net sock %hu échec d'ouverture - client (err=%d '%s')", portC, errSock, errStr );
			RETURN FALSE;
	END;
END InitCommClient;

PROCEDURE InitCommServer( VAR srv : SOCK; port : UInt16 ) : BOOLEAN;
VAR
	errStr : ARRAY[0..1023] OF CHAR;
BEGIN
	IF ( InitIP() AND EtabliServeurUDPouTCP(srv, port, TRUE) )
		THEN
			QTils.LogMsgEx( "Net sock %hu ouvert - serveur", port );
			RETURN TRUE;
		ELSE
			MSWinErrorString( errSock, errStr );
			QTils.LogMsgEx( "Net sock %hu échec d'ouverture - serveur (err=%d '%s')", port, errSock, errStr );
			RETURN FALSE;
	END;
END InitCommServer;

PROCEDURE ServerCheckForClient( srv : SOCK; VAR clnt : SOCK;
																timeOutms : INTEGER; block : BOOLEAN ) : BOOLEAN;
BEGIN
	IF ( (srv <> sock_nulle) AND (clnt = sock_nulle) )
		THEN
%IF CHAUSSETTE2 %THEN
			IF AttenteConnexionDunClient( srv, timeOutms, block, clnt )
%ELSE
			IF AttenteConnexionDunClient( srv, block, clnt )
%END
				THEN
					QTils.LogMsg( "un client s'est connecté" );
					RETURN TRUE;
			END;
	END;
	RETURN FALSE;
END ServerCheckForClient;

PROCEDURE CloseCommClient( VAR clnt : SOCK );
BEGIN
	IF ( clnt <> sock_nulle )
		THEN
			FermeConnexionAuServeur(clnt);
			FermeClient(clnt);
			clnt := sock_nulle;
	END;
END CloseCommClient;

(*
	l'ordre de fermeture: le serveur doit s'executer en premier, mais il ferme
	ses sockets "client" d'abord.
	Si cette ordre n'est pas respectée, il peut y avoir des difficultés de
	reconnexion sur les mêmes port pendant un certain temps
*)
PROCEDURE CloseCommServeur( VAR srv, clnt : SOCK );
BEGIN
	IF ( clnt <> sock_nulle )
		THEN
			FermeServeur(clnt);
			clnt := sock_nulle;
	END;
	IF ( srv <> sock_nulle )
		THEN
			FermeServeur(srv);
			srv := sock_nulle;
	END;
END CloseCommServeur;

PROCEDURE SendMessageToNet( ss : SOCK; VAR msg : NetMessage; timeOutms : INTEGER; block : BOOLEAN;
														caller : ARRAY OF CHAR ) : BOOLEAN;
VAR
	ret : BOOLEAN;
BEGIN
	errSock := 0;
%IF COMMTIMING %THEN
	IF QueryPerformanceCounter(HPCval)
		THEN
			msg.sentTime := VAL(Real64,HPCval) * HPCcalibrator;
		ELSE
			msg.sentTime := -1.0;
	END;
%END
%IF CHAUSSETTE2 %THEN
		ret := BasicSendNetMessage( ss, msg, SIZE(msg), timeOutms, block );
%ELSE
		ret := BasicSendNetMessage( ss, msg, SIZE(msg), block );
%END
		IF ret
			THEN
				NetMessageToLogMsg( caller, "(envoyé)", msg );
			ELSE
				NetMessageToLogMsg( caller, "(échec d'envoi)", msg );
				IF ( errSock <> 0 )
					THEN
						SendErrors := SendErrors + 1;
						IF ( HandleSendErrors <> CommErrorHandler_nulle )
							THEN
								HandleSendErrors(SendErrors);
						END;
				END;
		END;
		RETURN ret;
END SendMessageToNet;

PROCEDURE ReceiveMessageFromNet(ss : SOCK; VAR msg : NetMessage; timeOutms : INTEGER; block : BOOLEAN;
	caller : ARRAY OF CHAR ) : BOOLEAN;
VAR
	ret : BOOLEAN;
BEGIN
%IF CHAUSSETTE2 %THEN
		ret := BasicReceiveNetMessage(ss, msg, SIZE(msg), timeOutms, block);
%ELSE
		ret := BasicReceiveNetMessage(ss, msg, SIZE(msg), block);
%END
	IF ret
		THEN
%IF COMMTIMING %THEN
			IF QueryPerformanceCounter(HPCval)
				THEN
					msg.recdTime := VAL(Real64,HPCval) * HPCcalibrator;
				ELSE
					msg.recdTime := -1.0;
			END;
%END
	END;
	RETURN ret;
END ReceiveMessageFromNet;

PROCEDURE NetMessageToString(VAR msg : NetMessage) : String256;
VAR
	str, timing : String256;
BEGIN
	CASE msg.flags.type OF
		qtvod_Open :
			str := "Open";
		| qtvod_Start :
			str := "Start";
		| qtvod_Stop :
			str := "Stop";
		| qtvod_Close :
			str := "Close";
		| qtvod_Reset :
			str := "Reset";
		| qtvod_Quit :
			str := "Quit";
		| qtvod_GotoTime :
			str := "GotoTime";
		| qtvod_GetTime :
			str := "GetTime";
		| qtvod_GetStartTime :
			str := "GetStartTime";
		| qtvod_GetDuration :
			str := "GetDuration";
		| qtvod_OK :
			str := "OK";
		| qtvod_Err :
			str := "Error";
		| qtvod_CurrentTime :
			str := "CurrentTime";
		| qtvod_StartTime :
			str := "StartTime";
		| qtvod_Duration :
			str := "Duration";
		| qtvod_NewChapter :
			str := "NewChapter";
		| qtvod_GetChapter :
			str := "GetChapter";
		| qtvod_Chapter :
			str := "Chapter";
		| qtvod_MarkIntervalTime :
			str := "MarkIntervalTime";
		| qtvod_GetLastInterval :
			str := "GetLastInterval";
		| qtvod_LastInterval :
			str := "LastInterval";
		| qtvod_MovieFinished :
			str := "MovieFinished";
		| qtvod_GetTimeSubscription :
			str := "GetTimeSubscription";
	ELSE
		str := "<??>";
	END;
	CASE msg.flags.class OF
		qtvod_Command:
			Append( " (commande)", str );
		| qtvod_Notification:
			Append( " (notification)", str );
		| qtvod_Confirmation:
			Append( " (confirmation)", str );
		| qtvod_Subscription:
			Append( " (subscription)", str );
		ELSE
			(* noop *)
	END;
%IF COMMTIMING %THEN
	IF (msg.sentTime >= 0.0) AND (msg.recdTime >= 0.0)
		THEN
			QTils.sprintf( timing, " [S@%gs R@%gs dt=%gs]", msg.sentTime, msg.recdTime, msg.recdTime - msg.sentTime );
			Append( timing, str );
	END;
%END
	RETURN str;
END NetMessageToString;

PROCEDURE NetMessageToLogMsg( title, caption : ARRAY OF CHAR; VAR msg : NetMessage );
VAR
	tType : ARRAY[0..15] OF CHAR;
BEGIN
	IF (msg.data.boolean)
		THEN
			tType := "(abs)";
		ELSE
			tType := "(rel)";
	END;
	msg.data.URN[ SIZE(msg.data.URN)-1 ] := CHR(0);
	CASE msg.flags.type OF
		qtvod_Open :
			WITH msg.data.description DO
				IF ( LENGTH(caption) > 0 )
					THEN
						QTils.LogMsgEx( '%s %s: %s fichier "%s", freq=%gHz scale=%g Tzone=%g DST=%hd flipGaucheDroite=%hd canal FW=%d Dr=%d L=%d R=%d',
							title, caption, NetMessageToString(msg),
							msg.data.URN, frequency, scale,
							timeZone, VAL(Int16, DST), VAL(Int16,flipLeftRight),
							channels.forward, channels.pilot,
							channels.left, channels.right
						);
					ELSE
						QTils.LogMsgEx( '%s: %s fichier "%s", freq=%gHz scale=%g Tzone=%g DST=%hd flipGaucheDroite=%hd canal FW=%d Dr=%d L=%d R=%d',
							title, NetMessageToString(msg),
							msg.data.URN, frequency, scale,
							timeZone, VAL(Int16, DST), VAL(Int16,flipLeftRight),
							channels.forward, channels.pilot,
							channels.left, channels.right
						);
				END;
			END;
		| qtvod_Start,
		qtvod_Stop,
		qtvod_Close,
		qtvod_Reset,
		qtvod_Quit,
		qtvod_GetTime,
		qtvod_GetStartTime,
		qtvod_GetDuration,
		qtvod_GetLastInterval,
		qtvod_OK :
			IF ( LENGTH(caption) > 0 )
				THEN
					QTils.LogMsgEx( '%s %s: %s', title, caption, NetMessageToString(msg) );
				ELSE
					QTils.LogMsgEx( '%s: %s', title, NetMessageToString(msg) );
			END;
		| qtvod_MovieFinished :
			IF ( LENGTH(caption) > 0 )
				THEN
					QTils.LogMsgEx( '%s %s: %s "%s" canal #%d', title, caption, NetMessageToString(msg), msg.data.URN, msg.data.iVal1 );
				ELSE
					QTils.LogMsgEx( '%s: %s "%s" canal #%d', title, NetMessageToString(msg), msg.data.URN, msg.data.iVal1 );
			END;
		| qtvod_Err :
			IF ( LENGTH(caption) > 0 )
				THEN
					QTils.LogMsgEx( '%s %s: %s "%s" erreur=%d', title, caption, NetMessageToString(msg), msg.data.URN, msg.data.error );
				ELSE
					QTils.LogMsgEx( '%s: %s "%s" erreur=%d', title, NetMessageToString(msg), msg.data.URN, msg.data.error );
			END;
		| qtvod_GotoTime,
		qtvod_CurrentTime,
		qtvod_StartTime,
		qtvod_Duration,
		qtvod_MarkIntervalTime,
		qtvod_LastInterval :
			IF ( LENGTH(caption) > 0 )
				THEN
					QTils.LogMsgEx( '%s %s: %s t=%gs %s', title, caption, NetMessageToString(msg), msg.data.val1, tType );
				ELSE
					QTils.LogMsgEx( '%s: %s t=%gs %s', title, NetMessageToString(msg), msg.data.val1, tType );
			END;
		| qtvod_GetTimeSubscription :
			IF ( LENGTH(caption) > 0 )
				THEN
					QTils.LogMsgEx( '%s %s: %s interval=%gs %s', title, caption, NetMessageToString(msg), msg.data.val1, tType );
				ELSE
					QTils.LogMsgEx( '%s: %s interval=%gs %s', title, NetMessageToString(msg), msg.data.val1, tType );
			END;
		| qtvod_NewChapter,
		qtvod_GetChapter,
		qtvod_Chapter :
			IF ( LENGTH(caption) > 0 )
				THEN
					QTils.LogMsgEx( '%s %s: %s title="%s" #%d start=%gs d=%gs %s',
						title, caption, NetMessageToString(msg), msg.data.URN, msg.data.iVal1, msg.data.val1, msg.data.val2, tType );
				ELSE
					QTils.LogMsgEx( '%s: %s title="%s" #%d start=%gs d=%gs %s',
						title, NetMessageToString(msg), msg.data.URN, msg.data.iVal1, msg.data.val1, msg.data.val2, tType );
			END;
	ELSE
		QTils.LogMsgEx( '%s: <??>', title );
	END;
END NetMessageToLogMsg;

(* =========================================================================================================== *)

(* Procedures to construct commands sent by clients to QTVODm2 *)

PROCEDURE msgOpenFile(VAR msg : NetMessage; URL : URLString; VAR descr : VODDescription);
BEGIN
	msg.flags.type := qtvod_Open;
	msg.flags.class := qtvod_Command;
	msg.data.URN := URL;
	msg.data.description := descr;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgOpenFile;

PROCEDURE msgPlayMovie( VAR msg : NetMessage );
BEGIN
	msg.flags.type := qtvod_Start;
	msg.flags.class := qtvod_Command;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgPlayMovie;

PROCEDURE msgStopMovie( VAR msg : NetMessage );
BEGIN
	msg.flags.type := qtvod_Stop;
	msg.flags.class := qtvod_Command;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgStopMovie;

PROCEDURE msgCloseMovie( VAR msg : NetMessage );
BEGIN
	msg.flags.type := qtvod_Close;
	msg.flags.class := qtvod_Command;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgCloseMovie;

PROCEDURE msgResetQTVOD( VAR msg : NetMessage; complete : BOOLEAN );
BEGIN
	msg.flags.type := qtvod_Reset;
	msg.flags.class := qtvod_Command;
	msg.data.boolean := complete;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgResetQTVOD;


PROCEDURE msgQuitQTVOD( VAR msg : NetMessage );
BEGIN
	msg.flags.type := qtvod_Quit;
	msg.flags.class := qtvod_Command;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgQuitQTVOD;

PROCEDURE msgGotoTime(VAR msg : NetMessage; t : Real64; absolute : BOOLEAN);
BEGIN
	msg.flags.type := qtvod_GotoTime;
	msg.flags.class := qtvod_Command;
	msg.data.val1 := t;
	msg.data.boolean := absolute;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgGotoTime;

PROCEDURE msgGetTime( VAR msg : NetMessage; absolute : BOOLEAN );
BEGIN
	msg.flags.type := qtvod_GetTime;
	msg.flags.class := qtvod_Command;
	msg.data.boolean := absolute;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgGetTime;

PROCEDURE msgGetTimeSubscription( VAR msg : NetMessage; interval : Real64; absolute : BOOLEAN );
BEGIN
	msg.flags.type := qtvod_GetTimeSubscription;
	msg.flags.class := qtvod_Command;
	msg.data.val1 := interval;
	msg.data.boolean := absolute;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgGetTimeSubscription;

PROCEDURE msgGetStartTime( VAR msg : NetMessage );
BEGIN
	msg.flags.type := qtvod_GetStartTime;
	msg.flags.class := qtvod_Command;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgGetStartTime;


PROCEDURE msgGetDuration(VAR msg : NetMessage);
BEGIN
	msg.flags.type := qtvod_GetDuration;
	msg.flags.class := qtvod_Command;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgGetDuration;

PROCEDURE msgGetChapter(VAR msg : NetMessage; idx : Int32);
BEGIN
	msg.flags.type := qtvod_GetChapter;
	msg.flags.class := qtvod_Command;
	IF idx < 0
		THEN
			msg.data.URN := "<list all>";
		ELSE
			msg.data.URN := "<in retrieval>";
	END;
	msg.data.iVal1 := idx;
	msg.data.val1 := 0.0/0.0;
	msg.data.val2 := 0.0/0.0;
	msg.data.boolean := FALSE;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgGetChapter;

PROCEDURE msgNewChapter(VAR msg : NetMessage; title : URLString; startTime, duration : Real64; absolute : BOOLEAN);
BEGIN
	msg.flags.type := qtvod_NewChapter;
	msg.flags.class := qtvod_Command;
	msg.data.URN := title;
	msg.data.iVal1 := -999;
	msg.data.val1 := startTime;
	msg.data.val2 := duration;
	msg.data.boolean := absolute;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgNewChapter;

PROCEDURE msgMarkIntervalTime(VAR msg : NetMessage; reset : BOOLEAN);
BEGIN
	msg.flags.type := qtvod_MarkIntervalTime;
	msg.flags.class := qtvod_Command;
	msg.data.boolean := reset;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgMarkIntervalTime;

PROCEDURE msgGetLastInterval(VAR msg : NetMessage);
BEGIN
	msg.flags.type := qtvod_GetLastInterval;
	msg.flags.class := qtvod_Command;
%IF COMMTIMING %THEN
	msg.sentTime := -1.0;
%END
END msgGetLastInterval;

(* =========================================================================================================== *)
(* Procedures for QTVODm2 to construct reply messages to send back in response to commands from the client: *)

PROCEDURE replyCurrentTime( VAR reply : NetMessage; class : NetMessageClass; wih : QTMovieWindowH; absolute : BOOLEAN ) : BOOLEAN;
VAR
	t : Real64;
BEGIN
%IF COMMTIMING %THEN
	reply.sentTime := -1.0;
%END
	IF QTils.QTMovieWindowGetTime(wih, t, VAL(Int32,absolute)) = noErr
		THEN
			reply.flags.type := qtvod_CurrentTime;
			reply.flags.class := class;
			reply.data.val1 := t;
			IF absolute
				THEN
					reply.data.val2 := wih^^.info^.TCframeRate;
				ELSE
					reply.data.val2 := wih^^.info^.frameRate;
			END;
			reply.data.boolean := absolute;
			RETURN TRUE;
		ELSE
			RETURN FALSE;
	END;
END replyCurrentTime;

PROCEDURE replyStartTime( VAR reply : NetMessage; class : NetMessageClass; wih : QTMovieWindowH ) : BOOLEAN;
BEGIN
%IF COMMTIMING %THEN
	reply.sentTime := -1.0;
%END
	IF ( QTils.QTMovieWindowH_isOpen(wih) )
		THEN
			reply.flags.type := qtvod_StartTime;
			reply.flags.class := class;
			reply.data.val1 := wih^^.info^.startTime;
			reply.data.val2 := wih^^.info^.frameRate;
			(* relative startTime is always 0 so querying it makes no sense... *)
			reply.data.boolean := TRUE;
			RETURN TRUE;
		ELSE
			RETURN FALSE;
	END;
END replyStartTime;

PROCEDURE replyDuration( VAR reply : NetMessage; class : NetMessageClass; wih : QTMovieWindowH ) : BOOLEAN;
BEGIN
%IF COMMTIMING %THEN
	reply.sentTime := -1.0;
%END
	IF ( QTils.QTMovieWindowH_isOpen(wih) )
		THEN
			reply.flags.type := qtvod_Duration;
			reply.flags.class := class;
			reply.data.val1 := wih^^.info^.duration;
			reply.data.val2 := wih^^.info^.frameRate;
			(* duration is always a relative time (i.e. w.r.t. the movie onset) *)
			reply.data.boolean := FALSE;
			RETURN TRUE;
		ELSE
			RETURN FALSE;
	END;
END replyDuration;

PROCEDURE replyChapter( VAR reply : NetMessage; class : NetMessageClass;
												title : URLString; idx : Int32; startTime, duration : Real64 ) : BOOLEAN;
BEGIN
%IF COMMTIMING %THEN
	reply.sentTime := -1.0;
%END
	IF (LENGTH(title) > 0) AND (startTime >= 0.0) AND (duration >= 0.0)
		THEN
			reply.flags.type := qtvod_Chapter;
			reply.flags.class := class;
			reply.data.URN := title;
			reply.data.iVal1 := idx;
			reply.data.val1 := startTime;
			reply.data.val2 := duration;
			(* duration is always a relative time (i.e. w.r.t. the movie onset) *)
			reply.data.boolean := FALSE;
			RETURN TRUE;
		ELSE
			RETURN FALSE;
	END;
END replyChapter;

PROCEDURE replyLastInterval( VAR reply : NetMessage; class : NetMessageClass ) : BOOLEAN;
BEGIN
%IF COMMTIMING %THEN
	reply.sentTime := -1.0;
%END
	reply.flags.type := qtvod_LastInterval;
	reply.flags.class := class;
	reply.data.val1 := theTimeInterVal.dt;
	(* an interval is always a relative time *)
	reply.data.boolean := FALSE;
	RETURN TRUE;
END replyLastInterval;

(* =========================================================================================================== *)
(* transceiving functions *)

PROCEDURE SendNetCommandOrNotification( ss : SOCK; type : NetMessageType; class : NetMessageClass );
VAR
	msg : NetMessage;
BEGIN
	IF ss <> sock_nulle
		THEN
			msg.flags.type := type;
			msg.flags.class := class;
			SendMessageToNet( ss, msg, SENDTIMEOUT, FALSE, "SendNetCommandOrNotification" );
	END;
END SendNetCommandOrNotification;

PROCEDURE SendNetErrorNotification( ss : SOCK; txt : ARRAY OF CHAR; err : ErrCode );
VAR
	msg : NetMessage;
BEGIN
	IF ss <> sock_nulle
		THEN
			msg.flags.type := qtvod_Err;
			msg.flags.class := qtvod_Notification;
			IF LENGTH(txt) > 0
				THEN
					msg.data.URN := txt;
			END;
			msg.data.error := err;
			SendMessageToNet( ss, msg, SENDTIMEOUT, FALSE, "SendNetErrorNotification" );
	END;
END SendNetErrorNotification;

(*
 * RJVB 20110215: on n'utilise plus WinExec pour lancer QTVODm2, car à priori
 * cette fonction est obsolète (compatibilité avec MSWin16...). CreateProcess est plus
 * complexe à utiliser mais ne semble pas bloquer autant de temps que WinExec peut le faire
 * (quelques millisecondes contre parfois des dizaines de secondes).
 *)
%IF USE_POSIX_DETACHPROCESS %THEN

PROCEDURE LaunchQTVODm2( path : ARRAY OF CHAR; fileName, extraArgs : URLString;
						descr :VODDescriptionPtr; serverIP : ARRAY OF CHAR;
						VAR mustSendVODDescription : BOOLEAN ) : BOOLEAN;
VAR
	commandline : URLString;
	retB : BOOLEAN;
	timer : CARDINAL32;
	errMsg : ARRAY[0..255] OF CHAR;
	errCode : INTEGER;
BEGIN

	mustSendVODDescription := FALSE;

	Concat(path, "\QTVODm2.exe", commandline);
	IF ( LENGTH(serverIP) > 0 )
		THEN
			Append( " -client ", commandline );
			Append( serverIP, commandline );
	END;
	IF ( LENGTH(extraArgs) > 0 )
		THEN
			Append( " ", commandline );
			Append( extraArgs, commandline );
	END;
	IF ( LENGTH(fileName) > 0 )
		THEN
			IF (descr = NIL)
				THEN
					Append( ' ', commandline );
					IF fileName[0] <> '"'
						THEN
							Append( '"', commandline );
					END;
					Append( fileName, commandline );
					IF fileName[0] <> '"'
						THEN
							Append( '"', commandline );
					END;
				ELSE
					Append( ' -attendVODDescription', commandline );
					mustSendVODDescription := TRUE;
			END;
		ELSE
			IF descr = NIL
				THEN
					(* noop - compilateur peut se tromper sur 'descr <> NIL' *)
				ELSE
					Append( ' -attendVODDescription', commandline );
					mustSendVODDescription := TRUE;
			END;
	END;
	timer := StartTimeEx();
	IF QTVODm2Process <> NULL_HANDLE
		THEN
			CloseHandle(QTVODm2Process);
			QTVODm2Process := NULL_HANDLE;
	END;
	QTVODm2Process := POSIX.DetachProcess(commandline);
	IF QTVODm2Process <> NULL_HANDLE
		THEN
			QTils.LogMsgEx( 'LaunchQTVODm2: DetachProcess("%s") a réussi après %gs (OK)',
				commandline, VAL(Real64,GetTimeEx(timer))/1000.0 );
			retB := TRUE;
		ELSE
			errCode := GetLastError();
			MSWinErrorString( errCode, errMsg );
			QTils.LogMsgEx( 'LaunchQTVODm2: DetachProcess("%s") a échoué après %gs (erreur %d=%s)',
				commandline, VAL(Real64,GetTimeEx(timer))/1000.0,
				errCode, errMsg );
			retB := FALSE;
	END;
	RETURN retB;
END LaunchQTVODm2;

%ELSE

PROCEDURE LaunchQTVODm2( path : ARRAY OF CHAR; fileName, extraArgs : URLString;
						descr :VODDescriptionPtr; serverIP : ARRAY OF CHAR;
						VAR mustSendVODDescription : BOOLEAN ) : BOOLEAN;
VAR
	qtvdPath : URLString;
	args : URLString;
	pInfo : PROCESS_INFORMATION;
	startup : STARTUPINFO;
	retB : BOOLEAN;
	timer : CARDINAL32;
	errMsg : ARRAY[0..255] OF CHAR;
	errCode : INTEGER;
BEGIN

	mustSendVODDescription := FALSE;
	Concat(path, "\QTVODm2.exe", qtvdPath);
%IF %NOT USE_SHELLEXECUTE %THEN
	startup.cb := SIZE(startup);
	startup.lpReserved := NIL;
	startup.lpDesktop := NIL;
	startup.lpTitle := NIL;
	startup.dwFlags := 0 (*STARTF_USESHOWWINDOW*);
	startup.dwX := 0;
	startup.dwY := 0;
	startup.dwXSize := 0;
	startup.dwYSize := 0;
	startup.cbReserved2 := 0;
	startup.lpReserved2 := NIL;
	startup.wShowWindow := 0 (*SW_SHOWDEFAULT*);

	Concat(path, "\QTVODm2.exe", args);
%ELSE
	Assign( "", args );
%END
	IF ( LENGTH(serverIP) > 0 )
		THEN
			Append( " -client ", args );
			Append( serverIP, args );
	END;
	IF ( LENGTH(extraArgs) > 0 )
		THEN
			Append( " ", args );
			Append( extraArgs, args );
	END;
	IF ( LENGTH(fileName) > 0 )
		THEN
			IF (descr = NIL)
				THEN
					Append( ' ', args );
					IF fileName[0] <> '"'
						THEN
							Append( '"', args );
					END;
					Append( fileName, args );
					IF fileName[0] <> '"'
						THEN
							Append( '"', args );
					END;
				ELSE
					Append( ' -attendVODDescription', args );
					mustSendVODDescription := TRUE;
			END;
		ELSE
			IF descr = NIL
				THEN
					(* noop - compilateur peut se tromper sur 'descr <> NIL' *)
				ELSE
					Append( ' -attendVODDescription', args );
					mustSendVODDescription := TRUE;
			END;
	END;
	timer := StartTimeEx();
	(*
	 * pInfo est une variable de sortie, mais il parait qu'il faut
	 * quand même initialiser hProcess!!
	 *)
%IF USE_SHELLEXECUTE %THEN
	errCode := CAST(INTEGER, ShellExecute( NIL, "open", qtvdPath, args, NIL_STR, SW_SHOWNORMAL ) );
	retB := errCode > 32;
	IF retB
		THEN
			QTils.LogMsgEx( 'LaunchQTVODm2: ShellExecute("%s","%s") retournait %d après %gs (OK)',
				qtvdPath, args, errCode, VAL(Real64,GetTimeEx(timer))/1000.0 );
		ELSE
			MSWinErrorString( GetLastError(), errMsg );
			QTils.LogMsgEx( 'LaunchQTVODm2: ShellExecute("%s","%s") retournait FALSE après %gs (erreur %d=%s)',
				qtvdPath, args, VAL(Real64,GetTimeEx(timer))/1000.0,
				errCode, errMsg );
	END;
%ELSE
	pInfo.hProcess := NULL_HANDLE;
	IF QTVODm2Process <> NULL_HANDLE
		THEN
			CloseHandle(QTVODm2Process);
			QTVODm2Process := NULL_HANDLE;
	END;
	retB := CreateProcess( qtvdPath, args, NIL_SECURITY_ATTRIBUTES, NIL_SECURITY_ATTRIBUTES,
		(*inheritHandles*) TRUE,
		CREATE_NEW_PROCESS_GROUP BOR DETACHED_PROCESS BOR CREATE_BREAKAWAY_FROM_JOB BOR
		CREATE_DEFAULT_ERROR_MODE BOR CREATE_PRESERVE_CODE_AUTHZ_LEVEL, NIL, NIL_STR, startup, pInfo );
	IF retB
		THEN
			QTVODm2Process := pInfo.hProcess;
			CloseHandle(pInfo.hThread);
			QTils.LogMsgEx( 'LaunchQTVODm2: CreateProcess("%s","%s") retournait TRUE après %gs (OK)',
				qtvdPath, args, VAL(Real64,GetTimeEx(timer))/1000.0 );
		ELSE
			errCode := GetLastError();
			MSWinErrorString( errCode, errMsg );
			QTils.LogMsgEx( 'LaunchQTVODm2: CreateProcess("%s","%s") retournait FALSE après %gs (erreur %d=%s)',
				qtvdPath, args, VAL(Real64,GetTimeEx(timer))/1000.0,
				errCode, errMsg );
	END;
%END
	RETURN retB;
END LaunchQTVODm2;

%END

PROCEDURE HasExtension( VAR URL : ARRAY OF CHAR; ext : ARRAY OF CHAR ) : BOOLEAN;
VAR
	extFound : BOOLEAN;
	extPos : CARDINAL;
	upURL, upExt : URLString;
BEGIN
	Assign( URL, upURL );
	Assign( ext, upExt );
	Capitalize( upURL );
	Capitalize( upExt );
	FindPrev( upExt, upURL, LENGTH(upURL)-1, extFound, extPos );
	RETURN extFound
END HasExtension;

PROCEDURE PruneExtension( VAR URL : ARRAY OF CHAR; ext : ARRAY OF CHAR );
VAR
	extFound : BOOLEAN;
	extPos : CARDINAL;
	upURL, upExt : URLString;
	c : CHAR;
BEGIN
	Assign( URL, upURL );
	Assign( ext, upExt );
	Capitalize( upURL );
	Capitalize( upExt );
	FindPrev( upExt, upURL, LENGTH(upURL)-1, extFound, extPos );
	IF extFound
		THEN
			c := URL[extPos+LENGTH(ext)];
			IF c = CHR(0)
				THEN
					(* c'est bien à la fin de la chaine qu'on a trouvé l'extension *)
					Delete( URL, extPos, LENGTH(ext) );
			END;
	END;
END PruneExtension;

(* =========================================================================================================== *)
BEGIN

	quitRequest := FALSE;
	resetRequest := FALSE;
	closeRequest := FALSE;

	HandleSendErrors := CommErrorHandler_nulle;
	HandleReceiveErrors := CommErrorHandler_nulle;
	SendErrors := 0;
	ReceiveErrors := 0;

	IF QueryPerformanceFrequency(HPCval)
		THEN
			HPCcalibrator := 1.0 / VAL(Real64, HPCval);
		ELSE
			HPCcalibrator := 0.0;
	END;
	QTVODm2Process := NIL;

FINALLY
	IF QTVODm2Process <> NULL_HANDLE
		THEN
			CloseHandle(QTVODm2Process);
			QTVODm2Process := NULL_HANDLE;
	END;
	FinIP;
END QTVODcomm.
