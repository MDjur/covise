@ECHO OFF

ECHO starting qmake


SET TMPFILE="%TMP%\~covtmpfile.txt"
CALL "%WINDIR%\system32\cscript.exe" /nologo "%~dp0\get8dot3path.vbs" "%~dp0">%TMPFILE%
SET /P INSTDIR=<%TMPFILE%
DEL /Q %TMPFILE%
ECHO COVISEDIR=%COVISEDIR%

CALL "%~dp0\..\combinePaths.bat"
CALL common.VISENSO.bat amdwin64opt %COVISEDIR%

SET COFRAMEWORKDIR=%COVISEDIR%
ECHO COFRAMEWORKDIR=%COFRAMEWORKDIR%

CD /D %COVISEDIR%\src\application\
qmake -r 


GOTO END

:END