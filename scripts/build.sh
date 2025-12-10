#!/bin/sh

usage="Usage: $0 [-h|--help] [<options>]"

diag ()
{
  echo "$*" 1>&2
}

run ()
{
  diag "+ $@"
  "$@"
  if test "$?" -ne "0"; then
    exit 1
  fi
}

bootstrap_opts=""

while test $# -ne 0; do
  case $1 in
    -h|--help)
      diag
      diag "$usage"
      diag
      diag "Bootstraps both MSVC and MinGW configurations and builds the project."
      diag
      diag "All options are passed through to the bootstrap scripts."
      diag "See build-msvc.sh --help and build-mingw.sh --help for available options."
      diag
      exit 0
      ;;
    *)
      bootstrap_opts="$bootstrap_opts $1"
      shift
      ;;
  esac
done

script_dir="$(dirname "$0")"
cd "$script_dir"

run ./build-msvc.sh $bootstrap_opts
run ./build-mingw.sh $bootstrap_opts

run bdep update -a
