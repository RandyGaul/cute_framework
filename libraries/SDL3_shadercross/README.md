# SDL3_shadercross

SDL Shadercross dependency is used to compile shaders for different graphics APIs. It's vendored and uses additional SPIRV headers.

## Updating

To update the shadercross dependency, you can run the following command:

```sh
  curl -o SDL_shadercross.c https://raw.githubusercontent.com/libsdl-org/SDL_shadercross/refs/heads/main/src/SDL_shadercross.c
  curl -o SDL_shadercross.h https://raw.githubusercontent.com/libsdl-org/SDL_shadercross/refs/heads/main/include/SDL3_shadercross/SDL_shadercross.h
  curl -o spirv.h https://raw.githubusercontent.com/KhronosGroup/SPIRV-Headers/refs/heads/main/include/spirv/unified1/spirv.h
  curl -o spirv_cross_c.h https://raw.githubusercontent.com/KhronosGroup/SPIRV-Cross/refs/heads/main/spirv_cross_c.h
```

## License

  * `SDL_shadercross.c` and `SDL_shadercross.h` are licensed under the [Zlib license](https://github.com/libsdl-org/SDL_shadercross/blob/main/LICENSE.txt)
  * `spirv.h` is licensed under the [MIT license](https://github.com/KhronosGroup/SPIRV-Headers/blob/main/include/spirv/unified1/spirv.h)
  * `spirv_cross_c.h` is licensed under the [Apache License 2.0 or MIT](https://raw.githubusercontent.com/KhronosGroup/SPIRV-Cross/refs/heads/main/spirv_cross_c.h)
