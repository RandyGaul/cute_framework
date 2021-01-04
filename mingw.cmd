@echo off
mkdir build_mingw > nul 2> nul
cmake -G "Unix Makefiles" -Bbuild_mingw .
cmake --build build_mingw
