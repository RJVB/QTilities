MODULE TstQTilsM2;

%IF StonyBrook %AND %NOT(LINUX) %THEN
FROM SYSTEM IMPORT
	AttachDebugger, AttachAll;
%END

FROM SYSTEM IMPORT
	ADR;

%IF AVEC_EXCEPTIONS %THEN
FROM EXCEPTIONS IMPORT *;
%ELSE
FROM Arret IMPORT
	ArretPgme;
%END

FROM QTilsM2 IMPORT *;

VAR
	qtwm : QTMovieWindows;
	qtwmPtr : QTMovieWindowPtr;
	qtwmH : QTMovieWindowH;
	n : Int32;
%IF AVEC_EXCEPTIONS %THEN
	exc : ExceptionSource;
%END

BEGIN
%IF StonyBrook %AND %NOT(LINUX) %THEN
	AttachDebugger(AttachAll);
%END
%IF AVEC_EXCEPTIONS %THEN
	AllocateSource(exc);
%END
	(* la maniere qu'il ne faut *pas* obtenir un QTMovieWindowH ... *)
	qtwmPtr := ADR(qtwm);
	qtwmH := ADR(qtwmPtr);
	(* QTMovieWindowH_isOpen() doit retourner FALSE pour ce QTMovieWindowH: *)
	IF QTMovieWindowH_isOpen( qtwmH )
		THEN
%IF AVEC_EXCEPTIONS %THEN
			RAISE( exc, 0, "QTMovieWindowH_isOpen() accepted an invalid QTMovieWindowH" );
%ELSE
			ArretPgme("pb", "QTMovieWindowH_isOpen() accepted an invalid QTMovieWindowH" );
%END
	END;
	IF QTOpened THEN
		qtwmH := OpenQTMovieInWindow(
			(* "d:\demos\motos\Perfect_Day.mp4", *)
			"Sample.mov",
			 1
		);
		IF NOT QTMovieWindowH_Check(qtwmH) THEN
%IF AVEC_EXCEPTIONS %THEN
			RAISE( exc, 0 (*LastQTError()*), "echec OpenQTMovieInWindow()" );
%ELSE
			ArretPgme("pb", "echec OpenQTMovieInWindow()" );
%END
		END;
		WHILE QTMovieWindowH_isOpen(qtwmH) DO
			n := PumpMessages(1)
		END;
		DisposeQTMovieWindow(qtwmH);
	END;
END TstQTilsM2.
