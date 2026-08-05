/*
    Cute Framework
    Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Cube map and 2D-array textures, end to end: create, upload per-face/layer, sample through
// samplerCube / sampler2DArray in a real fragment shader, and verify by readback. This exercises
// the whole chain -- CF_TextureType plumbing, cf_texture_update_layer, and the shader compiler's
// new sampler dims -- on whichever backend the suite runs.

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>

using namespace Cute;

#define W 64
#define H 64

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"void main() { gl_Position = vec4(in_pos, 0, 1); }\n";

static const char* s_cube_fs =
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform samplerCube u_cube;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_dir;\n"
"};\n"
"void main() { result = texture(u_cube, u_dir.xyz); }\n";

static const char* s_array_fs =
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2DArray u_layers;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_layer;\n"
"};\n"
"void main() { result = texture(u_layers, vec3(0.5, 0.5, u_layer.x)); }\n";

static CF_Mesh s_make_fullscreen_quad()
{
	struct Vertex { float x, y; };
	Vertex verts[6] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, -1 }, { 1, 1 }, { -1, 1 } };
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	return mesh;
}

static void s_fill(CF_Pixel* px, int count, uint8_t r, uint8_t g, uint8_t b)
{
	for (int i = 0; i < count; ++i) px[i] = cf_make_pixel_rgb(r, g, b);
}

static CF_Pixel s_draw_and_read(CF_Canvas canvas, CF_Mesh mesh, CF_Shader shader, CF_Material material, CF_Pixel* px)
{
	cf_app_update(NULL);
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

TEST_CASE(test_cube_map_sample)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_TextureParams tp = cf_texture_defaults(32, 32);
	tp.texture_type = CF_TEXTURE_TYPE_CUBE;
	CF_Texture cube = cf_make_texture(tp);
	REQUIRE(cube.id);

	// Each face its own color: +X red, -X green, +Y blue, -Y yellow, +Z magenta, -Z cyan.
	const uint8_t face_colors[6][3] = {
		{ 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 }, { 255, 255, 0 }, { 255, 0, 255 }, { 0, 255, 255 },
	};
	CF_Pixel* face = (CF_Pixel*)cf_alloc(32 * 32 * (int)sizeof(CF_Pixel));
	for (int i = 0; i < 6; ++i) {
		s_fill(face, 32 * 32, face_colors[i][0], face_colors[i][1], face_colors[i][2]);
		cf_texture_update_layer(cube, face, 32 * 32 * (int)sizeof(CF_Pixel), i);
	}
	cf_free(face);

	CF_Shader shader = cf_make_shader_from_source(s_vs, s_cube_fs);
	REQUIRE(shader.id);
	CF_Mesh mesh = s_make_fullscreen_quad();
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_material_set_texture_fs(material, "u_cube", cube);

	// Sample straight down +X: face 0, red.
	float dir[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	cf_material_set_uniform_fs(material, "u_dir", dir, CF_UNIFORM_TYPE_FLOAT4, 1);
	CF_Pixel c = s_draw_and_read(canvas, mesh, shader, material, px);
	REQUIRE(c.colors.r > 200 && c.colors.g < 60 && c.colors.b < 60);

	// Sample -Y: face 3, yellow.
	float dir2[4] = { 0.0f, -1.0f, 0.0f, 0.0f };
	cf_material_set_uniform_fs(material, "u_dir", dir2, CF_UNIFORM_TYPE_FLOAT4, 1);
	c = s_draw_and_read(canvas, mesh, shader, material, px);
	REQUIRE(c.colors.r > 200 && c.colors.g > 200 && c.colors.b < 60);

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_shader(shader);
	cf_destroy_texture(cube);
	test_destroy_app();
	return true;
}

TEST_CASE(test_texture_array_sample)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_TextureParams tp = cf_texture_defaults(32, 32);
	tp.texture_type = CF_TEXTURE_TYPE_2D_ARRAY;
	tp.layer_count = 3;
	CF_Texture layers = cf_make_texture(tp);
	REQUIRE(layers.id);

	const uint8_t layer_colors[3][3] = { { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 } };
	CF_Pixel* img = (CF_Pixel*)cf_alloc(32 * 32 * (int)sizeof(CF_Pixel));
	for (int i = 0; i < 3; ++i) {
		s_fill(img, 32 * 32, layer_colors[i][0], layer_colors[i][1], layer_colors[i][2]);
		cf_texture_update_layer(layers, img, 32 * 32 * (int)sizeof(CF_Pixel), i);
	}
	cf_free(img);

	CF_Shader shader = cf_make_shader_from_source(s_vs, s_array_fs);
	REQUIRE(shader.id);
	CF_Mesh mesh = s_make_fullscreen_quad();
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_material_set_texture_fs(material, "u_layers", layers);

	// Layer 2 is blue.
	float layer[4] = { 2.0f, 0.0f, 0.0f, 0.0f };
	cf_material_set_uniform_fs(material, "u_layer", layer, CF_UNIFORM_TYPE_FLOAT4, 1);
	CF_Pixel c = s_draw_and_read(canvas, mesh, shader, material, px);
	REQUIRE(c.colors.b > 200 && c.colors.r < 60);

	// Layer 0 is red.
	float layer0[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	cf_material_set_uniform_fs(material, "u_layer", layer0, CF_UNIFORM_TYPE_FLOAT4, 1);
	c = s_draw_and_read(canvas, mesh, shader, material, px);
	REQUIRE(c.colors.r > 200 && c.colors.b < 60);

	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_shader(shader);
	cf_destroy_texture(layers);
	test_destroy_app();
	return true;
}

// Render INTO cube faces through attached canvases (the point-light shadow pattern),
// then sample the cube to verify each face took its clear color.
TEST_CASE(test_render_to_cube_face)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_TextureParams tp = cf_texture_defaults(32, 32);
	tp.texture_type = CF_TEXTURE_TYPE_CUBE;
	tp.usage |= CF_TEXTURE_USAGE_COLOR_TARGET_BIT;
	CF_Texture cube = cf_make_texture(tp);
	REQUIRE(cube.id);

	// One canvas per face used; clear +X red and -Y green through them.
	struct { int face; CF_Color color; } faces[2] = {
		{ 0, cf_make_color_rgb_f(1.0f, 0, 0) },
		{ 3, cf_make_color_rgb_f(0, 1.0f, 0) },
	};
	cf_app_update(NULL);
	for (int i = 0; i < 2; ++i) {
		CF_CanvasParams params = cf_canvas_defaults(32, 32);
		params.attach_target = cube;
		params.attach_layer = faces[i].face;
		CF_Canvas canvas = cf_make_canvas(params);
		REQUIRE(canvas.id);
		cf_canvas_set_clear_color(canvas, faces[i].color);
		cf_clear_canvas(canvas);
		cf_destroy_canvas(canvas);
	}
	cf_app_draw_onto_screen(false);

	// Sample the cube: +X must be red, -Y green.
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_cube_fs);
	REQUIRE(shader.id);
	CF_Mesh mesh = s_make_fullscreen_quad();
	CF_Material material = cf_make_material();
	CF_Canvas out = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_material_set_texture_fs(material, "u_cube", cube);

	float dir[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	cf_material_set_uniform_fs(material, "u_dir", dir, CF_UNIFORM_TYPE_FLOAT4, 1);
	CF_Pixel c = s_draw_and_read(out, mesh, shader, material, px);
	REQUIRE(c.colors.r > 200 && c.colors.g < 60);

	float dir2[4] = { 0.0f, -1.0f, 0.0f, 0.0f };
	cf_material_set_uniform_fs(material, "u_dir", dir2, CF_UNIFORM_TYPE_FLOAT4, 1);
	c = s_draw_and_read(out, mesh, shader, material, px);
	REQUIRE(c.colors.g > 200 && c.colors.r < 60);

	cf_free(px);
	cf_destroy_canvas(out);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_shader(shader);
	cf_destroy_texture(cube);
	test_destroy_app();
	return true;
}

static const char* s_quad_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() { gl_Position = u_view_projection * vec4(in_pos, 1.0); }\n";

static const char* s_red_fs =
"layout (location = 0) out vec4 result;\n"
"void main() { result = vec4(1.0, 0.0, 0.0, 1.0); }\n";

// Pins the cube-face T-axis convention that samples/point_light.c relies on: a face camera
// built with cf_look_at + a Y-mirrored cf_perspective (see that sample's face_projection
// comment) must render geometry into the row a samplerCube fetch expects, not the row a
// plain (unmirrored) right-handed camera would put it in.
TEST_CASE(test_cube_face_orientation)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_TextureParams tp = cf_texture_defaults(64, 64);
	tp.texture_type = CF_TEXTURE_TYPE_CUBE;
	tp.usage |= CF_TEXTURE_USAGE_COLOR_TARGET_BIT;
	CF_Texture cube = cf_make_texture(tp);
	REQUIRE(cube.id);

	CF_CanvasParams cp = cf_canvas_defaults(64, 64);
	cp.attach_target = cube;
	cp.attach_layer = 0; // +X face.
	CF_Canvas face = cf_make_canvas(cp);
	REQUIRE(face.id);
	cf_canvas_set_clear_color(face, cf_make_color_rgb_f(1, 1, 1)); // "Saw nothing" background.

	// A small red quad above the eye, off to the +X side -- same face-camera construction as
	// samples/point_light.c: cf_look_at down +X, up (0,-1,0), and a Y-mirrored 90 degree
	// cf_perspective.
	struct { float x, y, z; } verts[6] = {
		{ 5, 1.5f, -0.5f }, { 5, 2.5f, -0.5f }, { 5, 2.5f,  0.5f },
		{ 5, 1.5f, -0.5f }, { 5, 2.5f,  0.5f }, { 5, 1.5f,  0.5f },
	};
	CF_VertexAttribute attrs[1] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = 0;
	CF_Mesh quad = cf_make_mesh(sizeof(verts), attrs, 1, sizeof(verts[0]));
	cf_mesh_update_vertex_data(quad, verts, 6);

	CF_M4x4 projection = cf_mul_m4(cf_m4_scale(cf_v3(1, -1, 1)), cf_perspective(CF_PI * 0.5f, 1.0f, 0.1f, 10.0f));
	CF_M4x4 view = cf_look_at(cf_v3(0, 0, 0), cf_v3(1, 0, 0), cf_v3(0, -1, 0));
	CF_M4x4 vp = cf_mul_m4(projection, view);

	CF_Shader quad_shader = cf_make_shader_from_source(s_quad_vs, s_red_fs);
	REQUIRE(quad_shader.id);
	CF_Material quad_material = cf_make_material();
	CF_RenderState rs = cf_render_state_defaults();
	rs.cull_mode = CF_CULL_MODE_NONE; // Winding depends on the mirror; don't cull either way.
	cf_material_set_render_state(quad_material, rs);
	cf_material_set_uniform_vs(quad_material, "u_view_projection", &vp, CF_UNIFORM_TYPE_MAT4, 1);

	cf_app_update(NULL);
	cf_apply_canvas(face, true);
	cf_apply_mesh(quad);
	cf_apply_shader(quad_shader, quad_material);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);

	// Sample the cube: a direction above the eye (matching the quad) must be red; a direction
	// below (matching nothing) must stay the white background. Getting these swapped is
	// exactly the bug the mirror in samples/point_light.c fixes.
	CF_Shader cube_shader = cf_make_shader_from_source(s_vs, s_cube_fs);
	REQUIRE(cube_shader.id);
	CF_Mesh fullscreen = s_make_fullscreen_quad();
	CF_Material material = cf_make_material();
	CF_Canvas out = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_material_set_texture_fs(material, "u_cube", cube);

	float dir_up[4] = { 1.0f, 0.4f, 0.0f, 0.0f };
	cf_material_set_uniform_fs(material, "u_dir", dir_up, CF_UNIFORM_TYPE_FLOAT4, 1);
	CF_Pixel c = s_draw_and_read(out, fullscreen, cube_shader, material, px);
	REQUIRE(c.colors.r > 200 && c.colors.g < 60 && c.colors.b < 60);

	float dir_down[4] = { 1.0f, -0.4f, 0.0f, 0.0f };
	cf_material_set_uniform_fs(material, "u_dir", dir_down, CF_UNIFORM_TYPE_FLOAT4, 1);
	c = s_draw_and_read(out, fullscreen, cube_shader, material, px);
	REQUIRE(c.colors.r > 200 && c.colors.g > 200 && c.colors.b > 200);

	cf_free(px);
	cf_destroy_canvas(out);
	cf_destroy_material(material);
	cf_destroy_mesh(fullscreen);
	cf_destroy_shader(cube_shader);
	cf_destroy_material(quad_material);
	cf_destroy_mesh(quad);
	cf_destroy_shader(quad_shader);
	cf_destroy_canvas(face);
	cf_destroy_texture(cube);
	test_destroy_app();
	return true;
}

static const char* s_lod_fs =
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_tex;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_lod;\n"
"};\n"
"void main() { result = textureLod(u_tex, vec2(0.5, 0.5), u_lod.x); }\n";

static const char* s_cube_lod_fs =
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform samplerCube u_cube;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_dir;\n"
"};\n"
"void main() { result = textureLod(u_cube, u_dir.xyz, u_dir.w); }\n";

// Rendering INTO a mip level: attach_mip picks which mip of attach_target a canvas renders to
// (the downsample-chain / bloom primitive), and cf_texture_update_layer_mip uploads straight to
// a (face, mip) pair (the baked-IBL-prefilter primitive). Mip 0 and mip 1 hold different colors
// throughout, so a wrong mip target can't pass.
TEST_CASE(test_attach_mip)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	// A mipmapped 2D render target: clear mip 0 blue and mip 1 red through two canvases.
	CF_TextureParams tp = cf_texture_defaults(64, 64);
	tp.usage |= CF_TEXTURE_USAGE_COLOR_TARGET_BIT;
	tp.allocate_mipmaps = true;
	tp.mip_filter = CF_MIP_FILTER_NEAREST;
	CF_Texture tex = cf_make_texture(tp);
	REQUIRE(tex.id);

	cf_app_update(NULL);
	struct { int mip; CF_Color color; } mips[2] = {
		{ 0, cf_make_color_rgb_f(0, 0, 1.0f) },
		{ 1, cf_make_color_rgb_f(1.0f, 0, 0) },
	};
	for (int i = 0; i < 2; ++i) {
		CF_CanvasParams params = cf_canvas_defaults(64, 64);
		params.attach_target = tex;
		params.attach_mip = mips[i].mip;
		CF_Canvas canvas = cf_make_canvas(params);
		REQUIRE(canvas.id);
		int cw = 0, ch = 0;
		cf_canvas_get_size(canvas, &cw, &ch);
		REQUIRE(cw == (64 >> mips[i].mip) && ch == (64 >> mips[i].mip));
		cf_canvas_set_clear_color(canvas, mips[i].color);
		cf_clear_canvas(canvas);
		cf_destroy_canvas(canvas);
	}
	cf_app_draw_onto_screen(false);

	CF_Shader shader = cf_make_shader_from_source(s_vs, s_lod_fs);
	REQUIRE(shader.id);
	CF_Mesh mesh = s_make_fullscreen_quad();
	CF_Material material = cf_make_material();
	CF_Canvas out = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));
	cf_material_set_texture_fs(material, "u_tex", tex);

	float lod0[4] = { 0, 0, 0, 0 };
	cf_material_set_uniform_fs(material, "u_lod", lod0, CF_UNIFORM_TYPE_FLOAT4, 1);
	CF_Pixel c = s_draw_and_read(out, mesh, shader, material, px);
	REQUIRE(c.colors.b > 200 && c.colors.r < 60);

	float lod1[4] = { 1.0f, 0, 0, 0 };
	cf_material_set_uniform_fs(material, "u_lod", lod1, CF_UNIFORM_TYPE_FLOAT4, 1);
	c = s_draw_and_read(out, mesh, shader, material, px);
	REQUIRE(c.colors.r > 200 && c.colors.b < 60);

	// Upload to a (face, mip) pair of a mipmapped cube and fetch it back at that lod --
	// exactly what uploading a baked prefiltered environment map does.
	CF_TextureParams ctp = cf_texture_defaults(32, 32);
	ctp.texture_type = CF_TEXTURE_TYPE_CUBE;
	ctp.allocate_mipmaps = true;
	ctp.mip_filter = CF_MIP_FILTER_NEAREST;
	CF_Texture cube = cf_make_texture(ctp);
	REQUIRE(cube.id);
	CF_Pixel* face_px = (CF_Pixel*)cf_alloc(16 * 16 * (int)sizeof(CF_Pixel));
	s_fill(face_px, 16 * 16, 0, 255, 0);
	cf_texture_update_layer_mip(cube, face_px, 16 * 16 * (int)sizeof(CF_Pixel), 0, 1); // +X face, mip 1.

	CF_Shader cube_shader = cf_make_shader_from_source(s_vs, s_cube_lod_fs);
	REQUIRE(cube_shader.id);
	CF_Material cube_material = cf_make_material();
	cf_material_set_texture_fs(cube_material, "u_cube", cube);
	float dir[4] = { 1.0f, 0, 0, 1.0f }; // +X at lod 1.
	cf_material_set_uniform_fs(cube_material, "u_dir", dir, CF_UNIFORM_TYPE_FLOAT4, 1);
	c = s_draw_and_read(out, mesh, cube_shader, cube_material, px);
	REQUIRE(c.colors.g > 200 && c.colors.r < 60);

	cf_free(face_px);
	cf_free(px);
	cf_destroy_canvas(out);
	cf_destroy_material(material);
	cf_destroy_material(cube_material);
	cf_destroy_mesh(mesh);
	cf_destroy_shader(shader);
	cf_destroy_shader(cube_shader);
	cf_destroy_texture(tex);
	cf_destroy_texture(cube);
	test_destroy_app();
	return true;
}

static const char* s_depth_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    vec4 u_depth;\n"
"};\n"
"void main() { gl_Position = vec4(in_pos, u_depth.x, 1.0); }\n";

static const char* s_depth_fs =
"layout (location = 0) out vec4 result;\n"
"void main() { result = vec4(1.0); }\n";

static const char* s_shadow_2d_fs =
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2DShadow u_shadow;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_dirref;\n"
"};\n"
"void main() { float vis = texture(u_shadow, vec3(0.5, 0.5, u_dirref.w)); result = vec4(vis, 0.0, 0.0, 1.0); }\n";

static const char* s_shadow_cube_fs =
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform samplerCubeShadow u_shadow;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_dirref;\n"
"};\n"
"void main() { float vis = texture(u_shadow, vec4(u_dirref.xyz, u_dirref.w)); result = vec4(vis, 0.0, 0.0, 1.0); }\n";

// Renders a known depth into an attached depth texture (layer `layer`), no color side at all,
// through a depth-only canvas.
static void s_render_depth_face(CF_Texture target, int layer, float depth_value, CF_Mesh mesh, CF_Shader shd, CF_Material mat)
{
	CF_CanvasParams params = cf_canvas_defaults(32, 32);
	params.attach_target = target;
	params.attach_layer = layer;
	CF_Canvas canvas = cf_make_canvas(params);
	float depth[4] = { depth_value, 0, 0, 0 };
	cf_material_set_uniform_vs(mat, "u_depth", depth, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_apply_canvas(canvas, true);
	cf_apply_mesh(mesh);
	cf_apply_shader(shd, mat);
	cf_draw_elements();
	cf_app_draw_onto_screen(false);
	cf_destroy_canvas(canvas);
}

// A depth-format attach_target becomes the canvas's DEPTH attachment: render known depths into
// user depth textures through depth-only canvases, then verify with hardware comparison fetches
// (LESS_THAN_OR_EQUAL: lit when ref <= stored). The 2D case runs everywhere; the cube case runs
// on GLES only for now -- SDL_GPU's D3D12 backend creates only TEXTURE2D-dimension depth views,
// so cube-face depth rendering silently misses there (Vulkan/Metal are expected to work).
TEST_CASE(test_depth_attach)
{
	const char* gles = getenv("CF_TEST_GLES");
	bool is_gles = gles && *gles == '1';
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_Shader depth_shd = cf_make_shader_from_source(s_depth_vs, s_depth_fs);
	REQUIRE(depth_shd.id);
	CF_Mesh mesh = s_make_fullscreen_quad();
	CF_Material depth_mat = cf_make_material();
	CF_RenderState rs = cf_render_state_defaults();
	rs.depth_write_enabled = true;
	rs.depth_compare = CF_COMPARE_FUNCTION_ALWAYS;
	cf_material_set_render_state(depth_mat, rs);

	CF_Material material = cf_make_material();
	CF_Canvas out = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	// 2D: attach a plain depth texture, write 0.25, comparison-fetch around it.
	{
		CF_TextureParams tp = cf_texture_defaults(32, 32);
		tp.pixel_format = CF_PIXEL_FORMAT_D32_FLOAT;
		tp.usage = CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT | CF_TEXTURE_USAGE_SAMPLER_BIT;
		tp.compare_enable = true;
		tp.compare_function = CF_COMPARE_FUNCTION_LESS_THAN_OR_EQUAL;
		CF_Texture depth_2d = cf_make_texture(tp);
		REQUIRE(depth_2d.id);

		cf_app_update(NULL);
		s_render_depth_face(depth_2d, 0, 0.25f, mesh, depth_shd, depth_mat);

		CF_Shader shadow_shd = cf_make_shader_from_source(s_vs, s_shadow_2d_fs);
		REQUIRE(shadow_shd.id);
		cf_material_set_texture_fs(material, "u_shadow", depth_2d);
		struct { float ref; bool lit; } probes[2] = { { 0.10f, true }, { 0.60f, false } };
		for (int i = 0; i < 2; ++i) {
			float dirref[4] = { 0, 0, 0, probes[i].ref };
			cf_material_set_uniform_fs(material, "u_dirref", dirref, CF_UNIFORM_TYPE_FLOAT4, 1);
			CF_Pixel c = s_draw_and_read(out, mesh, shadow_shd, material, px);
			if (probes[i].lit) { REQUIRE(c.colors.r > 200); }
			else { REQUIRE(c.colors.r < 60); }
		}
		cf_destroy_shader(shadow_shd);
		cf_destroy_texture(depth_2d);
	}

	// Cube: +X face holds 0.25, -Y face holds 0.75, probed through samplerCubeShadow.
	if (is_gles) {
		CF_TextureParams tp = cf_texture_defaults(32, 32);
		tp.texture_type = CF_TEXTURE_TYPE_CUBE;
		tp.pixel_format = CF_PIXEL_FORMAT_D32_FLOAT;
		tp.usage = CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT | CF_TEXTURE_USAGE_SAMPLER_BIT;
		tp.compare_enable = true;
		tp.compare_function = CF_COMPARE_FUNCTION_LESS_THAN_OR_EQUAL;
		CF_Texture cube = cf_make_texture(tp);
		REQUIRE(cube.id);

		cf_app_update(NULL);
		s_render_depth_face(cube, 0, 0.25f, mesh, depth_shd, depth_mat);
		s_render_depth_face(cube, 3, 0.75f, mesh, depth_shd, depth_mat);

		CF_Shader shadow_shd = cf_make_shader_from_source(s_vs, s_shadow_cube_fs);
		REQUIRE(shadow_shd.id);
		cf_material_set_texture_fs(material, "u_shadow", cube);
		struct { float dir[3]; float ref; bool lit; } probes[4] = {
			{ {  1, 0, 0 }, 0.10f, true  },
			{ {  1, 0, 0 }, 0.60f, false },
			{ { 0, -1, 0 }, 0.60f, true  },
			{ { 0, -1, 0 }, 0.90f, false },
		};
		for (int i = 0; i < 4; ++i) {
			float dirref[4] = { probes[i].dir[0], probes[i].dir[1], probes[i].dir[2], probes[i].ref };
			cf_material_set_uniform_fs(material, "u_dirref", dirref, CF_UNIFORM_TYPE_FLOAT4, 1);
			CF_Pixel c = s_draw_and_read(out, mesh, shadow_shd, material, px);
			if (probes[i].lit) { REQUIRE(c.colors.r > 200); }
			else { REQUIRE(c.colors.r < 60); }
		}
		cf_destroy_shader(shadow_shd);
		cf_destroy_texture(cube);
	}

	cf_free(px);
	cf_destroy_canvas(out);
	cf_destroy_material(material);
	cf_destroy_material(depth_mat);
	cf_destroy_mesh(mesh);
	cf_destroy_shader(depth_shd);
	test_destroy_app();
	return true;
}

// Standalone samplers: one texture, two ways to filter it. A 2x1 red|blue texture baked with
// NEAREST reads a pure texel at uv 0.5; overriding the binding with a LINEAR CF_Sampler must
// yield the 50/50 blend instead, and rebinding without the sampler must restore pure NEAREST.
TEST_CASE(test_standalone_sampler)
{
	if (!test_make_app(W, H)) return true; // Headless CI: no display/GPU.

	CF_TextureParams tp = cf_texture_defaults(2, 1);
	tp.filter = CF_FILTER_NEAREST;
	tp.mip_filter = CF_MIP_FILTER_NEAREST;
	tp.wrap_u = CF_WRAP_MODE_CLAMP_TO_EDGE;
	tp.wrap_v = CF_WRAP_MODE_CLAMP_TO_EDGE;
	CF_Texture tex = cf_make_texture(tp);
	REQUIRE(tex.id);
	CF_Pixel texels[2] = { cf_make_pixel_rgb(255, 0, 0), cf_make_pixel_rgb(0, 0, 255) };
	cf_texture_update(tex, texels, (int)sizeof(texels));

	const char* fs =
		"layout (location = 0) out vec4 result;\n"
		"layout (set = 2, binding = 0) uniform sampler2D u_tex;\n"
		"void main() { result = texture(u_tex, vec2(0.5, 0.5)); }\n";
	CF_Shader shader = cf_make_shader_from_source(s_vs, fs);
	REQUIRE(shader.id);
	CF_Mesh mesh = s_make_fullscreen_quad();
	CF_Material material = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(W * H * (int)sizeof(CF_Pixel));

	// Baked NEAREST: uv 0.5 lands on a single texel -- a pure color, no blend.
	cf_material_set_texture_fs(material, "u_tex", tex);
	CF_Pixel c = s_draw_and_read(canvas, mesh, shader, material, px);
	bool pure = (c.colors.r > 200 && c.colors.b < 60) || (c.colors.b > 200 && c.colors.r < 60);
	REQUIRE(pure);
	REQUIRE(c.colors.g < 60);

	// LINEAR standalone sampler on the same texture: uv 0.5 sits exactly between
	// texel centers, so red and blue blend about 50/50.
	CF_SamplerParams sp = cf_sampler_defaults();
	sp.wrap_u = CF_WRAP_MODE_CLAMP_TO_EDGE;
	sp.wrap_v = CF_WRAP_MODE_CLAMP_TO_EDGE;
	CF_Sampler linear = cf_make_sampler(sp);
	REQUIRE(linear.id);
	cf_material_set_texture_fs_sampler(material, "u_tex", tex, linear);
	c = s_draw_and_read(canvas, mesh, shader, material, px);
	REQUIRE(c.colors.r > 80 && c.colors.r < 180);
	REQUIRE(c.colors.b > 80 && c.colors.b < 180);

	// Rebind without the sampler: back to the texture's baked NEAREST state.
	cf_material_set_texture_fs(material, "u_tex", tex);
	c = s_draw_and_read(canvas, mesh, shader, material, px);
	pure = (c.colors.r > 200 && c.colors.b < 60) || (c.colors.b > 200 && c.colors.r < 60);
	REQUIRE(pure);

	cf_destroy_sampler(linear);
	cf_free(px);
	cf_destroy_canvas(canvas);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_shader(shader);
	cf_destroy_texture(tex);
	test_destroy_app();
	return true;
}

TEST_SUITE(test_texture_types)
{
	RUN_TEST_CASE(test_cube_map_sample);
	RUN_TEST_CASE(test_texture_array_sample);
	RUN_TEST_CASE(test_render_to_cube_face);
	RUN_TEST_CASE(test_cube_face_orientation);
	RUN_TEST_CASE(test_depth_attach);
	RUN_TEST_CASE(test_attach_mip);
	RUN_TEST_CASE(test_standalone_sampler);
}
