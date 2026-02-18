// Holographic Radiance Cascades
//
// Demonstrates HRC 2D global illumination based on the Amitabha-style SSBO pipeline with f16 packing and 4-rotation frustum.
//
// Reference: Freeman, Sannikov, Margel (2025) "Holographic Radiance Cascades"
// https://arxiv.org/pdf/2505.02041
// https://github.com/entropylost/amitabha

#include <cute.h>

//--------------------------------------------------------------------------------------------------
// Configuration.

#define POW2_LOG2(n) ( \
	(n) >= 4096 ? 12 : \
	(n) >= 2048 ? 11 : \
	(n) >= 1024 ? 10 : \
	(n) >=  512 ?  9 : \
	(n) >=  256 ?  8 : \
	(n) >=  128 ?  7 : \
	(n) >=   64 ?  6 : \
	(n) >=   32 ?  5 : \
	(n) >=   16 ?  4 : \
	(n) >=    8 ?  3 : \
	(n) >=    4 ?  2 : \
	(n) >=    2 ?  1 : 0 \
)

#define HRC_WORLD_SIZE    1024
#define HRC_MAX_N         POW2_LOG2(HRC_WORLD_SIZE)
#define HRC_WG            16
#define HRC_ABS_THRESHOLD 0.1f
#define HRC_NUM_DEBUG 8   // debug modes 0..7

//--------------------------------------------------------------------------------------------------
// Shader loading.

CF_ComputeShader load_compute_shader(const char* path)
{
	char* src = cf_fs_read_entire_file_to_memory_and_nul_terminate(path, NULL);
	CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
	cf_free(src);
	return cs;
}
//--------------------------------------------------------------------------------------------------
// HRC state.

typedef struct Hrc
{
	CF_Canvas emissivity;
	CF_Canvas absorption;
	CF_StorageBuffer vrays_rad[HRC_MAX_N + 1];
	CF_StorageBuffer vrays_trn[HRC_MAX_N + 1];
	CF_StorageBuffer r_rad[2];
	CF_StorageBuffer r_zero;
	CF_StorageBuffer frustum[4];
	CF_Canvas fluence;
	CF_Material mat_trace;
	CF_Material mat_extend;
	CF_Material mat_merge;
	CF_Material mat_composite;
	CF_Material mat_copy;
	CF_ComputeShader cs_seed;
	CF_ComputeShader cs_trace;
	CF_ComputeShader cs_extend;
	CF_ComputeShader cs_merge;
	CF_ComputeShader cs_copy;
	CF_ComputeShader cs_composite;
	int vrays_w[HRC_MAX_N + 1];
	int debug_mode;
	int grid;
	int n;
	int trace_levels; // how many cascade levels use direct trace (0..n+1)
	int cminus1;     // c-1 gathering enabled
	int blend_boundary; // angle-based weight blending near ±45° edges
	int clamp_y;        // clamp DDA y-range to entry-cell footprint
	float blend_width;  // blend falloff width in direction slots
} Hrc;

Hrc hrc;

//--------------------------------------------------------------------------------------------------
// Helpers.

CF_StorageBuffer hrc_make_buf(int w, int h)
{
	CF_StorageBufferParams p = cf_storage_buffer_defaults(w * h * 8);
	p.compute_readable = true;
	p.compute_writable = true;
	return cf_make_storage_buffer(p);
}

CF_Canvas hrc_make_canvas(int w, int h, CF_PixelFormat fmt, CF_Filter filter)
{
	CF_CanvasParams p = cf_canvas_defaults(w, h);
	p.target.pixel_format = fmt;
	p.target.filter = filter;
	p.target.usage = CF_TEXTURE_USAGE_SAMPLER_BIT | CF_TEXTURE_USAGE_COLOR_TARGET_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_READ_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT;
	p.target.wrap_u = CF_WRAP_MODE_CLAMP_TO_EDGE;
	p.target.wrap_v = CF_WRAP_MODE_CLAMP_TO_EDGE;
	return cf_make_canvas(p);
}

