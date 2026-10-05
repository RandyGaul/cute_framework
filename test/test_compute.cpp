/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Compute shaders, end to end on whichever backend the test app runs (SDL_GPU by default, the
// GLES3 backend's compute emulation with CF_TEST_GLES=1). Every case writes known values and
// checks them exactly against a CPU computation, so the same assertions hold on both backends.
// Results come back through rgba32f canvases used as storage images: a storage buffer has no
// readback, so buffer cases copy into an image with a second shader first.

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
#include <internal/cute_graphics_internal.h>
#include <stdio.h>
#include <string.h>

using namespace Cute;

// A failed REQUIRE returns early; the app still has to be released for the next case.
struct AppDestroyGuard
{
	~AppDestroyGuard() { test_destroy_app(); }
};

static bool s_is_gles() { return cf_query_backend() == CF_BACKEND_TYPE_GLES3; }

static CF_Canvas s_make_target(int w, int h)
{
	CF_CanvasParams p = cf_canvas_defaults(w, h);
	p.target.pixel_format = CF_PIXEL_FORMAT_R32G32B32A32_FLOAT;
	p.target.usage = CF_TEXTURE_USAGE_SAMPLER_BIT | CF_TEXTURE_USAGE_COLOR_TARGET_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_READ_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT;
	p.target.filter = CF_FILTER_NEAREST;
	CF_Canvas c = cf_make_canvas(p);
	cf_canvas_set_clear_color(c, cf_make_color_rgba_f(0, 0, 0, 0));
	cf_clear_canvas(c);
	return c;
}

// The canvas as floats, w * h * 4. False when the backend could not read it back.
static bool s_read(CF_Canvas c, int w, int h, float* out)
{
	// Dispatches record into the frame's command buffer; a readback submits its own.
	cf_gpu_sync();
	CF_Readback rb = cf_canvas_readback(c);
	if (!rb.id) return false;
	while (!cf_readback_ready(rb)) {}
	int n = cf_readback_data(rb, out, w * h * 4 * (int)sizeof(float));
	cf_destroy_readback(rb);
	return n == w * h * 4 * (int)sizeof(float);
}

static bool s_near(float a, float b) { return a == b; }

static bool s_expect_texel(const float* px, int w, int x, int y, float r, float g, float b, float a)
{
	const float* t = px + (y * w + x) * 4;
	if (s_near(t[0], r) && s_near(t[1], g) && s_near(t[2], b) && s_near(t[3], a)) return true;
	printf("texel (%d, %d) = (%g, %g, %g, %g), expected (%g, %g, %g, %g)\n", x, y, t[0], t[1], t[2], t[3], r, g, b, a);
	return false;
}

static void s_dispatch_image(CF_ComputeShader cs, CF_Material m, CF_Texture image, int gx, int gy, int gz, CF_StorageBuffer* ro, int ro_count, CF_StorageBuffer* rw, int rw_count)
{
	CF_ComputeDispatch d = cf_compute_dispatch_defaults(gx, gy, gz);
	d.rw_textures = &image;
	d.rw_texture_count = image.id ? 1 : 0;
	d.ro_buffers = ro;
	d.ro_buffer_count = ro_count;
	d.rw_buffers = rw;
	d.rw_buffer_count = rw_count;
	cf_dispatch_compute(cs, m, d);
}

//--------------------------------------------------------------------------------------------------
// Real compute GLSL, both backends.

// Copies `count` elements of a readonly buffer into an image, `w` texels per row: the readback
// path for every buffer case. One flavor per element type the cases write.
static const char* s_copy_uvec4 = R"(
layout (std430, set = 0, binding = 0) readonly buffer Src { uvec4 data[]; } u_src;
layout (set = 1, binding = 0, rgba32f) uniform writeonly image2D u_out;
layout (set = 2, binding = 0) uniform Params { int u_w; int u_count; };
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
	int i = int(gl_GlobalInvocationID.x);
	if (i >= u_count) return;
	imageStore(u_out, ivec2(i % u_w, i / u_w), vec4(u_src.data[i]));
}
)";

static const char* s_copy_uvec2 = R"(
layout (std430, set = 0, binding = 0) readonly buffer Src { uvec2 data[]; } u_src;
layout (set = 1, binding = 0, rgba32f) uniform writeonly image2D u_out;
layout (set = 2, binding = 0) uniform Params { int u_w; int u_count; };
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
	int i = int(gl_GlobalInvocationID.x);
	if (i >= u_count) return;
	uvec2 v = u_src.data[i];
	imageStore(u_out, ivec2(i % u_w, i / u_w), vec4(float(v.x), float(v.y), 0.0, 1.0));
}
)";

