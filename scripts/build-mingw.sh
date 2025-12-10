#!/bin/sh

usage="Usage: $0 [-h|--help] [<options>]"

diag ()
{
  echo "$*" 1>&2
}

# Note that this function will execute a command with arguments that contain
# spaces but it will not print them as quoted (and neither does set -x).
#
run ()
{
  diag "+ $@"
  "$@"
  if test "$?" -ne "0"; then
    exit 1
  fi
}

owd="$(pwd)"

# Derive project directory name.
#
project="$(basename "$(pwd)")"

idir="$(pwd)"
build2_dir="$HOME/.local"
mingw_prefix="x86_64-w64-mingw32"
wine_prefix="$HOME/.wine"
jobs=
verbose=
recreate=

while test $# -ne 0; do
  case $1 in
    -h|--help)
      diag
      diag "$usage"
      diag "Options:"
      diag "  --install-dir <dir>    Alternative installation directory."
      diag "  --build2-dir <dir>     build2 installation directory."
      diag "  --mingw-prefix <pfx>   MinGW prefix (default: x86_64-w64-mingw32)."
      diag "  --wine-prefix <dir>    Wine prefix directory."
      diag "  --recreate             Remove existing bdep state and host configurations."
      diag "  --jobs|-j <num>        Number of jobs to perform in parallel."
      diag "  --verbose <level>      Diagnostics verbosity level between 0 and 6."
      diag
      diag "By default the script will install build2 into ~/.local and"
      diag "MinGW from the system package manager if not already installed."
      diag
      diag "See the README file for details."
      diag
      exit 0
      ;;
    --install-dir)
      shift
      if test $# -eq 0; then
        diag "error: installation directory expected after --install-dir"
        diag "$usage"
        exit 1
      fi
      idir="$1"
      shift
      ;;
    --build2-dir)
      shift
      if test $# -eq 0; then
        diag "error: build2 directory expected after --build2-dir"
        diag "$usage"
        exit 1
      fi
      build2_dir="$1"
      shift
      ;;
    --mingw-prefix)
      shift
      if test $# -eq 0; then
        diag "error: MinGW prefix expected after --mingw-prefix"
        diag "$usage"
        exit 1
      fi
      mingw_prefix="$1"
      shift
      ;;
    --wine-prefix)
      shift
      if test $# -eq 0; then
        diag "error: Wine prefix directory expected after --wine-prefix"
        diag "$usage"
        exit 1
      fi
      wine_prefix="$1"
      shift
      ;;
    --recreate)
      recreate=true
      shift
      ;;
    -j|--jobs)
      shift
      if test $# -eq 0; then
        diag "error: number of jobs expected after --jobs"
        diag "$usage"
        exit 1
      fi
      jobs="$1"
      shift
      ;;
    --verbose)
      shift
      if test $# -eq 0; then
        diag "error: diagnostics level expected after --verbose"
        diag "$usage"
        exit 1
      fi
      verbose="$1"
      shift
      ;;
    *)
      diag "error: unexpected argument: $1"
      exit 1
      ;;
  esac
done

