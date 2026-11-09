@echo off
chcp 65001 > nul
echo ===================================================
echo  TeamGame Web中継サーバーを起動しています...
echo ===================================================
python "%~dp0relay_server.py"
pause
