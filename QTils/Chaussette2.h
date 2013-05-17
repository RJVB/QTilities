#ifndef CHAUSSET_H
#define CHAUSSET_H

#define max_sock 15

#include <stdio.h>

// RJVB 20100218: on teste pour la presence de differents macros, pour determiner si on compile sous MSWindows ou non:
#if !defined(MON_WIN_SOCK) && (defined(linux) || defined(__DARWIN__) || defined(__APPLE_CC__) || defined(__MACH__) )
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <sys/ioctl.h>
	#include <netinet/in.h>
	#include <errno.h>
	#include <netdb.h>
	#define sock_nulle -1
	#define SOCK int
	#define geterrno() errno
	#define SOCKADDR_IN struct sockaddr_in
	#define SOCKADDR struct sockaddr
	#define HOSTENT struct hostent
	#define TIMEVAL struct timeval
	#ifndef BOOL
		#define BOOL	unsigned char
	#endif
	#ifndef TRUE
		#define TRUE	1
	#endif
	#ifndef FALSE
		#define FALSE	0
	#endif
#else
	#ifndef MON_WIN_SOCK
		#define MON_WIN_SOCK
	#endif
	#include <winsock.h>
	#define sock_nulle INVALID_SOCKET
	#define SOCK SOCKET
	//redefinition de la fonction de winsock
	#define geterrno() h_errno
#endif

typedef enum Etats
{
	ouverte, connecte
}ETATS;

typedef struct EtatSock
{
	unsigned char ouverte : 1;
	unsigned char connecte : 1;

} ETAT_SOCK;

// contenu mininum de la partie 'service' des messages geres par SendNetMessage() et ReceiveNetMessage()
#define SOCKMSG_SERVICEMINIMUM \
	short length; \
	short service

/* la version minimiale de la partie service des messages; seul le champs 'length' est utilisee
 \ dans ReceiveNetMessage(). La definition d'une version etendue prendrait une forme comme:
	typedef struct sockMsg_Service{
		SOCKMSG_SERVICEMINIMUM;
		struct YourHeaderData here;
	} sockMsg_Service;
 \ ou la taille doit etre un multiple de 4
 \ et un message (trame) serait defini comme
	typedef struct Packets{
		sockMsg_Service header;
		struct YourMsgData msg;
	} Packets;
 \ La taille de la partie service telle que renseignee a ReceiveNetMessage() doit alors etre defini comme
	#define HEADERSIZE(p)	(((unsigned long)&(p).msg) - ((unsigned long)&(p).header))
 \ pour inclure les octets de rembourage (padding) entre header et msg.
 */
typedef struct sockMsg_ServiceMinimum {
	SOCKMSG_SERVICEMINIMUM;
} sockMsg_ServiceMinimum;

extern long errSock;
extern char carChaussette;

// le temps d'attente (en microsecondes) pour laisser select() faire son boulot.
// en cas de lecture non-bloquant, un receiveTimeOut=1 peut ameliorer la rapidite a laquelle la communication
// entre client et serveur s'etablit
extern int sendTimeOut, receiveTimeOut;

//prototypes des fonctions ---------------------------------------

extern BOOL InitIP();

extern BOOL EtabliClient(SOCK *s, unsigned short port);

extern BOOL ConnexionAuServeur(SOCK s, unsigned short port, char *nom, char *numeroIP, int timeOutms, BOOL *fatale);

extern void FermeClient(SOCK *s);
#define FermeServeur(s)	FermeClient(s)

extern void FinIP();

extern BOOL EstDsEtatS(SOCK s, ETATS ee);

extern void MajEtatS(SOCK s, ETAT_SOCK nvelEtat);

extern void TestEtat(SOCK s, BOOL *r, BOOL *w, BOOL *e, int timeOutms);

extern void FermeConnexionAuServeur(SOCK *s);

extern BOOL SendNetMessage(SOCK s, void *msg, short serviceLen, short msgLen, BOOL blocking);
// envoi d'un message sans partie service; retourne le nombre de trames utilisees.
extern int BasicSendNetMessage(SOCK s, void *msg, short msgLen, BOOL blocking);

// si Srvce et Msg font partie d'une meme structure envoyee par SendNetMessage(), attention a serviceLen
// (qui doit prendre en compte les eventuels octets de 'padding' entre Srvce et Msg dans la structure) et
// passer des pointeurs vers &trame.Srvce et &trame.Msg pour eviter des debordements a la lecture si
// il y du padding (cf. le commentaire a la definition de sockMsg_ServiceMinimum .
extern BOOL ReceiveNetMessage(SOCK s, void *Srvce, short serviceLen, void *Msg, short msgLen, 
		BOOL blocking);
// reception d'un message sans partie service; retourne le nombre de trames utilisees.
extern int BasicReceiveNetMessage(SOCK s, void *Msg, short msgLen, BOOL blocking );

extern BOOL EtabliServeur( SOCK *s, unsigned short port );
extern BOOL AttenteConnexionDunClient( SOCK s, BOOL blocking, SOCK *ss );

extern BOOL PortEstInutilise(unsigned short port);

extern char *errSockText(long errID);

#endif
