/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// cute_dds.h end to end: DDS files are built byte-by-byte in memory (legacy and DX10
// headers, handcrafted BC blocks with known decode values), parsed, uploaded through
// cf_make_texture_from_dds_mem, and sampled back through real shaders -- so the parser,
// the format mapping, the compressed mip/layer uploads, AND the GPU's decode of our
// hand-encoded blocks all get verified by pixels. Plus the parser's rejection paths.

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>

// The engine carries its own copy of cute_dds inside cute_image.cpp, but shared (DLL)
// builds don't export the cd_* symbols -- and shouldn't. The test compiles a private
// copy with internal linkage instead, which doubles as proof the header stands alone.
#define CUTE_DDS_STATIC
#define CUTE_DDS_NO_STDIO
#define CUTE_DDS_IMPLEMENTATION
#include <cute/cute_dds.h>

using namespace Cute;

#define W 64
#define H 64

// ---------------------------------------------------------------------------
// DDS writers. Everything little-endian, matching the file format.

struct DDSBuilder
{
	Cute::Array<uint8_t> bytes;
	void u32(uint32_t v) { bytes.add((uint8_t)v); bytes.add((uint8_t)(v >> 8)); bytes.add((uint8_t)(v >> 16)); bytes.add((uint8_t)(v >> 24)); }
	void raw(const void* data, int size) { for (int i = 0; i < size; ++i) bytes.add(((const uint8_t*)data)[i]); }
};

// Legacy header. `fourcc` 0 means uncompressed RGBA8 masks instead.
static void s_header(DDSBuilder* b, int w, int h, int mips, uint32_t fourcc, uint32_t caps2)
{
	b->u32(0x20534444); // "DDS "
	b->u32(124);        // dwSize
	b->u32(0x1007 | (mips > 1 ? 0x20000 : 0)); // CAPS | HEIGHT | WIDTH | PIXELFORMAT | MIPMAPCOUNT
	b->u32((uint32_t)h);
	b->u32((uint32_t)w);
	b->u32(0);          // dwPitchOrLinearSize (advisory; the parser computes its own).
	b->u32(0);          // dwDepth
	b->u32((uint32_t)mips);
	for (int i = 0; i < 11; ++i) b->u32(0); // dwReserved1
	b->u32(32);         // ddspf.dwSize
	if (fourcc) {
		b->u32(0x4);    // DDPF_FOURCC
		b->u32(fourcc);
		b->u32(0); b->u32(0); b->u32(0); b->u32(0); b->u32(0);
	} else {
		b->u32(0x41);   // DDPF_RGB | DDPF_ALPHAPIXELS
		b->u32(0);
		b->u32(32);
		b->u32(0x000000ff); b->u32(0x0000ff00); b->u32(0x00ff0000); b->u32(0xff000000);
	}
	b->u32(0x1000);     // dwCaps: TEXTURE
	b->u32(caps2);
	b->u32(0); b->u32(0); b->u32(0); // caps3, caps4, reserved2
}

#define FOURCC(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#define CAPS2_CUBEMAP_ALL 0xfe00u // CUBEMAP | all six face bits.

static void s_dx10_header(DDSBuilder* b, int w, int h, int mips, uint32_t dxgi, uint32_t array_size)
{
	s_header(b, w, h, mips, FOURCC('D', 'X', '1', '0'), 0);
	b->u32(dxgi);
	b->u32(3); // DIMENSION_TEXTURE2D
	b->u32(0); // miscFlag
	b->u32(array_size);
	b->u32(0); // miscFlags2
}

// A solid-color BC1 block: both endpoints the same RGB565 color, all indices 0.
static void s_bc1_block(DDSBuilder* b, uint16_t rgb565)
{
	b->bytes.add((uint8_t)rgb565); b->bytes.add((uint8_t)(rgb565 >> 8));
	b->bytes.add((uint8_t)rgb565); b->bytes.add((uint8_t)(rgb565 >> 8));
	for (int i = 0; i < 4; ++i) b->bytes.add(0);
}