static const char* s_copy_float = R"(
layout (std430, set = 0, binding = 0) readonly buffer Src { float data[]; } u_src;
layout (set = 1, binding = 0, rgba32f) uniform writeonly image2D u_out;
layout (set = 2, binding = 0) uniform Params { int u_w; int u_count; };
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
	int i = int(gl_GlobalInvocationID.x);
	if (i >= u_count) return;
	imageStore(u_out, ivec2(i % u_w, i / u_w), vec4(u_src.data[i], 0.0, 0.0, 1.0));
}
)";

// Runs a copy shader over `buffer` into a fresh w x h image and reads it back.
static bool s_copy_out(const char* copy_src, CF_StorageBuffer buffer, int count, int w, int h, float* px)
{
	CF_ComputeShader copy = cf_make_compute_shader_from_source(copy_src);
	if (!copy.id) { printf("copy shader failed to compile\n"); return false; }
	CF_Canvas out = s_make_target(w, h);
	CF_Material m = cf_make_material();
	cf_material_set_uniform_cs(m, "u_w", &w, CF_UNIFORM_TYPE_INT, 1);
	cf_material_set_uniform_cs(m, "u_count", &count, CF_UNIFORM_TYPE_INT, 1);
	s_dispatch_image(copy, m, cf_canvas_get_target(out), (count + 63) / 64, 1, 1, &buffer, 1, NULL, 0);
	bool ok = s_read(out, w, h, px);
	cf_destroy_material(m);
	cf_destroy_compute_shader(copy);
	cf_destroy_canvas(out);
	return ok;
}

TEST_CASE(test_compute_image_own_coord)
{
	if (!test_make_app(64, 64)) return true;
	AppDestroyGuard app_guard;
	const char* src = R"(
layout (set = 1, binding = 0, rgba32f) uniform writeonly image2D u_out;
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main() {
	ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= 13 || p.y >= 11) return; // Early return: the rest stays as cleared.
	imageStore(u_out, p, vec4(float(p.x), float(p.y), float(p.x * 100 + p.y), 1.0));
}
)";
	CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
	REQUIRE(cs.id);
	CF_Canvas out = s_make_target(16, 16);
	CF_Material m = cf_make_material();
	s_dispatch_image(cs, m, cf_canvas_get_target(out), 2, 2, 1, NULL, 0, NULL, 0);
	float px[16 * 16 * 4];
	REQUIRE(s_read(out, 16, 16, px));
	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			bool in = x < 13 && y < 11;
			REQUIRE(s_expect_texel(px, 16, x, y, in ? (float)x : 0, in ? (float)y : 0, in ? (float)(x * 100 + y) : 0, in ? 1.0f : 0));
		}
	}
	cf_destroy_material(m);
	cf_destroy_compute_shader(cs);
	cf_destroy_canvas(out);
	return true;
}

TEST_CASE(test_compute_buffer_scatter)
{
	if (!test_make_app(64, 64)) return true;
	AppDestroyGuard app_guard;
	// Each invocation writes a permuted element: the destination is not the invocation's own.
	const char* src = R"(
layout (std430, set = 1, binding = 0) buffer Dst { uvec4 data[]; } u_dst;
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
	int i = int(gl_GlobalInvocationID.x);
	if (i >= 100) return;
	u_dst.data[(i * 37 + 11) % 100] = uvec4(uint(i), uint(i * 2), 7u, 9u);
}
)";
	CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
	REQUIRE(cs.id);
	CF_StorageBufferParams bp = cf_storage_buffer_defaults(100 * 16);
	bp.compute_writable = true;
	CF_StorageBuffer buf = cf_make_storage_buffer(bp);
	CF_Material m = cf_make_material();
	CF_Texture none = { 0 };
	s_dispatch_image(cs, m, none, 2, 1, 1, NULL, 0, &buf, 1);
	float px[10 * 10 * 4];
	REQUIRE(s_copy_out(s_copy_uvec4, buf, 100, 10, 10, px));
	for (int i = 0; i < 100; ++i) {
		int j = (i * 37 + 11) % 100;
		REQUIRE(s_expect_texel(px, 10, j % 10, j / 10, (float)i, (float)(i * 2), 7, 9));
	}
	cf_destroy_material(m);
	cf_destroy_compute_shader(cs);
	cf_destroy_storage_buffer(buf);
	return true;
}

TEST_CASE(test_compute_buffer_uvec2_and_float)
{
	if (!test_make_app(64, 64)) return true;
	AppDestroyGuard app_guard;
	// Elements narrower than a 16-byte texel: two uvec2 or four floats share one.
	const char* src2 = R"(
layout (std430, set = 1, binding = 0) buffer Dst { uvec2 data[]; } u_dst;
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
	uint i = gl_GlobalInvocationID.x;
	u_dst.data[i] = uvec2(i * 3u, i * 5u + 1u);
}
)";
	const char* srcf = R"(
