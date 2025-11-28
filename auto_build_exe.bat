@echo off
setlocal enabledelayedexpansion

REM ----------------------------
REM auto_build_exe.bat
REM Usage:
REM   auto_build_exe.bat [project_path] [VsDevCmd.bat] [artifact_name]
REM Example:
REM   auto_build_exe.bat . "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" xeno_host.exe
REM ----------------------------

REM Defaults
set "PROJECT=%~1"
if "%PROJECT%"=="" set "PROJECT=."

set "VSDEV=%~2"
set "ARTIFACT=%~3"
if "%ARTIFACT%"=="" set "ARTIFACT=xeno_host.exe"

set "GENERATOR=Visual Studio 17 2022"
set "ARCH=x64"
set "CONFIG=Release"
set "BUILD_DIR=build"

REM Convert PROJECT to absolute path
pushd "%PROJECT%" 2>nul
if errorlevel 1 (
    echo ERROR: Project path not found: "%PROJECT%"
    pause
    exit /b 2
)
set "PROJECT_ABS=%CD%"
popd

set "BUILD_PATH=%PROJECT_ABS%\%BUILD_DIR%"
echo Project root: %PROJECT_ABS%
echo Build dir:   %BUILD_PATH%
echo Artifact:    %ARTIFACT%
echo.

REM Optionally call VsDevCmd to setup environment
if not "%VSDEV%"=="" (
    if exist "%VSDEV%" (
        echo Calling VS dev environment: "%VSDEV%"
        call "%VSDEV%"
        if errorlevel 1 (
            echo ERROR: VsDevCmd returned error.
            pause
            exit /b 3
        )
    ) else (
        echo WARNING: VsDevCmd not found at: "%VSDEV%". Continuing without it.
    )
)

REM Check cmake exists
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: 'cmake' not found in PATH.
    echo Please install CMake and ensure it's in PATH, or run this script from Developer Command Prompt.
    pause
    exit /b 4
)

REM Ensure build directory exists
if not exist "%BUILD_PATH%" (
    md "%BUILD_PATH%"
    if errorlevel 1 (
        echo ERROR: Failed to create build directory: "%BUILD_PATH%"
        pause
        exit /b 5
    )
)

pushd "%BUILD_PATH%"

echo.
echo === Running CMake configure ===
echo cmake .. -G "%GENERATOR%" -A %ARCH%
cmake .. -G "%GENERATOR%" -A %ARCH%
if errorlevel 1 (
    echo ERROR: CMake configuration failed.
    popd
    pause
    exit /b 6
)

echo.
echo === Running CMake build (config: %CONFIG%) ===
echo cmake --build . --config %CONFIG%
cmake --build . --config %CONFIG%
if errorlevel 1 (
    echo ERROR: Build failed.
    popd
    pause
    exit /b 7
)

REM Copy artifact from build\Release\<artifact> to build\<artifact>
set "SRC_ARTIFACT=%BUILD_PATH%\%CONFIG%\%ARTIFACT%"
set "DST_ARTIFACT=%BUILD_PATH%\%ARTIFACT%"

if not exist "%SRC_ARTIFACT%" (
    echo ERROR: Built artifact not found: "%SRC_ARTIFACT%"
    popd
    pause
    exit /b 8
)

echo Copying "%SRC_ARTIFACT%" -> "%DST_ARTIFACT%"
copy /Y "%SRC_ARTIFACT%" "%DST_ARTIFACT%" >nul
if errorlevel 1 (
    echo ERROR: Failed to copy artifact.
    popd
    pause
    exit /b 9
)

REM Now remove everything else in build dir except the artifact file
echo.
echo Cleaning up build directory, keeping only "%ARTIFACT%".
for /f "delims=" %%I in ('dir /b') do (
    if /I NOT "%%I"=="%ARTIFACT%" (
        if exist "%%I\" (
            echo Removing directory: "%%I"
            rmdir /S /Q "%%I" 2>nul
            if errorlevel 1 echo Warning: failed to remove directory "%%I"
        ) else (
            echo Deleting file: "%%I"
            del /F /Q "%%I" 2>nul
            if errorlevel 1 echo Warning: failed to delete file "%%I"
        )
    )
)

echo.
echo Build finished successfully.
echo The artifact is located at:
echo   %DST_ARTIFACT%
echo.

REM Open Explorer and select the artifact
if exist "%DST_ARTIFACT%" (
    echo Opening Explorer and selecting the file...
    start "" explorer /select,"%DST_ARTIFACT%"
) else (
    echo Warning: artifact not found to open in Explorer.
)

echo.
echo Press any key to exit...
pause >nul

popd
endlocal
exit /b 0
