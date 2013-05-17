/* Chausset.c
 \ librairie de fonctions pour la communication par socket TCP/IP inspirée sur et compatible avec Chaussette.mod
 \ revue, corrigée et étendue par RJVB, fév,mars 2010
 */

#include <stdio.h>

//#include "stdafx.h"
#include <string.h>
#include "Chausset.h"

#if defined(__APPLE_CC__) || defined(__MACH__)
	#include <sys/fcntl.h>
#endif

#if defined(_MSC_VER) || defined(WIN32)
#	define MON_WIN_SOCK
#endif

typedef struct TS
{
	SOCK s;
	ETAT_SOCK e;
} TS;

static TS ts[max_sock];
statit BOOL isInitialised;

long errSock;
char carChaussette;

// le temps d'attente (en microsecondes) pour laisser select() faire son boulot.
#define MIN_ATTENTE_USEC	0
//int sendTimeOut = MIN_ATTENTE_USEC;
//int receiveTimeOut = MIN_ATTENTE_USEC;

char *errSockText(long errID)
#ifdef MON_WIN_SOCK
// RJVB 20100317: convertit un code erreur MSWin en text:
{ static TCHAR *buffer = NULL;
	// on laisse le system allouer la chaine de retour, mais il faudra la liberer nous-memes.
	// pour rester compatible avec strerror(), on fait cela ici, si <buffer> n'est pas NULL
	if( buffer ){
		LocalFree(buffer);
		buffer = NULL;
	}
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER
		|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_MAX_WIDTH_MASK,
		0, errID, 0, (LPTSTR)&buffer, 0, NULL );
	return (char*) buffer;
}
#else
{
	return strerror(errID);
}
#endif

BOOL InitIP()
{
	short e;
#ifdef MON_WIN_SOCK
	WSADATA donneeInit;
	e = WSAStartup(0x101, &donneeInit);
  
	if( e != 0 ){
		errSock = geterrno();
		return FALSE;
	}
#endif
	
	for( e = 0; e < max_sock; e++ ){
		ts[e].e.ouverte = FALSE;
		ts[e].e.connecte = FALSE;
		ts[e].s = sock_nulle;
	}

	carChaussette = ' ';
	isInitialised = TRUE;
 	return TRUE;
} //InitIP

void MajEtatS(SOCK s, ETAT_SOCK newState)
{
	short i;

	for( i = 0; i < max_sock; i++ ){
		if( ts[i].s == s ){
			ts[i].e = newState; 
			return;
		}
	}

	for( i = 0; i < max_sock; i++ ){
		if( ts[i].s == sock_nulle ){
			ts[i].s = s;
			ts[i].e = newState;
			return;
		}
	}
}// MajEtatS;

#define SOCKET_PROTOCOL	SOCK_STREAM

// RJVB 20100314: etablir client ou serveur revient a presque la meme chose, donc une seule fonction
// avec juste quelques lignes dediees semble plus elegant que 2 copies largement identiques.
BOOL EtabliClientOuServeur(SOCK *s, unsigned short port, BOOL serveur )
{ SOCKADDR_IN lsock;
  short e;
#ifdef MON_WIN_SOCK
  unsigned long mode;
#endif
  ETAT_SOCK etatSock;

	*s = socket( AF_INET, SOCKET_PROTOCOL, 0 );

	if( *s == sock_nulle ){
		fprintf( stderr, "socket error (%s)\n", errSockText(geterrno()) );
		return FALSE;
	}

	if( !serveur ){
		// RJVB 20100309: de `man setsockopt` : "Most socket-level options utilise an int parameter for optval. 
		// For setsockopt(), the parameter should be non-zero to enable a boolean option,
		// or zero if the option is to be disabled."
		{ int oui = 1;
			e = setsockopt( *s, SOL_SOCKET, SO_REUSEADDR, &oui, sizeof(oui) );
		}
		if( e != 0 ){
			errSock = geterrno();
		}
	}

	lsock.sin_addr.s_addr = htonl(INADDR_ANY);

	// RJVB 20100218: comparer un 'unsigned' à -1 n'a pas de sens...
	if( port > 0 ){
		lsock.sin_port = htons(port);
		lsock.sin_family = AF_INET;

		errno = 0;
		e = bind( *s, (SOCKADDR*)&lsock, sizeof(SOCKADDR_IN) );
		if( e != 0 ){
			fprintf( stderr, "bind error (-> %d; %s)\n", e, errSockText(geterrno()) );
			return FALSE;
		}
	}
	
#if SOCKET_PROTOCOL != SOCK_DRAM
	if( serveur ){
		if( listen( *s, 3 ) ){
			fprintf( stderr, "listen error (%s)\n", errSockText(geterrno()));
			return FALSE;
		}
	}
#endif

	// set non-blocking socket
#ifdef MON_WIN_SOCK
	mode = 1; // non bloquant si # 0 
	e = ioctlsocket( *s, FIONBIO, &mode );
	if( e != 0 ){
		fprintf( stderr, "fnctl error (%s)\n", errSockText(geterrno()) );
		return FALSE;
	}
#else
	{ int delay_flag = fcntl(*s, F_GETFL, 0);
          delay_flag |= O_NONBLOCK;
		fcntl( *s, F_SETFL, delay_flag );
	}
#endif

	etatSock.ouverte = TRUE;
	etatSock.connecte = FALSE;

	MajEtatS(*s, etatSock);
	return TRUE;

} // anciennement EtabliClient