// A solid-value BC4-style alpha/channel block: both endpoints `v`, all indices 0.
static void s_bc4_block(DDSBuilder* b, uint8_t v)
{
	b->bytes.add(v); b->bytes.add(v);
	for (int i = 0; i < 6; ++i) b->bytes.add(0);
}

// Solid BC3 block: opaque interpolated alpha + a BC1 color block.
static void s_bc3_block(DDSBuilder* b, uint16_t rgb565)
{
	s_bc4_block(b, 255);
	s_bc1_block(b, rgb565);
}

// Solid opaque white BC7 block, mode 6: both endpoints all-ones (7-bit fields + P bits),
// all indices 0. Mode 6 is flagged by bit 6; endpoint bits span bits 7..62, P0/P1 at 63/64.
static void s_bc7_white_block(DDSBuilder* b)
{
	static const uint8_t block[16] = { 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0 };
	b->raw(block, 16);
}

#define RGB565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

// ---------------------------------------------------------------------------
// GPU sampling harness: fullscreen quad, sample the bound texture at a uv/lod,
// read the center pixel back.

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"void main() { gl_Position = vec4(in_pos, 0, 1); }\n";

static const char* s_fs =
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_tex;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_lod;\n"
"};\n"
"void main() { result = textureLod(u_tex, vec2(0.5, 0.5), u_lod.x); }\n";

static CF_Mesh s_make_fullscreen_quad()
{
	CF_V2 verts[6] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 } };
	CF_VertexAttribute attr = { };
	attr.name = "in_pos";
	attr.format = CF_VERTEX_FORMAT_FLOAT2;
	attr.offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), &attr, 1, sizeof(CF_V2));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	return mesh;
}

static CF_Pixel s_sample(CF_Canvas canvas, CF_Mesh mesh, CF_Shader shader, CF_Material material, CF_Texture tex, float lod, CF_Pixel* px)
{
	cf_app_update(NULL);
	cf_material_set_texture_fs(material, "u_tex", tex);
	CF_V4 lod4 = cf_v4(lod, 0, 0, 0);
	cf_material_set_uniform_fs(material, "u_lod", &lod4, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shader, material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);
	CF_Readback rb = cf_canvas_readback(canvas);
	if (!rb.id) return cf_make_pixel_rgba(0, 0, 0, 0);
	while (!cf_readback_ready(rb)) {}
	cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	return px[(H / 2) * W + (W / 2)];
}

#define NEAR(x, v) ((x) > (v) - 12 && (x) < (v) + 12)

