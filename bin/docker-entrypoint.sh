#!/usr/bin/env sh

set -e

CXXFLAGS="-pedantic -Wall -Wcast-qual -Wconversion -Werror -Wextra -Wshadow"
CFLAGS="${CXXFLAGS} -Wmissing-prototypes"
LDFLAGS=""

case "$1" in
"debug")
    TOOLCHAIN="llvm"
    CMAKE_BUILD_TYPE="Debug"
    ;;
"release")
    TOOLCHAIN="llvm"
    CMAKE_BUILD_TYPE="Release"
    CFLAGS="${CFLAGS} -flto=thin -march=native"
    CXXFLAGS="${CXXFLAGS} -flto=thin -march=native"
    ;;
"asan")
    TOOLCHAIN="llvm"
    CMAKE_BUILD_TYPE="Release"
    CFLAGS="${CFLAGS} -flto=thin -march=native -g -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fno-inline -fno-optimize-sibling-calls"
    CXXFLAGS="${CXXFLAGS} -flto=thin -march=native -g -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fno-inline -fno-optimize-sibling-calls"
    ;;
"ubsan")
    TOOLCHAIN="llvm"
    CMAKE_BUILD_TYPE="Release"
    CFLAGS="${CFLAGS} -flto=thin -march=native -g -fsanitize=undefined -fno-omit-frame-pointer -fno-inline -fno-optimize-sibling-calls"
    CXXFLAGS="${CXXFLAGS} -flto=thin -march=native -g -fsanitize=undefined -fno-omit-frame-pointer -fno-inline -fno-optimize-sibling-calls"
    ;;
"debug:gnu")
    TOOLCHAIN="gnu"
    CMAKE_BUILD_TYPE="Debug"
    ;;
"release:gnu")
    TOOLCHAIN="gnu"
    CMAKE_BUILD_TYPE="Release"
    CFLAGS="${CFLAGS} -flto -march=native"
    CXXFLAGS="${CXXFLAGS} -flto -march=native"
    ;;
*)
    echo "invalid build mode '$1'"
    exit 1
    ;;
esac

case "${TOOLCHAIN}" in
"llvm")
    CC="clang"
    CXX="clang++"
    LD="ld.lld"
    CFLAGS="${CFLAGS} -fuse-ld=lld -Wno-unused-command-line-argument"
    CXXFLAGS="${CXXFLAGS} -fuse-ld=lld -Wno-unused-command-line-argument"
    ;;
"gnu")
    CC="gcc"
    CXX="g++"
    LD="ld.gold"
    CFLAGS="${CFLAGS} -fuse-ld=gold"
    CXXFLAGS="${CXXFLAGS} -fuse-ld=gold"
    ;;
*)
    echo "invalid toolchain '${TOOLCHAIN}'"
    exit 1
    ;;
esac

mkdir build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
    -DCMAKE_C_COMPILER="${CC}" -DCMAKE_CXX_COMPILER="${CXX}" \
    -DCMAKE_C_FLAGS="${CFLAGS}" -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
    -DCMAKE_SHARED_LINKER_FLAGS="${LDFLAGS}" \
    -DCMAKE_STATIC_LINKER_FLAGS="${LDFLAGS}" \
    -DCMAKE_PREFIX_PATH=/usr \
    && cmake --build . -j `nproc` \
    && cmake --build . --target test
