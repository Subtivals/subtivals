; basic script template for NSIS installers
;
; Written by Philip Chu
; Modified by Mathieu Leplatre
;
; License: zlib/libpng-style (see original header)
 
RequestExecutionLevel admin

!include "MUI2.nsh" ; Modern UI 2

!ifndef PACKAGE_VERSION
  !define PACKAGE_VERSION "dev"
!endif
!echo "Building RactiSubs version ${PACKAGE_VERSION}"

!define setup "ractisubs-${PACKAGE_VERSION}.exe"
!define srcdir "deploy"
!define prodname "Racti Subs"
!define website "https://racti.com"
!define exec "ractisubs.exe"
!define licensefile "..\LICENSE"
!define regkey "Software\${prodname}"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}"
!define startmenu "$SMPROGRAMS\${prodname}"
!define uninstaller "uninstall.exe"

XPStyle on
ShowInstDetails hide
ShowUninstDetails hide
 
Name "${prodname}"
Caption "${prodname}"
 
OutFile "${setup}"
 
SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal
 
InstallDir "$PROGRAMFILES\${prodname}"
InstallDirRegKey HKLM "${regkey}" ""

; ------------------------
; MUI2 Pages configuration
; ------------------------
!define MUI_ABORTWARNING

!ifdef licensefile
  !insertmacro MUI_PAGE_LICENSE "${srcdir}\${licensefile}"
!endif

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; Finish page with "Launch" checkbox
!define MUI_FINISHPAGE_RUN "$INSTDIR\${exec}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${prodname}"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
; ------------------------

;--------------------------------
 
AutoCloseWindow false
ShowInstDetails show

; Main install section
Section
  WriteRegStr HKLM "${regkey}" "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "${uninstkey}" "DisplayName" "${prodname} (remove only)"
  WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$INSTDIR\${uninstaller}"'

  WriteRegStr HKCR "${prodname}\Shell\open\command\" "" '"$INSTDIR\${exec} "%1"'

  SetOutPath $INSTDIR

  ; Main executable
  File /a "${srcdir}\${exec}"

!ifdef licensefile
  File /a "${srcdir}\${licensefile}"
!endif
!ifdef notefile
  File /a "${srcdir}\${notefile}"
!endif
!ifdef icon
  File /a "${srcdir}\${icon}"
!endif

  ; All remaining application files
  SetOutPath $INSTDIR
  File /r "${srcdir}\*.*"

  WriteUninstaller "${uninstaller}"
SectionEnd

; Uninstaller section
Section "Uninstall"
  SetShellVarContext all
  DeleteRegKey HKLM "${uninstkey}"
  DeleteRegKey HKLM "${regkey}"

  Delete "${startmenu}\*.*"
  RMDir "${startmenu}"

!ifdef licensefile
  Delete "$INSTDIR\${licensefile}"
!endif
!ifdef notefile
  Delete "$INSTDIR\${notefile}"
!endif
!ifdef icon
  Delete "$INSTDIR\${icon}"
!endif

  Delete "$INSTDIR\${exec}"

  ; Remove all other installed files and folders
  Delete "$INSTDIR\*.*"
  RMDir /r "$INSTDIR"
SectionEnd
