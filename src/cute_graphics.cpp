/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_defines.h>
#include <cute_c_runtime.h>
#include <cute_graphics.h>
#include <cute_file_system.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_app_internal.h>
#include <internal/cute_graphics_internal.h>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#define SDL_GPU_SHADERCROSS_IMPLEMENTATION
#define SDL_GPU_SHADERCROSS_STATIC
#include <SDL_gpu_shadercross/SDL_gpu_shadercross.h>
#include <SDL_gpu_shadercross/spirv.h>
#include <SPIRV-Reflect/spirv_reflect.h>

struct CF_CanvasInternal;
static CF_CanvasInternal* s_canvas = NULL;
static CF_CanvasInternal* s_default_canvas = NULL;

#include <float.h>

using namespace Cute;

//--------------------------------------------------------------------------------------------------
// Builtin shaders. These get cross-compiled at runtime.

// Render as white -- for testing.
const char* s_basic_vs = R"(
layout (location = 0) in vec2 in_posH;

void main()
{
	gl_Position = vec4(in_posH, 0, 1);
}
)";
const char* s_basic_fs = R"(
layout(location = 0) out vec4 result;

void main()
{
	result = vec4(1);
}
)";

// Copy the app's offscreen canvas onto the swapchain texture (backfbuffer).
const char* s_backbuffer_vs = R"(
layout (location = 0) in vec2 in_posH;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec2 uv;

void main()
{
	uv = in_uv;
	gl_Position = vec4(in_posH, 0, 1);
}
)";
const char* s_backbuffer_fs = R"(
layout (location = 0) in vec2 uv;

layout(location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_image;

layout (set = 3, binding = 0) uniform uniform_block {
	vec2 u_texture_size;
};

#include "smooth_uv.shd"

void main()
{
	vec4 color = texture(u_image, smooth_uv(uv, u_texture_size));
	result = color;
}
)";

//--------------------------------------------------------------------------------------------------
// Utility shaders (included into other shaders).

const char* s_gamma = R"(
vec4 gamma(vec4 c)
{
	return vec4(pow(abs(c.rgb), vec3(1.0/2.2)), c.a);
}

vec4 de_gamma(vec4 c)
{
	return vec4(pow(abs(c.rgb), vec3(2.2)), c.a);
}
)";

const char* s_blend = R"(
// HSV <-> RGB from : http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
// And https://www.shadertoy.com/view/MsS3Wc

vec3 rgb_to_hsv(vec3 c)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
	vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);
	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv_to_rgb(vec3 c)
{
	vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0);
	rgb = rgb*rgb*(3.0-2.0*rgb);
	return c.z * mix(vec3(1.0), rgb, c.y);
}

vec3 hue(vec3 base, vec3 tint)
{
	base = rgb_to_hsv(base);
	tint = rgb_to_hsv(tint);
	return hsv_to_rgb(vec3(tint.r, base.gb));
}

vec4 hue(vec4 base, vec4 tint)
{
	return vec4(hue(base.rgb, tint.rgb), base.a);
}

float overlay(float base, float blend)
{
	return (base <= 0.5) ? 2*base * blend : 1-2*(1-base) * (1-blend);
}

vec3 overlay(vec3 base, vec3 blend)
{
	return vec3(overlay(base.r, blend.r), overlay(base.g, blend.g), overlay(base.b, blend.b));
}

vec4 overlay(vec4 base, vec4 blend)
{
	return vec4(overlay(base.rgb, blend.rgb), base.a);
}

float softlight(float base, float blend)
{
	if (blend <= 0.5) return base - (1-2*blend)*base*(1-base);
	else return base + (2.0 * blend - 1) * (((base <= 0.25) ? ((16.0 * base - 12.0) * base + 4.0) * base : sqrt(base)) - base);
}

vec3 softlight(vec3 base, vec3 blend)
{
	return vec3(softlight(base.r, blend.r), softlight(base.g, blend.g), softlight(base.b, blend.b));
}

vec4 softlight(vec4 base, vec4 blend)
{
	return vec4(softlight(base.rgb, blend.rgb), base.a);
}
)";

const char* s_distance = R"(
float safe_div(float a, float b)
{
	return b == 0.0 ? 0.0 : a / b;
}

float safe_len(vec2 v)
{
	float d = dot(v,v);
	return d == 0.0 ? 0.0 : sqrt(d);
}

vec2 safe_norm(vec2 v, float l)
{
	return mix(vec2(0), v / l, l == 0.0 ? 0.0 : 1.0);
}

vec2 skew(vec2 v)
{
	return vec2(-v.y, v.x);
}

float det2(vec2 a, vec2 b)
{
	return a.x * b.y - a.y * b.x;
}

float sdf_stroke(float d)
{
	return abs(d) - v_stroke;
}

float sdf_intersect(float a, float b)
{
	return max(a, b);
}

float sdf_union(float a, float b)
{
	return min(a, b);
}

float sdf_subtract(float d0, float d1)
{
	return max(d0, -d1);
}

float dd(float d)
{
	return length(vec2(dFdx(d), dFdy(d)));
}

vec4 sdf(vec4 a, vec4 b, float d)
{
	float wire_d = sdf_stroke(d);
	vec4 stroke_aa = mix(b, a, smoothstep(0.0, v_aa, wire_d));
	vec4 stroke_no_aa = wire_d <= 0.0 ? b : a;

	vec4 fill_aa = mix(b, a, smoothstep(0.0, v_aa, d));
	vec4 fill_no_aa = clamp(d, -1.0, 1.0) <= 0.0 ? b : a;

	vec4 stroke = mix(stroke_aa, stroke_aa, v_aa > 0.0 ? 1.0 : 0.0);
	vec4 fill = mix(fill_no_aa, fill_aa, v_aa > 0.0 ? 1.0 : 0.0);

	result = mix(stroke, fill, v_fill);
	return result;
}

float distance_aabb(vec2 p, vec2 he)
{
	vec2 d = abs(p) - he;
	return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float distance_box(vec2 p, vec2 c, vec2 he, vec2 u)
{
	mat2 m = transpose(mat2(u, skew(u)));
	p = p - c;
	p = m * p;
	return distance_aabb(p, he);
}

// Referenced from: https://www.shadertoy.com/view/3tdSDj
float distance_segment(vec2 p, vec2 a, vec2 b)
{
	vec2 n = b - a;
	vec2 pa = p - a;
	float d = safe_div(dot(pa,n), dot(n,n));
	float h = clamp(d, 0.0, 1.0);
	return safe_len(pa - h * n);
}

// Referenced from: https://www.shadertoy.com/view/XsXSz4
float distance_triangle(vec2 p, vec2 a, vec2 b, vec2 c)
{
	vec2 e0 = b - a;
	vec2 e1 = c - b;
	vec2 e2 = a - c;

	vec2 v0 = p - a;
	vec2 v1 = p - b;
	vec2 v2 = p - c;

	vec2 pq0 = v0 - e0 * clamp(safe_div(dot(v0, e0), dot(e0, e0)), 0.0, 1.0);
	vec2 pq1 = v1 - e1 * clamp(safe_div(dot(v1, e1), dot(e1, e1)), 0.0, 1.0);
	vec2 pq2 = v2 - e2 * clamp(safe_div(dot(v2, e2), dot(e2, e2)), 0.0, 1.0);

	float s = det2(e0, e2);
	vec2 d = min(min(vec2(dot(pq0, pq0), s * det2(v0, e0)),
						vec2(dot(pq1, pq1), s * det2(v1, e1))),
						vec2(dot(pq2, pq2), s * det2(v2, e2)));

	return -sqrt(d.x) * sign(d.y);
}
)";

const char* s_smooth_uv = R"(
vec2 smooth_uv(vec2 uv, vec2 texture_size)
{
	vec2 pixel = uv * texture_size;
	vec2 seam = floor(pixel + 0.5);
	pixel = seam + clamp((pixel - seam) / fwidth(pixel), -0.5, 0.5);
	return pixel / texture_size;
}
)";

// Stub function. This gets replaced by injected user-shader code via #include.
const char* s_shader_stub = R"(
vec4 shader(vec4 color, vec2 pos, vec2 atlas_uv, vec2 screen_uv, vec4 params)
{
	return color;
}
)";

//--------------------------------------------------------------------------------------------------
// Primary cute_draw.h shader.

const char* s_draw_vs = R"(
layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_posH;
layout (location = 2) in vec2 in_uv;

layout (location = 3) in vec2 in_a;
layout (location = 4) in vec2 in_b;
layout (location = 5) in vec2 in_c;
layout (location = 6) in vec4 in_col;
layout (location = 7) in float in_radius;
layout (location = 8) in float in_stroke;
layout (location = 9) in float in_aa;
layout (location = 10) in vec4 in_params;
layout (location = 11) in vec4 in_user_params;

