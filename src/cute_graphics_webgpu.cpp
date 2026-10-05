/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// WebGPU graphics backend: wgpu-native on desktop, emdawnwebgpu on the web. Built only with the
// CF_WEBGPU CMake option.
//
// Ordering model. Every upload is staged into a mapped-at-creation buffer and recorded into the
// frame's command encoder as a copy, so uploads interleave with passes exactly like SDL_GPU copy
// passes. Uniform data is the exception: it goes into a per-submission ring at unique offsets and
// is written with one wgpuQueueWriteBuffer right before the submit, which is legal because no two
// draws in a submission share a ring region.
//
// Binding model, which the WGSL emitter in cute_spirv.h must produce:
// - CF set N is @group(N). Graphics: 0 = vertex resources, 1 = vertex uniforms, 2 = fragment
//   resources, 3 = fragment uniforms. Compute: 0 = read-only resources, 1 = read-write
//   resources, 2 = uniforms.
// - In a resource group, sampled texture i is @binding(2i) and its sampler @binding(2i+1), then
//   storage textures, then storage buffers, each in declaration order.
// - Uniform block u is @binding(u) of its uniform group, bound with a dynamic offset into the ring.
// - A compute image both read and written, in a format core WebGPU cannot read-write, is split: a
//   write-only storage binding plus a sampled binding after everything else in group 1, fed by a
//   copy taken before the dispatch.
// - Layout details (texture dimension, sample type, storage format and access) are read from the
//   WGSL declarations, so the emitter's declarations are the source of truth.

#include <cute_defines.h>

#ifdef CF_WEBGPU

#include "internal/cute_graphics_internal.h"
#include "internal/cute_app_internal.h"
#include "internal/cute_alloc_internal.h"

#include <cute_math.h>

#include <webgpu/webgpu.h>
#ifdef CF_EMSCRIPTEN
#	include <emscripten/emscripten.h>
#else
#	include <webgpu/wgpu.h>
#endif
#include <SDL3/SDL.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_wgpu.h>

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace Cute;

static const char* s_wgsl_src(const CF_ShaderBytecode& bc) { return bc.wgsl_src; }

//--------------------------------------------------------------------------------------------------
// Internal types.

#define CF_WGPU_MAX_BINDINGS (64)
#define CF_WGPU_UNIFORM_ALIGN (256)
#define CF_WGPU_STAGING_CHUNK (4 * 1024 * 1024)
#define CF_WGPU_STAGING_MAX_PENDING 4 // Submitted chunks still remapping before an upload waits on one.

// Anonymous: the other backends define structs of the same names, and their inline members
// would otherwise collide across translation units.
namespace {

enum CF_WBindKind
{
	CF_WBIND_UNIFORM,
	CF_WBIND_STORAGE_RO,
	CF_WBIND_STORAGE_RW,
	CF_WBIND_SAMPLER,
	CF_WBIND_SAMPLER_CMP,
	CF_WBIND_TEXTURE,
	CF_WBIND_STORAGE_TEXTURE,
};

struct CF_WBinding
{
	int group;
	int binding;
	CF_WBindKind kind;
	WGPUShaderStage visibility;
	WGPUTextureViewDimension dim;
	WGPUTextureSampleType sample_type;
	bool multisampled;
	WGPUTextureFormat storage_format;
	WGPUStorageTextureAccess access;
	char name[64];
};

// All bindings one pipeline sees, sorted by (group, binding).
struct CF_WLayoutInfo
{
	int count;
	CF_WBinding b[CF_WGPU_MAX_BINDINGS];
	int group_count; // Highest group + 1.
};

// Shared by descriptor. refs counts textures and CF_Samplers using it; samplers derived only for
// a slot's layout are pinned (never released before shutdown).
struct CF_WSampler
{
	WGPUSampler sampler;
	WGPUSamplerDescriptor desc;
	int refs;
	bool pinned;
};

struct CF_TextureInternal
{
	int w, h;
	int layers;     // Array layers, 3D depth, or 6 for cube maps.
	int mip_count;
	int sample_count;
	CF_TextureType type;
	CF_PixelFormat pixel_format;
	WGPUTextureFormat format;
	WGPUTextureUsage usage;
	WGPUTexture tex;
	WGPUTextureView view;         // Sampling view: all mips and layers, depth aspect for depth-stencil.
	WGPUTextureView storage_view; // Lazily made: mip 0 only, as storage bindings require.
	CF_WSampler* sampler;         // NULL for depth targets that never sample.
	CF_WSampler* draw_samplers[2];
	WGPUTexture split_scratch;    // Load-side copy for split read+write storage textures.
	WGPUTextureView split_scratch_view;
	struct { WGPUTextureView view; WGPUSampler sampler; } binding;
};

struct CF_CanvasInternal
{
	int w, h;
	CF_Texture cf_texture;
	CF_Texture cf_resolve_texture;
	CF_Texture cf_depth_stencil;
	CF_SampleCount sample_count;
	int samples;

	bool attached;
	int attach_layer;
	int attach_mip;
	bool attached_depth;

	int target_count;
	CF_Texture cf_textures_mrt[CF_MAX_CANVAS_TARGETS];
	CF_Texture cf_resolve_textures_mrt[CF_MAX_CANVAS_TARGETS];

	// Render-target views, one mip and one layer each.
	WGPUTextureView color_views[CF_MAX_CANVAS_TARGETS];
	WGPUTextureView resolve_views[CF_MAX_CANVAS_TARGETS];
	WGPUTextureView depth_view;
	bool depth_has_stencil;

	bool clear;
	bool has_clear_color[CF_MAX_CANVAS_TARGETS];
	CF_Color clear_color[CF_MAX_CANVAS_TARGETS];
	bool has_clear_depth_stencil;
	float clear_depth;
	uint32_t clear_stencil;

	struct CF_MeshInternal* mesh;
};

static CF_Color s_clear_color2(const CF_CanvasInternal* c, int i) { return (c && c->has_clear_color[i]) ? c->clear_color[i] : app->clear_color; }
static float s_clear_depth(const CF_CanvasInternal* c) { return (c && c->has_clear_depth_stencil) ? c->clear_depth : app->clear_depth; }
static uint32_t s_clear_stencil(const CF_CanvasInternal* c) { return (c && c->has_clear_depth_stencil) ? c->clear_stencil : app->clear_stencil; }
static int s_color_target_count(const CF_CanvasInternal* c) { return c->attached_depth ? 0 : (c->target_count > 1 ? c->target_count : 1); }

struct CF_ReadbackInternal
{
	WGPUBuffer buffer;
	int w, h;
	int row_bytes;     // Tight row size.
	int padded_row;    // 256-aligned row pitch in the buffer.
	int size;          // w * h * texel size.
	bool mapped;
	bool failed;
	bool ready;
};

struct CF_WPipelineKey
{
	WGPUTextureFormat color_formats[CF_MAX_CANVAS_TARGETS];
	int color_target_count;
	WGPUTextureFormat depth_format;
	int sample_count;
	CF_RenderState render_state;
	int vertex_stride;
	int instance_stride;
	int index_stride;
	int attribute_count;
	struct
	{
		int location;
		CF_VertexFormat format;
		int offset;
		bool per_instance;
	} attrs[CF_MAX_SHADER_INPUTS];
	uint32_t unfilterable_mask;
};

struct CF_WPipeline
{
	CF_WPipelineKey key;
	WGPURenderPipeline pip;
};

// Bind group layouts depend on which sampled slots hold unfilterable textures (depth or 32-bit
// float without float32-filterable): those slots need unfilterable-float + non-filtering samplers.
struct CF_WLayoutVariant
{
	uint32_t unfilterable_mask;
	WGPUBindGroupLayout bgl[4];
	WGPUPipelineLayout layout;
};

struct CF_WStageInfo
{
	int sampled_count;          // Sampled textures (each a texture + sampler pair).
	int storage_texture_count;
	int storage_buffer_count;
	int uniform_block_count;
	int block_sizes[CF_MAX_UNIFORM_BLOCK_COUNT];
	Cute::Array<CF_UniformBlockMember> members[CF_MAX_UNIFORM_BLOCK_COUNT];
	Cute::Array<const char*> image_names;
	Cute::Array<int> image_slots;

	CF_INLINE int index(const char* name, int block_index)
	{
		for (int i = 0; i < members[block_index].size(); ++i) {
			if (members[block_index][i].name == name) return i;
		}
		return -1;
	}
};

// Uniform bind groups bind the ring at offset zero and get their offset per draw, so one bind
// group per ring serves every draw. Keyed by ring generation: a regrown ring can reuse the
// released ring's handle address.
struct CF_WUniformGroup
{
	WGPUBindGroup group;
	uint64_t ring_generation;
};

// Resource bind groups for one group of one shader, reused while the bound resources match.
// Keys hold raw handles, so the whole cache drops whenever any bindable resource is released
// (g.bind_epoch), before a reused address could match a stale entry.
#define CF_WGPU_BIND_CACHE_SIZE (8)

struct CF_WBindCacheEntry
{
	uint64_t hash;
	WGPUBindGroupLayout bgl;
	int count;
	WGPUBindGroupEntry* entries;
	WGPUBindGroup group;
};

struct CF_WBindCache
{
	CF_WBindCacheEntry slots[CF_WGPU_BIND_CACHE_SIZE];
	int next;
	uint64_t epoch;
};

struct CF_ShaderInternal
{
	WGPUShaderModule vs;
	WGPUShaderModule fs;
	CF_WLayoutInfo layout;
	CF_WStageInfo stage[2]; // [0] vertex, [1] fragment.
	int input_count;
	const char* input_names[CF_MAX_SHADER_INPUTS];
	int input_locations[CF_MAX_SHADER_INPUTS];
	CF_ShaderInputFormat input_formats[CF_MAX_SHADER_INPUTS];
	Cute::Array<CF_WLayoutVariant> variants;
	Cute::Array<CF_WPipeline> pip_cache;
	CF_WUniformGroup uniform_groups[2]; // Groups 1 and 3.
	CF_WBindCache bind_caches[2];       // Groups 0 and 2.

	CF_INLINE int get_input_index(const char* name)
	{
		for (int i = 0; i < input_count; ++i) {
			if (input_names[i] == name) return i;
		}
		return -1;
	}
};

struct CF_WBuffer
{
	int element_count;
	int size;
	int stride;
	WGPUBuffer buffer;
};

struct CF_MeshInternal
{
	CF_WBuffer vertices;
	CF_WBuffer indices;
	CF_WBuffer instances;
	int attribute_count;
	CF_VertexAttribute attributes[CF_MESH_MAX_VERTEX_ATTRIBUTES];
	bool draw3d_augmented;
};

struct CF_InstanceBufferInternal
{
	CF_WBuffer buf;
};

struct CF_StorageBufferInternal
{
	WGPUBuffer buffer;
	int size;
	WGPUBufferUsage usage;
};

// A read+write storage texture the WGSL split into a write-side storage binding and a read-side
// sampled binding: the backend copies the texture to a scratch before the dispatch.
struct CF_WSplit
{
	int store_binding; // Group 1 storage-texture binding.
	int load_binding;  // Group 1 sampled-texture binding appended after everything else.
};

struct CF_ComputeShaderInternal
{
	WGPUShaderModule module;
	CF_WLayoutInfo layout;
	CF_WStageInfo stage;
	int ro_storage_texture_count;
	int ro_storage_buffer_count;
	int rw_storage_texture_count;
	int rw_storage_buffer_count;
	Cute::Array<CF_WSplit> splits;
	Cute::Array<CF_WLayoutVariant> variants;
	Cute::Array<WGPUComputePipeline> pipelines; // Parallel to variants.
	CF_WUniformGroup uniform_group;
};

// Standard-size chunks are recycled: after a submission each one is remapped for writing and
// returns to the free list once the GPU is done reading it. Oversize chunks are released.
struct CF_WStagingChunk
{
	WGPUBuffer buffer;
	uint8_t* mapped;
	uint8_t* cpu; // Stands in for the mapping once the device is lost.
	uint64_t size;
	uint64_t used;
	volatile bool map_done;
	bool map_failed;
};

struct CF_WBlitPipeline
{
	WGPUTextureFormat format;
	WGPURenderPipeline pip;
	WGPUBindGroupLayout bgl;
};

// State that lives for one render pass, as in SDL_GPU. Set between passes, it waits for the
// next pass. A pass split by a full uniform ring carries it over: the split is invisible to CF.
// Unbound storage-buffer slots get the dummy buffer.
struct CF_WPassState
{
	bool has_viewport, has_scissor, has_blend_constant, has_stencil_reference;
	float viewport[4];
	int scissor[4];
	WGPUColor blend_constant;
	uint32_t stencil_reference;
	WGPUBuffer vs_storage[8];
	int vs_storage_count;
	WGPUBuffer fs_storage[8];
	int fs_storage_count;
};

struct CF_WDummyTexture
{
	WGPUTextureViewDimension dim;
	WGPUTextureSampleType sample_type;
	WGPUTextureFormat storage_format;
	WGPUTexture tex;
	WGPUTextureView view;
};

} // namespace

static struct
{
	bool debug;
	WGPUInstance instance;
	WGPUAdapter adapter;
	WGPUDevice device;
	WGPUQueue queue;
	WGPULimits limits;
	char adapter_name[256];

	bool float32_filterable;
	bool float32_blendable;
	bool depth32_stencil8;
	bool bc;
	bool formats_tier1;
	bool rg11b10_renderable;
	bool depth_clip_control;
	bool adapter_formats;
	bool bgra8_storage;
	volatile bool device_lost;
	bool lose_on_destroy; // Set by cf_webgpu_lose_device: Dawn's Destroyed is then a loss, not shutdown.
	int error_count; // Uncaptured errors while the device is alive.

	// Sample counts beyond the core 1 and 4 need the adapter-specific feature and a per-format
	// probe, cached here.
	struct { WGPUTextureFormat format; int samples; bool ok; } msaa_probes[32];
	int msaa_probe_count;

	SDL_Window* window;
#ifdef __APPLE__
	SDL_MetalView metal_view;
#endif
	WGPUSurface surface;
	WGPUTextureFormat surface_format;
	WGPUPresentMode present_mode;
	bool surface_configured;
	int surface_w, surface_h;
	WGPUPresentMode supported_present_modes[8];
	int supported_present_mode_count;

	WGPUTexture swapchain_tex;
	WGPUTextureView swapchain_view;
	bool skip_drawing;

	WGPUCommandEncoder encoder;
	WGPURenderPassEncoder pass;
	CF_CanvasInternal* canvas;

	CF_WPassState ps;

	// Draw bindings recorded by cf_apply_shader, flushed at the draw.
	CF_ShaderInternal* shader;
	CF_MaterialInternal* material;
	CF_WLayoutVariant* variant;
	uint32_t uniform_offsets[2][CF_MAX_UNIFORM_BLOCK_COUNT];
	uint64_t bind_epoch;

	CF_Filter filter_override;
	bool has_filter_override;
	WGPUBuffer instance_override;
	int instance_override_count;
	int instance_override_offset;

	// Uniform ring for the current submission.
	uint8_t* ring_cpu;
	int ring_size;
	int ring_used;
	WGPUBuffer ring;
	uint64_t ring_generation;
	int ring_wanted_size;

	Cute::Array<CF_WStagingChunk*> staging;         // Mapped, recording uploads this submission.
	Cute::Array<CF_WStagingChunk*> staging_pending; // Submitted, remapping.
	Cute::Array<CF_WStagingChunk*> staging_free;    // Mapped and ready for reuse.
	Cute::Array<CF_ReadbackInternal*> readbacks;    // Alive until cf_destroy_readback or cleanup.
	Cute::Array<CF_WSampler*> samplers; // Every sampler made, deduplicated by descriptor.
	Cute::Array<CF_WBlitPipeline> blit_pipelines;
	Cute::Array<CF_WDummyTexture> dummy_textures;
	WGPUBuffer dummy_buffer;
	WGPUBindGroupLayout empty_bgl;
	WGPUBindGroup empty_bg;
	WGPUShaderModule blit_module;
	bool imgui_ready;
} g = { };

//--------------------------------------------------------------------------------------------------
// Small helpers.

static inline WGPUStringView s_sv(const char* s) { WGPUStringView v; v.data = s; v.length = s ? WGPU_STRLEN : 0; return v; }
static inline int s_align(int x, int a) { return (x + a - 1) & ~(a - 1); }
static inline uint64_t s_align64(uint64_t x, uint64_t a) { return (x + a - 1) & ~(a - 1); }

static void s_process_events()
{
#ifdef CF_EMSCRIPTEN
	wgpuInstanceProcessEvents(g.instance);
#else
	wgpuDevicePoll(g.device, false, NULL);
	wgpuInstanceProcessEvents(g.instance);
#endif
}

// Blocks until *flag turns true, pumping WebGPU events. The web yields to the browser (ASYNCIFY).
static void s_wait(volatile bool* flag)
{
	while (!*flag) {
#ifdef CF_EMSCRIPTEN
		wgpuInstanceProcessEvents(g.instance);
		if (!*flag) emscripten_sleep(1);
#else
		if (g.device) wgpuDevicePoll(g.device, true, NULL);
		wgpuInstanceProcessEvents(g.instance);
#endif
	}
}

static void s_print_sv(const char* prefix, WGPUStringView v)
{
	if (v.data) {
		size_t n = v.length == WGPU_STRLEN ? strlen(v.data) : v.length;
		fprintf(stderr, "%s%.*s\n", prefix, (int)n, v.data);
	} else {
		fprintf(stderr, "%s\n", prefix);
	}
}

// A lost device stays lost: CF stops issuing GPU work and frames become no-ops. Recovering would
// mean recreating every resource, and switching to WebGL mid-run is out of scope.
static void s_mark_device_lost(WGPUStringView message)
{
	if (g.device_lost) return;
	g.device_lost = true;
	s_print_sv("CF: the WebGPU device was lost; rendering stops for the rest of the run. Reason: ", message);
}

// wgpu-native reports most calls on a lost device as validation errors, never reaching the lost
// callback. Missing the loss is fatal: wgpu-native aborts the process on a failed submit or map.
static bool s_error_means_lost(WGPUStringView message)
{
#ifdef CF_EMSCRIPTEN
	CF_UNUSED(message);
	return false;
#else
	if (!message.data) return false;
	String text;
	text.append(message.data, message.data + (message.length == WGPU_STRLEN ? strlen(message.data) : message.length));
	return text.contains("Parent device is lost");
#endif
}

#ifndef CF_EMSCRIPTEN
struct CF_WScope
{
	volatile bool done;
	bool failed;
	const char* prefix;
};

static void s_on_pop_scope(WGPUPopErrorScopeStatus status, WGPUErrorType type, WGPUStringView message, void* ud1, void* ud2)
{
	CF_UNUSED(ud2);
	CF_WScope* s = (CF_WScope*)ud1;
	if (status == WGPUPopErrorScopeStatus_Success && type != WGPUErrorType_NoError) {
		if (s_error_means_lost(message)) s_mark_device_lost(message);
		else if (s->prefix && !g.device_lost) s_print_sv(s->prefix, message);
		s->failed = true;
	}
	s->done = true;
}

// Pops a validation scope pushed by the caller and waits for its verdict. Native only: the web
// would have to yield to the browser. A NULL prefix keeps the error quiet.
static bool s_pop_validation_scope(const char* prefix)
{
	CF_WScope scope = { };
	scope.prefix = prefix;
	WGPUPopErrorScopeCallbackInfo cb = WGPU_POP_ERROR_SCOPE_CALLBACK_INFO_INIT;
	cb.mode = WGPUCallbackMode_AllowProcessEvents;
	cb.callback = s_on_pop_scope;
	cb.userdata1 = &scope;
	wgpuDevicePopErrorScope(g.device, cb);
	s_wait(&scope.done);
	return !scope.failed;
}
#endif

//--------------------------------------------------------------------------------------------------
// Format tables.

static WGPUTextureFormat s_wrap(CF_PixelFormat format)
{
	switch (format) {
	case CF_PIXEL_FORMAT_R8_UNORM:            return WGPUTextureFormat_R8Unorm;
	case CF_PIXEL_FORMAT_R8G8_UNORM:          return WGPUTextureFormat_RG8Unorm;
	case CF_PIXEL_FORMAT_R8G8B8A8_UNORM:      return WGPUTextureFormat_RGBA8Unorm;
	case CF_PIXEL_FORMAT_R16_UNORM:           return g.formats_tier1 ? WGPUTextureFormat_R16Unorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_R16G16_UNORM:        return g.formats_tier1 ? WGPUTextureFormat_RG16Unorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_R16G16B16A16_UNORM:  return g.formats_tier1 ? WGPUTextureFormat_RGBA16Unorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_R10G10B10A2_UNORM:   return WGPUTextureFormat_RGB10A2Unorm;
	case CF_PIXEL_FORMAT_B8G8R8A8_UNORM:      return WGPUTextureFormat_BGRA8Unorm;
	case CF_PIXEL_FORMAT_BC1_RGBA_UNORM:      return g.bc ? WGPUTextureFormat_BC1RGBAUnorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_BC2_RGBA_UNORM:      return g.bc ? WGPUTextureFormat_BC2RGBAUnorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_BC3_RGBA_UNORM:      return g.bc ? WGPUTextureFormat_BC3RGBAUnorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_BC4_R_UNORM:         return g.bc ? WGPUTextureFormat_BC4RUnorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_BC5_RG_UNORM:        return g.bc ? WGPUTextureFormat_BC5RGUnorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_BC7_RGBA_UNORM:      return g.bc ? WGPUTextureFormat_BC7RGBAUnorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_BC6H_RGB_FLOAT:      return g.bc ? WGPUTextureFormat_BC6HRGBFloat : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_BC6H_RGB_UFLOAT:     return g.bc ? WGPUTextureFormat_BC6HRGBUfloat : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_R8_SNORM:            return WGPUTextureFormat_R8Snorm;
	case CF_PIXEL_FORMAT_R8G8_SNORM:          return WGPUTextureFormat_RG8Snorm;
	case CF_PIXEL_FORMAT_R8G8B8A8_SNORM:      return WGPUTextureFormat_RGBA8Snorm;
	case CF_PIXEL_FORMAT_R16_SNORM:           return g.formats_tier1 ? WGPUTextureFormat_R16Snorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_R16G16_SNORM:        return g.formats_tier1 ? WGPUTextureFormat_RG16Snorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_R16G16B16A16_SNORM:  return g.formats_tier1 ? WGPUTextureFormat_RGBA16Snorm : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_R16_FLOAT:           return WGPUTextureFormat_R16Float;
	case CF_PIXEL_FORMAT_R16G16_FLOAT:        return WGPUTextureFormat_RG16Float;
	case CF_PIXEL_FORMAT_R16G16B16A16_FLOAT:  return WGPUTextureFormat_RGBA16Float;
	case CF_PIXEL_FORMAT_R32_FLOAT:           return WGPUTextureFormat_R32Float;
	case CF_PIXEL_FORMAT_R32G32_FLOAT:        return WGPUTextureFormat_RG32Float;
	case CF_PIXEL_FORMAT_R32G32B32A32_FLOAT:  return WGPUTextureFormat_RGBA32Float;
	case CF_PIXEL_FORMAT_R11G11B10_UFLOAT:    return WGPUTextureFormat_RG11B10Ufloat;
	case CF_PIXEL_FORMAT_R8_UINT:             return WGPUTextureFormat_R8Uint;
	case CF_PIXEL_FORMAT_R8G8_UINT:           return WGPUTextureFormat_RG8Uint;
	case CF_PIXEL_FORMAT_R8G8B8A8_UINT:       return WGPUTextureFormat_RGBA8Uint;
	case CF_PIXEL_FORMAT_R16_UINT:            return WGPUTextureFormat_R16Uint;
	case CF_PIXEL_FORMAT_R16G16_UINT:         return WGPUTextureFormat_RG16Uint;
	case CF_PIXEL_FORMAT_R16G16B16A16_UINT:   return WGPUTextureFormat_RGBA16Uint;
	case CF_PIXEL_FORMAT_R8_INT:              return WGPUTextureFormat_R8Sint;
	case CF_PIXEL_FORMAT_R8G8_INT:            return WGPUTextureFormat_RG8Sint;
	case CF_PIXEL_FORMAT_R8G8B8A8_INT:        return WGPUTextureFormat_RGBA8Sint;
	case CF_PIXEL_FORMAT_R16_INT:             return WGPUTextureFormat_R16Sint;
	case CF_PIXEL_FORMAT_R16G16_INT:          return WGPUTextureFormat_RG16Sint;
	case CF_PIXEL_FORMAT_R16G16B16A16_INT:    return WGPUTextureFormat_RGBA16Sint;
	case CF_PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB: return WGPUTextureFormat_RGBA8UnormSrgb;
	case CF_PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB: return WGPUTextureFormat_BGRA8UnormSrgb;
	case CF_PIXEL_FORMAT_BC1_RGBA_UNORM_SRGB: return g.bc ? WGPUTextureFormat_BC1RGBAUnormSrgb : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_BC2_RGBA_UNORM_SRGB: return g.bc ? WGPUTextureFormat_BC2RGBAUnormSrgb : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_BC3_RGBA_UNORM_SRGB: return g.bc ? WGPUTextureFormat_BC3RGBAUnormSrgb : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_BC7_RGBA_UNORM_SRGB: return g.bc ? WGPUTextureFormat_BC7RGBAUnormSrgb : WGPUTextureFormat_Undefined;
	case CF_PIXEL_FORMAT_D16_UNORM:           return WGPUTextureFormat_Depth16Unorm;
	case CF_PIXEL_FORMAT_D24_UNORM:           return WGPUTextureFormat_Depth24Plus;
	case CF_PIXEL_FORMAT_D32_FLOAT:           return WGPUTextureFormat_Depth32Float;
	case CF_PIXEL_FORMAT_D24_UNORM_S8_UINT:   return WGPUTextureFormat_Depth24PlusStencil8;
	case CF_PIXEL_FORMAT_D32_FLOAT_S8_UINT:   return g.depth32_stencil8 ? WGPUTextureFormat_Depth32FloatStencil8 : WGPUTextureFormat_Undefined;
	// A8, the 16-bit packed BGR formats: WebGPU has no equivalent.
	default:                                  return WGPUTextureFormat_Undefined;
	}
}

