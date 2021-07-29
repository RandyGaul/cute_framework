#!/bin/bash

mkdir -p build_bash
cmake -Bbuild_bash .
cd build_bash
cmake --build .
cd ..