layout (std430, set = 1, binding = 0) buffer Dst { float data[]; } u_dst;
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
	int i = int(gl_GlobalInvocationID.x);
	if (i >= 50) return;
	u_dst.data[49 - i] = float(i) * 0.5;
}
)";
	CF_ComputeShader cs2 = cf_make_compute_shader_from_source(src2);
	CF_ComputeShader csf = cf_make_compute_shader_from_source(srcf);
	REQUIRE(cs2.id && csf.id);
	CF_StorageBufferParams bp = cf_storage_buffer_defaults(64 * 8);
	bp.compute_writable = true;
	CF_StorageBuffer b2 = cf_make_storage_buffer(bp);
	CF_StorageBuffer bf = cf_make_storage_buffer(bp);
	CF_Material m = cf_make_material();
	CF_Texture none = { 0 };
	s_dispatch_image(cs2, m, none, 1, 1, 1, NULL, 0, &b2, 1);
	s_dispatch_image(csf, m, none, 1, 1, 1, NULL, 0, &bf, 1);
	float px[8 * 8 * 4];
	REQUIRE(s_copy_out(s_copy_uvec2, b2, 64, 8, 8, px));
	for (int i = 0; i < 64; ++i) REQUIRE(s_expect_texel(px, 8, i % 8, i / 8, (float)(i * 3), (float)(i * 5 + 1), 0, 1));
	REQUIRE(s_copy_out(s_copy_float, bf, 50, 8, 8, px));
	for (int i = 0; i < 50; ++i) REQUIRE(s_expect_texel(px, 8, (49 - i) % 8, (49 - i) / 8, (float)i * 0.5f, 0, 0, 1));
	cf_destroy_material(m);
	cf_destroy_compute_shader(cs2);
	cf_destroy_compute_shader(csf);
	cf_destroy_storage_buffer(b2);
	cf_destroy_storage_buffer(bf);
	return true;
}

TEST_CASE(test_compute_struct_buffer_read)
{
	if (!test_make_app(64, 64)) return true;
	AppDestroyGuard app_guard;
	// A readonly array of structs, std430: a at 0, b at 16, c at 24, stride 32.
	const char* src = R"(
struct Item { vec4 a; uvec2 b; float c; };
layout (std430, set = 0, binding = 0) readonly buffer Items { Item items[]; } u_items;
layout (set = 1, binding = 0, rgba32f) uniform writeonly image2D u_out;
layout (local_size_x = 8, local_size_y = 1, local_size_z = 1) in;
void main() {
	int i = int(gl_GlobalInvocationID.x);
	Item it = u_items.items[i];
	imageStore(u_out, ivec2(i, 0), vec4(it.a.x + it.a.w, float(it.b.x), float(it.b.y), it.c));
}
)";
	CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
	REQUIRE(cs.id);
	struct Item { float a[4]; uint32_t b[2]; float c; float pad; };
	Item items[8];
	for (int i = 0; i < 8; ++i) {
		items[i].a[0] = (float)i; items[i].a[1] = 0; items[i].a[2] = 0; items[i].a[3] = 100.0f;
		items[i].b[0] = (uint32_t)(i * 7); items[i].b[1] = (uint32_t)(i + 1000);
		items[i].c = 0.25f * (float)i;
		items[i].pad = 0;
	}
	CF_StorageBuffer buf = cf_make_storage_buffer(cf_storage_buffer_defaults(sizeof(items)));
	cf_update_storage_buffer(buf, items, sizeof(items));
	CF_Canvas out = s_make_target(8, 1);
	CF_Material m = cf_make_material();
	s_dispatch_image(cs, m, cf_canvas_get_target(out), 1, 1, 1, &buf, 1, NULL, 0);
	float px[8 * 4];
	REQUIRE(s_read(out, 8, 1, px));
	for (int i = 0; i < 8; ++i) REQUIRE(s_expect_texel(px, 8, i, 0, (float)i + 100.0f, (float)(i * 7), (float)(i + 1000), 0.25f * (float)i));
	cf_destroy_material(m);
	cf_destroy_compute_shader(cs);
	cf_destroy_storage_buffer(buf);
	cf_destroy_canvas(out);
	return true;
}

TEST_CASE(test_compute_image_read_modify_write)
{
	if (!test_make_app(64, 64)) return true;
	AppDestroyGuard app_guard;
	// imageLoad and imageStore of the same image: every invocation sees the old value of its texel.
	const char* fill = R"(
layout (set = 1, binding = 0, rgba32f) uniform writeonly image2D u_img;
layout (local_size_x = 4, local_size_y = 4, local_size_z = 1) in;
void main() {
	ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	imageStore(u_img, p, vec4(float(p.x), float(p.y), 1.0, 2.0));
}
)";
	const char* rmw = R"(
