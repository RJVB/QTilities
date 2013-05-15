IMPLEMENTATION MODULE POSIXm2;

(* definir POSIXm2_DEV pour charger la librairie POSIXm2-dev.dll au lieu de POSIXm2.dll *)
<*/VALIDVERSION:POSIXm2_DEV,POSIXm2_DEBUG*>

FROM SYSTEM IMPORT
%IF StonyBrook %THEN
	%IF POSIXm2_DEV %THEN
		CPU, FPP, CPUCOUNT, CPU_HYPERTHREAD,
	%END
	SOURCEFILE, SOURCELINE,
%END
	CAST, BYTE, ADDRESS, TSIZE, VA_START;
%IF ADW %THEN
FROM SYSTEM IMPORT
 VA_LIST;
%END

%IF WIN32 %THEN
	FROM WIN32 IMPORT
		FARPROC,
		(*IsBadCodePtr,*)
		GetProcAddress,
		HANDLE,
		LoadLibrary, FreeLibrary;
	FROM WINX IMPORT
		NULL_HANDLE;
%END

FROM WINUSER IMPORT
	MessageBox, MB_OK, MB_ICONEXCLAMATION;

FROM Strings IMPORT
	Assign;

(* ===== création de l'interface avec POSIXm2.dll ==== *)
TYPE

	test23Struct = RECORD
		a, b : INTEGER32;
	END;

	(* NB: il semble qu'il vaut mieux déclarer tout [CDECL] et non [StdCall]! *)
	LibPOSIXm2Base =
		RECORD
			(* fonctions privées *)
			cLogMsgEx : PROCEDURE((*msg*) ARRAY OF CHAR, (*va_list ap*) ADDRESS):CARDINAL32 [CDECL];

			csetjmp_adr : PROCEDURE() : setjmpProcedure;

			cvsscanf : PROCEDURE( (*source*) ARRAY OF CHAR, (*format*) ARRAY OF CHAR,
														(*va_list ap*) ADDRESS):INTEGER32 [CDECL];
			cvsprintf : PROCEDURE( VAR (*dest*) ARRAY OF CHAR, (*format*) ARRAY OF CHAR,
														(*va_list ap*) ADDRESS):INTEGER32 [CDECL];
			test23 : PROCEDURE( VAR ARRAY OF test23Struct, VAR INTEGER32 ) [CDECL];

			(* *** fonctions exportées *** *)
			cPOSIX : POSIXm2Exports;
		END;
	LibBaseGetter = PROCEDURE(VAR LibPOSIXm2Base):CARDINAL32 [CDECL];

VAR
	POSIXm2Handle : HANDLE;
	POSIXm2C : LibPOSIXm2Base;
	lEmptyArgString : ArgString;
	DevMode : BOOLEAN;

PROCEDURE POSIXAvailable() : BOOLEAN;
BEGIN
	RETURN POSIXm2Handle <> NULL_HANDLE;
END POSIXAvailable;

PROCEDURE pm2PostMessage( title, message : ARRAY OF CHAR ): CARDINAL;
BEGIN
	RETURN MessageBox( NIL, message, title, MB_OK BOR MB_ICONEXCLAMATION );
END pm2PostMessage;

(* obtient l'adresse de <symbol> dans la DLL <handle> *)
PROCEDURE dlsym(handle : HANDLE; symbol : ARRAY OF CHAR; VAR valid : BOOLEAN) : PROC;
%IF WIN32 %THEN
	VAR
		procAdr : FARPROC;
	BEGIN
		procAdr := GetProcAddress(handle, symbol);
		IF procAdr = CAST(FARPROC, NIL)
			THEN
				valid := FALSE;
				pm2PostMessage( "Fonction introuvable dans POSIXm2.dll", symbol );
			ELSE
				(*valid := NOT(IsBadCodePtr(procAdr));*)
				(* IsBadCodePtr should no longer be used! *)
				valid := TRUE;
			END;
		RETURN CAST(PROC, procAdr)
%ELSE
	BEGIN
		RETURN CAST(PROC, NIL)
	%END
END dlsym;

(* ===== code exporté ===== *)

PROCEDURE lLogMsg(msg : ARRAY OF CHAR) : CARDINAL32 [CDECL];
BEGIN
	IF DevMode
		THEN
			pm2PostMessage("LogMsg", msg);
	END;
	RETURN LENGTH(msg);
END lLogMsg;

PROCEDURE vLogMsgEx( formatStr : ARRAY OF CHAR; argsList : ADDRESS ): CARDINAL32;
VAR
	ret : CARDINAL32;
BEGIN
	IF ( POSIXm2Handle <> NULL_HANDLE )
		THEN
			ret := POSIXm2C.cLogMsgEx( formatStr, argsList );
		ELSE
			IF DevMode
				THEN
					pm2PostMessage( "LogMsgEx", formatStr );
			END;
			ret := LENGTH(formatStr);
		END;
	RETURN ret;
END vLogMsgEx;

PROCEDURE vPosixLogMsgEx(msg : ARRAY OF CHAR):CARDINAL32  [RightToLeft, LEAVES, VARIABLE];
VAR
	argsList : VA_LIST;
BEGIN
	VA_START(argsList);
	POSIX.LogLocation(SOURCEFILE,SOURCELINE);
	RETURN POSIX.vLogMsgEx( msg, CAST(ADDRESS,argsList) );
END vPosixLogMsgEx;

PROCEDURE LogMsgEx( formatStr : ARRAY OF CHAR ): CARDINAL32 [RightToLeft, LEAVES, VARIABLE];
VAR
	argsList : VA_LIST;
	ret : CARDINAL32;
(*
	msg : ARRAY[0..2048] OF CHAR;
*)
BEGIN
	IF ( POSIXm2Handle <> NULL_HANDLE )
		THEN
			VA_START(argsList); (* <= to get the variable arguments base address *)
(*
			IF FormatStringEx(formatStr, msg, argsList)
*)
			ret := POSIXm2C.cLogMsgEx( formatStr, CAST(ADDRESS,argsList) );
		ELSE
			(* on pourrait utiliser wvsprintf ici pour générer le message voulu... *)
			IF DevMode
				THEN
					pm2PostMessage( "LogMsgEx", formatStr );
			END;
			ret := LENGTH(formatStr);
		END;
	RETURN ret;
END LogMsgEx;

PROCEDURE vsscanf( source, formatStr : ARRAY OF CHAR ): INTEGER32 [RightToLeft, LEAVES, VARIABLE];
VAR
	argsList : VA_LIST;
	ret : INTEGER32;
BEGIN
	IF ( POSIXm2Handle <> NULL_HANDLE )
		THEN
			VA_START(argsList); (* <= to get the variable arguments base address *)
			ret := POSIXm2C.cvsscanf( source, formatStr, CAST(ADDRESS,argsList) );
		ELSE
			ret := -2;
		END;
	RETURN ret;
END vsscanf;

PROCEDURE vsprintf( VAR dest : ARRAY OF CHAR; formatStr : ARRAY OF CHAR ): INTEGER32 [RightToLeft, LEAVES, VARIABLE];
VAR
	argsList : VA_LIST;
	ret : INTEGER32;
BEGIN
	IF ( POSIXm2Handle <> NULL_HANDLE )
		THEN
			VA_START(argsList); (* <= to get the variable arguments base address *)
			ret := POSIXm2C.cvsprintf( dest, formatStr, CAST(ADDRESS,argsList) );
		ELSE
			ret := -2;
		END;
	RETURN ret;
END vsprintf;

PROCEDURE lmemset( VAR mem : NOHIGH ARRAY OF BYTE; val : BYTE; size : CARDINAL32 ) [CDECL];
VAR
	i : CARDINAL32;
BEGIN
	FOR i := 0 TO size-1 DO
		mem[i] := val;
	END;
END lmemset;

PROCEDURE lEmptyArg( arg : CARDINAL16 ) : ArgString [CDECL];
BEGIN
	RETURN lEmptyArgString;
END lEmptyArg;

(* initialisation du module POSIXm2 *)
VAR
	valid : BOOLEAN;
	getPOSIXm2C : LibBaseGetter;
	POSIXm2CSize, POSIXm2CObtained : CARDINAL;
	test23Array : ARRAY[0..2] OF test23Struct;
	test23Empty : ARRAY[0..0] OF test23Struct;
	test23N : INTEGER;

BEGIN

	lmemset( POSIX, 0, SIZE(POSIX) );

%IF WIN32 %THEN
	(* on charge la dll POSIXm2 *)
%IF POSIXm2_DEBUG %THEN
	POSIXm2Handle := LoadLibrary("POSIXm2-debug.dll");
	IF ( POSIXm2Handle = NULL_HANDLE )
		THEN
			POSIXm2Handle := LoadLibrary("POSIXm2-dev.dll");
	END;
	DevMode := TRUE;
%ELSIF POSIXm2_DEV %THEN
	(* mode "developer": on charge la version de POSIXm2.dll qui génère des messages dans une fenêtre journal *)
	POSIXm2Handle := LoadLibrary("POSIXm2-dev.dll");
	DevMode := TRUE;
%ELSE
	(* on charge POSIXm2-dev.dll si elle est présente *)
	POSIXm2Handle := LoadLibrary("POSIXm2-dev.dll");
	IF ( POSIXm2Handle = NULL_HANDLE )
		THEN
			(* mode "release": on charge la version de POSIXm2.dll qui ignore les messages journal *)
			POSIXm2Handle := LoadLibrary("POSIXm2.dll");
			DevMode := FALSE;
		ELSE
			DevMode := TRUE;
	END;
%END
	IF ( POSIXm2Handle <> NULL_HANDLE )
		THEN
			(* on obtient l'adresse de la fonction initDMBasePOSIXm2() qui va nous donner les adresses
				des fonctions d'intérêt dans le record POSIXm2C :
			*)
			getPOSIXm2C := CAST( LibBaseGetter, dlsym( POSIXm2Handle, "initDMBasePOSIXm2", valid ));
			IF valid
				THEN
					(* taille de la structure initialisée par initDMBasePOSIXm2(): la taille de LibPOSIXm2Base moins
					 * les (3) membres qui renvoient vers des fonctions Modula-2
					 *)
					POSIXm2CSize := TSIZE(LibPOSIXm2Base) - 5 * TSIZE(ADDRESS);
					POSIXm2CObtained := getPOSIXm2C( POSIXm2C );
					(* getPOSIXm2C() (= POSIXm2.dll::initDMBasePOSIXm2) retourne la taille de l'equivalent C du record
						LibPOSIXm2Base. Évidemment elle doit être égale à POSIXm2CSize, ce qui fournit un
						test rapide d'échec et/ou de la compatibilité de l'initialisation:
					*)
					IF POSIXm2CSize <> POSIXm2CObtained
						THEN
							pm2PostMessage( "Pb d'initialisation de POSIXm2", "Incompatibilité de version de POSIXm2.dll" );
							FreeLibrary(POSIXm2Handle);
							POSIXm2Handle := NULL_HANDLE;
						ELSE
							WITH POSIXm2C DO
								cPOSIX.LogMsgEx := LogMsgEx;
								cPOSIX.vLogMsgEx := vLogMsgEx;
								cPOSIX.sscanf := vsscanf;
								cPOSIX.sprintf := vsprintf;
								(* on doit impérativement appeler la fonction setjmp() de la librairie C système,
								 * et non pas un proxy dans POSIXm2.dll . On obtient l'adresse de la fonction via une
								 * fonction dans POSIXm2.c , pour éviter de devoir la chercher dans la dll système adéquate...
								 *)
								cPOSIX.setjmp := POSIXm2C.csetjmp_adr();
							END;
							(* on exporte la partie publique de POSIXm2C: *)
							POSIX := POSIXm2C.cPOSIX;
%IF POSIXm2_DEV %THEN
							MarkLog; POSIX.LogMsgEx( "initialisation POSIXm2 OK; CPU=%d x %d FFP=%d hyperthreading=%d",
								CPU, CPUCOUNT, FPP, CPU_HYPERTHREAD );
%END
%IF POSIXm2_DEBUG %THEN
							test23N := -2;
							test23Array[0].a := 2;
							test23Array[0].b := 3;
							test23Array[1].a := 4;
							test23Array[1].b := 5;
							test23Array[2].a := 7;
							test23Array[2].b := 8;
							POSIXm2C.test23( test23Array, test23N );
							POSIXm2C.test23( test23Empty, test23N );
%END
						END;
				ELSE
					pm2PostMessage( "Alerte", "Échec d'initialisation de POSIXm2" );
					FreeLibrary(POSIXm2Handle);
					POSIXm2Handle := NULL_HANDLE;
				END;
		ELSE
%IF POSIXm2_DEV %THEN
			pm2PostMessage( "Alerte", "Échec charge POSIXm2-dev.dll" );
%ELSE
			pm2PostMessage( "Alerte", "Échec charge POSIXm2.dll (et/ou de POSIXm2-dev.dll)" );
%END
	END;
	IF ( POSIXm2Handle = NULL_HANDLE )
		THEN
			(* because there'll always be dudes who try to use LogMsg(Ex) to notify the user we failed initialising... *)
			POSIXm2C.cPOSIX.LogMsg := lLogMsg;
			POSIXm2C.cPOSIX.LogMsgEx := LogMsgEx;
			POSIXm2C.cPOSIX.memset := lmemset;
			POSIXm2C.cPOSIX.argc := 0;
			Assign( "", lEmptyArgString );
			POSIXm2C.cPOSIX.Arg := lEmptyArg;
			POSIX := POSIXm2C.cPOSIX;
	END;
%ELSE
	POSIXm2Handle := NULL_HANDLE;
%END

FINALLY

%IF POSIXm2_DEV %THEN
	POSIX.LogMsg( "décharge de POSIXm2" );
%END
%IF WIN32 %THEN
	IF POSIXm2Handle # NULL_HANDLE
		THEN
			FreeLibrary(POSIXm2Handle);
			POSIXm2Handle := NULL_HANDLE
	END
%END

END POSIXm2.