BOOL EtabliClient(SOCK *s, unsigned short port)
{
	return EtabliClientOuServeur( s, port, FALSE );
}

BOOL ConnexionAuServeur(SOCK s, unsigned short port, char *nom, char *numeroIP, int timeOutms, BOOL *fatale)
{
	short e;
	SOCKADDR_IN fsock;
	HOSTENT *ptrEntree;
	BOOL rd, wr, ee;
	rd = wr = ee = 0;

	if( s == sock_nulle || !EstDsEtatS(s, ouverte) ){
		*fatale = TRUE;
		return FALSE;
	}

	if( numeroIP[0] == 0 ){
		ptrEntree = gethostbyname(nom);
		if( ptrEntree == 0 ){
  			return FALSE;
		}

		fsock.sin_addr.s_addr = inet_addr(ptrEntree->h_addr);
	}
	else{
		fsock.sin_addr.s_addr = inet_addr(numeroIP);
	}

	fsock.sin_port = htons(port);
	fsock.sin_family = AF_INET;

	for( e = 0; e < (short) strlen(fsock.sin_zero); e++ ){
		fsock.sin_zero[e] = 0;
	}

	errSock = connect( s, (SOCKADDR*)&fsock, sizeof(SOCKADDR_IN) );

	if( errSock == 0 ){
		ETAT_SOCK etatSock = { TRUE, TRUE };
		MajEtatS( s, etatSock );
		return TRUE;
	}
	errSock = geterrno();
	// errno contient la description de l'erreur 

#ifndef MON_WIN_SOCK
	if( errSock == EINPROGRESS ){
		// RJVB 20100315: connexion en cours: donnons un peu de temps au processus:
	  int ne;
	  struct timeval tv;
	  extern void TestEtatAttente(SOCK s, BOOL *r, BOOL *w, BOOL *e, TIMEVAL *temps );
		tv.tv_sec = 0;
		tv.tv_usec = 1000; // 1ms
		for( ne= 0, wr= 0; ne< 5 && !wr; ne++ ){
			TestEtatAttente( s, &rd, &wr, &ee, &tv );
		}
		if( wr ){
		  int res = connect( s, (SOCKADDR*)&fsock, sizeof(SOCKADDR_IN) );
			errSock = geterrno();
			if( res && errSock != EISCONN ){
				fprintf( stderr, "Erreur de gestion de \"connexion en cours\" %s\n", errSockText(errSock) );
			}
		}
	}
#endif

#ifdef MON_WIN_SOCK
	if( errSock != WSAEWOULDBLOCK && errSock != WSAEISCONN && errSock != WSAECONNREFUSED )
#else
	if( errSock != EWOULDBLOCK && errSock != EISCONN && errSock != ECONNREFUSED )
#endif
	{
		fprintf( stderr "socket connection error %d: %s(%u)\n", s, errSockText(errSock), errSock );
		*fatale = TRUE;
		return FALSE;
	}
#ifdef MON_WIN_SOCK
	else if( errSock == WSAEISCONN )
#else
	else if( errSock == EISCONN )
#endif
	{
		ETAT_SOCK EtatSock = {TRUE, TRUE};
		MajEtatS( s , EtatSock );
		return TRUE;
	}
	else{
	  int ne= 0;
		// EWOULDBLOCK => attente de la connexion
		e = 51;
		// RJVB 20100219: on accepte 5 "erreurs" ... qui en realite ne sont pertinentes que si on
		// peut avoir des donnees "out of band"
		// (cf. http://www.developerweb.net/forum/showpost.php?s=7dab7e2fb139d5b08f54ef2ca1ad11eb&p=19547&postcount=2)
		while( !(wr || ne> 5 || e == 0) ){
			e--;
			TestEtat( s, &rd, &wr, &ee, timeOutms );
			if( ee ){
				ne += 1;
			}
			if( timeOutms <= 0 ){
#ifdef MON_WIN_SOCK
				Sleep(200); // anciennement _sleep
#else
				usleep(200000);
#endif
			}
			else if( timeOutms > 10 ){
#ifdef MON_WIN_SOCK
				Sleep(timeOutms/10); // anciennement _sleep
#else
				usleep(timeOutms*100);
#endif
			}
			else{
#ifdef MON_WIN_SOCK
				Sleep(timeOutms); // anciennement _sleep
#else
				usleep(timeOutms*1000);
#endif
			}
		}

		if( wr ){
			// ça marche ! 
			ETAT_SOCK EtatSock = {TRUE, TRUE};
			MajEtatS(s, EtatSock);
			*fatale = FALSE;
			return TRUE;
		}
		else{
#ifdef MATLAB_MEX_FILE
			ssPrintf( "sock=%d rd=%d wr=%d ee=%d\n", s, rd, wr, ee );
#endif
#ifdef MON_WIN_SOCK
			fprintf( stderr, "ee=%d, error=%ld\n", ee, (long) WSAGetLastError() );
#endif
			*fatale = TRUE;
			return FALSE;
		}
		*fatale = FALSE;
		return FALSE;
	}
} // ConnexionAuServeur;

