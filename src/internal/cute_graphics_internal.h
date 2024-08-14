/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_GRAPHICS_INTERNAL_H
#define CF_GRAPHICS_INTERNAL_H

#include <SDL3/SDL.h>

CF_INLINE SDL_GpuTextureCreateInfo SDL_GpuTextureCreateInfoDefaults(int w, int h)
{
	SDL_GpuTextureCreateInfo createInfo;
	CF_MEMSET(&createInfo, 0, sizeof(createInfo));
	createInfo.width = (int)w;
	createInfo.height = (int)h;
	createInfo.depth = 1;
	createInfo.isCube = SDL_FALSE;
	createInfo.layerCount = 1;
	createInfo.levelCount = 1;
	createInfo.sampleCount = SDL_GPU_SAMPLECOUNT_1;
	createInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8;
	createInfo.usageFlags = SDL_GPU_TEXTUREUSAGE_SAMPLER_BIT | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET_BIT;
	return createInfo;
}

struct CF_CanvasInternal
{
	CF_Texture cf_texture;
	CF_Texture cf_depth_stencil;
	SDL_GpuTexture* texture;
	SDL_GpuSampler* sampler;
	SDL_GpuTexture* depth_stencil;

	bool clear;

	// These get set by cf_apply_* functions.
	struct CF_MeshInternal* mesh;
	SDL_GpuGraphicsPipeline* pip;
	SDL_GpuRenderPass* pass;
};

struct CF_TextureInternal
{
	int w, h;
	SDL_GpuFilter filter;
	SDL_GpuTexture* tex;
	SDL_GpuTransferBuffer* buf;
	SDL_GpuSampler* sampler;
	SDL_GpuTextureFormat format;
};

CF_INLINE SDL_GpuSamplerCreateInfo SDL_GpuSamplerCreateInfoDefaults()
{
	SDL_GpuSamplerCreateInfo samplerInfo;
	CF_MEMSET(&samplerInfo, 0, sizeof(samplerInfo));
	samplerInfo.minFilter = SDL_GPU_FILTER_NEAREST;
	samplerInfo.magFilter = SDL_GPU_FILTER_NEAREST;
	samplerInfo.mipmapMode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	samplerInfo.addressModeU = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	samplerInfo.addressModeV = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	samplerInfo.addressModeW = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.anisotropyEnable = SDL_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.compareEnable = SDL_FALSE;
	samplerInfo.compareOp = SDL_GPU_COMPAREOP_ALWAYS;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = FLT_MAX;
	return samplerInfo;
}


CF_INLINE SDL_GpuTextureRegion SDL_GpuTextureRegionDefaults(CF_TextureInternal* tex, float minx, float miny, float maxx, float maxy)
{
	SDL_GpuTextureRegion region;
	CF_MEMSET(&region, 0, sizeof(region));
	region.textureSlice.texture = tex->tex;
	region.x = (Uint32)(minx * tex->w);
	region.y = (Uint32)(miny * tex->h);
	region.w = (Uint32)((maxx - minx) * tex->w);
	region.h = (Uint32)((maxy - miny) * tex->h);
	region.d = 1;
	return region;
}

CF_Shader cf_make_draw_shader_internal(const char* path);
void cf_load_internal_shaders();
void cf_unload_shader_compiler();

#endif // CF_GRAPHICS_INTERNAL_H
