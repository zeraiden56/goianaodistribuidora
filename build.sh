#!/usr/bin/env bash

set -euo pipefail

# PowerShell:
# & "C:\msys64\usr\bin\bash.exe" ./build.sh
#
# Add --run to open the game after building.

case "${1:-}" in
    ""|--run) ;;
    *)
        echo "Usage: bash build.sh [--run]"
        exit 1
        ;;
esac

# Go to the directory where build.sh is located,
# without depending on the external `dirname` command.
script_path="${BASH_SOURCE[0]}"

if [[ "$script_path" == */* ]]; then
    script_dir="${script_path%/*}"
else
    script_dir="."
fi

cd -- "$script_dir"

case "$(uname -s)" in
    MINGW*|MSYS*)
        export PATH="/mingw64/bin:/usr/bin:$PATH"

        packages=(
            mingw-w64-x86_64-gcc
            mingw-w64-x86_64-cmake
            mingw-w64-x86_64-ninja
            mingw-w64-x86_64-SDL2
            mingw-w64-x86_64-SDL2_mixer
            mingw-w64-x86_64-libpng
        )

        missing=()

        for package in "${packages[@]}"; do
            if ! pacman -Q "$package" >/dev/null 2>&1; then
                missing+=("$package")
            fi
        done

        if (( ${#missing[@]} )); then
            echo "Installing missing build dependencies..."
            pacman -S --needed "${missing[@]}"
        fi

        build_dir="build-mingw"
        executable="distribuidora.exe"
        ;;

    *)
        echo "This script requires MSYS2 on Windows."
        exit 1
        ;;
esac

echo "Configuring Release build..."

cmake \
    -S . \
    -B "$build_dir" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=/mingw64/bin/g++.exe

echo "Building..."

cmake --build "$build_dir" --parallel

echo
echo "Build complete:"
echo "$PWD/$build_dir/$executable"

if [[ "${1:-}" == "--run" ]]; then
    exec "./$build_dir/$executable"
fi