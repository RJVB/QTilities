MODULE QTVODm2;

<*/resource:QTVODm2.res*>
<*/VALIDVERSION:QTVOD_TESTING*>

%IF StonyBrook %AND %NOT(LINUX) %THEN
	FROM SYSTEM IMPORT
		AttachDebugger, AttachAll;
%END

FROM SYSTEM IMPORT
	CAST, ADDRESS;

FROM Strings IMPORT
	Equal, Length, Assign, Capitalize;

FROM ChanConsts IMPORT *;
FROM IOChan IMPORT
	ChanId;

(*
FROM TextIO IMPORT
	ReadToken;
FROM ProgramArgs IMPORT
	ArgChan, IsArgPresent, NextArg;
*)

FROM LongStr IMPORT
	StrToReal, ConvResults;

FROM WholeStr IMPORT
	StrToInt;

FROM ElapsedTime IMPORT
	StartTimeEx, GetTimeEx;

FROM QTilsM2 IMPORT *;
FROM QTVODlib IMPORT *;
FROM POSIXm2 IMPORT
	POSIX, POSIXAvailable;

FROM WIN32 IMPORT
	Sleep;

<*/VALIDVERSION:CHAUSSETTE2*>
%IF CHAUSSETTE2 %THEN
	FROM Chaussette2 IMPORT
%ELSE
	FROM Chaussette IMPORT
%END
		sock_nulle, Sortie, errSock;

FROM QTVODcomm IMPORT *;

VAR
	movie : URLString (*ARRAY[0..1024] OF CHAR *);
	idx, n, l, m : UInt32;
	movieDescription : VODDescription;
	msgNet, Netreply : NetMessage;
	handlingIdleAction, isCompleteReset : BOOLEAN;
	msgTimer : CARDINAL32;
	ipAddress : ARRAY[0..63] OF CHAR;
	err : ErrCode;

PROCEDURE SendErrorHandler( errors : CARDINAL );
VAR
	errStr : ARRAY[0..1023] OF CHAR;
BEGIN
	IF ( errors > 5 )
		THEN
			CloseCommClient(sServeur);
			MSWinErrorString( errSock, errStr );
			QTils.LogMsgEx( "%u message sending errors (last %d '%s'): closing communications channel",
				errors, errSock, errStr );
			PostMessage( "QTVODm2", "Connexion au serveur fermée!" );
	END;
END SendErrorHandler;

PROCEDURE CompareKeystroke( ref : CHAR; VAR c : CHAR ) : BOOLEAN
		%IF DLL %THEN [EXPORT, PROPAGATEEXCEPTION] %END;
BEGIN
	(* lastKeyUp est mise à jour par la MCActionCallback pour les frappes clavier,
	 * et remise à 0 ici après sa lecture. *)
	IF ( lastKeyUp <> CHR(0) )
		THEN
			c := lastKeyUp;
			lastKeyUp := CHR(0);
			RETURN ( c = ref );
	END;
	RETURN FALSE;
END CompareKeystroke;

(*
	Traite un messages "commande" reçu du réseau. L'action est prise, la trame 'msg' est modifiée
	où nécessaire, et une confirmation de la commande effectuée est renvoyé à la fin.
	En cas d'erreur, une notification est renvoyé, et la routine retourne immédiatement.
	On va supposed que la vue conducteur est le plus probablement ouverte pour les commandes
	pour lesquelles il faut renvoyer une information relative à la vidéo.

*)
PROCEDURE ReplyNetMsg( VAR msg : NetMessage ) : ErrCode;
VAR
	err : ErrCode;
	t : Real64;
	idx, n : Int32;
	sendDefaultReply : BOOLEAN;