layout (location = 0) out vec2 v_pos;
layout (location = 1) out vec2 v_a;
layout (location = 2) out vec2 v_b;
layout (location = 3) out vec2 v_c;
layout (location = 4) out vec2 v_uv;
layout (location = 5) out vec4 v_col;
layout (location = 6) out float v_radius;
layout (location = 7) out float v_stroke;
layout (location = 8) out float v_aa;
layout (location = 9) out float v_type;
layout (location = 10) out float v_alpha;
layout (location = 11) out float v_fill;
layout (location = 12) out vec2 v_posH;
layout (location = 13) out vec4 v_user;

void main()
{
	v_pos = in_pos;
	v_a = in_a;
	v_b = in_b;
	v_c = in_c;
	v_uv = in_uv;
	v_col = in_col;
	v_radius = in_radius;
	v_stroke = in_stroke;
	v_aa = in_aa;
	v_type = in_params.r;
	v_alpha = in_params.g;
	v_fill = in_params.b;
	// = in_params.a;

	vec4 posH = vec4(in_posH, 0, 1);
	gl_Position = posH;
	v_posH = in_posH;
	v_user = in_user_params;
}
)";

const char* s_draw_fs = R"(
layout (location = 0) in vec2 v_pos;
layout (location = 1) in vec2 v_a;
layout (location = 2) in vec2 v_b;
layout (location = 3) in vec2 v_c;
layout (location = 4) in vec2 v_uv;
layout (location = 5) in vec4 v_col;
layout (location = 6) in float v_radius;
layout (location = 7) in float v_stroke;
layout (location = 8) in float v_aa;
layout (location = 9) in float v_type;
layout (location = 10) in float v_alpha;
layout (location = 11) in float v_fill;
layout (location = 12) in vec2 v_posH;
layout (location = 13) in vec4 v_user;

layout(location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_image;

layout (set = 3, binding = 0) uniform uniform_block {
	vec2 u_texture_size;
};

#include "blend.shd"
#include "gamma.shd"
#include "smooth_uv.shd"
#include "distance.shd"
#include "shader_stub.shd"

void main()
{
	bool is_sprite  = v_type >= (0.0/255.0) && v_type < (0.5/255.0);
	bool is_text    = v_type >  (0.5/255.0) && v_type < (1.5/255.0);
	bool is_box     = v_type >  (1.5/255.0) && v_type < (2.5/255.0);
	bool is_seg     = v_type >  (2.5/255.0) && v_type < (3.5/255.0);
	bool is_tri     = v_type >  (3.5/255.0) && v_type < (4.5/255.0);
	bool is_tri_sdf = v_type >  (4.5/255.0) && v_type < (5.5/255.0);

	// Traditional sprite/text/tri cases.
	vec4 c = vec4(0);
	c = !(is_sprite && is_text) ? de_gamma(texture(u_image, smooth_uv(v_uv, u_texture_size))) : c;
	c = is_sprite ? gamma(c) : c;
	c = is_text ? v_col * c.a : c;
	c = is_tri ? v_col : c;

	// SDF cases.
	float d = 0;
	if (is_box) {
		d = distance_box(v_pos, v_a, v_b, v_c);
	} else if (is_seg) {
		d = distance_segment(v_pos, v_a, v_b);
		d = min(d, distance_segment(v_pos, v_b, v_c));
	} else if (is_tri_sdf) {
		d = distance_triangle(v_pos, v_a, v_b, v_c);
	}
	c = (!is_sprite && !is_text && !is_tri) ? sdf(c, v_col, d - v_radius) : c;

	c *= v_alpha;
	vec2 screen_uv = (v_posH + vec2(1,-1)) * 0.5 * vec2(1,-1);
	c = shader(c, v_pos, v_uv, screen_uv, v_user);
	if (c.a == 0) discard;
	result = c;
}
)";

//--------------------------------------------------------------------------------------------------
// SDL_GPU wrapping implementation of cute_graphics.h.
// ...Variety of enum converters/struct initializers are in cute_graphics_internal.h.

CF_INLINE CF_UniformType s_uniform_type(SpvReflectTypeDescription* type_desc)
{
	switch (type_desc->op) {
	case SpvOpTypeFloat: return CF_UNIFORM_TYPE_FLOAT;
	case SpvOpTypeInt: return CF_UNIFORM_TYPE_INT;
	case SpvOpTypeVector:
		if (type_desc->traits.numeric.scalar.width == 32) {
			if (type_desc->traits.numeric.scalar.signedness == 0) {
				switch (type_desc->traits.numeric.vector.component_count) {
					case 2: return CF_UNIFORM_TYPE_FLOAT2;
					case 4: return CF_UNIFORM_TYPE_FLOAT4;
					default: return CF_UNIFORM_TYPE_UNKNOWN;
				}
			} else {
				switch (type_desc->traits.numeric.vector.component_count) {
					case 2: return CF_UNIFORM_TYPE_INT2;
					case 4: return CF_UNIFORM_TYPE_INT4;
					default: return CF_UNIFORM_TYPE_UNKNOWN;
				}
			}
		}
		break;
	case SpvOpTypeMatrix:
		if (type_desc->traits.numeric.matrix.column_count == 4 && type_desc->traits.numeric.matrix.row_count == 4)
			return CF_UNIFORM_TYPE_MAT4;
		break;
	default:
		return CF_UNIFORM_TYPE_UNKNOWN;
	}
	return CF_UNIFORM_TYPE_UNKNOWN;
}

CF_INLINE CF_ShaderInputFormat s_wrap(SpvReflectFormat format)
{
	switch (format) {
	case SPV_REFLECT_FORMAT_UNDEFINED:           return CF_SHADER_INPUT_FORMAT_UNKNOWN;
	case SPV_REFLECT_FORMAT_R32_UINT:            return CF_SHADER_INPUT_FORMAT_UINT;
	case SPV_REFLECT_FORMAT_R32_SINT:            return CF_SHADER_INPUT_FORMAT_INT;
	case SPV_REFLECT_FORMAT_R32_SFLOAT:          return CF_SHADER_INPUT_FORMAT_FLOAT;
	case SPV_REFLECT_FORMAT_R32G32_UINT:         return CF_SHADER_INPUT_FORMAT_UVEC2;
	case SPV_REFLECT_FORMAT_R32G32_SINT:         return CF_SHADER_INPUT_FORMAT_IVEC2;
	case SPV_REFLECT_FORMAT_R32G32_SFLOAT:       return CF_SHADER_INPUT_FORMAT_VEC2;
	case SPV_REFLECT_FORMAT_R32G32B32_UINT:      return CF_SHADER_INPUT_FORMAT_UVEC3;
	case SPV_REFLECT_FORMAT_R32G32B32_SINT:      return CF_SHADER_INPUT_FORMAT_IVEC3;
	case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:    return CF_SHADER_INPUT_FORMAT_VEC3;
	case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:   return CF_SHADER_INPUT_FORMAT_UVEC4;
	case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:   return CF_SHADER_INPUT_FORMAT_IVEC4;
	case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return CF_SHADER_INPUT_FORMAT_VEC4;
	default: return CF_SHADER_INPUT_FORMAT_UNKNOWN;
	}
}

struct CF_Buffer
{
	int element_count;
	int size;
	int stride;
	SDL_GPUBuffer* buffer;
};

struct CF_MeshInternal
{
	CF_Buffer vertices;
	CF_Buffer indices;
	CF_Buffer instances;
	int attribute_count;
	SDL_GPUTransferBuffer* transfer_buffer;
	CF_VertexAttribute attributes[CF_MESH_MAX_VERTEX_ATTRIBUTES];
};

CF_BackendType cf_query_backend()
{
	SDL_GPUDriver driver = SDL_GetGPUDriver(app->device);
	switch (driver) {
	case SDL_GPU_DRIVER_INVALID: return CF_BACKEND_TYPE_INVALID;
	case SDL_GPU_DRIVER_SECRET:  return CF_BACKEND_TYPE_SECRET_NDA;
	case SDL_GPU_DRIVER_VULKAN:  return CF_BACKEND_TYPE_VULKAN;
	case SDL_GPU_DRIVER_D3D11:   return CF_BACKEND_TYPE_D3D11;
	case SDL_GPU_DRIVER_D3D12:   return CF_BACKEND_TYPE_D3D12;
	case SDL_GPU_DRIVER_METAL:   return CF_BACKEND_TYPE_METAL;
	default: return CF_BACKEND_TYPE_INVALID;
	}
}

bool cf_texture_supports_format(CF_PixelFormat format, CF_TextureUsageBits usage)
{
	return SDL_GPUTextureSupportsFormat(
			app->device,
			s_wrap(format),
			SDL_GPU_TEXTURETYPE_2D,
			usage
			);
}

CF_TextureParams cf_texture_defaults(int w, int h)
{
	CF_TextureParams params;
	params.pixel_format = CF_PIXEL_FORMAT_R8G8B8A8_UNORM;
	params.filter = CF_FILTER_NEAREST;
	params.usage = CF_TEXTURE_USAGE_SAMPLER_BIT;
	params.wrap_u = CF_WRAP_MODE_REPEAT;
	params.wrap_v = CF_WRAP_MODE_REPEAT;
	params.width = w;
	params.height = h;
	params.stream = false;
	return params;
}

