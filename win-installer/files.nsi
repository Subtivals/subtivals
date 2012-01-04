; This list was obtained using Dependency Walker
; Do not forget to update unfiles.nsi when modified this.
File /a "C:\Windows\System32\MSVCR71.DLL"
File /a "${qtdirdesktop}\bin\LIBGCC_S_DW2-1.DLL"
File /a "${qtdirdesktop}\bin\MINGWM10.DLL"
File /a "${qtdirdesktop}\bin\QTCORE4.DLL"
File /a "${qtdirdesktop}\bin\QTGUI4.DLL"
File /a "${qtdirdesktop}\bin\QTSVG4.DLL"

CreateDirectory "$INSTDIR\iconengines"
File /a "/oname=$INSTDIR\iconengines\QSVGICON4.DLL" "${qtdirdesktop}\plugins\iconengines\QSVGICON4.DLL"

CreateDirectory "$INSTDIR\imageformats"
File /a "/oname=$INSTDIR\imageformats\QSVG4.DLL" "${qtdirdesktop}\plugins\imageformats\QSVG4.DLL"

;CreateDirectory "$INSTDIR\locale"
;File /a "/oname=$INSTDIR\locale\subtivals-fr.qm" "${srcdir}\..\..\locale\subtivals-fr.qm"
