@echo off
REM Push OTA firmware to Watcher device
REM Usage: ota_push_upload.bat [device-host-or-ip]

setlocal enabledelayedexpansion
cd /d "%~dp0"

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=192.168.1.7"

if not exist "build\watcher.bin" (
    echo ERROR: build\watcher.bin not found
    echo Run: idf.py build
    exit /b 1
)

echo [1/3] Arming OTA mode on %TARGET% ...
curl --silent --show-error --fail ^
  -X POST "http://%TARGET%/api/cmd" ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"ota\"}"
if errorlevel 1 (
    echo.
    echo ERROR: Failed to arm OTA mode.
    exit /b 1
)

echo.
echo [2/3] Uploading build\watcher.bin ...
curl --silent --show-error --fail ^
  -X POST "http://%TARGET%/api/ota/upload" ^
  -H "Content-Type: application/octet-stream" ^
  --data-binary "@build\watcher.bin"
if errorlevel 1 (
    echo.
    echo ERROR: OTA upload failed.
    exit /b 1
)

echo.
echo [3/3] Upload complete. Device should reboot automatically.
exit /b 0
