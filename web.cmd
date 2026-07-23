@echo off
mkdir build_web >nul 2>nul
emcmake cmake -S . -B build_web -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_SCAN_FOR_MODULES=OFF