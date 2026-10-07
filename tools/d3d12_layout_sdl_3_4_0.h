// gen_d3d12_layout's output for SDL 3.4.0's src/gpu/SDL_sysgpu.h and src/gpu/d3d12/SDL_gpu_d3d12.c.
// Used in place of the generator when CF_USE_SYSTEM_SDL is ON, since an installed SDL ships no private
// sources. The offsets only hold for SDL 3.4.0, so D3D12_LAYOUT_SDL_VERSION gates them at runtime.
#define SDL_GPU_DEVICE_FN_SLOT_COUNT 83
#define D3D12_RENDERER_DEVICE_BYTE_OFFSET 112
#define D3D12_LAYOUT_SDL_VERSION SDL_VERSIONNUM(3, 4, 0)
