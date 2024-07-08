@echo off
set LIBTORCH_VERSION=%1
set LIBTORCH_DIR=%~dp0

set TEMP_DIR=%LIBTORCH_DIR%\temp

if not exist "%TEMP_DIR%" mkdir "%TEMP_DIR%"

powershell -command "Invoke-WebRequest -Uri https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.3.1%2Bcpu.zip -OutFile %TEMP_DIR%\libtorch.zip"

powershell -command "Expand-Archive -Path %TEMP_DIR%\libtorch.zip -DestinationPath %LIBTORCH_DIR%"

del "%TEMP_DIR%\libtorch.zip"
rmdir /s /q "%TEMP_DIR%"