layout (set = 1, binding = 0, rgba32f) uniform image2D u_img;
layout (local_size_x = 4, local_size_y = 4, local_size_z = 1) in;
void main() {
	ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	vec4 v = imageLoad(u_img, p);
	imageStore(u_img, p, v * 2.0 + 1.0);
}
)";
	CF_ComputeShader a = cf_make_compute_shader_from_source(fill);
	CF_ComputeShader b = cf_make_compute_shader_from_source(rmw);
	REQUIRE(a.id && b.id);
	CF_Canvas out = s_make_target(8, 8);
	CF_Material m = cf_make_material();
	s_dispatch_image(a, m, cf_canvas_get_target(out), 2, 2, 1, NULL, 0, NULL, 0);
	s_dispatch_image(b, m, cf_canvas_get_target(out), 2, 2, 1, NULL, 0, NULL, 0);
	float px[8 * 8 * 4];
	REQUIRE(s_read(out, 8, 8, px));
	for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x) REQUIRE(s_expect_texel(px, 8, x, y, (float)(x * 2 + 1), (float)(y * 2 + 1), 3, 5));
	cf_destroy_material(m);
	cf_destroy_compute_shader(a);
	cf_destroy_compute_shader(b);
	cf_destroy_canvas(out);
	return true;
}

TEST_CASE(test_compute_uniforms_and_texture)
{
	if (!test_make_app(64, 64)) return true;
	AppDestroyGuard app_guard;
	// Uniforms and a sampled texture feed the result.
	const char* src = R"(
layout (set = 0, binding = 0) uniform sampler2D u_tex;
layout (set = 1, binding = 0, rgba32f) uniform writeonly image2D u_out;
layout (set = 2, binding = 0) uniform Params { int u_limit; float u_scale; };
layout (local_size_x = 4, local_size_y = 4, local_size_z = 1) in;
void main() {
	ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= u_limit) return;
	vec4 t = texelFetch(u_tex, ivec2(p.x & 1, p.y & 1), 0);
	imageStore(u_out, p, t * u_scale);
}
)";
	CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
	REQUIRE(cs.id);
	CF_TextureParams tp = cf_texture_defaults(2, 2);
	tp.filter = CF_FILTER_NEAREST;
	CF_Texture tex = cf_make_texture(tp);
	CF_Pixel pix[4] = { cf_make_pixel_rgba(255, 0, 0, 255), cf_make_pixel_rgba(0, 255, 0, 255), cf_make_pixel_rgba(0, 0, 255, 255), cf_make_pixel_rgba(255, 255, 255, 0) };
	cf_texture_update(tex, pix, sizeof(pix));
	CF_Canvas out = s_make_target(8, 8);
	CF_Material m = cf_make_material();
	int limit = 6;
	float scale = 4.0f;
	cf_material_set_uniform_cs(m, "u_limit", &limit, CF_UNIFORM_TYPE_INT, 1);
	cf_material_set_uniform_cs(m, "u_scale", &scale, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_texture_cs(m, "u_tex", tex);
	s_dispatch_image(cs, m, cf_canvas_get_target(out), 2, 2, 1, NULL, 0, NULL, 0);
	float px[8 * 8 * 4];
	REQUIRE(s_read(out, 8, 8, px));
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			if (x >= limit) { REQUIRE(s_expect_texel(px, 8, x, y, 0, 0, 0, 0)); continue; }
			CF_Pixel c = pix[(y & 1) * 2 + (x & 1)];
			REQUIRE(s_expect_texel(px, 8, x, y, c.colors.r / 255.0f * 4, c.colors.g / 255.0f * 4, c.colors.b / 255.0f * 4, c.colors.a / 255.0f * 4));
		}
	}
	cf_destroy_material(m);
	cf_destroy_compute_shader(cs);
	cf_destroy_texture(tex);
	cf_destroy_canvas(out);
	return true;
}

