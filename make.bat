@echo off
@if "%VS120COMNTOOLS%"=="" goto :error_no_vs_2013
@call "%VS120COMNTOOLS%\vsvars32.bat"

REM Release

devenv opencontig.sln /Build "Release|win32"
@if errorlevel 1 goto :error_broken_compilation
devenv opencontig.sln /Build "Release|x64"
@if errorlevel 1 goto :error_broken_compilation

REM Debug

devenv opencontig.sln /Build "Debug|win32"
@if errorlevel 1 goto :error_broken_compilation
devenv opencontig.sln /Build "Debug|x64"
@if errorlevel 1 goto :error_broken_compilation


@goto end
:error_no_vs_2013
@set errorlevel=1
@echo VS120COMNTOOLS variable is not set, VS2013 needed.
@goto end
:error_broken_compilation
@echo ERROR: Can not build solution. 
@goto end

:end
pause