struct CF_WFormatInfo
{
	int block_bytes; // Bytes per texel, or per 4x4 block for compressed formats.
	int block_dim;   // 1, or 4 for compressed formats.
	bool depth;
	bool stencil;
	bool sint, uint;
	bool float32;    // Needs float32-filterable to filter, float32-blendable to blend.
	bool renderable;
	bool storage;    // Usable as a storage texture in core WebGPU.
};

static CF_WFormatInfo s_format_info(WGPUTextureFormat f)
{
	CF_WFormatInfo r = { };
	r.block_dim = 1;
	r.renderable = true;
	switch (f) {
	case WGPUTextureFormat_R8Unorm: r.block_bytes = 1; break;
	case WGPUTextureFormat_R8Snorm: r.block_bytes = 1; r.renderable = false; break;
	case WGPUTextureFormat_R8Uint: r.block_bytes = 1; r.uint = true; break;
	case WGPUTextureFormat_R8Sint: r.block_bytes = 1; r.sint = true; break;
	case WGPUTextureFormat_R16Unorm: case WGPUTextureFormat_R16Snorm: r.block_bytes = 2; r.renderable = f == WGPUTextureFormat_R16Unorm; break;
	case WGPUTextureFormat_R16Uint: r.block_bytes = 2; r.uint = true; break;
	case WGPUTextureFormat_R16Sint: r.block_bytes = 2; r.sint = true; break;
	case WGPUTextureFormat_R16Float: r.block_bytes = 2; break;
	case WGPUTextureFormat_RG8Unorm: r.block_bytes = 2; break;
	case WGPUTextureFormat_RG8Snorm: r.block_bytes = 2; r.renderable = false; break;
	case WGPUTextureFormat_RG8Uint: r.block_bytes = 2; r.uint = true; break;
	case WGPUTextureFormat_RG8Sint: r.block_bytes = 2; r.sint = true; break;
	case WGPUTextureFormat_R32Float: r.block_bytes = 4; r.float32 = true; r.storage = true; break;
	case WGPUTextureFormat_R32Uint: r.block_bytes = 4; r.uint = true; r.storage = true; break;
	case WGPUTextureFormat_R32Sint: r.block_bytes = 4; r.sint = true; r.storage = true; break;
	case WGPUTextureFormat_RG16Unorm: case WGPUTextureFormat_RG16Snorm: r.block_bytes = 4; r.renderable = f == WGPUTextureFormat_RG16Unorm; break;
	case WGPUTextureFormat_RG16Uint: r.block_bytes = 4; r.uint = true; break;
	case WGPUTextureFormat_RG16Sint: r.block_bytes = 4; r.sint = true; break;
	case WGPUTextureFormat_RG16Float: r.block_bytes = 4; break;
	case WGPUTextureFormat_RGBA8Unorm: r.block_bytes = 4; r.storage = true; break;
	case WGPUTextureFormat_RGBA8UnormSrgb: r.block_bytes = 4; break;
	case WGPUTextureFormat_RGBA8Snorm: r.block_bytes = 4; r.renderable = false; r.storage = true; break;
	case WGPUTextureFormat_RGBA8Uint: r.block_bytes = 4; r.uint = true; r.storage = true; break;
	case WGPUTextureFormat_RGBA8Sint: r.block_bytes = 4; r.sint = true; r.storage = true; break;
	case WGPUTextureFormat_BGRA8Unorm: case WGPUTextureFormat_BGRA8UnormSrgb: r.block_bytes = 4; break;
	case WGPUTextureFormat_RGB10A2Unorm: r.block_bytes = 4; break;
	case WGPUTextureFormat_RG11B10Ufloat: r.block_bytes = 4; r.renderable = g.rg11b10_renderable; break;
	case WGPUTextureFormat_RG32Float: r.block_bytes = 8; r.float32 = true; r.storage = true; break;
	case WGPUTextureFormat_RG32Uint: r.block_bytes = 8; r.uint = true; r.storage = true; break;
	case WGPUTextureFormat_RG32Sint: r.block_bytes = 8; r.sint = true; r.storage = true; break;
	case WGPUTextureFormat_RGBA16Unorm: case WGPUTextureFormat_RGBA16Snorm: r.block_bytes = 8; r.renderable = f == WGPUTextureFormat_RGBA16Unorm; break;
	case WGPUTextureFormat_RGBA16Uint: r.block_bytes = 8; r.uint = true; r.storage = true; break;
	case WGPUTextureFormat_RGBA16Sint: r.block_bytes = 8; r.sint = true; r.storage = true; break;
	case WGPUTextureFormat_RGBA16Float: r.block_bytes = 8; r.storage = true; break;
	case WGPUTextureFormat_RGBA32Float: r.block_bytes = 16; r.float32 = true; r.storage = true; break;
	case WGPUTextureFormat_RGBA32Uint: r.block_bytes = 16; r.uint = true; r.storage = true; break;
	case WGPUTextureFormat_RGBA32Sint: r.block_bytes = 16; r.sint = true; r.storage = true; break;
	case WGPUTextureFormat_Depth16Unorm: r.block_bytes = 2; r.depth = true; break;
	case WGPUTextureFormat_Depth24Plus: r.block_bytes = 4; r.depth = true; break;
	case WGPUTextureFormat_Depth32Float: r.block_bytes = 4; r.depth = true; break;
	case WGPUTextureFormat_Depth24PlusStencil8: r.block_bytes = 4; r.depth = true; r.stencil = true; break;
	case WGPUTextureFormat_Depth32FloatStencil8: r.block_bytes = 8; r.depth = true; r.stencil = true; break;
	case WGPUTextureFormat_BC1RGBAUnorm: case WGPUTextureFormat_BC1RGBAUnormSrgb:
	case WGPUTextureFormat_BC4RUnorm: case WGPUTextureFormat_BC4RSnorm:
		r.block_bytes = 8; r.block_dim = 4; r.renderable = false; break;
	case WGPUTextureFormat_BC2RGBAUnorm: case WGPUTextureFormat_BC2RGBAUnormSrgb:
	case WGPUTextureFormat_BC3RGBAUnorm: case WGPUTextureFormat_BC3RGBAUnormSrgb:
	case WGPUTextureFormat_BC5RGUnorm: case WGPUTextureFormat_BC5RGSnorm:
	case WGPUTextureFormat_BC6HRGBUfloat: case WGPUTextureFormat_BC6HRGBFloat:
	case WGPUTextureFormat_BC7RGBAUnorm: case WGPUTextureFormat_BC7RGBAUnormSrgb:
		r.block_bytes = 16; r.block_dim = 4; r.renderable = false; break;
	default: r.block_bytes = 4; r.renderable = false; break;
	}
	return r;
}

// Storage-texture capability WebGPU guarantees with the features this device enabled. The
// adapter-specific-format feature may add more per adapter, but nothing reports it per format.
static bool s_format_storage(WGPUTextureFormat f)
{
	if (s_format_info(f).storage) return true;
	switch (f) {
	case WGPUTextureFormat_BGRA8Unorm:
		return g.bgra8_storage;
	case WGPUTextureFormat_R8Unorm: case WGPUTextureFormat_R8Snorm: case WGPUTextureFormat_R8Uint: case WGPUTextureFormat_R8Sint:
	case WGPUTextureFormat_RG8Unorm: case WGPUTextureFormat_RG8Snorm: case WGPUTextureFormat_RG8Uint: case WGPUTextureFormat_RG8Sint:
	case WGPUTextureFormat_R16Uint: case WGPUTextureFormat_R16Sint: case WGPUTextureFormat_R16Float:
	case WGPUTextureFormat_RG16Uint: case WGPUTextureFormat_RG16Sint: case WGPUTextureFormat_RG16Float:
	case WGPUTextureFormat_RGB10A2Unorm: case WGPUTextureFormat_RG11B10Ufloat:
	case WGPUTextureFormat_R16Unorm: case WGPUTextureFormat_R16Snorm: case WGPUTextureFormat_RG16Unorm:
	case WGPUTextureFormat_RG16Snorm: case WGPUTextureFormat_RGBA16Unorm: case WGPUTextureFormat_RGBA16Snorm:
		return g.formats_tier1;
	default:
		return false;
	}
}

#define CF_WGPU_STORAGE_USAGE (CF_TEXTURE_USAGE_GRAPHICS_STORAGE_READ_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_READ_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT)

// One line per process for a request WebGPU cannot honor.
#define CF_WGPU_WARN_ONCE(...) do { static bool s_warned = false; if (!s_warned) { s_warned = true; fprintf(stderr, __VA_ARGS__); } } while (0)

static bool s_format_filterable(WGPUTextureFormat f)
{
	CF_WFormatInfo fi = s_format_info(f);
	if (fi.depth || fi.sint || fi.uint) return false;
	if (fi.float32) return g.float32_filterable;
	return true;
}

static bool s_format_blendable(WGPUTextureFormat f)
{
	CF_WFormatInfo fi = s_format_info(f);
	if (fi.depth || fi.sint || fi.uint) return false;
	if (fi.float32) return g.float32_blendable;
	return true;
}

static WGPUTextureFormat s_storage_format_from_name(const char* s, int n)
{
	struct { const char* name; WGPUTextureFormat f; } table[] = {
		{ "rgba8unorm", WGPUTextureFormat_RGBA8Unorm }, { "rgba8snorm", WGPUTextureFormat_RGBA8Snorm },
		{ "rgba8uint", WGPUTextureFormat_RGBA8Uint }, { "rgba8sint", WGPUTextureFormat_RGBA8Sint },
		{ "rgba16uint", WGPUTextureFormat_RGBA16Uint }, { "rgba16sint", WGPUTextureFormat_RGBA16Sint },
		{ "rgba16float", WGPUTextureFormat_RGBA16Float }, { "r32uint", WGPUTextureFormat_R32Uint },
		{ "r32sint", WGPUTextureFormat_R32Sint }, { "r32float", WGPUTextureFormat_R32Float },
		{ "rg32uint", WGPUTextureFormat_RG32Uint }, { "rg32sint", WGPUTextureFormat_RG32Sint },
		{ "rg32float", WGPUTextureFormat_RG32Float }, { "rgba32uint", WGPUTextureFormat_RGBA32Uint },
		{ "rgba32sint", WGPUTextureFormat_RGBA32Sint }, { "rgba32float", WGPUTextureFormat_RGBA32Float },
		{ "bgra8unorm", WGPUTextureFormat_BGRA8Unorm }, { "r8unorm", WGPUTextureFormat_R8Unorm },
		{ "r8snorm", WGPUTextureFormat_R8Snorm }, { "r8uint", WGPUTextureFormat_R8Uint },
		{ "r8sint", WGPUTextureFormat_R8Sint }, { "rg8unorm", WGPUTextureFormat_RG8Unorm },
		{ "rg8uint", WGPUTextureFormat_RG8Uint }, { "rg8sint", WGPUTextureFormat_RG8Sint },
		{ "r16uint", WGPUTextureFormat_R16Uint }, { "r16sint", WGPUTextureFormat_R16Sint },
		{ "r16float", WGPUTextureFormat_R16Float }, { "rg16uint", WGPUTextureFormat_RG16Uint },
		{ "rg16sint", WGPUTextureFormat_RG16Sint }, { "rg16float", WGPUTextureFormat_RG16Float },
		{ "rgb10a2unorm", WGPUTextureFormat_RGB10A2Unorm }, { "rg11b10ufloat", WGPUTextureFormat_RG11B10Ufloat },
		{ "r16unorm", WGPUTextureFormat_R16Unorm }, { "rgba16unorm", WGPUTextureFormat_RGBA16Unorm },
	};
	for (int i = 0; i < (int)(sizeof(table) / sizeof(table[0])); ++i) {
		if ((int)strlen(table[i].name) == n && !strncmp(table[i].name, s, n)) return table[i].f;
	}
	return WGPUTextureFormat_Undefined;
}

static WGPUCompareFunction s_wrap(CF_CompareFunction f)
{
	switch (f) {
	case CF_COMPARE_FUNCTION_NEVER:                 return WGPUCompareFunction_Never;
	case CF_COMPARE_FUNCTION_LESS_THAN:             return WGPUCompareFunction_Less;
	case CF_COMPARE_FUNCTION_EQUAL:                 return WGPUCompareFunction_Equal;
	case CF_COMPARE_FUNCTION_NOT_EQUAL:             return WGPUCompareFunction_NotEqual;
	case CF_COMPARE_FUNCTION_LESS_THAN_OR_EQUAL:    return WGPUCompareFunction_LessEqual;
	case CF_COMPARE_FUNCTION_GREATER_THAN:          return WGPUCompareFunction_Greater;
	case CF_COMPARE_FUNCTION_GREATER_THAN_OR_EQUAL: return WGPUCompareFunction_GreaterEqual;
	default:                                        return WGPUCompareFunction_Always;
	}
}

static WGPUCullMode s_wrap(CF_CullMode m)
{
	switch (m) {
	case CF_CULL_MODE_FRONT: return WGPUCullMode_Front;
	case CF_CULL_MODE_BACK:  return WGPUCullMode_Back;
	default:                 return WGPUCullMode_None;
	}
}

static WGPUStencilOperation s_wrap(CF_StencilOp op)
{
	switch (op) {
	case CF_STENCIL_OP_ZERO:            return WGPUStencilOperation_Zero;
	case CF_STENCIL_OP_REPLACE:         return WGPUStencilOperation_Replace;
	case CF_STENCIL_OP_INCREMENT_CLAMP: return WGPUStencilOperation_IncrementClamp;
	case CF_STENCIL_OP_DECREMENT_CLAMP: return WGPUStencilOperation_DecrementClamp;
	case CF_STENCIL_OP_INVERT:          return WGPUStencilOperation_Invert;
	case CF_STENCIL_OP_INCREMENT_WRAP:  return WGPUStencilOperation_IncrementWrap;
	case CF_STENCIL_OP_DECREMENT_WRAP:  return WGPUStencilOperation_DecrementWrap;
	default:                            return WGPUStencilOperation_Keep;
	}
}

static WGPUBlendOperation s_wrap(CF_BlendOp op)
{
	switch (op) {
	case CF_BLEND_OP_SUBTRACT:         return WGPUBlendOperation_Subtract;
	case CF_BLEND_OP_REVERSE_SUBTRACT: return WGPUBlendOperation_ReverseSubtract;
	case CF_BLEND_OP_MIN:              return WGPUBlendOperation_Min;
	case CF_BLEND_OP_MAX:              return WGPUBlendOperation_Max;
	default:                           return WGPUBlendOperation_Add;
	}
}

static WGPUBlendFactor s_wrap(CF_BlendFactor f)
{
	switch (f) {
	case CF_BLENDFACTOR_ZERO:                     return WGPUBlendFactor_Zero;
	case CF_BLENDFACTOR_ONE:                      return WGPUBlendFactor_One;
	case CF_BLENDFACTOR_SRC_COLOR:                return WGPUBlendFactor_Src;
	case CF_BLENDFACTOR_ONE_MINUS_SRC_COLOR:      return WGPUBlendFactor_OneMinusSrc;
	case CF_BLENDFACTOR_DST_COLOR:                return WGPUBlendFactor_Dst;
	case CF_BLENDFACTOR_ONE_MINUS_DST_COLOR:      return WGPUBlendFactor_OneMinusDst;
	case CF_BLENDFACTOR_SRC_ALPHA:                return WGPUBlendFactor_SrcAlpha;
	case CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:      return WGPUBlendFactor_OneMinusSrcAlpha;
	case CF_BLENDFACTOR_DST_ALPHA:                return WGPUBlendFactor_DstAlpha;
	case CF_BLENDFACTOR_ONE_MINUS_DST_ALPHA:      return WGPUBlendFactor_OneMinusDstAlpha;
	case CF_BLENDFACTOR_CONSTANT_COLOR:           return WGPUBlendFactor_Constant;
	case CF_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR: return WGPUBlendFactor_OneMinusConstant;
	case CF_BLENDFACTOR_SRC_ALPHA_SATURATE:       return WGPUBlendFactor_SrcAlphaSaturated;
	default:                                      return WGPUBlendFactor_Zero;
	}
}

static WGPUPrimitiveTopology s_wrap(CF_PrimitiveType t)
{
	switch (t) {
	case CF_PRIMITIVE_TYPE_TRIANGLESTRIP: return WGPUPrimitiveTopology_TriangleStrip;
	case CF_PRIMITIVE_TYPE_LINELIST:      return WGPUPrimitiveTopology_LineList;
	case CF_PRIMITIVE_TYPE_LINESTRIP:     return WGPUPrimitiveTopology_LineStrip;
	default:                              return WGPUPrimitiveTopology_TriangleList;
	}
}

static WGPUFilterMode s_wrap(CF_Filter f) { return f == CF_FILTER_NEAREST ? WGPUFilterMode_Nearest : WGPUFilterMode_Linear; }
static WGPUMipmapFilterMode s_wrap(CF_MipFilter f) { return f == CF_MIP_FILTER_NEAREST ? WGPUMipmapFilterMode_Nearest : WGPUMipmapFilterMode_Linear; }

static WGPUAddressMode s_wrap(CF_WrapMode m)
{
	switch (m) {
	case CF_WRAP_MODE_CLAMP_TO_EDGE:   return WGPUAddressMode_ClampToEdge;
	case CF_WRAP_MODE_MIRRORED_REPEAT: return WGPUAddressMode_MirrorRepeat;
	default:                           return WGPUAddressMode_Repeat;
	}
}

static WGPUVertexFormat s_wrap(CF_VertexFormat f)
{
	switch (f) {
	case CF_VERTEX_FORMAT_INT:          return WGPUVertexFormat_Sint32;
	case CF_VERTEX_FORMAT_INT2:         return WGPUVertexFormat_Sint32x2;
	case CF_VERTEX_FORMAT_INT3:         return WGPUVertexFormat_Sint32x3;
	case CF_VERTEX_FORMAT_INT4:         return WGPUVertexFormat_Sint32x4;
	case CF_VERTEX_FORMAT_UINT:         return WGPUVertexFormat_Uint32;
	case CF_VERTEX_FORMAT_UINT2:        return WGPUVertexFormat_Uint32x2;
	case CF_VERTEX_FORMAT_UINT3:        return WGPUVertexFormat_Uint32x3;
	case CF_VERTEX_FORMAT_UINT4:        return WGPUVertexFormat_Uint32x4;
	case CF_VERTEX_FORMAT_FLOAT:        return WGPUVertexFormat_Float32;
	case CF_VERTEX_FORMAT_FLOAT2:       return WGPUVertexFormat_Float32x2;
	case CF_VERTEX_FORMAT_FLOAT3:       return WGPUVertexFormat_Float32x3;
	case CF_VERTEX_FORMAT_FLOAT4:       return WGPUVertexFormat_Float32x4;
	case CF_VERTEX_FORMAT_BYTE2:        return WGPUVertexFormat_Sint8x2;
	case CF_VERTEX_FORMAT_BYTE4:        return WGPUVertexFormat_Sint8x4;
	case CF_VERTEX_FORMAT_UBYTE2:       return WGPUVertexFormat_Uint8x2;
	case CF_VERTEX_FORMAT_UBYTE4:       return WGPUVertexFormat_Uint8x4;
	case CF_VERTEX_FORMAT_BYTE2_NORM:   return WGPUVertexFormat_Snorm8x2;
	case CF_VERTEX_FORMAT_BYTE4_NORM:   return WGPUVertexFormat_Snorm8x4;
	case CF_VERTEX_FORMAT_UBYTE2_NORM:  return WGPUVertexFormat_Unorm8x2;
	case CF_VERTEX_FORMAT_UBYTE4_NORM:  return WGPUVertexFormat_Unorm8x4;
	case CF_VERTEX_FORMAT_SHORT2:       return WGPUVertexFormat_Sint16x2;
	case CF_VERTEX_FORMAT_SHORT4:       return WGPUVertexFormat_Sint16x4;
	case CF_VERTEX_FORMAT_USHORT2:      return WGPUVertexFormat_Uint16x2;
	case CF_VERTEX_FORMAT_USHORT4:      return WGPUVertexFormat_Uint16x4;
	case CF_VERTEX_FORMAT_SHORT2_NORM:  return WGPUVertexFormat_Snorm16x2;
	case CF_VERTEX_FORMAT_SHORT4_NORM:  return WGPUVertexFormat_Snorm16x4;
	case CF_VERTEX_FORMAT_USHORT2_NORM: return WGPUVertexFormat_Unorm16x2;
	case CF_VERTEX_FORMAT_USHORT4_NORM: return WGPUVertexFormat_Unorm16x4;
	case CF_VERTEX_FORMAT_HALF2:        return WGPUVertexFormat_Float16x2;
	case CF_VERTEX_FORMAT_HALF4:        return WGPUVertexFormat_Float16x4;
	default:                            return WGPUVertexFormat_Float32;
	}
}

static CF_ShaderInputFormat s_wrap(CF_ShaderInfoDataType type)
{
	switch (type) {
	case CF_SHADER_INFO_TYPE_UINT:   return CF_SHADER_INPUT_FORMAT_UINT;
	case CF_SHADER_INFO_TYPE_SINT:   return CF_SHADER_INPUT_FORMAT_INT;
	case CF_SHADER_INFO_TYPE_FLOAT:  return CF_SHADER_INPUT_FORMAT_FLOAT;
	case CF_SHADER_INFO_TYPE_UINT2:  return CF_SHADER_INPUT_FORMAT_UVEC2;
	case CF_SHADER_INFO_TYPE_SINT2:  return CF_SHADER_INPUT_FORMAT_IVEC2;
	case CF_SHADER_INFO_TYPE_FLOAT2: return CF_SHADER_INPUT_FORMAT_VEC2;
	case CF_SHADER_INFO_TYPE_UINT3:  return CF_SHADER_INPUT_FORMAT_UVEC3;
	case CF_SHADER_INFO_TYPE_SINT3:  return CF_SHADER_INPUT_FORMAT_IVEC3;
	case CF_SHADER_INFO_TYPE_FLOAT3: return CF_SHADER_INPUT_FORMAT_VEC3;
	case CF_SHADER_INFO_TYPE_UINT4:  return CF_SHADER_INPUT_FORMAT_UVEC4;
	case CF_SHADER_INFO_TYPE_SINT4:  return CF_SHADER_INPUT_FORMAT_IVEC4;
	case CF_SHADER_INFO_TYPE_FLOAT4: return CF_SHADER_INPUT_FORMAT_VEC4;
	default:                         return CF_SHADER_INPUT_FORMAT_UNKNOWN;
	}
}

//--------------------------------------------------------------------------------------------------
// WGSL binding declarations.

static const char* s_skip_ws(const char* p)
{
	for (;;) {
		while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
		if (p[0] == '/' && p[1] == '/') { while (*p && *p != '\n') ++p; continue; }
		if (p[0] == '/' && p[1] == '*') { p += 2; while (*p && !(p[0] == '*' && p[1] == '/')) ++p; if (*p) p += 2; continue; }
		return p;
	}
}

static bool s_starts(const char* p, const char* word) { return !strncmp(p, word, strlen(word)); }

static WGPUTextureSampleType s_sample_type_from_param(const char* p)
{
	// p points just past '<'.
	p = s_skip_ws(p);
	if (s_starts(p, "i32")) return WGPUTextureSampleType_Sint;
	if (s_starts(p, "u32")) return WGPUTextureSampleType_Uint;
	return WGPUTextureSampleType_Float;
}

