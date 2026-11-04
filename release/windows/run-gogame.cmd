@echo off
setlocal
set "game_root=%~dp0"
set "model_path=%GOGAME_KATAGO_MODEL%"
if not defined model_path set "model_path=%game_root%models\model.bin.gz"

if not exist "%model_path%" (
    echo Model not found: "%model_path%" 1>&2
    exit /b 2
)

"%game_root%appGoGame.exe" --katago-model "%model_path%" %*
exit /b %errorlevel%
