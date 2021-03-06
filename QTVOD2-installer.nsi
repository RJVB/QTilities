; Script generated by the HM NIS Edit Script Wizard.

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "QTVODm2"
!define PRODUCT_VERSION "1.4RC4"
!define PRODUCT_PUBLISHER "IFSTTAR"
!define PRODUCT_WEB_SITE "http://www.inrets.fr"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\QTVODm2.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"

SetCompressor lzma
ShowInstDetails show

; MUI 1.67 compatible ------
!include "MUI.nsh"
!include "FontRegAdv.nsh"
!include "FontName.nsh"
!include "winmessages.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

;--------------------------------
;Interface Configuration

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "S:\MacOSX\QTilities\QTVODm2inst.bmp"

; Language Selection Dialog Settings
!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

; Welcome page
;Request application privileges for Windows Vista
RequestExecutionLevel admin

; setting env.vars:
!define env_hklm 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'


var QTDIR

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY
;  SetBrandingImage /RESIZETOFIT "S:\MacOSX\QTilities\QTils\QTils.ico"
  SetAutoClose false
  SetRegView 32
  ReadRegStr $0 HKLM "SOFTWARE\Apple Computer, Inc.\QuickTime" "InstallDir"
  IfErrors 0 +3
           MessageBox MB_ICONEXCLAMATION|MB_OK "QuickTime is required. http://www.apple.com/quicktime/download"
           Abort "QuickTime is required. http://www.apple.com/quicktime/download "
  StrCpy $QTDIR $0
  DetailPrint "QuickTime is installed in $QTDIR"
;  MessageBox MB_ICONEXCLAMATION|MB_OK "QuickTime is installed in $QTDIR"
FunctionEnd

; Welcome page
!insertmacro MUI_PAGE_WELCOME
Page Custom LockedListShow
; Components page
Function LockedListShow
  !insertmacro MUI_HEADER_TEXT "$(KillProcs1)" "$(KillProcs2)"
  LockedList::AddModule "QTVODm2.exe"
  LockedList::AddModule "\QTImage2Mov.qtx"
  LockedList::AddModule "\QTImage2Mov-dev.qtx"
  LockedList::AddModule "\QTils.dll"
  LockedList::AddModule "\POSIXm2.dll"
  LockedList::Dialog /autonext /autoclose "" "" "" ""
    pop $R0
FunctionEnd

Section
SectionEnd

!insertmacro MUI_PAGE_COMPONENTS
; Directory page
!insertmacro MUI_PAGE_DIRECTORY

; Start menu page
;var ICONS_GROUP
;!define MUI_STARTMENUPAGE_NODISABLE
;!define MUI_STARTMENUPAGE_DEFAULTFOLDER "QTVODm2"
;!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
;!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
;!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"
;!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP

; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "English"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "QTVODm2-v${PRODUCT_VERSION}-installer.exe"
InstallDir "$EXEDIR\QTVODm2"
; Icon "S:\MacOSX\QTilities\QTils\QTils.ico"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

; AddBrandingImage left 128

