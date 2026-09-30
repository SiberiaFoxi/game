@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul

rem ============================================================================
rem  S.T.A.L.K.E.R.: Тень Чернобыля — мод для GTA San Andreas
rem  Запуск: проверка игры, клонирование gta-reversed, сборка, установка, старт.
rem
rem  Использование:
rem     run_stalker.bat              — сборка + установка + запуск
rem     run_stalker.bat --no-build   — запуск без сборки (если уже собрано)
rem     run_stalker.bat --no-run     — только сборка и установка
rem ============================================================================

rem --- Папка игры: рядом с bat-файлом, либо укажите здесь вручную ---
set "GAME_DIR="
rem set "GAME_DIR=C:\Games\GTA San Andreas"

set "REPO_DIR=%~dp0"
set "REPO_DIR=%REPO_DIR:~0,-1%"
set "BUILD_DIR=%REPO_DIR%\build"
set "MOD_NAME=stalker_mod.asi"

rem --- Аргументы ---
set "DO_BUILD=1"
set "DO_RUN=1"
:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="--no-build" set "DO_BUILD=0"
if /i "%~1"=="--no-run"   set "DO_RUN=0"
shift
goto parse_args
:args_done

echo.
echo ============================================================
echo   S.T.A.L.K.E.R. mod for GTA San Andreas (1.0 US)
echo ============================================================
echo.

rem ----------------------------------------------------------------------------
rem 1. Проверка наличия gta_sa.exe
rem ----------------------------------------------------------------------------
if not defined GAME_DIR set "GAME_DIR=%REPO_DIR%"
if not exist "%GAME_DIR%\gta_sa.exe" (
    rem Ищем gta_sa.exe в текущей папке bat-файла
    if not "%GAME_DIR%"=="%REPO_DIR%" goto no_game
    cd /d "%REPO_DIR%"
    if exist "gta_sa.exe" (
        set "GAME_DIR=%REPO_DIR%"
    ) else (
        rem Проверка: bat лежит в подпапке, игра на уровень выше?
        if exist "%REPO_DIR%\..\gta_sa.exe" (
            for %%I in ("%REPO_DIR%\..") do set "GAME_DIR=%%~fI"
        ) else (
            goto no_game
        )
    )
)
echo [1/6] gta_sa.exe найден: %GAME_DIR%

rem Проверка версии игры (размер exe 1.0 US ~ 14 383 104 байт; предупреждаем,
rem если сильно отличается)
if exist "%GAME_DIR%\gta_sa.exe" (
    for %%F in ("%GAME_DIR%\gta_sa.exe") do set "EXE_SIZE=%%~zF"
    if not "!EXE_SIZE!"=="14383104" (
        echo       ВНИМАНИЕ: размер gta_sa.exe !EXE_SIZE! байт.
        echo       Мод рассчитан на GTA SA 1.0 US (14 383 104 байт).
        echo       Другие версии НЕ поддерживаются и могут упасть.
    )
)

rem ----------------------------------------------------------------------------
rem 2. Клонирование gta-reversed (база проекта)
rem ----------------------------------------------------------------------------
if not exist "%REPO_DIR%\gta-reversed\CMakeLists.txt" (
    echo [2/6] Клонирую gta-reversed...
    where git >nul 2>nul
    if errorlevel 1 (
        echo       ОШИБКА: git не найден. Установите Git для Windows:
        echo       https://git-scm.com/download/win
        goto fail
    )
    git clone --depth 1 https://github.com/gta-reversed/gta-reversed "%REPO_DIR%\gta-reversed"
    if errorlevel 1 (
        echo       ОШИБКА: не удалось клонировать gta-reversed.
        echo       Проверьте интернет-соединение.
        goto fail
    )
) else (
    echo [2/6] gta-reversed уже склонирован.
)
rem Справка: как собрать сам gta-reversed (необязательно!) — см. README.md.
rem Наш мод — самостоятельный ASI и НЕ требует сборки gta-reversed.

rem ----------------------------------------------------------------------------
rem 3. Сборка stalker_mod.asi (CMake + VS2022, Win32)
rem ----------------------------------------------------------------------------
if "%DO_BUILD%"=="1" (
    echo [3/6] Сборка %MOD_NAME%...

    where cmake >nul 2>nul
    if errorlevel 1 (
        echo       ОШИБКА: cmake не найден. Установите CMake или VS2022:
        echo       https://cmake.org/download/  ^|  https://visualstudio.microsoft.com/
        goto fail
    )

    cmake -B "%BUILD_DIR%" -A Win32 -S "%REPO_DIR%"
    if errorlevel 1 (
        echo       ОШИБКА: конфигурация CMake не удалась.
        echo       Убедитесь, что установлен Visual Studio 2022 с компонентом
        echo       «Разработка классических приложений на C++» (x86 tools).
        goto fail
    )

    cmake --build "%BUILD_DIR%" --config Release
    if errorlevel 1 (
        echo       ОШИБКА: сборка не удалась. Сообщите об этом в issues проекта.
        goto fail
    )
) else (
    echo [3/6] Сборка пропущена (--no-build).
)