TEST_CASE(test_compute_two_sites_and_builtins)
{
	if (!test_make_app(64, 64)) return true;
	AppDestroyGuard app_guard;
	// Two write sites (an image and a buffer) and the workgroup builtins over a 3D dispatch.
	// gl_NumWorkGroups comes in as a uniform: the HLSL emitter has no gl_NumWorkGroups.
	const char* src = R"(
layout (set = 1, binding = 0, rgba32f) uniform writeonly image2D u_out;
layout (std430, set = 1, binding = 1) buffer Dst { uvec4 data[]; } u_dst;
layout (set = 2, binding = 0) uniform Params { ivec4 u_groups; };
layout (local_size_x = 4, local_size_y = 2, local_size_z = 1) in;
void main() {
	uvec3 g = gl_GlobalInvocationID;
	uvec3 ext = uvec3(u_groups.xyz) * uvec3(4u, 2u, 1u);
	uint i = g.x + ext.x * (g.y + ext.y * g.z);
	imageStore(u_out, ivec2(int(i % 12u), int(i / 12u)), vec4(float(g.x), float(g.y), float(g.z), 1.0));
	u_dst.data[i] = uvec4(gl_WorkGroupID.x + 10u * gl_WorkGroupID.y + 100u * gl_WorkGroupID.z,
		gl_LocalInvocationID.x + 10u * gl_LocalInvocationID.y,
		gl_LocalInvocationIndex,
		gl_GlobalInvocationID.x + 10u * gl_GlobalInvocationID.y + 100u * gl_GlobalInvocationID.z);
}
)";
	CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
	REQUIRE(cs.id);
	const int GX = 3, GY = 2, GZ = 2, LX = 4, LY = 2;
	const int EX = GX * LX, EY = GY * LY, N = EX * EY * GZ; // 96 invocations.
	CF_StorageBufferParams bp = cf_storage_buffer_defaults(N * 16);
	bp.compute_writable = true;
	CF_StorageBuffer buf = cf_make_storage_buffer(bp);
	CF_Canvas out = s_make_target(12, 8);
	CF_Material m = cf_make_material();
	int groups[4] = { GX, GY, GZ, 0 };
	cf_material_set_uniform_cs(m, "u_groups", groups, CF_UNIFORM_TYPE_INT4, 1);
	s_dispatch_image(cs, m, cf_canvas_get_target(out), GX, GY, GZ, NULL, 0, &buf, 1);
	float img[12 * 8 * 4];
	REQUIRE(s_read(out, 12, 8, img));
	float px[12 * 8 * 4];
	REQUIRE(s_copy_out(s_copy_uvec4, buf, N, 12, 8, px));
	for (int z = 0; z < GZ; ++z) for (int y = 0; y < EY; ++y) for (int x = 0; x < EX; ++x) {
		int i = x + EX * (y + EY * z);
		REQUIRE(s_expect_texel(img, 12, i % 12, i / 12, (float)x, (float)y, (float)z, 1));
		int wg = (x / LX) + 10 * (y / LY) + 100 * z;
		int lid = (x % LX) + 10 * (y % LY);
		int lidx = (x % LX) + LX * (y % LY);
		REQUIRE(s_expect_texel(px, 12, i % 12, i / 12, (float)wg, (float)lid, (float)lidx, (float)(x + 10 * y + 100 * z)));
	}
	cf_destroy_material(m);
	cf_destroy_compute_shader(cs);
	cf_destroy_storage_buffer(buf);
	cf_destroy_canvas(out);
	return true;
}

TEST_CASE(test_compute_large_partial_grid)
{
	if (!test_make_app(64, 64)) return true;
	AppDestroyGuard app_guard;
	// More invocations than one grid row holds, and a count that is not a multiple of it.
	const char* src = R"(
layout (std430, set = 1, binding = 0) buffer Dst { uvec4 data[]; } u_dst;
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= 5000u) return;
	u_dst.data[i] = uvec4(i, i ^ 0x55u, i / 3u, 1u);
}
)";
	CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
	REQUIRE(cs.id);
	CF_StorageBufferParams bp = cf_storage_buffer_defaults(5000 * 16);
	bp.compute_writable = true;
	CF_StorageBuffer buf = cf_make_storage_buffer(bp);
	CF_Material m = cf_make_material();
	CF_Texture none = { 0 };
	s_dispatch_image(cs, m, none, (5000 + 63) / 64, 1, 1, NULL, 0, &buf, 1);
	static float px[100 * 50 * 4];
	REQUIRE(s_copy_out(s_copy_uvec4, buf, 5000, 100, 50, px));
	for (int i = 0; i < 5000; ++i) REQUIRE(s_expect_texel(px, 100, i % 100, i / 100, (float)i, (float)(i ^ 0x55), (float)(i / 3), 1));
	cf_destroy_material(m);
	cf_destroy_compute_shader(cs);
	cf_destroy_storage_buffer(buf);
	return true;
}

TEST_CASE(test_compute_gles_rejects)
{
	if (!test_make_app(64, 64)) return true;
	AppDestroyGuard app_guard;
	if (!s_is_gles()) return true;
	// Outside the emulated class: a clear compile failure, not a wrong result.
	const char* src = R"(
layout (std430, set = 1, binding = 0) buffer Dst { uint data[]; } u_dst;
shared uint s_tmp[64];
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
	s_tmp[gl_LocalInvocationIndex] = gl_LocalInvocationIndex;
	barrier();
	u_dst.data[gl_GlobalInvocationID.x] = s_tmp[63u - gl_LocalInvocationIndex];
}
)";
	CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
	REQUIRE(cs.id == 0);
	return true;
}

