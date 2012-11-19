IMPLEMENTATION MODULE QTilsM2_;

FROM SYSTEM IMPORT
	CAST, DIFADR;

CONST
	NULL_QTMovieWindowPtr	=	CAST(QTMovieWindowPtr, NULL);

PROCEDURE QTMovieWindowH_Check(wih : QTMovieWindowH) : BOOLEAN;
BEGIN
	RETURN ( (wih <> NULL_QTMovieWindowH) &(wih^ <> NULL_QTMovieWindowPtr)
		& (DIFADR(wih^^.self, wih^) = 0)
	);
END QTMovieWindowH_Check;

PROCEDURE QTMovieWindowH_isOpen(wih : QTMovieWindowH) : BOOLEAN;
BEGIN
	RETURN ( QTMovieWindowH_Check(wih) & (wih^^.theView <> NULL_NativeWindow) );
END QTMovieWindowH_isOpen;

PROCEDURE __cOpenQT__['_OpenQT']() : OSErr [StdCall];

PROCEDURE OpenQT() : OSErr;
BEGIN
	QTOpenError := __cOpenQT__();
	IF QTOpenError = noErr THEN
		QTOpened := TRUE;
	ELSE
		QTOpened := FALSE;
	END;
	RETURN QTOpenError;
END OpenQT;

PROCEDURE CloseQT;
BEGIN
	IF QTOpened THEN
		__cCloseQT__;
		QTOpened := FALSE;
	END;
END CloseQT;

VAR
	(*QTOpened : BOOLEAN;
	QTOpenError : OSErr; *)
	mca : MCActionsPtr;

BEGIN
	QTOpenError := OpenQT();
	mca := __cMCAction__();
	MCAction := mca^;
FINALLY
	CloseQT();
END QTilsM2_.
