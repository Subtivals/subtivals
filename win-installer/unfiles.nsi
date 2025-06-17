Delete "$INSTDIR\myapp.exe"
Delete "$INSTDIR\config.ini"
Delete "$INSTDIR\*.dll"
Delete "$INSTDIR\*.txt"

RMDir /r "$INSTDIR\platforms"
RMDir /r "$INSTDIR\imageformats"
RMDir /r "$INSTDIR\iconengines"
RMDir /r "$INSTDIR\locale"

RMDir /r "$INSTDIR"