BEGIN
	QTils.LogMsgEx( 'Msg Net lecteur "%s"', NetMessageToString(msg) );

	(* on retourne toute de suite si le message n'est pas une commande *)
	IF (msg.flags.class <> qtvod_Command)
		THEN
			RETURN noErr;
	END;

	msg.data.error := noErr;
	Netreply := msg;
	Netreply.flags.class := qtvod_Confirmation;
	sendDefaultReply := TRUE;

	CASE msg.flags.type OF

		qtvod_Close :
				IF handlingIdleAction
					THEN
						closeRequest := TRUE;
					ELSE
						CloseVideo(TRUE);
						(* avec numQTWM = -1, on restera actif en attente d'une nouvelle commande *)
						numQTWM := -1;
				END;

		| qtvod_Reset :
				IF handlingIdleAction
					THEN
						resetRequest := TRUE;
						sendDefaultReply := FALSE;
						isCompleteReset := msg.data.boolean;
					ELSE
						err := ResetVideo(msg.data.boolean);
						IF err = noErr
							THEN
								IF QTils.QTMovieWindowH_isOpen(qtwmH[fwWin])
									THEN
										QTils.QTMovieWindowGetTime( qtwmH[fwWin], Netreply.data.val1, 0 );
										Netreply.data.description.frequency := qtwmH[fwWin]^^.info^.frameRate;
								END;
							ELSE
								SendNetErrorNotification( sServeur, NetMessageToString(msg), err );
								msgCloseMovie( Netreply );
								Netreply.flags.class := qtvod_Notification;
								numQTWM := -1;
						END;
				END;

		| qtvod_Quit :
				IF handlingIdleAction
					THEN
						quitRequest := TRUE;
					ELSE
						CloseVideo(TRUE);
						numQTWM := 0;
				END;
				(* on laisse un peu de temps au serveur de se fermer *)
				Sleep(1000);
				(* we close the connection to the server now as it's probably gone. *)
				CloseCommClient(sServeur);

		| qtvod_Open :
				CloseVideo(TRUE);
				err := OpenVideo( msg.data.URN, msg.data.description );
				IF (numQTWM > 0)
					THEN
						(* notification de quelles vues ne se sont pas ouvertes: *)
						WITH msg.data.description.channels DO
							IF( qtwmH[pilotWin] = NULL_QTMovieWindowH )
								THEN
									pilot := -1;
							END;
							IF( qtwmH[leftWin] = NULL_QTMovieWindowH )
								THEN
									left := -1;
							END;
							IF( qtwmH[fwWin] = NULL_QTMovieWindowH )
								THEN
									forward := -1;
							ELSE
								(* on met à jour la fréquence demandée au départ par la fréquence de la vidéo
								 * (NB: la fréquence demandée ne sert que pour importer des fichiers VOD; les vidéos
								 * QuickTime ont une fréquence native et connue). *)
								msg.data.description.frequency := qtwmH[fwWin]^^.info^.frameRate;
								Netreply.data.description.frequency := qtwmH[fwWin]^^.info^.frameRate;
							END;
							IF( qtwmH[rightWin] = NULL_QTMovieWindowH )
								THEN
									right := -1;
							END;
						END;
					ELSE
						SendNetErrorNotification( sServeur, msg.data.URN, err );
						RETURN err;
				END;

		| qtvod_Stop :
				StopVideo(NULL_QTMovieWindowH);
				IF QTils.QTMovieWindowGetTime( qtwmH[tcWin], t, 0 ) = noErr
					THEN
						msg.data.val1 := t;
						Netreply.data.val1 := t;
					ELSE
						msg.data.val1 := 0.0;
						Netreply.data.val1 := 0.0;
				END;
				msg.data.boolean := FALSE;
				Netreply.data.boolean := FALSE;
		| qtvod_Start :
				StartVideo(NULL_QTMovieWindowH);
				IF QTils.QTMovieWindowGetTime( qtwmH[tcWin], t, 0 ) = noErr
					THEN
						msg.data.val1 := t;
						Netreply.data.val1 := t;
					ELSE
						msg.data.val1 := 0.0;
						Netreply.data.val1 := 0.0;
				END;
				msg.data.boolean := FALSE;
				Netreply.data.boolean := FALSE;

		| qtvod_GetTime :
				IF (qtwmH[fwWin] <> NULL_QTMovieWindowH)
					THEN
						replyCurrentTime( Netreply, qtvod_Confirmation, qtwmH[fwWin], msg.data.boolean );
					ELSE
						SendNetErrorNotification( sServeur, NetMessageToString(msg), 1 );
						RETURN 1;
				END;

		| qtvod_GotoTime :
				SetTimes( msg.data.val1, NULL_QTMovieWindowH, VAL(Int32,msg.data.boolean) );
				IF (qtwmH[fwWin] <> NULL_QTMovieWindowH)
					THEN
						(* check the resulting time. If different from the requested time, signal
						 * an error in the notification message *)
						QTils.QTMovieWindowGetTime(qtwmH[fwWin], t, VAL(Int32,msg.data.boolean) );
						IF (t <> msg.data.val1)
							THEN
								msg.data.error := 1;
								Netreply.data.error := 1;
						END;
				END;

		| qtvod_GetStartTime :
				IF NOT replyStartTime( Netreply, qtvod_Confirmation, qtwmH[fwWin] )
					THEN
						SendNetErrorNotification( sServeur, NetMessageToString(msg), 1 );
						RETURN 1;
				END;

		| qtvod_GetDuration :
				IF NOT replyDuration( Netreply, qtvod_Confirmation, qtwmH[fwWin] )
					THEN
						SendNetErrorNotification( sServeur, NetMessageToString(msg), 1 );
						RETURN 1;
				END;

		| qtvod_GetChapter :
			IF fullMovie <> NIL
				THEN
					IF msg.data.iVal1 < 0
						THEN
							(* on renvoie toute la liste des chapitres *)
							sendDefaultReply := FALSE;
							n := QTils.GetMovieChapterCount(fullMovie);
							QTils.LogMsgEx( "Envoi des %d titres de chapitre de '%s'\n", n, qtwmH[tcWin]^^.theURL^ );
							FOR idx := 0 TO n-1 DO
								err := QTils.GetMovieIndChapter( fullMovie, idx,
									Netreply.data.val1, Netreply.data.URN );
								IF err = noErr
									THEN
										replyChapter( Netreply, qtvod_Confirmation, Netreply.data.URN, idx,
											Netreply.data.val1, 0.0 );
										SendMessageToNet( sServeur, Netreply, SENDTIMEOUT, FALSE, "QTVODm2::ReplyNetMsg - client" );
									ELSE
										SendNetErrorNotification( sServeur, NetMessageToString(msg), err );
								END;
							END;
						ELSE
							(* on renvoie juste le chapitre demandé *)
							err := QTils.GetMovieIndChapter( fullMovie, msg.data.iVal1,
								Netreply.data.val1, Netreply.data.URN );
							IF err = noErr
								THEN
									replyChapter( Netreply, qtvod_Confirmation, Netreply.data.URN, msg.data.iVal1,
										Netreply.data.val1, 0.0 );
								ELSE
									SendNetErrorNotification( sServeur, NetMessageToString(msg), err );
									RETURN 1;
							END;
					END;
				ELSE
					SendNetErrorNotification( sServeur, NetMessageToString(msg), 1 );
					RETURN 1;
			END;

			| qtvod_NewChapter :
				(*
						on ajoute le nouveau chapitre au fullMovie mais également à qtwmH[tcWin] pour qu'il
						apparaisse.
				*)
				IF QTils.QTMovieWindowH_Check(qtwmH[tcWin])
					THEN
						(* on ignore msg.data.boolean, on prend tous les temps en relatif pour l'instant! *)
						IF (msg.data.val1 < 0.0) OR (msg.data.val1 > QTils.GetMovieDuration(fullMovie))
							THEN
								SendNetErrorNotification( sServeur, NetMessageToString(msg), invalidTime );
								RETURN 1;
							ELSIF (msg.data.val1 + msg.data.val2) > QTils.GetMovieDuration(fullMovie)
								THEN
									SendNetErrorNotification( sServeur, NetMessageToString(msg), invalidDuration );
									RETURN 1;
						END;
						err := QTils.MovieAddChapter( qtwmH[tcWin]^^.theMovie, 0, msg.data.URN, msg.data.val1, msg.data.val2 );
						err := QTils.MovieAddChapter( fullMovie, 0, msg.data.URN, msg.data.val1, msg.data.val2 );
						IF err = noErr
							THEN
								(* on va trouver l'indice du nouveau chapitre! *)
								sendDefaultReply := FALSE;
								n := QTils.GetMovieChapterCount(fullMovie);
								idx := 0;
								WHILE idx < n DO
									err := QTils.GetMovieIndChapter( fullMovie, idx,
										Netreply.data.val1, Netreply.data.URN );
									IF (err = noErr) AND Equal(Netreply.data.URN, msg.data.URN)
											AND (Netreply.data.val1 = msg.data.val1)
										THEN
											replyChapter( Netreply, qtvod_Confirmation, Netreply.data.URN, idx,
												Netreply.data.val1, 0.0 );
											idx := n;
											sendDefaultReply := TRUE;
										ELSE
											INC(idx);
									END;
								END;
							ELSE
								SendNetErrorNotification( sServeur, NetMessageToString(msg), err );
								RETURN 1;
						END;
					ELSE
						SendNetErrorNotification( sServeur, NetMessageToString(msg), 1 );
						RETURN 1;
				END;

			| qtvod_MarkIntervalTime :
				CalcTimeInterval(msg.data.boolean, FALSE);
				IF QTils.QTMovieWindowGetTime( qtwmH[tcWin], t, 0 ) = noErr
					THEN
						msg.data.val1 := t;
						Netreply.data.val1 := t;
					ELSE
						msg.data.val1 := 0.0;
						Netreply.data.val1 := 0.0;
				END;
				msg.data.boolean := FALSE;
				Netreply.data.boolean := FALSE;

			| qtvod_GetLastInterval :
				IF NOT replyLastInterval( Netreply, qtvod_Confirmation )
					THEN
						SendNetErrorNotification( sServeur, NetMessageToString(msg), 1 );
						RETURN 1;
				END;
				sendDefaultReply := TRUE;

			ELSE
				(* noop *)
	END;

	(* renvoi du message reçu en confirmation (par défaut) *)
	IF sendDefaultReply
		THEN
			SendMessageToNet( sServeur, Netreply, SENDTIMEOUT, FALSE, "QTVODm2::ReplyNetMsg - client" );
	END;
	msg.flags.class := qtvod_Confirmation;
	RETURN msg.data.error;
END ReplyNetMsg;

PROCEDURE PumpNetMessagesOnce(timeOutms : INTEGER) : BOOLEAN;
BEGIN
	msgTimer := StartTimeEx();
	IF ReceiveMessageFromNet( sServeur, msgNet, timeOutms, FALSE, "QTVODM2" )
		THEN
			ReplyNetMsg(msgNet);
			m := m + 1;
			RETURN TRUE;
		ELSE
			RETURN FALSE;
	END;
END PumpNetMessagesOnce;

PROCEDURE PumpNetMessages(firstTimeOutms : INTEGER) : UInt32;
VAR
	n : UInt32;
BEGIN
	n := 0;
	WHILE PumpNetMessagesOnce(firstTimeOutms) DO
		firstTimeOutms := 0;
		n := n + 1;
	END;
(*
	QTils.LogMsgEx( "QTVODm2 Received %u messages", n );
*)
	RETURN n;
END PumpNetMessages;

PROCEDURE movieIdle(wih : QTMovieWindowH; params : ADDRESS ) : Int32 [CDECL];
BEGIN
	IF ( handlingIdleAction OR (sServeur = sock_nulle) OR (GetTimeEx(msgTimer) < 100) )
		THEN
			RETURN 0;
		END;
	handlingIdleAction := TRUE;
	PumpNetMessagesOnce(0);
	handlingIdleAction := FALSE;
	IF ( closeRequest OR quitRequest OR resetRequest )
		THEN
			RETURN 1
		ELSE
			RETURN 0;
	END;
END movieIdle;

PROCEDURE ParseArgs;
VAR
	arg : UInt16;
	convResult : ConvResults;
	realVal : LONGREAL;
BEGIN
	arg := 1;
	WHILE arg < POSIX.argc  DO
		IF Equal(POSIX.Arg(arg), "-client")
			THEN
				IF arg < POSIX.argc-1
					THEN
						INC(arg);
						Assign( POSIX.Arg(arg), ipAddress );
					ELSE
						Assign( "127.0.0.1", ipAddress );
				END;
				InitCommClient( sServeur, ipAddress, ServerPortNr, ClientPortNr, 50 );
			ELSIF Equal(POSIX.Arg(arg), "-assocData") AND (arg < POSIX.argc-1)
				THEN
					IF LENGTH(POSIX.Arg(arg+1)) > 0
						THEN
							INC(arg);
							Assign( POSIX.Arg(arg), assocDataFileName );
						ELSE
							Assign( "*FromVODFile*", assocDataFileName );
					END;
			ELSIF Equal(POSIX.Arg(arg), "-attendVODDescription")
				THEN
					IF ( sServeur = sock_nulle )
						THEN
							PostMessage( "QTVODm2", "Pas de serveur pour nous décrire une vidéo à ouvrir!" );
						ELSE
							Assign( POSIX.Arg(arg), movie );
					END;
					RETURN;
			ELSIF ( Equal(POSIX.Arg(arg), "-freq") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					StrToReal( POSIX.Arg(arg), realVal, convResult );
					movieDescription.frequency := VAL(Real64,realVal);
					QTils.LogMsgEx( "-freq=%s -> %g", POSIX.Arg(arg), movieDescription.frequency );
			ELSIF ( Equal(POSIX.Arg(arg), "-timeZone") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					StrToReal( POSIX.Arg(arg), realVal, convResult );
					movieDescription.timeZone := VAL(Real64,realVal);
					QTils.LogMsgEx( "-timeZone=%s -> %g", POSIX.Arg(arg), movieDescription.timeZone );
			ELSIF ( Equal(POSIX.Arg(arg), "-DST") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					Capitalize(POSIX.argv^[arg]^);
					movieDescription.DST := Equal(POSIX.Arg(arg), "TRUE");
					QTils.LogMsgEx( "-DST=%s -> %d", POSIX.Arg(arg), VAL(INTEGER,movieDescription.DST) );
			ELSIF ( Equal(POSIX.Arg(arg), "-hFlip") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					Capitalize(POSIX.argv^[arg]^);
					movieDescription.flipLeftRight := Equal(POSIX.Arg(arg), "TRUE");
					QTils.LogMsgEx( "-hFlip=%s -> %d", POSIX.Arg(arg), VAL(INTEGER,movieDescription.flipLeftRight) );
			ELSIF ( Equal(POSIX.Arg(arg), "-VMGI") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					Capitalize(POSIX.argv^[arg]^);
					movieDescription.useVMGI := Equal(POSIX.Arg(arg), "TRUE");
					QTils.LogMsgEx( "-VMGI=%s -> %d", POSIX.Arg(arg), VAL(INTEGER,movieDescription.useVMGI) );
			ELSIF ( Equal(POSIX.Arg(arg), "-log") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					Capitalize(POSIX.argv^[arg]^);
					movieDescription.log := Equal(POSIX.Arg(arg), "TRUE");
					QTils.LogMsgEx( "-log=%s -> %d", POSIX.Arg(arg), VAL(INTEGER,movieDescription.log) );
			ELSIF ( Equal(POSIX.Arg(arg), "-scale") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					StrToReal( POSIX.Arg(arg), realVal, convResult );
					movieDescription.scale := VAL(Real64,realVal);
					QTils.LogMsgEx( "-scale=%s -> %g", POSIX.Arg(arg), movieDescription.scale );
			ELSIF ( Equal(POSIX.Arg(arg), "-chForward") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					StrToInt( POSIX.Arg(arg), movieDescription.channels.forward, convResult );
					ClipInt( movieDescription.channels.forward, 1, 4 );
					QTils.LogMsgEx( "-chForward=%s -> %d", POSIX.Arg(arg), movieDescription.channels.forward );
			ELSIF ( Equal(POSIX.Arg(arg), "-chPilot") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					StrToInt( POSIX.Arg(arg), movieDescription.channels.pilot, convResult );
					ClipInt( movieDescription.channels.pilot, 1, 4 );
					QTils.LogMsgEx( "-chPilot=%s -> %d", POSIX.Arg(arg), movieDescription.channels.pilot );
			ELSIF ( Equal(POSIX.Arg(arg), "-chLeft") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					StrToInt( POSIX.Arg(arg), movieDescription.channels.left, convResult );
					ClipInt( movieDescription.channels.left, 1, 4 );
					QTils.LogMsgEx( "-chLeft=%s -> %d", POSIX.Arg(arg), movieDescription.channels.left );
			ELSIF ( Equal(POSIX.Arg(arg), "-chRight") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					StrToInt( POSIX.Arg(arg), movieDescription.channels.right, convResult );
					ClipInt( movieDescription.channels.right, 1, 4 );
					QTils.LogMsgEx( "-chRight=%s -> %d", POSIX.Arg(arg), movieDescription.channels.right );
			ELSIF ( Equal(POSIX.Arg(arg), "-fcodec") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					Assign( POSIX.Arg(arg), movieDescription.codec );
					QTils.LogMsgEx( "-fcodec=%s", movieDescription.codec );
			ELSIF ( Equal(POSIX.Arg(arg), "-fbitrate") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					Assign( POSIX.Arg(arg), movieDescription.bitRate );
					QTils.LogMsgEx( "-fbitrate=%s", movieDescription.bitRate );
			ELSIF ( Equal(POSIX.Arg(arg), "-fsplit") AND (arg < POSIX.argc-1) )
				THEN
					INC(arg);
					Capitalize(POSIX.argv^[arg]^);
					movieDescription.splitQuad := Equal(POSIX.Arg(arg), "TRUE");
					QTils.LogMsgEx( "-fsplit=%s -> %d", POSIX.Arg(arg), VAL(INTEGER,movieDescription.splitQuad) );
			ELSE
				Assign( POSIX.Arg(arg), movie );
		END;
		INC(arg);
	END;
(*
	IF ( Length(movie) <= 0 )
		THEN
			PostMessage( "QTVODm2", "aucun fichier spécifié" );
	END;
*)
END ParseArgs;

%IF QTVOD_TESTING %THEN
PROCEDURE test23();
VAR
	t : Real64;
BEGIN
	IF (qtwmH[fwWin] <> NULL_QTMovieWindowH)
		THEN
			WHILE( QTils.QTMovieWindowStepNext( qtwmH[fwWin], 1 ) = noErr ) DO
				IF QTils.QTMovieWindowGetTime(qtwmH[fwWin], t, 0) = noErr
					THEN
						SetTimes( t, qtwmH[fwWin], 0 );
				END;
				QTils.PumpMessages(0);
			END;
			SetTimes( 0.0, NULL_QTMovieWindowH, 0 );
	END;
END test23;
%END

PROCEDURE BenchmarkStep;
VAR
	t : Real64;
BEGIN
	IF QTils.QTMovieWindowStepNext( qtwmH[fwWin], 1 ) = noErr
		THEN
			IF QTils.QTMovieWindowGetTime(qtwmH[fwWin], t, 0) = noErr
				THEN
					SetTimes( t, qtwmH[fwWin], 0 );
			END;
		QTils.PumpMessages(0);
	ELSE
		(* probably reached end of movie, but we stop benchmarking on any other error too *)
		BenchmarkPlaybackRate;
	END;
END BenchmarkStep;

(* ==================================== BEGIN ==================================== *)
BEGIN
%IF StonyBrook %AND %NOT(LINUX) %THEN
	AttachDebugger(AttachAll);
%END

	QTils.QTils_Allocator^.malloc := POSIX.malloc;
	QTils.QTils_Allocator^.calloc := POSIX.calloc;
	QTils.QTils_Allocator^.realloc := POSIX.realloc;
	QTils.QTils_Allocator^.free := POSIX.free;
	IF NOT POSIXAvailable()
		THEN
			PostMessage( "QTVODm2", "Pas de gestion d'arguments de commande sans POSIXm2.dll !" );
	END;

	(* on initialise QTilsM2 *)
	OpenQT();

	Sortie := CompareKeystroke;
	HandleSendErrors := SendErrorHandler;

	(* QTOpened est TRUE si QTilsM2 s'est initialisé correctement, sinon, QuickTime n'est pas disponible: *)
	IF QTOpened()
		THEN
			(* configuration par défaut de la lecture de fichiers VOD *)
			movieDescription.frequency := 12.5;
			movieDescription.scale := 1.0;
			(* zone par défaut: l'Europe de l'ouest *)
			movieDescription.timeZone := 1.0;
			movieDescription.DST := FALSE;
			movieDescription.flipLeftRight := TRUE;
			(* on utilise la structure VMGI par défaut! *)
			movieDescription.useVMGI := TRUE;
			movieDescription.log := FALSE;
			(*
			 * les 'canaux' contenant les vues des 4 caméras. A priori j'avais conçu les connexions
			 * pour que l'attribution suivante est la correcte:
			 *)
			movieDescription.channels.forward := 1;
			movieDescription.channels.pilot := 2;
			movieDescription.channels.left := 3;
			movieDescription.channels.right := 4;
			(*
			 * on tente de lire le fichier avec la description par défaut, dans le répertoire
			 * de démarrage.
			 *)
			err := ReadDefaultVODDescription( "VODdesign.xml", movieDescription );
			movie := "";
			ParseArgs();
			IF ( NOT Equal(movie, "-attendVODDescription") )
				THEN

					QTils.LogMsgEx( 'Display file "%s"', movie );
					(*
					 * Ouvre les fenêtres pour la visualisation
					 * associe évènements et procédure de traitement.
					 * Si movie="", l'utilisateur aura la possibilité de choisir
					 * un fichier via une fenêtre dialogue.
					*)
					OpenVideo( movie, movieDescription );
					IF ( QTils.QTMovieWindowH_isOpen(qtwmH[fwWin]) )
						THEN
							(* la frequence reelle n'est pas forcement exactement celle demandee! *)
							IF ( movieDescription.frequency <> qtwmH[fwWin]^^.info^.frameRate )
								THEN
									QTils.LogMsgEx( "Fréquence demandée %gHz, obtenue %gHz",
										movieDescription.frequency, qtwmH[fwWin]^^.info^.frameRate
									);
									movieDescription.frequency := qtwmH[fwWin]^^.info^.frameRate;
							END;
					END;
					(* notification du serveur de l'ouverture d'un fichier video: *)
					IF ( sServeur <> sock_nulle )
						THEN
							FOR idx := 0 TO maxQTWM DO
								(*
								 * si jamais OpenVideo n'a pas initalise <movie>, on obtient le nom
								 * du fichier ouvert a partir de la 1e fenetre ouverte
								 *)
								IF ( (movie[0] = CHR(0)) AND QTils.QTMovieWindowH_isOpen(qtwmH[idx]) )
									THEN
										movie := qtwmH[idx]^^.theURL^;
										PruneExtension( movie, ".mov" );
										PruneExtension( movie, "-ch1" );
										PruneExtension( movie, "-ch2" );
										PruneExtension( movie, "-ch3" );
										PruneExtension( movie, "-ch4" );
										PruneExtension( movie, "-ch5" );
										PruneExtension( movie, "-ch6" );
										PruneExtension( movie, "-forward" );
										PruneExtension( movie, "-pilot" );
										PruneExtension( movie, "-left" );
										PruneExtension( movie, "-right" );
										PruneExtension( movie, "-TC" );
								END;
							END;
							(* construction du message de notification qtvod_OpenFile: *)
							msgOpenFile( Netreply, CAST(URLString,movie), movieDescription );
							Netreply.flags.class := qtvod_Notification;
							SendMessageToNet( sServeur, Netreply, SENDTIMEOUT, FALSE, "QTVODm2 - file open" );
						ELSE
							(* construction du message de notification qtvod_OpenFile: *)
							msgOpenFile( Netreply, CAST(URLString,movie), movieDescription );
							Netreply.flags.class := qtvod_Notification;
							NetMessageToLogMsg( "QTVODm2", "void msg", Netreply );
					END;
				ELSE
					(*
					 * on est sensé recevoir le nom et la description de la vidéo à ouvrir de la part du serveur.
					 * Pour cela il suffit d'entrer dans la boucle et d'y rester... en traitant les messages reçus
					 *)
					numQTWM := -1;
			END;

%IF QTVOD_TESTING %THEN
			(* test *)
			test23();
%END

			(* on traite les évenements tant qu'au moins 1 des fenêtres reste ouverte: *)
			QTils.LogMsg( "entrée de la boucle principale" );
			n := 0; l:= 0; m := 0;
			msgTimer := StartTimeEx();
			WHILE ( (numQTWM <> 0) AND (NOT quitRequest) ) DO
				IF (theTimeInterVal.benchMarking) AND (qtwmH[fwWin] <> NULL_QTMovieWindowH)
					THEN
						BenchmarkStep();
				ELSIF ( sServeur <> sock_nulle )
					THEN

						(* le callback pour l'action Idle n'est installée qu'une seule fois, pour la vu 'forward',
						 * et uniquement quand on est en connexion avec un serveur!
						 *)
						IF ( QTils.get_MCAction(qtwmH[fwWin], MCAction.Idle) = NULL_MCActionCallback )
							THEN
								IF (qtwmH[fwWin] <> NULL_QTMovieWindowH)
									THEN
										QTils.register_MCAction( qtwmH[fwWin], MCAction.Idle, movieIdle );
										QTils.LogMsgEx( 'QTVODm2::movieIdle installée pour "%s"', qtwmH[fwWin]^^.theURL^ );
								END;
						END;

						PumpNetMessages(250);
						n := n + QTils.PumpMessages(0);
%IF %NOT CHAUSSETTE2 %THEN
						Sleep(250);
%END
					ELSE
						n := n + QTils.PumpMessages(1);
				END;
				l := l + 1;
				IF ( closeRequest )
					THEN
						CloseVideo(TRUE);
						(* avec numQTWM = -1, on restera actif en attente d'une nouvelle commande *)
						numQTWM := -1;
						closeRequest := FALSE;
				END;
				IF ( resetRequest )
					THEN
						err := ResetVideo(isCompleteReset);
						IF err = noErr
							THEN
								msgResetQTVOD( Netreply, isCompleteReset );
								(* ceci est forcément une confirmation... *)
								Netreply.flags.class := qtvod_Confirmation;
								IF QTils.QTMovieWindowH_isOpen(qtwmH[fwWin])
									THEN
										QTils.QTMovieWindowGetTime( qtwmH[fwWin], Netreply.data.val1, 0 );
										Netreply.data.description.frequency := qtwmH[fwWin]^^.info^.frameRate;
								END;
							ELSE
								SendNetErrorNotification( sServeur, "resetRequest", err );
								msgCloseMovie( Netreply );
								Netreply.flags.class := qtvod_Notification;
								numQTWM := -1;
						END;
						SendMessageToNet( sServeur, Netreply, SENDTIMEOUT, FALSE, "QTVODm2 - reset" );
						resetRequest := FALSE;
				END;
			END; (*boucleur principal*)
			QTils.LogMsgEx( "%d évenements et %d messages en %d boucles", n, m, l );
			CloseVideo(TRUE);
			IF sServeur <> sock_nulle
				THEN
					SendNetCommandOrNotification( sServeur, qtvod_Quit, qtvod_Notification );
					Sleep(1000);
			END;
(*
			IF (m = 0) AND (sServeur = sock_nulle)
				THEN
					PostMessage( "QTVODm2", "La Fin!" );
			END;
*)
			CloseCommClient(sServeur);
		ELSE
			QTils.LogMsgEx( "Echec d'initialisation de QuickTime et/ou QTilsM2... (%d)", QTOpenError() );
	END;

FINALLY

	IF sServeur <> sock_nulle
		THEN
			(*
				on arriverait ici en cas de sortie abnormale
			*)
			SendNetCommandOrNotification( sServeur, qtvod_Quit, qtvod_Notification );
			Sleep(1000);
			CloseCommClient(sServeur);
	END;

END QTVODm2.
