@echo off

setlocal EnableDelayedExpansion
goto start

:usage
echo.
echo Usage: %0 [/?] [^<options^>]
echo Options:
echo   --install-dir ^<dir^>    Alternative installation directory.
echo   --build2-dir ^<dir^>     build2 installation directory.
echo   --msvc-dir ^<dir^>       MSVC installation directory.
echo   --recreate               Remove existing bdep state and host configurations.
echo   --jobs^|-j ^<num^>       Number of jobs to perform in parallel.
echo   --verbose ^<level^>      Diagnostics verbosity level between 0 and 6.
echo.
echo By default the batch file will install build2 into C:\build2 and
echo MSVC into C:\msvc if they are not already installed.
echo.
echo See the README file for details.
echo.
goto end

:start

set "owd=%CD%"

for %%I in (.) do set "project=%%~nxI"

set "idir=%CD%"
set "build2_dir=C:\build2"
set "msvc_dir=C:\msvc"
set "recreate="
set "jobs="
set "verbose="

:options
if "_%~1_" == "_/?_"     goto usage
if "_%~1_" == "_-h_"     goto usage
if "_%~1_" == "_--help_" goto usage

if "_%~1_" == "_--install-dir_" (
  if "_%~2_" == "__" (
    echo error: installation directory expected after --install-dir
    goto error
  )
  set "idir=%~2"
  shift
  shift
  goto options
)

if "_%~1_" == "_--build2-dir_" (
  if "_%~2_" == "__" (
    echo error: build2 directory expected after --build2-dir
    goto error
  )
  set "build2_dir=%~2"
  shift
  shift
  goto options
)

if "_%~1_" == "_--msvc-dir_" (
  if "_%~2_" == "__" (
    echo error: MSVC directory expected after --msvc-dir
    goto error
  )
  set "msvc_dir=%~2"
  shift
  shift
  goto options
)

if "_%~1_" == "_--recreate_" (
  set "recreate=true"
  shift
  goto options
)

set "jo="
if "_%~1_" == "_-j_"     set "jo=true"
if "_%~1_" == "_--jobs_" set "jo=true"

if "_%jo%_" == "_true_" (
  if "_%~2_" == "__" (
    echo error: number of jobs expected after --jobs
    goto error
  )
  set "jobs=%~2"
  shift
  shift
  goto options
)

if "_%~1_" == "_--verbose_" (
  if "_%~2_" == "__" (
    echo error: diagnostics level expected after --verbose
    goto error
  )
  set "verbose=%~2"
  shift
  shift
  goto options
)

if not "_%~1_" == "__" (
  echo error: unexpected argument: %~1
  goto usage
)

for /F "delims=|" %%D in ("%idir%") do set "idir=%%~dpnxD"
for /F "delims=|" %%D in ("%build2_dir%") do set "build2_dir=%%~dpnxD"
for /F "delims=|" %%D in ("%msvc_dir%") do set "msvc_dir=%%~dpnxD"

if not "_%jobs%_" == "__" (
  set "jobs=-j %jobs%"
)

if not "_%verbose%_" == "__" (
  set "verbose=--verbose %verbose%"
)

@echo on

@where b >nul 2>nul
@if not errorlevel 1 goto check_msvc

@echo Installing build2...

@if not exist "%TEMP%\toolchain-bindist.sha256" (
  powershell -Command "Invoke-WebRequest -Uri 'https://stage.build2.org/0/toolchain-bindist.sha256' -OutFile '%TEMP%\toolchain-bindist.sha256'"
  @if errorlevel 1 goto error
)

@for /f "tokens=1,2" %%a in ('findstr /C:"windows" "%TEMP%\toolchain-bindist.sha256"') do (
  set "bindist_sha256=%%a"
  set "bindist_name=%%b"
)

@if "_%bindist_name%_" == "__" (
  echo error: could not find Windows bindist in toolchain-bindist.sha256
  goto error
)

@set "bindist_url=https://stage.build2.org/0/0.18.0-a.0/bindist/%bindist_name%"
@set "bindist_file=%TEMP%\%bindist_name%"

@if not exist "%bindist_file%" (
  echo Downloading %bindist_url%...
  powershell -Command "Invoke-WebRequest -Uri '%bindist_url%' -OutFile '%bindist_file%'"
  @if errorlevel 1 goto error
)

@if not exist "%build2_dir%" mkdir "%build2_dir%"
@echo Extracting %bindist_file%...
@tar -xf "%bindist_file%" -C "%build2_dir%" --strip-components=1
@if errorlevel 1 goto error

@set "PATH=%build2_dir%\bin;%PATH%"

:check_msvc
@where cl >nul 2>nul
@if not errorlevel 1 goto setup_config

@if exist "%msvc_dir%\bin\Hostx64\x64\cl.exe" (
  set "PATH=%msvc_dir%\bin\Hostx64\x64;%PATH%"
  goto setup_config
)

@echo Installing MSVC...

@set "vs_buildtools_url=https://aka.ms/vs/17/release/vs_buildtools.exe"
@set "vs_buildtools_file=%TEMP%\vs_buildtools.exe"