// Storage-texture support is reported per format, and a texture the device cannot use for
// storage is refused at creation instead of coming back as a handle that fails validation.
TEST_CASE(test_compute_storage_format_support)
{
	if (!test_make_app(64, 64)) return true;
	AppDestroyGuard app_guard;
	if (cf_query_backend() != CF_BACKEND_TYPE_WEBGPU) return true;
	CF_TextureUsageBits usage = (CF_TextureUsageBits)(CF_TEXTURE_USAGE_SAMPLER_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT);
	CF_PixelFormat formats[] = {
		CF_PIXEL_FORMAT_R8G8B8A8_UNORM, CF_PIXEL_FORMAT_R32_FLOAT, CF_PIXEL_FORMAT_R32G32B32A32_FLOAT,
		CF_PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB, CF_PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB, CF_PIXEL_FORMAT_D32_FLOAT,
		CF_PIXEL_FORMAT_D24_UNORM_S8_UINT, CF_PIXEL_FORMAT_BC1_RGBA_UNORM,
		CF_PIXEL_FORMAT_R16_FLOAT, CF_PIXEL_FORMAT_R16G16_FLOAT, CF_PIXEL_FORMAT_R8_UNORM,
	};
	for (int i = 0; i < (int)(sizeof(formats) / sizeof(formats[0])); ++i) {
		bool supported = cf_texture_supports_format(formats[i], usage);
		CF_TextureParams tp = cf_texture_defaults(4, 4);
		tp.pixel_format = formats[i];
		tp.usage = usage;
		CF_Texture t = cf_make_texture(tp);
		REQUIRE(supported == (t.id != 0));
		if (t.id) cf_destroy_texture(t);
		bool always = i < 3;
		bool never = i >= 3 && i < 8;
		if (always) REQUIRE(supported);
		if (never) REQUIRE(!supported);
	}

	// What the query allows actually works: a written r16f/rg16f (Tier1 formats) reads back.
	CF_PixelFormat tier1[] = { CF_PIXEL_FORMAT_R16_FLOAT, CF_PIXEL_FORMAT_R16G16_FLOAT };
	const char* names[] = { "r16f", "rg16f" };
	for (int i = 0; i < 2; ++i) {
		if (!cf_texture_supports_format(tier1[i], usage)) continue;
		char src[512];
		snprintf(src, sizeof(src),
			"layout (set = 1, binding = 0, %s) uniform writeonly image2D u_img;\n"
			"layout (local_size_x = 4, local_size_y = 4, local_size_z = 1) in;\n"
			"void main() { imageStore(u_img, ivec2(gl_GlobalInvocationID.xy), vec4(0.5, 0.25, 0, 0)); }\n", names[i]);
		CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
		REQUIRE(cs.id);
		CF_CanvasParams p = cf_canvas_defaults(4, 4);
		p.target.pixel_format = tier1[i];
		p.target.usage = CF_TEXTURE_USAGE_COLOR_TARGET_BIT | usage;
		p.target.filter = CF_FILTER_NEAREST;
		CF_Canvas c = cf_make_canvas(p);
		REQUIRE(c.id);
		CF_Material m = cf_make_material();
		s_dispatch_image(cs, m, cf_canvas_get_target(c), 1, 1, 1, NULL, 0, NULL, 0);
		cf_gpu_sync();
		uint16_t halves[4 * 4 * 2] = { };
		CF_Readback rb = cf_canvas_readback(c);
		REQUIRE(rb.id);
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, halves, (int)sizeof(halves));
		cf_destroy_readback(rb);
		REQUIRE(halves[0] == 0x3800); // 0.5
		if (i == 1) REQUIRE(halves[1] == 0x3400); // 0.25
		cf_destroy_material(m);
		cf_destroy_canvas(c);
		cf_destroy_compute_shader(cs);
	}
	return true;
}

//--------------------------------------------------------------------------------------------------
// The GLES runtime alone, against hand-written capture shaders that follow cute_spirv's capture
// convention exactly: a regression here is the runtime's, never the transpiler's.

static const char* s_hand_header =
	"#version 300 es\n"
	"precision highp float; precision highp int; precision highp usampler2D; precision highp isampler2D; precision highp sampler2D;\n"
	"uniform int u_cspv_site;\n"
	"uniform ivec3 u_cspv_groups;\n"
	"uniform int u_cspv_grid_w;\n"
	"layout(location = 0) out highp uvec4 cspv_value;\n"
	"layout(location = 1) out highp ivec4 cspv_target;\n";

