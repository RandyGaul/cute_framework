@echo off
mkdir build_web >nul 2>nul
emcmake cmake -S . -B build_web -DCMAKE_BUILD_TYPE=Release