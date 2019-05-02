#!/bin/bash

set -e

export OPTIMIZE="-Os"
export LDFLAGS="${OPTIMIZE}"
export CFLAGS="${OPTIMIZE}"
export CPPFLAGS="${OPTIMIZE}"

echo "============================================="
echo "Compiling wasm bindings"
echo "============================================="
(
  # Compile C/C++ code
  emcc \
    ${OPTIMIZE} \
    --bind \
    -s STRICT=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MALLOC=emmalloc \
    -s MODULARIZE=1 \
    -s EXPORT_ES6=1 \
    --post-js './src/module-post.js' \
    -o ./recast.js \
    src/*.cpp \
    recast/*.cpp

  # Create output folder
  mkdir -p dist
  # Move artifacts
  mv recast.{js,wasm} dist
)
echo "============================================="
echo "Compiling wasm bindings done"
echo "============================================="
