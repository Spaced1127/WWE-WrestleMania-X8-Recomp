@echo off
setlocal
set "ROOT=%~dp0.."
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "PATH=C:\Program Files\LLVM\bin;C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

"%ROOT%\build\moderngekko\moderngekko-port.exe" build ^
  "%ROOT%\work\game" ^
  --backend c --toolchain clang --opt-level 3 ^
  --output "%ROOT%\build\modules-opt"
if errorlevel 1 exit /b 1
echo BUILD_OK