// Parses one `var` declaration's type into b. Returns false for anything not a resource.
static bool s_parse_wgsl_type(CF_WBinding* b, const char* address_space, int address_space_len, const char* type)
{
	if (address_space_len > 0) {
		if (!strncmp(address_space, "uniform", 7)) { b->kind = CF_WBIND_UNIFORM; return true; }
		if (!strncmp(address_space, "storage", 7)) {
			const char* comma = (const char*)memchr(address_space, ',', address_space_len);
			bool read_only = true;
			if (comma) {
				const char* a = s_skip_ws(comma + 1);
				read_only = !s_starts(a, "read_write");
			} else {
				read_only = true; // var<storage> defaults to read.
			}
			b->kind = read_only ? CF_WBIND_STORAGE_RO : CF_WBIND_STORAGE_RW;
			return true;
		}
		return false;
	}
	if (s_starts(type, "sampler_comparison")) { b->kind = CF_WBIND_SAMPLER_CMP; return true; }
	if (s_starts(type, "sampler")) { b->kind = CF_WBIND_SAMPLER; return true; }
	if (s_starts(type, "texture_storage_")) {
		const char* t = type + strlen("texture_storage_");
		b->kind = CF_WBIND_STORAGE_TEXTURE;
		if (s_starts(t, "2d_array")) b->dim = WGPUTextureViewDimension_2DArray;
		else if (s_starts(t, "3d")) b->dim = WGPUTextureViewDimension_3D;
		else if (s_starts(t, "1d")) b->dim = WGPUTextureViewDimension_1D;
		else b->dim = WGPUTextureViewDimension_2D;
		const char* lt = strchr(t, '<');
		if (!lt) return false;
		const char* f = s_skip_ws(lt + 1);
		const char* fe = f;
		while ((*fe >= 'a' && *fe <= 'z') || (*fe >= '0' && *fe <= '9')) ++fe;
		b->storage_format = s_storage_format_from_name(f, (int)(fe - f));
		const char* a = strchr(fe, ',');
		b->access = WGPUStorageTextureAccess_WriteOnly;
		if (a) {
			a = s_skip_ws(a + 1);
			if (s_starts(a, "read_write")) b->access = WGPUStorageTextureAccess_ReadWrite;
			else if (s_starts(a, "read")) b->access = WGPUStorageTextureAccess_ReadOnly;
		}
		return true;
	}
	if (s_starts(type, "texture_depth_")) {
		const char* t = type + strlen("texture_depth_");
		b->kind = CF_WBIND_TEXTURE;
		b->sample_type = WGPUTextureSampleType_Depth;
		if (s_starts(t, "multisampled_2d")) { b->dim = WGPUTextureViewDimension_2D; b->multisampled = true; }
		else if (s_starts(t, "2d_array")) b->dim = WGPUTextureViewDimension_2DArray;
		else if (s_starts(t, "cube_array")) b->dim = WGPUTextureViewDimension_CubeArray;
		else if (s_starts(t, "cube")) b->dim = WGPUTextureViewDimension_Cube;
		else b->dim = WGPUTextureViewDimension_2D;
		return true;
	}
	if (s_starts(type, "texture_")) {
		const char* t = type + strlen("texture_");
		b->kind = CF_WBIND_TEXTURE;
		if (s_starts(t, "multisampled_2d")) { b->dim = WGPUTextureViewDimension_2D; b->multisampled = true; }
		else if (s_starts(t, "2d_array")) b->dim = WGPUTextureViewDimension_2DArray;
		else if (s_starts(t, "cube_array")) b->dim = WGPUTextureViewDimension_CubeArray;
		else if (s_starts(t, "cube")) b->dim = WGPUTextureViewDimension_Cube;
		else if (s_starts(t, "3d")) b->dim = WGPUTextureViewDimension_3D;
		else if (s_starts(t, "1d")) b->dim = WGPUTextureViewDimension_1D;
		else b->dim = WGPUTextureViewDimension_2D;
		const char* lt = strchr(t, '<');
		b->sample_type = lt ? s_sample_type_from_param(lt + 1) : WGPUTextureSampleType_Float;
		return true;
	}
	return false;
}

static void s_add_binding(CF_WLayoutInfo* info, CF_WBinding b)
{
	for (int i = 0; i < info->count; ++i) {
		if (info->b[i].group == b.group && info->b[i].binding == b.binding) {
			info->b[i].visibility = (WGPUShaderStage)(info->b[i].visibility | b.visibility);
			return;
		}
	}
	CF_ASSERT(info->count < CF_WGPU_MAX_BINDINGS);
	if (info->count >= CF_WGPU_MAX_BINDINGS) return;
	int i = info->count++;
	while (i > 0 && (info->b[i - 1].group > b.group || (info->b[i - 1].group == b.group && info->b[i - 1].binding > b.binding))) {
		info->b[i] = info->b[i - 1];
		--i;
	}
	info->b[i] = b;
	if (b.group + 1 > info->group_count) info->group_count = b.group + 1;
}

static void s_parse_wgsl_bindings(CF_WLayoutInfo* info, const char* src, WGPUShaderStage stage)
{
	const char* p = src;
	while ((p = strstr(p, "@group")) != NULL) {
		CF_WBinding b = { };
		b.visibility = stage;
		const char* q = s_skip_ws(p + 6);
		if (*q != '(') { p += 6; continue; }
		b.group = atoi(q + 1);
		const char* bb = strstr(q, "@binding");
		const char* var = strstr(q, "var");
		if (!bb || !var) break;
		const char* bq = s_skip_ws(bb + 8);
		b.binding = atoi(bq + 1);
		const char* r = var + 3;
		const char* as = NULL;
		int as_len = 0;
		r = s_skip_ws(r);
		if (*r == '<') {
			as = r + 1;
			const char* gt = strchr(r, '>');
			if (!gt) break;
			as_len = (int)(gt - as);
			as = s_skip_ws(as);
			r = gt + 1;
		}
		r = s_skip_ws(r);
		const char* name = r;
		while ((*r >= 'a' && *r <= 'z') || (*r >= 'A' && *r <= 'Z') || (*r >= '0' && *r <= '9') || *r == '_') ++r;
		int name_len = (int)(r - name);
		if (name_len >= (int)sizeof(b.name)) name_len = (int)sizeof(b.name) - 1;
		memcpy(b.name, name, name_len);
		b.name[name_len] = 0;
		r = s_skip_ws(r);
		if (*r == ':') r = s_skip_ws(r + 1);
		if (s_parse_wgsl_type(&b, as, as_len, r)) s_add_binding(info, b);
		p = r;
	}
}

static const CF_WBinding* s_find_binding(const CF_WLayoutInfo* info, int group, int binding)
{
	for (int i = 0; i < info->count; ++i) {
		if (info->b[i].group == group && info->b[i].binding == binding) return info->b + i;
	}
	return NULL;
}

//--------------------------------------------------------------------------------------------------
// Samplers, deduplicated by descriptor.

static CF_WSampler* s_get_sampler(WGPUSamplerDescriptor desc)
{
	desc.label = WGPU_STRING_VIEW_INIT;
	desc.nextInChain = NULL;
	// Anisotropy demands linear filtering everywhere; drop it otherwise rather than fail validation.
	if (desc.maxAnisotropy > 1 && (desc.minFilter != WGPUFilterMode_Linear || desc.magFilter != WGPUFilterMode_Linear || desc.mipmapFilter != WGPUMipmapFilterMode_Linear)) {
		desc.maxAnisotropy = 1;
	}
	if (desc.maxAnisotropy < 1) desc.maxAnisotropy = 1;
	for (int i = 0; i < g.samplers.count(); ++i) {
		const WGPUSamplerDescriptor& d = g.samplers[i]->desc;
		if (d.addressModeU == desc.addressModeU && d.addressModeV == desc.addressModeV && d.addressModeW == desc.addressModeW
			&& d.magFilter == desc.magFilter && d.minFilter == desc.minFilter && d.mipmapFilter == desc.mipmapFilter
			&& d.lodMinClamp == desc.lodMinClamp && d.lodMaxClamp == desc.lodMaxClamp && d.compare == desc.compare
			&& d.maxAnisotropy == desc.maxAnisotropy) {
			return g.samplers[i];
		}
	}
	CF_WSampler* s = (CF_WSampler*)CF_CALLOC(sizeof(CF_WSampler));
	s->desc = desc;
	s->sampler = wgpuDeviceCreateSampler(g.device, &desc);
	g.samplers.add(s);
	return s;
}

static CF_WSampler* s_acquire_sampler(WGPUSamplerDescriptor desc)
{
	CF_WSampler* s = s_get_sampler(desc);
	s->refs++;
	return s;
}

static CF_WSampler* s_pinned_sampler(WGPUSamplerDescriptor desc)
{
	CF_WSampler* s = s_get_sampler(desc);
	s->pinned = true;
	return s;
}

static void s_release_sampler(CF_WSampler* s)
{
	if (!s || --s->refs > 0 || s->pinned) return;
	for (int i = 0; i < g.samplers.count(); ++i) {
		if (g.samplers[i] == s) {
			g.samplers.unordered_remove(i);
			break;
		}
	}
	wgpuSamplerRelease(s->sampler);
	CF_FREE(s);
	g.bind_epoch++;
}

static WGPUSamplerDescriptor s_sampler_desc_defaults()
{
	WGPUSamplerDescriptor d = WGPU_SAMPLER_DESCRIPTOR_INIT;
	d.addressModeU = d.addressModeV = d.addressModeW = WGPUAddressMode_Repeat;
	d.magFilter = d.minFilter = WGPUFilterMode_Nearest;
	d.mipmapFilter = WGPUMipmapFilterMode_Linear;
	d.lodMinClamp = 0;
	d.lodMaxClamp = 1000.0f;
	d.compare = WGPUCompareFunction_Undefined;
	d.maxAnisotropy = 1;
	return d;
}

// The sampler for one sampled slot: the authored one, adjusted to what the slot's layout accepts.
static WGPUSampler s_slot_sampler(CF_WSampler* base, CF_WBindKind slot_kind, bool unfilterable)
{
	WGPUSamplerDescriptor d = base ? base->desc : s_sampler_desc_defaults();
	bool changed = false;
	if (slot_kind == CF_WBIND_SAMPLER_CMP) {
		if (d.compare == WGPUCompareFunction_Undefined) { d.compare = WGPUCompareFunction_LessEqual; changed = true; }
	} else {
		if (d.compare != WGPUCompareFunction_Undefined) { d.compare = WGPUCompareFunction_Undefined; changed = true; }
		if (unfilterable && (d.minFilter != WGPUFilterMode_Nearest || d.magFilter != WGPUFilterMode_Nearest || d.mipmapFilter != WGPUMipmapFilterMode_Nearest)) {
			d.minFilter = d.magFilter = WGPUFilterMode_Nearest;
			d.mipmapFilter = WGPUMipmapFilterMode_Nearest;
			d.maxAnisotropy = 1;
			changed = true;
		}
	}
	if (!changed && base) return base->sampler;
	return s_pinned_sampler(d)->sampler;
}

//--------------------------------------------------------------------------------------------------
// Command encoding and submission.

static WGPUCommandEncoder s_encoder()
{
	if (!g.encoder) {
		WGPUCommandEncoderDescriptor desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
		g.encoder = wgpuDeviceCreateCommandEncoder(g.device, &desc);
	}
	return g.encoder;
}

static void s_end_active_pass()
{
	if (g.pass) {
		wgpuRenderPassEncoderEnd(g.pass);
		wgpuRenderPassEncoderRelease(g.pass);
		g.pass = NULL;
		g.ps = { };
	}
	g.shader = NULL;
	g.material = NULL;
	g.variant = NULL;
}

static void s_make_ring(int size)
{
	if (g.ring) wgpuBufferRelease(g.ring);
	CF_FREE(g.ring_cpu);
	g.ring_size = size;
	g.ring_cpu = (uint8_t*)CF_ALLOC(size);
	WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
	desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
	desc.size = (uint64_t)size;
	g.ring = wgpuDeviceCreateBuffer(g.device, &desc);
	g.ring_generation++;
	g.ring_used = 0;
}

static void s_free_staging_chunk(CF_WStagingChunk* c)
{
	if (c->buffer) wgpuBufferRelease(c->buffer);
	CF_FREE(c->cpu);
	CF_FREE(c);
}

static void s_on_staging_mapped(WGPUMapAsyncStatus status, WGPUStringView message, void* ud1, void* ud2)
{
	CF_UNUSED(message); CF_UNUSED(ud2);
	CF_WStagingChunk* c = (CF_WStagingChunk*)ud1;
	c->map_failed = status != WGPUMapAsyncStatus_Success;
	c->map_done = true;
}

// Moves remapped chunks to the free list.
static void s_collect_staging()
{
	for (int i = 0; i < g.staging_pending.count();) {
		CF_WStagingChunk* c = g.staging_pending[i];
		if (!c->map_done) { ++i; continue; }
		g.staging_pending.unordered_remove(i);
		if (c->map_failed || g.device_lost) {
			s_free_staging_chunk(c);
		} else {
			c->mapped = (uint8_t*)wgpuBufferGetMappedRange(c->buffer, 0, (size_t)c->size);
			c->used = 0;
			g.staging_free.add(c);
		}
	}
}

static void s_submit()
{
	s_end_active_pass();
	for (int i = 0; i < g.staging.count(); ++i) {
		if (g.staging[i]->buffer) wgpuBufferUnmap(g.staging[i]->buffer);
	}
	if (g.device_lost) {
		// Nothing reaches a lost device; the recorded work is dropped.
		if (g.encoder) { wgpuCommandEncoderRelease(g.encoder); g.encoder = NULL; }
		for (int i = 0; i < g.staging.count(); ++i) s_free_staging_chunk(g.staging[i]);
		g.staging.clear();
		g.ring_used = 0;
		return;
	}
	if (g.ring_used) {
		wgpuQueueWriteBuffer(g.queue, g.ring, 0, g.ring_cpu, (size_t)g.ring_used);
	}
	if (g.encoder) {
		WGPUCommandBufferDescriptor cdesc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
		WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(g.encoder, &cdesc);
		wgpuQueueSubmit(g.queue, 1, &cmd);
		wgpuCommandBufferRelease(cmd);
		wgpuCommandEncoderRelease(g.encoder);
		g.encoder = NULL;
	}
	for (int i = 0; i < g.staging.count(); ++i) {
		CF_WStagingChunk* c = g.staging[i];
		if (c->size != CF_WGPU_STAGING_CHUNK) {
			s_free_staging_chunk(c);
			continue;
		}
		c->mapped = NULL;
		c->map_done = false;
		c->map_failed = false;
		WGPUBufferMapCallbackInfo cb = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
		cb.mode = WGPUCallbackMode_AllowProcessEvents;
		cb.callback = s_on_staging_mapped;
		cb.userdata1 = c;
		wgpuBufferMapAsync(c->buffer, WGPUMapMode_Write, 0, (size_t)c->size, cb);
		g.staging_pending.add(c);
	}
	g.staging.clear();
	g.ring_used = 0;
	if (g.ring_wanted_size > g.ring_size) {
		s_make_ring(g.ring_wanted_size);
	}
}

// Staging memory for one upload; the copy reading it must be recorded into the current encoder.
static uint8_t* s_stage(int size, WGPUBuffer* buffer, uint64_t* offset)
{
	uint64_t need = s_align64((uint64_t)size, 4);
	CF_WStagingChunk* chunk = g.staging.count() ? g.staging.last() : NULL;
	if (!chunk || s_align64(chunk->used, 256) + need > chunk->size) {
		chunk = NULL;
		if (need <= CF_WGPU_STAGING_CHUNK) {
			if (!g.staging_free.count() && g.staging_pending.count()) {
				s_process_events();
				s_collect_staging();
			}
			// While the GPU is busy (browsers compiling startup pipelines), maps return late; making
			// a fresh 4 MiB mapped chunk per upload instead exhausted the browser's GPU process.
			if (!g.staging_free.count() && g.staging_pending.count() >= CF_WGPU_STAGING_MAX_PENDING && !g.device_lost) {
				s_wait(&g.staging_pending[0]->map_done);
				s_collect_staging();
			}
			if (g.staging_free.count()) chunk = g.staging_free.pop();
		}
		if (!chunk) {
			uint64_t chunk_size = need > CF_WGPU_STAGING_CHUNK ? s_align64(need, 256) : CF_WGPU_STAGING_CHUNK;
			WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
			desc.usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_MapWrite;
			desc.size = chunk_size;
			desc.mappedAtCreation = true;
			chunk = (CF_WStagingChunk*)CF_CALLOC(sizeof(CF_WStagingChunk));
			chunk->buffer = wgpuDeviceCreateBuffer(g.device, &desc);
			chunk->size = chunk_size;
			// The browser throws instead of mapping once its GPU process is gone, before the lost
			// callback has run.
			if (!chunk->buffer) s_mark_device_lost(s_sv("a mapped staging buffer could not be created"));
			if (g.device_lost) {
				// Mapping a buffer the lost device failed to make aborts the process in wgpu-native.
				chunk->cpu = (uint8_t*)CF_ALLOC((size_t)chunk_size);
				chunk->mapped = chunk->cpu;
			} else {
				chunk->mapped = (uint8_t*)wgpuBufferGetMappedRange(chunk->buffer, 0, (size_t)chunk_size);
			}
		}
		g.staging.add(chunk);
	}
	uint64_t at = s_align64(chunk->used, 256);
	chunk->used = at + need;
	*buffer = chunk->buffer;
	*offset = at;
	return chunk->mapped + at;
}

// Makes room for `bytes` of uniform blocks in this submission. A full ring submits everything
// recorded so far (which reads the ring as it stands) and starts over; the next ring grows so
// steady-state frames fit. Called before any block of a draw is packed, so one draw's blocks
// never straddle two rings.
static void s_ring_reserve(int bytes)
{
	if (g.ring_used + bytes <= g.ring_size) return;
	g.ring_wanted_size = cf_max(g.ring_size * 2, bytes * 2);
	CF_WPassState ps = g.ps;
	s_submit();
	g.ps = ps;
}

static uint32_t s_ring_alloc(const void* data, int size)
{
	int need = s_align(cf_max(size, 16), CF_WGPU_UNIFORM_ALIGN);
	if (g.ring_used + need > g.ring_size) s_ring_reserve(need);
	uint32_t at = (uint32_t)g.ring_used;
	CF_MEMSET(g.ring_cpu + at, 0, need);
	if (data) CF_MEMCPY(g.ring_cpu + at, data, size);
	g.ring_used += need;
	return at;
}

// Ring bytes a group's uniform blocks take.
static int s_ring_bytes(const CF_WLayoutInfo* info, int group, const int* block_sizes)
{
	int bytes = 0;
	for (int i = 0; i < info->count; ++i) {
		const CF_WBinding& b = info->b[i];
		if (b.group != group || b.kind != CF_WBIND_UNIFORM) continue;
		int size = b.binding < CF_MAX_UNIFORM_BLOCK_COUNT ? block_sizes[b.binding] : 16;
		bytes += s_align(cf_max(size, 16), CF_WGPU_UNIFORM_ALIGN);
	}
	return bytes;
}

static WGPUBuffer s_make_buffer(int size, WGPUBufferUsage usage)
{
	WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
	desc.usage = usage | WGPUBufferUsage_CopyDst;
	desc.size = s_align64((uint64_t)cf_max(size, 4), 4);
	return wgpuDeviceCreateBuffer(g.device, &desc);
}

static void s_upload_buffer(WGPUBuffer dst, uint64_t dst_offset, const void* data, int size)
{
	if (size <= 0 || !data || g.device_lost) return;
	s_end_active_pass();
	WGPUBuffer src;
	uint64_t src_offset;
	uint8_t* p = s_stage(size, &src, &src_offset);
	CF_MEMCPY(p, data, size);
	int padded = s_align(size, 4);
	if (padded != size) CF_MEMSET(p + size, 0, padded - size);
	if (!src) return;
	wgpuCommandEncoderCopyBufferToBuffer(s_encoder(), src, src_offset, dst, dst_offset, (uint64_t)padded);
}

//--------------------------------------------------------------------------------------------------
// Device setup.

struct CF_WRequest
{
	volatile bool done;
	bool ok;
	WGPUAdapter adapter;
	WGPUDevice device;
};

static void s_on_adapter(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* ud1, void* ud2)
{
	CF_UNUSED(ud2);
	CF_WRequest* r = (CF_WRequest*)ud1;
	r->ok = status == WGPURequestAdapterStatus_Success && adapter;
	r->adapter = adapter;
	if (!r->ok) s_print_sv("WebGPU: no adapter: ", message);
	r->done = true;
}

static void s_on_device(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* ud1, void* ud2)
{
	CF_UNUSED(ud2);
	CF_WRequest* r = (CF_WRequest*)ud1;
	r->ok = status == WGPURequestDeviceStatus_Success && device;
	r->device = device;
	if (!r->ok) s_print_sv("WebGPU: device request failed: ", message);
	r->done = true;
}

static void s_on_uncaptured_error(WGPUDevice const* device, WGPUErrorType type, WGPUStringView message, void* ud1, void* ud2)
{
	CF_UNUSED(device); CF_UNUSED(ud1); CF_UNUSED(ud2);
	if (g.device_lost) return;
	if (s_error_means_lost(message)) {
		s_mark_device_lost(message);
		return;
	}
	g.error_count++;
	char prefix[64];
	snprintf(prefix, sizeof(prefix), "WebGPU error (type %d): ", (int)type);
	s_print_sv(prefix, message);
}

static void s_on_device_lost(WGPUDevice const* device, WGPUDeviceLostReason reason, WGPUStringView message, void* ud1, void* ud2)
{
	CF_UNUSED(device); CF_UNUSED(ud1); CF_UNUSED(ud2);
	if (reason == WGPUDeviceLostReason_CallbackCancelled) return;
	// wgpu-native reports every loss it does deliver as Destroyed. Dawn means it: CF dropping the
	// device at shutdown.
#ifdef CF_EMSCRIPTEN
	if (reason == WGPUDeviceLostReason_Destroyed && !g.lose_on_destroy) return;
#endif
	s_mark_device_lost(message);
}

#ifndef CF_EMSCRIPTEN
static void s_on_log(WGPULogLevel level, WGPUStringView message, void* userdata)
{
	CF_UNUSED(userdata);
	char prefix[32];
	snprintf(prefix, sizeof(prefix), "wgpu [%d]: ", (int)level);
	s_print_sv(prefix, message);
}
#endif

static bool s_has_feature(const WGPUSupportedFeatures* f, WGPUFeatureName name)
{
	for (size_t i = 0; i < f->featureCount; ++i) if (f->features[i] == name) return true;
	return false;
}

#ifdef CF_EMSCRIPTEN
// SDL picks the window's canvas from this hint, else "#canvas".
static WGPUSurface s_create_web_surface()
{
	const char* selector = SDL_GetHint(SDL_HINT_EMSCRIPTEN_CANVAS_SELECTOR);
	if (!selector || !*selector) selector = "#canvas";
	WGPUEmscriptenSurfaceSourceCanvasHTMLSelector src = WGPU_EMSCRIPTEN_SURFACE_SOURCE_CANVAS_HTML_SELECTOR_INIT;
	src.selector = s_sv(selector);
	WGPUSurfaceDescriptor sdesc = WGPU_SURFACE_DESCRIPTOR_INIT;
	sdesc.nextInChain = &src.chain;
	return wgpuInstanceCreateSurface(g.instance, &sdesc);
}
#endif

