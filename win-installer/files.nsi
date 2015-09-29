; This list was obtained using Dependency Walker
; Do not forget to update unfiles.nsi when modified this.
File /a "C:\Windows\System32\MSVCR71.DLL"
File /a "${mingwdir}\bin\LIBGCC_S_DW2-1.DLL"
File /a "${mingwdir}\bin\LIBSTDC++-6.DLL"
File /a "${mingwdir}\bin\LIBWINPTHREAD-1.DLL"
File /a "${qtdirdesktop}\bin\ICUIN53.DLL"
File /a "${qtdirdesktop}\bin\ICUUC53.DLL"
File /a "${qtdirdesktop}\bin\ICUDT53.DLL"
File /a "${qtdirdesktop}\bin\QT5CORE.DLL"
File /a "${qtdirdesktop}\bin\QT5GUI.DLL"
File /a "${qtdirdesktop}\bin\QT5NETWORK.DLL"
File /a "${qtdirdesktop}\bin\QT5SVG.DLL"
File /a "${qtdirdesktop}\bin\QT5WIDGETS.DLL"
File /a "${qtdirdesktop}\bin\QT5XML.DLL"
File /a "${qtdirdesktop}\bin\QT5WEBSOCKETS.DLL"


CreateDirectory "$INSTDIR\platforms"
File /a "/oname=$INSTDIR\platforms\QMINIMAL.DLL" "${qtdirdesktop}\plugins\platforms\QMINIMAL.DLL"
File /a "/oname=$INSTDIR\platforms\QWINDOWS.DLL" "${qtdirdesktop}\plugins\platforms\QWINDOWS.DLL"

CreateDirectory "$INSTDIR\iconengines"
File /a "/oname=$INSTDIR\iconengines\QSVGICON.DLL" "${qtdirdesktop}\plugins\iconengines\QSVGICON.DLL"

CreateDirectory "$INSTDIR\imageformats"
File /a "/oname=$INSTDIR\imageformats\QSVG.DLL" "${qtdirdesktop}\plugins\imageformats\QSVG.DLL"

CreateDirectory "$INSTDIR\locale"
File /a "/oname=$INSTDIR\locale\fr_FR.qm" "..\locale\fr_FR.qm"
File /a "/oname=$INSTDIR\locale\es_ES.qm" "..\locale\es_ES.qm"
File /a "/oname=$INSTDIR\locale\ca_ES.qm" "..\locale\ca_ES.qm"