void FermeConnexionAuServeur(SOCK *s)
{
	short e;
	long res;
	struct{ short l_onoff, l_linger; } lin;
	ETAT_SOCK etatSock = {FALSE, FALSE};

//  il n'existe pas de moyen pour le serveur de détecter une connexion
//	si celle ci n'a pas été fermée 

	if( *s != sock_nulle ){
		res = shutdown( *s, 2 );
		lin.l_onoff = 1;
		lin.l_linger = 0;
		res = setsockopt( *s, SOL_SOCKET, SO_LINGER, (char*)&lin, sizeof(lin) );
#ifdef MON_WIN_SOCK
		e = closesocket(*s);
#else
		e = close(*s);
#endif
		if( e != 0 ){
#ifdef MON_WIN_SOCK
			errSock = WSAGetLastError();
#else
			errSock = geterrno();
#endif
			if(errSock != 38 && errSock != 10038){
				fprintf( stderr, "socket closing error %s", errSockText(errSock) );
			}
		}
		MajEtatS( *s, etatSock );
		*s = sock_nulle;
	}
} // FermeConnexionAuServeur;

void FermeClient(SOCK *s)
{
	short res;
	ETAT_SOCK etatSock = {FALSE, FALSE};

	if(*s != sock_nulle ){
#ifdef MON_WIN_SOCK
		res = closesocket(*s);
#else
		res = close(*s);
#endif
		MajEtatS( *s, etatSock );
		*s = sock_nulle;
	}
} // FermeClient;


BOOL EtabliServeur( SOCK *s, unsigned short port )
{
	return( EtabliClientOuServeur( s, port, TRUE ) );
}

// RJVB 20100314
BOOL AttenteConnexionDunClient( SOCK s, int timeOutms, BOOL blocking, SOCK *ss )
{ BOOL rd, wr, ee;

	if( s == sock_nulle || !EstDsEtatS(s, ouverte) ){
		return FALSE;
	}
	do{
		TestEtat( s, &rd, &wr, &ee, timeOutms );
		if( rd ){
		  struct sockaddr fsock;
#ifdef MON_WIN_SOCK
		  unsigned int flen = sizeof(fsock);
#else
		  socklen_t flen = sizeof(fsock);
#endif
			*ss = accept( s, &fsock, &flen );
			if( *ss != sock_nulle ){
			  ETAT_SOCK EtatSock = {TRUE, TRUE};
				MajEtatS( *ss, EtatSock);
				{ int oui = 1;
					if( setsockopt( s, SOL_SOCKET, SO_REUSEADDR, &oui, sizeof(oui)) ){
						errSock = geterrno();
					}
				}
				return TRUE;
			}
			else{
#ifdef MON_WIN_SOCK
				errSock = WSAGetLastError();
#else
				errSock = geterrno();
#endif
				fprintf( stderr, "accept() error %d (%s)\n", errSock, errSockText(errSock) );
			}
		}
		if( !blocking ){
			*ss = sock_nulle;
			return FALSE;
		}
	} while( *ss != sock_nulle );
	return FALSE;
}

