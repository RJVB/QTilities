MODULE TstQTVDSrv;

<*/VALIDVERSION:TESTING*>
<*/VERSION:TESTING*>
<*/VERSION:QTILS_DEBUG*>

FROM SYSTEM IMPORT
	CAST, ADR,
%IF StonyBrook %AND %NOT(LINUX) %THEN
		AttachDebugger, AttachAll;
%END

%IF CHAUSSETTE2 %THEN
FROM Chaussette2 IMPORT
%ELSE
FROM Chaussette IMPORT
%END
	SOCK, sock_nulle;

FROM Terminal IMPORT
	WriteString, WriteLn, CharAvail, Read;

FROM ChanConsts IMPORT *;
FROM IOChan IMPORT
	ChanId;
FROM TextIO IMPORT
	ReadToken;
FROM ProgramArgs IMPORT
	ArgChan, IsArgPresent, NextArg;

FROM WIN32 IMPORT
	Sleep;

FROM ElapsedTime IMPORT
	StartTimeEx, GetTimeEx;

FROM QTVODcomm IMPORT *;
FROM QTilsM2 IMPORT
	UInt32, Real64, ErrCode, noErr, QTils, MovieFrameTime;

VAR
	c : CHAR;
	srv, s : SOCK;
	receipt, reply, command : NetMessage;
	finished, clientQuit, sendNewFileName, currentTimeSubscribed : BOOLEAN;
	received, sent : UInt32;
	msgTimer, clntTimer : CARDINAL32;

	fName : URLString;
	descrFichierVOD : VODDescription;
	startTimeAbs, Duration : Real64;

	ArgsChan : ChanId;

PROCEDURE CheckQuit() : BOOLEAN;
BEGIN
	IF CharAvail()
		THEN
			Read(c);
			CASE c OF
				'q', 'Q' :
					RETURN TRUE;
				ELSE
					(* noop *)
			END;
	END;
	RETURN FALSE;
END CheckQuit;

(* gestion des messages reçus, qui sont des confirmations ou des notifications *)
PROCEDURE TraiteMsgDeQTVODm2( VAR msg, reply : NetMessage ) : ErrCode;
VAR
	ft : MovieFrameTime;
	reply2 : NetMessage;
