/*! @file Chaussette2.c
 \ librairie de fonctions pour la communication par socket TCP/IP inspirée sur et compatible avec Chaussette.mod
 \ revue, corrigée et étendue par RJVB, fév,mars 2010, may/june 2013
 */

#include <stdio.h>

#include <string.h>
#include "Chaussette2.h"

#if defined(__APPLE_CC__) || defined(__MACH__) || defined(linux)
#	include <unistd.h>
#	include <sys/fcntl.h>
#endif

#if defined(_MSC_VER) || defined(WIN32)
#	define __WINSOCK__
#endif

typedef struct TS
{
	SOCK s;
	STATE_SOCK e;
} TS;

static TS ts[max_sock];
static BOOL isInitialised;

long errSock;
char carChaussette;

// le temps d'attente (en microsecondes) pour laisser select() faire son boulot.
#define MIN_ATTENTE_USEC	0
//int sendTimeOut = MIN_ATTENTE_USEC;
//int receiveTimeOut = MIN_ATTENTE_USEC;

char *errSockText(long errID)
#ifdef __WINSOCK__
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
#ifdef __WINSOCK__
	WSADATA donneeInit;
	e = WSAStartup(0x101, &donneeInit);
  
	if( e != 0 ){
		errSock = geterrno();
		return FALSE;
	}
#endif
	
	for( e = 0; e < max_sock; e++ ){
		ts[e].e.opened = FALSE;
		ts[e].e.connected = FALSE;
		ts[e].s = NULLSOCKET;
	}

	carChaussette = ' ';
	isInitialised = TRUE;
 	return TRUE;
} //InitIP

void UpdateSocketState(SOCK s, STATE_SOCK newState)
{
	short i;

	for( i = 0; i < max_sock; i++ ){
		if( ts[i].s == s ){
			ts[i].e = newState; 
			return;
		}
	}

	for( i = 0; i < max_sock; i++ ){
		if( ts[i].s == NULLSOCKET ){
			ts[i].s = s;
			ts[i].e = newState;
			return;
		}
	}
}// UpdateSocketState;

#define SOCKET_PROTOCOL	SOCK_STREAM

