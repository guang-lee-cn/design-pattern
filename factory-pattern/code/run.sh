#!/bin/sh
# Build and run the before/after programs.
#
# Usage:
#   sh run.sh [Release|Debug] [section] [scene]
# Arguments may appear in any order; examples:
#   sh run.sh                     # build & run everything (Release)
#   sh run.sh Debug               # build & run everything (Debug)
#   sh run.sh 02_cost             # only section 02_cost
#   sh run.sh 02_cost 01_select   # only one scenario
# BUILD_TYPE=Debug is also honored.
# Release outputs go to build/, Debug outputs to build-debug/.
set -eu
cd "$(dirname "$0")"

build_type="${BUILD_TYPE:-Release}"
section=""
scene=""
for arg in "$@"; do
  case "$arg" in
    Release|Debug) build_type="$arg" ;;
    *)
      if [ -z "$section" ]; then
        section="$arg"
      elif [ -z "$scene" ]; then
        scene="$arg"
      else
        echo "usage: sh run.sh [Release|Debug] [section] [scene]" >&2
        exit 1
      fi
      ;;
  esac
done

case "$build_type" in
  Release) build_dir=build ;;
  Debug)   build_dir=build-debug ;;
esac

if [ -n "$scene" ] && [ -z "$section" ]; then
  echo "error: a scene needs its section: sh run.sh 02_cost 01_select" >&2
  exit 1
fi
if [ -n "$section" ] && [ ! -d "src/${section}" ]; then
  echo "error: no such section: src/${section}" >&2
  exit 1
fi
if [ -n "$scene" ] && [ ! -d "src/${section}/${scene}" ]; then
  echo "error: no such scenario: src/${section}/${scene}" >&2
  exit 1
fi

cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type"

# Resolve the target list; empty means "build everything".
targets=""
if [ -n "$scene" ]; then
  targets="${section}_${scene}_old ${section}_${scene}_new"
elif [ -n "$section" ]; then
  for scene_dir in "src/${section}"/*/; do
    [ -d "$scene_dir" ] || continue
    s=$(basename "$scene_dir")
    targets="${targets} ${section}_${s}_old ${section}_${s}_new"
  done
fi

if [ -n "$targets" ]; then
  cmake --build "$build_dir" -j --target $targets
else
  cmake --build "$build_dir" -j
fi

# Resolve which scenario directories to run.
if [ -n "$scene" ]; then
  pairs="src/${section}/${scene}"
elif [ -n "$section" ]; then
  pairs="src/${section}"/*
else
  pairs="src"/*/*
fi

for scene_dir in $pairs; do
  [ -d "$scene_dir" ] || continue
  sec=$(basename "$(dirname "$scene_dir")")
  scn=$(basename "$scene_dir")
  for kind in old new; do
    target="${sec}_${scn}_${kind}"
    exe="$build_dir/src/${target}"
    if [ -x "$exe" ]; then
      echo "==================== ${sec}/${scn} / ${kind} (${build_type}) ===================="
      "$exe"
    fi
  done
done