rem ----------------------------------------------------------------------------
rem 4. Копирование stalker_mod.asi и ASI Loader
rem ----------------------------------------------------------------------------
echo [4/6] Установка мода в игру...

if not exist "%BUILD_DIR%\Release\%MOD_NAME%" (
    echo       ОШИБКА: %BUILD_DIR%\Release\%MOD_NAME% не найден.
    echo       Сначала соберите мод: run_stalker.bat
    goto fail
)
copy /y "%BUILD_DIR%\Release\%MOD_NAME%" "%GAME_DIR%\%MOD_NAME%" >nul
if errorlevel 1 goto copy_fail

rem ASI Loader: берём из contrib/plugins.zip gta-reversed (dinput8.dll)
if not exist "%GAME_DIR%\dinput8.dll" (
    if exist "%REPO_DIR%\gta-reversed\contrib\plugins.zip" (
        echo       Распаковываю ASI Loader (dinput8.dll) из gta-reversed...
        powershell -NoProfile -Command "Expand-Archive -Force -LiteralPath '%REPO_DIR%\gta-reversed\contrib\plugins.zip' -DestinationPath '%TEMP%\stalker_asi_loader'" >nul 2>nul
        if exist "%TEMP%\stalker_asi_loader\dinput8.dll" (
            copy /y "%TEMP%\stalker_asi_loader\dinput8.dll" "%GAME_DIR%\" >nul
        )
        rem vorbisFile.dll тоже полезен (Mouse Fix и утилиты реверса)
        if exist "%TEMP%\stalker_asi_loader\vorbisFile.dll" (
            copy /y "%TEMP%\stalker_asi_loader\vorbisFile.dll" "%GAME_DIR%\" >nul
        )
    ) else (
        echo       ВНИМАНИЕ: dinput8.dll (ASI Loader) не найден.
        echo       Скачайте любой ASI Loader и положите dinput8.dll в папку игры.
    )
) else (
    echo       ASI Loader (dinput8.dll) уже установлен.
)

rem ----------------------------------------------------------------------------
rem 5. Копирование данных мода (конфиги + модели)
rem ----------------------------------------------------------------------------
echo [5/6] Копирую данные мода (STALKER_MOD)...
if not exist "%GAME_DIR%\STALKER_MOD" mkdir "%GAME_DIR%\STALKER_MOD"
if not exist "%GAME_DIR%\STALKER_MOD\models" mkdir "%GAME_DIR%\STALKER_MOD\models"

if exist "%REPO_DIR%\scripts\*.cfg" copy /y "%REPO_DIR%\scripts\*.cfg" "%GAME_DIR%\STALKER_MOD\" >nul

rem Модели мутантов из assets/models (если скачаны пользователем)
if exist "%REPO_DIR%\assets\models\" (
    xcopy /y /e /i "%REPO_DIR%\assets\models" "%GAME_DIR%\STALKER_MOD\models" >nul
)

echo       Готово. Конфиги: %GAME_DIR%\STALKER_MOD\*.cfg

rem ----------------------------------------------------------------------------
rem 6. Запуск игры
rem ----------------------------------------------------------------------------
if "%DO_RUN%"=="1" (
    echo [6/6] Запускаю GTA San Andreas...
    start "" /d "%GAME_DIR%" "gta_sa.exe"
    echo       Игра запущена. Лог мода: STALKER_MOD\stalker_mod.log
) else (
    echo [6/6] Запуск отключён (--no-run).
)

echo.
echo ============================================================
echo   Удачной охоты, сталкер!
echo   Читы: STALKERHEALTH STALKERARMOR STALKERARTIFACT STALKERRAD
echo         STALKERWEATHER STALKERDETECTOR STALKERNOMUTANTS
echo ============================================================
goto end

:no_game
echo.
echo ОШИБКА: gta_sa.exe не найден.
echo Положите run_stalker.bat в папку GTA San Andreas, либо укажите
echo путь в переменной GAME_DIR в начале этого файла.
goto fail

:copy_fail
echo.
echo ОШИБКА: не удалось скопировать файлы в %GAME_DIR%.
echo Возможно, игра запущена или нет прав на запись.
goto fail

:fail
echo.
pause
exit /b 1

:end
pause
exit /b 0
