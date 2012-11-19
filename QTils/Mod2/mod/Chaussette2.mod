IMPLEMENTATION MODULE Chaussette2;

<*/VALIDVERSION:CHAUSSETTE2,GDOS*>

(*
	Procédures présentes dans ce module :
		RecupErreurOS
		EcritMsg
		FinIP
		MajEtatS
		EstDansEtatS
		FermeSock
		FermeServeur
		TestEtat
		InitDate
		DaterPaquet
		BasicSendFrameUDP
		SendShortNetMessageUDP
		SendNetMessage
		BasicSendNetMessage
		BasicReceiveFrameUDP
		ReceiveShortNetMessageUDP
		ReceiveNetMessage
		BasicReceiveNetMessage
		EtabliClientUDPouTCP
		EtabliClient
		ConnexionAuServeur
		EtabliServeur
		EtabliServeurUDPouTCP
		AttenteConnexionDunClient
		InitIP
		NumeroIPMachineDistante
		NumeroIPMachineLocale
		NumeroIPMachine
		EstDistante
		(** IndTs **)
*)

FROM Terminal IMPORT
	CharAvail, Read;

FROM TypeConv IMPORT
	infini_i16,
	Int16, Card16, Int32, Card32;

FROM Strings IMPORT
	Length, Compare, equal;

FROM WholeStr IMPORT
	StrToCard, IntToStr, ConvResults;

FROM SYSTEM IMPORT
%IF %NOT StonyBrook %THEN
	ADDRESS,
%END
	CAST,
	ADR, BYTE, WORD, DWORD, TSIZE;

%IF WIN32 %THEN
	FROM WINX IMPORT
		NULL_HWND;
	FROM WIN32 IMPORT
		BOOL, WINT(*, LPSTR*);
	FROM WINUSER IMPORT
		MessageBox, MB_OK;

(*
%IF SB49 %THEN
*)
	FROM WINSOCK2 IMPORT
(*
		__WSAFDIsSet, timeval, select,
*)
(*
%ELSE
	FROM WINSOCK IMPORT
%END
*)
		WSADATA, WSAGetLastError, WSAStartup, WSACleanup,
		setsockopt, SOL_SOCKET, SO_LINGER, SO_REUSEADDR,
		WSAEWOULDBLOCK, WSAEISCONN, WSAECONNREFUSED,
		FIONBIO;
%ELSE
	FROM Terminal IMPORT
		WriteString, WriteLn;
%END
%IF LINUX %THEN
IMPORT
	UNIX;
%END

FROM ElapsedTime IMPORT
	Delay, GetTime;

IMPORT Socket;

(*
FROM QTilsM2 IMPORT
	QTils, BoolStr;
*)

%IF %NOT WIN16 %THEN

TYPE

	IP_CARD16 = ARRAY[0..1] OF CARDINAL16;

VAR

	ts : ARRAY [0..max_sock] OF
		RECORD
			s : SOCK;
			e : ETAT_SOCK;
		END;
	estInitialise : BOOLEAN;
	i : Int16;
	dateUDP, dtUDP, tempsUDPOkPred : CARDINAL32;

(*--------------------------------------------------------------------------*)

PROCEDURE RecupErreurOS() : INTEGER;
VAR
	err : INTEGER;
BEGIN
%IF GDOS %OR WIN16 %THEN
			err := GetErrNo();
%END
%IF WIN32 %THEN
			err := WSAGetLastError();
%END
%IF SGI %OR GPLINUX %THEN
			err := errno;
%END
%IF LINUX %THEN
			err := UNIX.errno();
%END
	RETURN err
END RecupErreurOS;

