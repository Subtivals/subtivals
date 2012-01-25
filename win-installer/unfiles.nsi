Delete "$INSTDIR\MSVCR71.DLL"
Delete "$INSTDIR\LIBGCC_S_DW2-1.DLL"
Delete "$INSTDIR\MINGWM10.DLL"
Delete "$INSTDIR\QTCORE4.DLL"
Delete "$INSTDIR\QTGUI4.DLL"
Delete "$INSTDIR\QTSVG4.DLL"

Delete "$INSTDIR\imageformats\QSVG4.DLL"
RMDir "$INSTDIR\imageformats"

Delete "$INSTDIR\iconengines\QSVGICON4.DLL"
RMDir "$INSTDIR\iconengines"

Delete "$INSTDIR\locale\fr_FR.qm"
Delete "$INSTDIR\locale\es_ES.qm"
Delete "$INSTDIR\locale\ca_ES.qm"
RMDir "$INSTDIR\locale"

Delete "$INSTDIR\uninstall.exe"
RMDir "$INSTDIR"