CF_INLINE bool s_is_depth(CF_PixelFormat format)
{
	return format >= PIXEL_FORMAT_D16_UNORM;
}

CF_Texture cf_make_texture(CF_TextureParams params)
{
	SDL_GPUTextureCreateInfo tex_info = SDL_GPUTextureCreateInfoDefaults(params.width, params.height);
	tex_info.width = (Uint32)params.width;
	tex_info.height = (Uint32)params.height;
	tex_info.format = s_wrap(params.pixel_format);
	tex_info.usageFlags = params.usage;
	SDL_GPUTexture* tex = SDL_CreateGPUTexture(app->device, &tex_info);
	CF_ASSERT(tex);
	if (!tex) return { 0 };

	SDL_GPUSampler* sampler = NULL;
	// Depth/stencil textures don't need their own sampler, as the associated color
	// texture in the owning canvas already has a sampler attached.
	if (!s_is_depth(params.pixel_format)) {
		SDL_GPUSamplerCreateInfo sampler_info = SDL_GPUSamplerCreateInfoDefaults();
		sampler_info.minFilter = s_wrap(params.filter);
		sampler_info.magFilter = s_wrap(params.filter);
		sampler_info.addressModeU = s_wrap(params.wrap_u);
		sampler_info.addressModeV = s_wrap(params.wrap_v);
		sampler = SDL_CreateGPUSampler(app->device, &sampler_info);
		CF_ASSERT(sampler);
		if (!sampler) {
			SDL_ReleaseGPUTexture(app->device, tex);
			return { 0 };
		}
	}

	SDL_GPUTransferBuffer* buf = NULL;
	if (params.stream) {
		int texel_size = (int)SDL_GPUTextureFormatTexelBlockSize(tex_info.format);
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.sizeInBytes = (Uint32)(texel_size * params.width * params.height),
			.props = 0,
		};
		SDL_GPUTransferBuffer* buf = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
	}

	CF_TextureInternal* tex_internal = CF_NEW(CF_TextureInternal);
	tex_internal->w = params.width;
	tex_internal->h = params.height;
	tex_internal->filter = sampler ? s_wrap(params.filter) : SDL_GPU_FILTER_NEAREST;
	tex_internal->tex = tex;
	tex_internal->buf = buf;
	tex_internal->sampler = sampler;
	tex_internal->format = tex_info.format;
	CF_Texture result;
	result.id = { (uint64_t)tex_internal };
	return result;
}

void cf_destroy_texture(CF_Texture texture_handle)
{
	CF_TextureInternal* tex = (CF_TextureInternal*)texture_handle.id;
	SDL_ReleaseGPUTexture(app->device, tex->tex);
	if (tex->sampler) SDL_ReleaseGPUSampler(app->device, tex->sampler);
	if (tex->buf) SDL_ReleaseGPUTransferBuffer(app->device, tex->buf);
	CF_FREE(tex);
}

static SDL_GPUTextureLocation SDL_GPUTextureLocationDefaults(CF_TextureInternal* tex, float x, float y)
{
	SDL_GPUTextureLocation location;
	CF_MEMSET(&location, 0, sizeof(location));
	location.texture = tex->tex;
	location.x = (Uint32)(x * tex->w);
	location.y = (Uint32)(y * tex->h);
	return location;
}

void cf_texture_update(CF_Texture texture_handle, void* data, int size)
{
	CF_TextureInternal* tex = (CF_TextureInternal*)texture_handle.id;

	// Copy bytes over to the driver.
	SDL_GPUTransferBuffer* buf = tex->buf;
	if (!buf) {
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.sizeInBytes = (Uint32)size,
			.props = 0,
		};
		buf = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
	}
	void* p = SDL_MapGPUTransferBuffer(app->device, buf, tex->buf ? true : false);
	CF_MEMCPY(p, data, size);
	SDL_UnmapGPUTransferBuffer(app->device, buf);

	// Tell the driver to upload the bytes to the GPU.
	SDL_GPUCommandBuffer* cmd = app->cmd ? app->cmd : SDL_AcquireGPUCommandBuffer(app->device);
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTextureTransferInfo src;
	src.transferBuffer = buf;
	src.offset = 0;
	src.imagePitch = tex->w;
	src.imageHeight = tex->h;
	SDL_GPUTextureRegion dst = SDL_GPUTextureRegionDefaults(tex, tex->w, tex->h);
	SDL_UploadToGPUTexture(pass, &src, &dst, tex->buf ? true : false);
	SDL_EndGPUCopyPass(pass);
	if (!tex->buf) SDL_ReleaseGPUTransferBuffer(app->device, buf);
	if (!app->cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

uint64_t cf_texture_handle(CF_Texture texture)
{
	return (uint64_t)((CF_TextureInternal*)texture.id)->tex;
}

static void s_shader_directory_recursive(Path path)
{
	Array<Path> dir = Directory::enumerate(app->shader_directory + path);
	for (int i = 0; i < dir.size(); ++i) {
		Path p = app->shader_directory + path + dir[i];
		if (p.is_directory()) {
			s_shader_directory_recursive(p);
		} else {
			CF_Stat stat;
			fs_stat(p, &stat);
			String ext = p.ext();
			if (ext == ".vs" || ext == ".fs" || ext == ".shd") {
				// Exclude app->shader_directory for easier lookups.
				// e.g. app->shader_directory is "/shaders" and contains
				// "/shaders/my_shader.shd", the user needs to only reference it by:
				// "my_shader.shd".
				CF_ShaderFileInfo info;
				info.stat = stat;
				info.path = sintern(p);
				const char* key = sintern(path + dir[i]);
				app->shader_file_infos.add(key, info);
			}
		}
	}
}

void cf_shader_directory(const char* path)
{
	CF_ASSERT(!app->shader_directory_set);
	if (app->shader_directory_set) return;
	app->shader_directory_set = true;
	app->shader_directory = path;
	s_shader_directory_recursive("/");
}

void cf_shader_on_changed(void (*on_changed_fn)(const char* path, void* udata), void* udata)
{
	app->on_shader_changed_fn = on_changed_fn;
	app->on_shader_changed_udata = udata;
}

static void s_shader_watch_recursive(Path path)
{
	Array<Path> dir = Directory::enumerate(app->shader_directory + path);
	for (int i = 0; i < dir.size(); ++i) {
		Path p = app->shader_directory + path + dir[i];
		if (p.is_directory()) {
			s_shader_directory_recursive(p);
		} else {
			CF_Stat stat;
			fs_stat(p, &stat);
			String ext = p.ext();
			if (ext == ".vs" || ext == ".fs" || ext == ".shd") {
				const char* key = sintern(path + dir[i]);
				CF_ShaderFileInfo& info = app->shader_file_infos.find(key);
				if (info.stat.last_modified_time < stat.last_modified_time) {
					info.stat.last_modified_time = stat.last_modified_time;
					app->on_shader_changed_fn(key, app->on_shader_changed_udata);
				}
			}
		}
	}
}

void cf_shader_watch()
{
	if (!app->on_shader_changed_fn) return;
	s_shader_watch_recursive("/");
}

const dyna uint8_t* cf_compile_shader_to_bytecode(const char* shader_src, CF_ShaderStage cf_stage)
{
	EShLanguage stage = EShLangVertex;
	switch (cf_stage) {
	default: CF_ASSERT(false); break; // No valid stage provided.
	case CF_SHADER_STAGE_VERTEX: stage = EShLangVertex; break;
	case CF_SHADER_STAGE_FRAGMENT: stage = EShLangFragment; break;
	}

	glslang::TShader shader(stage);

	const char* shader_strings[1];
	shader_strings[0] = shader_src;
	shader.setStrings(shader_strings, 1);

	shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 450);
	shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
	shader.setEntryPoint("main");
	shader.setSourceEntryPoint("main");
	shader.setAutoMapLocations(true);
	shader.setAutoMapBindings(true);

	if (!shader.parse(GetDefaultResources(), 450, false, EShMsgDefault)) {
		fprintf(stderr, "GLSL parsing failed...\n");
		fprintf(stderr, "%s\n\n%s\n", shader.getInfoLog(), shader.getInfoDebugLog());
		return NULL;
	}

	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(EShMsgDefault)) {
		fprintf(stderr, "GLSL linking failed...\n");
		fprintf(stderr, "%s\n\n%s\n", program.getInfoLog(), program.getInfoDebugLog());
		return NULL;
	}

	std::vector<uint32_t> spirv;
	glslang::SpvOptions options;
	options.generateDebugInfo = false;
	options.stripDebugInfo = false;
	options.disableOptimizer = false;
	options.optimizeSize = false;
	options.disassemble = false;
	options.validate = false;
	glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &options);

	dyna uint8_t* bytecode = NULL;
	int size = (int)(sizeof(uint32_t) * spirv.size());
	afit(bytecode, size);
	CF_MEMCPY(bytecode, spirv.data(), size);
	alen(bytecode) = size;

	return bytecode;
}

