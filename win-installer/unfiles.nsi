Delete "$INSTDIR\LIBGCC_S_SEH-1.DLL"
Delete "$INSTDIR\LIBSTDC++-6.DLL"
Delete "$INSTDIR\LIBWINPTHREAD-1.DLL"
Delete "$INSTDIR\QT5CORE.DLL"
Delete "$INSTDIR\QT5GUI.DLL"
Delete "$INSTDIR\QT5NETWORK.DLL"
Delete "$INSTDIR\QT5SVG.DLL"
Delete "$INSTDIR\QT5WIDGETS.DLL"
Delete "$INSTDIR\QT5XML.DLL"
Delete "$INSTDIR\QT5WEBSOCKETS.DLL"

Delete "$INSTDIR\platforms\QMINIMAL.DLL"
Delete "$INSTDIR\platforms\QWINDOWS.DLL"
RMDir "$INSTDIR\platforms"

Delete "$INSTDIR\imageformats\QSVG.DLL"
RMDir "$INSTDIR\imageformats"

Delete "$INSTDIR\iconengines\QSVGICON.DLL"
RMDir "$INSTDIR\iconengines"

Delete "$INSTDIR\locale\fr_FR.qm"
Delete "$INSTDIR\locale\es_ES.qm"
Delete "$INSTDIR\locale\ca_ES.qm"
RMDir "$INSTDIR\locale"

Delete "$INSTDIR\uninstall.exe"
RMDir "$INSTDIR"
