# Watcher OTA HTTPS Server Launcher (PowerShell)
# Safe in repo root; serves build/watcher.bin

Set-Location $PSScriptRoot

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host " WATCHER OTA SERVER LAUNCHER (PowerShell)" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# Check built firmware path
if (-not (Test-Path "build\watcher.bin")) {
    Write-Host "ERROR: build\watcher.bin not found" -ForegroundColor Red
    Write-Host "Run first: idf.py build" -ForegroundColor Yellow
    Write-Host "Then run from repo root:" -ForegroundColor Yellow
    Write-Host "  .\start_ota_server.ps1" -ForegroundColor Gray
    Read-Host "Press Enter to exit"
    exit 1
}

# Check if Python script exists
if (-not (Test-Path "start_ota_server.py")) {
    Write-Host "ERROR: start_ota_server.py not found" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Run the Python server
Write-Host "Starting HTTPS server...`n" -ForegroundColor Green
python start_ota_server.py

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nERROR: Failed to start server" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
