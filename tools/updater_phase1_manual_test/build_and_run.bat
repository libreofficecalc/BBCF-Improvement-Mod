@echo off
setlocal
set ROOT=%~dp0..\..
set OUT=%ROOT%\build\updater_phase1_manual_test
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /EHsc /W4 /DUNICODE /D_UNICODE /I"%ROOT%\src" ^
  "%ROOT%\tools\updater_phase1_manual_test\updater_phase1_manual_test.cpp" ^
  "%ROOT%\src\Updater\SemVersion.cpp" ^
  "%ROOT%\src\Updater\UpdateManifest.cpp" ^
  "%ROOT%\src\Updater\UpdateStateStore.cpp" ^
  /Fe:"%OUT%\updater_phase1_manual_test.exe"
if errorlevel 1 exit /b 1
"%OUT%\updater_phase1_manual_test.exe"