LangString Name ${LANG_ENGLISH} "English"
LangString Name ${LANG_FRENCH} "French"
LangString Sec2Name ${LANG_ENGLISH} "QTVODm2 Player"
LangString Sec2Name ${LANG_FRENCH} "Lecteur QTVODm2"
LangString Sec1Descr ${LANG_ENGLISH} "The QuickTime Importer QTImage2Mov that allows to import VOD and Qi2M files. Installs in $QTDIR QTComponents"
LangString Sec2Descr ${LANG_ENGLISH} "The QTVODm2 Player application and an example launch script and config file"
LangString Sec3Descr ${LANG_ENGLISH} "Documentation for QTVODm2 and QTImage2Mov (the engine used by QTVODm2)"
LangString Sec1Descr ${LANG_FRENCH} "Le 'QuickTime Importer' QTImage2Mov qui permet d'importer les fichiers VOD et QI2M dans QuickTime.  s'Installe dans $QTDIR QTComponents"
LangString Sec2Descr ${LANG_FRENCH} "Le Lecteur QTVODm2 ainsi qu'un exemple de script de lancement et de fichier de configuration"
LangString Sec3Descr ${LANG_FRENCH} "Documentation pour QTVODm2 et QTImage2Mov (le moteur utilis� par QTVODm2)"
LangString Sec4Descr  ${LANG_ENGLISH} "Interface/API for client/server remote control"
LangString Sec4Descr  ${LANG_FRENCH} "Interface (API) pour contr�le distant client/serveur"
LangString Sec5Descr  ${LANG_ENGLISH} "Font used for the time and GPS info track in imported videos"
LangString Sec5Descr  ${LANG_FRENCH} "La police utilis�e pour la piste d'horodatage et GPS dans les vid�os import�es"
LangString Sec8Descr ${LANG_ENGLISH} "FFmpeg and FFprobe, used by the QTImage2Mov Importer. Installs in $QTDIR QTSystem"
LangString Sec8Descr ${LANG_FRENCH} "FFmpeg et FFprobe, utilis�s par le plugin QTImage2Mov. s'Installent dans $QTDIR QTSystem"
LangString QI2Mfail  ${LANG_ENGLISH} "could not be installed; to be copied manually into"
LangString QI2Mfail  ${LANG_FRENCH} "echec d'installation, � copier � la main dans"
LangString DirSelectSText  ${LANG_ENGLISH} "Install dir (QTImage2Mov will go into $QTDIR QTComponents!)"
LangString DirSelectSText  ${LANG_FRENCH} "R�pertoire d'installation (QTImage2Mov ira dans $QTDIR QTComponents!)"
LangString AuthorText  ${LANG_ENGLISH} "QTImage2Mov and QTVODm2 � 2010-2012 R.J.V. Bertin/INRETS/IFSTTAR"
LangString AuthorText  ${LANG_FRENCH} "QTImage2Mov et QTVODm2 � 2010-2012 R.J.V. Bertin/INRETS/IFSTTAR"
LangString KillProcs1   ${LANG_ENGLISH} "Please wait"
LangString KillProcs1   ${LANG_FRENCH} "Merci de patienter"
LangString KillProcs2   ${LANG_ENGLISH} "Verifying running processes"
LangString KillProcs2   ${LANG_FRENCH} "Verification des processus en cours d'execution"

DirText "" "$(DirSelectSText)"

ComponentText "$(AuthorText)"

Section "QuickTime VOD & Qi2M Importer Component" SEC01
  SetRegView 32
;  ReadRegStr $0 HKLM "SOFTWARE\Apple Computer, Inc.\QuickTime" "InstallDir"
;  IfErrors 0 +2
;           Abort "QuickTime is required"
  DetailPrint "QuickTime is installed in $QTDIR"
  KillProcDLL::KillProc "QuickTimePlayer.exe"
  DetailPrint "KillProc QuickTimePlayer returned $R0"
  KillProcDLL::KillProc "QTVODm2.exe"
  DetailPrint "KillProc QTVODm2 returned $R0"
  KillProcDLL::KillProc "Safari.exe"
  DetailPrint "KillProc Safari returned $R0"
  SetOutPath "$QTDIR\QTComponents"
  SetOverwrite ifnewer
  Delete "$QTDir\QTComponents\QTImage2Mov-dev.qtx"
  File "S:\MacOSX\QTImage2Mov\QTImage2Mov.qtx"
  IfErrors 0 +2
           MessageBox MB_ICONEXCLAMATION|MB_OK "QTImage2Mov $(QI2Mfail) $QTDIR\QTComponents"
;  Sleep 1000
SectionEnd

Section "FFmpeg 64bits" SEC08
  SetRegView 32
  KillProcDLL::KillProc "ffmpeg.exe"
  DetailPrint "KillProc ffmpeg returned $R0"
  KillProcDLL::KillProc "ffprobe.exe"
  DetailPrint "KillProc ffprobe returned $R0"
  SetOutPath "$QTDIR\QTSystem"
  SetOverwrite ifnewer
  File "s:\MacOSX\brigade\FFmpeg\win64\ffmpeg.exe"
  File "s:\MacOSX\brigade\FFmpeg\win64\ffprobe.exe"
;  NSISdl::download http://www.domain.com/file "$QTDIR\QTSystem\ffmpeg.exe"
;  Pop $R0 ;Get the return value
;    StrCmp $R0 "success" +3
;    MessageBox MB_ICONEXCLAMATION|MB_OK "Download of ffmpeg failed: $R0"
SectionEnd

