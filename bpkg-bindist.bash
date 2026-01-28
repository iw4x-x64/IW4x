#!/usr/bin/env bash

usage="Usage: $0 [-h|--help] [--debug] [--output <dir>]"

diag ()
{
  echo "$*" 1>&2
}

mode="debug"
output_dir="$(pwd)"
wipe_output=

while test $# -ne 0; do
  case "$1" in
    -h|--help)
      diag "$usage"
      diag
      diag "Create a binary distribution archive (bindist) for the project."
      diag
      diag "This script automatically locates the appropriate build configuration,"
      diag "switches context, and runs 'bpkg pkg-bindist' to generate a self-"
      diag "contained archive of the project and its dependencies."
      diag
      diag "Options:"
      diag "  --release        Use the @mingw32-release configuration (default: debug)"
      diag "  --output <dir>   Directory to place the archive (default: current dir)"
      diag "  --wipe-output    Wipe the output root directory clean before use"
      diag
      exit 0
      ;;
    --release)
      mode="release"
      shift
      ;;
    --output)
      shift
      output_dir="$1"
      shift
      ;;
    --wipe|--wipe-output)
      wipe_output=yes
      shift
      ;;
    *)
      diag "error: unknown option '$1'"
      diag "$usage"
      exit 1
      ;;
  esac
done

if ! command -v bdep >/dev/null 2>&1; then
  diag "error: bdep not found in PATH"
  exit 1
fi

# Derive project name.
#
project="libiw4x"
config="@mingw32-$mode"

diag "Targeting configuration: $config"

# Locate configuration directory.
#
# We ask bdep for the physical path of the configuration. The output format
# is usually: @config /path/to/config type ...
#
config_path="$(bdep config list | grep "^$config " | awk '{print $2}')"

if test -z "$config_path"; then
  diag "error: configuration '$config' not found."
  diag "hint: have you run the bootstrap script to initialize it?"
  exit 1
fi

if test ! -d "$config_path"; then
  diag "error: configuration directory '$config_path' does not exist."
  exit 1
fi

# Create output directory if it doesn't exist.
#
if test ! -d "$output_dir"; then
  mkdir -p "$output_dir"
fi

# Canonicalize output path (absolute).
#
# Since we are changing directories below, we need an absolute path to know
# where to write the result.
#
output_dir="$(cd "$output_dir" && pwd)"

diag "Configuration path: $config_path"
diag "Output directory:   $output_dir"
diag

# Enter configuration.
#
# bpkg commands generally operate on the "current" configuration. While some
# commands accept a directory argument, switching context allow `pkg-bindist`
# to correctly resolves the package state and dependencies.
#
cd "$config_path" || exit 1

# Generate bindist.
#
# We use:
#  --distribution=archive    Generate installation archive
#  --archive-lang            Map language to runtime for build metadata
#  --recursive=auto          Bundle required shared library dependencies
#  --output-root             Output directory for the archive
#
diag "Running pkg-bindist..."
bpkg pkg-bindist                                    \
  --distribution=archive                            \
  --archive-lang cc=x86_64-w64-mingw32              \
  --recursive=auto                                  \
  --output-root=/tmp/iw4x                           \
  $(test -n "$wipe_output" && echo "--wipe-output") \
  "$project"
