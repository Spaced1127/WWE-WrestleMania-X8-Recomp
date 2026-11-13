@echo off
setlocal
set "ROOT=%~dp0.."
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

cmake -S "%ROOT%\lib\ModernGekko-Template\lib\ModernGekko" ^
      -B "%ROOT%\build\moderngekko" ^
      -G Ninja ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DMODERNGEKKO_ENABLE_DOLPHIN_RUNTIME=ON ^
      -DMODERNGEKKO_ENABLE_IMGUI_LAUNCHER=ON ^
      -DMODERNGEKKO_ENABLE_DOLPHIN_TESTS=OFF ^
      -DMODERNGEKKO_ENABLE_DYNAMIC_MODULES=ON ^
      -DMODERNGEKKO_FRONTEND_NAME="WWE WrestleMania X8 Recomp" ^
      -DMODERNGEKKO_LAUNCHER_OUTPUT_NAME="WrestleManiaX8Recomp" ^
      -DMODERNGEKKO_RUNNER_OUTPUT_NAME="WrestleManiaX8RecompRunner" ^
      -DMODERNGEKKO_USER_DIRECTORY_NAME="WrestleManiaX8Recomp" ^
      -DMODERNGEKKO_DEFAULT_WINDOW_TITLE="WWE WrestleMania X8" ^
      -DMODERNGEKKO_LOG_FILENAME="WrestleManiaX8.log" ^
      -DMODERNGEKKO_REQUIRED_DISC_ID="GW3E78" ^
      -DMODERNGEKKO_REQUIRED_DOL_SHA256="91af3136f2a48c18a6b96a5726d1f171c9e714bb8d4d504f835070fc230a303e" ^
      -DMODERNGEKKO_PORTABLE_DEFAULT_GAME=ON ^
      -DMODERNGEKKO_GAMECUBE_CONTROLLERS=ON
if errorlevel 1 exit /b 1

cmake --build "%ROOT%\build\moderngekko" --target moderngekko-launcher moderngekko-run -j 16
if errorlevel 1 exit /b 1

echo BUILD_OK