@if not exist "%vs_buildtools_file%" (
  echo Downloading Visual Studio Build Tools...
  powershell -Command "Invoke-WebRequest -Uri '%vs_buildtools_url%' -OutFile '%vs_buildtools_file%'"
  @if errorlevel 1 goto error
)

@echo Installing MSVC components (this may take a while)...
"%vs_buildtools_file%" --quiet --wait --norestart ^
  --installPath "%msvc_dir%" ^
  --add Microsoft.VisualStudio.Workload.VCTools ^
  --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
  --add Microsoft.VisualStudio.Component.Windows11SDK.22000
@if errorlevel 1 (
  if errorlevel 3010 (
    echo warning: MSVC installation requires restart
  ) else (
    goto error
  )
)

@for /f "usebackq delims=" %%i in (`dir /b /s "%msvc_dir%\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe" 2^>nul`) do (
  set "msvc_bin=%%~dpi"
  goto found_msvc
)

:found_msvc
@if "_%msvc_bin%_" == "__" (
  echo error: could not find cl.exe after MSVC installation
  goto error
)

@set "PATH=%msvc_bin%;%PATH%"

@for /f "usebackq delims=" %%i in (`dir /b /s "%msvc_dir%\Windows Kits\*\Include\um\windows.h" 2^>nul`) do (
  set "sdk_include=%%~dpi.."
  goto found_sdk
)

:found_sdk
@if not "_%sdk_include%_" == "__" (
  for %%i in ("%sdk_include%\..") do set "sdk_root=%%~fi"
  for %%i in ("%sdk_include%") do set "sdk_version=%%~nxi"

  set "INCLUDE=%msvc_dir%\VC\Tools\MSVC\*\include;%sdk_root%\Include\%sdk_version%\um;%sdk_root%\Include\%sdk_version%\ucrt;%sdk_root%\Include\%sdk_version%\shared;%INCLUDE%"
  set "LIB=%msvc_dir%\VC\Tools\MSVC\*\lib\x64;%sdk_root%\Lib\%sdk_version%\um\x64;%sdk_root%\Lib\%sdk_version%\ucrt\x64;%LIB%"
)

:setup_config
@where b >nul 2>nul
@if errorlevel 1 (
  echo error: b not found in PATH
  goto error
)

@where cl >nul 2>nul
@if errorlevel 1 (
  echo error: cl not found in PATH
  goto error
)

@set "cache=%LOCALAPPDATA%\iw4x\bootstrap-root"

@if "%recreate%" == "true" (
  echo Recreating configurations: removing existing state...

  if exist .bdep (
    rmdir /s /q .bdep >nul 2>nul
  )

  if exist "..\%project%-host" (
    rmdir /s /q "..\%project%-host" >nul 2>nul
  )
)

@set "iw4x="
@if exist "%cache%" (
  set /p iw4x=<"%cache%"
  if not exist "!iw4x!" set "iw4x="
)

@if "%iw4x%" == "" (
  echo.
  echo Specify the absolute path to the IW4x installation root:
  set /p "iw4x=> "

  if "!iw4x!" == "" (
    echo error: empty path is not valid
    goto error
  )

  if not exist "!iw4x!" (
    echo error: directory '!iw4x!' does not exist
    goto error
  )

  if "!iw4x:~-1!" == "\" set "iw4x=!iw4x:~0,-1!"

  if not exist "%LOCALAPPDATA%\iw4x" mkdir "%LOCALAPPDATA%\iw4x"
  echo !iw4x!>"%cache%"
)

@echo.
@echo Using IW4x installation root: %iw4x%
@echo.

@cd /d "%idir%"

@rem Debug configuration.
@rem
@bdep init -C @msvc-debug %verbose% %jobs%                       ^
  config.cxx=cl                                                  ^
  config.cc.coptions="/Z7                                        ^
                      /MTd                                       ^
                      /Od                                        ^
                      /Oi                                        ^
                      /EHsc"                                     ^
  config.cc.loptions="/DEBUG:FULL                                ^
                      /INCREMENTAL:NO"                           ^
  cc                                                             ^
  config.install.filter="include/@false lib/@false share/@false" ^
  config.install.root="%iw4x%"                                   ^
  config.install.bin="%iw4x%"                                    ^
  --wipe                                                         ^
  -- config.libiw4x.cpptrace=true
@if errorlevel 1 goto error

@rem Release configuration.
@rem
@bdep init -C @msvc-release %verbose% %jobs%                     ^
  config.cxx=cl                                                  ^
  config.cc.coptions="/O2                                        ^
                      /Z7                                        ^
                      /MT                                        ^
                      /Oi                                        ^
                      /GL                                        ^
                      /EHsc"                                     ^
  config.cc.loptions="/DEBUG:FULL                                ^
                      /INCREMENTAL:NO                            ^
                      /LTCG                                      ^
                      /OPT:REF                                   ^
                      /OPT:ICF"                                  ^
  cc                                                             ^
  config.install.filter="include/@false lib/@false share/@false" ^
  config.install.root="%iw4x%"                                   ^
  config.install.bin="%iw4x%"                                    ^
  --wipe                                                         ^
  -- config.libiw4x.cpptrace=true
@if errorlevel 1 goto error

@echo.
@echo Build configurations created.
@echo.

@goto end

:error
@echo.
@endlocal
@exit /b 1

:end
@endlocal