void FinIP()
{
	short e, res;

	for(e = 0; e < max_sock; e++)
	{
		if(ts[e].e.ouverte == TRUE){
#ifdef MON_WIN_SOCK
			res = closesocket(ts[e].s);
#else
			res = close(ts[e].s);
#endif
			ts[e].e.ouverte = FALSE;
			ts[e].e.connecte = FALSE;
			fprintf( stderr, "closing %d\n", ts[e].s);
		}
	}

#ifdef MON_WIN_SOCK
	if(isInitialised)
	{
		if( WSACleanup() == 0 ){
			isInitialised = FALSE;
		}
		else{
			fprintf( stderr, "FinIP error\n");
		}
	}
#endif

} // FinIP;


BOOL EstDsEtatS(SOCK s, ETATS ee)
{
	short i;

	for(i = 0; i < max_sock; i++){
		if(ts[i].s == s){
			switch( ee ){
				case connecte :
					return ts[i].e.connecte;
					break;

				case ouverte : 
					return ts[i].e.ouverte;
					break;
			}
		}
	}
	return FALSE;
} // EstDsEtatS;

void TestEtatAttente(SOCK s, BOOL *r, BOOL *w, BOOL *e, TIMEVAL *temps )
{
	short trouve;
	fd_set readFds, writeFds, exceptFds;

	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);
	FD_ZERO(&exceptFds);
	if( s != sock_nulle ){
		FD_SET(s, &writeFds);
		FD_SET(s, &readFds);
		FD_SET(s, &exceptFds);
		// RJVB 20100219: 1e argument select() doit etre 1 plus la valeur max. des descripteurs
		// de socket associes aux 3 fd_set, donc ici s+1 et non pas 16:
		trouve = select(/*16*/ s+1, &readFds, &writeFds, &exceptFds, temps);
//		fprintf( stderr, "select->%d (%s)\n", trouve, errSockText(errno));
	}
	*r = FD_ISSET(s, &readFds) != 0; 
	*w = FD_ISSET(s, &writeFds) != 0;
	*e = FD_ISSET(s, &exceptFds) != 0;
} // anciennement TestEtat;

void TestEtat(SOCK s, BOOL *r, BOOL *w, BOOL *e, int timeOutms)
{ TIMEVAL temps;

	temps.tv_sec = timeOutms / 1000;
	temps.tv_usec = (timeOutms - temps.tv_sec * 1000) * 1000; 
	TestEtatAttente( s, r, w, e, &temps );
}

// RJVB 20100317: char *msg --> void *msg
BOOL SendNetMessage(SOCK s, void *msg, short serviceLen, short msgLen, int timeOutms, BOOL blocking)
{
	short e;
	short trouve;
	fd_set writeFds;
	TIMEVAL temps;

	
	if((serviceLen % 4) != 0){
		fprintf( stderr, "invalid service part of the message (not a multiple of 4) : %d\n", serviceLen );
		return FALSE;
	}

	if( s == sock_nulle || !EstDsEtatS(s, connecte) ){
		return FALSE;
	}

	carChaussette = ' ';
	// RJVB 20100317: remettre errSock a 0 pour pouvoir detecter une nouvelle erreur!
	errSock = 0;
	temps.tv_sec = timeOutms/1000;
	temps.tv_usec = (timeOutms - temps.tv_sec * 1000) * 1000;
	while(1){
		// on initialise writeFds ici, une seule fois suffit.
		FD_ZERO(&writeFds);
		do{
			FD_SET( s, &writeFds );
			// RJVB 20100311: cf. la remarque concernant select() ci-dessus
			trouve = select( /*16*/ s+1, NULL, &writeFds, NULL, &temps);
			// RJVB 20100317:le test ci-dessous doit etre ici, et non pas dans l'expression while()
			// car sinon on retourne toujours FALSE si non bloquant, meme si le socket est pret a etre
			// ecrit!!!
			if( trouve > 0 && FD_ISSET(s, &writeFds) != 0 ){
				break;
			}
			if( !blocking ){
				return FALSE;
			}
		} while( 1 );

		e = send( s, msg, msgLen, 0);

		if( e < 0 ){
			errSock = geterrno();
			// errno contient la description de l'erreur 
#ifdef MON_WIN_SOCK
			if( errSock != WSAEWOULDBLOCK )
#else
			if( errSock != EWOULDBLOCK )
#endif
			{
				fprintf( stderr, "send error %d (%s)\n", errSock, errSockText(errSock) );
				return FALSE;
			}
		}
		else{
			msgLen -= e;
			if(msgLen <= 0 ){
				break;
			}
		}
	}
	return TRUE;
} // SendNetMessage;

