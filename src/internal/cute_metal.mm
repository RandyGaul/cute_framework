/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "cute_metal.h"

#include <cute_c_runtime.h>
#include <SDL3/SDL.h>
#include <sokol/sokol_gfx.h>

#ifdef SOKOL_METAL

#ifdef CF_MACOS
#import <Cocoa/Cocoa.h>
#endif
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

static struct
{
	int w, h, sample_count;
	SDL_Window* window;
	id<MTLDevice> device;
	SDL_MetalView view;
	CAMetalLayer* layer;
	id<CAMetalDrawable> drawable;
	id<MTLTexture> depth_texture;
} state;

static void s_init_depth_texture()
{
	MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8 width:state.w height:state.h mipmapped:NO];
	descriptor.storageMode = MTLStorageModePrivate;
	descriptor.usage = MTLTextureUsageRenderTarget;
	state.depth_texture = [state.layer.device newTextureWithDescriptor:descriptor];
	state.depth_texture.label = @"DepthStencil";
}

void cf_metal_init(void* sdl_window, int w, int h, int sample_count)
{
	state.w = w;
	state.h = h;
	state.sample_count = sample_count;
	state.window = (SDL_Window*)sdl_window;
	state.device = MTLCreateSystemDefaultDevice();
	state.view = SDL_Metal_CreateView(state.window);
	state.layer = (__bridge __typeof__ (CAMetalLayer*))SDL_Metal_GetLayer(state.view);
	state.layer.device = state.device;
	state.layer.framebufferOnly = true;
	s_init_depth_texture();
}

sg_context_desc cf_metal_get_context()
{
	sg_context_desc desc = { };
	desc.color_format = SG_PIXELFORMAT_BGRA8;
	desc.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
	desc.sample_count = state.sample_count;
	desc.metal.device = (__bridge const void*)state.device;
	desc.metal.drawable_cb = cf_metal_get_drawable;
	desc.metal.renderpass_descriptor_cb = cf_metal_get_render_pass_descriptor;
	return desc;
}

const void *cf_metal_get_render_pass_descriptor()
{
	state.drawable = [state.layer nextDrawable];

	MTLRenderPassDescriptor* render_pass_descriptor = NULL;
	render_pass_descriptor = [[MTLRenderPassDescriptor alloc] init];
	render_pass_descriptor.colorAttachments[0].texture = state.drawable.texture;
	render_pass_descriptor.depthAttachment.texture = state.depth_texture;
	render_pass_descriptor.stencilAttachment.texture = state.depth_texture;
	return (__bridge const void*)render_pass_descriptor;
}

const void* cf_metal_get_drawable()
{
	return (__bridge const void*)state.drawable;
}

void cf_metal_present(bool vsync)
{
	#ifdef CF_MACOS
	if (@available(macOS 10.13, *)) {
		state.layer.displaySyncEnabled = vsync;
	}
	#else
	CF_UNUSED(vsync);
	#endif

	int w, h;
	cf_metal_get_drawable_size(&w, &h);
	// Handle resizing the frame/depth+stencil buffers.
	if (((w > 0) && (w != state.w)) || ((h > 0) && (h != state.h))) {
		state.w = w;
		state.h = h;
		s_init_depth_texture();
	}
}

float cf_metal_get_dpi_scale()
{
	return state.layer.contentsScale;
}

void cf_metal_get_drawable_size(int* w, int* h)
{
	return SDL_Metal_GetDrawableSize(state.window, w, h);
}

#else // SOKOL_METAL

void cf_metal_init(void* sdl_window, int w, int h, int sample_count) { CF_UNUSED(sdl_window); CF_UNUSED(w); CF_UNUSED(h); CF_UNUSED(sample_count); }
sg_context_desc cf_metal_get_context() { sg_context_desc desc; CF_MEMSET(&desc, 0, sizeof(desc)); return desc; }
const void *cf_metal_get_render_pass_descriptor() { return NULL; }
const void* cf_metal_get_drawable() { return NULL; }
void cf_metal_present(bool vsync) { CF_UNUSED(vsync); }
float cf_metal_get_dpi_scale() { return 0; }
void cf_metal_get_drawable_size(int* w, int* h) { CF_UNUSED(w); CF_UNUSED(h); }

#endif // SOKOL_METAL

// We must compile sokol_gfx.h implementation in a .mm file for Apple builds.
#define SOKOL_IMPL
#define SOKOL_TRACE_HOOKS
#ifdef SOKOL_D3D11
#	define D3D11_NO_HELPERS
#endif

#include <sokol/sokol_gfx.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#define SOKOL_IMGUI_IMPL
#define SOKOL_IMGUI_NO_SOKOL_APP
#include <internal/imgui/sokol_imgui.h>
#include <sokol/sokol_gfx_imgui.h>
