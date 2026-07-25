// Renders a 3d scene into a low-resolution canvas and composites it back with a pixel-art
// outline pass, in the spirit of three.js's webgl_postprocessing_pixel example.
//
// The three passes, all on CF's existing low-level graphics API:
//   1. colour   -- toon-quantized lighting into a 320x240 canvas
//   2. g-buffer -- world normal + linear depth into a second 320x240 float canvas
//   3. composite -- upscale the colour buffer and ink the edges the g-buffer reveals
//
// Geometry is built procedurally: CF reads no model format, and a sample should not need an
// asset pipeline. Press SPACE to switch to CF's built-in lit shader, N to inspect the g-buffer.

#include <cute.h>

#define LOWRES_W 320
#define LOWRES_H 240
#define WINDOW_W 960
#define WINDOW_H 720

#define ZNEAR 1.0f
#define ZFAR 60.0f

// Matches the `uniform_block` in mesh.vs.
typedef struct MeshVSUniforms
{
	CF_M4x4 mvp;
	CF_M4x4 mv;
	CF_M4x4 normal_matrix;
} MeshVSUniforms;

// ---------------------------------------------------------------------------------------------
// Procedural geometry.

// Appends a box centered at `center` with half-extents `half`. Each face gets its own four
// vertices so the normals stay flat -- sharing corner vertices between faces would average the
// normals and round off exactly the creases the outline pass looks for.
static void push_box(CF_Vertex3D* verts, uint32_t* indices, int* vert_count, int* index_count, CF_V3 center, CF_V3 half)
{
	static const float face_normals[6][3] = {
		{  0,  0,  1 }, {  0,  0, -1 },
		{  1,  0,  0 }, { -1,  0,  0 },
		{  0,  1,  0 }, {  0, -1,  0 },
	};
	// Tangent and bitangent per face, so each face's quad is wound counter-clockwise when seen
	// from outside (CF_CULL_MODE_BACK culls clockwise ones).
	static const float face_tangents[6][3] = {
		{  1,  0,  0 }, { -1,  0,  0 },
		{  0,  0, -1 }, {  0,  0,  1 },
		{  1,  0,  0 }, {  1,  0,  0 },
	};
	static const float face_bitangents[6][3] = {
		{  0,  1,  0 }, {  0,  1,  0 },
		{  0,  1,  0 }, {  0,  1,  0 },
		{  0,  0, -1 }, {  0,  0,  1 },
	};

	for (int f = 0; f < 6; ++f) {
		CF_V3 n = cf_v3(face_normals[f][0], face_normals[f][1], face_normals[f][2]);
		CF_V3 t = cf_v3(face_tangents[f][0], face_tangents[f][1], face_tangents[f][2]);
		CF_V3 b = cf_v3(face_bitangents[f][0], face_bitangents[f][1], face_bitangents[f][2]);

		CF_V3 face_center = cf_add(center, cf_mul(n, cf_dot(half, cf_abs(n))));
		CF_V3 tv = cf_mul(t, cf_dot(half, cf_abs(t)));
		CF_V3 bv = cf_mul(b, cf_dot(half, cf_abs(b)));

		int base = *vert_count;
		const float uvs[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
		const float signs[4][2] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
		for (int i = 0; i < 4; ++i) {
			CF_V3 p = cf_add(face_center, cf_add(cf_mul(tv, signs[i][0]), cf_mul(bv, signs[i][1])));
			verts[base + i].pos = p;
			verts[base + i].normal = n;
			verts[base + i].uv = cf_v2(uvs[i][0], uvs[i][1]);
		}
		*vert_count += 4;

		uint32_t quad[6] = { 0, 1, 2, 0, 2, 3 };
		for (int i = 0; i < 6; ++i) indices[(*index_count)++] = (uint32_t)base + quad[i];
	}
}

// A ground plane plus a little skyline of blocks. Enough overlapping silhouettes and creases to
// give the outline pass something to find.
static CF_Mesh build_scene(void)
{
	enum { MAX_BOXES = 12, MAX_VERTS = MAX_BOXES * 24, MAX_INDICES = MAX_BOXES * 36 };
	CF_Vertex3D* verts = (CF_Vertex3D*)cf_alloc(MAX_VERTS * (int)sizeof(CF_Vertex3D));
	uint32_t* indices = (uint32_t*)cf_alloc(MAX_INDICES * (int)sizeof(uint32_t));
	int vert_count = 0, index_count = 0;

	push_box(verts, indices, &vert_count, &index_count, cf_v3(0, -1.0f, 0), cf_v3(9.0f, 0.5f, 9.0f)); // Ground.

	push_box(verts, indices, &vert_count, &index_count, cf_v3(-3.0f, 0.5f, -2.0f), cf_v3(1.0f, 1.0f, 1.0f));
	push_box(verts, indices, &vert_count, &index_count, cf_v3(-0.5f, 1.2f,  0.5f), cf_v3(0.8f, 1.7f, 0.8f));
	push_box(verts, indices, &vert_count, &index_count, cf_v3( 2.5f, 0.0f, -1.5f), cf_v3(1.2f, 0.5f, 1.2f));
	push_box(verts, indices, &vert_count, &index_count, cf_v3( 3.0f, 1.4f,  2.0f), cf_v3(0.6f, 2.0f, 0.6f));
	push_box(verts, indices, &vert_count, &index_count, cf_v3(-2.0f, 0.1f,  3.0f), cf_v3(1.4f, 0.6f, 0.9f));
	push_box(verts, indices, &vert_count, &index_count, cf_v3( 0.5f, 0.3f, -4.0f), cf_v3(2.0f, 0.8f, 0.7f));
	push_box(verts, indices, &vert_count, &index_count, cf_v3(-4.5f, 0.8f,  1.0f), cf_v3(0.5f, 1.3f, 0.5f));

	CF_Mesh mesh = cf_make_mesh_3d(verts, vert_count, indices, index_count);
	cf_free(verts);
	cf_free(indices);
	return mesh;
}

// A small checker texture, so the flat toon bands still have some surface detail to sit on.
static CF_Texture make_checker_texture(void)
{
	enum { N = 16 };
	CF_Pixel px[N * N];
	for (int y = 0; y < N; ++y) {
		for (int x = 0; x < N; ++x) {
			bool on = ((x / 4) + (y / 4)) % 2 == 0;
			px[y * N + x] = on ? cf_make_pixel_rgb(226, 222, 210) : cf_make_pixel_rgb(198, 192, 178);
		}
	}
	CF_TextureParams params = cf_texture_defaults(N, N);
	params.filter = CF_FILTER_NEAREST; // Crisp texels, to match the low-resolution look.
	CF_Texture tex = cf_make_texture(params);
	cf_texture_update(tex, px, (int)sizeof(px));
	return tex;
}

static CF_Canvas make_lowres_canvas(CF_PixelFormat format)
{
	CF_CanvasParams params = cf_canvas_defaults(LOWRES_W, LOWRES_H);
	params.target.pixel_format = format;
	params.target.filter = CF_FILTER_NEAREST;   // Upscaling must not blur the pixels.
	params.depth_stencil_enable = true;         // Without this all depth render state is ignored.
	return cf_make_canvas(params);
}

int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app("Pixel 3D", 0, 0, 0, WINDOW_W, WINDOW_H,
		CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);
	if (cf_is_error(result)) return -1;

	// Mount this sample's data folder under its own name, then point the shader directory at it so
	// the shaders can be found and hot-reloaded while the sample runs. The mount has to come first,
	// or cf_shader_directory walks a path that does not exist yet.
	char* base = cf_path_normalize(cf_fs_get_base_directory());
	char* data_dir = cf_string_append(base, "/pixel_3d_data");
	cf_fs_mount(data_dir, "/pixel_3d_data", false);
	cf_string_free(data_dir);
	cf_shader_directory("/pixel_3d_data");

	CF_Mesh mesh = build_scene();
	CF_Texture albedo = make_checker_texture();

	// Two fragment stages over one shared vertex stage -- the case the two-file cf_make_shader
	// path exists for. A draw shader cannot do this: it has no hook for a custom vertex stage.
	CF_Shader toon_shader = cf_make_shader("/pixel_3d_data/mesh.vs", "/pixel_3d_data/mesh_toon.fs");
	CF_Shader normal_shader = cf_make_shader("/pixel_3d_data/mesh.vs", "/pixel_3d_data/mesh_normal.fs");
	CF_Shader lit_shader = cf_make_lit_shader_3d();
	CF_Shader post_shader = cf_make_draw_shader("pixel_post.shd");

	CF_Material toon_material = cf_make_material();
	CF_Material normal_material = cf_make_material();
	CF_Material lit_material = cf_make_material();

	CF_RenderState rs = cf_render_state_3d_defaults();
	cf_material_set_render_state(toon_material, rs);
	cf_material_set_render_state(normal_material, rs);
	cf_material_set_render_state(lit_material, rs);

	cf_material_set_texture_fs(toon_material, "u_albedo", albedo);
	cf_material_set_texture_fs(normal_material, "u_albedo", albedo);
	cf_lit_shader_3d_set_albedo(lit_material, albedo);

	CF_Canvas scene_canvas = make_lowres_canvas(CF_PIXEL_FORMAT_R8G8B8A8_UNORM);
	// Normals need a signed format with range beyond [0,1], and the depth in alpha wants more
	// than 8 bits or the silhouette threshold picks up banding.
	CF_Canvas normal_canvas = make_lowres_canvas(CF_PIXEL_FORMAT_R16G16B16A16_FLOAT);

	CF_V3 light_dir = cf_safe_norm(cf_v3(-0.4f, -1.0f, -0.35f));
	bool use_builtin_shader = false;
	bool show_normals = false;
	float t = 0;

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		if (cf_key_just_pressed(CF_KEY_SPACE)) use_builtin_shader = !use_builtin_shader;
		if (cf_key_just_pressed(CF_KEY_N)) show_normals = !show_normals;
		t += CF_DELTA_TIME;

		// Slow orbit, so silhouettes sweep across each other and the outlines stay interesting.
		float angle = t * 0.25f;
		CF_V3 eye = cf_v3(CF_COSF(angle) * 13.0f, 7.0f, CF_SINF(angle) * 13.0f);
		CF_M4x4 view = cf_look_at(eye, cf_v3(0, 0.5f, 0), cf_v3(0, 1.0f, 0));
		CF_M4x4 proj = cf_perspective(CF_PI / 4.0f, (float)LOWRES_W / (float)LOWRES_H, ZNEAR, ZFAR);
		CF_M4x4 model = cf_m4_identity();

		MeshVSUniforms u;
		u.mvp = cf_mul(proj, cf_mul(view, model));
		u.mv = cf_mul(view, model);
		u.normal_matrix = cf_m4_normal_matrix(model);

		// --- Pass 1: colour. ---
		cf_clear_color(0.10f, 0.11f, 0.16f, 1.0f);
		if (use_builtin_shader) {
			cf_lit_shader_3d_set_transform(lit_material, u.mvp, model);
			cf_lit_shader_3d_set_light(lit_material, light_dir,
				cf_make_color_rgb_f(1.0f, 0.95f, 0.85f), cf_make_color_rgb_f(0.22f, 0.24f, 0.34f));
			cf_apply_canvas(scene_canvas, true);
			cf_apply_mesh(mesh);
			cf_apply_shader(lit_shader, lit_material);
			cf_draw_elements();
		} else {
			CF_V4 dir4 = cf_v4_from_v3(light_dir, 0);
			CF_Color light_color = cf_make_color_rgb_f(1.0f, 0.95f, 0.85f);
			CF_Color ambient = cf_make_color_rgb_f(0.22f, 0.24f, 0.34f);
			CF_V4 params = cf_v4(4.0f, 0, 0, 0); // Four shading bands.

			cf_material_set_uniform_vs(toon_material, "u_mvp", &u.mvp, CF_UNIFORM_TYPE_MAT4, 1);
			cf_material_set_uniform_vs(toon_material, "u_mv", &u.mv, CF_UNIFORM_TYPE_MAT4, 1);
			cf_material_set_uniform_vs(toon_material, "u_normal_matrix", &u.normal_matrix, CF_UNIFORM_TYPE_MAT4, 1);
			cf_material_set_uniform_fs(toon_material, "u_light_direction", &dir4, CF_UNIFORM_TYPE_FLOAT4, 1);
			cf_material_set_uniform_fs(toon_material, "u_light_color", &light_color, CF_UNIFORM_TYPE_FLOAT4, 1);
			cf_material_set_uniform_fs(toon_material, "u_ambient", &ambient, CF_UNIFORM_TYPE_FLOAT4, 1);
			cf_material_set_uniform_fs(toon_material, "u_params", &params, CF_UNIFORM_TYPE_FLOAT4, 1);

			cf_apply_canvas(scene_canvas, true);
			cf_apply_mesh(mesh);
			cf_apply_shader(toon_shader, toon_material);
			cf_draw_elements();
		}

		// --- Pass 2: normal and depth g-buffer. ---
		// Clears to a far, zero-length normal so the background never registers as an edge
		// against itself -- only geometry silhouetted against it does.
		cf_clear_color(0, 0, 0, 1.0f);
		CF_V4 depth_params = cf_v4(ZNEAR, ZFAR, 0, 0);
		cf_material_set_uniform_vs(normal_material, "u_mvp", &u.mvp, CF_UNIFORM_TYPE_MAT4, 1);
		cf_material_set_uniform_vs(normal_material, "u_mv", &u.mv, CF_UNIFORM_TYPE_MAT4, 1);
		cf_material_set_uniform_vs(normal_material, "u_normal_matrix", &u.normal_matrix, CF_UNIFORM_TYPE_MAT4, 1);
		cf_material_set_uniform_fs(normal_material, "u_params", &depth_params, CF_UNIFORM_TYPE_FLOAT4, 1);

		cf_apply_canvas(normal_canvas, true);
		cf_apply_mesh(mesh);
		cf_apply_shader(normal_shader, normal_material);
		cf_draw_elements();

		// --- Pass 3: composite, upscaled to the window. ---
		// cf_clear_color is global state read by every clear, so restore something sane before
		// the deferred draw stream renders onto the app canvas.
		cf_clear_color(0.10f, 0.11f, 0.16f, 1.0f);

		float w = (float)cf_app_get_width(), h = (float)cf_app_get_height();
		cf_draw_push_shader(post_shader);
		cf_draw_set_texture("scene_tex", cf_canvas_get_target(scene_canvas));
		cf_draw_set_texture("normal_tex", cf_canvas_get_target(normal_canvas));
		cf_draw_set_uniform_v2("u_texel", cf_v2(1.0f / LOWRES_W, 1.0f / LOWRES_H));
		cf_draw_set_uniform_float("u_edge_strength", use_builtin_shader ? 0.0f : 1.0f);
		cf_draw_set_uniform_float("u_show_normals", show_normals ? 1.0f : 0.0f);
		cf_draw_push_shape_aa(0);
		cf_draw_box_fill(cf_make_aabb(cf_v2(-w * 0.5f, -h * 0.5f), cf_v2(w * 0.5f, h * 0.5f)), 0);
		cf_draw_pop_shape_aa();
		cf_draw_pop_shader();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_shader(toon_shader);
	cf_destroy_shader(normal_shader);
	cf_destroy_shader(lit_shader);
	cf_destroy_shader(post_shader);
	cf_destroy_material(toon_material);
	cf_destroy_material(normal_material);
	cf_destroy_material(lit_material);
	cf_destroy_canvas(scene_canvas);
	cf_destroy_canvas(normal_canvas);
	cf_destroy_texture(albedo);
	cf_destroy_mesh(mesh);
	cf_destroy_app();
	return 0;
}