// RJVB 20100317:
int BasicSendNetMessage(SOCK s, void *msg, short msgLen, int timeOutms, BOOL blocking)
{
	short e;
	short trouve;
	int chunks = 0;
	fd_set writeFds;
	TIMEVAL temps;
	
// on vise a ne pas passer du temps sur des operations que celui qui nous appelle pourrait
// faire, dans cette fonction:	
//	if( s == sock_nulle || !EstDsEtatS(s, connecte) )
//		return FALSE;
	
	errSock = 0;
	temps.tv_sec = timeOutms / 1000;
	temps.tv_usec = (timeOutms - temps.tv_sec * 1000) * 1000;
	while(1){
		FD_ZERO(&writeFds);
		do{
			FD_SET( s, &writeFds );
			trouve = select( s+1, NULL, &writeFds, NULL, &temps);
			// RJVB 20100317:le test ci-dessous doit etre ici, et non pas dans l'expression while()
			// car sinon on retourne toujours FALSE si non bloquant, meme si le socket est pret a etre
			// ecrit!!!
			if( trouve > 0 && FD_ISSET(s, &writeFds) != 0 ){
				break;
			}
			if( !blocking ){
				return chunks;
			}
		} while( 1 );
		
		e = send( s, msg, msgLen, 0);
		
		if( e < 0 ){
			errSock = geterrno();
			// errno contient la description de l'erreur 
#ifdef MON_WIN_SOCK
			if( errSock != WSAEWOULDBLOCK )
#else
			if( errSock != EWOULDBLOCK )
#endif
			{
				fprintf( stderr, "send error %d (%s)\n", errSock, errSockText(errSock) );
				return FALSE;
			}
		}
		else{
			msgLen -= e;
			chunks += 1;
			if(msgLen <= 0 ){
				break;
			}
		}
	}
	return chunks;
} // BasicSendNetMessage;