// ---------------------------------------------------------------------------
// Parser-only coverage: formats, layout arithmetic, and every rejection path.
TEST_CASE(test_dds_parse)
{
	REQUIRE(cd_block_size(CD_FORMAT_BC1) == 8);
	REQUIRE(cd_block_size(CD_FORMAT_BC7) == 16);
	REQUIRE(cd_block_size(CD_FORMAT_RGBA8) == 0);

	// BC1, 8x8, 2 mips: 4 blocks then 1 block, zero-copy views at the right offsets.
	{
		DDSBuilder b;
		s_header(&b, 8, 8, 2, FOURCC('D', 'X', 'T', '1'), 0);
		int data_start = b.bytes.count();
		for (int i = 0; i < 4; ++i) s_bc1_block(&b, RGB565(255, 0, 0));
		s_bc1_block(&b, RGB565(0, 255, 0));
		cd_dds_t dds = cd_parse_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(dds.slice_count == 2);
		REQUIRE(dds.format == CD_FORMAT_BC1);
		REQUIRE(dds.w == 8 && dds.h == 8);
		REQUIRE(dds.mip_count == 2 && dds.face_count == 1 && !dds.is_cubemap);
		cd_slice_t* m0 = cd_slice(&dds, 0, 0);
		cd_slice_t* m1 = cd_slice(&dds, 0, 1);
		REQUIRE(m0->size == 32 && m0->w == 8 && m0->h == 8);
		REQUIRE(m1->size == 8 && m1->w == 4 && m1->h == 4);
		REQUIRE(m0->data == b.bytes.data() + data_start);
		REQUIRE(m1->data == b.bytes.data() + data_start + 32);
		REQUIRE(cd_slice(&dds, 0, 2) == NULL);
		REQUIRE(cd_slice(&dds, 1, 0) == NULL);
		cd_free_dds(&dds);
	}
	// DX10 BC7 sRGB with an array of 2.
	{
		DDSBuilder b;
		s_dx10_header(&b, 4, 4, 1, 99 /* BC7_UNORM_SRGB */, 2);
		s_bc7_white_block(&b);
		s_bc7_white_block(&b);
		cd_dds_t dds = cd_parse_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(dds.slice_count == 2);
		REQUIRE(dds.format == CD_FORMAT_BC7_SRGB);
		REQUIRE(dds.face_count == 2 && !dds.is_cubemap);
		cd_free_dds(&dds);
	}
	// Legacy cube map: 6 faces, all views present.
	{
		DDSBuilder b;
		s_header(&b, 4, 4, 1, FOURCC('D', 'X', 'T', '1'), CAPS2_CUBEMAP_ALL);
		for (int i = 0; i < 6; ++i) s_bc1_block(&b, RGB565(255, 0, 0));
		cd_dds_t dds = cd_parse_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(dds.slice_count == 6);
		REQUIRE(dds.is_cubemap && dds.face_count == 6);
		cd_free_dds(&dds);
	}
	// Non-multiple-of-4 dimensions still round up to whole blocks.
	{
		DDSBuilder b;
		s_header(&b, 6, 3, 1, FOURCC('D', 'X', 'T', '5'), 0);
		s_bc3_block(&b, RGB565(0, 0, 255));
		s_bc3_block(&b, RGB565(0, 0, 255));
		cd_dds_t dds = cd_parse_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(dds.slice_count == 1);
		REQUIRE(dds.slices[0].size == 32); // 2x1 blocks.
		cd_free_dds(&dds);
	}
	// Rejections, each with a non-NULL reason and a zeroed result.
	{
		cd_dds_t dds = cd_parse_dds_mem("nope", 4);
		REQUIRE(dds.slice_count == 0 && cd_error_reason != NULL);
	}
	{
		DDSBuilder b;
		s_header(&b, 8, 8, 1, FOURCC('D', 'X', 'T', '1'), 0);
		b.bytes.add(0); // 1 byte of surface data instead of 32.
		cd_dds_t dds = cd_parse_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(dds.slice_count == 0);
	}
	{
		DDSBuilder b; // Volume flag.
		s_header(&b, 4, 4, 1, FOURCC('D', 'X', 'T', '1'), 0x200000);
		s_bc1_block(&b, 0);
		cd_dds_t dds = cd_parse_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(dds.slice_count == 0);
	}
	{
		DDSBuilder b; // Partial cube map (CUBEMAP bit, only one face bit).
		s_header(&b, 4, 4, 1, FOURCC('D', 'X', 'T', '1'), 0x200 | 0x400);
		s_bc1_block(&b, 0);
		cd_dds_t dds = cd_parse_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(dds.slice_count == 0);
	}
	{
		DDSBuilder b; // Unsupported FourCC.
		s_header(&b, 4, 4, 1, FOURCC('R', 'G', 'B', 'G'), 0);
		cd_dds_t dds = cd_parse_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(dds.slice_count == 0);
	}
	{
		DDSBuilder b; // Mip count beyond any possible chain.
		s_header(&b, 4, 4, 99, FOURCC('D', 'X', 'T', '1'), 0);
		cd_dds_t dds = cd_parse_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(dds.slice_count == 0);
	}
	{
		DDSBuilder b; // Mip chain deeper than the dimensions allow (8x8 supports 4).
		s_header(&b, 8, 8, 5, FOURCC('D', 'X', 'T', '1'), 0);
		for (int i = 0; i < 8; ++i) s_bc1_block(&b, 0);
		cd_dds_t dds = cd_parse_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(dds.slice_count == 0);
	}
	{
		DDSBuilder b; // X8R8G8B8-style: 32-bit RGB with no alpha mask must be rejected,
		              // or the undefined X byte would load as (typically zero) alpha.
		s_header(&b, 4, 4, 1, 0, 0);
		for (int i = 0; i < 16; ++i) b.u32(0xffffffff);
		for (int i = 0; i < 4; ++i) b.bytes[104 + i] = 0; // Zero the amask field.
		cd_dds_t dds = cd_parse_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(dds.slice_count == 0);
	}
	return true;
}

