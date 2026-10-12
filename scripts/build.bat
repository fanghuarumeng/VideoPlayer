@echo off
setlocal
cd /d "%~dp0\.."

REM Optional: set QT_PREFIX before calling, or pass after --
REM   scripts\build.bat
REM   scripts\build.bat server
REM   scripts\build.bat client
REM   scripts\build.bat all D:\Qt\Qt5.12.11\5.12.11\msvc2017_64

set TARGET=%~1
if "%TARGET%"=="" set TARGET=all

set QT_ARG=
if not "%~2"=="" set QT_ARG=-QtPrefix "%~2"
if not "%CMAKE_PREFIX_PATH%"=="" if "%QT_ARG%"=="" set QT_ARG=-QtPrefix "%CMAKE_PREFIX_PATH%"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" -Target %TARGET% %QT_ARG%
exit /b %ERRORLEVEL%
