@echo off
mkdir build_msvc_2026 > nul 2> nul
cmake -G "Visual Studio 18 2026" -A x64 -Bbuild_msvc_2026 .
