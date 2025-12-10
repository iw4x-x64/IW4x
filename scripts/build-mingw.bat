@echo off

setlocal EnableDelayedExpansion
goto start

:usage
echo.
echo Usage: %0 [/?] [^<options^>]
echo Options:
echo   --install-dir ^<dir^>    Alternative installation directory.
echo   --build2-dir ^<dir^>     build2 installation directory.
echo   --mingw-dir ^<dir^>      MinGW installation directory.
echo   --recreate               Remove existing bdep state and host configurations.
echo   --jobs^|-j ^<num^>       Number of jobs to perform in parallel.
echo   --verbose ^<level^>      Diagnostics verbosity level between 0 and 6.
echo.
echo By default the batch file will install build2 into C:\build2 and
echo MinGW into C:\mingw64 if they are not already installed.
echo.
echo See the README file for details.
echo.
goto end

:start

set "owd=%CD%"

for %%I in (.) do set "project=%%~nxI"

set "idir=%CD%"
set "build2_dir=C:\build2"
set "mingw_dir=C:\mingw64"
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

if "_%~1_" == "_--mingw-dir_" (
  if "_%~2_" == "__" (
    echo error: MinGW directory expected after --mingw-dir
    goto error
  )
  set "mingw_dir=%~2"
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
for /F "delims=|" %%D in ("%mingw_dir%") do set "mingw_dir=%%~dpnxD"

if not "_%jobs%_" == "__" (
  set "jobs=-j %jobs%"
)

if not "_%verbose%_" == "__" (
  set "verbose=--verbose %verbose%"
)

@echo on

@where b >nul 2>nul
@if not errorlevel 1 goto check_mingw

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

:check_mingw
@where x86_64-w64-mingw32-g++ >nul 2>nul
@if not errorlevel 1 goto setup_config

@if exist "%mingw_dir%\bin\x86_64-w64-mingw32-g++.exe" (
  set "PATH=%mingw_dir%\bin;%PATH%"
  goto setup_config
)

@echo Installing MinGW...

@set "msys2_url=https://github.com/msys2/msys2-installer/releases/download/nightly-x86_64/msys2-x86_64-latest.exe"
@set "msys2_file=%TEMP%\msys2-x86_64-latest.exe"

@if not exist "%msys2_file%" (
  echo Downloading MSYS2...
  powershell -Command "Invoke-WebRequest -Uri '%msys2_url%' -OutFile '%msys2_file%'"
  @if errorlevel 1 goto error
)

@echo Installing MSYS2 (this may take a while)...
"%msys2_file%" install --root "%mingw_dir%" --confirm-command
@if errorlevel 1 goto error

@echo Installing MinGW toolchain...
"%mingw_dir%\usr\bin\bash.exe" -lc "pacman -Syu --noconfirm"
@if errorlevel 1 goto error

"%mingw_dir%\usr\bin\bash.exe" -lc "pacman -S --noconfirm mingw-w64-x86_64-toolchain"
@if errorlevel 1 goto error

@set "PATH=%mingw_dir%\mingw64\bin;%PATH%"

:setup_config
@where b >nul 2>nul
@if errorlevel 1 (
  echo error: b not found in PATH
  goto error
)

@where x86_64-w64-mingw32-g++ >nul 2>nul
@if errorlevel 1 (
  echo error: x86_64-w64-mingw32-g++ not found in PATH
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

  if "!iw4x!:~-1!" == "\" set "iw4x=!iw4x:~0,-1!"

  if not exist "%LOCALAPPDATA%\iw4x" mkdir "%LOCALAPPDATA%\iw4x"
  echo !iw4x!>"%cache%"
)

@echo.
@echo Using IW4x installation root: %iw4x%
@echo.

@cd /d "%idir%"

@rem Debug configuration.
@rem
@bdep init -C @mingw32-debug %verbose% %jobs%                    ^
  config.cxx=x86_64-w64-mingw32-g++                              ^
  config.cc.coptions="-ggdb                                      ^
                      -grecord-gcc-switches                      ^
                      -pipe                                      ^
                      -mtune=generic                             ^
                      -fasynchronous-unwind-tables               ^
                      -fno-omit-frame-pointer                    ^
                      -mno-omit-leaf-frame-pointer"              ^
  config.cc.compiledb=./                                         ^
  cc                                                             ^
  config.install.filter="include/@false lib/@false share/@false" ^
  config.install.root="%iw4x%"                                   ^
  config.install.bin="%iw4x%"                                    ^
  --wipe                                                         ^
  -- config.libiw4x.cpptrace=true
@if errorlevel 1 goto error

@rem Release configuration.
@rem
@bdep init -C @mingw32-release %verbose% %jobs%                  ^
  config.cxx=x86_64-w64-mingw32-g++                              ^
  config.cc.coptions="-O2                                        ^
                      -ggdb                                      ^
                      -grecord-gcc-switches                      ^
                      -pipe                                      ^
                      -mtune=generic                             ^
                      -fasynchronous-unwind-tables               ^
                      -fno-omit-frame-pointer                    ^
                      -mno-omit-leaf-frame-pointer"              ^
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