(*
	mise à jour de la valeur erreur errCode ... uniquement quand il y a un code d'erreur
	(pour ce coup, 0 est un code de succès, pas d'erreur...!)
*)
PROCEDURE MajErreurOS(VAR errCode : INTEGER) : BOOLEAN;
VAR
	err : INTEGER;
BEGIN
	err := RecupErreurOS();
	IF err <> 0
		THEN
			errCode := err;
			RETURN TRUE;
		ELSE
			RETURN FALSE;
	END;
END MajErreurOS;

PROCEDURE EcritMsg(ch : ARRAY OF CHAR; err : Int32);
VAR
	chErr : ARRAY[0..5+1] OF CHAR;
%IF WIN16 %OR WIN32 %THEN
	r : INTEGER;
%END
BEGIN
	IF err = -1
		THEN
			err := RecupErreurOS()
	END;
	IntToStr(err, chErr);

%IF WIN16 %OR WIN32 %THEN
	r := MessageBox(NULL_HWND, chErr, ch, MB_OK);
%ELSE
%IF LINUX %THEN
	UNIX.perror("");
%ELSE
	WriteString(ch); WriteString(" ");
	WriteString(chErr);
	WriteLn
%END
%END
END EcritMsg;

(*--------------------------------------------------------------------------*)

PROCEDURE FinIP();
VAR
	e : Int16;
BEGIN
	FOR e := 0 TO max_sock DO
		IF ouverte IN ts[e].e
			THEN
				SockClose(ts[e].s);
				ts[e].e := ETAT_SOCK{};
				EcritMsg('Erreur fermeture socket', ts[e].s)
		END
	END;
%IF WIN32 %THEN
	IF estInitialise
		THEN
			IF WSACleanup() # 0
				THEN
					EcritMsg('PB FinIP', 0)
				ELSE
					estInitialise := FALSE
			END;
	END
%END
END FinIP;

PROCEDURE MajEtatS(s : SOCK; nvelEtat : ETAT_SOCK);
VAR
	i : Int16;
BEGIN
	FOR i := 0 TO max_sock DO
		IF ts[i].s = s
			THEN ts[i].e := nvelEtat; RETURN
		END
	END;
	FOR i := 0 TO max_sock DO
		IF ts[i].s = sock_nulle
			THEN
				ts[i].s := s;
				ts[i].e := nvelEtat;
				RETURN
		END
	END
END MajEtatS;

PROCEDURE EstDsEtatS(s : SOCK; ee : ETATS) : BOOLEAN;
VAR
	i : Int16;
BEGIN
	FOR i := 0 TO max_sock DO
		IF (ts[i].s = s)
			THEN
				RETURN (ee IN ts[i].e)
		END
	END;
	RETURN FALSE
END EstDsEtatS;

PROCEDURE FermeSock(VAR s : SOCK);
VAR
	b : BOOLEAN;
%IF WIN16 %OR WIN32 %THEN
	res : WINT;
	lin : RECORD l_onoff, l_linger : Int16 END;
%END
BEGIN
	b := Socket.Shutdown(s, Socket.SHUT_RDWR);
%IF WIN16 %OR WIN32 %THEN
	lin.l_onoff := 1; lin.l_linger := 0;
	res := setsockopt(s, SOL_SOCKET, SO_LINGER, lin, SIZE(lin));
%END
%IF LINUX %THEN
	(* Socket.sockclose met uniquement sock à - 1 mais ne ferme pas !! *)
	b := (UNIX.close(s) # -1);
%END
	SockClose(s);
	errSock := RecupErreurOS();
(*
	IF (errSock # 38) AND (errSock # 10038)
		THEN
			EcritMsg('Fermeture socket', -1)
	END;
*)
	MajEtatS(s, ETAT_SOCK{});
	s := sock_nulle
END FermeSock;

(*--------------------------------------------------------------------------*)

PROCEDURE FermeServeur(
	VAR s : SOCK);
BEGIN
	IF (s # sock_nulle) AND (EstDsEtatS(s, ouverte) OR EstDsEtatS(s, udp))
		THEN
			(*SendNetMessage(s, ADR(s), 0);*)
			FermeSock(s)
	END
END FermeServeur;

(*--------------------------------------------------------------------------*)

VAR
	fd_set_vide : Socket.fd_set;

PROCEDURE TestEtat(s : SOCK; testR, testW, testE : BOOLEAN; VAR R, W, E : BOOLEAN; timeOutms : INTEGER);
VAR
	trouve : INTEGER;
	readFdsE, writeFdsE, exceptFdsE,
	readFds, writeFds, exceptFds : Socket.fd_set;
%IF %NOT StonyBrook %THEN
	temps : timeval;
%END
BEGIN
%IF StonyBrook %THEN
	(* remise à 0 de tous les sockets demandés *)
	(* FD_ZERO est un macro qui ne touche que le membre fd_count, donc est plus rapide
	 * qu'une initialisation avec fd_set_vide *)
	Socket.FD_ZERO(readFdsE); Socket.FD_ZERO(writeFdsE); Socket.FD_ZERO(exceptFdsE);
	(* readFdsE := fd_set_vide; writeFdsE := fd_set_vide; exceptFdsE := fd_set_vide; *)
	(* on demande seulement "notre" socket spécifique! *)
	(* 20110114: pour que le timeOut ait un sens, il faut demander uniquement l'état
	 * qui nous intéresse surtout sinon il y aura interférence entre les états
	 * (on ne veut pas savoir toute de suite si on peut écrire quand on veut lire, par exemple) *)
	IF testR THEN Socket.FD_SET(s, readFdsE); END;
	IF testW THEN Socket.FD_SET(s, writeFdsE); END;
	IF testE THEN Socket.FD_SET(s, exceptFdsE); END;
	trouve := Socket.Select2(readFdsE, writeFdsE, exceptFdsE,
		readFds, writeFds, exceptFds, timeOutms);
(*
	%IF SB44 %OR SB49 %THEN
	temps.tv_sec := 0; temps.tv_usec := 0;
	trouve := select(32, readFdsE, writeFdsE, exceptFdsE,
		temps);
	r := (__WSAFDIsSet(s, readFds) # 0);
	w := (__WSAFDIsSet(s, writeFds) # 0);
	e := (__WSAFDIsSet(s, exceptFds) # 0);
	%ELSE
*)
	IF testR THEN R := Socket.FD_ISSET(s, readFds); END;
	IF testW THEN W := Socket.FD_ISSET(s, writeFds); END;
	IF testE THEN E := Socket.FD_ISSET(s, exceptFds); END;
(*
	%END
*)
%ELSE
	IF ( timeOutms >= 0 )
		THEN
			temps.tvSec := timeOutms/1000; temps.tvUsec := (timeOutms - temps.tvSec*1000) * 1000;
		ELSE
			(* timeOutms < 0 veut dire attente indéfinie. Si Select se comporte comme son homologue en C,
			 * il faudrait alors passer NIL au lieu de temps, mais on va simuler cette fonctionnalité en
			 * spécifiant une durée maximale *)
			temps.tvSec := 7fFFFFFFh; temps.tvUsec := 7FFFFFFFh;
	END;
	(* A VERIFIER!! *)
	(* readFds := BITSET{0..max_sock}; writeFds := BITSET{0..max_sock}; exceptFds := BITSET{0..max_sock}; *)
	readFds := fd_set_vide; writeFds := fd_set_vide; exceptFds := fd_set_vide;
	trouve := Select(max_sock+1, ADR(readFds), ADR(writeFds), ADR(exceptFds), temps);
	r := ORD(s) IN readFds; w := ORD(s) IN writeFds; e := ORD(s) IN exceptFds
%END
END TestEtat;

(*--------------------------------------------------------------------------*)

(*
	Initialisation de la date.
*)
PROCEDURE InitDate();
BEGIN
	dateUDP := 0;
END InitDate;

(*
	récupération de la date et incrémentation afin de donner une autre
	"date" au paquet suivant.
*)
PROCEDURE DaterPaquet():CARDINAL32;
VAR
	res : CARDINAL32;
BEGIN
	res := dateUDP;
	INC(dateUDP);
	RETURN res
END DaterPaquet;

(*
	message <= à la trame ! (1450 octets)
	la datation est à la charge de l'envoyeur

	msg doit etre une stucture ayant 8 octets de libre au début.
	2 pour la taille du message, 2 pour l'identifiant de l'envoyeur,
	et 4 pour la date du message.
	Avant l'envoie le message est "daté". On laisse la gestion des paquets
	en fonction de la date à l'utilisateur.

	ip : ip où l'on veut envoyer le message.
	port : port vers lequel on veut envoyer le message.
	lTotal : taille du message (en prenant en compte les premiers 8 octets).
	lSrvce : taille de la partie service du message (8).

	Si on veut faire du BROADCAST il faut se limiter à son sous réseau.
	par exemple, si l'ip source est 137.121.2.175 il faudra envoyer au
	maximun a 137.121.255.255 (toutes les ips commençant par "137.121.")
	ou a 137.121.2.255 (toutes les ips commençant par "137.121.2.").
*)
PROCEDURE BasicSendFrameUDP(
	s : SOCK;
	VAR msg : ARRAY OF BYTE; lMsg : Int16; timeOutms : INTEGER;
	bloquant : BOOLEAN; port : CARDINAL16; ip : DWORD) : BOOLEAN;
VAR
	res : INTEGER;
	r, w, e : BOOLEAN;
BEGIN
(*
	IF (lMsg > 1450)
		THEN
			EcritMsg('message trop grand (limite à 1450 octets)', lMsg);
			RETURN FALSE
	END;
*)
	IF (s = sock_nulle) OR NOT EstDsEtatS(s, udp) THEN RETURN FALSE END;
	carChaussette := ' ';
	REPEAT
		TestEtat(s, FALSE, TRUE, FALSE, r, w, e, timeOutms);
		IF NOT w
			THEN
				IF NOT bloquant THEN RETURN FALSE END;
%IF %NOT (SGI) %THEN
				IF Sortie(33C, carChaussette) THEN RETURN FALSE END
%END
		END
	UNTIL w;

	res := SendTo(s, msg, lMsg, Socket.AF_INET, port, ip);
	IF res < 0
		THEN
			errSock := RecupErreurOS();
			(* errno contient la description de l'erreur *)
%IF %NOT (WIN32 %OR WIN16) %THEN
			IF (errSock # EWOULDBLOCK)
%ELSE
			IF (errSock # WSAEWOULDBLOCK)
%END
				THEN
					(* EcritMsg( "erreur émission", errSock); *)
					RETURN FALSE
			END
	END;
	RETURN TRUE
END BasicSendFrameUDP;

(*--------------------------------------------------------------------------*)

(*
	msg doit etre une stucture ayant 8 octets de libre au début.
	2 pour la taille du message, 2 pour l'identifiant de l'envoyeur,
	et 4 pour la date du message.
	Avant l'envoie le message est "daté". On laisse la gestion des paquets
	en fonction de la date à l'utilisateur.

	ip : ip où l'on veut envoyer le message.
	port : port vers lequel on veut envoyer le message.
	lTotal : taille du message (en prenant en compte les premiers 8 octets).
	lSrvce : taille de la partie service du message (8).

	Si on veut faire du BROADCAST il faut se limiter à son sous réseau.
	par exemple, si l'ip source est 137.121.2.175 il faudra envoyer au
	maximun a 137.121.255.255 (toutes les ips commençant par "137.121.")
	ou a 137.121.2.255 (toutes les ips commençant par "137.121.2.").
*)
PROCEDURE SendShortNetMessageUDP(
	s : SOCK;
	VAR msg : ARRAY OF BYTE; lSrvce, lTotal : Int16; timeOutms : INTEGER;
	bloquant : BOOLEAN; port : CARDINAL16; ip : DWORD) : BOOLEAN;
VAR
	res : INTEGER;
	r, w, e : BOOLEAN;
BEGIN
	IF (lSrvce MOD 4) # 0
		THEN
			EcritMsg('partie service du message incompatible (non multiple de 4)', lSrvce);
			RETURN FALSE
	END;
	IF (lTotal > 550)
		THEN
			EcritMsg('message trop grand (limite à 550 octets)', lTotal);
			RETURN FALSE
	END;
	IF (s = sock_nulle) OR NOT EstDsEtatS(s, udp) THEN RETURN FALSE END;
	carChaussette := ' ';
	msg:SERVICE_UDP.date := DaterPaquet();
	msg:SERVICE_UDP.taille := lTotal;
	REPEAT
		TestEtat(s, FALSE, TRUE, FALSE, r, w, e, timeOutms);
		IF NOT w
			THEN
				IF NOT bloquant THEN RETURN FALSE END;
%IF %NOT (SGI) %THEN
				IF Sortie(33C, carChaussette) THEN RETURN FALSE END
%END
		END
	UNTIL w;

	res := SendTo( s, msg, lTotal, Socket.AF_INET, port, ip);
	IF res < 0
		THEN
			errSock := RecupErreurOS();
			(* errno contient la description de l'erreur *)
%IF %NOT (WIN32 %OR WIN16) %THEN
			IF (errSock # EWOULDBLOCK)
%ELSE
			IF (errSock # WSAEWOULDBLOCK)
%END
				THEN
					(* EcritMsg( "erreur émission", errSock); *)
					RETURN FALSE
			END
	END;
	RETURN TRUE
END SendShortNetMessageUDP;

(*--------------------------------------------------------------------------*)

(*
	le premier mot (16b) de srvce contient la lg totale : srvce + msg
	ecrit msg pour une taille lTotal
*)
PROCEDURE SendNetMessage(
	s : SOCK;
	VAR msg : ARRAY OF BYTE; lSrvce, lTotal : Int16; timeOutms : INTEGER;
	 bloquant : BOOLEAN) : BOOLEAN;
VAR
	res, idxEnvoi : INTEGER;
	r, w, e : BOOLEAN;
BEGIN
	IF (lSrvce MOD 4) # 0
		THEN
			EcritMsg('partie service du message incompatible (non multiple de 4)', lSrvce);
			RETURN FALSE
	END;
	IF (s = sock_nulle) OR NOT EstDsEtatS(s, connecte) THEN RETURN FALSE END;
	carChaussette := ' '; idxEnvoi := 0;
	LOOP
		REPEAT
			TestEtat(s, FALSE, TRUE, FALSE, r, w, e, timeOutms);
			IF NOT w
				THEN
					IF NOT bloquant THEN RETURN FALSE END;
%IF %NOT (SGI) %THEN
					IF Sortie(33C, carChaussette) THEN RETURN FALSE END
%END
			END
		UNTIL w;
(*
	Essai de découpe des messages. Mais sur SGI cela provoque BEAUCOUP
	de sacade sur le visuel!
	En fait la fonction de réception qui reconstruit le message doit
	bloquer temporairement...
*)
(*
		IF lTotal > TAILLEMAXMSG
			THEN
				res := Send( s, msg[idxEnvoi], TAILLEMAXMSG);
			ELSE
				res := Send( s, msg[idxEnvoi], lTotal);
		END;
*)
				res := Send( s, msg[idxEnvoi], lTotal);
		IF res < 0
			THEN
				errSock := RecupErreurOS();
				(* errno contient la description de l'erreur *)
%IF %NOT (WIN32 %OR WIN16) %THEN
				IF (errSock # EWOULDBLOCK)
%ELSE
				IF (errSock # WSAEWOULDBLOCK)
%END
					THEN
						(* EcritMsg( "erreur émission", errSock); *)
						RETURN FALSE
				END
			ELSE
				DEC(lTotal, res);
				IF lTotal <= 0
					THEN
						EXIT
				END;
				INC(idxEnvoi, res)
		END
	END (* loop *);
	RETURN TRUE
END SendNetMessage;

PROCEDURE BasicSendNetMessage(
	s : SOCK;
	VAR msg : ARRAY OF BYTE; lMsg : Int16; timeOutms : INTEGER;
	 bloquant : BOOLEAN) : BOOLEAN;
VAR
	res, idxEnvoi : INTEGER;
	r, w, e : BOOLEAN;
BEGIN
	IF (s = sock_nulle) OR NOT EstDsEtatS(s, connecte) THEN RETURN FALSE END;
	carChaussette := ' '; idxEnvoi := 0;
	LOOP
		REPEAT
			TestEtat(s, FALSE, TRUE, FALSE, r, w, e, timeOutms);
			IF NOT w
				THEN
					IF NOT bloquant THEN RETURN FALSE END;
					IF Sortie(33C, carChaussette) THEN RETURN FALSE END
			END
		UNTIL w;

(*
	Essai de découpe des messages. Mais sur SGI cela provoque BEAUCOUP
	de sacade sur le visuel!
	En fait la fonction de réception qui reconstruit le message doit
	bloquer temporairement...
*)
(*
		IF lMsg > TAILLEMAXMSG
			THEN
				res := Send( s, msg[idxEnvoi], TAILLEMAXMSG);
			ELSE
				res := Send( s, msg[idxEnvoi], lMsg);
		END;
*)
		res := Send( s, msg[idxEnvoi], lMsg);
		IF res < 0
			THEN
				errSock := RecupErreurOS();
				(* errno contient la description de l'erreur *)
%IF %NOT (WIN32 %OR WIN16) %THEN
				IF (errSock # EWOULDBLOCK)
%ELSE
				IF (errSock # WSAEWOULDBLOCK)
%END
					THEN
(*
						EcritMsg( "erreur émission", errSock);
*)
						RETURN FALSE
				END
			ELSE
				DEC(lMsg, res);
				IF lMsg <= 0
					THEN
						EXIT
				END;
				INC(idxEnvoi, res)
		END
	END (* loop *);
	RETURN TRUE
END BasicSendNetMessage;

(*--------------------------------------------------------------------------*)

(*
	message <= à la trame ! (1450 octets)
	la gestion de la datation est à la charge du receveur
	SORTIE
		msg
		portDEnvoi
		ipEnvoi
		Renvoie TRUE si lMsg = lReçue
*)
PROCEDURE BasicReceiveFrameUDP(
	s : SOCK;
	VAR msg : ARRAY OF BYTE; lMsg : INTEGER;
	timeOutms : INTEGER; bloquant : BOOLEAN;
	VAR portDEnvoi : CARDINAL16; VAR ipDEnvoi : DWORD) : BOOLEAN;
VAR
	r, w, e : BOOLEAN;
	len, longMax : INTEGER;
BEGIN
	IF (s = sock_nulle) OR NOT EstDsEtatS(s, udp) THEN RETURN FALSE END;
	carChaussette := ' ';
	longMax := lMsg;
	LOOP
		TestEtat(s, TRUE, FALSE, FALSE, r, w, e, timeOutms);
		IF r
			THEN EXIT
		END;
		IF NOT bloquant THEN RETURN FALSE	END;
%IF %NOT (SGI) %THEN
		IF Sortie(33C, carChaussette) THEN RETURN FALSE END
%END
	END (* loop *);
	len := RecvFrom(s, msg, longMax, portDEnvoi, ipDEnvoi);

	IF (len < 0)
		THEN
			errSock := RecupErreurOS();
%IF %NOT (WIN32 %OR WIN16) %THEN
			IF (errSock # EWOULDBLOCK)
%ELSE
			IF (errSock # WSAEWOULDBLOCK)
%END
				THEN
(*
					EcritMsg("erreur réception : ", -1);
*)
					RETURN FALSE
			END
		ELSIF ( len > 0 )
			THEN (* if any, show it *)
				RETURN len = lMsg
			ELSE (* len = 0 => fin de la connection *)
				FermeSock(s);
				carChaussette := 33C;
				RETURN FALSE
	END;
%IF %NOT (SGI) %THEN
	IF Sortie(33C, carChaussette) THEN RETURN FALSE END;
%END
	RETURN TRUE
END BasicReceiveFrameUDP;

(*
	le premier mot (16b) de msg contient la lg totale : srvce + msg
	le deuxième mot (16b) sera l'identifiant de l'envoyeur
	les deux suivants (32b) représentent la "date" du message

	à la charge de l'utilisateur de gérer les messages qui pourraient
	être périmés.
*)
PROCEDURE ReceiveShortNetMessageUDP(
	s : SOCK;
	VAR msg : ARRAY OF BYTE;
	timeOutms : INTEGER; bloquant : BOOLEAN) : BOOLEAN;
VAR
	r, w, e : BOOLEAN;
	len, longMax : INTEGER;
	port : CARDINAL16;
	ip : DWORD;
BEGIN
	IF (s = sock_nulle) OR NOT EstDsEtatS(s, udp) THEN RETURN FALSE END;
	carChaussette := ' ';
	longMax := 550;
	LOOP
		TestEtat(s, TRUE, FALSE, FALSE, r, w, e, timeOutms);
		IF r
			THEN EXIT
		END;
		IF NOT bloquant THEN RETURN FALSE	END;
%IF %NOT (SGI) %THEN
		IF Sortie(33C, carChaussette) THEN RETURN FALSE END
%END
	END (* loop *);
	len := RecvFrom( s, msg, longMax, port, ip);

	IF (len < 0)
		THEN
			errSock := RecupErreurOS();
%IF %NOT (WIN32 %OR WIN16) %THEN
			IF (errSock # EWOULDBLOCK)
%ELSE
			IF (errSock # WSAEWOULDBLOCK)
%END
				THEN
(*
					EcritMsg("erreur réception : ", -1);
*)
					RETURN FALSE
			END
		ELSIF (len > 0 )
			THEN (* if any, show it *)
				msg:SERVICE_UDP.adr := ip:IP_CARD16[0];
			ELSE (* len = 0 => fin de la connection *)
				FermeSock(s);
				carChaussette := 33C;
				RETURN FALSE
	END;
%IF %NOT (SGI) %THEN
	IF Sortie(33C, carChaussette) THEN RETURN FALSE END;
%END
	msg:SERVICE_UDP.adr := ip:IP_CARD16[0];
	msg:SERVICE_UDP.taille := VAL(Int16, len);
	RETURN TRUE
END ReceiveShortNetMessageUDP;

PROCEDURE ReceiveShortNetMessageUDPTime(
	s : SOCK;
	VAR msg : ARRAY OF BYTE;
	bloquant : BOOLEAN;
	tempsAttente : Card32) : BOOLEAN;
VAR
	r, w, e : BOOLEAN;
	len, longMax : INTEGER;
	port : CARDINAL16;
	ip : DWORD;
	tempsUDPOk : Card32;
BEGIN
	IF (s = sock_nulle) OR NOT EstDsEtatS(s, udp) THEN RETURN FALSE END;
	carChaussette := ' ';
	longMax := 550;
	LOOP
		TestEtat(s, TRUE, FALSE, FALSE, r, w, e, ABS(VAL(INTEGER,tempsAttente)) );
		IF r
			THEN
				tempsUDPOk := GetTime();
				dtUDP := tempsUDPOk - tempsUDPOkPred;
				tempsUDPOkPred := tempsUDPOk;
				EXIT
		END;
		IF NOT bloquant THEN RETURN FALSE	END;
%IF %NOT (SGI) %THEN
		IF Sortie(33C, carChaussette) THEN RETURN FALSE END;
%END
		IF ((GetTime() - tempsUDPOkPred) > 2 * dtUDP) OR
				((GetTime() - tempsUDPOkPred) > tempsAttente)
			THEN
				RETURN FALSE
		END;
	END (* loop *);
	len := RecvFrom( s, msg, longMax, port, ip);

	IF (len < 0)
		THEN
			errSock := RecupErreurOS();
%IF %NOT (WIN32 %OR WIN16) %THEN
			IF (errSock # EWOULDBLOCK)
%ELSE
			IF (errSock # WSAEWOULDBLOCK)
%END
				THEN
(*
					EcritMsg("erreur réception : ", -1);
*)
					RETURN FALSE
			END
		ELSIF (len > 0 )
			THEN (* if any, show it *)
				msg:SERVICE_UDP.adr := ip:IP_CARD16[0];
			ELSE (* len = 0 => fin de la connection *)
				FermeSock(s);
				carChaussette := 33C;
				RETURN FALSE
	END;
%IF %NOT (SGI) %THEN
	IF Sortie(33C, carChaussette) THEN RETURN FALSE END;
%END
	msg:SERVICE_UDP.adr := ip:IP_CARD16[0];
	msg:SERVICE_UDP.taille := VAL(Int16, len);
	RETURN TRUE
END ReceiveShortNetMessageUDPTime;

(*--------------------------------------------------------------------------*)

(*
	gestion de messages découpés
	le premier mot (16b) de srvce contient la lg totale : srvce + msg
	lit msg pour une taille MIN(lg totale - lg srvce, HIGH(msg) + 1)
*)
PROCEDURE ReceiveNetMessage(
	s : SOCK;
	VAR srvce : ARRAY OF WORD; VAR msg : ARRAY OF BYTE;
	timeOutms : INTEGER; bloquant : BOOLEAN) : BOOLEAN;
(*		le premier mot (16b) de srvce contient la lg totale : srvce + msg		 *)
VAR
	deb, r, w, e : BOOLEAN;
	l, len, long, longMax : INTEGER;
BEGIN
	IF (s = sock_nulle) OR NOT EstDsEtatS(s, connecte) THEN RETURN FALSE END;
	carChaussette := ' ';
	long := infini_i16; l := 0; longMax := HIGH(msg) + 1; deb := TRUE;
	WHILE long > 0 DO
		LOOP
			TestEtat(s, TRUE, FALSE, FALSE, r, w, e, timeOutms);
			IF r
				THEN EXIT
			END;
			IF NOT bloquant THEN RETURN FALSE	END;
%IF %NOT (SGI) %THEN
			IF Sortie(33C, carChaussette) THEN RETURN FALSE END
%END
		END (* loop *);

		(* now try to receive the echo back *)
		IF long = infini_i16
			THEN
				len := Recv(s, srvce, SIZE(srvce));
				long := INT(srvce[0])
			ELSE
				len := Recv(s, msg[l], longMax);
		END;
		IF (len < 0)
			THEN
				errSock := RecupErreurOS();
%IF %NOT (WIN32 %OR WIN16) %THEN
				IF (errSock # EWOULDBLOCK)
%ELSE
				IF (errSock # WSAEWOULDBLOCK)
%END
					THEN
(*
						EcritMsg("erreur réception : ", -1);
*)
						RETURN FALSE
				END
			ELSIF ( len > 0 )
				THEN (* if any, show it *)
					IF deb
						THEN
							deb := FALSE;
							IF longMax > long
								THEN
									longMax := long - len
							END
						ELSE
							INC(l, len); DEC(longMax, len)
					END;
					DEC(long, len)
				ELSE (* len = 0 => fin de la connection *)
					FermeSock(s);
					carChaussette := 33C;
					RETURN FALSE
		END;
%IF %NOT (SGI) %THEN
		IF Sortie(33C, carChaussette) THEN RETURN FALSE END
%END
	END (* while *);
	srvce[0] := VAL(Int16, l);
	RETURN TRUE
END ReceiveNetMessage;

(*--------------------------------------------------------------------------*)

(*
	gère la réception par partie ...
*)
PROCEDURE BasicReceiveNetMessage(
	s : SOCK;
	VAR msg : ARRAY OF BYTE; long : CARDINAL;
	timeOutms : INTEGER; bloquant : BOOLEAN)
	: BOOLEAN;
VAR
	res : INTEGER;
	res1 : CARDINAL;
	r, w, e : BOOLEAN;
	ee : Int16;
BEGIN
	res1 := 0; ee := 0;
	REPEAT
		LOOP
			TestEtat(s, TRUE, FALSE, FALSE, r, w, e, timeOutms);
			IF r
				THEN EXIT
			END;
			IF NOT bloquant THEN RETURN FALSE	END;
%IF %NOT (SGI) %THEN
			IF Sortie(33C, carChaussette) THEN RETURN FALSE END;
%END
			(* pour tests ! *)
			IF ee = infini_i16
				THEN
					ee := 0
				ELSE
					INC(ee)
			END;
		END (* loop *);

%IF StonyBrook %THEN
		res := Recv(s, msg[res1],
			long - res1);
%ELSE
		res := Recv(s, ADR(msg[res1]),
			long - res1, 0);
%END
		IF (res < 0)
			THEN
(*
				EcritMsg("erreur réception : ", -1);
*)
				RETURN FALSE
			ELSIF (res > 0 )
				THEN
					INC(res1, VAL(CARDINAL, res))
				ELSE (* res = 0 => fin de la connection *)
					FermeSock(s);
					carChaussette := 33C;
					RETURN FALSE
		END;
%IF %NOT (SGI) %THEN
		IF Sortie(33C, carChaussette) THEN RETURN FALSE END
%END
	UNTIL res1 = long;
	RETURN TRUE
END BasicReceiveNetMessage;

(*--------------------------------------------------------------------------*)

(*
	le port est utilisé pour faire un "bind" sauf s'il est passé avec une valeur
	MAX(Card16)
	c'est le cas général car en client on n'a généralement pas à faire de "bind"
*)
PROCEDURE EtabliClientUDPouTCP(
	VAR s : SOCK; port : Card16; modeTCP : BOOLEAN) : BOOLEAN;
VAR
%IF %NOT StonyBrook %THEN
	lsock : SOCKADDR_IN;
%END
	e : INTEGER;
	typeSock : CARDINAL;
%IF WIN16 %OR WIN32 %THEN
	mode : CARDINAL;
%END
	b : BOOLEAN;
BEGIN
	IF modeTCP
		THEN
			typeSock := Socket.SOCK_STREAM
		ELSE
			typeSock := Socket.SOCK_DGRAM
	END;
	IF NOT SockCreate(Socket.AF_INET, typeSock, Socket.PF_UNSPEC, s)
		THEN
			EcritMsg("erreur socket", -1);
			RETURN FALSE
	END;
%IF (WIN32 %OR WIN16) %THEN
	e := setsockopt(s, SOL_SOCKET, SO_REUSEADDR, TRUE, TSIZE(BOOL));
	IF e # 0
		THEN
			errSock := RecupErreurOS()
	END;
%END
	IF port # MAX(Card16)
		THEN
%IF %NOT StonyBrook %THEN
			lsock.sinAddr.a := CAST(ADDRESS, htonl(INADDR_ANY));
			lsock.sinPort := htons(port);
			lsock.sinFamily := AF_INET;
			e := Bind(s, ADR(lsock), TSIZE(SOCKADDR_IN));
			IF e < 0
%ELSE
			b := Bind(s, Socket.AF_INET, port, Socket.INADDR_ANY);
			IF NOT b
%END
				THEN
					EcritMsg("erreur bind", -1);
					FermeSock(s);
					RETURN FALSE
			END
	END;

	(* set non-blocking socket *)
%IF %NOT StonyBrook %THEN
	e := Fcntl( s, F_SETFL, FNDELAY );
	IF ( e = -1 )
%ELSE
%IF WIN32 %THEN
	mode := 1; (* non bloquant si # 0 *)
	e := Fcntl( s, FIONBIO, mode);
%ELSE (* LINUX *)
	e := Fcntl( s, UNIX.F_SETFL, UNIX.O_NONBLOCK );
%END
	IF ( e # 0 )
%END
		THEN
			EcritMsg( "erreur fnctl", e);
			FermeSock(s);
			RETURN FALSE
	END;
	IF modeTCP
		THEN
			MajEtatS(s, ETAT_SOCK{ouverte})
		ELSE
			MajEtatS(s, ETAT_SOCK{udp})
	END;
	RETURN TRUE
END EtabliClientUDPouTCP;

(*--------------------------------------------------------------------------*)

PROCEDURE EtabliClient(
	VAR s : SOCK; port : Card16) : BOOLEAN;
VAR
%IF %NOT StonyBrook %THEN
	lsock : SOCKADDR_IN;
%END
	e : INTEGER;
%IF WIN16 %OR WIN32 %THEN
	mode : CARDINAL;
%END
	b : BOOLEAN;
BEGIN
	IF NOT SockCreate(Socket.AF_INET, Socket.SOCK_STREAM, Socket.PF_UNSPEC, s)
		THEN
			EcritMsg("erreur socket", -1);
			RETURN FALSE
	END;
%IF (WIN32 %OR WIN16) %THEN
	e := setsockopt(s, SOL_SOCKET, SO_REUSEADDR, TRUE, TSIZE(BOOL));
	IF e # 0
		THEN
			errSock := RecupErreurOS()
	END;
%END
	IF port # MAX(Card16)
		THEN
%IF %NOT StonyBrook %THEN
			lsock.sinAddr.a := CAST(ADDRESS, htonl(INADDR_ANY));
			lsock.sinPort := htons(port);
			lsock.sinFamily := AF_INET;
			e := Bind(s, ADR(lsock), TSIZE(SOCKADDR_IN));
			IF e < 0
%ELSE
			b := Bind(s, Socket.AF_INET, port, Socket.INADDR_ANY);
			IF NOT b
%END
				THEN
					EcritMsg("erreur bind", -1);
					FermeSock(s);
					RETURN FALSE
			END
	END;

	(* set non-blocking socket *)
%IF %NOT StonyBrook %THEN
	e := Fcntl( s, F_SETFL, FNDELAY );
	IF ( e = -1 )
%ELSE
%IF WIN32 %THEN
	mode := 1; (* non bloquant si # 0 *)
	e := Fcntl( s, FIONBIO, mode);
%ELSE (* LINUX *)
	e := Fcntl( s, UNIX.F_SETFL, UNIX.O_NONBLOCK );
%END
	IF ( e # 0 )
%END
		THEN
			EcritMsg( "erreur fnctl", e);
			FermeSock(s);
			RETURN FALSE
	END;
	MajEtatS(s, ETAT_SOCK{ouverte});
	RETURN TRUE
END EtabliClient;

(*--------------------------------------------------------------------------*)

PROCEDURE ConnexionAuServeur(
	s : SOCK; port : Card16;
	VAR nom, numeroIP : ARRAY OF CHAR;
	timeOutms : INTEGER; VAR fatale : BOOLEAN) : BOOLEAN;
TYPE
	AA = ARRAY[0..3] OF CHAR;
VAR
	fsock : SOCKADDR_IN;
%IF %NOT StonyBrook %THEN
	ptrEntree : PTR_HOST_ENT;
%END
	i, j, l, n : Card16;
	k : CARDINAL;
	nIP : ARRAY[0..4] OF CHAR;
	fait : ConvResults;
(*
%IF (WIN32 %OR WIN16) %THEN
*)
	e : Int16;
	rd, wr, ee : BOOLEAN;
(*
%END
*)
	b : BOOLEAN;
BEGIN
	IF (s = sock_nulle) OR NOT EstDsEtatS(s, ouverte) THEN fatale := TRUE; RETURN FALSE END;
	IF numeroIP[0] = 0C
		THEN
%IF StonyBrook %THEN
			IF NOT Socket.GetHostAddr(nom, fsock.sinAddr)
				THEN
					RETURN FALSE
			END;
%ELSE
			ptrEntree := CAST(PTR_HOST_ENT, GetHostByName(nom));
			IF ptrEntree = CAST(PTR_HOST_ENT, 0)
				THEN
					RETURN FALSE
			END;
			fsock.sinAddr.a := CAST(ADDRESS, ptrEntree^.hAddr^);
%END
		ELSE
			i := 0; j := 0; n := 0; l := Length(numeroIP);
			WHILE i <= l DO
				IF (i = l) OR (numeroIP[i] = '.')
					THEN
						 nIP[j] := 0C; StrToCard(nIP, k, fait);
						 IF (fait # strAllRight) THEN RETURN FALSE END;
						 fsock.sinAddr:AA[n] := CHR(k);
						 INC(n); j := 0; INC(i)
				END;
				IF (i < l) AND (numeroIP[i] # '.')
					THEN
						 nIP[j] := numeroIP[i];
						 INC(i); INC(j)
				END
			END;
	END;
%IF %NOT StonyBrook %THEN
	fsock.sinPort := htons(port);
	fsock.sinFamily := AF_INET;
	FOR e := 0 TO VAL(Int16, HIGH(fsock.sinZero)) DO
		fsock.sinZero[e] := 0C
	END;
	errSock := Connect(s, ADR(fsock), TSIZE(SOCKADDR_IN));
	IF errSock >= 0
%ELSE
	b := Connect(s, Socket.AF_INET, port, fsock.sinAddr);
	errSock := RecupErreurOS();
	IF b
%END
		THEN
			MajEtatS(s, ETAT_SOCK{ouverte, connecte});
(*
			QTils.LogMsgEx("ConnexionAuServeur(%d,%s:%hu): succès, err=%d\n", s, numeroIP, port, errSock );
*)
			RETURN TRUE
	END;
(*
	QTils.LogMsgEx("ConnexionAuServeur(%d,%s:%hu): err=%d\n", s, numeroIP, port, errSock );
*)
	(* errSock contient la description de l'erreur *)
%IF %NOT (WIN32 %OR WIN16) %THEN
	IF (errSock # EWOULDBLOCK) AND (errSock # EISCONN) AND
			(errSock # ECONNREFUSED) AND (errSock # EINPROGRESS)
%ELSE
	IF (errSock # WSAEWOULDBLOCK) AND (errSock # WSAEISCONN) AND
			(errSock # WSAECONNREFUSED)
%END
		THEN
			fatale := TRUE;
			RETURN FALSE
%IF %NOT (WIN32 %OR WIN16) %THEN
		ELSIF (errSock = EISCONN)
%ELSE
		ELSIF (errSock = WSAEISCONN)
%END
			THEN
				MajEtatS(s, ETAT_SOCK{ouverte, connecte});
				RETURN TRUE
			ELSE
(*
%IF (WIN32 %OR WIN16) %THEN
*)
				(* EWOULDBLOCK => attente de la connexion *)
				e := 51;
				rd := FALSE; ee := FALSE;
				LOOP
					DEC(e);
					TestEtat(s, FALSE, TRUE, FALSE, rd, wr, ee, timeOutms);
(*
					QTils.LogMsgEx("ConnexionAuServeur(%d,%s:%hu): TestEtat #%hd wr=%s ee=%s\n", s, numeroIP, port,
						51-e, BoolStr(wr), BoolStr(ee) );
*)
					IF wr OR ee OR (e=0) THEN EXIT END;
					IF timeOutms <= 0
						THEN
							Delay(200);
						ELSIF timeOutms > 10
							THEN
								Delay(timeOutms/10);
						ELSE
							Delay(timeOutms);
					END;
				END;
				IF wr
					THEN (* ça marche ! *)
						MajEtatS(s, ETAT_SOCK{ouverte, connecte});
						fatale := FALSE;
						errSock := 0;
(*
						QTils.LogMsgEx("ConnexionAuServeur(%d,%s:%hu): succès après %hd essais\n", s, numeroIP, port, 51-e );
*)
						RETURN TRUE
					ELSE
						fatale := TRUE;
						(* RJVB: on remet à jour errSock! *)
						MajErreurOS(errSock);
(*
						QTils.LogMsgEx("ConnexionAuServeur(%d,%s:%hu): échec après %hd essais, err=%d\n", s, numeroIP, port,
							51-e, errSock );
*)
						RETURN FALSE
				END;
(*
%END
*)
				fatale := FALSE;
				RETURN FALSE
	END
END ConnexionAuServeur;

(*--------------------------------------------------------------------------*)

PROCEDURE EtabliServeur(
	VAR s : SOCK; port : Card16) : BOOLEAN;
VAR
	e : Int16;
%IF %NOT StonyBrook %THEN
	lsock : SOCKADDR_IN;
%END
%IF WIN32 %THEN
	mode : CARDINAL;
%END
	b : BOOLEAN;
BEGIN
	IF NOT SockCreate(Socket.AF_INET, Socket.SOCK_STREAM, Socket.PF_UNSPEC, s)
		THEN
			EcritMsg("réseau non actif", 0);
			RETURN FALSE
	END;
%IF %NOT StonyBrook %THEN
	lsock.sinFamily := AF_INET;
	lsock.sinAddr.a := CAST(ADDRESS, htonl(INADDR_ANY));
	lsock.sinPort := htons(port);
	e := Bind(s, ADR(lsock), TSIZE(SOCKADDR_IN));
	IF e < 0
%ELSE
	b := Bind(s, Socket.AF_INET, port, Socket.INADDR_ANY);
	IF NOT b
%END
		THEN
			EcritMsg("erreur bind", -1);
			FermeSock(s);
			RETURN FALSE
	END;

%IF %NOT StonyBrook %THEN
	e := Listen(s, 3);
	IF e < 0
%ELSE
	b := Listen(s);
	IF NOT b
%END
		THEN
			EcritMsg("erreur listen", -1);
			FermeSock(s);
			RETURN FALSE
	END;

	(* set non-blocking socket *)
%IF %NOT StonyBrook %THEN
	e := Fcntl( s, F_SETFL, FNDELAY );
	IF ( e = -1 )
%ELSE
%IF WIN32 %THEN
	mode := 1; (* non bloquant si # 0 *)
	e := Fcntl( s, FIONBIO, mode);
%ELSE (* LINUX *)
	e := Fcntl( s, UNIX.F_SETFL, UNIX.O_NONBLOCK );
%END
	IF ( e # 0 )
%END
		THEN
			EcritMsg( "erreur fcntl", e);
			FermeSock(s);
			RETURN FALSE
	END;
	MajEtatS(s, ETAT_SOCK{ouverte});
	RETURN TRUE
END EtabliServeur;

(*--------------------------------------------------------------------------*)

PROCEDURE EtabliServeurUDPouTCP(
	VAR s : SOCK; port : Card16; modeTCP : BOOLEAN) : BOOLEAN;
VAR
	e : Int16;
%IF %NOT StonyBrook %THEN
	lsock : SOCKADDR_IN;
%END
	typeSock : CARDINAL;
%IF WIN32 %THEN
	mode : CARDINAL;
%END
	b : BOOLEAN;
BEGIN
	IF modeTCP
		THEN
			typeSock := Socket.SOCK_STREAM
		ELSE
			typeSock := Socket.SOCK_DGRAM
	END;
	IF NOT SockCreate(Socket.AF_INET, typeSock, Socket.PF_UNSPEC, s)
		THEN
			EcritMsg("réseau non actif", 0);
			RETURN FALSE
	END;
%IF %NOT StonyBrook %THEN
	lsock.sinFamily := AF_INET;
	lsock.sinAddr.a := CAST(ADDRESS, htonl(INADDR_ANY));
	lsock.sinPort := htons(port);
	e := Bind(s, ADR(lsock), TSIZE(SOCKADDR_IN));
	IF e < 0
%ELSE
	b := Bind(s, Socket.AF_INET, port, Socket.INADDR_ANY);
	IF NOT b
%END
		THEN
			EcritMsg("erreur bind", -1);
			FermeSock(s);
			RETURN FALSE
	END;

	IF modeTCP
		THEN
%IF %NOT StonyBrook %THEN
			e := Listen(s, 3);
			IF e < 0
%ELSE
			b := Listen(s);
			IF NOT b
%END
				THEN
					EcritMsg("erreur listen", -1);
					FermeSock(s);
					RETURN FALSE
			END;
	END;

	(* set non-blocking socket *)
%IF %NOT StonyBrook %THEN
	e := Fcntl( s, F_SETFL, FNDELAY );
	IF ( e = -1 )
%ELSE
%IF WIN32 %THEN
	mode := 1; (* non bloquant si # 0 *)
	e := Fcntl( s, FIONBIO, mode);
%ELSE (* LINUX *)
	e := Fcntl( s, UNIX.F_SETFL, UNIX.O_NONBLOCK );
%END
	IF ( e # 0 )
%END
		THEN
			EcritMsg( "erreur fcntl", e);
			FermeSock(s);
			RETURN FALSE
	END;
	IF modeTCP
		THEN
			MajEtatS(s, ETAT_SOCK{ouverte})
		ELSE
			MajEtatS(s, ETAT_SOCK{udp})
	END;
	RETURN TRUE
END EtabliServeurUDPouTCP;

(*--------------------------------------------------------------------------*)

PROCEDURE AttenteConnexionDunClient(
	s : SOCK; timeOutms : INTEGER; infinie : BOOLEAN; VAR ss : SOCK) : BOOLEAN;
VAR
	len : INTEGER;
	e : Int16;
%IF %NOT StonyBrook %THEN
	fsock : SOCKADDR_IN;
%END
	rd, wr, ee : BOOLEAN;
	b : BOOLEAN;
BEGIN
	IF (s = sock_nulle) OR NOT EstDsEtatS(s, ouverte) THEN RETURN FALSE END;
	carChaussette := ' ';
	LOOP
		len := TSIZE(SOCKADDR_IN);
		REPEAT
			TestEtat(s, TRUE, FALSE, FALSE, rd, wr, ee, timeOutms);
			IF rd
				THEN
					%IF %NOT StonyBrook %THEN
					ss := Accept(s, ADR(fsock), ADR(len));
					%ELSE
					b := Accept(s, ss);
					%END
					IF ss # sock_nulle
						THEN
							MajEtatS(ss, ETAT_SOCK{ouverte, connecte});
							%IF (WIN32 %OR WIN16) %THEN
							e := setsockopt(s, SOL_SOCKET, SO_REUSEADDR, TRUE, TSIZE(BOOL));
							IF e # 0
									 THEN
											EcritMsg("erreur reuseaddr", e)
							END;
							%END
							RETURN TRUE
						ELSE
							e := RecupErreurOS()
					END
			END;
			IF infinie
				THEN
%IF %NOT (SGI) %THEN
					IF Sortie(33C, carChaussette) THEN RETURN FALSE END
%END
				ELSE
					ss := sock_nulle;
					RETURN FALSE
			END
		UNTIL (ss # sock_nulle);
	END
END AttenteConnexionDunClient;

(*--------------------------------------------------------------------------*)

PROCEDURE InitIP() : BOOLEAN;
VAR
	e : Int16;
%IF (WIN32) %THEN
	donneeInit : WSADATA;
%END
BEGIN
	IF estInitialise
		THEN RETURN TRUE
	END;
%IF GDOS %THEN
	e := SocketInit();
	IF e < 0
		THEN
			errSock := RecupErreurOS();
			RETURN FALSE
	END;
%END
%IF WIN32 %THEN
	e := WSAStartup(101H, ADR(donneeInit));
	IF e # 0
		THEN
			errSock := RecupErreurOS();
			RETURN FALSE
	END;
%END
	FOR e := 0 TO max_sock DO
		(*
		  modification dans le cas du projet GERICO :
			initialisation des sockets UDP se fait dans le BEGIN du module Enregistr
			mais avant InitIp qui est appelé dans le BEGIN du module ComMDV2
			il faut donc tester si une des sockets n'a pas deja une valeur au
			niveau de son etat
		*)
		IF ts[e].e = ETAT_SOCK{}
			THEN
				ts[e].s := sock_nulle
		END;
(*
	Ancienne forme :

		ts[e].e := ETAT_SOCK{};
		ts[e].s := sock_nulle
*)
	END;
	InitDate();
	carChaussette := ' ';
	estInitialise := TRUE;
	RETURN TRUE
END InitIP;

(*
(*--------------------------------------------------------------------------*)
PROCEDURE NumeroIPMachineDistante(
	VAR nIP : SOCKADDR; s : SOCK) : BOOLEAN;
VAR
%IF %NOT(WIN32) %THEN
	g : Card16;
%ELSE
	g : INTEGER;
%END
	e : Int16;
BEGIN
	g := SIZE(nIP);
%IF %NOT(WIN32) %THEN
	e := GetPeerName(s, ADR(nIP), g);
	IF e = -1
%ELSE
	e := GetPeerName(s, nIP, g);
	IF e # 0
%END
		THEN
			e := GetErrNo();
			RETURN FALSE
		ELSE
			RETURN TRUE
	END
END NumeroIPMachineDistante;


(*--------------------------------------------------------------------------*)
PROCEDURE NumeroIPMachineLocale(
	VAR nIP : SOCKADDR; s : SOCK) : BOOLEAN;
VAR
%IF WIN32 %THEN
	g : INTEGER;
%ELSE
	g : Card16;
%END
	e : Int16;
BEGIN
	g := SIZE(nIP);
%IF %NOT (FlashTek %OR GDOS) %THEN
	e := GetSockName(s, nIP, g);
	IF e # 0
%ELSE
	e := GetSockName(s, ADR(nIP), g);
	IF e = -1
%END
		THEN
			e := GetErrNo();
			RETURN FALSE
		ELSE
			RETURN TRUE
	END
END NumeroIPMachineLocale;

(*
	rend l'adresse IP de la machine
*)
PROCEDURE NumeroIPMachine(
	VAR numeroIP : ARRAY OF CHAR);
VAR
	nomMachine : ARRAY[0..127] OF CHAR;
	host : LPHOSTENT;
	adr : LPSTR;
	pAdr : LPIN_ADDR;
%IF WIN32 %THEN
	res : WINT;
%END
BEGIN
	(* récupération de l'adresse IP de la machine locale *)
%IF %NOT(WIN32) %THEN
	GetHostName(ADR(nomMachine), SIZE(nomMachine));
%ELSE
	res := GetHostName(nomMachine, SIZE(nomMachine));
%END
	host := GetHostByName(nomMachine);
	pAdr := CAST(LPIN_ADDR, host^.h_addr_list^);
	adr := inet_ntoa(pAdr^);
	numeroIP := adr^
END NumeroIPMachine;
*)
(*
	Rend Vrai si nomRecherche n'est pas le nom de la machine locale,
	Faux sinon
*)
PROCEDURE EstDistante(
	nomRecherche : ARRAY OF CHAR) : BOOLEAN;
VAR
	nomMachine : ARRAY[0..127] OF CHAR;
	res : INTEGER;
	pAdr : Card32;
	host : PTR_HOST_ENT;
	%IF StonyBrook %THEN
	nomIPLocal : ARRAY[0..15] OF CHAR;
	%ELSE
	adr : PTR_CHAINE;
	%END
BEGIN
	(* récupération de l'adresse IP de la machine locale *)
	res := GetHostName(nomMachine, SIZE(nomMachine));
	host := CAST(PTR_HOST_ENT, GetHostByName(nomMachine));
	pAdr := host^.hAddr^[0];
%IF StonyBrook %THEN
	Socket.IPtoStr(pAdr, nomIPLocal);
	RETURN Compare(nomRecherche, nomIPLocal) # equal
%ELSE
	adr := CAST(PTR_CHAINE, ntoa(pAdr));
	RETURN Compare(nomRecherche, adr^) # equal
%END
END EstDistante;


(*
	répond vrai si IP est présent et que le port n'est pas déjà utilisé
*)
PROCEDURE PortEstInutilise(
	port : Card16) : BOOLEAN;
VAR
%IF %NOT StonyBrook %THEN
	e : Int16;
	lsock : SOCKADDR_IN;
%END
	s : SOCK;
	inutilise, b : BOOLEAN;
BEGIN
	IF NOT SockCreate(Socket.AF_INET, Socket.SOCK_STREAM, Socket.PF_UNSPEC, s)
		THEN
			RETURN FALSE
	END;
%IF %NOT StonyBrook %THEN
	lsock.sinFamily := AF_INET;
	lsock.sinAddr.a := CAST(ADDRESS, htonl(INADDR_ANY));
	lsock.sinPort := htons(port);
	e := Bind(s, ADR(lsock), TSIZE(SOCKADDR_IN));
	IF e < 0
%ELSE
	b := Bind(s, Socket.AF_INET, port, Socket.INADDR_ANY);
	IF NOT b
%END
		THEN
			inutilise := FALSE
		ELSE
			inutilise := TRUE
	END;
	FermeSock(s);
	RETURN inutilise
END PortEstInutilise;

(**
PROCEDURE IndTs(s : SOCK) : CARDINAL;
VAR
	i : CARDINAL;
BEGIN
	i := 0;
	WHILE (i <= max_sock) AND (ts[i].s # s) DO
		INC(i)
	END;
	RETURN i
END IndTs;
**)

PROCEDURE lSortie(charSortie : CHAR; VAR c : CHAR) : BOOLEAN
		%IF DLL %THEN [EXPORT, PROPAGATEEXCEPTION] %END;
BEGIN
	IF CharAvail()
		THEN
			Read(c);
			RETURN ( c = charSortie );
	END;
	RETURN FALSE;
END lSortie;

BEGIN

	Sortie := lSortie;
	estInitialise := FALSE;
	tempsUDPOkPred := GetTime();
	dtUDP := 10000;
	(* pourquoi à l'envers? *)
	FOR i := max_sock TO 0 BY -1 DO
		ts[i].s := sock_nulle;
		ts[i].e := ETAT_SOCK{}
	END;
%IF StonyBrook %THEN
	Socket.FD_ZERO(fd_set_vide);
%ELSE
	fd_set_vide := BITSET{0..max_sock};
%END

FINALLY

	IF estInitialise
		THEN
			FinIP
	END

%END

END Chaussette2.
