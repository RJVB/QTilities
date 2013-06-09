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
FROM COMMDLG IMPORT
	OPENFILENAME, GetOpenFileName, OFN_FILEMUSTEXIST, OFN_NOCHANGEDIR, OFN_EXPLORER, OFN_ENABLETEMPLATE;

FROM ElapsedTime IMPORT
	StartTimeEx, GetTimeEx;

FROM QTVODcomm IMPORT *;
FROM QTilsM2 IMPORT
	UInt32, Real64, ErrCode, noErr, QTils, MovieFrameTime, PostMessage;
FROM POSIXm2 IMPORT *;

VAR
	c : CHAR;
	srv, s : SOCK;
	receipt, reply, command : NetMessage;
	finished, clientQuit, sendNewFileName, currentTimeSubscribed : BOOLEAN;
	received, sent : UInt32;
	msgTimer, clntTimer, tAttente : CARDINAL32;

	fName : URLString;
	descrFichierVOD : VODDescription;
	startTimeAbs, Duration, prevCurrentTime : Real64;
	str : URLString;

	ArgsChan : ChanId;

PROCEDURE CheckQuit() : BOOLEAN;
BEGIN
	IF CharAvail()
		THEN
			Read(c);
			CASE c OF
				'q', 'Q' :
					RETURN TRUE;
				| 's', 'S' :
					IF (NOT currentTimeSubscribed)
						THEN
							msgGetTimeSubscription( command, 1.5, FALSE );
							currentTimeSubscribed := TRUE;
						ELSE
							msgGetTimeSubscription( command, 0.0, FALSE );
							currentTimeSubscribed := FALSE;
					END;
					SendMessageToNet( s, command, SENDTIMEOUT, FALSE, "TstQTVDSrv-envoi" );
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
	reply2, reply3 : NetMessage;
	txt : ARRAY[0..127] OF CHAR;
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
	reply3.flags.type := qtvod_NoType;

	CASE msg.flags.type OF

		qtvod_Close :
				WriteString( QTils.lastSSLogMsg^ ) ; WriteLn;

		| qtvod_Quit :
				WriteString( QTils.lastSSLogMsg^ ) ; WriteLn;
				WriteString( "Exit!" ) ; WriteLn;
				finished := TRUE;
				clientQuit := TRUE;
				PostMessage( "TstQTVDSrv", "Player client is quitting, so are we" );

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
				msgGetChapter(reply3, -1);

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
				WriteString( QTils.lastSSLogMsg^ );
				QTils.sprintf( txt, " dt=%gs", msg.data.val1 - prevCurrentTime );
				WriteString(txt);
				WriteLn;
				prevCurrentTime := msg.data.val1;
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

		| qtvod_Chapter :
				NetMessageToLogMsg( "Chapitre", "TstQTVDSrv", msg );
				WriteString( QTils.lastSSLogMsg^ ) ; WriteLn;

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
	IF ( reply3.flags.type <> qtvod_NoType )
		THEN
			SendMessageToNet( s, reply3, SENDTIMEOUT, FALSE, "TraiteMsgDeQTVODm2" );
	END;

	RETURN msg.data.error;
END TraiteMsgDeQTVODm2;

