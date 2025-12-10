@echo off

setlocal EnableDelayedExpansion
goto start

:usage
echo.
echo Usage: %0 [/?] [^<options^>]
echo.
echo Bootstraps both MSVC and MinGW configurations and builds the project.
echo.
echo All options are passed through to the bootstrap scripts.
echo See build-msvc.bat /? and build-mingw.bat /? for available options.
echo.
goto end

:start

set "bootstrap_opts="

:options
if "_%~1_" == "_/?_"     goto usage
if "_%~1_" == "_-h_"     goto usage
if "_%~1_" == "_--help_" goto usage

if not "_%~1_" == "__" (
  set "bootstrap_opts=!bootstrap_opts! %~1"
  shift
  goto options
)

@echo on

@call build-msvc.bat %bootstrap_opts%
@if errorlevel 1 goto error

@call build-mingw.bat %bootstrap_opts%
@if errorlevel 1 goto error

@bdep update -a
@if errorlevel 1 goto error

@goto end

:error
@echo.
@endlocal
@exit /b 1

:end
@endlocal