static SDL_GPUShader* s_compile(CF_ShaderInternal* shader_internal, const dyna uint8_t* bytecode, CF_ShaderStage stage)
{
	bool vs = stage == CF_SHADER_STAGE_VERTEX ? true : false;
	SpvReflectShaderModule module;
	spvReflectCreateShaderModule(asize(bytecode), bytecode, &module);

	// Gather up counts for samplers/textures/buffers.
	// ...SDL_GPU needs these counts.
	uint32_t binding_count = 0;
	spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr);
	dyna SpvReflectDescriptorBinding** bindings = NULL;
	afit(bindings, (int)binding_count);
	if (binding_count) alen(bindings) = binding_count;
	spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings);
	int sampler_count = 0;
	int storage_texture_count = 0;
	int storage_buffer_count = 0;
	int uniform_buffer_count = 0;
	for (int i = 0; i < (int)binding_count; ++i) {
		SpvReflectDescriptorBinding* binding = bindings[i];

		switch (binding->descriptor_type) {
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		{
			shader_internal->image_names.add(sintern(binding->name));
		}    // Fall-thru.
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: sampler_count++; break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: storage_texture_count++; break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: storage_buffer_count++; break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		{
			// Grab information about the uniform block.
			// ...This allows CF_Material to dynamically match uniforms to a shader.
			uniform_buffer_count++;
			int block_index = binding->binding;
			if (vs) {
				shader_internal->vs_block_sizes[block_index] = binding->block.size;
			} else {
				shader_internal->fs_block_sizes[block_index] = binding->block.size;
			}
			for (uint32_t i = 0; i < binding->block.member_count; ++i) {
				const SpvReflectBlockVariable* member = &binding->block.members[i];
				CF_UniformType uniform_type = s_uniform_type(member->type_description);
				CF_ASSERT(uniform_type != CF_UNIFORM_TYPE_UNKNOWN);
				int array_length = 1;
				if (member->type_description->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY && member->type_description->traits.array.dims_count > 0) {
					array_length = (int)member->type_description->traits.array.dims[0];
				}

				CF_UniformBlockMember block_member;
				block_member.name = sintern(member->name);
				block_member.block_name = sintern(binding->type_description->type_name);
				block_member.type = uniform_type;
				block_member.array_element_count = array_length;
				block_member.size = s_uniform_size(block_member.type) * array_length;
				block_member.offset = (int)member->offset;
				if (vs) {
					shader_internal->vs_uniform_block_members[block_index].add(block_member);
				} else {
					shader_internal->fs_uniform_block_members[block_index].add(block_member);
				}
			}
		} break;
		}
	}
	afree(bindings);
	shader_internal->uniform_block_count = uniform_buffer_count;

	// Gather up type information on shader inputs.
	if (vs) {
		uint32_t input_count = 0;
		spvReflectEnumerateInputVariables(&module, &input_count, nullptr);
		CF_ASSERT(input_count <= CF_MAX_SHADER_INPUTS); // Increase `CF_MAX_SHADER_INPUTS`, or refactor the shader with less vertex attributes.
		shader_internal->input_count = input_count;
		dyna SpvReflectInterfaceVariable** inputs = NULL;
		afit(inputs, (int)input_count);
		alen(inputs) = (int)input_count;
		spvReflectEnumerateInputVariables(&module, &input_count, inputs);
		for (int i = 0; i < alen(inputs); ++i) {
			SpvReflectInterfaceVariable* input = inputs[i];

			shader_internal->input_names[i] = sintern(input->name);
			shader_internal->input_locations[i] = input->location;
			shader_internal->input_formats[i] = s_wrap(input->format);
		}
		afree(inputs);
	}

	// Create the actual shader.
	SDL_GPUShaderCreateInfo shaderCreateInfo = {};
	shaderCreateInfo.code = bytecode;
	shaderCreateInfo.codeSize = asize(bytecode);
	shaderCreateInfo.entryPointName = "main";
	shaderCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
	shaderCreateInfo.stage = s_wrap(stage);
	shaderCreateInfo.samplerCount = sampler_count;
	shaderCreateInfo.storageTextureCount = storage_texture_count;
	shaderCreateInfo.storageBufferCount = storage_buffer_count;
	shaderCreateInfo.uniformBufferCount = uniform_buffer_count;
	SDL_GPUShader* sdl_shader = NULL;
	if (SDL_GetGPUDriver(app->device) == SDL_GPU_DRIVER_VULKAN) {
		sdl_shader = (SDL_GPUShader*)SDL_CreateGPUShader(app->device, &shaderCreateInfo);
	} else {
		sdl_shader = (SDL_GPUShader*)SDL_ShaderCross_CompileFromSPIRV(app->device, &shaderCreateInfo, false);
	}
	afree(bytecode);
	CF_ASSERT(sdl_shader);
	return sdl_shader;
}

CF_Shader cf_make_shader_from_bytecode(const dyna uint8_t* vertex_bytecode, const dyna uint8_t* fragment_bytecode)
{
	CF_ShaderInternal* shader_internal = CF_NEW(CF_ShaderInternal);
	CF_MEMSET(shader_internal, 0, sizeof(*shader_internal));

	shader_internal->vs = s_compile(shader_internal, vertex_bytecode, CF_SHADER_STAGE_VERTEX);
	shader_internal->fs = s_compile(shader_internal, fragment_bytecode, CF_SHADER_STAGE_FRAGMENT);
	CF_ASSERT(shader_internal->vs);
	CF_ASSERT(shader_internal->fs);

	CF_Shader result;
	result.id = { (uint64_t)shader_internal };
	return result;
}

// Return the index of the first #include substring that's not in a comment.
static int s_find_first_include(const char *src)
{
	const char *in = src;
	while (*in) {
		if (*in == '/' && *(in + 1) == '/') {
			in += 2;
			while (*in && *in != '\n') {
				in++;
			}
		} else if (*in == '/' && *(in + 1) == '*') {
			in += 2;
			while (*in && !(*in == '*' && *(in + 1) == '/')) {
				in++;
			}
			if (*in) {
				in += 2;
			}
		} else if (*in == '#' && !sequ(in, "#include")) {
			return (int)(in - src);
		} else {
			in++;
		}
	}
	return -1;
}

// Recursively apply #include directives in shaders.
// ...A cache is used to protect against multiple includes and infinite loops.
static String s_include_recurse(Map<const char*, const char*>& incl_protection, String shd, bool builtin, const char* user_shd)
{
	while (1) {
		int idx = s_find_first_include(shd);
		if (idx < 0) break;

		// Cut out the #include substring, and record the path.
		char* s = shd + idx + 8;
		int n = 0;
		while (*s++ != '\n') ++n;
		++n; // skip '\n'
		String path = String(shd + idx + 8, s).trim();
		shd.erase(idx, n + 8);
		path.replace("\"", "");
		path.replace("'", "");

		// Search for the shader to include.
		if (builtin || fs_file_exists(path)) {
			String ext = Path(path).ext();
			if (ext == ".vs" || ext == ".fs" || ext == ".shd") {
				String incl;
				bool found = false;
				if (builtin) {
					if (sequ(path.c_str(), "shader_stub.shd")) {
						// Inject the user shader if-applicable, stub if not.
						incl = user_shd ? user_shd : s_shader_stub;
						found = true;
					} else {
						// Builtin shaders can include other builtin shaders.
						const char* result = app->builtin_shaders.find(sintern(path));
						if (result) {
							incl = result;
							found = true;
						}
					}
				}

				// Wasn't a builtin shader, try including a user shader.
				if (!found) {
					// Processing a user-include shader.
					const char* result = fs_read_entire_file_to_memory_and_nul_terminate(path);
					if (result) {
						incl = result;
						found = true;
						cf_free((void*)result);
					}
				}

				if (found) {
					// Prevent infinite include loops.
					const char* incl_path = sintern(path);
					if (!incl_protection.has(incl_path)) {
						incl_protection.add(incl_path);
						incl = s_include_recurse(incl_protection, incl, builtin, user_shd);
						
						// Perform the actual string splice + inclusion.
						shd = String(shd, shd + idx)
							.append("// -- begin include ")
							.append(path)
							.append(" --\n")
							.append(incl)
							.append("// -- end include ")
							.append(path)
							.append(" --\n")
							.append(shd + idx);
					}
				}
			}
		}
	}
	return shd;
}

// Parse + perform include directives across shaders.
static String s_include(String shd, bool builtin, const char* user_shd)
{
	Map<const char*, const char*> incl_protection;
	return s_include_recurse(incl_protection, shd, builtin, user_shd);
}