// RJVB 20100317: pourquoi srvce etait-ce un char* au lieu d'un short* ?!
// en interne, c'est plus elegant d'utiliser une structure, sockMsg_ServiceMinimum.
// Cela resoud au meme temps le souci du point faible/point fort qui existait dans la presomption concernant
// ou la longueur (le premier mot) se trouvait dans le message de service!
BOOL ReceiveNetMessage(SOCK s, void *Srvce, short serviceLen, void *Msg, short msgLen, int timeOutms, BOOL blocking)
//	le premier mot (16b) de srvce contient la lg totale : srvce + msg
{
	BOOL deb;
	short trouve, l, len, lg, longMax;
	char *msg = (char*) Msg;
	sockMsg_ServiceMinimum *srvce = (sockMsg_ServiceMinimum*) Srvce;
	fd_set readFds;
	TIMEVAL temps;

	if( s == sock_nulle || !EstDsEtatS(s, connecte) ){
		return FALSE;
	}

	carChaussette = ' ';
	lg = 32767;
	l = 0;
	// RJVB 20100310: M2 expr. longMax := HIGH(msg) + 1; donne le nombre d'elements dans msg
	// donc longMax doit etre egal a msgLen et non pas
	// longMax = msgLen + 1;
	longMax = msgLen;
	deb = TRUE;
	// RJVB 20100317: remettre errSock a 0 pour pouvoir detecter une nouvelle erreur!
	errSock = 0;
	temps.tv_sec = timeOutms / 1000;
	temps.tv_usec = (timeOutms - temps.tv_sec * 1000) * 1000;
	while(lg > 0){
		FD_ZERO(&readFds);
		while(1){
			FD_SET( s, &readFds );
			// RJVB 20100311:
			trouve = select( /*16*/ s+1, &readFds, NULL, NULL, &temps );
			if( trouve > 0 && FD_ISSET(s, &readFds) != 0 ){
				break;
			}

			if( !blocking ) {
				return FALSE;
			}
		}

		// now try to receive the echo back 
		if(lg == 32767){
			len = recv( s, (char*) srvce, serviceLen, 0);
			lg = srvce->length;
		}
		else{
			len = recv( s, &(msg[l]), longMax, 0);
		}

		if(len < 0){
			errSock = geterrno();

#ifdef MON_WIN_SOCK
			if( errSock != WSAEWOULDBLOCK )
#else
			if( errSock != EWOULDBLOCK )
#endif
			{
				fprintf( stderr, "receive error %d (%s) : ", errSock, errSockText(errSock) );
				fflush( stderr );
				return FALSE;
			}
			else{
				len = 0;
			}
		}
		else if( len > 0 ){
			//if any, show it
			if( deb ){
				deb = FALSE;
			}
			else{
				l += len; 
				longMax -= len;
			}
			lg -= len;
		}
		else{ // len = 0 => fin de la connection 
			carChaussette = 33;
			return FALSE;
		}
	} // while
	// RJVB 20100317: champs service via structure sockMsg_ServiceMinimum, avec support pour champs length < 0 !
	if( srvce->length >= 0 ){
		srvce->length = l;
	}
	return TRUE;
} //ReceiveNetMessage;

// RJVB 20100317:
int BasicReceiveOptions = 0;
int BasicReceiveNetMessage(SOCK s, void *Msg, short msgLen, int timeOutms, BOOL blocking )
{ short trouve, l, len, longMax;
  int chunks = 0;
  char *msg = (char*) Msg;
  fd_set readFds;
  TIMEVAL temps;
	
	l = 0;
	longMax = msgLen;
	// RJVB 20100317: remettre errSock a 0 pour pouvoir detecter une nouvelle erreur!
	errSock = 0;
	temps.tv_sec = timeOutms / 1000;
	temps.tv_usec = (timeOutms - temps.tv_sec * 1000) * 1000;
	while( longMax > 0 ){
		FD_ZERO(&readFds);
		while(1){
			FD_SET(s, &readFds);
			trouve = select( s+1, &readFds, NULL, NULL, &temps );
			if( trouve > 0 && FD_ISSET(s, &readFds) != 0){
				break;
			}
			if( !blocking ) {
				return chunks;
			}
		}

		// now try to read the message 
		len = recv( s, &(msg[l]), longMax, BasicReceiveOptions );
		
		if(len < 0){
			errSock = geterrno();
#ifdef MON_WIN_SOCK
			if( errSock != WSAEWOULDBLOCK )
#else
			if( errSock != EWOULDBLOCK )
#endif
			{
				fflush( stderr );
				return FALSE;
			}
			else{
				len = 0;
				chunks += 1;
			}
		}
		else if( len > 0 ){
			//if any, show it
			l += len; 
			longMax -= len;
			chunks += 1;
		}
		else{ // len = 0 => fin de la connection 
			return FALSE;
		}
	} // while
	return chunks;
} // BasicReceiveNetMessage;

/*
	répond vrai si IP est présent et que le port n'est pas déjà utilisé
*/
BOOL PortEstInutilise(unsigned short port)
{
	short e;
	SOCK s;
	SOCKADDR_IN lsock;
	BOOL inutilise;
	long res;

	s = socket(AF_INET, SOCK_STREAM, 0);
	
	if( s == sock_nulle ){
		return FALSE;
	}

	lsock.sin_family = AF_INET;
	lsock.sin_addr.s_addr = htonl(INADDR_ANY);
	lsock.sin_port = htons(port);
	e = bind(s, (SOCKADDR*)&lsock, sizeof(SOCKADDR_IN));

	if( e != 0 ){
		inutilise = FALSE;
	}
	else{
		inutilise = TRUE;
	}

#ifdef MON_WIN_SOCK
	res = closesocket(s);
#else
	res = close(s);
#endif
	return inutilise;
}// PortEstInutilise;

