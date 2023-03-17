#!/bin/bash
set -e
rm -rf build
mkdir build
cd build
cmake ..
make
cp -r ../data .
cp -r ../res .
echo "run barkanoid in build/ to play!"