static CF_Shader s_compile(const char* vs_src, const char* fs_src, bool builtin = false, const char* user_shd = NULL)
{
	// Support #include directives.
	String vs = s_include(vs_src, builtin, NULL);
	String fs = s_include(fs_src, builtin, user_shd);

#if 0
	printf(vs.c_str());
	printf("---\n");
	printf(fs.c_str());
	printf("---\n");
#endif

	// Compile to bytecode.
	const char* vertex = vs.c_str();
	const char* fragment = fs.c_str();
	const dyna uint8_t* vs_bytecode = cf_compile_shader_to_bytecode(vertex, CF_SHADER_STAGE_VERTEX);
	if (!vs_bytecode) {
		CF_Shader result = { 0 };
		return result;
	}
	const dyna uint8_t* fs_bytecode = cf_compile_shader_to_bytecode(fragment, CF_SHADER_STAGE_FRAGMENT);
	if (!fs_bytecode) {
		afree(vs_bytecode);
		CF_Shader result = { 0 };
		return result;
	}

	// Create the actual shader object.
	return cf_make_shader_from_bytecode(vs_bytecode, fs_bytecode);
}

void cf_load_internal_shaders()
{
#ifdef CF_RUNTIME_SHADER_COMPILATION
	glslang::InitializeProcess();
#endif

	// Map out all the builtin includable shaders.
	app->builtin_shaders.add(sintern("shader_stub.shd"), s_shader_stub);
	app->builtin_shaders.add(sintern("gamma.shd"), s_gamma);
	app->builtin_shaders.add(sintern("distance.shd"), s_distance);
	app->builtin_shaders.add(sintern("smooth_uv.shd"), s_smooth_uv);
	app->builtin_shaders.add(sintern("blend.shd"), s_blend);

	// Compile built-in shaders.
	app->draw_shader = s_compile(s_draw_vs, s_draw_fs, true, NULL);
	app->basic_shader = s_compile(s_basic_vs, s_basic_fs, true, NULL);
	app->backbuffer_shader = s_compile(s_backbuffer_vs, s_backbuffer_fs, true, NULL);
}

void cf_unload_internal_shaders()
{
	cf_destroy_shader(app->draw_shader);
	cf_destroy_shader(app->basic_shader);
	cf_destroy_shader(app->backbuffer_shader);
#ifdef CF_RUNTIME_SHADER_COMPILATION
	glslang::FinalizeProcess();
#endif
}

// Create a user shader by injecting their `shader` function into CF's draw shader.
CF_Shader cf_make_draw_shader_internal(const char* path)
{
	Path p = Path("/") + path;
	const char* path_s = sintern(p);
	CF_ShaderFileInfo info = app->shader_file_infos.find(path_s);
	if (!info.path) return { 0 };
	char* shd = fs_read_entire_file_to_memory_and_nul_terminate(info.path);
	if (!shd) return { 0 };
	CF_Shader result = cf_make_draw_shader_from_source_internal(shd);
	cf_free(shd);
	return result;
}

CF_Shader cf_make_draw_shader_from_source_internal(const char* src)
{
	return s_compile(s_draw_vs, s_draw_fs, true, src);
}

CF_Shader cf_make_shader(const char* vertex_path, const char* fragment_path)
{
	// Make sure each file can be found.
	const char* vs = fs_read_entire_file_to_memory_and_nul_terminate(vertex_path);
	const char* fs = fs_read_entire_file_to_memory_and_nul_terminate(fragment_path);
	CF_ASSERT(vs);
	CF_ASSERT(fs);
	return s_compile(vs, fs);
}

CF_Shader cf_make_shader_from_source(const char* vertex_src, const char* fragment_src)
{
	return s_compile(vertex_src, fragment_src);
}

void cf_destroy_shader(CF_Shader shader_handle)
{
	CF_ShaderInternal* shd = (CF_ShaderInternal*)shader_handle.id;
	SDL_ReleaseGPUShader(app->device, shd->vs);
	SDL_ReleaseGPUShader(app->device, shd->fs);
	for (int i = 0; i < shd->pip_cache.count(); ++i) {
		SDL_ReleaseGPUGraphicsPipeline(app->device, shd->pip_cache[i].pip);
	}
	shd->~CF_ShaderInternal();
	CF_FREE(shd);
}

CF_CanvasParams cf_canvas_defaults(int w, int h)
{
	CF_CanvasParams params;
	if (w == 0 || h == 0) {
		params.name = NULL;
		params.target = { };
		params.depth_stencil_target = { };
	} else {
		params.name = NULL;
		params.target = cf_texture_defaults(w, h);
		params.target.usage |= CF_TEXTURE_USAGE_COLOR_TARGET_BIT;
		params.depth_stencil_enable = false;
		params.depth_stencil_target = cf_texture_defaults(w, h);
		params.depth_stencil_target.pixel_format = CF_PIXEL_FORMAT_D32_FLOAT_S8_UINT;
    if (cf_texture_supports_format(CF_PIXEL_FORMAT_D24_UNORM_S8_UINT, CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT)) {
      params.depth_stencil_target.pixel_format = CF_PIXEL_FORMAT_D24_UNORM_S8_UINT;
    }
		params.depth_stencil_target.usage = CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT;
	}
	return params;
}

CF_Canvas cf_make_canvas(CF_CanvasParams params)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)CF_CALLOC(sizeof(CF_CanvasInternal));
	if (params.target.width > 0 && params.target.height > 0) {
		canvas->cf_texture = cf_make_texture(params.target);
		if (canvas->cf_texture.id) {
			canvas->texture = ((CF_TextureInternal*)canvas->cf_texture.id)->tex;
			canvas->sampler = ((CF_TextureInternal*)canvas->cf_texture.id)->sampler;
		}
		if (params.depth_stencil_enable) {
			canvas->cf_depth_stencil = cf_make_texture(params.depth_stencil_target);
			if (canvas->cf_depth_stencil.id) {
				canvas->depth_stencil = ((CF_TextureInternal*)canvas->cf_depth_stencil.id)->tex;
			}
		} else {
			canvas->cf_depth_stencil = { 0 };
		}
	} else {
		return { 0 };
	}
	CF_Canvas result;
	result.id = (uint64_t)canvas;
	return result;
}

void cf_destroy_canvas(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	cf_destroy_texture(canvas->cf_texture);
	if (canvas->depth_stencil) cf_destroy_texture(canvas->cf_depth_stencil);
	CF_FREE(canvas);
}

CF_Texture cf_canvas_get_target(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	return canvas->cf_texture;
}

CF_Texture cf_canvas_get_depth_stencil_target(CF_Canvas canvas_handle)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	return canvas->cf_depth_stencil;
}

void cf_canvas_blit(CF_Canvas src, CF_V2 u0, CF_V2 v0, CF_Canvas dst, CF_V2 u1, CF_V2 v1)
{
	CF_Texture src_tex_cf = cf_canvas_get_target(src);
	CF_Texture dst_tex_cf = cf_canvas_get_target(dst);
	CF_TextureInternal* src_tex_internal = (CF_TextureInternal*)src_tex_cf.id;
	CF_TextureInternal* dst_tex_internal = (CF_TextureInternal*)dst_tex_cf.id;
	SDL_GPUTexture* src_texture = (SDL_GPUTexture*)cf_texture_handle(src_tex_cf);
	SDL_GPUTexture* dst_texture = (SDL_GPUTexture*)cf_texture_handle(dst_tex_cf);

	float src_width = (float)src_tex_internal->w;
	float src_height = (float)src_tex_internal->h;
	float src_x = u0.x * src_width;
	float src_y = u0.y * src_height;
	float src_w = (v0.x - u0.x) * src_width;
	float src_h = (v0.y - u0.y) * src_height;
	
	float dst_width = (float)dst_tex_internal->w;
	float dst_height = (float)dst_tex_internal->h;
	float dst_x = u1.x * dst_width;
	float dst_y = u1.y * dst_height;
	float dst_w = (v1.x - u1.x) * dst_width;
	float dst_h = (v1.y - u1.y) * dst_height;

	int flip = SDL_FLIP_NONE;
	if (src_w < 0) {
		flip |= SDL_FLIP_HORIZONTAL;
		src_w = -src_w;
	}
	if (src_h < 0) {
		flip |= SDL_FLIP_VERTICAL;
		src_h = -src_h;
	}

	SDL_GPUBlitRegion src_region = {
		.texture = src_texture,
		.x = (Uint32)src_x,
		.y = (Uint32)src_y,
		.w = (Uint32)src_w,
		.h = (Uint32)src_h
	};
	SDL_GPUBlitRegion dst_region = {
		.texture = dst_texture,
		.x = (Uint32)dst_x,
		.y = (Uint32)dst_y,
		.w = (Uint32)dst_w,
		.h = (Uint32)dst_h
	};

	SDL_BlitGPUTexture(app->cmd, &src_region, &dst_region, (SDL_FlipMode)flip, SDL_GPU_FILTER_NEAREST, SDL_FALSE);
}

CF_Mesh cf_make_mesh(int vertex_buffer_size, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)CF_CALLOC(sizeof(CF_MeshInternal));
	mesh->vertices.size = vertex_buffer_size;
	if (vertex_buffer_size) {
		SDL_GPUBufferCreateInfo buf_info = {
			.usageFlags = SDL_GPU_BUFFERUSAGE_VERTEX_BIT,
			.sizeInBytes = (Uint32)vertex_buffer_size,
			.props = 0,
		};
		mesh->vertices.buffer = SDL_CreateGPUBuffer(app->device, &buf_info);
	}
	SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.sizeInBytes = (Uint32)vertex_buffer_size,
		.props = 0,
	};
	mesh->transfer_buffer = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
	attribute_count = min(attribute_count, CF_MESH_MAX_VERTEX_ATTRIBUTES);
	mesh->attribute_count = attribute_count;
	mesh->vertices.stride = vertex_stride;
	for (int i = 0; i < attribute_count; ++i) {
		mesh->attributes[i] = attributes[i];
		mesh->attributes[i].name = sintern(attributes[i].name);
	}
	CF_Mesh result = { (uint64_t)mesh };
	return result;
}