CF_Result cf_webgpu_init(bool debug)
{
	g.debug = debug;
#ifndef CF_EMSCRIPTEN
	if (debug) {
		wgpuSetLogCallback(s_on_log, NULL);
		wgpuSetLogLevel(WGPULogLevel_Warn);
	}
#endif
	WGPUInstanceDescriptor idesc = WGPU_INSTANCE_DESCRIPTOR_INIT;
	g.instance = wgpuCreateInstance(&idesc);
	if (!g.instance) return cf_result_error("WebGPU: failed to create an instance.");

	CF_WRequest req = { };
	WGPURequestAdapterOptions aopts = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
	aopts.powerPreference = WGPUPowerPreference_HighPerformance;
	WGPURequestAdapterCallbackInfo acb = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
	acb.mode = WGPUCallbackMode_AllowProcessEvents;
	acb.callback = s_on_adapter;
	acb.userdata1 = &req;
	wgpuInstanceRequestAdapter(g.instance, &aopts, acb);
	s_wait(&req.done);
	if (!req.ok) {
		wgpuInstanceRelease(g.instance);
		g.instance = NULL;
		return cf_result_error("WebGPU: no adapter available.");
	}
	g.adapter = req.adapter;

	WGPUAdapterInfo ainfo = WGPU_ADAPTER_INFO_INIT;
	if (wgpuAdapterGetInfo(g.adapter, &ainfo) == WGPUStatus_Success) {
		const WGPUStringView& d = ainfo.device.length ? ainfo.device : ainfo.description;
		size_t n = d.data ? (d.length == WGPU_STRLEN ? strlen(d.data) : d.length) : 0;
		if (n >= sizeof(g.adapter_name)) n = sizeof(g.adapter_name) - 1;
		if (n) memcpy(g.adapter_name, d.data, n);
		g.adapter_name[n] = 0;
		wgpuAdapterInfoFreeMembers(ainfo);
	}

	// Ask for every optional feature CF can use, and the adapter's full limits.
	WGPUSupportedFeatures supported = WGPU_SUPPORTED_FEATURES_INIT;
	wgpuAdapterGetFeatures(g.adapter, &supported);
	WGPUFeatureName wanted[] = {
		WGPUFeatureName_Float32Filterable, WGPUFeatureName_Float32Blendable, WGPUFeatureName_Depth32FloatStencil8,
		WGPUFeatureName_TextureCompressionBC, WGPUFeatureName_TextureFormatsTier1, WGPUFeatureName_RG11B10UfloatRenderable,
		WGPUFeatureName_DepthClipControl, WGPUFeatureName_IndirectFirstInstance, WGPUFeatureName_BGRA8UnormStorage,
#ifndef CF_EMSCRIPTEN
		(WGPUFeatureName)WGPUNativeFeature_TextureAdapterSpecificFormatFeatures,
#endif
	};
	WGPUFeatureName features[16];
	int feature_count = 0;
	for (int i = 0; i < (int)(sizeof(wanted) / sizeof(wanted[0])); ++i) {
		if (s_has_feature(&supported, wanted[i])) features[feature_count++] = wanted[i];
	}
	g.float32_filterable = s_has_feature(&supported, WGPUFeatureName_Float32Filterable);
	g.float32_blendable = s_has_feature(&supported, WGPUFeatureName_Float32Blendable);
	g.depth32_stencil8 = s_has_feature(&supported, WGPUFeatureName_Depth32FloatStencil8);
	g.bc = s_has_feature(&supported, WGPUFeatureName_TextureCompressionBC);
	g.formats_tier1 = s_has_feature(&supported, WGPUFeatureName_TextureFormatsTier1);
	g.rg11b10_renderable = s_has_feature(&supported, WGPUFeatureName_RG11B10UfloatRenderable) || g.formats_tier1;
	g.depth_clip_control = s_has_feature(&supported, WGPUFeatureName_DepthClipControl);
	g.bgra8_storage = s_has_feature(&supported, WGPUFeatureName_BGRA8UnormStorage);
#ifndef CF_EMSCRIPTEN
	g.adapter_formats = s_has_feature(&supported, (WGPUFeatureName)WGPUNativeFeature_TextureAdapterSpecificFormatFeatures);
#endif
	wgpuSupportedFeaturesFreeMembers(supported);

	WGPULimits limits = WGPU_LIMITS_INIT;
	wgpuAdapterGetLimits(g.adapter, &limits);

	WGPUDeviceDescriptor ddesc = WGPU_DEVICE_DESCRIPTOR_INIT;
	ddesc.requiredFeatureCount = (size_t)feature_count;
	ddesc.requiredFeatures = features;
	ddesc.requiredLimits = &limits;
	ddesc.uncapturedErrorCallbackInfo.callback = s_on_uncaptured_error;
	ddesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
	ddesc.deviceLostCallbackInfo.callback = s_on_device_lost;
	req = { };
	WGPURequestDeviceCallbackInfo dcb = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
	dcb.mode = WGPUCallbackMode_AllowProcessEvents;
	dcb.callback = s_on_device;
	dcb.userdata1 = &req;
	wgpuAdapterRequestDevice(g.adapter, &ddesc, dcb);
	s_wait(&req.done);
	if (!req.ok) {
		wgpuAdapterRelease(g.adapter);
		wgpuInstanceRelease(g.instance);
		g.adapter = NULL;
		g.instance = NULL;
		return cf_result_error("WebGPU: failed to create a device.");
	}
	g.device = req.device;
#ifdef CF_EMSCRIPTEN
	// The canvas is known before the window exists, so a canvas that refuses a WebGPU context
	// fails here, while the app can still choose WebGL 2.
	g.surface = s_create_web_surface();
	if (!g.surface) {
		wgpuDeviceRelease(g.device);
		wgpuAdapterRelease(g.adapter);
		wgpuInstanceRelease(g.instance);
		g.device = NULL;
		g.adapter = NULL;
		g.instance = NULL;
		return cf_result_error("WebGPU: the canvas did not provide a WebGPU context.");
	}
#endif
	g.queue = wgpuDeviceGetQueue(g.device);
	g.limits = WGPU_LIMITS_INIT;
	wgpuDeviceGetLimits(g.device, &g.limits);

	s_make_ring(4 * 1024 * 1024);
#ifdef CF_EMSCRIPTEN
	printf("CF: WebGPU backend on %s\n", g.adapter_name[0] ? g.adapter_name : "an unnamed adapter");
#endif

	WGPUBindGroupLayoutDescriptor edesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
	g.empty_bgl = wgpuDeviceCreateBindGroupLayout(g.device, &edesc);
	WGPUBindGroupDescriptor ebg = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
	ebg.layout = g.empty_bgl;
	g.empty_bg = wgpuDeviceCreateBindGroup(g.device, &ebg);
	g.dummy_buffer = s_make_buffer(256, WGPUBufferUsage_Storage | WGPUBufferUsage_Uniform);
	return cf_result_success();
}

const char* cf_webgpu_adapter_name()
{
	return g.adapter_name;
}

//--------------------------------------------------------------------------------------------------
// Surface.

static void s_configure_surface(int w, int h)
{
	if (!g.surface || w <= 0 || h <= 0) return;
	WGPUSurfaceConfiguration conf = WGPU_SURFACE_CONFIGURATION_INIT;
	conf.device = g.device;
	conf.format = g.surface_format;
	conf.usage = WGPUTextureUsage_RenderAttachment;
	conf.width = (uint32_t)w;
	conf.height = (uint32_t)h;
	conf.alphaMode = WGPUCompositeAlphaMode_Auto;
	conf.presentMode = g.present_mode;
	wgpuSurfaceConfigure(g.surface, &conf);
	g.surface_configured = true;
	g.surface_w = w;
	g.surface_h = h;
}

static bool s_present_mode_supported(WGPUPresentMode mode)
{
	for (int i = 0; i < g.supported_present_mode_count; ++i) if (g.supported_present_modes[i] == mode) return true;
	return false;
}

void cf_webgpu_attach(SDL_Window* window)
{
	g.window = window;
#ifdef CF_EMSCRIPTEN
	// Made by cf_webgpu_init, which fails over to WebGL 2 without it.
#elif defined(_WIN32)
	WGPUSurfaceSourceWindowsHWND src = WGPU_SURFACE_SOURCE_WINDOWS_HWND_INIT;
	src.hwnd = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	src.hinstance = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, NULL);
	WGPUSurfaceDescriptor sdesc = WGPU_SURFACE_DESCRIPTOR_INIT;
	sdesc.nextInChain = &src.chain;
	g.surface = wgpuInstanceCreateSurface(g.instance, &sdesc);
#elif defined(__APPLE__)
	// SDL makes the CAMetalLayer for a metal view; the window was created with SDL_WINDOW_METAL.
	g.metal_view = SDL_Metal_CreateView(window);
	WGPUSurfaceSourceMetalLayer src = WGPU_SURFACE_SOURCE_METAL_LAYER_INIT;
	src.layer = SDL_Metal_GetLayer(g.metal_view);
	WGPUSurfaceDescriptor sdesc = WGPU_SURFACE_DESCRIPTOR_INIT;
	sdesc.nextInChain = &src.chain;
	g.surface = wgpuInstanceCreateSurface(g.instance, &sdesc);
#else
	SDL_PropertiesID props = SDL_GetWindowProperties(window);
	void* wl_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
	void* wl_surface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
	WGPUSurfaceDescriptor sdesc = WGPU_SURFACE_DESCRIPTOR_INIT;
	WGPUSurfaceSourceWaylandSurface wl = WGPU_SURFACE_SOURCE_WAYLAND_SURFACE_INIT;
	WGPUSurfaceSourceXlibWindow xl = WGPU_SURFACE_SOURCE_XLIB_WINDOW_INIT;
	if (wl_display && wl_surface) {
		wl.display = wl_display;
		wl.surface = wl_surface;
		sdesc.nextInChain = &wl.chain;
	} else {
		xl.display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
		xl.window = (uint64_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
		sdesc.nextInChain = &xl.chain;
	}
	g.surface = wgpuInstanceCreateSurface(g.instance, &sdesc);
#endif
	if (!g.surface) {
		// Offscreen canvases and readbacks still work; nothing reaches the window.
		fprintf(stderr, "CF: WebGPU could not create a surface for the window; nothing will be presented.\n");
		g.surface_format = WGPUTextureFormat_BGRA8Unorm;
		return;
	}

	WGPUSurfaceCapabilities caps = WGPU_SURFACE_CAPABILITIES_INIT;
	wgpuSurfaceGetCapabilities(g.surface, g.adapter, &caps);
	g.surface_format = caps.formatCount ? caps.formats[0] : WGPUTextureFormat_BGRA8Unorm;
	// CF composes in non-sRGB (SDL_GPU's SDR swapchain), so prefer a UNORM format.
	for (size_t i = 0; i < caps.formatCount; ++i) {
		if (caps.formats[i] == WGPUTextureFormat_BGRA8Unorm || caps.formats[i] == WGPUTextureFormat_RGBA8Unorm) {
			g.surface_format = caps.formats[i];
			break;
		}
	}
	g.supported_present_mode_count = 0;
	for (size_t i = 0; i < caps.presentModeCount && g.supported_present_mode_count < 8; ++i) {
		g.supported_present_modes[g.supported_present_mode_count++] = caps.presentModes[i];
	}
	wgpuSurfaceCapabilitiesFreeMembers(caps);

	g.present_mode = s_present_mode_supported(WGPUPresentMode_Immediate) ? WGPUPresentMode_Immediate : WGPUPresentMode_Fifo;
	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(window, &w, &h);
	s_configure_surface(w, h);
}

bool cf_webgpu_set_present_mode(CF_PresentMode mode)
{
	WGPUPresentMode m;
	switch (mode) {
	case CF_PRESENT_MODE_IMMEDIATE: m = WGPUPresentMode_Immediate; break;
	case CF_PRESENT_MODE_VSYNC:     m = WGPUPresentMode_Fifo; break;
	case CF_PRESENT_MODE_MAILBOX:   m = WGPUPresentMode_Mailbox; break;
	default: return false;
	}
	if (g.supported_present_mode_count && !s_present_mode_supported(m)) return false;
	g.present_mode = m;
	if (g.surface_configured) s_configure_surface(g.surface_w, g.surface_h);
	return true;
}

// Core WebGPU multisamples at 4x only, for these formats. 2x and 8x exist only through the
// adapter-specific feature, which reports nothing per format, so a tiny texture probes them.
static bool s_format_samples(WGPUTextureFormat f, int samples)
{
	if (samples == 1) return f != WGPUTextureFormat_Undefined;
	CF_WFormatInfo fi = s_format_info(f);
	bool multisample = fi.depth || (fi.renderable && !fi.float32 && !fi.sint && !fi.uint);
	if (!multisample) return false;
	if (samples == 4) return true;
	if ((samples != 2 && samples != 8) || !g.adapter_formats) return false;
#ifdef CF_EMSCRIPTEN
	return false;
#else
	for (int i = 0; i < g.msaa_probe_count; ++i) {
		if (g.msaa_probes[i].format == f && g.msaa_probes[i].samples == samples) return g.msaa_probes[i].ok;
	}
	WGPUTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
	desc.usage = WGPUTextureUsage_RenderAttachment;
	desc.size = { 4, 4, 1 };
	desc.format = f;
	desc.sampleCount = (uint32_t)samples;
	wgpuDevicePushErrorScope(g.device, WGPUErrorFilter_Validation);
	WGPUTexture tex = wgpuDeviceCreateTexture(g.device, &desc);
	bool ok = s_pop_validation_scope(NULL) && tex;
	if (tex) wgpuTextureRelease(tex);
	if (g.msaa_probe_count < (int)(sizeof(g.msaa_probes) / sizeof(g.msaa_probes[0]))) {
		int i = g.msaa_probe_count++;
		g.msaa_probes[i].format = f;
		g.msaa_probes[i].samples = samples;
		g.msaa_probes[i].ok = ok;
	}
	return ok;
#endif
}

// The app canvas: default color and depth formats.
bool cf_webgpu_supports_msaa(int sample_count)
{
	CF_CanvasParams p = cf_canvas_defaults(1, 1);
	return s_format_samples(s_wrap(p.target.pixel_format), sample_count) && s_format_samples(s_wrap(p.depth_stencil_target.pixel_format), sample_count);
}

void cf_webgpu_flush()
{
	s_submit();
}

static void s_release_swapchain()
{
	if (g.swapchain_view) { wgpuTextureViewRelease(g.swapchain_view); g.swapchain_view = NULL; }
	if (g.swapchain_tex) { wgpuTextureRelease(g.swapchain_tex); g.swapchain_tex = NULL; }
}

void cf_webgpu_begin_frame()
{
	// A frame still open here means cf_app_update ran twice without drawing: keep its work.
	if (g.encoder) {
		s_submit();
		s_release_swapchain();
	}
	g.canvas = NULL;
	g.skip_drawing = false;
}

static CF_WBlitPipeline* s_blit_pipeline(WGPUTextureFormat format);

static void s_acquire_swapchain()
{
	if (g.swapchain_tex || g.skip_drawing || !g.surface || g.device_lost) return;
	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(g.window, &w, &h);
	if (w <= 0 || h <= 0) { g.skip_drawing = true; return; }
	if (!g.surface_configured || w != g.surface_w || h != g.surface_h) s_configure_surface(w, h);
	WGPUSurfaceTexture st = WGPU_SURFACE_TEXTURE_INIT;
	wgpuSurfaceGetCurrentTexture(g.surface, &st);
	if (st.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal && st.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
		if (st.texture) wgpuTextureRelease(st.texture);
		if (st.status == WGPUSurfaceGetCurrentTextureStatus_Outdated || st.status == WGPUSurfaceGetCurrentTextureStatus_Lost) {
			s_configure_surface(w, h);
		}
		g.skip_drawing = true;
		return;
	}
	g.swapchain_tex = st.texture;
	WGPUTextureViewDescriptor vdesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
	g.swapchain_view = wgpuTextureCreateView(st.texture, &vdesc);
}

static void s_blit(WGPUTextureView src_view, WGPUTextureView dst_view, WGPUTextureFormat dst_format, WGPUFilterMode filter, bool clear)
{
	CF_WBlitPipeline* bp = s_blit_pipeline(dst_format);
	WGPUSamplerDescriptor sd = s_sampler_desc_defaults();
	sd.addressModeU = sd.addressModeV = sd.addressModeW = WGPUAddressMode_ClampToEdge;
	sd.minFilter = sd.magFilter = filter;
	sd.mipmapFilter = WGPUMipmapFilterMode_Nearest;
	WGPUBindGroupEntry e[2] = { WGPU_BIND_GROUP_ENTRY_INIT, WGPU_BIND_GROUP_ENTRY_INIT };
	e[0].binding = 0;
	e[0].textureView = src_view;
	e[1].binding = 1;
	e[1].sampler = s_pinned_sampler(sd)->sampler;
	WGPUBindGroupDescriptor bgd = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
	bgd.layout = bp->bgl;
	bgd.entryCount = 2;
	bgd.entries = e;
	WGPUBindGroup bg = wgpuDeviceCreateBindGroup(g.device, &bgd);

	WGPURenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
	ca.view = dst_view;
	ca.loadOp = clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
	ca.storeOp = WGPUStoreOp_Store;
	ca.clearValue = { 0, 0, 0, 1 };
	WGPURenderPassDescriptor rp = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
	rp.colorAttachmentCount = 1;
	rp.colorAttachments = &ca;
	WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(s_encoder(), &rp);
	wgpuRenderPassEncoderSetPipeline(pass, bp->pip);
	wgpuRenderPassEncoderSetBindGroup(pass, 0, bg, 0, NULL);
	wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
	wgpuRenderPassEncoderEnd(pass);
	wgpuRenderPassEncoderRelease(pass);
	wgpuBindGroupRelease(bg);
}

void cf_webgpu_blit_canvas(CF_Canvas canvas)
{
	s_end_active_pass();
	s_acquire_swapchain();
	if (!g.swapchain_view) return;
	CF_CanvasInternal* c = (CF_CanvasInternal*)canvas.id;
	CF_TextureInternal* src = (CF_TextureInternal*)(c->cf_resolve_texture.id ? c->cf_resolve_texture.id : c->cf_texture.id);
	s_blit(src->view, g.swapchain_view, g.surface_format, s_wrap(app->canvas_blit_filter), true);
}

void cf_webgpu_end_frame()
{
	s_submit();
#ifdef CF_EMSCRIPTEN
	// The browser presents once control returns to it. A polling-loop app (no main callbacks)
	// never returns, so yield here the way SDL_GL_SwapWindow does for WebGL.
	if (!app->using_main_callbacks) emscripten_sleep(0);
#else
	if (g.swapchain_tex) wgpuSurfacePresent(g.surface);
#endif
	s_release_swapchain();
	g.canvas = NULL;
}

//--------------------------------------------------------------------------------------------------
// Internal shaders.

static const char* s_blit_wgsl = R"(
struct VO { @builtin(position) pos: vec4f, @location(0) uv: vec2f };
@vertex fn vs(@builtin(vertex_index) i: u32) -> VO {
	var o: VO;
	let p = vec2f(f32((i << 1u) & 2u), f32(i & 2u));
	o.pos = vec4f(p.x * 2.0 - 1.0, 1.0 - p.y * 2.0, 0.0, 1.0);
	o.uv = p;
	return o;
}
@group(0) @binding(0) var t: texture_2d<f32>;
@group(0) @binding(1) var s: sampler;
@fragment fn fs(v: VO) -> @location(0) vec4f { return textureSampleLevel(t, s, v.uv, 0.0); }
)";

static WGPUShaderModule s_make_module(const char* wgsl, const char* label)
{
	WGPUShaderSourceWGSL src = WGPU_SHADER_SOURCE_WGSL_INIT;
	src.code = s_sv(wgsl);
	WGPUShaderModuleDescriptor desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
	desc.nextInChain = &src.chain;
	desc.label = s_sv(label);
	return wgpuDeviceCreateShaderModule(g.device, &desc);
}

static CF_WBlitPipeline* s_blit_pipeline(WGPUTextureFormat format)
{
	for (int i = 0; i < g.blit_pipelines.count(); ++i) {
		if (g.blit_pipelines[i].format == format) return &g.blit_pipelines[i];
	}
	if (!g.blit_module) g.blit_module = s_make_module(s_blit_wgsl, "cf_blit");
	WGPUBindGroupLayoutEntry e[2] = { WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT, WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT };
	e[0].binding = 0;
	e[0].visibility = WGPUShaderStage_Fragment;
	e[0].texture.sampleType = WGPUTextureSampleType_Float;
	e[0].texture.viewDimension = WGPUTextureViewDimension_2D;
	e[1].binding = 1;
	e[1].visibility = WGPUShaderStage_Fragment;
	e[1].sampler.type = WGPUSamplerBindingType_Filtering;
	WGPUBindGroupLayoutDescriptor bd = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
	bd.entryCount = 2;
	bd.entries = e;
	CF_WBlitPipeline bp = { };
	bp.format = format;
	bp.bgl = wgpuDeviceCreateBindGroupLayout(g.device, &bd);
	WGPUPipelineLayoutDescriptor pld = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
	pld.bindGroupLayoutCount = 1;
	pld.bindGroupLayouts = &bp.bgl;
	WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(g.device, &pld);
	WGPUColorTargetState ct = WGPU_COLOR_TARGET_STATE_INIT;
	ct.format = format;
	ct.writeMask = WGPUColorWriteMask_All;
	WGPUFragmentState fs = WGPU_FRAGMENT_STATE_INIT;
	fs.module = g.blit_module;
	fs.entryPoint = s_sv("fs");
	fs.targetCount = 1;
	fs.targets = &ct;
	WGPURenderPipelineDescriptor pd = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
	pd.layout = layout;
	pd.vertex.module = g.blit_module;
	pd.vertex.entryPoint = s_sv("vs");
	pd.primitive.topology = WGPUPrimitiveTopology_TriangleList;
	pd.multisample.count = 1;
	pd.fragment = &fs;
	bp.pip = wgpuDeviceCreateRenderPipeline(g.device, &pd);
	wgpuPipelineLayoutRelease(layout);
	g.blit_pipelines.add(bp);
	return &g.blit_pipelines.last();
}

//--------------------------------------------------------------------------------------------------
// Textures.

static WGPUTextureViewDimension s_view_dim(CF_TextureType type)
{
	switch (type) {
	case CF_TEXTURE_TYPE_CUBE:     return WGPUTextureViewDimension_Cube;
	case CF_TEXTURE_TYPE_3D:       return WGPUTextureViewDimension_3D;
	case CF_TEXTURE_TYPE_2D_ARRAY: return WGPUTextureViewDimension_2DArray;
	default:                       return WGPUTextureViewDimension_2D;
	}
}

// Steps the count down until the format supports it, as the SDL_GPU backend does.
static int s_supported_sample_count(WGPUTextureFormat format, int samples)
{
	while (samples > 1 && !s_format_samples(format, samples)) samples >>= 1;
	return samples < 1 ? 1 : samples;
}

static int s_samples(CF_SampleCount sc)
{
	switch (sc) {
	case CF_SAMPLE_COUNT_2: return 2;
	case CF_SAMPLE_COUNT_4: return 4;
	case CF_SAMPLE_COUNT_8: return 8;
	default: return 1;
	}
}

bool cf_webgpu_texture_supports_format(CF_PixelFormat format, CF_TextureUsageBits usage)
{
	WGPUTextureFormat f = s_wrap(format);
	if (f == WGPUTextureFormat_Undefined) return false;
	CF_WFormatInfo fi = s_format_info(f);
	if ((usage & CF_TEXTURE_USAGE_COLOR_TARGET_BIT) && (!fi.renderable || fi.depth)) return false;
	if ((usage & CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT) && !fi.depth) return false;
	if ((usage & CF_WGPU_STORAGE_USAGE) && !s_format_storage(f)) return false;
	return true;
}

bool cf_webgpu_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op)
{
	WGPUTextureFormat f = s_wrap(format);
	if (f == WGPUTextureFormat_Undefined) return false;
	CF_WFormatInfo fi = s_format_info(f);
	switch (op) {
	case CF_PIXELFORMAT_OP_NEAREST_FILTER:  return !fi.depth;
	case CF_PIXELFORMAT_OP_BILINEAR_FILTER: return s_format_filterable(f);
	case CF_PIXELFORMAT_OP_RENDER_TARGET:   return fi.renderable && !fi.depth;
	case CF_PIXELFORMAT_OP_ALPHA_BLENDING:  return fi.renderable && !fi.depth && s_format_blendable(f) && cf_pixel_format_has_alpha(format);
	case CF_PIXELFORMAT_OP_MSAA:            return s_format_samples(f, 4);
	case CF_PIXELFORMAT_OP_DEPTH:           return fi.depth;
	default:                                return false;
	}
}

static CF_Texture s_make_texture(CF_TextureParams params, CF_SampleCount sample_count)
{
	WGPUTextureFormat format = s_wrap(params.pixel_format);
	if (format == WGPUTextureFormat_Undefined) {
		fprintf(stderr, "WebGPU: pixel format %d has no WebGPU equivalent on this device.\n", (int)params.pixel_format);
		return { 0 };
	}
	CF_WFormatInfo fi = s_format_info(format);
	int samples = s_supported_sample_count(format, s_samples(sample_count));
	int layers = 1;
	WGPUTextureDimension dimension = WGPUTextureDimension_2D;
	switch (params.texture_type) {
	case CF_TEXTURE_TYPE_CUBE:     layers = 6; break;
	case CF_TEXTURE_TYPE_3D:       layers = cf_max(params.layer_count, 1); dimension = WGPUTextureDimension_3D; break;
	case CF_TEXTURE_TYPE_2D_ARRAY: layers = cf_max(params.layer_count, 1); break;
	default: break;
	}
	int mips = 1;
	if (params.allocate_mipmaps && samples == 1) {
		mips = params.mip_count > 0 ? params.mip_count : (1 + (int)CF_FLOORF(CF_LOG2F((float)cf_max(params.width, params.height))));
	}

	if ((params.usage & CF_WGPU_STORAGE_USAGE) && !s_format_storage(format)) {
		fprintf(stderr, "WebGPU: pixel format %d cannot be a storage texture on this device (see cf_texture_supports_format).\n", (int)params.pixel_format);
		return { 0 };
	}
	WGPUTextureUsage usage = WGPUTextureUsage_None;
	if (samples == 1) {
		usage |= WGPUTextureUsage_TextureBinding;
		if (!fi.depth || format == WGPUTextureFormat_Depth32Float || format == WGPUTextureFormat_Depth16Unorm) {
			usage |= WGPUTextureUsage_CopySrc | WGPUTextureUsage_CopyDst;
		}
		if (params.usage & CF_WGPU_STORAGE_USAGE) usage |= WGPUTextureUsage_StorageBinding;
	}
	if (params.usage & (CF_TEXTURE_USAGE_COLOR_TARGET_BIT | CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT)) {
		if (fi.renderable || fi.depth) usage |= WGPUTextureUsage_RenderAttachment;
	}
	// Mip generation renders each level from the one above.
	if (mips > 1 && fi.renderable && !fi.depth && !fi.sint && !fi.uint && dimension == WGPUTextureDimension_2D) {
		usage |= WGPUTextureUsage_RenderAttachment;
	}

	WGPUTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
	desc.usage = usage;
	desc.dimension = dimension;
	desc.size.width = (uint32_t)params.width;
	desc.size.height = (uint32_t)params.height;
	desc.size.depthOrArrayLayers = (uint32_t)layers;
	desc.format = format;
	desc.mipLevelCount = (uint32_t)mips;
	desc.sampleCount = (uint32_t)samples;
	WGPUTexture tex = wgpuDeviceCreateTexture(g.device, &desc);
	if (!tex) return { 0 };

	CF_TextureInternal* t = (CF_TextureInternal*)CF_CALLOC(sizeof(CF_TextureInternal));
	t->w = params.width;
	t->h = params.height;
	t->layers = layers;
	t->mip_count = mips;
	t->sample_count = samples;
	t->type = params.texture_type;
	t->pixel_format = params.pixel_format;
	t->format = format;
	t->usage = usage;
	t->tex = tex;
	if (samples == 1) {
		WGPUTextureViewDescriptor vd = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
		vd.dimension = s_view_dim(params.texture_type);
		if (fi.depth && fi.stencil) vd.aspect = WGPUTextureAspect_DepthOnly;
		vd.usage = WGPUTextureUsage_TextureBinding;
		t->view = wgpuTextureCreateView(tex, &vd);
	}

	if (!fi.depth || params.compare_enable || (params.usage & CF_TEXTURE_USAGE_SAMPLER_BIT)) {
		WGPUSamplerDescriptor sd = s_sampler_desc_defaults();
		sd.minFilter = sd.magFilter = s_wrap(params.filter);
		sd.mipmapFilter = s_wrap(params.mip_filter);
		sd.addressModeU = s_wrap(params.wrap_u);
		sd.addressModeV = s_wrap(params.wrap_v);
		sd.maxAnisotropy = (uint16_t)cf_clamp((int)params.max_anisotropy, 1, 16);
		if (params.compare_enable) sd.compare = s_wrap(params.compare_function);
		if (params.mip_lod_bias != 0) CF_WGPU_WARN_ONCE("WebGPU: samplers have no LOD bias; mip_lod_bias is ignored.\n");
		t->sampler = s_acquire_sampler(sd);
	}
	t->binding.view = t->view;
	t->binding.sampler = t->sampler ? t->sampler->sampler : NULL;
	CF_Texture result;
	result.id = (uint64_t)(uintptr_t)t;
	return result;
}