int hrc_div_ceil(int a, int b)
{
	return (a + b - 1) / b;
}

//--------------------------------------------------------------------------------------------------
// Init / shutdown.

void hrc_set_grid(int grid)
{
	hrc.grid = grid;
	hrc.n = POW2_LOG2(grid);
	for (int i = 0; i <= hrc.n; i++) {
		int interval = 1 << i;
		int rays = interval + 1;
		int probes = grid >> i;
		hrc.vrays_w[i] = probes * rays;
	}
}

void hrc_init()
{
	CF_MEMSET(&hrc, 0, sizeof(hrc));

	// Start at half resolution.
	hrc_set_grid(HRC_WORLD_SIZE / 2);
	hrc.trace_levels = 3; // trace T_0..T_2, extend T_3..T_N
	hrc.cminus1 = 1;
	hrc.blend_width = 4.0f;

	// Precompute max T cascade dimensions for allocation.
	int max_vrays_w[HRC_MAX_N + 1];
	for (int i = 0; i <= HRC_MAX_N; i++) {
		int interval = 1 << i;
		int rays = interval + 1;
		int probes = HRC_WORLD_SIZE >> i;
		max_vrays_w[i] = probes * rays;
	}

	// Scene input canvases (nearest-neighbor: discrete pixel grid, no sRGB-space blending).
	hrc.emissivity = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	hrc.absorption = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);

	// Per-cascade T SSBOs (uvec2 per texel = 8 bytes, f16-packed). Allocated at max size.
	for (int i = 0; i <= HRC_MAX_N; i++) {
		hrc.vrays_rad[i] = hrc_make_buf(max_vrays_w[i], HRC_WORLD_SIZE);
		hrc.vrays_trn[i] = hrc_make_buf(max_vrays_w[i], HRC_WORLD_SIZE);
	}

	// R ping-pong SSBOs + zero buffer for R_N = 0.
	for (int i = 0; i < 2; i++)
		hrc.r_rad[i] = hrc_make_buf(HRC_WORLD_SIZE, HRC_WORLD_SIZE);
	hrc.r_zero = hrc_make_buf(HRC_WORLD_SIZE, HRC_WORLD_SIZE);
	{
		int sz = HRC_WORLD_SIZE * HRC_WORLD_SIZE * 8;
		void* zeros = cf_calloc(sz, 1);
		cf_update_storage_buffer(hrc.r_zero, zeros, sz);
		cf_free(zeros);
	}

	// Per-frustum output SSBOs (4 rotations).
	for (int i = 0; i < 4; i++)
		hrc.frustum[i] = hrc_make_buf(HRC_WORLD_SIZE, HRC_WORLD_SIZE);

	// Final output canvas (linear filtering for smooth display).
	hrc.fluence = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R8G8B8A8_UNORM, CF_FILTER_LINEAR);

	// Materials.
	hrc.mat_trace = cf_make_material();
	hrc.mat_extend = cf_make_material();
	hrc.mat_merge = cf_make_material();
	hrc.mat_composite = cf_make_material();
	hrc.mat_copy = cf_make_material();

	// Compute shaders (loaded from hrc_data/ next to the executable).
	hrc.cs_seed = load_compute_shader("/hrc_data/hrc_seed.c_shd");
	hrc.cs_trace = load_compute_shader("/hrc_data/hrc_trace.c_shd");
	hrc.cs_extend = load_compute_shader("/hrc_data/hrc_extend.c_shd");
	hrc.cs_merge = load_compute_shader("/hrc_data/hrc_merge.c_shd");
	hrc.cs_copy = load_compute_shader("/hrc_data/hrc_copy.c_shd");
	hrc.cs_composite = load_compute_shader("/hrc_data/hrc_composite.c_shd");
}

