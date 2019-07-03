@echo off
mkdir build & pushd build
cmake -G "Visual Studio 16 2019" -A x64 ..
popd
cmake --build build --config Release