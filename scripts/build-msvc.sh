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
msvc_wine_dir="$HOME/.wine-msvc"
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
      diag "  --msvc-wine-dir <dir>  MSVC Wine installation directory."
      diag "  --wine-prefix <dir>    Wine prefix directory."
      diag "  --recreate             Remove existing bdep state and host configurations."
      diag "  --jobs|-j <num>        Number of jobs to perform in parallel."
      diag "  --verbose <level>      Diagnostics verbosity level between 0 and 6."
      diag
      diag "By default the script will install build2 into ~/.local and"
      diag "MSVC into ~/.wine-msvc if they are not already installed."
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
    --msvc-wine-dir)
      shift
      if test $# -eq 0; then
        diag "error: MSVC Wine directory expected after --msvc-wine-dir"
        diag "$usage"
        exit 1
      fi
      msvc_wine_dir="$1"
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

case $msvc_wine_dir in
  /*) ;;
  *) msvc_wine_dir="$owd/$msvc_wine_dir" ;;
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

deps_needed=false
if ! command -v wine >/dev/null 2>&1; then
  deps_needed=true
elif ! command -v python3 >/dev/null 2>&1; then
  deps_needed=true
elif ! command -v msidb >/dev/null 2>&1; then
  deps_needed=true
elif ! command -v git >/dev/null 2>&1; then
  deps_needed=true
fi

if test "$deps_needed" = "true"; then
  diag "Installing Wine and dependencies..."

  if command -v apt-get >/dev/null 2>&1; then
    run sudo apt-get install -y wine wine64 wine32 python3 msitools ca-certificates winbind git curl
  elif command -v dnf >/dev/null 2>&1; then
    run sudo dnf install -y wine python3 msitools ca-certificates samba-winbind git curl
  elif command -v pacman >/dev/null 2>&1; then
    run sudo pacman -S --noconfirm wine python msitools ca-certificates samba git curl
  elif command -v zypper >/dev/null 2>&1; then
    run sudo zypper install -y wine python3 msitools ca-certificates samba-winbind git curl
  else
    diag "error: unsupported package manager"
    exit 1
  fi
fi

msvc_wine_installed=false
if test -d "$msvc_wine_dir" && test -f "$msvc_wine_dir/bin/x64/cl"; then
  msvc_wine_installed=true
fi

if test "$msvc_wine_installed" = "false"; then
  diag "Installing MSVC via msvc-wine..."

  msvc_wine_repo="/tmp/msvc-wine"
  if ! test -d "$msvc_wine_repo"; then
    run git clone https://github.com/mstorsjo/msvc-wine.git "$msvc_wine_repo"
  fi

  cd "$msvc_wine_repo"
  run ./vsdownload.py --dest "$msvc_wine_dir" --cache /tmp/msvc --accept-license
  run ./install.sh "$msvc_wine_dir"
  cd "$owd"
fi

export PATH="$msvc_wine_dir/bin/x64:$PATH"

export WINEPREFIX="$wine_prefix"
export WINEDLLOVERRIDES="mscoree,mshtml="

if ! test -d "$wine_prefix"; then
  diag "Initializing Wine prefix..."
  run wineboot -u
fi

wine_mono_dir="$wine_prefix/drive_c/windows/mono"
if ! test -d "$wine_mono_dir"; then
  diag "Installing Wine Mono..."

  mono_html="/tmp/wine-mono-listing.html"
  if ! test -f "$mono_html"; then
    run curl -fsSL -o "$mono_html" 'https://dl.winehq.org/wine/wine-mono/'
  fi

  mono_version=$(grep -oP 'href="\K[0-9]+\.[0-9]+\.[0-9]+(?=/)' "$mono_html" | sort -V | tail -n1)

  if test -z "$mono_version"; then
    diag "error: could not find Wine Mono version in directory listing"
    exit 1
  fi

  diag "Found Wine Mono version: $mono_version"

  mono_url="https://dl.winehq.org/wine/wine-mono/$mono_version/wine-mono-$mono_version-x86.msi"
  mono_file="/tmp/wine-mono-$mono_version-x86.msi"

  if ! test -f "$mono_file"; then
    diag "Downloading $mono_url..."
    run curl -fsSL -o "$mono_file" "$mono_url"
  fi

  diag "Installing Wine Mono $mono_version..."
  run wine msiexec /i "$mono_file" /qn /norestart
fi

wine_system_reg="$wine_prefix/system.reg"

if test -f "$wine_system_reg"; then
  diag "Updating Wine registry with MSVC paths..."

  msvc_bin_x64=$(echo "Z:$msvc_wine_dir/bin/x64" | sed 's|/|\\\\\\\\|g')
  msvc_bin_x86=$(echo "Z:$msvc_wine_dir/bin/x86" | sed 's|/|\\\\\\\\|g')
  msvc_check_x64=$(echo "$msvc_wine_dir/bin/x64" | sed 's|/|\\\\\\\\|g')
  msvc_check_x86=$(echo "$msvc_wine_dir/bin/x86" | sed 's|/|\\\\\\\\|g')

  if ! grep -q "$msvc_check_x64" "$wine_system_reg" && ! grep -q "$msvc_check_x86" "$wine_system_reg"; then
    awk -v msvc_x64="$msvc_bin_x64" -v msvc_x86="$msvc_bin_x86" '
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
        # Append MSVC paths.
        print prefix ";" msvc_x64 ";" msvc_x86 "\""
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

if ! command -v b >/dev/null 2>&1; then
  diag "error: b not found in PATH"
  exit 1
fi

if ! test -f "$msvc_wine_dir/bin/x64/cl"; then
  diag "error: cl not found in $msvc_wine_dir/bin/x64"
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
run bdep init -C @msvc-debug $verbose $jobs                      \
  config.cxx="$msvc_wine_dir/bin/x64/cl"                         \
  config.cc.coptions="/Z7                                        \
                      /MTd                                       \
                      /Od                                        \
                      /Oi                                        \
                      /EHsc"                                     \
  config.cc.loptions="/DEBUG:FULL                                \
                      /INCREMENTAL:NO"                           \
  cc                                                             \
  config.install.filter='include/@false lib/@false share/@false' \
  config.install.root="$iw4x"                                    \
  config.install.bin="$iw4x"                                     \
  --wipe                                                         \
  -- config.libiw4x.cpptrace=true

# Release configuration.
#
run bdep init -C @msvc-release $verbose $jobs                    \
  config.cxx="$msvc_wine_dir/bin/x64/cl"                         \
  config.cc.coptions="/O2                                        \
                      /Z7                                        \
                      /MT                                        \
                      /Oi                                        \
                      /EHsc"                                     \
  config.cc.loptions="/DEBUG:FULL                                \
                      /INCREMENTAL:NO"                           \
  cc                                                             \
  config.install.filter='include/@false lib/@false share/@false' \
  config.install.root="$iw4x"                                    \
  config.install.bin="$iw4x"                                     \
  --wipe                                                         \
  -- config.libiw4x.cpptrace=true

diag
diag "Build configurations created."
diag