CF_Texture cf_webgpu_make_texture(CF_TextureParams params)
{
	return s_make_texture(params, CF_SAMPLE_COUNT_1);
}

void cf_webgpu_destroy_texture(CF_Texture texture_handle)
{
	CF_TextureInternal* t = (CF_TextureInternal*)texture_handle.id;
	if (!t) return;
	if (t->view) wgpuTextureViewRelease(t->view);
	if (t->storage_view) wgpuTextureViewRelease(t->storage_view);
	if (t->split_scratch_view) wgpuTextureViewRelease(t->split_scratch_view);
	if (t->split_scratch) wgpuTextureRelease(t->split_scratch);
	wgpuTextureRelease(t->tex);
	s_release_sampler(t->sampler);
	s_release_sampler(t->draw_samplers[0]);
	s_release_sampler(t->draw_samplers[1]);
	CF_FREE(t);
	g.bind_epoch++;
}

static void s_upload_texture(CF_TextureInternal* t, const void* data, int size, int x, int y, int z, int w, int h, int mip)
{
	if (!t || !data || g.device_lost) return;
	s_end_active_pass();
	CF_WFormatInfo fi = s_format_info(t->format);
	int bw = (w + fi.block_dim - 1) / fi.block_dim;
	int bh = (h + fi.block_dim - 1) / fi.block_dim;
	int row = bw * fi.block_bytes;
	int pitch = s_align(row, 256);
	if (row * bh > size) bh = size / cf_max(row, 1);
	if (bh <= 0) return;
	WGPUBuffer src;
	uint64_t offset;
	uint8_t* p = s_stage(pitch * bh, &src, &offset);
	for (int r = 0; r < bh; ++r) {
		CF_MEMCPY(p + (size_t)r * pitch, (const uint8_t*)data + (size_t)r * row, row);
	}
	if (!src) return;
	WGPUTexelCopyBufferInfo bi = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
	bi.buffer = src;
	bi.layout.offset = offset;
	bi.layout.bytesPerRow = (uint32_t)pitch;
	bi.layout.rowsPerImage = (uint32_t)bh;
	WGPUTexelCopyTextureInfo ti = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
	ti.texture = t->tex;
	ti.mipLevel = (uint32_t)mip;
	ti.origin = { (uint32_t)x, (uint32_t)y, (uint32_t)z };
	WGPUExtent3D ext = { (uint32_t)(bw * fi.block_dim), (uint32_t)(bh * fi.block_dim), 1 };
	// Compressed copies must cover whole blocks but may not run past the mip's edge.
	int mw = cf_max(t->w >> mip, 1), mh = cf_max(t->h >> mip, 1);
	if (fi.block_dim > 1) {
		if ((int)ext.width > mw - x && s_align(mw, 4) - x < (int)ext.width) ext.width = (uint32_t)(s_align(mw, 4) - x);
		if ((int)ext.height > mh - y && s_align(mh, 4) - y < (int)ext.height) ext.height = (uint32_t)(s_align(mh, 4) - y);
	}
	wgpuCommandEncoderCopyBufferToTexture(s_encoder(), &bi, &ti, &ext);
}

void cf_webgpu_texture_update(CF_Texture texture_handle, void* data, int size)
{
	CF_TextureInternal* t = (CF_TextureInternal*)texture_handle.id;
	s_upload_texture(t, data, size, 0, 0, 0, t->w, t->h, 0);
}

void cf_webgpu_texture_update_layer_mip(CF_Texture texture_handle, void* data, int size, int layer, int mip_level)
{
	CF_TextureInternal* t = (CF_TextureInternal*)texture_handle.id;
	int layers_at_mip = t && t->type == CF_TEXTURE_TYPE_3D ? cf_max(t->layers >> mip_level, 1) : (t ? t->layers : 0);
	if (!t || layer < 0 || layer >= layers_at_mip || mip_level >= t->mip_count) return;
	s_upload_texture(t, data, size, 0, 0, layer, cf_max(t->w >> mip_level, 1), cf_max(t->h >> mip_level, 1), mip_level);
}

void cf_webgpu_texture_update_layer(CF_Texture texture_handle, void* data, int size, int layer)
{
	cf_webgpu_texture_update_layer_mip(texture_handle, data, size, layer, 0);
}

void cf_webgpu_texture_update_mip(CF_Texture texture_handle, void* data, int size, int mip_level)
{
	CF_TextureInternal* t = (CF_TextureInternal*)texture_handle.id;
	if (!t || mip_level >= t->mip_count) return;
	s_upload_texture(t, data, size, 0, 0, 0, cf_max(t->w >> mip_level, 1), cf_max(t->h >> mip_level, 1), mip_level);
}

void cf_webgpu_texture_update_region(CF_Texture texture_handle, int x, int y, int w, int h, void* pixels)
{
	CF_TextureInternal* t = (CF_TextureInternal*)texture_handle.id;
	CF_WFormatInfo fi = s_format_info(t->format);
	s_upload_texture(t, pixels, w * h * fi.block_bytes, x, y, 0, w, h, 0);
}

void cf_webgpu_texture_copy_region(CF_Texture dst_handle, int dst_x, int dst_y, CF_Texture src_handle, int src_x, int src_y, int w, int h)
{
	s_end_active_pass();
	CF_TextureInternal* dst = (CF_TextureInternal*)dst_handle.id;
	CF_TextureInternal* src = (CF_TextureInternal*)src_handle.id;
	WGPUTexelCopyTextureInfo s = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
	s.texture = src->tex;
	s.origin = { (uint32_t)src_x, (uint32_t)src_y, 0 };
	WGPUTexelCopyTextureInfo d = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
	d.texture = dst->tex;
	d.origin = { (uint32_t)dst_x, (uint32_t)dst_y, 0 };
	WGPUExtent3D ext = { (uint32_t)w, (uint32_t)h, 1 };
	wgpuCommandEncoderCopyTextureToTexture(s_encoder(), &s, &d, &ext);
}

void cf_webgpu_generate_mipmaps(CF_Texture texture_handle)
{
	CF_TextureInternal* t = (CF_TextureInternal*)texture_handle.id;
	if (!t || t->mip_count <= 1) return;
	if (t->type == CF_TEXTURE_TYPE_3D) {
		CF_WGPU_WARN_ONCE("WebGPU: cf_generate_mipmaps does not support 3D textures; upload their mips with cf_texture_update_layer_mip.\n");
		return;
	}
	if (!(t->usage & WGPUTextureUsage_RenderAttachment)) return;
	s_end_active_pass();
	WGPUFilterMode filter = s_format_filterable(t->format) ? WGPUFilterMode_Linear : WGPUFilterMode_Nearest;
	for (int layer = 0; layer < t->layers; ++layer) {
		for (int mip = 1; mip < t->mip_count; ++mip) {
			WGPUTextureViewDescriptor vd = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
			vd.dimension = WGPUTextureViewDimension_2D;
			vd.baseArrayLayer = (uint32_t)layer;
			vd.arrayLayerCount = 1;
			vd.baseMipLevel = (uint32_t)(mip - 1);
			vd.mipLevelCount = 1;
			vd.usage = WGPUTextureUsage_TextureBinding;
			WGPUTextureView src = wgpuTextureCreateView(t->tex, &vd);
			vd.baseMipLevel = (uint32_t)mip;
			vd.usage = WGPUTextureUsage_RenderAttachment;
			WGPUTextureView dst = wgpuTextureCreateView(t->tex, &vd);
			s_blit(src, dst, t->format, filter, true);
			wgpuTextureViewRelease(src);
			wgpuTextureViewRelease(dst);
		}
	}
}

uint64_t cf_webgpu_texture_handle(CF_Texture texture)
{
	return (uint64_t)(uintptr_t)((CF_TextureInternal*)texture.id)->view;
}

uint64_t cf_webgpu_texture_binding_handle(CF_Texture texture)
{
	return (uint64_t)(uintptr_t)&((CF_TextureInternal*)texture.id)->binding;
}

CF_Sampler cf_webgpu_make_sampler(CF_SamplerParams params)
{
	WGPUSamplerDescriptor sd = s_sampler_desc_defaults();
	sd.minFilter = sd.magFilter = s_wrap(params.filter);
	sd.mipmapFilter = s_wrap(params.mip_filter);
	sd.addressModeU = s_wrap(params.wrap_u);
	sd.addressModeV = s_wrap(params.wrap_v);
	sd.addressModeW = s_wrap(params.wrap_w);
	sd.maxAnisotropy = (uint16_t)cf_clamp(params.max_anisotropy, 1, 16);
	sd.lodMinClamp = params.min_lod;
	sd.lodMaxClamp = params.max_lod >= params.min_lod ? params.max_lod : 1000.0f;
	if (params.compare_enable) sd.compare = s_wrap(params.compare_function);
	if (params.mip_lod_bias != 0) CF_WGPU_WARN_ONCE("WebGPU: samplers have no LOD bias; mip_lod_bias is ignored.\n");
	CF_Sampler result;
	result.id = (uint64_t)(uintptr_t)s_acquire_sampler(sd);
	return result;
}

void cf_webgpu_destroy_sampler(CF_Sampler sampler)
{
	s_release_sampler((CF_WSampler*)sampler.id);
}

static WGPUTextureView s_storage_view(CF_TextureInternal* t)
{
	if (!t->storage_view) {
		WGPUTextureViewDescriptor vd = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
		vd.dimension = t->type == CF_TEXTURE_TYPE_3D ? WGPUTextureViewDimension_3D : (t->layers > 1 ? WGPUTextureViewDimension_2DArray : WGPUTextureViewDimension_2D);
		vd.mipLevelCount = 1;
		vd.usage = WGPUTextureUsage_StorageBinding;
		t->storage_view = wgpuTextureCreateView(t->tex, &vd);
	}
	return t->storage_view;
}

//--------------------------------------------------------------------------------------------------
// Canvases.

static WGPUTextureView s_target_view(CF_TextureInternal* t, int layer, int mip)
{
	WGPUTextureViewDescriptor vd = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
	vd.dimension = WGPUTextureViewDimension_2D;
	vd.baseArrayLayer = (uint32_t)layer;
	vd.arrayLayerCount = 1;
	vd.baseMipLevel = (uint32_t)mip;
	vd.mipLevelCount = 1;
	vd.usage = WGPUTextureUsage_RenderAttachment;
	return wgpuTextureCreateView(t->tex, &vd);
}

CF_Canvas cf_webgpu_make_canvas(CF_CanvasParams params)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)CF_CALLOC(sizeof(CF_CanvasInternal));
	canvas->sample_count = CF_SAMPLE_COUNT_1;
	canvas->samples = 1;
	if (params.attach_target.id) {
		CF_TextureInternal* attach = (CF_TextureInternal*)params.attach_target.id;
		canvas->attached = true;
		canvas->attach_layer = params.attach_layer;
		canvas->attach_mip = params.attach_mip;
		canvas->w = cf_max(attach->w >> params.attach_mip, 1);
		canvas->h = cf_max(attach->h >> params.attach_mip, 1);
		if (s_format_info(attach->format).depth) {
			canvas->attached_depth = true;
			canvas->cf_depth_stencil = params.attach_target;
			canvas->depth_view = s_target_view(attach, params.attach_layer, params.attach_mip);
			canvas->depth_has_stencil = s_format_info(attach->format).stencil;
		} else {
			canvas->cf_texture = params.attach_target;
			canvas->color_views[0] = s_target_view(attach, params.attach_layer, params.attach_mip);
			if (params.depth_stencil_enable) {
				CF_TextureParams dsp = params.depth_stencil_target;
				dsp.width = canvas->w;
				dsp.height = canvas->h;
				canvas->cf_depth_stencil = s_make_texture(dsp, CF_SAMPLE_COUNT_1);
				if (canvas->cf_depth_stencil.id) {
					CF_TextureInternal* d = (CF_TextureInternal*)canvas->cf_depth_stencil.id;
					canvas->depth_view = s_target_view(d, 0, 0);
					canvas->depth_has_stencil = s_format_info(d->format).stencil;
				}
			}
		}
		CF_Canvas result;
		result.id = (uint64_t)(uintptr_t)canvas;
		return result;
	}
	if (params.target.width <= 0 || params.target.height <= 0) {
		CF_FREE(canvas);
		return { 0 };
	}
	canvas->w = params.target.width;
	canvas->h = params.target.height;
	int samples = s_supported_sample_count(s_wrap(params.target.pixel_format), s_samples(params.sample_count));
	if (samples > 1) {
		// Formats that cannot multisample keep the canvas single-sampled.
		int tc = params.target_count > 1 ? params.target_count : 1;
		for (int t = 0; t < tc; ++t) {
			CF_PixelFormat pf = t == 0 ? params.target.pixel_format : params.targets[t].pixel_format;
			if (!cf_webgpu_query_pixel_format(pf, CF_PIXELFORMAT_OP_MSAA)) samples = 1;
		}
		if (params.depth_stencil_enable && s_wrap(params.depth_stencil_target.pixel_format) == WGPUTextureFormat_Undefined) {
			params.depth_stencil_target.pixel_format = CF_PIXEL_FORMAT_D24_UNORM_S8_UINT;
		}
	}
	canvas->samples = samples;
	canvas->sample_count = samples == 8 ? CF_SAMPLE_COUNT_8 : samples == 4 ? CF_SAMPLE_COUNT_4 : samples == 2 ? CF_SAMPLE_COUNT_2 : CF_SAMPLE_COUNT_1;
	canvas->target_count = params.target_count > 1 ? cf_min(params.target_count, CF_MAX_CANVAS_TARGETS) : 1;
	for (int i = 0; i < canvas->target_count; ++i) {
		CF_TextureParams tp = i == 0 ? params.target : params.targets[i];
		tp.width = params.target.width;
		tp.height = params.target.height;
		CF_Texture t = s_make_texture(tp, canvas->sample_count);
		if (i == 0) canvas->cf_texture = t; else canvas->cf_textures_mrt[i] = t;
		if (t.id) canvas->color_views[i] = s_target_view((CF_TextureInternal*)t.id, 0, 0);
		if (samples > 1) {
			tp.usage = CF_TEXTURE_USAGE_COLOR_TARGET_BIT | CF_TEXTURE_USAGE_SAMPLER_BIT;
			tp.allocate_mipmaps = false;
			CF_Texture r = s_make_texture(tp, CF_SAMPLE_COUNT_1);
			canvas->cf_resolve_textures_mrt[i] = r;
			if (i == 0) canvas->cf_resolve_texture = r;
			if (r.id) canvas->resolve_views[i] = s_target_view((CF_TextureInternal*)r.id, 0, 0);
		}
	}
	if (params.depth_stencil_enable) {
		CF_TextureParams dsp = params.depth_stencil_target;
		dsp.width = params.target.width;
		dsp.height = params.target.height;
		canvas->cf_depth_stencil = s_make_texture(dsp, canvas->sample_count);
		if (canvas->cf_depth_stencil.id) {
			CF_TextureInternal* d = (CF_TextureInternal*)canvas->cf_depth_stencil.id;
			canvas->depth_view = s_target_view(d, 0, 0);
			canvas->depth_has_stencil = s_format_info(d->format).stencil;
		}
	}
	CF_Canvas result;
	result.id = (uint64_t)(uintptr_t)canvas;
	return result;
}

void cf_webgpu_destroy_canvas(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* c = (CF_CanvasInternal*)canvas_handle.id;
	if (!c) return;
	if (g.canvas == c) { s_end_active_pass(); g.canvas = NULL; }
	for (int i = 0; i < CF_MAX_CANVAS_TARGETS; ++i) {
		if (c->color_views[i]) wgpuTextureViewRelease(c->color_views[i]);
		if (c->resolve_views[i]) wgpuTextureViewRelease(c->resolve_views[i]);
	}
	if (c->depth_view) wgpuTextureViewRelease(c->depth_view);
	if (!c->attached) cf_webgpu_destroy_texture(c->cf_texture);
	for (int i = 1; i < c->target_count; ++i) if (c->cf_textures_mrt[i].id) cf_webgpu_destroy_texture(c->cf_textures_mrt[i]);
	for (int i = 0; i < c->target_count; ++i) if (c->cf_resolve_textures_mrt[i].id) cf_webgpu_destroy_texture(c->cf_resolve_textures_mrt[i]);
	if (c->cf_depth_stencil.id && !c->attached_depth) cf_webgpu_destroy_texture(c->cf_depth_stencil);
	CF_FREE(c);
}

CF_Texture cf_webgpu_canvas_get_target(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* c = (CF_CanvasInternal*)canvas_handle.id;
	return c->cf_resolve_texture.id ? c->cf_resolve_texture : c->cf_texture;
}

CF_Texture cf_webgpu_canvas_get_target2(CF_Canvas canvas_handle, int index)
{
	CF_CanvasInternal* c = (CF_CanvasInternal*)canvas_handle.id;
	if (!c || index < 0 || index >= (c->target_count > 1 ? c->target_count : 1)) return { 0 };
	if (index == 0) return cf_webgpu_canvas_get_target(canvas_handle);
	return c->cf_resolve_textures_mrt[index].id ? c->cf_resolve_textures_mrt[index] : c->cf_textures_mrt[index];
}

CF_Texture cf_webgpu_canvas_get_depth_stencil_target(CF_Canvas canvas_handle)
{
	return ((CF_CanvasInternal*)canvas_handle.id)->cf_depth_stencil;
}

void cf_webgpu_canvas_get_size(CF_Canvas canvas_handle, int* w, int* h)
{
	CF_CanvasInternal* c = (CF_CanvasInternal*)canvas_handle.id;
	if (c) {
		if (w) *w = c->w;
		if (h) *h = c->h;
	}
}

static void s_begin_pass(CF_CanvasInternal* c, bool clear)
{
	WGPURenderPassColorAttachment ca[CF_MAX_CANVAS_TARGETS];
	int n = s_color_target_count(c);
	for (int i = 0; i < n; ++i) {
		ca[i] = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
		ca[i].view = c->color_views[i];
		ca[i].resolveTarget = c->resolve_views[i];
		ca[i].loadOp = clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
		ca[i].storeOp = WGPUStoreOp_Store;
		CF_Color cc = s_clear_color2(c, i);
		ca[i].clearValue = { cc.r, cc.g, cc.b, cc.a };
	}
	WGPURenderPassDepthStencilAttachment da = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
	if (c->depth_view) {
		da.view = c->depth_view;
		da.depthLoadOp = clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
		da.depthStoreOp = WGPUStoreOp_Store;
		da.depthClearValue = s_clear_depth(c);
		if (c->depth_has_stencil) {
			da.stencilLoadOp = clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
			da.stencilStoreOp = WGPUStoreOp_Store;
			da.stencilClearValue = s_clear_stencil(c);
		}
	}
	WGPURenderPassDescriptor rp = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
	rp.colorAttachmentCount = (size_t)n;
	rp.colorAttachments = ca;
	rp.depthStencilAttachment = c->depth_view ? &da : NULL;
	g.pass = wgpuCommandEncoderBeginRenderPass(s_encoder(), &rp);
}

void cf_webgpu_clear_canvas(CF_Canvas canvas_handle)
{
	s_end_active_pass();
	CF_CanvasInternal* c = (CF_CanvasInternal*)canvas_handle.id;
	s_begin_pass(c, true);
	s_end_active_pass();
	c->clear = false;
}

void cf_webgpu_canvas_set_clear_color(CF_Canvas canvas_handle, CF_Color color)
{
	CF_CanvasInternal* c = (CF_CanvasInternal*)canvas_handle.id;
	if (!c) return;
	for (int i = 0; i < CF_MAX_CANVAS_TARGETS; ++i) {
		c->has_clear_color[i] = true;
		c->clear_color[i] = color;
	}
}

void cf_webgpu_canvas_set_clear_color2(CF_Canvas canvas_handle, int index, CF_Color color)
{
	CF_CanvasInternal* c = (CF_CanvasInternal*)canvas_handle.id;
	if (!c || index < 0 || index >= CF_MAX_CANVAS_TARGETS) return;
	c->has_clear_color[index] = true;
	c->clear_color[index] = color;
}

void cf_webgpu_canvas_set_clear_depth_stencil(CF_Canvas canvas_handle, float depth, uint32_t stencil)
{
	CF_CanvasInternal* c = (CF_CanvasInternal*)canvas_handle.id;
	if (!c) return;
	c->has_clear_depth_stencil = true;
	c->clear_depth = depth;
	c->clear_stencil = stencil;
}

void cf_webgpu_apply_canvas(CF_Canvas canvas_handle, bool clear)
{
	CF_CanvasInternal* c = (CF_CanvasInternal*)canvas_handle.id;
	CF_ASSERT(c);
	if (g.pass && (c != g.canvas || clear)) s_end_active_pass();
	if (c != g.canvas) g.ps.has_viewport = g.ps.has_scissor = false;
	g.canvas = c;
	c->clear = clear;
}

void cf_webgpu_current_canvas_size(int* w, int* h)
{
	CF_ASSERT(g.canvas);
	*w = g.canvas->w;
	*h = g.canvas->h;
}

//--------------------------------------------------------------------------------------------------
// Readbacks.

static void s_on_map(WGPUMapAsyncStatus status, WGPUStringView message, void* ud1, void* ud2)
{
	CF_UNUSED(ud2);
	CF_ReadbackInternal* rb = (CF_ReadbackInternal*)ud1;
	if (status != WGPUMapAsyncStatus_Success) {
		if (!g.device_lost) s_print_sv("WebGPU: readback map failed: ", message);
		rb->failed = true;
	} else {
		rb->mapped = true;
	}
	rb->ready = true;
}

CF_Readback cf_webgpu_canvas_readback2(CF_Canvas canvas_handle, int index)
{
	CF_CanvasInternal* c = (CF_CanvasInternal*)canvas_handle.id;
	if (!c || index < 0 || index >= (c->target_count > 1 ? c->target_count : 1)) return { 0 };
	CF_Texture th = cf_webgpu_canvas_get_target2(canvas_handle, index);
	CF_TextureInternal* t = (CF_TextureInternal*)th.id;
	if (!t || c->attached_depth) return { 0 };
	s_end_active_pass();
	CF_WFormatInfo fi = s_format_info(t->format);
	CF_ReadbackInternal* rb = (CF_ReadbackInternal*)CF_CALLOC(sizeof(CF_ReadbackInternal));
	g.readbacks.add(rb);
	rb->w = c->w;
	rb->h = c->h;
	rb->row_bytes = c->w * fi.block_bytes;
	rb->padded_row = s_align(rb->row_bytes, 256);
	rb->size = rb->row_bytes * c->h;
	if (g.device_lost) {
		// Ready at once with no data, so a loop waiting on cf_readback_ready still ends.
		rb->failed = true;
		rb->ready = true;
		CF_Readback result;
		result.id = (uint64_t)(uintptr_t)rb;
		return result;
	}
	WGPUBufferDescriptor bd = WGPU_BUFFER_DESCRIPTOR_INIT;
	bd.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
	bd.size = (uint64_t)rb->padded_row * (uint64_t)c->h;
	rb->buffer = wgpuDeviceCreateBuffer(g.device, &bd);
	WGPUTexelCopyTextureInfo src = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
	src.texture = t->tex;
	if (c->attached) {
		src.mipLevel = (uint32_t)c->attach_mip;
		src.origin.z = (uint32_t)c->attach_layer;
	}
	WGPUTexelCopyBufferInfo dst = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
	dst.buffer = rb->buffer;
	dst.layout.bytesPerRow = (uint32_t)rb->padded_row;
	dst.layout.rowsPerImage = (uint32_t)c->h;
	WGPUExtent3D ext = { (uint32_t)c->w, (uint32_t)c->h, 1 };
	wgpuCommandEncoderCopyTextureToBuffer(s_encoder(), &src, &dst, &ext);
	// The copy has to be submitted before the buffer can map; work recorded so far rides along.
	s_submit();
	WGPUBufferMapCallbackInfo cb = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
	cb.mode = WGPUCallbackMode_AllowProcessEvents;
	cb.callback = s_on_map;
	cb.userdata1 = rb;
	wgpuBufferMapAsync(rb->buffer, WGPUMapMode_Read, 0, (size_t)bd.size, cb);
	CF_Readback result;
	result.id = (uint64_t)(uintptr_t)rb;
	return result;
}