case $idir in
  /*) ;;
  *) idir="$owd/$idir" ;;
esac

case $build2_dir in
  /*) ;;
  *) build2_dir="$owd/$build2_dir" ;;
esac

case $wine_prefix in
  /*) ;;
  *) wine_prefix="$owd/$wine_prefix" ;;
esac

if test -n "$jobs"; then
  jobs="-j $jobs"
fi

if test -n "$verbose"; then
  verbose="--verbose $verbose"
fi

sys="$(uname -s)"
arch="$(uname -m)"

case "$sys" in
  Linux)
    ;;
  *)
    diag "error: unsupported system: $sys"
    exit 1
    ;;
esac

if ! command -v b >/dev/null 2>&1; then
  diag "Installing build2..."

  if ! test -f /tmp/toolchain-bindist.sha256; then
    run curl -fsSL -o /tmp/toolchain-bindist.sha256 \
      'https://stage.build2.org/0/toolchain-bindist.sha256'
  fi

  bindist_name=$(grep "linux" /tmp/toolchain-bindist.sha256 | grep "$arch" | head -n1 | awk '{print $2}')

  if test -z "$bindist_name"; then
    diag "error: could not find Linux bindist for $arch in toolchain-bindist.sha256"
    exit 1
  fi

  bindist_url="https://stage.build2.org/0/0.18.0-a.0/bindist/$bindist_name"
  bindist_file="/tmp/$bindist_name"

  if ! test -f "$bindist_file"; then
    diag "Downloading $bindist_url..."
    run curl -fsSL -o "$bindist_file" "$bindist_url"
  fi

  mkdir -p "$build2_dir"
  diag "Extracting $bindist_file..."
  run tar -xf "$bindist_file" -C "$build2_dir" --strip-components=1

  export PATH="$build2_dir/bin:$PATH"
fi

mingw_gcc="${mingw_prefix}-gcc"
mingw_gxx="${mingw_prefix}-g++"

if ! command -v "$mingw_gxx" >/dev/null 2>&1; then
  diag "Installing MinGW..."

  if command -v apt-get >/dev/null 2>&1; then
    run sudo apt-get install -y mingw-w64
  elif command -v dnf >/dev/null 2>&1; then
    run sudo dnf install -y mingw64-gcc mingw64-gcc-c++
  elif command -v pacman >/dev/null 2>&1; then
    run sudo pacman -S --noconfirm mingw-w64-gcc
  elif command -v zypper >/dev/null 2>&1; then
    run sudo zypper install -y mingw64-cross-gcc mingw64-cross-gcc-c++
  else
    diag "error: unsupported package manager, please install MinGW manually"
    exit 1
  fi
fi

if ! command -v wine >/dev/null 2>&1; then
  diag "Installing Wine..."

  if command -v apt-get >/dev/null 2>&1; then
    run sudo apt-get install -y wine wine64 wine32
  elif command -v dnf >/dev/null 2>&1; then
    run sudo dnf install -y wine
  elif command -v pacman >/dev/null 2>&1; then
    run sudo pacman -S --noconfirm wine
  elif command -v zypper >/dev/null 2>&1; then
    run sudo zypper install -y wine
  else
    diag "warning: could not install Wine, skipping Wine registry updates"
  fi
fi

# Set up Wine environment if Wine is available.
#
if command -v wine >/dev/null 2>&1; then
  export WINEPREFIX="$wine_prefix"

  # Initialize Wine prefix if needed.
  #
  if ! test -d "$wine_prefix"; then
    diag "Initializing Wine prefix..."
    run wineboot -u
  fi

  # Update Wine registry with MinGW paths.
  #
  wine_system_reg="$wine_prefix/system.reg"

  if test -f "$wine_system_reg"; then
    diag "Updating Wine registry with MinGW paths..."

    mingw_bin_dir=$(dirname "$(command -v "$mingw_gxx")")
    mingw_wine_path=$(echo "Z:$mingw_bin_dir" | sed 's|/|\\\\\\\\|g')
    mingw_check_path=$(echo "$mingw_bin_dir" | sed 's|/|\\\\\\\\|g')

    if ! grep -q "$mingw_check_path" "$wine_system_reg"; then
      awk -v mingw_path="$mingw_wine_path" '
      /^\[System\\\\ControlSet[^]]*\\\\Control\\\\Session Manager\\\\Environment\]/ {
        print
        in_env=1
        next
      }
      in_env && /^"PATH"=/ {
        # Handle both regular strings and str(2): format.
        if (match($0, /^"PATH"=(str\(2\):)?"([^"]*)"/)) {
          prefix = substr($0, RSTART, RLENGTH - length(substr($0, RSTART + RLENGTH - 1)))
          # Remove trailing quote.
          sub(/"$/, "", prefix)
          # Append MinGW path.
          print prefix ";" mingw_path "\""
        } else {
          print
        }
        in_env=0
        next
      }
      in_env && /^\[/ {
        in_env=0
      }
      { print }
      ' "$wine_system_reg" > "$wine_system_reg.tmp"

      mv "$wine_system_reg.tmp" "$wine_system_reg"
    fi
  fi
fi

if ! command -v b >/dev/null 2>&1; then
  diag "error: b not found in PATH"
  exit 1
fi

if ! command -v "$mingw_gxx" >/dev/null 2>&1; then
  diag "error: $mingw_gxx not found in PATH"
  exit 1
fi

cache="${XDG_CACHE_HOME:-$HOME/.cache}/iw4x/bootstrap-root"

if test "$recreate" = "true"; then
  diag "Recreating configurations: removing existing state..."

  if test -d .bdep; then
    rm -rf .bdep >/dev/null 2>&1 || true
  fi

  if test -d "../${project}-host"; then
    rm -rf "../${project}-host" >/dev/null 2>&1 || true
  fi
fi

iw4x=
if test -f "$cache"; then
  cached="$(cat "$cache")"
  if test -d "$cached"; then
    iw4x="$cached"
  fi
fi

if test -z "$iw4x"; then
  diag "Specify the absolute path to the IW4x installation root:"
  printf "> "
  read iw4x

  if test -z "$iw4x"; then
    diag "error: empty path is not valid"
    exit 1
  fi

  if test ! -d "$iw4x"; then
    diag "error: directory '$iw4x' does not exist"
    exit 1
  fi

  iw4x="${iw4x%/}"

  mkdir -p "$(dirname "$cache")" >/dev/null 2>&1
  printf "%s\n" "$iw4x" > "$cache"
fi

cd "$idir"

# Debug configuration.
#
bdep init -C @mingw32-debug $verbose $jobs                           \
  config.cxx=$mingw_gxx                                              \
  config.cc.coptions="-ggdb                                          \
                      -grecord-gcc-switches                          \
                      -pipe                                          \
                      -mtune=generic                                 \
                      -fasynchronous-unwind-tables                   \
                      -fno-omit-frame-pointer                        \
                      -mno-omit-leaf-frame-pointer"                  \
  config.cc.compiledb=./                                             \
  cc                                                                 \
  config.install.filter='include/@false lib/@false share/@false'     \
  config.install.root="$iw4x"                                        \
  config.install.bin="$iw4x"                                         \
  --wipe                                                             \
  -- config.libiw4x.cpptrace=true

# Release configuration.
#
run bdep init -C @mingw32-release $verbose $jobs                     \
  config.cxx="$mingw_gxx"                                            \
  config.cc.coptions="-O2                                            \
                      -ggdb                                          \
                      -grecord-gcc-switches                          \
                      -pipe                                          \
                      -mtune=generic                                 \
                      -fasynchronous-unwind-tables                   \
                      -fno-omit-frame-pointer                        \
                      -mno-omit-leaf-frame-pointer"                  \
  config.cc.compiledb=./                                             \
  cc                                                                 \
  config.install.filter='include/@false lib/@false share/@false'     \
  config.install.root="$iw4x"                                        \
  config.install.bin="$iw4x"                                         \
  --wipe                                                             \
  -- config.libiw4x.cpptrace=true

diag
diag "Build configurations created."
diag