void cf_mesh_set_index_buffer(CF_Mesh mesh_handle, int index_buffer_size_in_bytes, int index_bit_count)
{
	CF_ASSERT(index_bit_count == 16 || index_bit_count == 32);
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	mesh->indices.size = index_buffer_size_in_bytes;
	mesh->indices.stride = index_bit_count / 8;
	SDL_GPUBufferCreateInfo buf_info = {
		.usageFlags = SDL_GPU_BUFFERUSAGE_INDEX_BIT,
		.sizeInBytes = (Uint32)index_buffer_size_in_bytes,
		.props = 0,
	};
	mesh->indices.buffer = SDL_CreateGPUBuffer(app->device, &buf_info);
	SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.sizeInBytes = (Uint32)index_buffer_size_in_bytes,
		.props = 0,
	};
}

void cf_mesh_set_instance_buffer(CF_Mesh mesh_handle, int instance_buffer_size_in_bytes, int instance_stride)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	mesh->instances.size = instance_buffer_size_in_bytes;
	mesh->instances.stride = instance_stride;
	SDL_GPUBufferCreateInfo buf_info = {
		.usageFlags = SDL_GPU_BUFFERUSAGE_VERTEX_BIT,
		.sizeInBytes = (Uint32)instance_buffer_size_in_bytes,
		.props = 0,
	};
	mesh->instances.buffer = SDL_CreateGPUBuffer(app->device, &buf_info);
	SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.sizeInBytes = (Uint32)instance_buffer_size_in_bytes,
		.props = 0,
	};
}

void cf_destroy_mesh(CF_Mesh mesh_handle)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	if (mesh->vertices.buffer) {
		SDL_ReleaseGPUBuffer(app->device, mesh->vertices.buffer);
	}
	if (mesh->indices.buffer) {
		SDL_ReleaseGPUBuffer(app->device, mesh->indices.buffer);
	}
	if (mesh->instances.buffer) {
		SDL_ReleaseGPUBuffer(app->device, mesh->instances.buffer);
	}
	SDL_ReleaseGPUTransferBuffer(app->device, mesh->transfer_buffer);
	CF_FREE(mesh);
}

static void s_update_buffer(SDL_GPUTransferBuffer* transfer_buffer, CF_Buffer* buffer, int element_count, void* data, int size, SDL_GPUBufferUsageFlags flags)
{
	// Resize buffer if necessary.
	if (size > buffer->size) {
		SDL_ReleaseGPUBuffer(app->device, buffer->buffer);

		int new_size = size * 2;
		buffer->size = new_size;
		SDL_GPUBufferCreateInfo buf_info = {
				.usageFlags = flags,
				.sizeInBytes = (Uint32)new_size,
				.props = 0,
		};
		buffer->buffer = SDL_CreateGPUBuffer(app->device, &buf_info);
	}

	// Copy vertices over to the driver.
	CF_ASSERT(size <= buffer->size);
	void* p = SDL_MapGPUTransferBuffer(app->device, transfer_buffer, true);
	CF_MEMCPY(p, data, size);
	SDL_UnmapGPUTransferBuffer(app->device, transfer_buffer);
	buffer->element_count = element_count;

	// Submit the upload command to the GPU.
	SDL_GPUCommandBuffer* cmd = app->cmd ? app->cmd : SDL_AcquireGPUCommandBuffer(app->device);
	SDL_GPUCopyPass *pass = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTransferBufferLocation location;
	location.offset = 0;
	location.transferBuffer = transfer_buffer;
	SDL_GPUBufferRegion region;
	region.buffer = buffer->buffer;
	region.offset = 0;
	region.size = size;
	SDL_UploadToGPUBuffer(pass, &location, &region, true);
	SDL_EndGPUCopyPass(pass);
	if (!app->cmd) SDL_SubmitGPUCommandBuffer(cmd);
}

void cf_mesh_update_vertex_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	CF_ASSERT(mesh->attribute_count);
	s_update_buffer(mesh->transfer_buffer, &mesh->vertices, count, data, count * mesh->vertices.stride, SDL_GPU_BUFFERUSAGE_VERTEX_BIT);
}

void cf_mesh_update_index_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_update_buffer(mesh->transfer_buffer, &mesh->indices, count, data, count * mesh->indices.stride, SDL_GPU_BUFFERUSAGE_INDEX_BIT);
}

void cf_mesh_update_instance_data(CF_Mesh mesh_handle, void* data, int count)
{
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_update_buffer(mesh->transfer_buffer, &mesh->instances, count, data, count * mesh->instances.stride, SDL_GPU_BUFFERUSAGE_VERTEX_BIT);
}

CF_RenderState cf_render_state_defaults()
{
	CF_RenderState state;
	state.blend.enabled = true;
	state.cull_mode = CF_CULL_MODE_NONE;
	state.blend.pixel_format = PIXEL_FORMAT_R8G8B8A8_UNORM;
	state.blend.write_R_enabled = true;
	state.blend.write_G_enabled = true;
	state.blend.write_B_enabled = true;
	state.blend.write_A_enabled = true;
	state.blend.rgb_op = CF_BLEND_OP_ADD;
	state.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	state.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ZERO;
	state.blend.alpha_op = CF_BLEND_OP_ADD;
	state.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	state.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ZERO;
	state.depth_compare = CF_COMPARE_FUNCTION_ALWAYS;
	state.depth_write_enabled = false;
	state.stencil.enabled = false;
	state.stencil.read_mask = 0;
	state.stencil.write_mask = 0;
	state.stencil.reference = 0;
	state.stencil.front.compare = CF_COMPARE_FUNCTION_ALWAYS;
	state.stencil.front.fail_op = CF_STENCIL_OP_KEEP;
	state.stencil.front.depth_fail_op = CF_STENCIL_OP_KEEP;
	state.stencil.front.pass_op = CF_STENCIL_OP_KEEP;
	state.stencil.back.compare = CF_COMPARE_FUNCTION_ALWAYS;
	state.stencil.back.fail_op = CF_STENCIL_OP_KEEP;
	state.stencil.back.depth_fail_op = CF_STENCIL_OP_KEEP;
	state.stencil.back.pass_op = CF_STENCIL_OP_KEEP;
	return state;
}

CF_Material cf_make_material()
{
	CF_MaterialInternal* material = CF_NEW(CF_MaterialInternal);
	cf_arena_init(&material->uniform_arena, 4, 1024);
	cf_arena_init(&material->block_arena, 4, 1024);
	material->state = cf_render_state_defaults();
	CF_Material result = { (uint64_t)material };
	return result;
}

void cf_destroy_material(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	cf_arena_reset(&material->uniform_arena);
	cf_arena_reset(&material->block_arena);
	material->~CF_MaterialInternal();
	CF_FREE(material);
}

void cf_material_set_render_state(CF_Material material_handle, CF_RenderState render_state)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	if (CF_MEMCMP(&material->state, &render_state, sizeof(material->state))) {
		material->state = render_state;
		material->dirty = true;
	}
}

static void s_material_set_texture(CF_MaterialInternal* material, CF_MaterialState* state, const char* name, CF_Texture texture)
{
	bool found = false;
	for (int i = 0; i < state->textures.count(); ++i) {
		if (state->textures[i].name == name) {
			state->textures[i].handle = texture;
			found = true;
			break;
		}
	}
	if (!found) {
		CF_MaterialTex tex;
		tex.name = name;
		tex.handle = texture;
		state->textures.add(tex);
		material->dirty = true;
	}
}

void cf_material_set_texture_vs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_texture(material, &material->vs, name, texture);
}

void cf_material_set_texture_fs(CF_Material material_handle, const char* name, CF_Texture texture)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_texture(material, &material->fs, name, texture);
}

void cf_material_clear_textures(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	material->vs.textures.clear();
	material->fs.textures.clear();
	material->dirty = true;
}

static void s_material_set_uniform(CF_Arena* arena, CF_MaterialState* state, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	if (array_length <= 0) array_length = 1;
	CF_Uniform* uniform = NULL;
	for (int i = 0; i < state->uniforms.count(); ++i) {
		CF_Uniform* u = state->uniforms + i;
		if (u->block_name == block_name && u->name == name) {
			uniform = u;
			break;
		}
	}
	int size = s_uniform_size(type) * array_length;
	if (!uniform) {
		uniform = &state->uniforms.add();
		uniform->name = name;
		uniform->block_name = block_name;
		uniform->data = cf_arena_alloc(arena, size);
		uniform->size = size;
		uniform->type = type;
		uniform->array_length = array_length;
	}
	CF_ASSERT(uniform->type == type);
	CF_ASSERT(uniform->array_length == array_length);
	CF_MEMCPY(uniform->data, data, size);
}