CF_Readback cf_webgpu_canvas_readback(CF_Canvas canvas_handle)
{
	return cf_webgpu_canvas_readback2(canvas_handle, 0);
}

bool cf_webgpu_readback_ready(CF_Readback readback)
{
	CF_ReadbackInternal* rb = (CF_ReadbackInternal*)readback.id;
	if (!rb) return false;
	if (!rb->ready) s_process_events();
	return rb->ready;
}

int cf_webgpu_readback_data(CF_Readback readback, void* data, int size)
{
	CF_ReadbackInternal* rb = (CF_ReadbackInternal*)readback.id;
	if (!rb || !data || size <= 0 || !rb->ready || !rb->mapped) return 0;
	const uint8_t* p = (const uint8_t*)wgpuBufferGetConstMappedRange(rb->buffer, 0, (size_t)rb->padded_row * rb->h);
	if (!p) return 0;
	int bytes = size < rb->size ? size : rb->size;
	int copied = 0;
	for (int r = 0; r < rb->h && copied < bytes; ++r) {
		int n = cf_min(rb->row_bytes, bytes - copied);
		CF_MEMCPY((uint8_t*)data + copied, p + (size_t)r * rb->padded_row, n);
		copied += n;
	}
	return copied;
}

int cf_webgpu_readback_size(CF_Readback readback)
{
	CF_ReadbackInternal* rb = (CF_ReadbackInternal*)readback.id;
	return rb ? rb->size : 0;
}

// The map callback holds rb, so the map has to land before the free.
static void s_free_readback(CF_ReadbackInternal* rb)
{
	s_wait(&rb->ready);
	if (rb->mapped) wgpuBufferUnmap(rb->buffer);
	if (rb->buffer) wgpuBufferRelease(rb->buffer);
	CF_FREE(rb);
}

void cf_webgpu_destroy_readback(CF_Readback readback)
{
	CF_ReadbackInternal* rb = (CF_ReadbackInternal*)readback.id;
	if (!rb) return;
	for (int i = 0; i < g.readbacks.count(); ++i) {
		if (g.readbacks[i] == rb) { g.readbacks.unordered_remove(i); break; }
	}
	s_free_readback(rb);
}

//--------------------------------------------------------------------------------------------------
// Meshes and buffers.

CF_Mesh cf_webgpu_make_mesh(int vertex_buffer_size, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)CF_CALLOC(sizeof(CF_MeshInternal));
	mesh->vertices.size = vertex_buffer_size;
	if (vertex_buffer_size) mesh->vertices.buffer = s_make_buffer(vertex_buffer_size, WGPUBufferUsage_Vertex);
	attribute_count = cf_min(attribute_count, CF_MESH_MAX_VERTEX_ATTRIBUTES);
	mesh->attribute_count = attribute_count;
	mesh->vertices.stride = vertex_stride;
	for (int i = 0; i < attribute_count; ++i) {
		mesh->attributes[i] = attributes[i];
		mesh->attributes[i].name = sintern(attributes[i].name);
	}
	CF_Mesh result = { (uint64_t)(uintptr_t)mesh };
	return result;
}

void cf_webgpu_mesh_set_index_buffer(CF_Mesh mesh_handle, int index_buffer_size_in_bytes, int index_bit_count)
{
	CF_ASSERT(index_bit_count == 16 || index_bit_count == 32);
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	mesh->indices.size = index_buffer_size_in_bytes;
	mesh->indices.stride = index_bit_count / 8;
	mesh->indices.buffer = s_make_buffer(index_buffer_size_in_bytes, WGPUBufferUsage_Index);
}

void cf_webgpu_mesh_set_instance_buffer(CF_Mesh mesh_handle, int instance_buffer_size_in_bytes, int instance_stride)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	mesh->instances.size = instance_buffer_size_in_bytes;
	mesh->instances.stride = instance_stride;
	mesh->instances.buffer = s_make_buffer(instance_buffer_size_in_bytes, WGPUBufferUsage_Vertex);
}

void cf_webgpu_mesh_append_attributes(CF_Mesh mesh_handle, const CF_VertexAttribute* attributes, int attribute_count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	for (int i = 0; i < attribute_count && mesh->attribute_count < CF_MESH_MAX_VERTEX_ATTRIBUTES; ++i) {
		CF_VertexAttribute attr = attributes[i];
		attr.name = sintern(attr.name);
		mesh->attributes[mesh->attribute_count++] = attr;
	}
}

bool cf_webgpu_mesh_has_vertex_attribute(CF_Mesh mesh_handle, const char* name)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	for (int i = 0; i < mesh->attribute_count; ++i) {
		if (!CF_STRCMP(mesh->attributes[i].name, name)) return true;
	}
	return false;
}

int cf_webgpu_mesh_instance_stride(CF_Mesh mesh_handle) { return ((CF_MeshInternal*)mesh_handle.id)->instances.stride; }
bool cf_webgpu_mesh_draw3d_augmented(CF_Mesh mesh_handle) { return ((CF_MeshInternal*)mesh_handle.id)->draw3d_augmented; }
void cf_webgpu_mesh_set_draw3d_augmented(CF_Mesh mesh_handle) { ((CF_MeshInternal*)mesh_handle.id)->draw3d_augmented = true; }

static void s_update_buffer(CF_WBuffer* buffer, int element_count, const void* data, int size, WGPUBufferUsage usage)
{
	if (size > buffer->size || !buffer->buffer) {
		if (buffer->buffer) wgpuBufferRelease(buffer->buffer);
		buffer->size = cf_max(size * 2, 4);
		buffer->buffer = s_make_buffer(buffer->size, usage);
	}
	s_upload_buffer(buffer->buffer, 0, data, size);
	buffer->element_count = element_count;
}

void cf_webgpu_mesh_update_vertex_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	CF_ASSERT(mesh->attribute_count);
	s_update_buffer(&mesh->vertices, count, data, count * mesh->vertices.stride, WGPUBufferUsage_Vertex);
}

void cf_webgpu_mesh_update_index_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_update_buffer(&mesh->indices, count, data, count * mesh->indices.stride, WGPUBufferUsage_Index);
}

void cf_webgpu_mesh_update_instance_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_update_buffer(&mesh->instances, count, data, count * mesh->instances.stride, WGPUBufferUsage_Vertex);
}

void cf_webgpu_destroy_mesh(CF_Mesh mesh_handle)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	if (!mesh) return;
	if (g.canvas && g.canvas->mesh == mesh) g.canvas->mesh = NULL;
	if (mesh->vertices.buffer) wgpuBufferRelease(mesh->vertices.buffer);
	if (mesh->indices.buffer) wgpuBufferRelease(mesh->indices.buffer);
	if (mesh->instances.buffer) wgpuBufferRelease(mesh->instances.buffer);
	CF_FREE(mesh);
}

void cf_webgpu_apply_mesh(CF_Mesh mesh_handle)
{
	CF_ASSERT(g.canvas);
	g.canvas->mesh = (CF_MeshInternal*)mesh_handle.id;
}

uint64_t cf_webgpu_make_instance_buffer(int size_in_bytes, int stride)
{
	CF_InstanceBufferInternal* b = (CF_InstanceBufferInternal*)CF_CALLOC(sizeof(CF_InstanceBufferInternal));
	b->buf.size = size_in_bytes;
	b->buf.stride = stride;
	b->buf.buffer = s_make_buffer(size_in_bytes, WGPUBufferUsage_Vertex);
	return (uint64_t)(uintptr_t)b;
}

void cf_webgpu_update_instance_buffer(uint64_t handle, void* data, int count)
{
	CF_InstanceBufferInternal* b = (CF_InstanceBufferInternal*)(uintptr_t)handle;
	s_update_buffer(&b->buf, count, data, count * b->buf.stride, WGPUBufferUsage_Vertex);
}

void cf_webgpu_destroy_instance_buffer(uint64_t handle)
{
	CF_InstanceBufferInternal* b = (CF_InstanceBufferInternal*)(uintptr_t)handle;
	if (!b) return;
	if (b->buf.buffer) wgpuBufferRelease(b->buf.buffer);
	CF_FREE(b);
}

void cf_webgpu_apply_instance_buffer_override(uint64_t handle, int count, int offset_bytes)
{
	CF_InstanceBufferInternal* b = (CF_InstanceBufferInternal*)(uintptr_t)handle;
	g.instance_override = b ? b->buf.buffer : NULL;
	g.instance_override_count = b ? count : 0;
	g.instance_override_offset = b ? offset_bytes : 0;
}

// Unbinds a storage buffer about to be released, so no draw binds a dead handle.
static void s_release_storage_buffer(WGPUBuffer buffer)
{
	CF_WPassState& ps = g.ps;
	for (int i = 0; i < ps.vs_storage_count; ++i) if (ps.vs_storage[i] == buffer) ps.vs_storage[i] = NULL;
	for (int i = 0; i < ps.fs_storage_count; ++i) if (ps.fs_storage[i] == buffer) ps.fs_storage[i] = NULL;
	wgpuBufferRelease(buffer);
	g.bind_epoch++;
}

CF_StorageBuffer cf_webgpu_make_storage_buffer(CF_StorageBufferParams params)
{
	CF_StorageBufferInternal* sb = (CF_StorageBufferInternal*)CF_CALLOC(sizeof(CF_StorageBufferInternal));
	sb->size = params.size;
	sb->usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc;
	if (params.indirect_drawable) sb->usage |= WGPUBufferUsage_Indirect;
	sb->buffer = s_make_buffer(params.size, sb->usage);
	CF_StorageBuffer result;
	result.id = (uint64_t)(uintptr_t)sb;
	return result;
}

void cf_webgpu_update_storage_buffer(CF_StorageBuffer buffer, const void* data, int size)
{
	CF_StorageBufferInternal* sb = (CF_StorageBufferInternal*)buffer.id;
	if (size > sb->size) {
		s_release_storage_buffer(sb->buffer);
		sb->size = size * 2;
		sb->buffer = s_make_buffer(sb->size, sb->usage);
	}
	s_upload_buffer(sb->buffer, 0, data, size);
}

void cf_webgpu_destroy_storage_buffer(CF_StorageBuffer buffer)
{
	CF_StorageBufferInternal* sb = (CF_StorageBufferInternal*)buffer.id;
	if (!sb) return;
	s_release_storage_buffer(sb->buffer);
	CF_FREE(sb);
}

void cf_webgpu_apply_fs_storage_buffers(CF_StorageBuffer* buffers, int count)
{
	CF_ASSERT(count <= 8);
	g.ps.fs_storage_count = count;
	for (int i = 0; i < count; ++i) g.ps.fs_storage[i] = ((CF_StorageBufferInternal*)buffers[i].id)->buffer;
}

void cf_webgpu_apply_vs_storage_buffers(CF_StorageBuffer* buffers, int count)
{
	CF_ASSERT(count <= 8);
	g.ps.vs_storage_count = count;
	for (int i = 0; i < count; ++i) g.ps.vs_storage[i] = ((CF_StorageBufferInternal*)buffers[i].id)->buffer;
}

//--------------------------------------------------------------------------------------------------
// Shaders.

static void s_dump_wgsl(const char* src, const char* kind)
{
	const char* dir = getenv("CF_DUMP_WGSL");
	if (!dir || !src) return;
	static int counter = 0;
	char path[512];
	snprintf(path, sizeof(path), "%s/%s_%03d.wgsl", dir, kind, counter++);
	FILE* fp = fopen(path, "wb");
	if (fp) { fwrite(src, 1, strlen(src), fp); fclose(fp); }
}

// Creates a module from emitted WGSL, reporting compile errors with the source attached.
static WGPUShaderModule s_make_user_module(const char* wgsl, const char* kind)
{
	if (!wgsl) {
		fprintf(stderr, "WebGPU: the shader carries no WGSL (was it compiled with skip_wgsl?).\n");
		return NULL;
	}
	s_dump_wgsl(wgsl, kind);
#ifndef CF_EMSCRIPTEN
	wgpuDevicePushErrorScope(g.device, WGPUErrorFilter_Validation);
#endif
	WGPUShaderModule m = s_make_module(wgsl, kind);
#ifndef CF_EMSCRIPTEN
	if (!s_pop_validation_scope("WebGPU shader error: ")) {
		if (!g.device_lost) fprintf(stderr, "---- WGSL (%s) ----\n%s\n----\n", kind, wgsl);
		if (m) wgpuShaderModuleRelease(m);
		return NULL;
	}
#endif
	return m;
}

static void s_load_stage_info(CF_WStageInfo* st, const CF_ShaderInfo* info)
{
	st->sampled_count = info->num_samplers;
	st->storage_texture_count = info->num_storage_textures;
	st->storage_buffer_count = info->num_storage_buffers;
	for (int i = 0; i < info->num_images; ++i) {
		st->image_names.add(sintern(info->image_names[i]));
		st->image_slots.add(info->image_binding_slots ? info->image_binding_slots[i] : i);
	}
	st->uniform_block_count = info->num_uniforms;
	const CF_ShaderUniformMemberInfo* member_infos = info->uniform_members;
	for (int i = 0; i < info->num_uniforms; ++i) {
		const CF_ShaderUniformInfo* block_info = &info->uniforms[i];
		int block_index = block_info->block_index;
		st->block_sizes[block_index] = block_info->block_size;
		const char* block_name = sintern(block_info->block_name);
		for (int j = 0; j < block_info->num_members; ++j) {
			const CF_ShaderUniformMemberInfo* mi = &member_infos[j];
			CF_UniformBlockMember m;
			m.name = sintern(mi->name);
			m.block_name = block_name;
			m.type = s_uniform_type(mi->type);
			m.array_element_count = mi->array_length;
			m.size = s_uniform_size(m.type) * mi->array_length;
			m.offset = mi->offset;
			st->members[block_index].add(m);
		}
		member_infos += block_info->num_members;
	}
}

// Bind group layout for one group of a layout info. unfilterable_mask bit (b/2) marks sampled
// pairs whose texture cannot be filtered.
static WGPUBindGroupLayout s_make_bgl(const CF_WLayoutInfo* info, int group, uint32_t unfilterable_mask, bool uniform_group)
{
	WGPUBindGroupLayoutEntry entries[CF_WGPU_MAX_BINDINGS];
	int n = 0;
	for (int i = 0; i < info->count; ++i) {
		const CF_WBinding& b = info->b[i];
		if (b.group != group) continue;
		WGPUBindGroupLayoutEntry e = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
		e.binding = (uint32_t)b.binding;
		e.visibility = b.visibility;
		bool unfilterable = b.binding / 2 < 32 && (unfilterable_mask >> (b.binding / 2)) & 1;
		switch (b.kind) {
		case CF_WBIND_UNIFORM:
			e.buffer.type = WGPUBufferBindingType_Uniform;
			e.buffer.hasDynamicOffset = uniform_group;
			break;
		case CF_WBIND_STORAGE_RO:
			e.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
			break;
		case CF_WBIND_STORAGE_RW:
			e.buffer.type = WGPUBufferBindingType_Storage;
			break;
		case CF_WBIND_SAMPLER:
			e.sampler.type = unfilterable ? WGPUSamplerBindingType_NonFiltering : WGPUSamplerBindingType_Filtering;
			break;
		case CF_WBIND_SAMPLER_CMP:
			e.sampler.type = WGPUSamplerBindingType_Comparison;
			break;
		case CF_WBIND_TEXTURE:
			e.texture.sampleType = b.sample_type;
			if (b.sample_type == WGPUTextureSampleType_Float && unfilterable) e.texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
			e.texture.viewDimension = b.dim;
			e.texture.multisampled = b.multisampled;
			break;
		case CF_WBIND_STORAGE_TEXTURE:
			e.storageTexture.access = b.access;
			e.storageTexture.format = b.storage_format;
			e.storageTexture.viewDimension = b.dim;
			break;
		}
		entries[n++] = e;
	}
	if (!n) {
		wgpuBindGroupLayoutAddRef(g.empty_bgl);
		return g.empty_bgl;
	}
	WGPUBindGroupLayoutDescriptor d = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
	d.entryCount = (size_t)n;
	d.entries = entries;
	return wgpuDeviceCreateBindGroupLayout(g.device, &d);
}

static CF_WLayoutVariant s_make_variant(const CF_WLayoutInfo* info, int group_count, const bool* uniform_groups, uint32_t mask)
{
	CF_WLayoutVariant v = { };
	v.unfilterable_mask = mask;
	for (int grp = 0; grp < group_count; ++grp) {
		// Graphics masks: bits 0-15 cover group 0, bits 16-31 group 2. Compute uses group 0 only.
		uint32_t gm = grp == 0 ? (mask & 0xFFFF) : grp == 2 ? (mask >> 16) : 0;
		v.bgl[grp] = s_make_bgl(info, grp, gm, uniform_groups[grp]);
	}
	WGPUPipelineLayoutDescriptor pld = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
	pld.bindGroupLayoutCount = (size_t)group_count;
	pld.bindGroupLayouts = v.bgl;
	v.layout = wgpuDeviceCreatePipelineLayout(g.device, &pld);
	return v;
}

static void s_release_variant(CF_WLayoutVariant* v)
{
	for (int i = 0; i < 4; ++i) if (v->bgl[i]) wgpuBindGroupLayoutRelease(v->bgl[i]);
	if (v->layout) wgpuPipelineLayoutRelease(v->layout);
}

CF_Shader cf_webgpu_make_shader_from_bytecode(CF_ShaderBytecode vertex_bytecode, CF_ShaderBytecode fragment_bytecode)
{
	const char* vs_src = s_wgsl_src(vertex_bytecode);
	const char* fs_src = s_wgsl_src(fragment_bytecode);
	WGPUShaderModule vs = s_make_user_module(vs_src, "vs");
	WGPUShaderModule fs = vs ? s_make_user_module(fs_src, "fs") : NULL;
	if (!vs || !fs) {
		if (vs) wgpuShaderModuleRelease(vs);
		return { 0 };
	}
	CF_ShaderInternal* shd = CF_NEW(CF_ShaderInternal);
	CF_MEMSET(shd, 0, sizeof(*shd));
	shd->vs = vs;
	shd->fs = fs;
	s_parse_wgsl_bindings(&shd->layout, vs_src, WGPUShaderStage_Vertex);
	s_parse_wgsl_bindings(&shd->layout, fs_src, WGPUShaderStage_Fragment);
	s_load_stage_info(&shd->stage[0], &vertex_bytecode.shader_info);
	s_load_stage_info(&shd->stage[1], &fragment_bytecode.shader_info);
	const CF_ShaderInfo* vi = &vertex_bytecode.shader_info;
	CF_ASSERT(vi->num_inputs <= CF_MAX_SHADER_INPUTS);
	shd->input_count = vi->num_inputs;
	for (int i = 0; i < vi->num_inputs; ++i) {
		shd->input_names[i] = sintern(vi->inputs[i].name);
		shd->input_locations[i] = vi->inputs[i].location;
		shd->input_formats[i] = s_wrap(vi->inputs[i].format);
	}
	CF_Shader result;
	result.id = (uint64_t)(uintptr_t)shd;
	return result;
}

static void s_release_uniform_group(CF_WUniformGroup* ug)
{
	if (ug->group) wgpuBindGroupRelease(ug->group);
	ug->group = NULL;
	ug->ring_generation = 0;
}

static void s_clear_bind_cache(CF_WBindCache* cache);

void cf_webgpu_destroy_shader_internal(CF_Shader shader_handle)
{
	CF_ShaderInternal* shd = (CF_ShaderInternal*)shader_handle.id;
	if (!shd) return;
	if (g.shader == shd) { s_end_active_pass(); }
	for (int i = 0; i < shd->pip_cache.count(); ++i) wgpuRenderPipelineRelease(shd->pip_cache[i].pip);
	for (int i = 0; i < shd->variants.count(); ++i) s_release_variant(&shd->variants[i]);
	s_release_uniform_group(&shd->uniform_groups[0]);
	s_release_uniform_group(&shd->uniform_groups[1]);
	s_clear_bind_cache(&shd->bind_caches[0]);
	s_clear_bind_cache(&shd->bind_caches[1]);
	wgpuShaderModuleRelease(shd->vs);
	wgpuShaderModuleRelease(shd->fs);
	shd->~CF_ShaderInternal();
	CF_FREE(shd);
}

void cf_webgpu_shader_swap_contents(CF_Shader a, CF_Shader b)
{
	CF_ShaderInternal* pa = (CF_ShaderInternal*)a.id;
	CF_ShaderInternal* pb = (CF_ShaderInternal*)b.id;
	uint8_t tmp[sizeof(CF_ShaderInternal)];
	CF_MEMCPY(tmp, pa, sizeof(tmp));
	CF_MEMCPY(pa, pb, sizeof(tmp));
	CF_MEMCPY(pb, tmp, sizeof(tmp));
}

bool cf_webgpu_shader_consumes_uniform(CF_Shader shader_handle, const char* interned_name)
{
	CF_ShaderInternal* shd = (CF_ShaderInternal*)shader_handle.id;
	for (int s = 0; s < 2; ++s) {
		for (int b = 0; b < CF_MAX_UNIFORM_BLOCK_COUNT; ++b) {
			for (int i = 0; i < shd->stage[s].members[b].count(); ++i) {
				if (shd->stage[s].members[b][i].name == interned_name) return true;
			}
		}
	}
	return false;
}

bool cf_webgpu_shader_consumes_texture(CF_Shader shader_handle, const char* interned_name)
{
	CF_ShaderInternal* shd = (CF_ShaderInternal*)shader_handle.id;
	for (int s = 0; s < 2; ++s) {
		for (int i = 0; i < shd->stage[s].image_names.count(); ++i) {
			if (shd->stage[s].image_names[i] == interned_name) return true;
		}
	}
	return false;
}

//--------------------------------------------------------------------------------------------------
// Pipelines and draws.

static bool s_texture_unfilterable(const CF_TextureInternal* t)
{
	CF_WFormatInfo fi = s_format_info(t->format);
	if (fi.depth) return true;
	if (fi.float32 && !g.float32_filterable) return true;
	return false;
}

static CF_TextureInternal* s_material_texture(CF_MaterialState* ms, CF_WStageInfo* st, int slot, CF_MaterialTex** out_binding)
{
	for (int j = 0; j < st->image_names.count(); ++j) {
		if (st->image_slots[j] != slot) continue;
		const char* name = st->image_names[j];
		for (int i = 0; i < ms->textures.count(); ++i) {
			if (ms->textures[i].name == name) {
				if (out_binding) *out_binding = &ms->textures[i];
				return (CF_TextureInternal*)ms->textures[i].handle.id;
			}
		}
	}
	return NULL;
}

// Bit per sampled pair of the given stage whose bound texture needs an unfilterable layout.
static uint32_t s_unfilterable_mask(const CF_WLayoutInfo* info, int group, CF_MaterialState* ms, CF_WStageInfo* st)
{
	uint32_t mask = 0;
	for (int i = 0; i < info->count; ++i) {
		const CF_WBinding& b = info->b[i];
		if (b.group != group || b.kind != CF_WBIND_TEXTURE || b.sample_type != WGPUTextureSampleType_Float) continue;
		if (b.binding / 2 >= st->sampled_count || b.binding / 2 >= 16) continue;
		CF_TextureInternal* t = s_material_texture(ms, st, b.binding / 2, NULL);
		if (t && s_texture_unfilterable(t)) mask |= 1u << (b.binding / 2);
	}
	return mask;
}

static CF_WLayoutVariant* s_graphics_variant(CF_ShaderInternal* shd, uint32_t mask)
{
	for (int i = 0; i < shd->variants.count(); ++i) {
		if (shd->variants[i].unfilterable_mask == mask) return &shd->variants[i];
	}
	bool uniform_groups[4] = { false, true, false, true };
	shd->variants.add(s_make_variant(&shd->layout, 4, uniform_groups, mask));
	return &shd->variants.last();
}

static CF_WPipelineKey s_make_pipeline_key(CF_RenderState* state, CF_MeshInternal* mesh, CF_ShaderInternal* shader, uint32_t mask)
{
	CF_WPipelineKey key;
	CF_MEMSET(&key, 0, sizeof(key));
	CF_CanvasInternal* c = g.canvas;
	key.color_target_count = s_color_target_count(c);
	for (int i = 0; i < key.color_target_count; ++i) {
		CF_TextureInternal* t = (CF_TextureInternal*)(i == 0 ? c->cf_texture.id : c->cf_textures_mrt[i].id);
		key.color_formats[i] = t ? t->format : WGPUTextureFormat_Undefined;
	}
	key.sample_count = c->samples;
	CF_TextureInternal* d = c->depth_view ? (CF_TextureInternal*)c->cf_depth_stencil.id : NULL;
	key.depth_format = d ? d->format : WGPUTextureFormat_Undefined;
	key.render_state = *state;
	key.vertex_stride = mesh->vertices.buffer ? mesh->vertices.stride : 0;
	key.instance_stride = mesh->instances.buffer ? mesh->instances.stride : 0;
	key.index_stride = mesh->indices.buffer ? mesh->indices.stride : 0;
	int n = 0;
	for (int i = 0; i < mesh->attribute_count && n < CF_MAX_SHADER_INPUTS; ++i) {
		int idx = shader->get_input_index(mesh->attributes[i].name);
		if (idx >= 0) {
			key.attrs[n].location = shader->input_locations[idx];
			key.attrs[n].format = mesh->attributes[i].format;
			key.attrs[n].offset = mesh->attributes[i].offset;
			key.attrs[n].per_instance = mesh->attributes[i].per_instance;
			++n;
		}
	}
	key.attribute_count = n;
	key.unfilterable_mask = mask;
	return key;
}