Section !$(Sec2Name) SEC02
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  File "S:\MacOSX\QTilities\QTils\Mod2\QTVODm2.exe"
  CreateShortCut "$INSTDIR\QTVODm2-mjpeg2M.lnk" "$INSTDIR\QTVODm2.exe" "-fcodec mjpeg -fbitrate 2000k"
  CreateShortCut "$INSTDIR\QTVODm2-mjpeg2M-fsplit.lnk" "$INSTDIR\QTVODm2.exe" "-fcodec mjpeg -fbitrate 2000k -fsplit TRUE"
  CreateShortCut "$INSTDIR\QTVODm2-mpeg2-fsplit.lnk" "$INSTDIR\QTVODm2.exe" "-fcodec mpeg2video -fbitrate 4000k -fsplit TRUE"
  File "S:\MacOSX\QTilities\QTils\Mod2\LanceQTVODm2.bat"
  File "S:\MacOSX\QTilities\QTils\Mod2\VODdesign.xml"
  File "S:\MacOSX\QTilities\QTils\QTils.dll"
  File "S:\MacOSX\QTilities\QTils\POSIXm2.dll"
  File "S:\MacOSX\QTilities\QTils\Release\QTMoviePlayer.exe"
  DetailPrint "Setting QTMW_DoubleBuffering=true in environment..."
  WriteRegExpandStr ${env_hklm} "QTMW_DoubleBuffering" "true"
  ; make sure windows knows about the change
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
; Shortcuts
;  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
;  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "Documentation" SEC03
  SetOutPath "$INSTDIR\Documentation"
  SetOverwrite ifnewer
  File /nonfatal "S:\MacOSX\QTilities\QTVODm2.pdf"
  File /nonfatal /r "S:\MacOSX\QTilities\FFmpeg-licenses"
  File /nonfatal "S:\MacOSX\QTilities\ffmpeg-20121125-git-26c531c-win64-static-readme.txt"
  File /nonfatal "S:\MacOSX\QTImage2Mov\QTImage2Mov-v1.2.pdf"
SectionEnd

Section /o "Interface" SEC04
  SetOutPath "$INSTDIR\API"
  SetOverwrite ifnewer
  File /nonfatal "S:\MacOSX\QTilities\QTils\Mod2\mod\QTVODcomm.mod"
  File /nonfatal "S:\MacOSX\QTilities\QTils\Mod2\mod\QTilsM2.mod"
  File /nonfatal "S:\MacOSX\QTilities\QTils\Mod2\mod\POSIXm2.mod"
  File /nonfatal "S:\MacOSX\QTilities\QTils\Mod2\def\QTVODcomm.def"
  File /nonfatal "S:\MacOSX\QTilities\QTils\Mod2\def\QTilsM2.def"
  File /nonfatal "S:\MacOSX\QTilities\QTils\Mod2\def\POSIXm2.def"
SectionEnd

Section "Monaco font" SEC05
  StrCpy $FONT_DIR $FONTS
  !insertmacro InstallTTF "S:\MacOSX\QTImage2Mov\Monaco.ttf"
  SendMessage ${HWND_BROADCAST} ${WM_FONTCHANGE} 0 0 /TIMEOUT=5000
SectionEnd

;Section -AdditionalIcons
;  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
;  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
;  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk" "$INSTDIR\uninst.exe"
;  !insertmacro MUI_STARTMENU_WRITE_END
;SectionEnd

;Section -Post
;  WriteUninstaller "$INSTDIR\uninst.exe"
;  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\QTVODm2.exe"
;  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
;  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
;  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\QTVODm2.exe"
;  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
;  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
;  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
;SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} $(Sec1Descr)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC08} $(Sec8Descr)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} $(Sec2Descr)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} $(Sec3Descr)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC04} $(Sec4Descr)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC05} $(Sec5Descr)
!insertmacro MUI_FUNCTION_DESCRIPTION_END


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
!insertmacro MUI_UNGETLANGUAGE
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

;Section Uninstall
;  !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
;  Delete "$INSTDIR\uninst.exe"
;  Delete "$INSTDIR\QTImage2Mov-v1.1.pdf"
;  Delete "$INSTDIR\QTImage2Mov.qtx"
;  Delete "$INSTDIR\QTilities-v1.2.pdf"
;  Delete "$INSTDIR\POSIXm2.dll"
;  Delete "$INSTDIR\QTils.dll"
;  Delete "$INSTDIR\VODdesign.xml"
;  Delete "$INSTDIR\LanceQTVODm2.bat"
;  Delete "$INSTDIR\QTVODm2.exe"
;
;  Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk"
;
;  RMDir "$SMPROGRAMS\$ICONS_GROUP"
;  RMDir "$INSTDIR"
;
;  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
;  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
;  SetAutoClose true
;SectionEnd
