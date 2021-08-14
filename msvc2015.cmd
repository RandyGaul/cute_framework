@echo off
mkdir build_msvc_2015 > nul 2> nul
cmake -G "Visual Studio 14 2015" -A x64 -Bbuild_msvc_2015 .
