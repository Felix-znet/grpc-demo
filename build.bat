@echo off
setlocal enabledelayedexpansion

rem ============================================================
rem  grpc-demo one-click script
rem  Usage:
rem    build.bat             configure + build (first time, and after adding files)
rem    build.bat build       build only (recompile)
rem    build.bat rebuild     clean build dir and full rebuild
rem    build.bat server      build then start server
rem    build.bat client      build then start client
rem    build.bat run         build then start server and client
rem ============================================================

set "BUILD_DIR=build"
set "CONFIG=Release"
set "GRPC_ROOT=%~dp0depend\grpc"
set "EXE_DIR=%BUILD_DIR%\%CONFIG%"

if /i "%~1"=="rebuild" (
    echo Clearing %BUILD_DIR% and rebuilding from scratch ...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

call :configure
call :build

goto %1 2>nul
rem default (no args): stop here

:server
if not exist "%EXE_DIR%\server.exe" ( echo [Error] server.exe not found, run build first & exit /b 1 )
start "grpc-server" "%EXE_DIR%\server.exe"
exit /b 0

:client
if not exist "%EXE_DIR%\client.exe" ( echo [Error] client.exe not found, run build first & exit /b 1 )
"%EXE_DIR%\client.exe"
exit /b 0

:run
start "grpc-server" "%EXE_DIR%\server.exe"
timeout /t 2 /nobreak >nul
"%EXE_DIR%\client.exe"
exit /b 0

rem ------------------------------------------------
rem  configure (create build dir / VS project)
rem ------------------------------------------------
:configure
if not exist "%BUILD_DIR%" (
    echo [1/2] First configure: create %BUILD_DIR% ...
    cmake -S . -B %BUILD_DIR% -DCMAKE_PREFIX_PATH="%GRPC_ROOT%"
    if errorlevel 1 ( echo [Config failed] check gRPC dependency at depend\grpc & exit /b 1 )
) else (
    echo [1/2] %BUILD_DIR% exists, re-configure to pick up new src files ...
    cmake -S . -B %BUILD_DIR% >nul 2>&1
)
exit /b 0

rem ------------------------------------------------
rem  build
rem ------------------------------------------------
:build
echo [2/2] Building %CONFIG% ...
cmake --build %BUILD_DIR% --config %CONFIG%
if errorlevel 1 ( echo [Build failed] & exit /b 1 )
echo.
echo Done! Executables in: %EXE_DIR%
echo   start server:  build.bat server
echo   start client:  build.bat client
echo   run both:      build.bat run
exit /b 0