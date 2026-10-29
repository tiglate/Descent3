#!/usr/bin/env bash
#
# make.sh -- convenient CMake/Ninja workflow for Descent 3 Open Source.
# Original game assets are selected and remembered by the application on its
# first run; compiling and installing do not require commercial game data.
set -euo pipefail

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SOURCE_DIR/build"
INSTALL_PREFIX="${HOME}/Apps/Descent3"
BUILD_TYPE=""
JOBS="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
VERBOSE=0

usage() {
    cat <<'EOF'
Usage: ./make.sh <action> [options]

Actions:
  configure       Configure the Linux Ninja Multi-Config build.
  build           Configure if needed, then compile the Debug build.
  release         Configure if needed, then compile the optimized Release build.
  install         Build Release and install it into ~/Apps/Descent3 (or --prefix).
  run [args...]   Install Release if needed, then start the game. Arguments are
                  passed directly to Descent3 (for example: -windowed).
  test            Build Debug and run CTest.
  clean           Remove compiled files but retain the CMake configuration.
  purge           Remove generated build and install directories.

Options (configure, build, release, install, test):
  -p, --prefix DIR  Install prefix (default: ~/Apps/Descent3).
  -j, --jobs N      Parallel build jobs (default: all available cores).
  -v, --verbose     Show underlying build commands.
  -h, --help        Show this help.

Examples:
  ./make.sh build
  ./make.sh release
  ./make.sh install
  ./make.sh run -windowed -width 1920 -height 1080

The first launch asks where the original game is installed and saves that
location in your preferences. To set or change it explicitly:
  ./make.sh run -assetdir "/path/to/Descent 3"
EOF
}

die() { echo "make.sh: error: $*" >&2; exit 1; }

need_prog() { command -v "$1" >/dev/null 2>&1 || die "'$1' is not on PATH"; }

canonical_dir() {
    [ -d "$1" ] || die "directory does not exist: $1"
    cd "$1" && pwd -P
}

cache_var() {
    [ -f "$BUILD_DIR/CMakeCache.txt" ] || return 1
    sed -n "s|^$1:[^=]*=||p" "$BUILD_DIR/CMakeCache.txt" | head -n 1
}

needs_configure() {
    [ ! -f "$BUILD_DIR/build.ninja" ] && return 0
    [ "$(cache_var CMAKE_HOME_DIRECTORY || true)" != "$SOURCE_DIR" ] && return 0
    [ "$(cache_var CMAKE_INSTALL_PREFIX || true)" != "$INSTALL_PREFIX" ] && return 0
    # Clear asset paths embedded by older versions of this wrapper.
    [ -n "$(cache_var DEFAULT_ADDITIONAL_DIRS || true)" ] && return 0
    return 1
}

configure() {
    need_prog cmake
    need_prog ninja
    mkdir -p "$BUILD_DIR"
    cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G "Ninja Multi-Config" \
        "-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX" \
        -DDEFAULT_ADDITIONAL_DIRS= \
        -DUSE_VCPKG=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
}

ensure_configured() {
    if needs_configure; then
        configure
    fi
}

build() {
    local config="$1"
    ensure_configured
    local args=(--build "$BUILD_DIR" --config "$config" --parallel "$JOBS")
    [ "$VERBOSE" -eq 1 ] && args+=(-v)
    cmake "${args[@]}"
}

install() {
    build Release
    cmake --install "$BUILD_DIR" --config Release
    mkdir -p "$INSTALL_PREFIX/custom" "$INSTALL_PREFIX/savegame"
}

ACTION="${1:-}"
[ -n "$ACTION" ] || { usage; exit 1; }
shift

case "$ACTION" in
    -h|--help|help) usage; exit 0 ;;
esac

# Game arguments intentionally remain unparsed, so every documented Descent 3
# command-line option may be passed through unchanged.
if [ "$ACTION" = run ]; then
    install
    cd "$INSTALL_PREFIX"
    exec ./Descent3 "$@"
fi

while [ $# -gt 0 ]; do
    case "$1" in
        -p|--prefix)
            [ $# -ge 2 ] || die "$1 requires a directory"
            INSTALL_PREFIX="$(canonical_dir "$(dirname "$2")")/$(basename "$2")"
            shift 2
            ;;
        -j|--jobs)
            [ $# -ge 2 ] || die "$1 requires a number"
            JOBS="$2"
            shift 2
            ;;
        -v|--verbose) VERBOSE=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) die "unknown option: $1" ;;
    esac
done

case "$ACTION" in
    configure) configure ;;
    build)     build Debug ;;
    release)   build Release ;;
    install)   install ;;
    test)      build Debug; ctest --test-dir "$BUILD_DIR" -C Debug --output-on-failure --parallel "$JOBS" ;;
    clean)     [ -f "$BUILD_DIR/build.ninja" ] && cmake --build "$BUILD_DIR" --target clean || echo "nothing configured yet" ;;
    purge)     rm -rf "$BUILD_DIR" "$INSTALL_PREFIX" ;;
    *)         die "unknown action: $ACTION (see ./make.sh --help)" ;;
esac