// RJVB 20100314: etablir client ou serveur revient a presque la meme chose, donc une seule fonction
// avec juste quelques lignes dediees semble plus elegant que 2 copies largement identiques.
BOOL CreateClientOrServer(SOCK *s, unsigned short port, BOOL serveur, BOOL useTCP )
{ SOCKADDR_IN lsock;
  short e;
#ifdef __WINSOCK__
  unsigned long mode;
#endif
  STATE_SOCK etatSock;

	if( useTCP ){
		*s = socket( AF_INET, SOCK_STREAM, 0 );
	}
	else{
		*s = socket( AF_INET, SOCK_DGRAM, 0 );
	}

	if( *s == NULLSOCKET ){
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
	
	if( useTCP ){
		if( serveur ){
			if( listen( *s, 3 ) ){
				fprintf( stderr, "listen error (%s)\n", errSockText(geterrno()));
				return FALSE;
			}
		}
	}

	// set non-blocking socket
#ifdef __WINSOCK__
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

	etatSock.opened = TRUE;
	etatSock.connected = FALSE;

	UpdateSocketState(*s, etatSock);
	return TRUE;

} // anciennement EtabliClient

BOOL CreateClient(SOCK *s, unsigned short port, BOOL useTCP)
{
	return CreateClientOrServer( s, port, FALSE, useTCP );
}

BOOL ConnectToServer(SOCK s, unsigned short port, char *nom, char *address, int timeOutms, BOOL *fatal)
{
	short e;
	SOCKADDR_IN fsock;
	HOSTENT *ptrEntree;
	BOOL rd, wr, ee;
	rd = wr = ee = 0;

	if( s == NULLSOCKET || !LookupSocketState(s, opened) ){
		*fatal = TRUE;
		return FALSE;
	}

	if( address[0] == 0 ){
		ptrEntree = gethostbyname(nom);
		if( ptrEntree == 0 ){
  			return FALSE;
		}

		fsock.sin_addr.s_addr = inet_addr(ptrEntree->h_addr);
	}
	else{
		fsock.sin_addr.s_addr = inet_addr(address);
	}

	fsock.sin_port = htons(port);
	fsock.sin_family = AF_INET;

	for( e = 0; e < (short) strlen(fsock.sin_zero); e++ ){
		fsock.sin_zero[e] = 0;
	}

	errSock = connect( s, (SOCKADDR*)&fsock, sizeof(SOCKADDR_IN) );

	if( errSock == 0 ){
		STATE_SOCK etatSock = { TRUE, TRUE };
		UpdateSocketState( s, etatSock );
		return TRUE;
	}
	errSock = geterrno();
	// errno contient la description de l'erreur 

#ifndef __WINSOCK__
	if( errSock == EINPROGRESS ){
		// RJVB 20100315: connexion en cours: donnons un peu de temps au processus:
	  int ne;
	  struct timeval tv;
	  extern void TestSocketStateWait(SOCK s, BOOL *r, BOOL *w, BOOL *e, TIMEVAL *temps );
		tv.tv_sec = 0;
		tv.tv_usec = 1000; // 1ms
		for( ne= 0, wr= 0; ne< 5 && !wr; ne++ ){
			TestSocketStateWait( s, &rd, &wr, &ee, &tv );
		}
		if( wr ){
		  int res = connect( s, (SOCKADDR*)&fsock, sizeof(SOCKADDR_IN) );
			errSock = geterrno();
			if( res && errSock != EISCONN ){
				fprintf( stderr, "Error during connection (EINPROGRESS): %s\n", errSockText(errSock) );
				// 20130605:
				wr = 0;
			}
		}
	}
#endif

#ifdef __WINSOCK__
	if( errSock != WSAEWOULDBLOCK && errSock != WSAEISCONN && errSock != WSAECONNREFUSED )
#else
	if( errSock != EWOULDBLOCK && errSock != EISCONN && errSock != ECONNREFUSED )
#endif
	{
		fprintf( stderr, "socket connection error %d: %s(%u)\n", s, errSockText(errSock), errSock );
		*fatal = TRUE;
		return FALSE;
	}
#ifdef __WINSOCK__
	else if( errSock == WSAEISCONN )
#else
	else if( errSock == EISCONN )
#endif
	{
		STATE_SOCK EtatSock = {TRUE, TRUE};
		UpdateSocketState( s , EtatSock );
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
			TestSocketState( s, &rd, &wr, &ee, timeOutms );
			if( ee ){
				ne += 1;
			}
			if( timeOutms <= 0 ){
#ifdef __WINSOCK__
				Sleep(200); // anciennement _sleep
#else
				usleep(200000);
#endif
			}
			else if( timeOutms > 10 ){
#ifdef __WINSOCK__
				Sleep(timeOutms/10); // anciennement _sleep
#else
				usleep(timeOutms*100);
#endif
			}
			else{
#ifdef __WINSOCK__
				Sleep(timeOutms); // anciennement _sleep
#else
				usleep(timeOutms*1000);
#endif
			}
		}

		if( wr ){
			// ça marche ! 
			STATE_SOCK EtatSock = {TRUE, TRUE};
			UpdateSocketState(s, EtatSock);
			*fatal = FALSE;
			return TRUE;
		}
		else{
#ifdef MATLAB_MEX_FILE
			ssPrintf( "sock=%d rd=%d wr=%d ee=%d\n", s, rd, wr, ee );
#endif
#ifdef __WINSOCK__
			fprintf( stderr, "ee=%d, error=%ld\n", ee, (long) WSAGetLastError() );
#endif
			*fatal = TRUE;
			return FALSE;
		}
		*fatal = FALSE;
		return FALSE;
	}
} // ConnectToServer;

void CloseConnectionToServer(SOCK *s)
{
	short e;
	long res;
	struct{ short l_onoff, l_linger; } lin;
	STATE_SOCK etatSock = {FALSE, FALSE};

//  il n'existe pas de moyen pour le serveur de détecter une connexion
//	si celle ci n'a pas été fermée 

	if( *s != NULLSOCKET ){
		res = shutdown( *s, 2 );
		lin.l_onoff = 1;
		lin.l_linger = 0;
		res = setsockopt( *s, SOL_SOCKET, SO_LINGER, (char*)&lin, sizeof(lin) );
#ifdef __WINSOCK__
		e = closesocket(*s);
#else
		e = close(*s);
#endif
		if( e != 0 ){
#ifdef __WINSOCK__
			errSock = WSAGetLastError();
#else
			errSock = geterrno();
#endif
			if(errSock != 38 && errSock != 10038){
				fprintf( stderr, "socket closing error %s", errSockText(errSock) );
			}
		}
		UpdateSocketState( *s, etatSock );
		*s = NULLSOCKET;
	}
} // CloseConnectionToServer;

void CloseClient(SOCK *s)
{
	short res;
	STATE_SOCK etatSock = {FALSE, FALSE};

	if(*s != NULLSOCKET ){
#ifdef __WINSOCK__
		res = closesocket(*s);
#else
		res = close(*s);
#endif
		UpdateSocketState( *s, etatSock );
		*s = NULLSOCKET;
	}
} // CloseClient;


BOOL CreateServer( SOCK *s, unsigned short port, BOOL useTCP )
{
	return( CreateClientOrServer( s, port, TRUE, useTCP ) );
}