void hrc_shutdown()
{
	cf_destroy_canvas(hrc.emissivity);
	cf_destroy_canvas(hrc.absorption);
	for (int i = 0; i <= HRC_MAX_N; i++) {
		cf_destroy_storage_buffer(hrc.vrays_rad[i]);
		cf_destroy_storage_buffer(hrc.vrays_trn[i]);
	}
	for (int i = 0; i < 2; i++)
		cf_destroy_storage_buffer(hrc.r_rad[i]);
	cf_destroy_storage_buffer(hrc.r_zero);
	for (int i = 0; i < 4; i++)
		cf_destroy_storage_buffer(hrc.frustum[i]);
	cf_destroy_canvas(hrc.fluence);
	cf_destroy_material(hrc.mat_trace);
	cf_destroy_material(hrc.mat_extend);
	cf_destroy_material(hrc.mat_merge);
	cf_destroy_material(hrc.mat_composite);
	cf_destroy_material(hrc.mat_copy);
	cf_destroy_compute_shader(hrc.cs_seed);
	cf_destroy_compute_shader(hrc.cs_trace);
	cf_destroy_compute_shader(hrc.cs_extend);
	cf_destroy_compute_shader(hrc.cs_merge);
	cf_destroy_compute_shader(hrc.cs_copy);
	cf_destroy_compute_shader(hrc.cs_composite);
}

//--------------------------------------------------------------------------------------------------
// Cascade compute pipeline.

