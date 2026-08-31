@echo off
rem ==== Autostart wrapper (registry Run key calls this at logon) ====
rem 1) already listening on 8000? skip to avoid duplicate instances
rem 2) otherwise launch the server as a detached hidden process
rem NOTE: keep this file pure ASCII - cmd parses bat in GBK codepage
cd /d "%~dp0"
netstat -ano | findstr ":8000" | findstr "LISTENING" >nul && exit /b 0
set NOPAUSE=1
powershell -NoProfile -Command "Start-Process -FilePath 'cmd.exe' -ArgumentList '/c start_server.bat' -WindowStyle Hidden -WorkingDirectory '%~dp0'"