static WGPURenderPipeline s_build_pipeline(CF_ShaderInternal* shader, CF_RenderState* state, CF_MeshInternal* mesh, const CF_WPipelineKey& key, CF_WLayoutVariant* variant)
{
	WGPUColorTargetState targets[CF_MAX_CANVAS_TARGETS];
	WGPUBlendState blends[CF_MAX_CANVAS_TARGETS];
	for (int i = 0; i < key.color_target_count; ++i) {
		const CF_BlendState* b = (i == 0 || state->blend_count <= i) ? &state->blend : &state->blends[i];
		targets[i] = WGPU_COLOR_TARGET_STATE_INIT;
		targets[i].format = key.color_formats[i];
		WGPUColorWriteMask mask = WGPUColorWriteMask_None;
		if (b->write_R_enabled) mask |= WGPUColorWriteMask_Red;
		if (b->write_G_enabled) mask |= WGPUColorWriteMask_Green;
		if (b->write_B_enabled) mask |= WGPUColorWriteMask_Blue;
		if (b->write_A_enabled) mask |= WGPUColorWriteMask_Alpha;
		targets[i].writeMask = mask;
		if (b->enabled && s_format_blendable(key.color_formats[i])) {
			blends[i].color.operation = s_wrap(b->rgb_op);
			blends[i].color.srcFactor = s_wrap(b->rgb_src_blend_factor);
			blends[i].color.dstFactor = s_wrap(b->rgb_dst_blend_factor);
			blends[i].alpha.operation = s_wrap(b->alpha_op);
			blends[i].alpha.srcFactor = s_wrap(b->alpha_src_blend_factor);
			blends[i].alpha.dstFactor = s_wrap(b->alpha_dst_blend_factor);
			// WebGPU requires min/max to use factor one.
			if (blends[i].color.operation == WGPUBlendOperation_Min || blends[i].color.operation == WGPUBlendOperation_Max) blends[i].color.srcFactor = blends[i].color.dstFactor = WGPUBlendFactor_One;
			if (blends[i].alpha.operation == WGPUBlendOperation_Min || blends[i].alpha.operation == WGPUBlendOperation_Max) blends[i].alpha.srcFactor = blends[i].alpha.dstFactor = WGPUBlendFactor_One;
			targets[i].blend = &blends[i];
		}
	}

	bool has_vertex_data = mesh->vertices.buffer != NULL;
	bool has_instance_data = mesh->instances.buffer != NULL;
	WGPUVertexAttribute vattrs[2][CF_MAX_SHADER_INPUTS];
	int vcount[2] = { 0, 0 };
	for (int i = 0; i < key.attribute_count; ++i) {
		int slot = (has_vertex_data && key.attrs[i].per_instance) ? 1 : 0;
		WGPUVertexAttribute a = WGPU_VERTEX_ATTRIBUTE_INIT;
		a.format = s_wrap(key.attrs[i].format);
		a.offset = (uint64_t)key.attrs[i].offset;
		a.shaderLocation = (uint32_t)key.attrs[i].location;
		vattrs[slot][vcount[slot]++] = a;
	}
	CF_ASSERT(key.attribute_count == shader->input_count);
	WGPUVertexBufferLayout vbl[2];
	int vbl_count = 0;
	if (has_vertex_data) {
		vbl[vbl_count] = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
		vbl[vbl_count].stepMode = WGPUVertexStepMode_Vertex;
		vbl[vbl_count].arrayStride = (uint64_t)mesh->vertices.stride;
		vbl[vbl_count].attributeCount = (size_t)vcount[0];
		vbl[vbl_count].attributes = vattrs[0];
		++vbl_count;
	}
	if (has_instance_data) {
		int slot = has_vertex_data ? 1 : 0;
		vbl[vbl_count] = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
		vbl[vbl_count].stepMode = WGPUVertexStepMode_Instance;
		vbl[vbl_count].arrayStride = (uint64_t)mesh->instances.stride;
		vbl[vbl_count].attributeCount = (size_t)vcount[slot];
		vbl[vbl_count].attributes = vattrs[slot];
		++vbl_count;
	}

	WGPURenderPipelineDescriptor pd = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
	pd.layout = variant->layout;
	pd.vertex.module = shader->vs;
	pd.vertex.entryPoint = s_sv("main");
	pd.vertex.bufferCount = (size_t)vbl_count;
	pd.vertex.buffers = vbl;
	pd.primitive.topology = s_wrap(state->primitive_type);
	if (key.index_stride && (pd.primitive.topology == WGPUPrimitiveTopology_TriangleStrip || pd.primitive.topology == WGPUPrimitiveTopology_LineStrip)) {
		pd.primitive.stripIndexFormat = key.index_stride == 2 ? WGPUIndexFormat_Uint16 : WGPUIndexFormat_Uint32;
	}
	pd.primitive.frontFace = WGPUFrontFace_CCW;
	pd.primitive.cullMode = s_wrap(state->cull_mode);
	pd.primitive.unclippedDepth = g.depth_clip_control && !state->enable_depth_clip && key.depth_format != WGPUTextureFormat_Undefined;
	if (!g.depth_clip_control && !state->enable_depth_clip && key.depth_format != WGPUTextureFormat_Undefined) {
		CF_WGPU_WARN_ONCE("WebGPU: this device lacks depth-clip-control; enable_depth_clip = false is ignored and depth still clips.\n");
	}

	WGPUDepthStencilState ds = WGPU_DEPTH_STENCIL_STATE_INIT;
	if (key.depth_format != WGPUTextureFormat_Undefined) {
		bool has_stencil = s_format_info(key.depth_format).stencil;
		ds.format = key.depth_format;
		ds.depthWriteEnabled = state->depth_write_enabled ? WGPUOptionalBool_True : WGPUOptionalBool_False;
		ds.depthCompare = s_wrap(state->depth_compare);
		if (has_stencil && state->stencil.enabled) {
			ds.stencilFront.compare = s_wrap(state->stencil.front.compare);
			ds.stencilFront.failOp = s_wrap(state->stencil.front.fail_op);
			ds.stencilFront.depthFailOp = s_wrap(state->stencil.front.depth_fail_op);
			ds.stencilFront.passOp = s_wrap(state->stencil.front.pass_op);
			ds.stencilBack.compare = s_wrap(state->stencil.back.compare);
			ds.stencilBack.failOp = s_wrap(state->stencil.back.fail_op);
			ds.stencilBack.depthFailOp = s_wrap(state->stencil.back.depth_fail_op);
			ds.stencilBack.passOp = s_wrap(state->stencil.back.pass_op);
			ds.stencilReadMask = state->stencil.read_mask;
			ds.stencilWriteMask = state->stencil.write_mask;
		} else {
			ds.stencilFront.compare = ds.stencilBack.compare = WGPUCompareFunction_Always;
			ds.stencilFront.failOp = ds.stencilFront.depthFailOp = ds.stencilFront.passOp = WGPUStencilOperation_Keep;
			ds.stencilBack.failOp = ds.stencilBack.depthFailOp = ds.stencilBack.passOp = WGPUStencilOperation_Keep;
			ds.stencilReadMask = 0xFF;
			ds.stencilWriteMask = has_stencil ? 0 : 0xFF;
		}
		bool triangles = pd.primitive.topology == WGPUPrimitiveTopology_TriangleList || pd.primitive.topology == WGPUPrimitiveTopology_TriangleStrip;
		if (state->enable_depth_bias && triangles) {
			ds.depthBias = (int32_t)state->depth_bias_constant_factor;
			ds.depthBiasSlopeScale = state->depth_bias_slope_factor;
			ds.depthBiasClamp = state->depth_bias_clamp;
		}
		pd.depthStencil = &ds;
	}
	pd.multisample.count = (uint32_t)key.sample_count;
	pd.multisample.mask = 0xFFFFFFFF;
	pd.multisample.alphaToCoverageEnabled = state->alpha_to_coverage && key.sample_count > 1 && key.color_target_count > 0;

	WGPUFragmentState fs = WGPU_FRAGMENT_STATE_INIT;
	fs.module = shader->fs;
	fs.entryPoint = s_sv("main");
	fs.targetCount = (size_t)key.color_target_count;
	fs.targets = targets;
	pd.fragment = &fs;
	WGPURenderPipeline pip = wgpuDeviceCreateRenderPipeline(g.device, &pd);
	CF_ASSERT(pip);
	return pip;
}

static WGPUSampler s_draw_sampler(CF_MaterialTex* binding, CF_TextureInternal* t, CF_WBindKind slot_kind, bool unfilterable)
{
	CF_WSampler* base = binding && binding->sampler.id ? (CF_WSampler*)binding->sampler.id : t->sampler;
	if (binding && !binding->sampler.id && g.has_filter_override && binding->name == sintern("u_image") && t->sampler) {
		int index = g.filter_override == CF_FILTER_LINEAR;
		if (!t->draw_samplers[index]) {
			WGPUSamplerDescriptor d = t->sampler->desc;
			d.minFilter = d.magFilter = s_wrap(g.filter_override);
			t->draw_samplers[index] = s_acquire_sampler(d);
		}
		base = t->draw_samplers[index];
	}
	return s_slot_sampler(base, slot_kind, unfilterable);
}

static WGPUTextureView s_dummy_view(WGPUTextureViewDimension dim, WGPUTextureSampleType st, WGPUTextureFormat storage_format)
{
	for (int i = 0; i < g.dummy_textures.count(); ++i) {
		const CF_WDummyTexture& d = g.dummy_textures[i];
		if (d.dim == dim && d.sample_type == st && d.storage_format == storage_format) return d.view;
	}
	CF_WDummyTexture d = { };
	d.dim = dim;
	d.sample_type = st;
	d.storage_format = storage_format;
	WGPUTextureDescriptor td = WGPU_TEXTURE_DESCRIPTOR_INIT;
	td.size = { 1, 1, 1 };
	td.mipLevelCount = 1;
	td.sampleCount = 1;
	td.dimension = dim == WGPUTextureViewDimension_3D ? WGPUTextureDimension_3D : WGPUTextureDimension_2D;
	if (dim == WGPUTextureViewDimension_Cube || dim == WGPUTextureViewDimension_CubeArray) td.size.depthOrArrayLayers = 6;
	if (storage_format != WGPUTextureFormat_Undefined) {
		td.format = storage_format;
		td.usage = WGPUTextureUsage_StorageBinding;
	} else {
		switch (st) {
		case WGPUTextureSampleType_Depth: td.format = WGPUTextureFormat_Depth32Float; break;
		case WGPUTextureSampleType_Sint:  td.format = WGPUTextureFormat_R32Sint; break;
		case WGPUTextureSampleType_Uint:  td.format = WGPUTextureFormat_R32Uint; break;
		default:                          td.format = WGPUTextureFormat_RGBA8Unorm; break;
		}
		td.usage = WGPUTextureUsage_TextureBinding;
	}
	d.tex = wgpuDeviceCreateTexture(g.device, &td);
	WGPUTextureViewDescriptor vd = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
	vd.dimension = dim;
	d.view = wgpuTextureCreateView(d.tex, &vd);
	g.dummy_textures.add(d);
	return d.view;
}

struct CF_WResourceSource
{
	CF_MaterialState* material;
	CF_WStageInfo* stage;
	int sampled_count;
	int storage_texture_count;
	CF_Texture* storage_textures;     // Compute only.
	int storage_texture_given;
	WGPUBuffer* buffers;
	int buffer_count;
	uint32_t unfilterable_mask;
};

// Entries for a resource group: sampled pairs, then storage textures, then storage buffers.
static int s_resource_entries(const CF_WLayoutInfo* info, int group, const CF_WResourceSource& src, WGPUBindGroupEntry* entries)
{
	int n = 0;
	for (int i = 0; i < info->count; ++i) {
		const CF_WBinding& b = info->b[i];
		if (b.group != group) continue;
		WGPUBindGroupEntry e = WGPU_BIND_GROUP_ENTRY_INIT;
		e.binding = (uint32_t)b.binding;
		int sampled_end = 2 * src.sampled_count;
		int storage_tex_end = sampled_end + src.storage_texture_count;
		if (b.binding < sampled_end && (b.kind == CF_WBIND_TEXTURE || b.kind == CF_WBIND_SAMPLER || b.kind == CF_WBIND_SAMPLER_CMP)) {
			int slot = b.binding / 2;
			CF_MaterialTex* mt = NULL;
			CF_TextureInternal* t = src.material ? s_material_texture(src.material, src.stage, slot, &mt) : NULL;
			bool unfilterable = slot < 32 && ((src.unfilterable_mask >> slot) & 1);
			if (b.kind == CF_WBIND_TEXTURE) {
				if (t && t->view) {
					e.textureView = t->view;
				} else {
					WGPUTextureSampleType st = b.sample_type == WGPUTextureSampleType_Float && unfilterable ? WGPUTextureSampleType_UnfilterableFloat : b.sample_type;
					e.textureView = s_dummy_view(b.dim, st, WGPUTextureFormat_Undefined);
				}
			} else {
				e.sampler = t ? s_draw_sampler(mt, t, b.kind, unfilterable) : s_slot_sampler(NULL, b.kind, unfilterable);
			}
		} else if (b.kind == CF_WBIND_STORAGE_TEXTURE) {
			int j = b.binding - sampled_end;
			CF_TextureInternal* t = (j >= 0 && j < src.storage_texture_given) ? (CF_TextureInternal*)src.storage_textures[j].id : NULL;
			e.textureView = t ? s_storage_view(t) : s_dummy_view(b.dim, WGPUTextureSampleType_Undefined, b.storage_format);
		} else if (b.kind == CF_WBIND_STORAGE_RO || b.kind == CF_WBIND_STORAGE_RW || b.kind == CF_WBIND_UNIFORM) {
			int k = b.binding - storage_tex_end;
			e.buffer = (k >= 0 && k < src.buffer_count && src.buffers[k]) ? src.buffers[k] : g.dummy_buffer;
			e.offset = 0;
			e.size = WGPU_WHOLE_SIZE;
		} else if (b.kind == CF_WBIND_TEXTURE) {
			e.textureView = s_dummy_view(b.dim, b.sample_type, WGPUTextureFormat_Undefined);
		} else {
			e.sampler = s_slot_sampler(NULL, b.kind, false);
		}
		entries[n++] = e;
	}
	return n;
}

static WGPUBindGroup s_create_bind_group(WGPUBindGroupLayout bgl, const WGPUBindGroupEntry* entries, int n)
{
	if (!n) {
		wgpuBindGroupAddRef(g.empty_bg);
		return g.empty_bg;
	}
	WGPUBindGroupDescriptor d = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
	d.layout = bgl;
	d.entryCount = (size_t)n;
	d.entries = entries;
	return wgpuDeviceCreateBindGroup(g.device, &d);
}

static WGPUBindGroup s_make_resource_group(const CF_WLayoutInfo* info, int group, WGPUBindGroupLayout bgl, const CF_WResourceSource& src)
{
	WGPUBindGroupEntry entries[CF_WGPU_MAX_BINDINGS];
	int n = s_resource_entries(info, group, src, entries);
	return s_create_bind_group(bgl, entries, n);
}

static bool s_same_entry(const WGPUBindGroupEntry& a, const WGPUBindGroupEntry& b)
{
	return a.binding == b.binding && a.buffer == b.buffer && a.offset == b.offset && a.size == b.size && a.sampler == b.sampler && a.textureView == b.textureView;
}

static uint64_t s_hash_entries(WGPUBindGroupLayout bgl, const WGPUBindGroupEntry* entries, int n)
{
	uint64_t h = 14695981039346656037ull;
	auto mix = [&](uint64_t v) { h = (h ^ v) * 1099511628211ull; };
	mix((uint64_t)(uintptr_t)bgl);
	for (int i = 0; i < n; ++i) {
		mix(entries[i].binding);
		mix((uint64_t)(uintptr_t)entries[i].buffer);
		mix((uint64_t)(uintptr_t)entries[i].sampler);
		mix((uint64_t)(uintptr_t)entries[i].textureView);
	}
	return h;
}

static void s_clear_bind_cache(CF_WBindCache* cache)
{
	for (int i = 0; i < CF_WGPU_BIND_CACHE_SIZE; ++i) {
		CF_WBindCacheEntry* e = cache->slots + i;
		if (e->group) wgpuBindGroupRelease(e->group);
		CF_FREE(e->entries);
		*e = { };
	}
	cache->next = 0;
}

// Returns a borrowed bind group: the cache owns it.
static WGPUBindGroup s_cached_bind_group(CF_WBindCache* cache, WGPUBindGroupLayout bgl, const WGPUBindGroupEntry* entries, int n)
{
	if (cache->epoch != g.bind_epoch) {
		s_clear_bind_cache(cache);
		cache->epoch = g.bind_epoch;
	}
	uint64_t h = s_hash_entries(bgl, entries, n);
	for (int i = 0; i < CF_WGPU_BIND_CACHE_SIZE; ++i) {
		CF_WBindCacheEntry* e = cache->slots + i;
		if (!e->group || e->hash != h || e->bgl != bgl || e->count != n) continue;
		bool same = true;
		for (int j = 0; j < n && same; ++j) same = s_same_entry(e->entries[j], entries[j]);
		if (same) return e->group;
	}
	CF_WBindCacheEntry* e = cache->slots + cache->next;
	cache->next = (cache->next + 1) % CF_WGPU_BIND_CACHE_SIZE;
	if (e->group) wgpuBindGroupRelease(e->group);
	CF_FREE(e->entries);
	e->hash = h;
	e->bgl = bgl;
	e->count = n;
	e->entries = n ? (WGPUBindGroupEntry*)CF_ALLOC(sizeof(WGPUBindGroupEntry) * n) : NULL;
	if (n) CF_MEMCPY(e->entries, entries, sizeof(WGPUBindGroupEntry) * n);
	e->group = s_create_bind_group(bgl, entries, n);
	return e->group;
}

static WGPUBindGroup s_uniform_group(CF_WUniformGroup* ug, const CF_WLayoutInfo* info, int group, WGPUBindGroupLayout bgl, CF_WStageInfo* st)
{
	if (ug->group && ug->ring_generation == g.ring_generation) return ug->group;
	s_release_uniform_group(ug);
	WGPUBindGroupEntry entries[CF_WGPU_MAX_BINDINGS];
	int n = 0;
	for (int i = 0; i < info->count; ++i) {
		const CF_WBinding& b = info->b[i];
		if (b.group != group) continue;
		WGPUBindGroupEntry e = WGPU_BIND_GROUP_ENTRY_INIT;
		e.binding = (uint32_t)b.binding;
		e.buffer = g.ring;
		e.offset = 0;
		int size = b.binding < CF_MAX_UNIFORM_BLOCK_COUNT ? st->block_sizes[b.binding] : 0;
		e.size = (uint64_t)s_align(cf_max(size, 16), 16);
		entries[n++] = e;
	}
	if (!n) {
		wgpuBindGroupAddRef(g.empty_bg);
		ug->group = g.empty_bg;
	} else {
		WGPUBindGroupDescriptor d = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
		d.layout = bgl;
		d.entryCount = (size_t)n;
		d.entries = entries;
		ug->group = wgpuDeviceCreateBindGroup(g.device, &d);
	}
	ug->ring_generation = g.ring_generation;
	return ug->group;
}

// Packs material uniforms into ring blocks; offsets[u] receives each declared block's offset.
static void s_copy_uniforms(CF_Arena* arena, CF_WStageInfo* st, CF_MaterialState* ms, const CF_WLayoutInfo* info, int group, uint32_t* offsets)
{
	for (int i = 0; i < info->count; ++i) {
		const CF_WBinding& b = info->b[i];
		if (b.group != group || b.kind != CF_WBIND_UNIFORM) continue;
		int block_index = b.binding;
		int size = block_index < CF_MAX_UNIFORM_BLOCK_COUNT ? st->block_sizes[block_index] : 16;
		size = s_align(cf_max(size, 16), 16);
		uint8_t* block = (uint8_t*)cf_arena_alloc(arena, size);
		CF_MEMSET(block, 0, size);
		if (block_index < CF_MAX_UNIFORM_BLOCK_COUNT) {
			for (int u = 0; u < ms->uniforms.count(); ++u) {
				CF_Uniform uniform = ms->uniforms[u];
				int idx = st->index(uniform.name, block_index);
				if (idx >= 0) {
					int offset = st->members[block_index][idx].offset;
					int n = cf_min(uniform.size, size - offset);
					if (n > 0) CF_MEMCPY(block + offset, uniform.data, n);
				}
			}
		}
		if (block_index < CF_MAX_UNIFORM_BLOCK_COUNT) offsets[block_index] = s_ring_alloc(block, size);
	}
	cf_arena_reset(arena);
}

static void s_apply_pass_state()
{
	const CF_WPassState& ps = g.ps;
	if (ps.has_viewport) wgpuRenderPassEncoderSetViewport(g.pass, ps.viewport[0], ps.viewport[1], ps.viewport[2], ps.viewport[3], 0, 1);
	if (ps.has_scissor) wgpuRenderPassEncoderSetScissorRect(g.pass, (uint32_t)ps.scissor[0], (uint32_t)ps.scissor[1], (uint32_t)ps.scissor[2], (uint32_t)ps.scissor[3]);
	if (ps.has_blend_constant) wgpuRenderPassEncoderSetBlendConstant(g.pass, &ps.blend_constant);
	if (ps.has_stencil_reference) wgpuRenderPassEncoderSetStencilReference(g.pass, ps.stencil_reference);
}