void cf_material_set_uniform_vs(CF_Material material_handle, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->vs, sintern("uniform_block"), name, data, type, array_length);
}

void cf_material_set_uniform_vs_internal(CF_Material material_handle, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->vs, sintern(block_name), name, data, type, array_length);
}

void cf_material_set_uniform_fs(CF_Material material_handle, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->fs, sintern("uniform_block"), name, data, type, array_length);
}

void cf_material_set_uniform_fs_internal(CF_Material material_handle, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	name = sintern(name);
	s_material_set_uniform(&material->uniform_arena, &material->fs, sintern(block_name), name, data, type, array_length);
}

void cf_material_clear_uniforms(CF_Material material_handle)
{
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	arena_reset(&material->uniform_arena);
	material->vs.uniforms.clear();
	material->fs.uniforms.clear();
}

void cf_clear_color(float red, float green, float blue, float alpha)
{
	app->clear_color = make_color(red, green, blue, alpha);
}

void cf_clear_depth_stencil(float depth, uint32_t stencil)
{
	app->clear_depth = depth;
	app->clear_stencil = stencil;
}

void cf_apply_canvas(CF_Canvas canvas_handle, bool clear)
{
	CF_CanvasInternal* canvas = (CF_CanvasInternal*)canvas_handle.id;
	CF_ASSERT(canvas);
	s_canvas = canvas;
	s_canvas->clear = clear;
}

void cf_apply_viewport(int x, int y, int w, int h)
{
	CF_ASSERT(s_canvas);
	CF_ASSERT(s_canvas->pass);
	SDL_GPUViewport viewport;
	viewport.x = (float)x;
	viewport.y = (float)y;
	viewport.w = (float)w;
	viewport.h = (float)h;
	viewport.minDepth = 0;
	viewport.maxDepth = 1;
	SDL_SetGPUViewport(s_canvas->pass, &viewport);
}

void cf_apply_scissor(int x, int y, int w, int h)
{
	CF_ASSERT(s_canvas);
	CF_ASSERT(s_canvas->pass);
	SDL_Rect scissor;
	scissor.x = x;
	scissor.y = y;
	scissor.w = w;
	scissor.h = h;
	SDL_SetGPUScissor(s_canvas->pass, &scissor);
}

void cf_apply_mesh(CF_Mesh mesh_handle)
{
	CF_ASSERT(s_canvas);
	CF_MeshInternal* mesh = (CF_MeshInternal*)mesh_handle.id;
	s_canvas->mesh = mesh;
}

static void s_copy_uniforms(SDL_GPUCommandBuffer* cmd, CF_Arena* arena, CF_ShaderInternal* shd, CF_MaterialState* mstate, bool vs)
{
	// Create any required uniform blocks for all uniforms matching between which uniforms
	// the material has and the shader needs.
	void* ub_ptrs[CF_MAX_UNIFORM_BLOCK_COUNT] = { };
	int ub_sizes[CF_MAX_UNIFORM_BLOCK_COUNT] = { };
	for (int block_index = 0; block_index < shd->uniform_block_count; ++block_index) {
		for (int i = 0; i < mstate->uniforms.count(); ++i) {
			CF_Uniform uniform = mstate->uniforms[i];
			int idx = vs ? shd->vs_index(uniform.name, block_index) : shd->fs_index(uniform.name, block_index);
			if (idx >= 0) {
				if (!ub_ptrs[block_index]) {
					// Create temporary space for a uniform block.
					int size = vs ? shd->vs_block_sizes[block_index] : shd->fs_block_sizes[block_index];
					void* block = cf_arena_alloc(arena, size);
					CF_MEMSET(block, 0, size);
					ub_ptrs[block_index] = block;
					ub_sizes[block_index] = size;
				}

				// Copy in the uniform's value into the block.
				int offset = vs ? shd->vs_uniform_block_members[block_index][idx].offset : shd->fs_uniform_block_members[block_index][idx].offset;
				void* block = ub_ptrs[block_index];
				void* dst = (void*)(((uintptr_t)block) + offset);
				CF_MEMCPY(dst, uniform.data, uniform.size);
			}
		}
	}

	// Send uniform data to the GPU.
	for (int i = 0; i < CF_MAX_UNIFORM_BLOCK_COUNT; ++i) {
		if (ub_ptrs[i]) {
			void* block = ub_ptrs[i];
			int size = ub_sizes[i];
			if (vs) {
				SDL_PushGPUVertexUniformData(cmd, i, block, (uint32_t)size);
			} else {
				SDL_PushGPUFragmentUniformData(cmd, i, block, (uint32_t)size);
			}
		}
	}

	// @TODO Use a different allocation scheme that caches better.
	cf_arena_reset(arena);
}

static SDL_GPUGraphicsPipeline* s_build_pipeline(CF_ShaderInternal* shader, CF_RenderState* state, CF_MeshInternal* mesh)
{
	SDL_GPUColorAttachmentDescription color_info;
	CF_MEMSET(&color_info, 0, sizeof(color_info));
	CF_ASSERT(s_canvas->texture);
	color_info.format = ((CF_TextureInternal*)s_canvas->cf_texture.id)->format;
	color_info.blendState.blendEnable = state->blend.enabled;
	color_info.blendState.alphaBlendOp = s_wrap(state->blend.alpha_op);
	color_info.blendState.colorBlendOp = s_wrap(state->blend.rgb_op);
	color_info.blendState.srcColorBlendFactor = s_wrap(state->blend.rgb_src_blend_factor);
	color_info.blendState.srcAlphaBlendFactor = s_wrap(state->blend.alpha_src_blend_factor);
	color_info.blendState.dstColorBlendFactor = s_wrap(state->blend.rgb_dst_blend_factor);
	color_info.blendState.dstAlphaBlendFactor = s_wrap(state->blend.alpha_dst_blend_factor);
	int mask_r = (int)state->blend.write_R_enabled << 0;
	int mask_g = (int)state->blend.write_G_enabled << 1;
	int mask_b = (int)state->blend.write_B_enabled << 2;
	int mask_a = (int)state->blend.write_A_enabled << 3;
	color_info.blendState.colorWriteMask = (uint32_t)(mask_r | mask_g | mask_b | mask_a);

	SDL_GPUGraphicsPipelineCreateInfo pip_info;
	CF_MEMSET(&pip_info, 0, sizeof(pip_info));
	pip_info.attachmentInfo.colorAttachmentCount = 1;
	pip_info.attachmentInfo.colorAttachmentDescriptions = &color_info;
	pip_info.vertexShader = shader->vs;
	pip_info.fragmentShader = shader->fs;
	pip_info.attachmentInfo.hasDepthStencilAttachment = state->depth_write_enabled;
	if (s_canvas->cf_depth_stencil.id) {
		pip_info.attachmentInfo.depthStencilFormat = ((CF_TextureInternal*)s_canvas->cf_depth_stencil.id)->format;
		pip_info.attachmentInfo.hasDepthStencilAttachment = true;
	}

	// Make sure the mesh vertex format is fully compatible with the vertex shader inputs.
	bool has_vertex_data = mesh->vertices.buffer ? true : false;
	bool has_instance_data = mesh->instances.buffer ? true : false;
	SDL_GPUVertexAttribute* attributes = SDL_stack_alloc(SDL_GPUVertexAttribute, mesh->attribute_count);
	int attribute_count = 0;
	for (int i = 0; i < mesh->attribute_count; ++i) {
		SDL_GPUVertexAttribute* attr = attributes + attribute_count;
		int idx = shader->get_input_index(mesh->attributes[i].name);
		if (idx >= 0) {
			CF_ShaderInputFormat input_fmt = shader->input_formats[idx];
			CF_VertexFormat mesh_fmt = mesh->attributes[i].format;
			CF_ASSERT(s_is_compatible(input_fmt, mesh_fmt));
			if (has_vertex_data) {
				attr->binding = mesh->attributes[i].per_instance ? 1 : 0; // Index in `vertex_bindings` below.
			} else {
				attr->binding = 0;
			}
			attr->location = shader->input_locations[idx];
			attr->format = s_wrap(mesh->attributes[i].format);
			attr->offset = mesh->attributes[i].offset;
			++attribute_count;
		}
	}
	CF_ASSERT(attribute_count == shader->input_count);
	pip_info.vertexInputState.vertexAttributeCount = attribute_count;
	pip_info.vertexInputState.vertexAttributes = attributes;
	SDL_GPUVertexBinding vertex_bindings[2];
	int vertex_bindings_count = 0;
	if (has_vertex_data) {
		vertex_bindings[vertex_bindings_count].binding = 0;
		vertex_bindings[vertex_bindings_count].stride = mesh->vertices.stride;
		vertex_bindings[vertex_bindings_count].inputRate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
		vertex_bindings[vertex_bindings_count].instanceStepRate = 0;
		vertex_bindings_count++;
	}
	if (has_instance_data) {
		vertex_bindings[vertex_bindings_count].binding = 1;
		vertex_bindings[vertex_bindings_count].stride = mesh->instances.stride;
		vertex_bindings[vertex_bindings_count].inputRate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
		vertex_bindings[vertex_bindings_count].instanceStepRate = 1;
		vertex_bindings_count++;
	}
	pip_info.vertexInputState.vertexBindingCount = vertex_bindings_count;
	pip_info.vertexInputState.vertexBindings = vertex_bindings;

	pip_info.primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pip_info.rasterizerState.fillMode = SDL_GPU_FILLMODE_FILL;
	pip_info.rasterizerState.cullMode = s_wrap(state->cull_mode);
	pip_info.rasterizerState.frontFace = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
	pip_info.rasterizerState.depthBiasEnable = false;
	pip_info.rasterizerState.depthBiasConstantFactor = 0;
	pip_info.rasterizerState.depthBiasClamp = 0;
	pip_info.rasterizerState.depthBiasSlopeFactor = 0;
	pip_info.multisampleState.sampleCount = SDL_GPU_SAMPLECOUNT_1;
	pip_info.multisampleState.sampleMask = 0xFFFF;

	pip_info.depthStencilState.depthTestEnable = state->depth_write_enabled;
	pip_info.depthStencilState.depthWriteEnable = state->depth_write_enabled;
	pip_info.depthStencilState.compareOp = s_wrap(state->depth_compare);
	pip_info.depthStencilState.stencilTestEnable = state->stencil.enabled;
	pip_info.depthStencilState.backStencilState.failOp = s_wrap(state->stencil.back.fail_op);
	pip_info.depthStencilState.backStencilState.passOp = s_wrap(state->stencil.back.pass_op);
	pip_info.depthStencilState.backStencilState.depthFailOp = s_wrap(state->stencil.back.depth_fail_op);
	pip_info.depthStencilState.backStencilState.compareOp = s_wrap(state->stencil.back.compare);
	pip_info.depthStencilState.frontStencilState.failOp = s_wrap(state->stencil.front.fail_op);
	pip_info.depthStencilState.frontStencilState.passOp = s_wrap(state->stencil.front.pass_op);
	pip_info.depthStencilState.frontStencilState.depthFailOp = s_wrap(state->stencil.front.depth_fail_op);
	pip_info.depthStencilState.frontStencilState.compareOp = s_wrap(state->stencil.front.compare);
	pip_info.depthStencilState.compareMask = state->stencil.read_mask;
	pip_info.depthStencilState.writeMask = state->stencil.write_mask;
	pip_info.depthStencilState.reference = state->stencil.reference;

	SDL_GPUGraphicsPipeline* pip = SDL_CreateGPUGraphicsPipeline(app->device, &pip_info);
	CF_ASSERT(pip);
	return pip;
}

