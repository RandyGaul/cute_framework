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
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL))) return true; // Headless CI: no display/GPU.

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
	cf_destroy_app();
	return true;
}

TEST_CASE(test_texture_array_sample)
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, W, H, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL))) return true; // Headless CI: no display/GPU.

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
	cf_destroy_app();
	return true;
}

TEST_SUITE(test_texture_types)
{
	RUN_TEST_CASE(test_cube_map_sample);
	RUN_TEST_CASE(test_texture_array_sample);
}