void hrc_compute()
{
	CF_Texture emiss_tex = cf_canvas_get_target(hrc.emissivity);
	CF_Texture absrp_tex = cf_canvas_get_target(hrc.absorption);
	CF_Texture fluence_tex = cf_canvas_get_target(hrc.fluence);

	int dim = hrc.grid;
	int N = hrc.n;

	for (int j = 0; j < 4; j++) {
		// Seed T_0 when no levels are traced (sample at probe, no raymarching).
		if (hrc.trace_levels == 0) {
			int params[4] = { 0, j, dim, hrc.vrays_w[0] };
			cf_material_set_texture_cs(hrc.mat_trace, "u_emissivity", emiss_tex);
			cf_material_set_texture_cs(hrc.mat_trace, "u_absorption", absrp_tex);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_rotate", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_world_size", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_curr_w", params + 3, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(hrc.vrays_w[0], HRC_WG),
				hrc_div_ceil(dim, HRC_WG),
				1
			);
			CF_StorageBuffer rw[2] = { hrc.vrays_rad[0], hrc.vrays_trn[0] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 2;
			cf_dispatch_compute(hrc.cs_seed, hrc.mat_trace, d);
		}

		// Direct trace T_0..T_{trace_levels-1}.
		for (int i = 0; i < hrc.trace_levels; i++) {
			int params[6] = { i, j, dim, hrc.vrays_w[i], hrc.blend_boundary, hrc.clamp_y };
			cf_material_set_texture_cs(hrc.mat_trace, "u_emissivity", emiss_tex);
			cf_material_set_texture_cs(hrc.mat_trace, "u_absorption", absrp_tex);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_rotate", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_world_size", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_curr_w", params + 3, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_blend_boundary", params + 4, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_clamp_y", params + 5, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_blend_width", &hrc.blend_width, CF_UNIFORM_TYPE_FLOAT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(hrc.vrays_w[i], HRC_WG),
				hrc_div_ceil(dim, HRC_WG),
				1
			);
			CF_StorageBuffer rw[2] = { hrc.vrays_rad[i], hrc.vrays_trn[i] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 2;
			cf_dispatch_compute(hrc.cs_trace, hrc.mat_trace, d);
		}

		// Extend remaining levels.
		int ext_start = hrc.trace_levels > 0 ? hrc.trace_levels : 1;
		for (int i = ext_start; i <= N; i++) {
			int params[4] = { i, dim, hrc.vrays_w[i - 1], hrc.vrays_w[i] };
			cf_material_set_uniform_cs(hrc.mat_extend, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_extend, "u_world_size", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_extend, "u_prev_w", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_extend, "u_curr_w", params + 3, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(hrc.vrays_w[i], HRC_WG),
				hrc_div_ceil(dim, HRC_WG),
				1
			);
			CF_StorageBuffer ro[2] = { hrc.vrays_rad[i - 1], hrc.vrays_trn[i - 1] };
			d.ro_buffers = ro;
			d.ro_buffer_count = 2;
			CF_StorageBuffer rw[2] = { hrc.vrays_rad[i], hrc.vrays_trn[i] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 2;
			cf_dispatch_compute(hrc.cs_extend, hrc.mat_extend, d);
		}

		// Merge R_{N-1} down to R_0.
		int r_ping = 0;
		for (int i = N - 1; i >= 0; i--) {
			int params[6] = { i, dim, hrc.vrays_w[i], hrc.vrays_w[i + 1], dim, dim };
			cf_material_set_uniform_cs(hrc.mat_merge, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_world_size", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_t_curr_w", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_t_next_w", params + 3, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_r_prev_w", params + 4, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_r_curr_w", params + 5, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(dim, HRC_WG),
				hrc_div_ceil(dim, HRC_WG),
				1
			);
			CF_StorageBuffer r_prev = (i == N - 1) ? hrc.r_zero : hrc.r_rad[1 - r_ping];
			CF_StorageBuffer ro[5] = {
				hrc.vrays_rad[i], hrc.vrays_trn[i],
				hrc.vrays_rad[i + 1], hrc.vrays_trn[i + 1],
				r_prev
			};
			d.ro_buffers = ro;
			d.ro_buffer_count = 5;
			CF_StorageBuffer rw[1] = { hrc.r_rad[r_ping] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 1;
			cf_dispatch_compute(hrc.cs_merge, hrc.mat_merge, d);
			r_ping = 1 - r_ping;
		}

		// Copy R_0 -> frustum[j].
		{
			int count = dim * dim;
			cf_material_set_uniform_cs(hrc.mat_copy, "u_count", &count, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(count, 256),
				1,
				1
			);
			CF_StorageBuffer ro[1] = { hrc.r_rad[1 - r_ping] };
			d.ro_buffers = ro;
			d.ro_buffer_count = 1;
			CF_StorageBuffer rw[1] = { hrc.frustum[j] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 1;
			cf_dispatch_compute(hrc.cs_copy, hrc.mat_copy, d);
		}
	}

	// Composite: c-1 gathering, sum 4 quadrants, cross blur, output.
	{
		int world = HRC_WORLD_SIZE;
		float abs_thresh = HRC_ABS_THRESHOLD;
		int debug = hrc.debug_mode <= 5 ? hrc.debug_mode : 0;
		cf_material_set_texture_cs(hrc.mat_composite, "u_emissivity", emiss_tex);
		cf_material_set_texture_cs(hrc.mat_composite, "u_absorption", absrp_tex);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_world_size", &world, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_grid_size", &dim, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_abs_threshold", &abs_thresh, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_debug_mode", &debug, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_cminus1", &hrc.cminus1, CF_UNIFORM_TYPE_INT, 1);

		CF_ComputeDispatch d = cf_compute_dispatch_defaults(
			hrc_div_ceil(world, HRC_WG),
			hrc_div_ceil(world, HRC_WG),
			1
		);
		CF_StorageBuffer ro[4] = {
			hrc.frustum[0], hrc.frustum[1],
			hrc.frustum[2], hrc.frustum[3]
		};
		d.ro_buffers = ro;
		d.ro_buffer_count = 4;
		CF_Texture rw_tex[1] = { fluence_tex };
		d.rw_textures = rw_tex;
		d.rw_texture_count = 1;
		cf_dispatch_compute(hrc.cs_composite, hrc.mat_composite, d);
	}
}

//--------------------------------------------------------------------------------------------------
// Demo scene state.

typedef struct OrbLight
{
	float radius;
	float speed;
	float angle;
	CF_Color color;
} OrbLight;

float time_acc = 0.0f;
int test_scene = 0;

OrbLight orbs[4];

void scene_init()
{
	orbs[0] = (OrbLight){ 140.0f, 0.7f, 0.0f,        cf_make_color_rgb_f(1.0f, 0.2f, 0.1f) };
	orbs[1] = (OrbLight){ 160.0f, -0.5f, CF_PI * 0.5f,   cf_make_color_rgb_f(0.1f, 1.0f, 0.2f) };
	orbs[2] = (OrbLight){ 120.0f, 0.9f, CF_PI,            cf_make_color_rgb_f(0.2f, 0.3f, 1.0f) };
	orbs[3] = (OrbLight){ 180.0f, -0.3f, CF_PI * 1.5f,    cf_make_color_rgb_f(1.0f, 0.9f, 0.1f) };
}

//--------------------------------------------------------------------------------------------------
// Input handling.

void handle_input()
{
	if (cf_key_just_pressed(CF_KEY_D)) {
		hrc.debug_mode = (hrc.debug_mode + 1) % HRC_NUM_DEBUG;
	}
	if (cf_key_just_pressed(CF_KEY_T)) {
		test_scene = !test_scene;
	}
	if (cf_key_just_pressed(CF_KEY_H)) {
		if (hrc.grid == HRC_WORLD_SIZE)
			hrc_set_grid(HRC_WORLD_SIZE / 2);
		else
			hrc_set_grid(HRC_WORLD_SIZE);
		if (hrc.trace_levels > hrc.n + 1)
			hrc.trace_levels = hrc.n + 1;
	}
	if (cf_key_just_pressed(CF_KEY_R)) {
		hrc.trace_levels = (hrc.trace_levels + 1) % (hrc.n + 2);
	}
	if (cf_key_just_pressed(CF_KEY_C)) {
		hrc.cminus1 = !hrc.cminus1;
	}
	if (cf_key_just_pressed(CF_KEY_B)) {
		hrc.blend_boundary = !hrc.blend_boundary;
	}
	if (cf_key_just_pressed(CF_KEY_Y)) {
		hrc.clamp_y = !hrc.clamp_y;
	}
	if (cf_key_just_pressed(CF_KEY_LEFTBRACKET)) {
		hrc.blend_width = cf_max(0.5f, hrc.blend_width - 0.5f);
	}
	if (cf_key_just_pressed(CF_KEY_RIGHTBRACKET)) {
		hrc.blend_width += 0.5f;
	}
}

//--------------------------------------------------------------------------------------------------
// Drawing helpers.

void begin_canvas_draw()
{
	cf_draw_push();
	float ws = (float)HRC_WORLD_SIZE;
	float half = ws * 0.5f;
	cf_draw_TSR_absolute(cf_v2(0, 0), cf_v2(1, 1), 0);
	cf_draw_projection(cf_ortho_2d(0, 0, ws, ws));
	cf_draw_translate(-half, -half);
}

void end_canvas_draw()
{
	cf_draw_pop();
}

void draw_circle_at(float x, float y, float r)
{
	cf_draw_circle_fill2(cf_v2(x, y), r);
}

// Push a render state with premultiplied alpha blending and the correct pixel format for f16 canvases.
void push_f16_render_state()
{
	CF_RenderState rs = cf_render_state_defaults();
	rs.blend.pixel_format = CF_PIXEL_FORMAT_R16G16B16A16_FLOAT;
	rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	rs.blend.rgb_op = CF_BLEND_OP_ADD;
	rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	rs.blend.alpha_op = CF_BLEND_OP_ADD;
	cf_draw_push_render_state(rs);
}

// Per-frame light positions (computed once, drawn into both canvases).
typedef struct Light
{
	float x, y, r;
	CF_Color color;
} Light;

#define MAX_LIGHTS 16
Light frame_lights[MAX_LIGHTS];
int frame_light_count;

// Compute all light positions for this frame and store them.
void update_lights()
{
	float half = (float)HRC_WORLD_SIZE * 0.5f;
	float ws = (float)HRC_WORLD_SIZE;
	float dt = CF_DELTA_TIME;
	time_acc += dt;
	frame_light_count = 0;

	// Orbiting lights.
	for (int i = 0; i < 4; i++) {
		orbs[i].angle += orbs[i].speed * dt;
		float cx = half + cosf(orbs[i].angle) * orbs[i].radius;
		float cy = half + sinf(orbs[i].angle) * orbs[i].radius;
		frame_lights[frame_light_count++] = (Light){ cx, cy, 15.0f, orbs[i].color };
	}

	// Corner accent lights.
	{
		float margin = 60.0f;
		CF_Color corners[4] = {
			cf_make_color_rgb_f(0.0f, 0.4f, 0.4f),
			cf_make_color_rgb_f(0.5f, 0.3f, 0.0f),
			cf_make_color_rgb_f(0.4f, 0.1f, 0.3f),
			cf_make_color_rgb_f(0.2f, 0.5f, 0.0f),
		};
		float lx[4] = { margin, ws - margin, margin, ws - margin };
		float ly[4] = { margin, margin, ws - margin, ws - margin };
		for (int i = 0; i < 4; i++) {
			frame_lights[frame_light_count++] = (Light){ lx[i], ly[i], 8.0f, corners[i] };
		}
	}
}

// Draw all lights as colored circles (for emissivity canvas).
void draw_lights_emissive()
{
	for (int i = 0; i < frame_light_count; i++) {
		cf_draw_push_color(frame_lights[i].color);
		draw_circle_at(frame_lights[i].x, frame_lights[i].y, frame_lights[i].r);
		cf_draw_pop_color();
	}
}

// Draw all lights as white circles (for absorption canvas).
// Light sources are opaque emitters -- they must absorb to emit.
void draw_lights_absorbing()
{
	cf_draw_push_color(cf_make_color_rgb_f(1.0f, 1.0f, 1.0f));
	for (int i = 0; i < frame_light_count; i++) {
		draw_circle_at(frame_lights[i].x, frame_lights[i].y, frame_lights[i].r);
	}
	cf_draw_pop_color();
}

//--------------------------------------------------------------------------------------------------
// Draw emissivity (lights).

void draw_emissivity()
{
	begin_canvas_draw();
	push_f16_render_state();

	if (test_scene) {
		float half = (float)HRC_WORLD_SIZE * 0.5f;
		float s = (float)HRC_WORLD_SIZE / (float)hrc.grid;
		cf_draw_push_color(cf_make_color_rgb_f(1.0f, 1.0f, 1.0f));
		cf_draw_quad_fill(cf_make_aabb(cf_v2(half - s, half - s), cf_v2(half + s, half + s)), 0);
		cf_draw_pop_color();
	} else {
		draw_lights_emissive();
	}

	cf_draw_pop_render_state();
	cf_render_to(hrc.emissivity, true);
	end_canvas_draw();
}

//--------------------------------------------------------------------------------------------------
// Draw absorption (shadow casters + light sources).

void draw_absorption()
{
	begin_canvas_draw();
	push_f16_render_state();

	if (test_scene) {
		float half = (float)HRC_WORLD_SIZE * 0.5f;
		float s = (float)HRC_WORLD_SIZE / (float)hrc.grid;
		cf_draw_push_color(cf_make_color_rgb_f(1.0f, 1.0f, 1.0f));
		cf_draw_quad_fill(cf_make_aabb(cf_v2(half - s, half - s), cf_v2(half + s, half + s)), 0);
		cf_draw_pop_color();
	} else {
		cf_draw_push_color(cf_make_color_rgb_f(1.0f, 1.0f, 1.0f));

		float ws = (float)HRC_WORLD_SIZE;
		float half = ws * 0.5f;

		// 5 circular pillars in quincunx pattern.
		draw_circle_at(half, half, 25.0f);
		draw_circle_at(half - 110.0f, half - 110.0f, 20.0f);
		draw_circle_at(half + 110.0f, half - 110.0f, 20.0f);
		draw_circle_at(half - 110.0f, half + 110.0f, 20.0f);
		draw_circle_at(half + 110.0f, half + 110.0f, 20.0f);

		// 2 angled wall segments.
		cf_draw_quad_fill2(
			cf_v2(100.0f, 200.0f), cf_v2(110.0f, 200.0f),
			cf_v2(200.0f, 310.0f), cf_v2(190.0f, 310.0f), 0
		);
		cf_draw_quad_fill2(
			cf_v2(400.0f, 200.0f), cf_v2(390.0f, 200.0f),
			cf_v2(300.0f, 310.0f), cf_v2(310.0f, 310.0f), 0
		);

		// 3 small scattered square blocks.
		cf_draw_quad_fill(cf_make_aabb(cf_v2(340.0f, 400.0f), cf_v2(365.0f, 425.0f)), 0);
		cf_draw_quad_fill(cf_make_aabb(cf_v2(150.0f, 380.0f), cf_v2(175.0f, 405.0f)), 0);
		cf_draw_quad_fill(cf_make_aabb(cf_v2(370.0f, 120.0f), cf_v2(395.0f, 145.0f)), 0);

		cf_draw_pop_color();

		// Light sources must absorb to emit (radiative transfer: rad = emiss * (1 - T)).
		draw_lights_absorbing();
	}

	cf_draw_pop_render_state();
	cf_render_to(hrc.absorption, true);
	end_canvas_draw();
}

//--------------------------------------------------------------------------------------------------
// Display fluence onto screen.

void display_fluence()
{
	CF_Canvas display = hrc.fluence;
	if (hrc.debug_mode == 6) display = hrc.emissivity;
	else if (hrc.debug_mode == 7) display = hrc.absorption;

	float ws = (float)HRC_WORLD_SIZE;
	cf_draw_canvas(display, cf_v2(0, 0), cf_v2(ws, ws));
}

//--------------------------------------------------------------------------------------------------
// HUD overlay.

void draw_hud()
{
	const char* mode_names[] = {
		"0: Normal (blur)",
		"1: Rot 0 (+x)",
		"2: Rot 1 (+y)",
		"3: Rot 2 (-x)",
		"4: Rot 3 (-y)",
		"5: All rotations (no blur)",
		"6: Emissivity",
		"7: Absorption",
	};

	char buf[512];
	snprintf(buf, sizeof(buf),
		"[B] Blend boundary: %s\n"
		"[[] []] Blend width: %.1f\n"
		"[C] c-1 gathering: %s\n"
		"[D] Debug: %s\n"
		"[H] Grid: %d (%s)\n"
		"[R] Trace: %d / %d\n"
		"[T] Test scene: %s\n"
		"[Y] Clamp Y: %s",
		hrc.blend_boundary ? "on" : "off",
		hrc.blend_width,
		hrc.cminus1 ? "on" : "off",
		mode_names[hrc.debug_mode],
		hrc.grid, hrc.grid == HRC_WORLD_SIZE ? "full" : "half",
		hrc.trace_levels, hrc.n + 1,
		test_scene ? "on" : "off",
		hrc.clamp_y ? "on" : "off"
	);

	float half = (float)HRC_WORLD_SIZE * 0.5f;
	cf_draw_push_layer(1);
	cf_draw_push_color(cf_color_white());
	cf_push_font_size(16.0f);
	cf_draw_text(buf, cf_v2(-half + 10.0f, half - 20.0f), -1);
	cf_pop_font_size();
	cf_draw_pop_color();
	cf_draw_pop_layer();
}

//--------------------------------------------------------------------------------------------------
// Entry point.

int main(int argc, char* argv[])
{
	cf_make_app("HRC - Holographic Radiance Cascades", 0, 0, 0, HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);

	hrc_init();
	scene_init();
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		cf_draw_push_shape_aa(0);
		handle_input();
		update_lights();
		draw_emissivity();
		draw_absorption();
		hrc_compute();
		display_fluence();
		draw_hud();
		cf_app_draw_onto_screen(true);
	}

	hrc_shutdown();
	cf_destroy_app();
	return 0;
}
