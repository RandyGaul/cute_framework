@echo off
mkdir build_msvc_2017 > nul 2> nul
cmake -G "Visual Studio 15 2017" -A x64 -Bbuild_msvc_2017 .
cmake --build build_msvc_2017 --config Debug
cmake --build build_msvc_2017 --config Release