void cf_webgpu_apply_shader(CF_Shader shader_handle, CF_Material material_handle)
{
	CF_ASSERT(g.canvas);
	CF_ASSERT(g.canvas->mesh);
	if (g.device_lost) {
		g.shader = NULL;
		return;
	}
	CF_MeshInternal* mesh = g.canvas->mesh;
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	CF_ShaderInternal* shader = (CF_ShaderInternal*)shader_handle.id;
	CF_RenderState* state = &material->state;

	uint32_t mask = s_unfilterable_mask(&shader->layout, 0, &material->vs, &shader->stage[0])
		| (s_unfilterable_mask(&shader->layout, 2, &material->fs, &shader->stage[1]) << 16);
	CF_WLayoutVariant* variant = s_graphics_variant(shader, mask);
	CF_WPipelineKey key = s_make_pipeline_key(state, mesh, shader, mask);
	WGPURenderPipeline pip = NULL;
	for (int i = 0; i < shader->pip_cache.count(); ++i) {
		if (CF_MEMCMP(&key, &shader->pip_cache[i].key, sizeof(key)) == 0) {
			pip = shader->pip_cache[i].pip;
			break;
		}
	}
	if (!pip) {
		pip = s_build_pipeline(shader, state, mesh, key, variant);
		CF_WPipeline entry;
		entry.key = key;
		entry.pip = pip;
		shader->pip_cache.add(entry);
	}

	// Uniforms go into the ring first: a full ring submits, which must not cut the pass below.
	s_ring_reserve(s_ring_bytes(&shader->layout, 1, shader->stage[0].block_sizes) + s_ring_bytes(&shader->layout, 3, shader->stage[1].block_sizes));
	uint32_t offsets[2][CF_MAX_UNIFORM_BLOCK_COUNT] = { };
	s_copy_uniforms(&material->block_arena, &shader->stage[0], &material->vs, &shader->layout, 1, offsets[0]);
	s_copy_uniforms(&material->block_arena, &shader->stage[1], &material->fs, &shader->layout, 3, offsets[1]);

	if (!g.pass) {
		s_begin_pass(g.canvas, g.canvas->clear);
		g.canvas->clear = false;
		s_apply_pass_state();
	}

	wgpuRenderPassEncoderSetPipeline(g.pass, pip);
	bool has_vertex_data = mesh->vertices.buffer != NULL;
	bool has_instance_data = mesh->instances.buffer != NULL;
	WGPUBuffer instance_buffer = g.instance_override ? g.instance_override : mesh->instances.buffer;
	uint64_t instance_offset = g.instance_override ? (uint64_t)g.instance_override_offset : 0;
	if (has_vertex_data) wgpuRenderPassEncoderSetVertexBuffer(g.pass, 0, mesh->vertices.buffer, 0, WGPU_WHOLE_SIZE);
	if (has_instance_data) wgpuRenderPassEncoderSetVertexBuffer(g.pass, has_vertex_data ? 1 : 0, instance_buffer, instance_offset, WGPU_WHOLE_SIZE);
	if (mesh->indices.buffer) {
		wgpuRenderPassEncoderSetIndexBuffer(g.pass, mesh->indices.buffer, mesh->indices.stride == 2 ? WGPUIndexFormat_Uint16 : WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
	}
	g.ps.has_stencil_reference = true;
	g.ps.stencil_reference = state->stencil.reference;
	wgpuRenderPassEncoderSetStencilReference(g.pass, state->stencil.reference);

	g.shader = shader;
	g.material = material;
	g.variant = variant;
	CF_MEMCPY(g.uniform_offsets, offsets, sizeof(offsets));
}

// Binds the four groups for the draw about to be issued.
static void s_bind_for_draw()
{
	CF_ShaderInternal* shd = g.shader;
	CF_MaterialInternal* mat = g.material;
	CF_WLayoutVariant* v = g.variant;
	if (!shd || !mat || !v) return;
	for (int stage = 0; stage < 2; ++stage) {
		int rgroup = stage * 2;
		CF_WResourceSource src = { };
		src.material = stage == 0 ? &mat->vs : &mat->fs;
		src.stage = &shd->stage[stage];
		src.sampled_count = shd->stage[stage].sampled_count;
		src.storage_texture_count = shd->stage[stage].storage_texture_count;
		src.buffers = stage == 0 ? g.ps.vs_storage : g.ps.fs_storage;
		src.buffer_count = stage == 0 ? g.ps.vs_storage_count : g.ps.fs_storage_count;
		src.unfilterable_mask = stage == 0 ? (v->unfilterable_mask & 0xFFFF) : (v->unfilterable_mask >> 16);
		WGPUBindGroupEntry entries[CF_WGPU_MAX_BINDINGS];
		int n = s_resource_entries(&shd->layout, rgroup, src, entries);
		WGPUBindGroup bg = s_cached_bind_group(&shd->bind_caches[stage], v->bgl[rgroup], entries, n);
		wgpuRenderPassEncoderSetBindGroup(g.pass, (uint32_t)rgroup, bg, 0, NULL);

		int ugroup = rgroup + 1;
		WGPUBindGroup ubg = s_uniform_group(&shd->uniform_groups[stage], &shd->layout, ugroup, v->bgl[ugroup], &shd->stage[stage]);
		uint32_t dyn[CF_WGPU_MAX_BINDINGS];
		int dn = 0;
		for (int i = 0; i < shd->layout.count; ++i) {
			const CF_WBinding& b = shd->layout.b[i];
			if (b.group == ugroup) dyn[dn++] = b.binding < CF_MAX_UNIFORM_BLOCK_COUNT ? g.uniform_offsets[stage][b.binding] : 0;
		}
		wgpuRenderPassEncoderSetBindGroup(g.pass, (uint32_t)ugroup, ubg, (size_t)dn, dn ? dyn : NULL);
	}
	g.has_filter_override = false;
}

void cf_webgpu_apply_viewport(int x, int y, int w, int h)
{
	CF_WPassState& ps = g.ps;
	ps.viewport[0] = (float)x; ps.viewport[1] = (float)y; ps.viewport[2] = (float)w; ps.viewport[3] = (float)h;
	ps.has_viewport = true;
	if (g.pass) wgpuRenderPassEncoderSetViewport(g.pass, (float)x, (float)y, (float)w, (float)h, 0, 1);
}

void cf_webgpu_apply_scissor(int x, int y, int w, int h)
{
	// WebGPU rejects scissors outside the target; SDL_GPU clips them.
	int cw = g.canvas ? g.canvas->w : x + w;
	int ch = g.canvas ? g.canvas->h : y + h;
	int x0 = cf_clamp(x, 0, cw), y0 = cf_clamp(y, 0, ch);
	int x1 = cf_clamp(x + w, 0, cw), y1 = cf_clamp(y + h, 0, ch);
	CF_WPassState& ps = g.ps;
	ps.scissor[0] = x0; ps.scissor[1] = y0; ps.scissor[2] = cf_max(x1 - x0, 0); ps.scissor[3] = cf_max(y1 - y0, 0);
	ps.has_scissor = true;
	if (g.pass) wgpuRenderPassEncoderSetScissorRect(g.pass, (uint32_t)ps.scissor[0], (uint32_t)ps.scissor[1], (uint32_t)ps.scissor[2], (uint32_t)ps.scissor[3]);
}

void cf_webgpu_apply_stencil_reference(int reference)
{
	g.ps.stencil_reference = (uint32_t)reference;
	g.ps.has_stencil_reference = true;
	if (g.pass) wgpuRenderPassEncoderSetStencilReference(g.pass, (uint32_t)reference);
}

void cf_webgpu_apply_blend_constants(float r, float gg, float b, float a)
{
	g.ps.blend_constant = { r, gg, b, a };
	g.ps.has_blend_constant = true;
	if (g.pass) wgpuRenderPassEncoderSetBlendConstant(g.pass, &g.ps.blend_constant);
}

// SDL_GPU logs and skips a draw without a render pass; so does this backend. Anything that ends
// the pass (an upload, a readback, a dispatch) between cf_apply_shader and the draw causes it.
static bool s_draw_ready(const char* fn)
{
	if (g.device_lost) return false;
	if (g.pass && g.shader) return true;
	fprintf(stderr, "WebGPU: %s skipped: no render pass is active. Something ended it after cf_apply_shader (a mesh, texture, or buffer update, a readback, or a dispatch); call cf_apply_shader again first.\n", fn);
	return false;
}

void cf_webgpu_draw_elements()
{
	CF_MeshInternal* mesh = g.canvas->mesh;
	WGPUBuffer instance_override = g.instance_override;
	g.instance_override = NULL;
	if (!s_draw_ready("cf_draw_elements")) return;
	s_bind_for_draw();
	int instances = 1;
	if (mesh->instances.buffer) instances = instance_override ? g.instance_override_count : mesh->instances.element_count;
	if (mesh->indices.buffer) {
		wgpuRenderPassEncoderDrawIndexed(g.pass, (uint32_t)mesh->indices.element_count, (uint32_t)instances, 0, 0, 0);
	} else {
		wgpuRenderPassEncoderDraw(g.pass, (uint32_t)mesh->vertices.element_count, (uint32_t)instances, 0, 0);
	}
	app->draw_call_count++;
}

void cf_webgpu_draw_elements_range(int first_element, int element_count, int instance_count)
{
	CF_MeshInternal* mesh = g.canvas->mesh;
	WGPUBuffer instance_override = g.instance_override;
	g.instance_override = NULL;
	if (!s_draw_ready("cf_draw_elements_range")) return;
	if (instance_override) {
		wgpuRenderPassEncoderSetVertexBuffer(g.pass, mesh->vertices.buffer ? 1 : 0, instance_override, (uint64_t)g.instance_override_offset, WGPU_WHOLE_SIZE);
	}
	s_bind_for_draw();
	int ninst = instance_count;
	if (ninst <= 0) {
		if (instance_override) ninst = g.instance_override_count;
		else if (mesh->instances.buffer) ninst = mesh->instances.element_count;
		else ninst = 1;
	}
	if (mesh->indices.buffer) {
		int count = element_count >= 0 ? element_count : mesh->indices.element_count;
		wgpuRenderPassEncoderDrawIndexed(g.pass, (uint32_t)count, (uint32_t)ninst, (uint32_t)first_element, 0, 0);
	} else {
		int count = element_count >= 0 ? element_count : mesh->vertices.element_count;
		wgpuRenderPassEncoderDraw(g.pass, (uint32_t)count, (uint32_t)ninst, (uint32_t)first_element, 0);
	}
	app->draw_call_count++;
}

void cf_webgpu_draw_elements_instanced(int instance_count)
{
	CF_MeshInternal* mesh = g.canvas->mesh;
	if (!s_draw_ready("cf_draw_elements_instanced")) return;
	s_bind_for_draw();
	wgpuRenderPassEncoderDraw(g.pass, (uint32_t)mesh->vertices.element_count, (uint32_t)instance_count, 0, 0);
	app->draw_call_count++;
}

void cf_webgpu_draw_elements_indirect(CF_StorageBuffer args, int offset, int draw_count)
{
	CF_MeshInternal* mesh = g.canvas->mesh;
	CF_StorageBufferInternal* sb = (CF_StorageBufferInternal*)args.id;
	CF_ASSERT(sb && (sb->usage & WGPUBufferUsage_Indirect));
	if (!s_draw_ready("cf_draw_elements_indirect")) return;
	s_bind_for_draw();
	for (int i = 0; i < draw_count; ++i) {
		if (mesh->indices.buffer) {
			wgpuRenderPassEncoderDrawIndexedIndirect(g.pass, sb->buffer, (uint64_t)(offset + i * 20));
		} else {
			wgpuRenderPassEncoderDrawIndirect(g.pass, sb->buffer, (uint64_t)(offset + i * 16));
		}
	}
	app->draw_call_count++;
}

void* cf_webgpu_create_draw_sampler(CF_Filter filter)
{
	return (void*)((uintptr_t)filter + 1);
}

void cf_webgpu_destroy_draw_sampler(void* sampler)
{
	CF_UNUSED(sampler);
}

void cf_webgpu_set_sampler_override(void* sampler)
{
	g.has_filter_override = sampler != NULL;
	if (sampler) g.filter_override = (CF_Filter)((uintptr_t)sampler - 1);
}

// Debug groups must nest inside one encoder or pass, which CF's labels do not respect.
void cf_webgpu_push_gpu_label(const char* name) { CF_UNUSED(name); }
void cf_webgpu_pop_gpu_label() { }

//--------------------------------------------------------------------------------------------------
// Compute.

CF_ComputeShader cf_webgpu_make_compute_shader_from_bytecode(CF_ShaderBytecode bytecode)
{
	const char* src = s_wgsl_src(bytecode);
	WGPUShaderModule module = s_make_user_module(src, "cs");
	if (!module) return { 0 };
	CF_ComputeShaderInternal* cs = CF_NEW(CF_ComputeShaderInternal);
	CF_MEMSET(cs, 0, sizeof(*cs));
	cs->module = module;
	s_parse_wgsl_bindings(&cs->layout, src, WGPUShaderStage_Compute);
	const CF_ShaderInfo* info = &bytecode.shader_info;
	s_load_stage_info(&cs->stage, info);
	cs->ro_storage_texture_count = info->num_storage_textures;
	cs->ro_storage_buffer_count = info->num_storage_buffers;
	cs->rw_storage_texture_count = info->num_readwrite_storage_textures;
	cs->rw_storage_buffer_count = info->num_readwrite_storage_buffers;
	for (int i = 0; i < info->num_wgsl_splits; ++i) {
		CF_WSplit split = { info->wgsl_splits[i].store_binding, info->wgsl_splits[i].load_binding };
		cs->splits.add(split);
	}
	CF_ComputeShader result;
	result.id = (uint64_t)(uintptr_t)cs;
	return result;
}

static WGPUComputePipeline s_compute_pipeline(CF_ComputeShaderInternal* cs, uint32_t mask, CF_WLayoutVariant** out_variant)
{
	for (int i = 0; i < cs->variants.count(); ++i) {
		if (cs->variants[i].unfilterable_mask == mask) {
			*out_variant = &cs->variants[i];
			return cs->pipelines[i];
		}
	}
	bool uniform_groups[4] = { false, false, true, false };
	cs->variants.add(s_make_variant(&cs->layout, 3, uniform_groups, mask));
	WGPUComputePipelineDescriptor pd = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
	pd.layout = cs->variants.last().layout;
	pd.compute.module = cs->module;
	pd.compute.entryPoint = s_sv("main");
	WGPUComputePipeline pip = wgpuDeviceCreateComputePipeline(g.device, &pd);
	CF_ASSERT(pip);
	cs->pipelines.add(pip);
	*out_variant = &cs->variants.last();
	return pip;
}

void cf_webgpu_destroy_compute_shader(CF_ComputeShader shader)
{
	CF_ComputeShaderInternal* cs = (CF_ComputeShaderInternal*)shader.id;
	if (!cs) return;
	for (int i = 0; i < cs->pipelines.count(); ++i) wgpuComputePipelineRelease(cs->pipelines[i]);
	for (int i = 0; i < cs->variants.count(); ++i) s_release_variant(&cs->variants[i]);
	s_release_uniform_group(&cs->uniform_group);
	wgpuShaderModuleRelease(cs->module);
	cs->~CF_ComputeShaderInternal();
	CF_FREE(cs);
}

void cf_webgpu_compute_shader_swap_contents(CF_ComputeShader a, CF_ComputeShader b)
{
	CF_ComputeShaderInternal* pa = (CF_ComputeShaderInternal*)a.id;
	CF_ComputeShaderInternal* pb = (CF_ComputeShaderInternal*)b.id;
	uint8_t tmp[sizeof(CF_ComputeShaderInternal)];
	CF_MEMCPY(tmp, pa, sizeof(tmp));
	CF_MEMCPY(pa, pb, sizeof(tmp));
	CF_MEMCPY(pb, tmp, sizeof(tmp));
}

static WGPUTextureView s_split_scratch(CF_TextureInternal* t)
{
	if (!t->split_scratch) {
		WGPUTextureDescriptor td = WGPU_TEXTURE_DESCRIPTOR_INIT;
		td.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
		td.dimension = t->type == CF_TEXTURE_TYPE_3D ? WGPUTextureDimension_3D : WGPUTextureDimension_2D;
		td.size = { (uint32_t)t->w, (uint32_t)t->h, (uint32_t)t->layers };
		td.format = t->format;
		td.mipLevelCount = 1;
		td.sampleCount = 1;
		t->split_scratch = wgpuDeviceCreateTexture(g.device, &td);
		WGPUTextureViewDescriptor vd = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
		vd.dimension = t->type == CF_TEXTURE_TYPE_3D ? WGPUTextureViewDimension_3D : (t->layers > 1 ? WGPUTextureViewDimension_2DArray : WGPUTextureViewDimension_2D);
		t->split_scratch_view = wgpuTextureCreateView(t->split_scratch, &vd);
	}
	WGPUTexelCopyTextureInfo s = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
	s.texture = t->tex;
	WGPUTexelCopyTextureInfo d = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
	d.texture = t->split_scratch;
	WGPUExtent3D ext = { (uint32_t)t->w, (uint32_t)t->h, (uint32_t)t->layers };
	wgpuCommandEncoderCopyTextureToTexture(s_encoder(), &s, &d, &ext);
	return t->split_scratch_view;
}

void cf_webgpu_dispatch_compute(CF_ComputeShader shader, CF_Material material_handle, CF_ComputeDispatch dispatch)
{
	CF_ComputeShaderInternal* cs = (CF_ComputeShaderInternal*)shader.id;
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	s_end_active_pass();
	if (g.device_lost) return;

	uint32_t mask = s_unfilterable_mask(&cs->layout, 0, &material->cs, &cs->stage);
	CF_WLayoutVariant* variant = NULL;
	WGPUComputePipeline pip = s_compute_pipeline(cs, mask, &variant);

	s_ring_reserve(s_ring_bytes(&cs->layout, 2, cs->stage.block_sizes));
	uint32_t offsets[CF_MAX_UNIFORM_BLOCK_COUNT] = { };
	s_copy_uniforms(&material->block_arena, &cs->stage, &material->cs, &cs->layout, 2, offsets);

	// Load sides of split images read a copy taken before the dispatch.
	WGPUTextureView split_views[CF_WGPU_MAX_BINDINGS];
	for (int i = 0; i < cs->splits.count(); ++i) {
		int j = cs->splits[i].store_binding;
		CF_TextureInternal* t = (j >= 0 && j < dispatch.rw_texture_count) ? (CF_TextureInternal*)dispatch.rw_textures[j].id : NULL;
		split_views[i] = t ? s_split_scratch(t) : NULL;
	}

	WGPUBuffer ro_buffers[16], rw_buffers[16];
	for (int i = 0; i < dispatch.ro_buffer_count && i < 16; ++i) ro_buffers[i] = ((CF_StorageBufferInternal*)dispatch.ro_buffers[i].id)->buffer;
	for (int i = 0; i < dispatch.rw_buffer_count && i < 16; ++i) rw_buffers[i] = ((CF_StorageBufferInternal*)dispatch.rw_buffers[i].id)->buffer;

	CF_WResourceSource ro = { };
	ro.material = &material->cs;
	ro.stage = &cs->stage;
	ro.sampled_count = cs->stage.sampled_count;
	ro.storage_texture_count = cs->ro_storage_texture_count;
	ro.storage_textures = dispatch.ro_textures;
	ro.storage_texture_given = dispatch.ro_texture_count;
	ro.buffers = ro_buffers;
	ro.buffer_count = cf_min(dispatch.ro_buffer_count, 16);
	ro.unfilterable_mask = mask;
	WGPUBindGroup bg0 = s_make_resource_group(&cs->layout, 0, variant->bgl[0], ro);

	CF_WResourceSource rw = { };
	rw.storage_texture_count = cs->rw_storage_texture_count;
	rw.storage_textures = dispatch.rw_textures;
	rw.storage_texture_given = dispatch.rw_texture_count;
	rw.buffers = rw_buffers;
	rw.buffer_count = cf_min(dispatch.rw_buffer_count, 16);
	WGPUBindGroup bg1 = NULL;
	if (cs->splits.count()) {
		// Build group 1 by hand: the load sides sit past the rw buffers.
		WGPUBindGroupEntry entries[CF_WGPU_MAX_BINDINGS];
		int n = 0;
		for (int i = 0; i < cs->layout.count; ++i) {
			const CF_WBinding& b = cs->layout.b[i];
			if (b.group != 1) continue;
			WGPUBindGroupEntry e = WGPU_BIND_GROUP_ENTRY_INIT;
			e.binding = (uint32_t)b.binding;
			if (b.kind == CF_WBIND_STORAGE_TEXTURE) {
				CF_TextureInternal* t = b.binding < dispatch.rw_texture_count ? (CF_TextureInternal*)dispatch.rw_textures[b.binding].id : NULL;
				e.textureView = t ? s_storage_view(t) : s_dummy_view(b.dim, WGPUTextureSampleType_Undefined, b.storage_format);
			} else if (b.kind == CF_WBIND_TEXTURE) {
				WGPUTextureView v = NULL;
				for (int s = 0; s < cs->splits.count(); ++s) if (cs->splits[s].load_binding == b.binding) v = split_views[s];
				e.textureView = v ? v : s_dummy_view(b.dim, b.sample_type, WGPUTextureFormat_Undefined);
			} else {
				int k = b.binding - cs->rw_storage_texture_count;
				e.buffer = (k >= 0 && k < rw.buffer_count) ? rw_buffers[k] : g.dummy_buffer;
				e.size = WGPU_WHOLE_SIZE;
			}
			entries[n++] = e;
		}
		WGPUBindGroupDescriptor d = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
		d.layout = variant->bgl[1];
		d.entryCount = (size_t)n;
		d.entries = entries;
		bg1 = wgpuDeviceCreateBindGroup(g.device, &d);
	} else {
		bg1 = s_make_resource_group(&cs->layout, 1, variant->bgl[1], rw);
	}
	WGPUBindGroup bg2 = s_uniform_group(&cs->uniform_group, &cs->layout, 2, variant->bgl[2], &cs->stage);
	uint32_t dyn[CF_WGPU_MAX_BINDINGS];
	int dn = 0;
	for (int i = 0; i < cs->layout.count; ++i) {
		const CF_WBinding& b = cs->layout.b[i];
		if (b.group == 2) dyn[dn++] = b.binding < CF_MAX_UNIFORM_BLOCK_COUNT ? offsets[b.binding] : 0;
	}

	WGPUComputePassDescriptor cpd = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
	WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(s_encoder(), &cpd);
	wgpuComputePassEncoderSetPipeline(pass, pip);
	wgpuComputePassEncoderSetBindGroup(pass, 0, bg0, 0, NULL);
	wgpuComputePassEncoderSetBindGroup(pass, 1, bg1, 0, NULL);
	wgpuComputePassEncoderSetBindGroup(pass, 2, bg2, (size_t)dn, dn ? dyn : NULL);
	wgpuComputePassEncoderDispatchWorkgroups(pass, (uint32_t)dispatch.group_count_x, (uint32_t)dispatch.group_count_y, (uint32_t)dispatch.group_count_z);
	wgpuComputePassEncoderEnd(pass);
	wgpuComputePassEncoderRelease(pass);
	wgpuBindGroupRelease(bg0);
	wgpuBindGroupRelease(bg1);
}

void cf_webgpu_gpu_sync()
{
	s_submit();
	volatile bool done = false;
	WGPUQueueWorkDoneCallbackInfo cb = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
	cb.mode = WGPUCallbackMode_AllowProcessEvents;
	cb.callback = [](WGPUQueueWorkDoneStatus status, WGPUStringView message, void* ud1, void* ud2) {
		CF_UNUSED(status); CF_UNUSED(message); CF_UNUSED(ud2);
		*(volatile bool*)ud1 = true;
	};
	cb.userdata1 = (void*)&done;
	wgpuQueueOnSubmittedWorkDone(g.queue, cb);
	s_wait(&done);
}

bool cf_webgpu_device_is_lost()
{
	return g.device_lost;
}

int cf_webgpu_error_count()
{
	return g.error_count;
}

// Loses the device the way a driver reset does: wgpu-native reports it to the lost callback
// from the next call that fails on the device, not from here.
void cf_webgpu_lose_device()
{
	if (!g.device || g.device_lost) return;
	g.lose_on_destroy = true;
	wgpuDeviceDestroy(g.device);
}

//--------------------------------------------------------------------------------------------------
// Dear ImGui.

void cf_webgpu_imgui_init()
{
	ImGui_ImplSDL3_InitForOther(g.window);
	ImGui_ImplWGPU_InitInfo info;
	info.Device = g.device;
	info.NumFramesInFlight = 3;
	info.RenderTargetFormat = g.surface_format;
	info.DepthStencilFormat = WGPUTextureFormat_Undefined;
	ImGui_ImplWGPU_Init(&info);
	g.imgui_ready = true;
}

void cf_webgpu_imgui_new_frame()
{
	ImGui_ImplWGPU_NewFrame();
}

void cf_webgpu_imgui_draw()
{
	ImDrawData* draw_data = ImGui::GetDrawData();
	bool minimized = draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f;
	if (!g.swapchain_view || minimized) return;
	s_end_active_pass();
	WGPURenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
	ca.view = g.swapchain_view;
	ca.loadOp = WGPULoadOp_Load;
	ca.storeOp = WGPUStoreOp_Store;
	WGPURenderPassDescriptor rp = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
	rp.colorAttachmentCount = 1;
	rp.colorAttachments = &ca;
	WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(s_encoder(), &rp);
	ImGui_ImplWGPU_RenderDrawData(draw_data, pass);
	wgpuRenderPassEncoderEnd(pass);
	wgpuRenderPassEncoderRelease(pass);
}

void cf_webgpu_imgui_shutdown()
{
	if (g.imgui_ready) ImGui_ImplWGPU_Shutdown();
	g.imgui_ready = false;
}

//--------------------------------------------------------------------------------------------------
// Shutdown.

void cf_webgpu_cleanup()
{
	s_submit();
	s_release_swapchain();
#ifndef CF_EMSCRIPTEN
	if (g.device) wgpuDevicePoll(g.device, true, NULL);
#endif
	// Map callbacks hold readback and chunk pointers, so every pending map has to land before the
	// free, and before the instance goes: releasing it fires them with a cancel.
	for (int i = 0; i < g.readbacks.count(); ++i) s_free_readback(g.readbacks[i]);
	g.readbacks.clear();
	for (int i = 0; i < g.staging_pending.count(); ++i) s_wait(&g.staging_pending[i]->map_done);
	for (int i = 0; i < g.staging_pending.count(); ++i) s_free_staging_chunk(g.staging_pending[i]);
	for (int i = 0; i < g.staging_free.count(); ++i) s_free_staging_chunk(g.staging_free[i]);
	g.staging_pending.clear();
	g.staging_free.clear();
	for (int i = 0; i < g.blit_pipelines.count(); ++i) {
		wgpuRenderPipelineRelease(g.blit_pipelines[i].pip);
		wgpuBindGroupLayoutRelease(g.blit_pipelines[i].bgl);
	}
	g.blit_pipelines.clear();
	if (g.blit_module) wgpuShaderModuleRelease(g.blit_module);
	for (int i = 0; i < g.dummy_textures.count(); ++i) {
		wgpuTextureViewRelease(g.dummy_textures[i].view);
		wgpuTextureRelease(g.dummy_textures[i].tex);
	}
	g.dummy_textures.clear();
	for (int i = 0; i < g.samplers.count(); ++i) {
		wgpuSamplerRelease(g.samplers[i]->sampler);
		CF_FREE(g.samplers[i]);
	}
	g.samplers.clear();
	if (g.dummy_buffer) wgpuBufferRelease(g.dummy_buffer);
	if (g.empty_bg) wgpuBindGroupRelease(g.empty_bg);
	if (g.empty_bgl) wgpuBindGroupLayoutRelease(g.empty_bgl);
	if (g.ring) wgpuBufferRelease(g.ring);
	CF_FREE(g.ring_cpu);
	if (g.surface) {
		if (g.surface_configured) wgpuSurfaceUnconfigure(g.surface);
		wgpuSurfaceRelease(g.surface);
	}
#ifdef __APPLE__
	if (g.metal_view) SDL_Metal_DestroyView(g.metal_view);
#endif
	if (g.queue) wgpuQueueRelease(g.queue);
	if (g.device) wgpuDeviceRelease(g.device);
	if (g.adapter) wgpuAdapterRelease(g.adapter);
	if (g.instance) wgpuInstanceRelease(g.instance);
	g.staging.~Array();
	g.staging_pending.~Array();
	g.staging_free.~Array();
	g.readbacks.~Array();
	g.samplers.~Array();
	g.blit_pipelines.~Array();
	g.dummy_textures.~Array();
	CF_MEMSET(&g, 0, sizeof(g));
}

#endif // CF_WEBGPU