// ---------------------------------------------------------------------------
// GPU end to end: upload through cf_make_texture_from_dds_mem, sample the
// handcrafted blocks back, and check the known decode values.
TEST_CASE(test_dds_gpu)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	REQUIRE(shader.id);
	CF_Mesh mesh = s_make_fullscreen_quad();
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	// BC1 with a mip chain: red mip 0, green mip 1, selected via textureLod.
	if (cf_texture_supports_format(CF_PIXEL_FORMAT_BC1_RGBA_UNORM, CF_TEXTURE_USAGE_SAMPLER_BIT)) {
		DDSBuilder b;
		s_header(&b, 8, 8, 2, FOURCC('D', 'X', 'T', '1'), 0);
		for (int i = 0; i < 4; ++i) s_bc1_block(&b, RGB565(255, 0, 0));
		s_bc1_block(&b, RGB565(0, 255, 0));
		CF_Texture tex = cf_make_texture_from_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(tex.id);
		CF_Pixel c = s_sample(canvas, mesh, shader, material, tex, 0, px);
		REQUIRE(c.colors.r > 200 && c.colors.g < 60);
		c = s_sample(canvas, mesh, shader, material, tex, 1.0f, px);
		REQUIRE(c.colors.g > 200 && c.colors.r < 60);
		cf_destroy_texture(tex);
	}

	// BC3: solid blue, opaque interpolated alpha.
	if (cf_texture_supports_format(CF_PIXEL_FORMAT_BC3_RGBA_UNORM, CF_TEXTURE_USAGE_SAMPLER_BIT)) {
		DDSBuilder b;
		s_header(&b, 4, 4, 1, FOURCC('D', 'X', 'T', '5'), 0);
		s_bc3_block(&b, RGB565(0, 0, 255));
		CF_Texture tex = cf_make_texture_from_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(tex.id);
		CF_Pixel c = s_sample(canvas, mesh, shader, material, tex, 0, px);
		REQUIRE(c.colors.b > 200 && c.colors.r < 60 && c.colors.a > 200);
		cf_destroy_texture(tex);
	}

	// BC5: two BC4 channel blocks with known solid values.
	if (cf_texture_supports_format(CF_PIXEL_FORMAT_BC5_RG_UNORM, CF_TEXTURE_USAGE_SAMPLER_BIT)) {
		DDSBuilder b;
		s_header(&b, 4, 4, 1, FOURCC('A', 'T', 'I', '2'), 0);
		s_bc4_block(&b, 200);
		s_bc4_block(&b, 60);
		CF_Texture tex = cf_make_texture_from_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(tex.id);
		CF_Pixel c = s_sample(canvas, mesh, shader, material, tex, 0, px);
		REQUIRE(NEAR(c.colors.r, 200) && NEAR(c.colors.g, 60));
		cf_destroy_texture(tex);
	}

	// BC7 via the DX10 header: the hand-encoded mode 6 solid white block.
	if (cf_texture_supports_format(CF_PIXEL_FORMAT_BC7_RGBA_UNORM, CF_TEXTURE_USAGE_SAMPLER_BIT)) {
		DDSBuilder b;
		s_dx10_header(&b, 4, 4, 1, 98 /* BC7_UNORM */, 1);
		s_bc7_white_block(&b);
		CF_Texture tex = cf_make_texture_from_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(tex.id);
		CF_Pixel c = s_sample(canvas, mesh, shader, material, tex, 0, px);
		REQUIRE(c.colors.r > 240 && c.colors.g > 240 && c.colors.b > 240 && c.colors.a > 240);
		cf_destroy_texture(tex);
	}

	// Uncompressed RGBA8 through legacy masks -- works on every backend.
	{
		DDSBuilder b;
		s_header(&b, 4, 4, 1, 0, 0);
		for (int i = 0; i < 16; ++i) { b.bytes.add(255); b.bytes.add(128); b.bytes.add(0); b.bytes.add(255); }
		CF_Texture tex = cf_make_texture_from_dds_mem(b.bytes.data(), b.bytes.count());
		REQUIRE(tex.id);
		CF_Pixel c = s_sample(canvas, mesh, shader, material, tex, 0, px);
		REQUIRE(c.colors.r > 240 && NEAR(c.colors.g, 128) && c.colors.b < 20);
		cf_destroy_texture(tex);
	}

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_shader(shader);
	test_destroy_app();
	return true;
}

