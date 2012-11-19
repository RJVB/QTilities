MODULE DllMain;
<*/DLL*>

FROM QTilsM2 IMPORT *;

EXPORTS
    OpenQT INDEX 1,
    CloseQT INDEX 2,
    PumpMessages INDEX 3;

BEGIN
    RETURN TRUE;
END DllMain.
