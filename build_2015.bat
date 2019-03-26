@echo off
mkdir build & pushd build
cmake -G "Visual Studio 14 2015 Win64" ..
popd
cmake --build build --config Release