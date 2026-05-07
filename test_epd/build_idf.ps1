#!/usr/bin/env pwsh
# Helper: source ESP-IDF environment and build the project
param()

if (-not $env:IDF_PATH -or -not (Test-Path $env:IDF_PATH)) {
    Write-Host "IDF_PATH is not set or not found."
    $default = "$env:USERPROFILE\\esp\\esp-idf"
    if (Test-Path $default) {
        Write-Host "Found esp-idf at $default — setting IDF_PATH."
        $env:IDF_PATH = $default
    } else {
        Write-Error "ESP-IDF not found. Install ESP-IDF and set the IDF_PATH environment variable."
        exit 1
    }
}

$export = Join-Path $env:IDF_PATH 'export.ps1'
if (-not (Test-Path $export)) {
    Write-Error "export.ps1 not found at $export. Ensure ESP-IDF is installed correctly."
    exit 1
}

Write-Host "Sourcing $export"
& $export

Write-Host "Running idf.py build"
idf.py build
