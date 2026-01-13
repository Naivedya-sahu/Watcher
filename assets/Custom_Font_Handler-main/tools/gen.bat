@echo off
setlocal enabledelayedexpansion

echo ========================================================================
echo fonts.h Declaration Generator - Recursive Version
echo ========================================================================
echo.

REM Get current directory
set "current_dir=%CD%"
echo Scanning directory: %current_dir%
echo.

set count=0
set output_file=fonts_declarations.txt

REM Clear/create output file
echo // fonts.h Declarations - Auto-generated > %output_file%
echo // Copy this block into fonts.h >> %output_file%
echo // Generated from: %current_dir% >> %output_file%
echo. >> %output_file%

echo Scanning for .cpp files (including subdirectories)...
echo.

REM Process each .cpp file recursively
for /r %%f in (*.cpp) do (
    set "filepath=%%f"
    set "filename=%%~nf"
    set "relpath=%%f"
    
    REM Get relative path from current directory
    set "relpath=!relpath:%current_dir%\=!"
    
    REM Skip built-in font files (font8, font12, font16, font20, font24)
    echo !filename! | findstr /i /r "^font[0-9][0-9]*$" >nul
    if errorlevel 1 (
        REM Search for sFONT pattern in file
        findstr /C:"sFONT " "%%f" >nul
        if not errorlevel 1 (
            REM Extract the font name from "sFONT FontName = {"
            for /f "tokens=2 delims= " %%s in ('findstr /C:"sFONT " "%%f"') do (
                set "fontname=%%s"
                REM Remove everything after = or {
                for /f "tokens=1 delims=={" %%n in ("!fontname!") do (
                    set "cleanname=%%n"
                    REM Remove trailing spaces
                    set "cleanname=!cleanname: =!"
                    
                    if defined cleanname (
                        REM Convert to uppercase for define
                        set "definename=!cleanname!"
                        call :ToUpper definename
                        
                        REM Display to console with path info
                        echo [92mFound:[0m !cleanname! 
                        echo       File: !relpath!
                        echo.
                        
                        REM Write to output file
                        echo // !cleanname! from !relpath! >> %output_file%
                        echo #ifdef FONT_ENABLE_!definename! >> %output_file%
                        echo   extern sFONT !cleanname!; >> %output_file%
                        echo #endif >> %output_file%
                        echo. >> %output_file%
                        
                        set /a count+=1
                    )
                )
            )
        )
    )
)

echo ========================================================================
echo Summary
echo ========================================================================
echo Total fonts found: !count!
echo Output file: %output_file%
echo.

if !count! GTR 0 (
    echo ========================================================================
    echo Generated Declarations Preview:
    echo ========================================================================
    echo.
    type %output_file%
    echo.
    echo ========================================================================
    echo Next Steps:
    echo ========================================================================
    echo.
    echo 1. Open file: %output_file%
    echo 2. Copy all declarations
    echo 3. Open: fonts.h
    echo 4. Find insertion point:
    echo      #ifdef __cplusplus
    echo      }
    echo      #endif
    echo 5. Paste declarations BEFORE #ifdef __cplusplus
    echo 6. Save fonts.h
    echo.
    echo ========================================================================
    echo Arduino Usage Example:
    echo ========================================================================
    echo.
    echo // In your sketch (.ino file):
    echo #define FONT_ENABLE_YOURFONTNAME
    echo #include ^<fonts.h^>
    echo.
    echo void setup(^) {
    echo     Paint_DrawString_EN(10, 50, "Hello", ^&YourFontName, BLACK, WHITE^);
    echo }
    echo.
) else (
    echo No custom fonts found in this directory tree
    echo Make sure you are in the correct directory:
    echo   esp32-waveshare-epd_mod\src\fonts\
    del %output_file% 2>nul
)

echo ========================================================================
pause
goto :eof

REM Function to convert to uppercase
:ToUpper
setlocal enabledelayedexpansion
set "str=!%~1!"
set "str=!str:a=A!"
set "str=!str:b=B!"
set "str=!str:c=C!"
set "str=!str:d=D!"
set "str=!str:e=E!"
set "str=!str:f=F!"
set "str=!str:g=G!"
set "str=!str:h=H!"
set "str=!str:i=I!"
set "str=!str:j=J!"
set "str=!str:k=K!"
set "str=!str:l=L!"
set "str=!str:m=M!"
set "str=!str:n=N!"
set "str=!str:o=O!"
set "str=!str:p=P!"
set "str=!str:q=Q!"
set "str=!str:r=R!"
set "str=!str:s=S!"
set "str=!str:t=T!"
set "str=!str:u=U!"
set "str=!str:v=V!"
set "str=!str:w=W!"
set "str=!str:x=X!"
set "str=!str:y=Y!"
set "str=!str:z=Z!"
endlocal & set "%~1=%str%"
goto :eof