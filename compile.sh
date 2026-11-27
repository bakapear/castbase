#!/usr/bin/env bash

# run sdk2013/src/buildallprojects to get missing .a files

# windows: wsl --install
# debian: sudo apt install -y build-essential cmake ninja-build nasm pkg-config git libvulkan-dev vulkan-tools glslc
# arch: sudo pacman -S --needed base-devel cmake ninja nasm pkgconf git vulkan-headers vulkan-icd-loader shaderc

ROOT="$(cd "$(dirname "$0")" && pwd)"
BC="$ROOT/build/linux/cmake"
cmake -E remove_directory "$BC"
cmake -G Ninja -S "$ROOT" -B "$BC"
ninja -C "$BC"