// Cube map DDS end to end: six BC1 faces, sampled through samplerCube.
TEST_CASE(test_dds_cube)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.
	if (!cf_texture_supports_format(CF_PIXEL_FORMAT_BC1_RGBA_UNORM, CF_TEXTURE_USAGE_SAMPLER_BIT)) { test_destroy_app(); return true; }

	static const char* cube_fs =
	"layout (location = 0) out vec4 result;\n"
	"layout (set = 2, binding = 0) uniform samplerCube u_cube;\n"
	"layout (set = 3, binding = 0) uniform uniform_block {\n"
	"    vec4 u_dir;\n"
	"};\n"
	"void main() { result = texture(u_cube, u_dir.xyz); }\n";

	// Face order: +X -X +Y -Y +Z -Z. Red, green, blue, yellow, magenta, cyan.
	const uint16_t face_colors[6] = {
		RGB565(255, 0, 0), RGB565(0, 255, 0), RGB565(0, 0, 255),
		RGB565(255, 255, 0), RGB565(255, 0, 255), RGB565(0, 255, 255),
	};
	DDSBuilder b;
	s_header(&b, 4, 4, 1, FOURCC('D', 'X', 'T', '1'), CAPS2_CUBEMAP_ALL);
	for (int i = 0; i < 6; ++i) s_bc1_block(&b, face_colors[i]);
	CF_Texture cube = cf_make_texture_from_dds_mem(b.bytes.data(), b.bytes.count());
	REQUIRE(cube.id);

	CF_Shader shader = cf_make_shader_from_source(s_vs, cube_fs);
	REQUIRE(shader.id);
	CF_Mesh mesh = s_make_fullscreen_quad();
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_material_set_texture_fs(material, "u_cube", cube);

	struct { CF_V4 dir; int r, g, bl; } probes[3] = {
		{ cf_v4( 1, 0, 0, 0), 255, 0, 0 },   // +X red
		{ cf_v4( 0, -1, 0, 0), 255, 255, 0 }, // -Y yellow
		{ cf_v4( 0, 0, -1, 0), 0, 255, 255 }, // -Z cyan
	};
	for (int i = 0; i < 3; ++i) {
		cf_app_update(NULL);
		cf_material_set_uniform_fs(material, "u_dir", &probes[i].dir, CF_UNIFORM_TYPE_FLOAT4, 1);
		cf_apply_canvas(canvas, true);
		cf_apply_mesh(mesh);
		cf_apply_shader(shader, material);
		cf_draw_elements();
		cf_app_draw_onto_screen(false);
		CF_Readback rb = cf_canvas_readback(canvas);
		REQUIRE(rb.id);
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, px, W * H * (int)sizeof(CF_Pixel));
		cf_destroy_readback(rb);
		CF_Pixel c = px[(H / 2) * W + (W / 2)];
		REQUIRE(NEAR(c.colors.r, probes[i].r) && NEAR(c.colors.g, probes[i].g) && NEAR(c.colors.b, probes[i].bl));
	}

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_shader(shader);
	cf_destroy_texture(cube);
	test_destroy_app();
	return true;
}

TEST_SUITE(test_dds)
{
	RUN_TEST_CASE(test_dds_parse);
	RUN_TEST_CASE(test_dds_gpu);
	RUN_TEST_CASE(test_dds_cube);
}
