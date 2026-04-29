@echo off
setlocal

set "RESEARCH=%~dp0"
set "PROJECT_DIR=%RESEARCH%BBCF-Ghidra-Project"
set "PROJECT_NAME=BBCF"
set "GHIDRA_HEADLESS=D:\Programs\ghidra_11.4.2_PUBLIC\support\analyzeHeadless.bat"
set "BBCF_EXE=D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF.exe"
set "SCRIPT_DIR=%RESEARCH%ghidra_scripts"
set "REPORT=%RESEARCH%RankedLpGhidraReport.txt"

"%GHIDRA_HEADLESS%" "%PROJECT_DIR%" "%PROJECT_NAME%" -import "%BBCF_EXE%" -overwrite -analysisTimeoutPerFile 1800 -scriptPath "%SCRIPT_DIR%" -postScript DecompileRankedLp.py "%REPORT%"
