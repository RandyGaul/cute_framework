@echo off

rem MSVC 2019 v142
mkdir build_msvc_2019 > nul 2> nul
cmake -G "Visual Studio 16 2019" -A x64 -Bbuild_msvc_2019 .
cmake --build build_msvc_2019 --config Debug
cmake --build build_msvc_2019 --config Release
robocopy "build_msvc_2019/Debug" "github_release/msvc/Debug/v142" "cute.lib" > nul
robocopy "build_msvc_2019/Debug" "github_release/msvc/Debug/v142" "cute.pdb" > nul
robocopy "build_msvc_2019/Release" "github_release/msvc/Release/v142" "cute.lib" > nul
robocopy "build_msvc_2019/Debug" "github_release/msvc/Debug/v142" "cute.dll" > nul
robocopy "build_msvc_2019/Debug" "github_release/msvc/Debug/v142" "cute.pdb" > nul
robocopy "build_msvc_2019/Release" "github_release/msvc/Release/v142" "cute.dll" > nul

rem MingW
mkdir build_mingw > nul 2> nul
cmake -G "Unix Makefiles" -Bbuild_mingw .
cmake --build build_mingw
robocopy "build_mingw" "github_release/mingw/" "libcute.dll" > nul
robocopy "build_mingw" "github_release/mingw/" "libcute.dll.a" > nul

rem Include headers
robocopy "include" "github_release/include" > nul
robocopy "include/cute" "github_release/include/cute" > nul
robocopy "include/sokol" "github_release/include/sokol" > nul

7z a -tzip cute_framework.zip ./github_release/*
