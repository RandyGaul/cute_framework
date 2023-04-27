@echo off
if not exist build_emscripten mkdir build_emscripten
pushd build_emscripten
call emcmake cmake ..
call emmake make
popd