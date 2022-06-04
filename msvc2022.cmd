@echo off
mkdir build_msvc_2022 > nul 2> nul
cmake -G "Visual Studio 17 2022" -A x64 -Bbuild_msvc_2022 .
