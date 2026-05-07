@echo off
REM Watcher OTA HTTPS Server Launcher (Windows batch)
REM Safe in repo root; serves build\watcher.bin

setlocal enabledelayedexpansion
cd /d "%~dp0"

echo.
echo ================================================
echo  WATCHER OTA SERVER LAUNCHER
echo ================================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found in PATH
    echo Install Python from https://www.python.org/ or use esp-idf Python
    pause
    exit /b 1
)

REM Check built firmware path
if not exist "build\watcher.bin" (
    echo ERROR: build\watcher.bin not found
    echo Run first:
    echo   idf.py build
    echo Then run this script from repo root:
    echo   start_ota_server.bat
    pause
    exit /b 1
)

REM Check for Python script
if not exist "start_ota_server.py" (
    echo ERROR: start_ota_server.py not found
    pause
    exit /b 1
)

REM Run the Python server
echo Starting HTTPS server...
echo.
python start_ota_server.py
if errorlevel 1 (
    echo.
    echo ERROR: Failed to start server
    pause
    exit /b 1
)
