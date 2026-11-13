@echo off
setlocal
set "ROOT=%~dp0.."
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "PATH=C:\Program Files\LLVM\bin;C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\tools\pgo_cycle.ps1" -TrainSeconds 75
if errorlevel 1 exit /b 1
echo PGO_CYCLE_OK
