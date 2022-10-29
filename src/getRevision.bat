@echo off
set "SCRIPT_DIRECTORY=%~dp0"

for /f "delims=" %%A in ('git rev-parse --short HEAD') do set "rev=%%A"
echo current revision %rev%

REM Skip writing to the file if it matches our current revision.
set /p existingRev=< "%SCRIPT_DIRECTORY%Revision.h"
set newRev=#define PLUGIN_REVISION "%rev%"
if "%existingRev%"=="%newRev%" goto DONE

echo #define PLUGIN_REVISION "%rev%">"%SCRIPT_DIRECTORY%Revision.h"
echo #define PLUGIN_REVISION_W L"%rev%">>"%SCRIPT_DIRECTORY%Revision.h"

:DONE