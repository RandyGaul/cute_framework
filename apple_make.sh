#!/bin/bash

mkdir -p build_apple_make
cmake -Bbuild_apple_make .
cd build_apple_make
make .
cd ..