void cf_apply_shader(CF_Shader shader_handle, CF_Material material_handle)
{
	CF_ASSERT(s_canvas);
	CF_ASSERT(s_canvas->mesh);
	CF_MeshInternal* mesh = s_canvas->mesh;
	CF_MaterialInternal* material = (CF_MaterialInternal*)material_handle.id;
	CF_ShaderInternal* shader = (CF_ShaderInternal*)shader_handle.id;
	CF_RenderState* state = &material->state;

	// Cache the pipeline to avoid create/release each frame.
	// ...Build a new one if the material marks itself as dirty.
	SDL_GPUGraphicsPipeline* pip = NULL;
	bool found = false;
	for (int i = 0; i < shader->pip_cache.count(); ++i) {
		CF_Pipeline pip_cache = shader->pip_cache[i];
		if (pip_cache.material == material && pip_cache.mesh == mesh) {
			found = true;
			if (material->dirty) {
				material->dirty = false;
				pip = s_build_pipeline(shader, state, mesh);
				if (pip_cache.pip) {
					SDL_ReleaseGPUGraphicsPipeline(app->device, pip_cache.pip);
				}
				shader->pip_cache[i].pip = pip;
			} else {
				pip = pip_cache.pip;
			}
		}
	}
	if (!found) {
		pip = s_build_pipeline(shader, state, mesh);
		shader->pip_cache.add({ material, pip, mesh });
	}
	CF_ASSERT(pip);

	SDL_GPUCommandBuffer* cmd = app->cmd;
	CF_ASSERT(cmd);
	s_canvas->pip = pip;

	SDL_GPUColorAttachmentInfo pass_color_info;
	CF_MEMSET(&pass_color_info, 0, sizeof(pass_color_info));
	pass_color_info.texture = s_canvas->texture;
	pass_color_info.clearColor = { app->clear_color.r, app->clear_color.g, app->clear_color.b, app->clear_color.a };
	pass_color_info.loadOp = s_canvas->clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
	pass_color_info.storeOp = SDL_GPU_STOREOP_STORE;
	pass_color_info.cycle = s_canvas->clear ? true : false;
	SDL_GPUDepthStencilAttachmentInfo pass_depth_stencil_info;
	CF_MEMSET(&pass_depth_stencil_info, 0, sizeof(pass_depth_stencil_info));
	pass_depth_stencil_info.texture = s_canvas->depth_stencil;
	if (s_canvas->depth_stencil) {
		pass_depth_stencil_info.depthStencilClearValue.depth = app->clear_depth;
		pass_depth_stencil_info.depthStencilClearValue.stencil = app->clear_stencil;
		pass_depth_stencil_info.loadOp = s_canvas->clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
		pass_depth_stencil_info.storeOp = SDL_GPU_STOREOP_STORE;
		pass_depth_stencil_info.stencilLoadOp = s_canvas->clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
		pass_depth_stencil_info.stencilStoreOp = SDL_GPU_STOREOP_DONT_CARE;
		pass_depth_stencil_info.cycle = pass_color_info.cycle;
	}
	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &pass_color_info, 1, s_canvas->depth_stencil ? &pass_depth_stencil_info : NULL);
	CF_ASSERT(pass);
	s_canvas->pass = pass;
	SDL_BindGPUGraphicsPipeline(pass, pip);
	SDL_GPUBufferBinding bind[2];
	bind[0].buffer = mesh->vertices.buffer;
	bind[0].offset = 0;
	bind[1].buffer = mesh->instances.buffer;
	bind[1].offset = 0;
	SDL_BindGPUVertexBuffers(pass, 0, bind, mesh->instances.buffer ? 2 : 1);

	if (mesh->indices.buffer) {
		SDL_GPUBufferBinding index_bind = {
			.buffer = mesh->indices.buffer,
			.offset = 0
		};
		SDL_BindGPUIndexBuffer(pass, &index_bind, mesh->indices.stride == 2 ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT);
	}
	// @TODO Storage/compute.

	// Bind images to all their respective slots.
	int sampler_count = shader->image_names.count();
	SDL_GPUTextureSamplerBinding* sampler_bindings = SDL_stack_alloc(SDL_GPUTextureSamplerBinding, sampler_count);
	int found_image_count = 0;
	for (int i = 0; found_image_count < sampler_count && i < material->fs.textures.count(); ++i) {
		const char* image_name = material->fs.textures[i].name;
		for (int i = 0; i < shader->image_names.size(); ++i) {
			if (shader->image_names[i] == image_name) {
				sampler_bindings[found_image_count].sampler = ((CF_TextureInternal*)material->fs.textures[i].handle.id)->sampler;
				sampler_bindings[found_image_count].texture = ((CF_TextureInternal*)material->fs.textures[i].handle.id)->tex;
				found_image_count++;
			}
		}
	}
	CF_ASSERT(found_image_count == sampler_count);
	SDL_BindGPUFragmentSamplers(pass, 0, sampler_bindings, (Uint32)found_image_count);

	// Copy over uniform data.
	s_copy_uniforms(cmd, &material->block_arena, shader, &material->vs, true);
	s_copy_uniforms(cmd, &material->block_arena, shader, &material->fs, false);

	// Prevent the same canvas from clearing itself more than once.
	s_canvas->clear = false;
}

void cf_draw_elements()
{
	CF_MeshInternal* mesh = s_canvas->mesh;
	if (mesh->instances.buffer) {
		if (mesh->indices.buffer) {
			SDL_DrawGPUIndexedPrimitives(s_canvas->pass, mesh->indices.element_count, mesh->instances.element_count, 0, 0, 0);
		} else {
			SDL_DrawGPUPrimitives(s_canvas->pass, mesh->vertices.element_count, mesh->instances.element_count, 0, 0);
		}
	} else {
		if (mesh->indices.buffer) {
			SDL_DrawGPUIndexedPrimitives(s_canvas->pass, mesh->indices.element_count, 1, 0, 0, 0);
		} else {
			SDL_DrawGPUPrimitives(s_canvas->pass, mesh->vertices.element_count, 1, 0, 0);
		}
	}
	app->draw_call_count++;
}

void cf_commit()
{
	SDL_EndGPURenderPass(s_canvas->pass);
}

#include <SPIRV-Reflect/spirv_reflect.c>
