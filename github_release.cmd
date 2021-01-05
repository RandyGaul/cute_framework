@echo off

REM MSVC 2019 v142.
mkdir build_msvc_2019 > nul 2> nul
cmake -G "Visual Studio 16 2019" -A x64 -Bbuild_msvc_2019 .
cmake --build build_msvc_2019 --config Debug
cmake --build build_msvc_2019 --config Release
robocopy "build_msvc_2019/Debug" "github_release/msvc/Debug/v142" "cute.lib" /MIR > nul
robocopy "build_msvc_2019/Debug" "github_release/msvc/Debug/v142" "cute.pdb" /MIR > nul
robocopy "build_msvc_2019/Release" "github_release/msvc/Release/v142" "cute.lib" /MIR > nul
robocopy "build_msvc_2019/Debug" "github_release/msvc/Debug/v142" "cute.dll" /MIR > nul
robocopy "build_msvc_2019/Debug" "github_release/msvc/Debug/v142" "cute.pdb" /MIR > nul
robocopy "build_msvc_2019/Release" "github_release/msvc/Release/v142" "cute.dll" /MIR > nul

REM MingW.
mkdir build_mingw > nul 2> nul
cmake -G "Unix Makefiles" -Bbuild_mingw .
cmake --build build_mingw
robocopy "build_mingw" "github_release/mingw/bin/" "libcute.dll" /MIR > nul
robocopy "build_mingw" "github_release/mingw/lib/" "libcute.dll.a" /MIR > nul

REM Include headers.
robocopy "include" "github_release/include" /MIR > nul
robocopy "include/cute" "github_release/include/cute" /MIR > nul
robocopy "include/sokol" "github_release/include/sokol" /MIR > nul

REM Tools.
robocopy "tools" "github_release/tools" /MIR > nul

7z a -tzip cute_framework.zip ./github_release/*