PROCEDURE GetClient( VAR s : SOCK ) : BOOLEAN;
VAR
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
					tAttente := GetTimeEx(clntTimer);
					ts := VAL(Real64,tAttente)/1000.0;
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
	prevCurrentTime := 0.0;
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
(*
	QTils.sprintf( str, "qtvod_NoClass=%d qtvod_Command=%d qtvod_NoType=%d qtvod_Open=%d qtvod_Quit=%d size NMC,NMT=%d,%d\r\n",
		 qtvod_NoClass, qtvod_Command, qtvod_NoType, qtvod_Open, qtvod_Quit,
		 VAL(INTEGER,SIZE(NetMessageClass)), VAL(INTEGER,SIZE(NetMessageType)) );
	WriteString( str );
	QTils.sprintf( str, "size Str64=%d URLString=%d VODDescription=%d VODChannels=%d ErrCode=%d NetMessage=%d\r\n",
		VAL(INTEGER,SIZE(Str64)), VAL(INTEGER,SIZE(URLString)),
		VAL(INTEGER,SIZE(VODDescription)), VAL(INTEGER,SIZE(descrFichierVOD.channels)), VAL(INTEGER,SIZE(ErrCode)), VAL(INTEGER,SIZE(NetMessage)) );
	WriteString( str );
	QTils.sprintf( str, "@freq,scale,timeZone=%lu,%lu,%lu @@DST,useVMGI,log,flip=%lu,%lu,%lu,%lu @channels=%lu,%lu,%lu,%lu @codec,bitrate=%lu,%lu @split=%lu\r\n",
		CAST(UInt32,ADR(descrFichierVOD.frequency))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.scale))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.timeZone))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.DST))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.useVMGI))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.log))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.flipLeftRight))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.channels.forward))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.channels.pilot))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.channels.left))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.channels.right))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.codec))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.bitRate))-CAST(UInt32,ADR(descrFichierVOD)),
		CAST(UInt32,ADR(descrFichierVOD.splitQuad))-CAST(UInt32,ADR(descrFichierVOD)) );
	WriteString( str );
	QTils.sprintf( str, "@size=%lu @protocol=%lu @f.type,class=%lu,%lu @d.val1,val2=%lu,%lu @d.iVal1,boolean=%lu,%lu @d.URN=%lu @d.descr=%lu @d.error=%lu\r\n",
		CAST(UInt32,ADR(reply.size))-CAST(UInt32,ADR(reply)),
		CAST(UInt32,ADR(reply.protocol))-CAST(UInt32,ADR(reply)),
		CAST(UInt32,ADR(reply.flags.type))-CAST(UInt32,ADR(reply)),
		CAST(UInt32,ADR(reply.flags.class))-CAST(UInt32,ADR(reply)),
		CAST(UInt32,ADR(reply.data.val1))-CAST(UInt32,ADR(reply)),
		CAST(UInt32,ADR(reply.data.val2))-CAST(UInt32,ADR(reply)),
		CAST(UInt32,ADR(reply.data.iVal1))-CAST(UInt32,ADR(reply)),
		CAST(UInt32,ADR(reply.data.boolean))-CAST(UInt32,ADR(reply)),
		CAST(UInt32,ADR(reply.data.URN))-CAST(UInt32,ADR(reply)),
		CAST(UInt32,ADR(reply.data.description))-CAST(UInt32,ADR(reply)),
		CAST(UInt32,ADR(reply.data.error))-CAST(UInt32,ADR(reply)) );
	WriteString( str );
*)
	IF InitCommServer(srv, ServerPortNr)
		THEN
			clntTimer := StartTimeEx();
			WriteString( "Serveur lance... (q/Q pour sortir)" ); WriteLn;
			QTils.LogMsg( "Serveur lancé...\n(q/Q pour sortir)\n" );
			IF LENGTH(fName) = 0
				THEN
					GetFileName( "Merci de choisir un fichier vidéo", fName );
			END;
			IF ( LENGTH(fName) > 0 )
				THEN
					LaunchQTVODm2( ".", fName, "-debugWait", ADR(descrFichierVOD), "127.0.0.1", sendNewFileName );
				ELSE
					sendNewFileName := FALSE;
			END;
			IF ( GetClient(s) )
				THEN
					IF (NOT currentTimeSubscribed) AND (Duration >= 0.0)
						THEN
							msgGetTimeSubscription( command, 1.5, FALSE );
							SendMessageToNet( s, command, SENDTIMEOUT, FALSE, "TstQTVDSrv-envoi" );
							currentTimeSubscribed := TRUE;
					END;
			END;
			LOOP
				IF ( GetClient(s) )
					THEN
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
										Sleep(tAttente);
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
