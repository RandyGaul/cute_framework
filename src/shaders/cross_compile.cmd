@echo off
for %%f in ("*.glsl") do call :compile %%~nf
exit /B

:compile
@echo Compiling %~1.glsl into %~1_shader.h ...
call "../../tools/sokol-shdc/win32/sokol-shdc.exe" --input %~1.glsl --output %~1_shader.h --slang glsl330:hlsl5:metal_macos:glsl300es