static CF_ComputeShader s_hand_shader(const char* body, int lx, int ly, CF_ShaderResourceInfo* images, int num_images, CF_ShaderResourceInfo* buffers, int num_buffers, CF_ShaderWriteSite* sites, int num_sites)
{
	static char src[8192];
	snprintf(src, sizeof(src), "%s%s", s_hand_header, body);
	CF_ShaderBytecode bc;
	memset(&bc, 0, sizeof(bc));
	static const uint8_t s_dummy = 0;
	bc.content = &s_dummy;
	bc.size = 1;
	bc.glsl300_src = src;
	bc.glsl300_src_size = strlen(src);
	bc.shader_info.local_size[0] = lx;
	bc.shader_info.local_size[1] = ly;
	bc.shader_info.local_size[2] = 1;
	bc.shader_info.num_storage_image_infos = num_images;
	bc.shader_info.storage_image_infos = images;
	bc.shader_info.num_storage_buffer_infos = num_buffers;
	bc.shader_info.storage_buffer_infos = buffers;
	bc.shader_info.num_write_sites = num_sites;
	bc.shader_info.write_sites = sites;
	return cf_make_compute_shader_from_bytecode(bc);
}

TEST_CASE(test_compute_gles_runtime_handwritten)
{
	if (!test_make_app(64, 64)) return true;
	AppDestroyGuard app_guard;
	if (!s_is_gles()) return true;

	// An imageStore at the invocation's own coordinate (local 8x8).
	CF_ShaderResourceInfo img_out = { "u_out", 1, 0, false };
	CF_ShaderWriteSite site_img = { CF_SHADER_WRITE_KIND_IMAGE, "u_out", 1, 0, 4 };
	CF_ComputeShader own = s_hand_shader(R"(
void main() {
	cspv_value = uvec4(0u); cspv_target = ivec4(0);
	int L = int(gl_FragCoord.y) * u_cspv_grid_w + int(gl_FragCoord.x);
	ivec3 ext = u_cspv_groups * ivec3(8, 8, 1);
	if (L >= ext.x * ext.y * ext.z) return;
	ivec2 p = ivec2(L % ext.x, (L / ext.x) % ext.y);
	if (p.x >= 13 || p.y >= 11) return;
	if (u_cspv_site == 0) { cspv_value = floatBitsToUint(vec4(float(p.x), float(p.y), float(p.x * 100 + p.y), 1.0)); cspv_target = ivec4(p, 0, 1); }
}
)", 8, 8, &img_out, 1, NULL, 0, &site_img, 1);
	REQUIRE(own.id);
	CF_Canvas out = s_make_target(16, 16);
	CF_Material m = cf_make_material();
	s_dispatch_image(own, m, cf_canvas_get_target(out), 2, 2, 1, NULL, 0, NULL, 0);
	float px[16 * 16 * 4];
	REQUIRE(s_read(out, 16, 16, px));
	for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) {
		bool in = x < 13 && y < 11;
		REQUIRE(s_expect_texel(px, 16, x, y, in ? (float)x : 0, in ? (float)y : 0, in ? (float)(x * 100 + y) : 0, in ? 1.0f : 0));
	}

	// A permuted uvec4 buffer write, and a uvec2 one (two first-component classes per texel).
	CF_ShaderResourceInfo buf_rw = { "u_dst", 1, 0, false };
	CF_ShaderWriteSite site_buf4 = { CF_SHADER_WRITE_KIND_BUFFER, "u_dst", 1, 0, 4 };
	CF_ComputeShader scatter = s_hand_shader(R"(
void main() {
	cspv_value = uvec4(0u); cspv_target = ivec4(0);
	int L = int(gl_FragCoord.y) * u_cspv_grid_w + int(gl_FragCoord.x);
	if (L >= u_cspv_groups.x * 64) return;
	int i = L;
	if (i >= 100) return;
	if (u_cspv_site == 0) { int w = ((i * 37 + 11) % 100) * 4; cspv_value = uvec4(uint(i), uint(i * 2), 7u, 9u); cspv_target = ivec4((w >> 2) & 1023, (w >> 2) >> 10, w & 3, 1); }
}
)", 64, 1, NULL, 0, &buf_rw, 1, &site_buf4, 1);
	CF_ShaderWriteSite site_buf2 = { CF_SHADER_WRITE_KIND_BUFFER, "u_dst", 1, 0, 2 };
	CF_ComputeShader pairs = s_hand_shader(R"(
void main() {
	cspv_value = uvec4(0u); cspv_target = ivec4(0);
	int L = int(gl_FragCoord.y) * u_cspv_grid_w + int(gl_FragCoord.x);
	if (L >= u_cspv_groups.x * 64) return;
	uint i = uint(L);
	if (u_cspv_site == 0) { int w = L * 2; int c = w & 3; uvec4 bits = uvec4(0u); bits[c] = i * 3u; bits[c + 1] = i * 5u + 1u; cspv_value = bits; cspv_target = ivec4((w >> 2) & 1023, (w >> 2) >> 10, c, 1); }
}
)", 64, 1, NULL, 0, &buf_rw, 1, &site_buf2, 1);
	REQUIRE(scatter.id && pairs.id);
	// The copy out: a readonly buffer read through the emulation's sampler, rank 0.
	CF_ShaderResourceInfo buf_ro = { "u_src", 0, 0, true };
	CF_ComputeShader copy4 = s_hand_shader(R"(
uniform highp usampler2D u_cs_storage_0;
void main() {
	cspv_value = uvec4(0u); cspv_target = ivec4(0);
	int L = int(gl_FragCoord.y) * u_cspv_grid_w + int(gl_FragCoord.x);
	if (L >= 100) return;
	uvec4 v = texelFetch(u_cs_storage_0, ivec2(L & 1023, L >> 10), 0);
	if (u_cspv_site == 0) { cspv_value = floatBitsToUint(vec4(v)); cspv_target = ivec4(L % 10, L / 10, 0, 1); }
}
)", 64, 1, &img_out, 1, &buf_ro, 1, &site_img, 1);
	CF_ComputeShader copy2 = s_hand_shader(R"(
uniform highp usampler2D u_cs_storage_0;
void main() {
	cspv_value = uvec4(0u); cspv_target = ivec4(0);
	int L = int(gl_FragCoord.y) * u_cspv_grid_w + int(gl_FragCoord.x);
	if (L >= 64) return;
	int w = L * 2;
	uvec4 t = texelFetch(u_cs_storage_0, ivec2((w >> 2) & 1023, (w >> 2) >> 10), 0);
	uint x = t[w & 3], y = t[(w & 3) + 1];
	if (u_cspv_site == 0) { cspv_value = floatBitsToUint(vec4(float(x), float(y), 0.0, 1.0)); cspv_target = ivec4(L % 8, L / 8, 0, 1); }
}
)", 64, 1, &img_out, 1, &buf_ro, 1, &site_img, 1);
	REQUIRE(copy4.id && copy2.id);

	CF_StorageBufferParams bp = cf_storage_buffer_defaults(100 * 16);
	bp.compute_writable = true;
	CF_StorageBuffer b4 = cf_make_storage_buffer(bp);
	CF_StorageBuffer b2 = cf_make_storage_buffer(bp);
	CF_Texture none = { 0 };
	s_dispatch_image(scatter, m, none, 2, 1, 1, NULL, 0, &b4, 1);
	s_dispatch_image(pairs, m, none, 1, 1, 1, NULL, 0, &b2, 1);

	CF_Canvas out4 = s_make_target(10, 10);
	s_dispatch_image(copy4, m, cf_canvas_get_target(out4), 2, 1, 1, &b4, 1, NULL, 0);
	float p4[10 * 10 * 4];
	REQUIRE(s_read(out4, 10, 10, p4));
	for (int i = 0; i < 100; ++i) {
		int j = (i * 37 + 11) % 100;
		REQUIRE(s_expect_texel(p4, 10, j % 10, j / 10, (float)i, (float)(i * 2), 7, 9));
	}
	CF_Canvas out2 = s_make_target(8, 8);
	s_dispatch_image(copy2, m, cf_canvas_get_target(out2), 1, 1, 1, &b2, 1, NULL, 0);
	float p2[8 * 8 * 4];
	REQUIRE(s_read(out2, 8, 8, p2));
	for (int i = 0; i < 64; ++i) REQUIRE(s_expect_texel(p2, 8, i % 8, i / 8, (float)(i * 3), (float)(i * 5 + 1), 0, 1));

	cf_destroy_material(m);
	cf_destroy_compute_shader(own);
	cf_destroy_compute_shader(scatter);
	cf_destroy_compute_shader(pairs);
	cf_destroy_compute_shader(copy4);
	cf_destroy_compute_shader(copy2);
	cf_destroy_storage_buffer(b4);
	cf_destroy_storage_buffer(b2);
	cf_destroy_canvas(out);
	cf_destroy_canvas(out4);
	cf_destroy_canvas(out2);
	return true;
}

TEST_SUITE(test_compute)
{
	RUN_TEST_CASE(test_compute_gles_runtime_handwritten);
	RUN_TEST_CASE(test_compute_image_own_coord);
	RUN_TEST_CASE(test_compute_buffer_scatter);
	RUN_TEST_CASE(test_compute_buffer_uvec2_and_float);
	RUN_TEST_CASE(test_compute_struct_buffer_read);
	RUN_TEST_CASE(test_compute_image_read_modify_write);
	RUN_TEST_CASE(test_compute_uniforms_and_texture);
	RUN_TEST_CASE(test_compute_two_sites_and_builtins);
	RUN_TEST_CASE(test_compute_large_partial_grid);
	RUN_TEST_CASE(test_compute_gles_rejects);
	RUN_TEST_CASE(test_compute_storage_format_support);
}