BEGIN
	QTils.LogMsgEx( 'Msg Net serveur "%s"', NetMessageToString(msg) );
	received := received + 1;

	(* on retourne toute de suite si le message est une commande *)
	IF (msg.flags.class = qtvod_Command)
		THEN
			RETURN noErr;
	END;
	msg.data.error := noErr;
	(* par defaut on ne renvoie aucun message: *)
	reply.flags.type := qtvod_NoType;
	reply2.flags.type := qtvod_NoType;

	CASE msg.flags.type OF

		qtvod_Close :
				WriteString( QTils.lastSSLogMsg^ ) ; WriteLn;

		| qtvod_Quit :
				WriteString( QTils.lastSSLogMsg^ ) ; WriteLn;
				WriteString( "Exit!" ) ; WriteLn;
				finished := TRUE;
				clientQuit := TRUE;

		| qtvod_Open :
				NetMessageToLogMsg( "Fichier ouvert", "TstQTVDSrv", msg );
				WriteString( QTils.lastSSLogMsg^ ) ; WriteLn;
				(* le client (QTVODm2) nous notifie/confirme qu'il a ouvert une vidéo. Le message
				 * contient le nom et la description de cette vidéo, qu'on récupère ici. *)
				fName := msg.data.URN;
				descrFichierVOD := msg.data.description;
				(* on veut aussi connaître la durée et l'heure de début de l'enregistrement: *)
				msgGetStartTime(reply);
				msgGetDuration(reply2);

		| qtvod_Start,
		qtvod_Stop, qtvod_MovieFinished :
				WriteString( NetMessageToString(msg) ); WriteLn;
				IF ( msg.flags.class = qtvod_Notification )
					THEN
						NetMessageToLogMsg( "Notification", "TstQTVDSrv", msg );
						(* l'utilisateur a demarre/arrete la lecture: quelle heure est-il la-bas? *)
						msgGetTime( reply, TRUE );
						msgTimer := StartTimeEx();
						IF (msg.flags.type = qtvod_MovieFinished) AND (msg.data.iVal1 = 0)
							THEN
								msgGotoTime( reply2, 60.0, FALSE );
						END;
				END;

		| qtvod_CurrentTime :
				NetMessageToLogMsg( "Temps actuel", "TstQTVDSrv", msg );
				WriteString( QTils.lastSSLogMsg^ ) ; WriteLn;
				IF msg.data.boolean
					THEN
						QTils.secondsToFrameTime( msg.data.val1, msg.data.val2, ft );
						QTils.LogMsgEx( "Temps abs: %02d:%02d:%02d;%d",
							VAL(INTEGER,ft.hours), VAL(INTEGER,ft.minutes),
							VAL(INTEGER,ft.seconds), VAL(INTEGER,ft.frames) );
						WriteString( QTils.lastSSLogMsg^ ) ; WriteLn;
				END;

		| qtvod_StartTime :
				NetMessageToLogMsg( "Temps début", "TstQTVDSrv", msg );
				WriteString( QTils.lastSSLogMsg^ ) ; WriteLn;
				startTimeAbs := msg.data.val1;

		| qtvod_Duration :
				NetMessageToLogMsg( "Durée", "TstQTVDSrv", msg );
				WriteString( QTils.lastSSLogMsg^ ) ; WriteLn;
				Duration := msg.data.val1;

		ELSE
			(* noop *)
	END;

	(* envoi du ou des message(s) vers le client *)
	IF ( reply.flags.type <> qtvod_NoType )
		THEN
			SendMessageToNet( s, reply, SENDTIMEOUT, FALSE, "TraiteMsgDeQTVODm2" );
	END;
	IF ( reply2.flags.type <> qtvod_NoType )
		THEN
			SendMessageToNet( s, reply2, SENDTIMEOUT, FALSE, "TraiteMsgDeQTVODm2" );
	END;

	RETURN msg.data.error;
END TraiteMsgDeQTVODm2;

PROCEDURE GetClient( VAR s : SOCK ) : BOOLEAN;
VAR
	t : CARDINAL32;
	ts : Real64;
BEGIN
	IF ( s = sock_nulle )
		THEN
			IF NOT ServerCheckForClient( srv, s, 250, FALSE )
				THEN
%IF %NOT CHAUSSETTE2 %THEN
					(* Chaussete2 utilise Select pour imposer un delai d'attente, sinon
					 * il faut utiliser Sleep ou semblable (250ms) *)
					Sleep(250);
%END
				ELSE
					t := GetTimeEx(clntTimer);
					ts := VAL(Real64,t)/1000.0;
					QTils.LogMsgEx( "GetClient() client connect apres %gs", ts );
					WriteString( QTils.lastSSLogMsg^ ) ; WriteLn;
			END;
	END;
	IF ( s <> sock_nulle )
		THEN
			IF sendNewFileName
				THEN
					msgOpenFile( command, fName, descrFichierVOD );
					SendMessageToNet( s, command, SENDTIMEOUT, FALSE, "TstQTVDSrv::GetClient-envoi" );
					sendNewFileName := FALSE;
			END;
			RETURN TRUE;
		ELSE
			RETURN FALSE;
	END;
END GetClient;

(* =========================================================================================================== *)

BEGIN
%IF StonyBrook %AND %NOT(LINUX) %THEN
	AttachDebugger(AttachAll);
%END

	finished := FALSE;
	Duration := -1.0;
	s := sock_nulle;
	received := 0;
	sent := 0;
	clientQuit := FALSE;
	currentTimeSubscribed := FALSE;
	ArgsChan := ArgChan();
	IF IsArgPresent()
		THEN
			ReadToken( ArgsChan, fName );
			descrFichierVOD.frequency := 12.5;
			descrFichierVOD.scale := 0.75;
			descrFichierVOD.channels.forward := 1;
			descrFichierVOD.channels.pilot := 2;
			descrFichierVOD.channels.left := 3;
			descrFichierVOD.channels.right := 4;
			QTils.LogMsgEx( "file to be opened: %s\n", fName );
		ELSE
			fName := "";
			QTils.LogMsg( "No arguments specified" );
	END;
	IF InitCommServer(srv, ServerPortNr)
		THEN
			clntTimer := StartTimeEx();
			WriteString( "Serveur lance... (q/Q pour sortir)" ); WriteLn;
			QTils.LogMsg( "Serveur lancé...\n(q/Q pour sortir)\n" );
			IF ( LENGTH(fName) > 0 )
				THEN
					LaunchQTVODm2( ".", fName, "-debugWait", ADR(descrFichierVOD), "127.0.0.1", sendNewFileName );
				ELSE
					sendNewFileName := FALSE;
			END;
			LOOP
				IF ( GetClient(s) )
					THEN
						IF (NOT currentTimeSubscribed) AND (Duration >= 0.0)
							THEN
								msgGetTimeSubscription( command, 1.5, FALSE );
								SendMessageToNet( s, command, SENDTIMEOUT, FALSE, "TstQTVDSrv-envoi" );
								currentTimeSubscribed := TRUE;
						END;
						IF ReceiveMessageFromNet( s, receipt, 250, FALSE, "TstQTVDSrv-lecture" )
							THEN
								TraiteMsgDeQTVODm2(receipt, reply);
								IF finished THEN EXIT; END;
							ELSE
%IF %NOT CHAUSSETTE2 %THEN
								Sleep(250);
%END
						END;
						finished := CheckQuit();
						IF finished
							THEN
								(* on n'envoie pas de commande Quit is on vient de recevoir la confirmation/notification
								 * de l'arret du lecteur QTVOD! *)
								IF ( receipt.flags.type <> qtvod_Quit )
									THEN
										msgCloseMovie(command);
										SendMessageToNet( s, command, SENDTIMEOUT, FALSE, "TstQTVDSrv-envoi" );
										WriteString("commande 'Close' envoyee"); WriteLn;
										Sleep(2000);
%IF TESTING %THEN
										(* pour tester, demandons QTVODm2 de rouvrir le même fichier mais avec
										 * un petit changement *)
										descrFichierVOD.channels.pilot := descrFichierVOD.channels.forward;
										msgOpenFile( command, fName, descrFichierVOD );
										SendMessageToNet( s, command, SENDTIMEOUT, FALSE, "TstQTVDSrv-envoi" );
										WriteString("commande 'Open' envoyee"); WriteLn;
										Sleep(3000);
%END (* TESTING *)
										msgQuitQTVOD(command);
										SendMessageToNet( s, command, SENDTIMEOUT, FALSE, "TstQTVDSrv-envoi" );
										WriteString("commande 'Quit' envoyee"); WriteLn;
								END;
								EXIT;
							ELSE (* !finished *)
								(* on n'est pas en train de s'arrêter, donc on peut envoyer
								 * des 'polls' réguliers pour connaître l'heure du lecteur. On fait ça après
								 * le 1e message reçu de QTVODm2 et sinon pas plus souvent qu'une fois toutes
								 * les 5 secondes. NB: msgTimer est mis à jour à d'autres endroits aussi! *)
								IF ( (received = 1 ) OR ((received > 0) AND (GetTimeEx(msgTimer) >= 5000)) )
									AND (NOT currentTimeSubscribed)
									THEN
										msgGetTime( command, TRUE );
										(* MAJ de msgTimer *)
										msgTimer := StartTimeEx();
										IF NOT SendMessageToNet( s, command, 1000, FALSE, "TstQTVDSrv-envoi" )
											THEN
												WriteString("SendMessageToNet failed..."); WriteLn;
%IF %NOT CHAUSSETTE2 %THEN
												Sleep(1000);
%END
										END;
								END;
						END;
					ELSE (* !GetClient *)
						(* même sans client connecté il convient de vérifier s'il faut s'arrêter ... *)
						IF CheckQuit() THEN EXIT; END;
				END;
			END;
		ELSE
			QTils.LogMsg( "Initialisation serveur échoué!" );
	END;

FINALLY

	IF ( (s <> sock_nulle) AND (NOT clientQuit) )
		THEN
				(* si on a une connexion d'un client et ce n'est pas le client qui a pris l'initiative
				 * de quitter, on envoie un (dernier) message qtvod_Quit, et on laisse le temps au client
				 * de le traiter *)
				msgQuitQTVOD(command);
				SendMessageToNet( s, command, 1000, FALSE, "TstQTVDSrv-envoi-finalQuit" );
				Sleep(3000);
	END;
	CloseCommServeur(srv, s);

END TstQTVDSrv.