// RJVB 20100314
BOOL WaitForClientConnection( SOCK s, int timeOutms, BOOL blocking, SOCK *ss )
{ BOOL rd, wr, ee;

	if( s == NULLSOCKET || !LookupSocketState(s, opened) ){
		return FALSE;
	}
	do{
		TestSocketState( s, &rd, &wr, &ee, timeOutms );
		if( rd ){
		  struct sockaddr fsock;
#ifdef __WINSOCK__
		  unsigned int flen = sizeof(fsock);
#else
		  socklen_t flen = sizeof(fsock);
#endif
			*ss = accept( s, &fsock, &flen );
			if( *ss != NULLSOCKET ){
			  STATE_SOCK EtatSock = {TRUE, TRUE};
				UpdateSocketState( *ss, EtatSock);
				{ int oui = 1;
					if( setsockopt( s, SOL_SOCKET, SO_REUSEADDR, &oui, sizeof(oui)) ){
						errSock = geterrno();
					}
				}
				return TRUE;
			}
			else{
#ifdef __WINSOCK__
				errSock = WSAGetLastError();
#else
				errSock = geterrno();
#endif
				fprintf( stderr, "accept() error %d (%s)\n", errSock, errSockText(errSock) );
			}
		}
		if( !blocking ){
			*ss = NULLSOCKET;
			return FALSE;
		}
	} while( *ss != NULLSOCKET );
	return FALSE;
}

void EndIP()
{
	short e, res;

	for(e = 0; e < max_sock; e++)
	{
		if(ts[e].e.opened == TRUE){
#ifdef __WINSOCK__
			res = closesocket(ts[e].s);
#else
			res = close(ts[e].s);
#endif
			ts[e].e.opened = FALSE;
			ts[e].e.connected = FALSE;
			fprintf( stderr, "closing %d\n", ts[e].s);
		}
	}

#ifdef __WINSOCK__
	if(isInitialised)
	{
		if( WSACleanup() == 0 ){
			isInitialised = FALSE;
		}
		else{
			fprintf( stderr, "EndIP error\n");
		}
	}
#endif

} // EndIP;


BOOL LookupSocketState(SOCK s, STATES ee)
{
	short i;

	for(i = 0; i < max_sock; i++){
		if(ts[i].s == s){
			switch( ee ){
				case connected :
					return ts[i].e.connected;
					break;

				case opened : 
					return ts[i].e.opened;
					break;
			}
		}
	}
	return FALSE;
} // LookupSocketState;

void TestSocketStateWait(SOCK s, BOOL *r, BOOL *w, BOOL *e, TIMEVAL *temps )
{
	short found;
	fd_set readFds, writeFds, exceptFds;

	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);
	FD_ZERO(&exceptFds);
	if( s != NULLSOCKET ){
		FD_SET(s, &writeFds);
		FD_SET(s, &readFds);
		FD_SET(s, &exceptFds);
		// RJVB 20100219: 1e argument select() doit etre 1 plus la valeur max. des descripteurs
		// de socket associes aux 3 fd_set, donc ici s+1 et non pas 16:
		found = select(/*16*/ s+1, &readFds, &writeFds, &exceptFds, temps);
//		fprintf( stderr, "select->%d (%s)\n", trouve, errSockText(errno));
	}
	*r = FD_ISSET(s, &readFds) != 0; 
	*w = FD_ISSET(s, &writeFds) != 0;
	*e = FD_ISSET(s, &exceptFds) != 0;
} // anciennement TestSocketState;

void TestSocketState(SOCK s, BOOL *r, BOOL *w, BOOL *e, int timeOutms)
{ TIMEVAL temps;

	temps.tv_sec = timeOutms / 1000;
	temps.tv_usec = (timeOutms - temps.tv_sec * 1000) * 1000; 
	TestSocketStateWait( s, r, w, e, &temps );
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

	if( s == NULLSOCKET || !LookupSocketState(s, connected) ){
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
#ifdef __WINSOCK__
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
//	if( s == NULLSOCKET || !LookupSocketState(s, connected) )
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
#ifdef __WINSOCK__
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

	if( s == NULLSOCKET || !LookupSocketState(s, connected) ){
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

#ifdef __WINSOCK__
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
#ifdef __WINSOCK__
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
BOOL IsPortAvailable(unsigned short port)
{
	short e;
	SOCK s;
	SOCKADDR_IN lsock;
	BOOL unused;
	long res;

	s = socket(AF_INET, SOCK_STREAM, 0);
	
	if( s == NULLSOCKET ){
		return FALSE;
	}

	lsock.sin_family = AF_INET;
	lsock.sin_addr.s_addr = htonl(INADDR_ANY);
	lsock.sin_port = htons(port);
	e = bind(s, (SOCKADDR*)&lsock, sizeof(SOCKADDR_IN));

	if( e != 0 ){
		unused = FALSE;
	}
	else{
		unused = TRUE;
	}

#ifdef __WINSOCK__
	res = closesocket(s);
#else
	res = close(s);
#endif
	return unused;
}// IsPortAvailable;

