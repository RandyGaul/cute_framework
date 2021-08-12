@echo off
if not exist build_emscripten mkdir build_emscripten
pushd build_emscripten
call emcmake cmake --config Release ..
call emmake make
popd