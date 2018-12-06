@echo off

>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"

if '%errorlevel%' NEQ '0' (

echo 请求管理员权限...

goto UACPrompt

) else ( goto gotAdmin )

:UACPrompt

echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"

echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getadmin.vbs"

"%temp%\getadmin.vbs"

exit /B

:gotAdmin
md "C:\Program Files (x86)\file_search"
xcopy /y /c /h /r /s "%~dp0*.*" "C:\Program Files (x86)\file_search\"


sc create file_search777 binPath= "C:\Program Files (x86)\file_search\file_search.exe"
sc start file_search777
sc config file_search777 start=auto

REG ADD "HKEY_CLASSES_ROOT\Directory\Background\shell\search file"  /T REG_SZ /D "搜索此目录" /F
REG ADD "HKEY_CLASSES_ROOT\Directory\Background\shell\search file" /V Icon /T REG_SZ /D "C:\Program Files (x86)\file_search\search_file.exe" /F
REG ADD "HKEY_CLASSES_ROOT\Directory\Background\shell\search file\command"  /T REG_SZ /D "C:\Program Files (x86)\file_search\search_file.exe  \"%%v.\"" /F

c:
cd  "C:\Program Files (x86)\file_search"
start desktop_icon.VBS