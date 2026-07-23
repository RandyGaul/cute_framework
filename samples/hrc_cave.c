// Bioluminal -- a bioluminescent cave pool lit entirely by HRC 2D global illumination.
//
// Interactive mood piece in the spirit of Animal Well: near-total darkness, a single
// moon shaft falling through a cracked ceiling, and a pool of water that glows
// cyan-green when disturbed. Stir the water with the mouse, click for a splash.
//
// Showcases, on top of the HRC pipeline from samples/hrc.c:
//   - CSG shape groups for the cave rock (smooth unions/subtracts, retained draw lists)
//   - HDR draw colors: scene canvases hold physical units (linear emission,
//     absorption coefficient per world pixel) with no 8-bit encode dance
//   - CPU particle water: position-based fluids (XSPH viscosity, unified
//     ground-SDF contact, soft sleep) + a 1D spring heightfield surface with drip rings,
//     screen-space reflection, foam, spray, and agitation-driven bioluminescence
//   - Coroutine drip script with synthesized cave plink audio
//   - A custom SDF jellyfish (cf_make_custom_shape)
//   - Directional-fluence rim lighting on the rock silhouette
//   - Curve-text runes revealed by bounce light, swaying Bezier root strands
//
// Controls: mouse move = stir, click = splash, F1 = overlay with feature toggles.
//
// Env vars for scripted runs:
//   HRC_CAVE_AUTOSTIR=1        virtual mouse swishes through the pool
//   HRC_CAVE_SCREENSHOT_MODE=1 fixed drip RNG seed
//   HRC_CAVE_PERF=1            print frame ms every 60 frames
//   HRC_CAVE_HDRTEST=1         draw-color HDR smoke test at startup
//   HRC_CAVE_VIEW=...          raw canvas view: emissivity|absorption|diffuse|density
//   HRC_CAVE_GI/FOG/RIM/WATER/BOUNCE/OVERLAY=0|1   toggle overrides
//   HRC_CAVE_SKIP=...          skip draws for perf bisection (w/r/j/k/p)

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--------------------------------------------------------------------------------------------------
// Configuration.
//
// The cave is authored in a fixed 1024x1024 DESIGN space (CAVE_REF). The runtime
// WORLD (scene canvases + GI grid + window) can be any non-square resolution: the
// design scene is uniformly scaled by scene_s and centered into the world, so
// shapes never distort and a square world reduces exactly to the old 1x layout.
// The HRC cascade runs genuinely rectangular over the full world grid.

#define CAVE_REF      1024      // design-space resolution the scene is authored in
#define CAVE_UPSCALE  2         // world pixels per cascade grid cell (per axis)
#define CAVE_N_MAX    11        // array sizing: log2_ceil of the largest grid axis
#define CAVE_TRACE_LEVELS 3
#define CAVE_WG    16
#define CAVE_ABS_THRESHOLD 0.1f

// Runtime world / grid dimensions (set in main() from the window size).
int world_w = CAVE_REF, world_h = CAVE_REF;   // scene canvas + display resolution
int grid_w = CAVE_REF / CAVE_UPSCALE;         // cascade probe lattice (world / upscale)
int grid_h = CAVE_REF / CAVE_UPSCALE;
int n_horiz = 9, n_vert = 9, n_max = 9;       // per-axis cascade depth (log2_ceil grid)

// Design-space -> world transform: uniform scale + centering offset.
float scene_s = 1.0f;
float scene_ox = 0.0f, scene_oy = 0.0f;

CF_V2 design_to_world(CF_V2 p) { return cf_v2(p.x * scene_s + scene_ox, p.y * scene_s + scene_oy); }
CF_V2 world_to_design(CF_V2 p) { return cf_v2((p.x - scene_ox) / scene_s, (p.y - scene_oy) / scene_s); }

#define WATERLINE 275.0f

// Moon crack through the ceiling: centerline x = CRACK_X0 + (y - CRACK_Y0) * CRACK_SLOPE,
// ~15 degrees off vertical (light falls down-and-left).
#define CRACK_Y0     790.0f
#define CRACK_X0     380.0f
#define CRACK_SLOPE  0.268f  // tan(15 deg)
#define CRACK_HALF_W 17.0f

//--------------------------------------------------------------------------------------------------
// Shader loading.

CF_ComputeShader load_compute_shader(const char* path)
{
	char* src = cf_fs_read_entire_file_to_memory_and_nul_terminate(path, NULL);
	if (!src) {
		printf("FATAL: failed to read %s\n", path);
		exit(1);
	}
	CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
	cf_free(src);
	return cs;
}

CF_Shader load_draw_shader(const char* path)
{
	char* src = cf_fs_read_entire_file_to_memory_and_nul_terminate(path, NULL);
	if (!src) {
		printf("FATAL: failed to read %s\n", path);
		exit(1);
	}
	CF_Shader shd = cf_make_draw_shader_from_source(src);
	cf_free(src);
	return shd;
}

//--------------------------------------------------------------------------------------------------
// HRC state (trimmed from samples/hrc.c: fixed grid 512, trace 3, cminus1 on,
// minmax upscale, dense directions, no debug modes).

typedef struct Hrc
{
	CF_Canvas emissivity;
	CF_Canvas absorption;
	CF_Canvas diffuse;      // per-pixel albedo for multibounce feedback
	CF_Canvas fluence;      // tonemapped display (rgba8)
	CF_Canvas fluence_lin;  // linear fluence (feedback input)
	CF_Canvas minmax;
	CF_StorageBuffer vrays_rad[CAVE_N_MAX + 1];
	CF_StorageBuffer vrays_trn[CAVE_N_MAX + 1];
	CF_StorageBuffer r_rad[2];
	CF_StorageBuffer r_zero;
	CF_StorageBuffer frustum[4];
	CF_Material mat_trace;
	CF_Material mat_extend;
	CF_Material mat_merge;
	CF_Material mat_composite;
	CF_Material mat_copy;
	CF_Material mat_minmax;
	CF_Material mat_feedback;
	CF_ComputeShader cs_trace;
	CF_ComputeShader cs_extend;
	CF_ComputeShader cs_merge;
	CF_ComputeShader cs_copy;
	CF_ComputeShader cs_composite;
	CF_ComputeShader cs_minmax;
	CF_ComputeShader cs_feedback;
	int vrays_w_horiz[CAVE_N_MAX + 1]; // dense T widths for horizontal rotations (cascade axis = width)
	int vrays_w_vert[CAVE_N_MAX + 1];  // dense T widths for vertical rotations (cascade axis = height)
} Hrc;

Hrc hrc;

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

int hrc_log2_ceil(int x)
{
	int n = 0;
	while ((1 << n) < x) n++;
	return n;
}

// Dense-direction T width at cascade i for a cascade axis of `dim` grid cells:
//   num_probes(i) = ceil(dim / 2^i),  directions(i) = 2^(i+1) + 1.
// ceil_div (not >>) so a non-power-of-two axis still collapses to one probe at n.
int hrc_vrays_w(int dim, int i)
{
	int probes = hrc_div_ceil(dim, 1 << i);
	int rays = 2 * (1 << i) + 1;
	return probes * rays;
}

// Dense R (angular fluence) width at cascade i: num_probes(i) * 2^(i+1).
int hrc_r_w(int dim, int i)
{
	return hrc_div_ceil(dim, 1 << i) * (2 * (1 << i));
}

void hrc_init()
{
	CF_MEMSET(&hrc, 0, sizeof(hrc));

	// Per-axis cascade depth. Horizontal rotations (0,2) cascade along the width
	// (grid_w); vertical rotations (1,3) along the height (grid_h).
	n_horiz = hrc_log2_ceil(grid_w);
	n_vert = hrc_log2_ceil(grid_h);
	n_max = n_horiz > n_vert ? n_horiz : n_vert;

	for (int i = 0; i <= n_horiz; i++) hrc.vrays_w_horiz[i] = hrc_vrays_w(grid_w, i);
	for (int i = 0; i <= n_vert; i++)  hrc.vrays_w_vert[i]  = hrc_vrays_w(grid_h, i);

	hrc.emissivity = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	hrc.absorption = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	hrc.diffuse = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	hrc.fluence = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R8G8B8A8_UNORM, CF_FILTER_LINEAR);
	hrc.fluence_lin = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	hrc.minmax = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16_FLOAT, CF_FILTER_NEAREST);

	// T buffers: sized to the larger of the two rotation groups per cascade,
	// horiz(vrays_w_horiz[i] * grid_h) vs vert(vrays_w_vert[i] * grid_w).
	for (int i = 0; i <= n_max; i++) {
		int hw = (i <= n_horiz) ? hrc.vrays_w_horiz[i] * grid_h : 0;
		int vw = (i <= n_vert)  ? hrc.vrays_w_vert[i]  * grid_w : 0;
		int elems = hw > vw ? hw : vw;
		hrc.vrays_rad[i] = hrc_make_buf(elems, 1);
		hrc.vrays_trn[i] = hrc_make_buf(elems, 1);
	}

	// R ping-pong + zero buffer: max over cascades and rotation groups of
	// (r_w(cascade_dim, i) * cross_dim).
	int max_r_elems = 0;
	for (int i = 0; i < n_horiz; i++) {
		int e = hrc_r_w(grid_w, i) * grid_h;
		if (e > max_r_elems) max_r_elems = e;
	}
	for (int i = 0; i < n_vert; i++) {
		int e = hrc_r_w(grid_h, i) * grid_w;
		if (e > max_r_elems) max_r_elems = e;
	}
	for (int i = 0; i < 2; i++)
		hrc.r_rad[i] = hrc_make_buf(max_r_elems, 1);
	hrc.r_zero = hrc_make_buf(max_r_elems, 1);
	{
		int sz = max_r_elems * 8;
		void* zeros = cf_calloc(sz, 1);
		cf_update_storage_buffer(hrc.r_zero, zeros, sz);
		cf_free(zeros);
	}

	// Per-frustum R_0 output: cascade_dim*2 * cross_dim = 2 * grid_w * grid_h
	// either rotation group.
	for (int i = 0; i < 4; i++)
		hrc.frustum[i] = hrc_make_buf(2 * grid_w * grid_h, 1);

	hrc.mat_trace = cf_make_material();
	hrc.mat_extend = cf_make_material();
	hrc.mat_merge = cf_make_material();
	hrc.mat_composite = cf_make_material();
	hrc.mat_copy = cf_make_material();
	hrc.mat_minmax = cf_make_material();
	hrc.mat_feedback = cf_make_material();

	hrc.cs_trace = load_compute_shader("/hrc_cave_data/hrc_trace.c_shd");
	hrc.cs_extend = load_compute_shader("/hrc_cave_data/hrc_extend.c_shd");
	hrc.cs_merge = load_compute_shader("/hrc_cave_data/hrc_merge.c_shd");
	hrc.cs_copy = load_compute_shader("/hrc_cave_data/hrc_copy.c_shd");
	hrc.cs_composite = load_compute_shader("/hrc_cave_data/hrc_composite.c_shd");
	hrc.cs_minmax = load_compute_shader("/hrc_cave_data/hrc_minmax.c_shd");
	hrc.cs_feedback = load_compute_shader("/hrc_cave_data/hrc_feedback.c_shd");
}

void hrc_shutdown()
{
	cf_destroy_canvas(hrc.emissivity);
	cf_destroy_canvas(hrc.absorption);
	cf_destroy_canvas(hrc.diffuse);
	cf_destroy_canvas(hrc.fluence);
	cf_destroy_canvas(hrc.fluence_lin);
	cf_destroy_canvas(hrc.minmax);
	for (int i = 0; i <= n_max; i++) {
		cf_destroy_storage_buffer(hrc.vrays_rad[i]);
		cf_destroy_storage_buffer(hrc.vrays_trn[i]);
	}
	for (int i = 0; i < 2; i++)
		cf_destroy_storage_buffer(hrc.r_rad[i]);
	cf_destroy_storage_buffer(hrc.r_zero);
	for (int i = 0; i < 4; i++)
		cf_destroy_storage_buffer(hrc.frustum[i]);
	cf_destroy_material(hrc.mat_trace);
	cf_destroy_material(hrc.mat_extend);
	cf_destroy_material(hrc.mat_merge);
	cf_destroy_material(hrc.mat_composite);
	cf_destroy_material(hrc.mat_copy);
	cf_destroy_material(hrc.mat_minmax);
	cf_destroy_material(hrc.mat_feedback);
	cf_destroy_compute_shader(hrc.cs_trace);
	cf_destroy_compute_shader(hrc.cs_extend);
	cf_destroy_compute_shader(hrc.cs_merge);
	cf_destroy_compute_shader(hrc.cs_copy);
	cf_destroy_compute_shader(hrc.cs_composite);
	cf_destroy_compute_shader(hrc.cs_minmax);
	cf_destroy_compute_shader(hrc.cs_feedback);
}

//--------------------------------------------------------------------------------------------------
// Feature toggles (F1 overlay).

const char* skip_flags = ""; // HRC_CAVE_SKIP: w=water draws, r=roots, j=jelly, k=rock lists, p=particle sim

int skip(char c)
{
	return strchr(skip_flags, c) != NULL;
}

int show_overlay = 0;
int gi_on = 1;
int fog_on = 1;
int rim_on = 1;
int water_sdf_on = 1;
int bounce_on = 1;
int paused = 0;
float smoothed_fps = 60.0f;

// HRC_CAVE_TESTLIGHT: isolated-light debug mode that exercises the rectangular
// cascade in isolation (no water/rock/jelly/drip/runes/roots, no design-space
// letterbox). The scene is authored DIRECTLY in world space filling the window.
//   1 = a single centered 8x8 emissive square (the paper's minimum supported
//       emitter, matching samples/hrc.c's known-good static test light). Pure
//       radial-symmetry test: a centered point light must read the same at any
//       world aspect (circle, no directional bias, no spokes).
//   2 = the 8x8 light plus two box occluders offset from center (shadow-shape
//       check: shadows must be straight and correctly oriented).
int testlight = 0;
float testlight_e = 2.0f; // HRC_CAVE_TESTLIGHT_E: emitter linear emission (per channel)
float testlight_r = 8.0f; // HRC_CAVE_TESTLIGHT_R: emitter radius in world units. Default 8,
                          // the paper's minimum supported source size -- a filled disc of this
                          // radius should spread/soften the ±45° seam spokes that a near-point
                          // source maximizes. Sweep smaller (1/2) to reproduce the point-source X.
float testlight_ox = 0.0f, testlight_oy = 0.0f; // HRC_CAVE_TESTLIGHT_OX/OY: light offset from
                                                // world center, as a fraction of world_w/world_h

//--------------------------------------------------------------------------------------------------
// Cascade compute pipeline (fixed config).

void hrc_compute()
{
	CF_Texture emiss_tex = cf_canvas_get_target(hrc.emissivity);
	CF_Texture absrp_tex = cf_canvas_get_target(hrc.absorption);
	CF_Texture fluence_tex = cf_canvas_get_target(hrc.fluence);
	CF_Texture minmax_tex = cf_canvas_get_target(hrc.minmax);

	int upscale = CAVE_UPSCALE;

	cf_material_set_uniform_cs(hrc.mat_trace, "u_upscale", &upscale, CF_UNIFORM_TYPE_INT, 1);

	for (int j = 0; j < 4; j++) {
		// Per-rotation dims: horizontal rotations (0,2) cascade along the width,
		// vertical rotations (1,3) along the height. cdim = cascade axis grid
		// dimension, xdim = cross axis grid dimension.
		int horiz = (j == 0 || j == 2);
		int cdim = horiz ? grid_w : grid_h;
		int xdim = horiz ? grid_h : grid_w;
		int N = horiz ? n_horiz : n_vert;
		int* vw = horiz ? hrc.vrays_w_horiz : hrc.vrays_w_vert;

		// Direct trace T_0..T_2.
		for (int i = 0; i < CAVE_TRACE_LEVELS && i <= N; i++) {
			int params[6] = { i, j, cdim, xdim, vw[i], 0 };
			cf_material_set_texture_cs(hrc.mat_trace, "u_emissivity", emiss_tex);
			cf_material_set_texture_cs(hrc.mat_trace, "u_absorption", absrp_tex);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_rotate", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_cascade_dim", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_cross_dim", params + 3, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_curr_w", params + 4, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_world_w", &world_w, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_world_h", &world_h, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(vw[i], CAVE_WG),
				hrc_div_ceil(xdim, CAVE_WG),
				1
			);
			CF_StorageBuffer rw[2] = { hrc.vrays_rad[i], hrc.vrays_trn[i] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 2;
			cf_dispatch_compute(hrc.cs_trace, hrc.mat_trace, d);
		}

		// Extend remaining levels.
		for (int i = CAVE_TRACE_LEVELS; i <= N; i++) {
			int params[5] = { i, cdim, xdim, vw[i - 1], vw[i] };
			cf_material_set_uniform_cs(hrc.mat_extend, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_extend, "u_cascade_dim", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_extend, "u_cross_dim", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_extend, "u_prev_w", params + 3, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_extend, "u_curr_w", params + 4, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(vw[i], CAVE_WG),
				hrc_div_ceil(xdim, CAVE_WG),
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
			int r_prev_w = hrc_r_w(cdim, i + 1);
			int r_curr_w = hrc_r_w(cdim, i);
			int params[7] = { i, cdim, xdim, vw[i], vw[i + 1], r_prev_w, r_curr_w };
			cf_material_set_uniform_cs(hrc.mat_merge, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_cascade_dim", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_cross_dim", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_t_curr_w", params + 3, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_t_next_w", params + 4, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_r_prev_w", params + 5, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_r_curr_w", params + 6, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(r_curr_w, CAVE_WG),
				hrc_div_ceil(xdim, CAVE_WG),
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

		// Copy R_0 -> frustum[j]. R_0 element count = cascade_dim*2 * cross_dim.
		{
			int count = hrc_r_w(cdim, 0) * xdim;
			cf_material_set_uniform_cs(hrc.mat_copy, "u_count", &count, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(count, 256), 1, 1);
			CF_StorageBuffer ro[1] = { hrc.r_rad[1 - r_ping] };
			d.ro_buffers = ro;
			d.ro_buffer_count = 1;
			CF_StorageBuffer rw[1] = { hrc.frustum[j] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 1;
			cf_dispatch_compute(hrc.cs_copy, hrc.mat_copy, d);
		}
	}

	// Minmax absorption pass (for gated bilinear upscale in composite).
	{
		cf_material_set_texture_cs(hrc.mat_minmax, "u_absorption", absrp_tex);
		cf_material_set_uniform_cs(hrc.mat_minmax, "u_world_w", &world_w, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_minmax, "u_world_h", &world_h, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_minmax, "u_grid_w", &grid_w, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_minmax, "u_grid_h", &grid_h, CF_UNIFORM_TYPE_INT, 1);

		CF_ComputeDispatch d = cf_compute_dispatch_defaults(
			hrc_div_ceil(world_w, CAVE_WG),
			hrc_div_ceil(world_h, CAVE_WG),
			1
		);
		CF_Texture rw_tex[1] = { minmax_tex };
		d.rw_textures = rw_tex;
		d.rw_texture_count = 1;
		cf_dispatch_compute(hrc.cs_minmax, hrc.mat_minmax, d);
	}

	// Composite: c-1 gathering, sum 4 quadrants, rim light, tonemap, output.
	{
		float abs_thresh = CAVE_ABS_THRESHOLD;
		float exposure = 2.0f;
		float rim_strength = 0.3f;
		float rock_thresh = 5.0f;
		cf_material_set_texture_cs(hrc.mat_composite, "u_emissivity", emiss_tex);
		cf_material_set_texture_cs(hrc.mat_composite, "u_absorption", absrp_tex);
		cf_material_set_texture_cs(hrc.mat_composite, "u_minmax", minmax_tex);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_world_w", &world_w, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_world_h", &world_h, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_grid_w", &grid_w, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_grid_h", &grid_h, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_abs_threshold", &abs_thresh, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_exposure", &exposure, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_rim", &rim_on, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_rim_strength", &rim_strength, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_rock_thresh", &rock_thresh, CF_UNIFORM_TYPE_FLOAT, 1);

		CF_ComputeDispatch d = cf_compute_dispatch_defaults(
			hrc_div_ceil(world_w, CAVE_WG),
			hrc_div_ceil(world_h, CAVE_WG),
			1
		);
		CF_StorageBuffer ro[4] = {
			hrc.frustum[0], hrc.frustum[1],
			hrc.frustum[2], hrc.frustum[3]
		};
		d.ro_buffers = ro;
		d.ro_buffer_count = 4;
		CF_Texture rw_tex[2] = { fluence_tex, cf_canvas_get_target(hrc.fluence_lin) };
		d.rw_textures = rw_tex;
		d.rw_texture_count = 2;
		cf_dispatch_compute(hrc.cs_composite, hrc.mat_composite, d);
	}
}

// Multibounce feedback: inject last frame's fluence, tinted by albedo, into emissivity.
void hrc_feedback()
{
	cf_material_set_uniform_cs(hrc.mat_feedback, "u_world_w", &world_w, CF_UNIFORM_TYPE_INT, 1);
	cf_material_set_uniform_cs(hrc.mat_feedback, "u_world_h", &world_h, CF_UNIFORM_TYPE_INT, 1);

	CF_ComputeDispatch d = cf_compute_dispatch_defaults(
		hrc_div_ceil(world_w, CAVE_WG),
		hrc_div_ceil(world_h, CAVE_WG),
		1
	);
	CF_Texture ro_tex[3] = {
		cf_canvas_get_target(hrc.fluence_lin),
		cf_canvas_get_target(hrc.diffuse),
		cf_canvas_get_target(hrc.absorption)
	};
	d.ro_textures = ro_tex;
	d.ro_texture_count = 3;
	CF_Texture rw_tex[1] = { cf_canvas_get_target(hrc.emissivity) };
	d.rw_textures = rw_tex;
	d.rw_texture_count = 1;
	cf_dispatch_compute(hrc.cs_feedback, hrc.mat_feedback, d);
}

//--------------------------------------------------------------------------------------------------
// Drawing helpers.

// Project the world canvas (world_w x world_h) and fold in the design->world
// scene transform, so all design-authored draws (rock, water, jelly, ...) land
// uniformly scaled + centered into the world. At a square world this is the
// identity 1x layout.
// The design box mapped into the world, as a framebuffer-pixel scissor rect
// (y-down from the top). Clips scene draws to the design region so content
// authored just past the design edge (the ceiling overshoot + moon emitter
// above y=1024) stays out of the letterbox margins -- matching the square case,
// where the canvas bounds did the clipping. Reduces to the full canvas at 1x.
CF_Rect scene_scissor()
{
	int box = (int)(CAVE_REF * scene_s + 0.5f);
	CF_Rect r;
	r.x = (int)(scene_ox + 0.5f);
	r.w = box;
	r.y = world_h - ((int)(scene_oy + 0.5f) + box);
	r.h = box;
	return r;
}

void begin_canvas_draw()
{
	cf_draw_push();
	cf_draw_push_scissor(scene_scissor());
	cf_draw_TSR_absolute(cf_v2(0, 0), cf_v2(1, 1), 0);
	cf_draw_projection(cf_ortho_2d(0, 0, (float)world_w, (float)world_h));
	cf_draw_translate(-world_w * 0.5f, -world_h * 0.5f);
	cf_draw_translate(scene_ox, scene_oy);
	cf_draw_scale(scene_s, scene_s);
}

void end_canvas_draw()
{
	cf_draw_pop_scissor();
	cf_draw_pop();
}

// Design-space projection for the density canvas (which is authored and sampled
// purely in the fixed REF design box, independent of the world resolution). The
// water shader reconstructs design coords from the density uv, then maps to the
// world fluence itself.
void begin_design_draw()
{
	cf_draw_push();
	float ws = (float)CAVE_REF;
	float half = ws * 0.5f;
	cf_draw_TSR_absolute(cf_v2(0, 0), cf_v2(1, 1), 0);
	cf_draw_projection(cf_ortho_2d(0, 0, ws, ws));
	cf_draw_translate(-half, -half);
}

// Premultiplied alpha blending onto an f16 canvas: alpha-1 draws replace what's below.
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

// Pure additive blending onto an f16 canvas (particle density splats).
void push_additive_f16_render_state()
{
	CF_RenderState rs = cf_render_state_defaults();
	rs.blend.pixel_format = CF_PIXEL_FORMAT_R16G16B16A16_FLOAT;
	rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.rgb_op = CF_BLEND_OP_ADD;
	rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.alpha_op = CF_BLEND_OP_ADD;
	cf_draw_push_render_state(rs);
}

//--------------------------------------------------------------------------------------------------
// Unified ground SDF. ONE primitive table drives everything that must agree:
//   - the CPU particle collision (project onto the SDF along its gradient)
//   - the rendered rock silhouette (the SAME primitives drawn as a CSG shape
//     group, so visuals match physics exactly)
//   - the HRC absorption/diffuse canvases (drawn from those shape groups)
//   - a baked 256x256 R32F distance texture that cave_water.shd samples to
//     suppress the surface line under rock and place meniscus dabs
//
// The fold below mirrors CF's shape-group math (csg_smin in the tile shader):
// smooth-union of irregular circles/capsules forming a rocky floor, two
// smooth-subtracted scoops carving an ASYMMETRIC pool (deep bowl on the west,
// shallow shelf "beach" on the east), and a rock knuckle unioned back in after
// the scoops so it pokes through the waterline mid-pool.

typedef struct GroundPrim
{
	int kind;   // 0 = circle, 1 = capsule, 2 = axis-aligned box
	int op;     // CF_ShapeOp folding this primitive into the running distance
	float k;    // smoothing constant
	CF_V2 a, b; // circle: a = center; capsule: a/b = endpoints; box: a = min, b = max
	float r;    // radius (unused for boxes)
} GroundPrim;

static const GroundPrim ground_prims[] = {
	// Rocky bedrock + shore masses.
	{ 2, CF_SHAPE_OP_UNION, 18.0f, { 0, 0 },       { 1024, 120 }, 0 },      // bedrock slab
	{ 0, CF_SHAPE_OP_UNION, 18.0f, { 70, 280 },    { 0, 0 },      150.0f }, // left shore mound (steep side)
	{ 0, CF_SHAPE_OP_UNION, 18.0f, { 20, 120 },    { 0, 0 },      170.0f }, // left base
	{ 0, CF_SHAPE_OP_UNION, 18.0f, { 250, -20 },   { 0, 0 },      170.0f }, // deep floor west
	{ 0, CF_SHAPE_OP_UNION, 18.0f, { 420, -10 },   { 0, 0 },      170.0f }, // deep floor east
	{ 0, CF_SHAPE_OP_UNION, 18.0f, { 540, 45 },    { 0, 0 },      175.0f }, // mid-pool sill under the knuckle
	{ 0, CF_SHAPE_OP_UNION, 18.0f, { 760, 180 },   { 0, 0 },      180.0f }, // shelf mass (scooped into the beach)
	{ 0, CF_SHAPE_OP_UNION, 18.0f, { 900, 170 },   { 0, 0 },      150.0f }, // beach rise
	{ 0, CF_SHAPE_OP_UNION, 18.0f, { 1030, 340 },  { 0, 0 },      200.0f }, // right wall base (rune wall)
	{ 1, CF_SHAPE_OP_UNION, 18.0f, { 18, 420 },    { 35, 1060 },  60.0f },  // left wall
	{ 1, CF_SHAPE_OP_UNION, 18.0f, { 1008, 420 },  { 995, 1060 }, 60.0f },  // right wall
	{ 0, CF_SHAPE_OP_UNION, 18.0f, { 380, 130 },   { 0, 0 },      45.0f },  // deep-floor boulder
	{ 0, CF_SHAPE_OP_UNION, 18.0f, { 790, 232 },   { 0, 0 },      24.0f },  // shelf pebble (submerged bump)
	// Stalagmite on the dry left shore.
	{ 1, CF_SHAPE_OP_UNION, 14.0f, { 128, 400 },   { 134, 470 },  13.0f },
	{ 0, CF_SHAPE_OP_UNION, 14.0f, { 136, 492 },   { 0, 0 },      6.0f },
	// The pool: two smooth-subtracted scoops carve the asymmetric depression.
	{ 0, CF_SHAPE_OP_SUBTRACT, 20.0f, { 330, 330 }, { 0, 0 },     172.0f }, // deep bowl (west)
	{ 0, CF_SHAPE_OP_SUBTRACT, 20.0f, { 720, 470 }, { 0, 0 },     235.0f }, // shallow shelf (east beach)
	// Knuckle poking through the waterline mid-pool (unioned AFTER the scoops).
	{ 0, CF_SHAPE_OP_UNION, 12.0f, { 585, 245 },   { 0, 0 },      62.0f },
	{ 0, CF_SHAPE_OP_UNION, 12.0f, { 612, 208 },   { 0, 0 },      50.0f },
};
#define GROUND_PRIM_COUNT ((int)(sizeof(ground_prims) / sizeof(ground_prims[0])))

// CF's csg_smin (tile shader): polynomial smooth min, hard min at k = 0.
float ground_smin(float a, float b, float k)
{
	if (k <= 0) return cf_min(a, b);
	float h = cf_clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
	return (b + (a - b) * h) - k * h * (1.0f - h);
}

float ground_prim_dist(const GroundPrim* g, CF_V2 p)
{
	if (g->kind == 0) {
		return cf_len(cf_sub(p, g->a)) - g->r;
	} else if (g->kind == 1) {
		CF_V2 ab = cf_sub(g->b, g->a);
		CF_V2 pa = cf_sub(p, g->a);
		float t = cf_clamp(cf_dot(pa, ab) / cf_dot(ab, ab), 0.0f, 1.0f);
		return cf_len(cf_sub(pa, cf_mul_v2_f(ab, t))) - g->r;
	} else {
		CF_V2 c = cf_mul_v2_f(cf_add(g->a, g->b), 0.5f);
		CF_V2 he = cf_mul_v2_f(cf_sub(g->b, g->a), 0.5f);
		CF_V2 d = cf_v2(fabsf(p.x - c.x) - he.x, fabsf(p.y - c.y) - he.y);
		CF_V2 dc = cf_v2(cf_max(d.x, 0.0f), cf_max(d.y, 0.0f));
		return cf_len(dc) + cf_min(cf_max(d.x, d.y), 0.0f);
	}
}

// Signed distance to the ground rock: positive in air/water, negative inside rock.
float cave_ground_sdf(CF_V2 p)
{
	float d = 0;
	for (int i = 0; i < GROUND_PRIM_COUNT; i++) {
		const GroundPrim* g = ground_prims + i;
		float di = ground_prim_dist(g, p);
		if (i == 0) d = di;
		else if (g->op == CF_SHAPE_OP_UNION) d = ground_smin(d, di, g->k);
		else if (g->op == CF_SHAPE_OP_SUBTRACT) d = -ground_smin(-d, di, g->k);
		else d = -ground_smin(-d, -di, g->k);
	}
	return d;
}

// Outward SDF gradient (points away from rock) via central differences.
CF_V2 cave_ground_grad(CF_V2 p)
{
	float e = 0.75f;
	float dx = cave_ground_sdf(cf_v2(p.x + e, p.y)) - cave_ground_sdf(cf_v2(p.x - e, p.y));
	float dy = cave_ground_sdf(cf_v2(p.x, p.y + e)) - cave_ground_sdf(cf_v2(p.x, p.y - e));
	float len = sqrtf(dx * dx + dy * dy);
	if (len < 1e-6f) return cf_v2(0, 1);
	return cf_v2(dx / len, dy / len);
}

// Bake the static ground SDF into a small R32F texture for cave_water.shd
// (256x256 over the 1024 world = 4 px texels, linearly filtered).
#define GSDF_N 256
CF_Texture gsdf_tex;

void ground_bake_sdf_tex()
{
	float* data = (float*)cf_alloc(GSDF_N * GSDF_N * sizeof(float));
	for (int row = 0; row < GSDF_N; row++) {
		float wy = (1.0f - ((float)row + 0.5f) / (float)GSDF_N) * 1024.0f; // row 0 = world top
		for (int col = 0; col < GSDF_N; col++) {
			float wx = (((float)col + 0.5f) / (float)GSDF_N) * 1024.0f;
			data[row * GSDF_N + col] = cave_ground_sdf(cf_v2(wx, wy));
		}
	}
	CF_TextureParams tp = cf_texture_defaults(GSDF_N, GSDF_N);
	tp.pixel_format = CF_PIXEL_FORMAT_R32_FLOAT;
	tp.filter = CF_FILTER_LINEAR;
	tp.wrap_u = CF_WRAP_MODE_CLAMP_TO_EDGE;
	tp.wrap_v = CF_WRAP_MODE_CLAMP_TO_EDGE;
	gsdf_tex = cf_make_texture(tp);
	cf_texture_update(gsdf_tex, data, GSDF_N * GSDF_N * sizeof(float));
	cf_free(data);
}

//--------------------------------------------------------------------------------------------------
// Cave rock geometry. Static CSG shape groups, recorded once into retained draw
// lists (one per destination canvas, since recorded draws bake their colors).

CF_DrawList list_rock_absorption;
CF_DrawList list_rock_diffuse;

// Stalactite tips (drip spawn points), filled in by draw_cave_rock.
CF_V2 stalactite_tips[5];

void draw_cave_rock()
{
	// Ceiling mass: slab + blobby underside + stalactites, with the moon crack
	// smooth-subtracted through the whole composite. The slab overshoots the
	// frame top so the rock mass reads as extending past the visible edge; the
	// crack channel is the ONLY opening through it.
	cf_draw_shape_group_begin();
	cf_draw_shape_group_op(CF_SHAPE_OP_UNION, 16.0f);
	cf_draw_quad_fill(cf_make_aabb(cf_v2(0, 860), cf_v2(1024, 1040)), 0);
	cf_draw_circle_fill2(cf_v2(100, 862), 58);
	cf_draw_circle_fill2(cf_v2(240, 852), 50);
	cf_draw_circle_fill2(cf_v2(520, 850), 58);
	cf_draw_circle_fill2(cf_v2(700, 858), 54);
	cf_draw_circle_fill2(cf_v2(880, 850), 56);
	// Stalactites: capsule + circle chains, tips recorded for the drip script.
	cf_draw_shape_group_op(CF_SHAPE_OP_UNION, 14.0f);
	cf_draw_capsule_fill2(cf_v2(250, 880), cf_v2(250, 790), 16);
	cf_draw_circle_fill2(cf_v2(250, 770), 9);
	cf_draw_capsule_fill2(cf_v2(480, 875), cf_v2(485, 760), 18);
	cf_draw_circle_fill2(cf_v2(487, 738), 10);
	cf_draw_circle_fill2(cf_v2(488, 722), 5);
	cf_draw_capsule_fill2(cf_v2(660, 870), cf_v2(658, 795), 12);
	cf_draw_circle_fill2(cf_v2(657, 780), 6);
	cf_draw_capsule_fill2(cf_v2(700, 875), cf_v2(705, 745), 20);
	cf_draw_circle_fill2(cf_v2(707, 720), 11);
	cf_draw_circle_fill2(cf_v2(709, 700), 5);
	cf_draw_capsule_fill2(cf_v2(850, 872), cf_v2(848, 800), 13);
	cf_draw_circle_fill2(cf_v2(847, 782), 7);
	// The crack: a slanted channel (~15 degrees off vertical) cut clean through
	// slab, blobs, everything, from past the frame top down into the cave.
	cf_draw_shape_group_op(CF_SHAPE_OP_SUBTRACT, 8.0f);
	{
		float xb = CRACK_X0, xt = CRACK_X0 + (1040.0f - CRACK_Y0) * CRACK_SLOPE;
		cf_draw_quad_fill2(
			cf_v2(xt - CRACK_HALF_W, 1040), cf_v2(xt + CRACK_HALF_W, 1040),
			cf_v2(xb + CRACK_HALF_W, CRACK_Y0), cf_v2(xb - CRACK_HALF_W, CRACK_Y0), 0
		);
	}
	cf_draw_shape_group_end();

	// Ground: the unified SDF's primitive table drawn as ONE shape group, so
	// the silhouette IS the collision surface (see cave_ground_sdf).
	cf_draw_shape_group_begin();
	{
		float last_k = -1.0f;
		int last_op = -1;
		for (int i = 0; i < GROUND_PRIM_COUNT; i++) {
			const GroundPrim* g = ground_prims + i;
			if (g->op != last_op || g->k != last_k) {
				cf_draw_shape_group_op((CF_ShapeOp)g->op, g->k);
				last_op = g->op;
				last_k = g->k;
			}
			if (g->kind == 0) cf_draw_circle_fill2(g->a, g->r);
			else if (g->kind == 1) cf_draw_capsule_fill2(g->a, g->b, g->r);
			else cf_draw_quad_fill(cf_make_aabb(g->a, g->b), 0);
		}
	}
	cf_draw_shape_group_end();

	// A floating rock arch, right of center, above the pool.
	cf_draw_shape_group_begin();
	cf_draw_shape_group_op(CF_SHAPE_OP_UNION, 18.0f);
	cf_draw_capsule_fill2(cf_v2(580, 500), cf_v2(640, 560), 22);
	cf_draw_capsule_fill2(cf_v2(640, 560), cf_v2(720, 540), 20);
	cf_draw_capsule_fill2(cf_v2(720, 540), cf_v2(760, 480), 16);
	cf_draw_shape_group_end();
}

// Runes: curve-text glyphs stroked onto the wall face by the pool. They carry
// rock-level absorption but DOUBLE the rock albedo, so they stay invisible in
// the gloom until bounce light from agitated water rakes across the wall.
void draw_runes()
{
	const char* glyphs[7] = { "X", "V", "A", "K", "I", "Z", "M" };
	cf_push_text_curves(true);
	cf_push_text_stroke(2.0f);
	cf_push_font_size(34.0f);
	for (int i = 0; i < 7; i++) {
		cf_draw_text(glyphs[i], cf_v2(932.0f + ((i % 2) ? 6.0f : 0.0f), 500.0f - (float)i * 40.0f), -1);
	}
	cf_pop_font_size();
	cf_pop_text_stroke();
	cf_pop_text_curves();
}

void record_rock_lists()
{
	stalactite_tips[0] = cf_v2(250, 760);
	stalactite_tips[1] = cf_v2(488, 716);
	stalactite_tips[2] = cf_v2(657, 773);
	stalactite_tips[3] = cf_v2(709, 694);
	stalactite_tips[4] = cf_v2(847, 774);

	// Absorption: rock is a dense absorber, ~30 per world pixel.
	list_rock_absorption = cf_make_draw_list();
	cf_draw_list_begin(list_rock_absorption);
	cf_draw_push_color(cf_make_color_rgb_f(30.0f, 30.0f, 30.0f));
	draw_cave_rock();
	draw_runes();
	cf_draw_pop_color();
	cf_draw_list_end();

	// Diffuse albedo: warm grey rock for the multibounce feedback; the runes at
	// 0.5 so they light up first.
	list_rock_diffuse = cf_make_draw_list();
	cf_draw_list_begin(list_rock_diffuse);
	cf_draw_push_color(cf_make_color_rgb_f(0.27f, 0.25f, 0.22f));
	draw_cave_rock();
	cf_draw_pop_color();
	cf_draw_push_color(cf_make_color_rgb_f(0.5f, 0.5f, 0.48f));
	draw_runes();
	cf_draw_pop_color();
	cf_draw_list_end();
}

//--------------------------------------------------------------------------------------------------
// Roots: Bezier strands hanging through the crack, swaying slowly. Drawn to
// absorption (they subdivide the god ray into moving sub-shafts) and as
// silhouettes on screen.
//
// NOTE: these were originally CF_DrawPath objects (atlas-backed curve strips).
// The bake-once corruption that forced them out is fixed now, but retained
// paths only pay off for static curves -- these strands move their control
// points every frame, so a path would be rebuilt per frame (fresh atlas ids,
// cache churn) anyway. Immediate-mode cubic polylines stay: same curves, no
// atlas involvement, negligible cost.

float root_time = 0;

void draw_roots(float thickness)
{
	for (int i = 0; i < 4; i++) {
		// Bases sit inside the (slanted) crack channel at y ~830.
		float bx = 381.0f + (float)i * 6.0f;
		float ph = (float)i * 1.7f;
		float s1 = sinf(root_time * 0.7f + ph) * 6.0f;
		float s2 = sinf(root_time * 0.9f + ph + 1.3f) * 10.0f;
		float s3 = sinf(root_time * 1.1f + ph + 2.1f) * 14.0f;
		float tip_y = 585.0f + (float)i * 24.0f;

		CF_V2 p0 = cf_v2(bx, 830);
		CF_V2 mid = cf_v2(bx + s1 * 1.2f, 690);
		cf_draw_bezier_line2(p0, cf_v2(bx, 770), cf_v2(bx + s1, 735), mid, 12, thickness);
		cf_draw_bezier_line2(mid, cf_v2(bx + s2, 655), cf_v2(bx + s2 * 1.3f, 620), cf_v2(bx + s3, tip_y), 12, thickness);
	}
}

//--------------------------------------------------------------------------------------------------
// Water: CPU particles with a spatial hash, agitation-driven bioluminescence.

#define P_MAX 512
#define P_RADIUS 4.5f
#define P_H 12.0f       // interaction radius
#define P_SPLAT 22.0f   // density splat radius (wide gaussian, soft metaballs)
#define HASH_CELL 16
#define HASH_DIM 64     // 1024 / 16

typedef struct Particle
{
	CF_V2 p;
	CF_V2 v;
	float agitation;
	int airborne; // lone ejecta above the surface: drawn as a droplet, not a metaball
} Particle;

Particle particles[P_MAX];
int particle_count = 0;
int hash_head[HASH_DIM * HASH_DIM];
int hash_next[P_MAX];

int clampi(int v, int lo, int hi)
{
	return v < lo ? lo : v > hi ? hi : v;
}

CF_V2 mouse_world = { 512, 512 };
CF_V2 mouse_vel = { 0, 0 };
int autostir = 0;
float autostir_t = 0;

void particles_init()
{
	particle_count = 0;
	// Lattice spacing slightly ABOVE the rest separation (9), or frame one is a
	// constraint-resolution storm that sloshes the whole pool up the walls.
	// Bottom-up row fill over every open lattice site below the waterline, so
	// the asymmetric depression fills to a level surface at the waterline.
	for (float y = 130.0f; y < WATERLINE - 1.0f && particle_count < 500; y += 9.5f) {
		for (float x = 80.0f; x < 944.0f && particle_count < 500; x += 9.5f) {
			CF_V2 p = cf_v2(x + ((particle_count % 3) - 1) * 0.8f, y);
			if (cave_ground_sdf(p) < 11.0f) continue; // keep a rest-spacing margin off the rock
			Particle* pt = particles + particle_count++;
			pt->p = p;
			pt->v = cf_v2(0, 0);
			pt->agitation = 0;
			pt->airborne = 0;
		}
	}
}

void hash_build()
{
	for (int i = 0; i < HASH_DIM * HASH_DIM; i++) hash_head[i] = -1;
	for (int i = 0; i < particle_count; i++) {
		int cx = (int)(particles[i].p.x / HASH_CELL);
		int cy = (int)(particles[i].p.y / HASH_CELL);
		cx = clampi(cx, 0, HASH_DIM - 1);
		cy = clampi(cy, 0, HASH_DIM - 1);
		int cell = cy * HASH_DIM + cx;
		hash_next[i] = hash_head[cell];
		hash_head[cell] = i;
	}
}

//--------------------------------------------------------------------------------------------------
// Surface heightfield: a 1D lattice of springs spanning the basin chord at the
// waterline. Neighbor coupling propagates ripple rings; drips, clicks, mouse
// stir, and near-surface particle churn all kick it. Uploaded per-frame as a
// 256x1 R32F texture that cave_water.shd reads as the water's top edge.

#define HF_N 256
#define HF_SUBSTEPS 3   // keeps the explicit wave step under the CFL limit

float hf_h[HF_N];       // world-px offset from the waterline
float hf_v[HF_N];
float hf_x0, hf_x1;     // world x extent (pool opening at the waterline)
unsigned char hf_rock[HF_N]; // columns where rock crosses the waterline (the knuckle): springs pinned
CF_Texture hf_tex;

void heightfield_init()
{
	// Scan the ground SDF along the waterline for the pool's open extent:
	// out from the deep bowl for the west contact, out from the shelf for the
	// east contact. The knuckle interrupts the middle; those columns get
	// pinned so ripples reflect off it instead of tunneling through rock.
	float xl = 350.0f, xr = 750.0f;
	while (xl > 60.0f && cave_ground_sdf(cf_v2(xl - 1.0f, WATERLINE)) > 0) xl -= 1.0f;
	while (xr < 980.0f && cave_ground_sdf(cf_v2(xr + 1.0f, WATERLINE)) > 0) xr += 1.0f;
	hf_x0 = xl;
	hf_x1 = xr;
	float colw = (hf_x1 - hf_x0) / (float)HF_N;
	for (int i = 0; i < HF_N; i++) {
		float xc = hf_x0 + ((float)i + 0.5f) * colw;
		hf_rock[i] = cave_ground_sdf(cf_v2(xc, WATERLINE)) < 0 ? 1 : 0;
	}
	CF_MEMSET(hf_h, 0, sizeof(hf_h));
	CF_MEMSET(hf_v, 0, sizeof(hf_v));

	CF_TextureParams tp = cf_texture_defaults(HF_N, 1);
	tp.pixel_format = CF_PIXEL_FORMAT_R32_FLOAT;
	tp.filter = CF_FILTER_LINEAR;
	tp.wrap_u = CF_WRAP_MODE_CLAMP_TO_EDGE;
	tp.wrap_v = CF_WRAP_MODE_CLAMP_TO_EDGE;
	tp.stream = true;
	hf_tex = cf_make_texture(tp);
	cf_texture_update(hf_tex, hf_h, sizeof(hf_h));
}

// Velocity kick to the springs around x (negative = push the surface down).
void hf_splash(float x, float radius, float amount)
{
	float colw = (hf_x1 - hf_x0) / (float)HF_N;
	int c0 = clampi((int)((x - radius - hf_x0) / colw), 0, HF_N - 1);
	int c1 = clampi((int)((x + radius - hf_x0) / colw), 0, HF_N - 1);
	for (int i = c0; i <= c1; i++) {
		float xc = hf_x0 + ((float)i + 0.5f) * colw;
		float t = (xc - x) / radius;
		if (t * t > 1.0f) continue;
		float w = 1.0f - t * t;
		hf_v[i] += amount * w * w;
	}
}

void heightfield_update(float dt)
{
	dt = cf_min(dt, 1.0f / 60.0f);
	float colw = (hf_x1 - hf_x0) / (float)HF_N;
	float c = 170.0f;                     // ripple propagation speed, px/s
	float k = (c * c) / (colw * colw);
	float sub_dt = dt / (float)HF_SUBSTEPS;
	for (int s = 0; s < HF_SUBSTEPS; s++) {
		for (int i = 0; i < HF_N; i++) {
			float l = hf_h[i > 0 ? i - 1 : i];
			float r = hf_h[i < HF_N - 1 ? i + 1 : i];
			// Neighbor wave coupling + a slow pull back to flat (drips push net
			// volume down; without this the mean level never recovers).
			hf_v[i] += ((l + r - 2.0f * hf_h[i]) * k - hf_h[i] * 9.0f) * sub_dt;
		}
		for (int i = 0; i < HF_N; i++) hf_h[i] += hf_v[i] * sub_dt;
	}
	// Damping authored as ~0.985/frame at 60fps, applied dt-correct so the
	// waves survive the same wall-clock time at any refresh rate.
	float damp = powf(0.985f, dt * 60.0f);
	for (int i = 0; i < HF_N; i++) {
		hf_v[i] *= damp;
		hf_h[i] = cf_clamp(hf_h[i], -18.0f, 18.0f);
		if (hf_rock[i]) { hf_h[i] = 0; hf_v[i] = 0; } // pinned at the knuckle
	}
	cf_texture_update(hf_tex, hf_h, sizeof(hf_h));
}

//--------------------------------------------------------------------------------------------------
// Spray: short-lived specks thrown off where the water churns hard. Purely
// cosmetic ballistic points, drawn as tiny soft glints above the surface.

#define SPRAY_MAX 96
#define SPRAY_LIFE 0.5f

typedef struct Spray
{
	CF_V2 p;
	CF_V2 v;
	float life;
} Spray;

Spray sprays[SPRAY_MAX];
int spray_count = 0;
CF_Rnd spray_rnd;

void spray_spawn(CF_V2 at, CF_V2 vel)
{
	if (spray_count >= SPRAY_MAX) return;
	Spray* s = sprays + spray_count++;
	s->p = at;
	s->v = vel;
	s->life = SPRAY_LIFE;
}

void spray_update(float dt)
{
	for (int i = 0; i < spray_count;) {
		Spray* s = sprays + i;
		s->life -= dt;
		if (s->life <= 0) {
			*s = sprays[--spray_count];
			continue;
		}
		s->v.y -= 700.0f * dt;
		s->p = cf_add(s->p, cf_mul_v2_f(s->v, dt));
		i++;
	}
}

// Splash agitation + outward impulse at a point (drips, clicks).
void water_impulse(CF_V2 at, float radius, float strength, float agitation)
{
	for (int i = 0; i < particle_count; i++) {
		CF_V2 d = cf_sub(particles[i].p, at);
		float len = cf_len(d);
		if (len > radius) continue;
		float w = 1.0f - len / radius;
		CF_V2 dir = len > 1e-3f ? cf_div_v2_f(d, len) : cf_v2(0, 1);
		particles[i].v = cf_add(particles[i].v, cf_mul_v2_f(dir, strength * w));
		particles[i].v.y += strength * 0.35f * w;
		particles[i].agitation = cf_min(particles[i].agitation + agitation * w, 1.4f);
	}
	// Ring the surface springs too, so splashes raise real ripple rings.
	hf_splash(at.x, radius, -strength * 0.45f);
}

// Position-based fluids: predict with gravity, relax positional constraints
// (pairwise separation + basin SDF), then DERIVE velocity from positions
// (v = (x - x_prev) / dt). Overlap resolution never injects velocity impulses,
// so a resting pile stays resting; XSPH viscosity bleeds neighbor velocities
// together so disturbances damp as a group instead of rattling forever.
#define P_VMAX 500.0f // max speed, about a basin radius per second

CF_V2 p_old[P_MAX];
CF_V2 v_half[P_MAX];    // velocity snapshot for the XSPH gather
int p_neighbors[P_MAX]; // neighbor count within P_H (density proxy for droplets)
float mean_speed = 0;   // diagnostics (HRC_CAVE_PERF) + foam/chop drive

void particles_update(float dt)
{
	dt = cf_min(dt, 1.0f / 60.0f); // sim runs slow-mo on frame drops rather than exploding
	float sep = 9.0f; // rest spacing between particle centers

	// Mouse stir: swirl + push within a radius while the mouse moves.
	float mouse_speed = cf_len(mouse_vel);
	if (mouse_speed > 10.0f) {
		for (int i = 0; i < particle_count; i++) {
			CF_V2 d = cf_sub(particles[i].p, mouse_world);
			float len = cf_len(d);
			if (len > 60.0f) continue;
			float w = 1.0f - len / 60.0f;
			CF_V2 push = cf_mul_v2_f(mouse_vel, 0.22f * w * dt);
			CF_V2 dir = len > 1e-3f ? cf_div_v2_f(d, len) : cf_v2(0, 1);
			CF_V2 swirl = cf_mul_v2_f(cf_v2(-dir.y, dir.x), mouse_speed * 0.15f * w * dt);
			particles[i].v = cf_add(particles[i].v, cf_add(push, swirl));
			particles[i].agitation = cf_min(particles[i].agitation + mouse_speed * 0.0007f * w * dt * 60.0f, 1.4f);
		}
		// Stirring near the surface chops it up directly: a moving hand piles
		// water ahead and digs behind (dipole -- net zero volume, so sustained
		// stirring can't sink the mean level into the clamp).
		if (fabsf(mouse_world.y - WATERLINE) < 60.0f) {
			float dir = mouse_vel.x >= 0 ? 1.0f : -1.0f;
			float kick = cf_min(mouse_speed, 600.0f) * 0.30f * dt;
			hf_splash(mouse_world.x + dir * 26.0f, 34.0f, kick);
			hf_splash(mouse_world.x - dir * 26.0f, 34.0f, -kick);
		}
	}

	// Predict.
	float drag = expf(-0.35f * dt); // gentle bulk drag, tau ~3s
	for (int i = 0; i < particle_count; i++) {
		Particle* pt = particles + i;
		pt->v.y -= 500.0f * dt;
		pt->v = cf_mul_v2_f(pt->v, drag);
		float speed = cf_len(pt->v);
		if (speed > P_VMAX) pt->v = cf_mul_v2_f(pt->v, P_VMAX / speed);
		p_old[i] = pt->p;
		pt->p = cf_add(pt->p, cf_mul_v2_f(pt->v, dt));
	}

	// Relax positional constraints: pairwise separation + ground containment.
	for (int iter = 0; iter < 4; iter++) {
		hash_build();
		for (int i = 0; i < particle_count; i++) {
			Particle* a = particles + i;
			int cx = clampi((int)(a->p.x / HASH_CELL), 0, HASH_DIM - 1);
			int cy = clampi((int)(a->p.y / HASH_CELL), 0, HASH_DIM - 1);
			for (int oy = -1; oy <= 1; oy++) {
				for (int ox = -1; ox <= 1; ox++) {
					int nx = cx + ox;
					int ny = cy + oy;
					if (nx < 0 || nx >= HASH_DIM || ny < 0 || ny >= HASH_DIM) continue;
					for (int j = hash_head[ny * HASH_DIM + nx]; j != -1; j = hash_next[j]) {
						if (j == i) continue;
						Particle* b = particles + j;
						CF_V2 d = cf_sub(a->p, b->p);
						float len = cf_len(d);
						if (len >= sep || len < 1e-4f) continue;
						// Each pair is visited twice (once per endpoint), so each
						// visit moves only this particle by half the correction.
						CF_V2 dir = cf_div_v2_f(d, len);
						a->p = cf_add(a->p, cf_mul_v2_f(dir, (sep - len) * 0.5f * 0.5f));
					}
				}
			}
			// Unified ground SDF: project out of the rock along the gradient.
			// The SDF is the real rock everywhere, so no altitude gate is needed
			// -- splash arcs over open water fly free (positive distance), and
			// anything meeting rock (floor, walls, the knuckle, the beach) gets
			// a pure projection. No restitution or tangential nudging here:
			// either becomes perpetual sliding once velocity derives from
			// positions.
			float d = cave_ground_sdf(a->p);
			if (d < P_RADIUS) {
				CF_V2 n = cave_ground_grad(a->p);
				a->p = cf_add(a->p, cf_mul_v2_f(n, P_RADIUS - d));
			}
			// Pseudo-hydrostatic relief: pairwise separation alone behaves like
			// dry sand -- stirred piles hold their shape instead of leveling.
			// Excess height above the waterline gets a gentle positional pull
			// down; the separation constraints turn it into lateral flow, so
			// the pool always finds its level again. Too small to matter for
			// ballistic splash arcs.
			float over = a->p.y - (WATERLINE + 4.0f);
			if (over > 0) a->p.y -= cf_min(over, 12.0f) * 0.02f;
		}
	}

	// Velocities from positions + basin contact response.
	for (int i = 0; i < particle_count; i++) {
		Particle* pt = particles + i;
		// Rest bias: a settled particle's frame displacement is just solver
		// micro-jitter around gravity's ~0.1px step. Blending small displacements
		// back toward the previous position keeps that noise out of the derived
		// velocity (the blend fades in smoothly, so nothing visibly freezes).
		// Agitation is the wake signal: actively driven water (stir, drips,
		// splashes) must be free to accumulate slow motion, so the bias fades
		// out where the pool glows and returns as the glow dies.
		CF_V2 dp = cf_sub(pt->p, p_old[i]);
		float disp = cf_len(dp);
		float rest = 1.0f - cf_min(disp / (30.0f * dt), 1.0f);
		rest *= 1.0f - cf_min(pt->agitation * 2.0f, 1.0f);
		// Deliberately per-FRAME, not dt-scaled: the jitter it filters is
		// created fresh by each solver pass, at any refresh rate.
		if (rest > 0) pt->p = cf_sub(pt->p, cf_mul_v2_f(dp, rest * 0.7f));
		pt->v = cf_div_v2_f(cf_sub(pt->p, p_old[i]), dt);
		// Deep interpenetration corrections can spike the derived velocity.
		float speed = cf_len(pt->v);
		if (speed > P_VMAX) pt->v = cf_mul_v2_f(pt->v, P_VMAX / speed);

		// In contact with the ground: kill the into-rock normal component (no
		// restitution) and rub the tangential one down with friction. Same
		// contact model as the old basin circle, driven by the unified SDF.
		float gd = cave_ground_sdf(pt->p);
		if (gd < P_RADIUS + 2.0f) {
			CF_V2 n = cave_ground_grad(pt->p); // points away from rock
			float vn = cf_dot(pt->v, n);
			CF_V2 vt = cf_sub(pt->v, cf_mul_v2_f(n, vn));
			if (vn < 0) vn = 0;
			float friction = powf(0.92f, dt * 60.0f); // ~0.92 per frame at 60fps
			pt->v = cf_add(cf_mul_v2_f(vt, friction), cf_mul_v2_f(n, vn));
		}
		v_half[i] = pt->v;
	}

	// XSPH viscosity + soft sleep + agitation diffusion, one neighbor gather.
	hash_build();
	float speed_acc = 0;
	for (int i = 0; i < particle_count; i++) {
		Particle* pt = particles + i;
		int cx = clampi((int)(pt->p.x / HASH_CELL), 0, HASH_DIM - 1);
		int cy = clampi((int)(pt->p.y / HASH_CELL), 0, HASH_DIM - 1);
		CF_V2 xsph = cf_v2(0, 0);
		float neighbor_agit = 0;
		int neighbor_n = 0;
		int agit_n = 0;
		for (int oy = -1; oy <= 1; oy++) {
			for (int ox = -1; ox <= 1; ox++) {
				int nx = cx + ox;
				int ny = cy + oy;
				if (nx < 0 || nx >= HASH_DIM || ny < 0 || ny >= HASH_DIM) continue;
				for (int j = hash_head[ny * HASH_DIM + nx]; j != -1; j = hash_next[j]) {
					if (j == i) continue;
					CF_V2 dp = cf_sub(particles[j].p, pt->p);
					float r2 = dp.x * dp.x + dp.y * dp.y;
					if (r2 < P_H * P_H) {
						float w = 1.0f - r2 / (P_H * P_H);
						xsph = cf_add(xsph, cf_mul_v2_f(cf_sub(v_half[j], v_half[i]), w));
						neighbor_n++;
					}
					neighbor_agit += particles[j].agitation;
					agit_n++;
				}
			}
		}
		p_neighbors[i] = neighbor_n;
		pt->v = cf_add(pt->v, cf_mul_v2_f(xsph, cf_min(0.16f * dt * 60.0f, 0.4f)));

		// Soft sleep: near-rest particles get extra damping, fading in smoothly
		// below the threshold so there is no visible freeze pop. Agitation
		// keeps actively driven water awake (same signal as the rest bias).
		float speed = cf_len(pt->v);
		float wake = cf_max(cf_min(speed / 26.0f, 1.0f), cf_min(pt->agitation * 2.0f, 1.0f));
		float damp = expf(-13.0f * dt) + (1.0f - expf(-13.0f * dt)) * wake;
		pt->v = cf_mul_v2_f(pt->v, damp);
		speed_acc += cf_len(pt->v);

		// Near-surface churn feeds the heightfield. Keyed off REAL motion --
		// the settled pool (< ~2 px/s) leaves the springs alone, so idle water
		// goes glassy instead of shimmering with solver noise.
		if (fabsf(pt->p.y - WATERLINE) < 20.0f && speed > 40.0f) {
			hf_splash(pt->p.x, 12.0f, cf_min(speed, 250.0f) * (pt->v.y > 0 ? 1.4f : -1.4f) * dt);
			// Hard churn throws off spray specks.
			if (speed > 130.0f && pt->v.y > 50.0f && cf_rnd_range_float(&spray_rnd, 0, 1) < 12.0f * dt) {
				CF_V2 sv = cf_v2(pt->v.x * 0.6f + cf_rnd_range_float(&spray_rnd, -40, 40), pt->v.y * 0.8f + cf_rnd_range_float(&spray_rnd, 30, 110));
				spray_spawn(cf_v2(pt->p.x, pt->p.y + 4.0f), sv);
			}
		}

		// Lone ejecta above the surface render as individual droplets.
		pt->airborne = (p_neighbors[i] <= 2 && pt->p.y > WATERLINE + 6.0f && speed > 40.0f);

		// Agitation diffuses to neighbors so disturbances spread as a wave.
		if (agit_n > 0) {
			float avg = neighbor_agit / (float)agit_n;
			pt->agitation += (avg - pt->agitation) * cf_min(3.0f * dt, 0.5f);
		}
		pt->agitation *= expf(-dt / 2.0f);
	}
	mean_speed = particle_count > 0 ? speed_acc / (float)particle_count : 0;
}

//--------------------------------------------------------------------------------------------------
// Drip loop: a coroutine script picks a stalactite, grows a droplet, releases it,
// and splashes the pool. The plink is synthesized at init (sine partials with a
// pitch chirp, one-pole low-pass, and a feedback delay for cave reverb).

CF_Coroutine drip_co;
float g_dt = 1.0f / 60.0f;
CF_Rnd cave_rnd;

typedef struct Drip
{
	int stage; // 0 = idle, 1 = growing at tip, 2 = falling
	CF_V2 pos;
	CF_V2 vel;
	float r;
} Drip;

Drip drip;
CF_Audio plink_audio;
void* plink_wav; // kept alive for the audio system

void write_le32(unsigned char* p, uint32_t v)
{
	p[0] = (unsigned char)(v & 0xff);
	p[1] = (unsigned char)((v >> 8) & 0xff);
	p[2] = (unsigned char)((v >> 16) & 0xff);
	p[3] = (unsigned char)((v >> 24) & 0xff);
}

void write_le16(unsigned char* p, uint16_t v)
{
	p[0] = (unsigned char)(v & 0xff);
	p[1] = (unsigned char)((v >> 8) & 0xff);
}

// Synthesize a cavernous water plink into an in-memory PCM16 wav.
CF_Audio make_plink_audio()
{
	int rate = 22050;
	float dur = 1.4f;
	int n = (int)(rate * dur);

	float* s = (float*)cf_calloc(n, sizeof(float));
	float lp = 0;
	for (int i = 0; i < n; i++) {
		float t = (float)i / (float)rate;
		// Partials with a slight downward chirp, like a droplet in a stone room.
		float chirp = 40.0f * expf(-t * 22.0f);
		float v = sinf(CF_TAU * ((1560.0f + chirp) * t)) * 0.55f * expf(-t * 6.0f)
		        + sinf(CF_TAU * (780.0f * t)) * 0.3f * expf(-t * 3.5f)
		        + sinf(CF_TAU * (2340.0f * t)) * 0.22f * expf(-t * 10.0f);
		float attack = t < 0.004f ? t / 0.004f : 1.0f;
		v *= attack;
		lp += (v - lp) * 0.32f; // low-pass softens it into the dark
		s[i] = lp;
	}
	// Feedback delay: cheap cave echo with a long decay tail.
	int delay = (int)(0.115f * rate);
	for (int i = delay; i < n; i++) s[i] += s[i - delay] * 0.42f;

	int data_bytes = n * 2;
	int total = 44 + data_bytes;
	unsigned char* wav = (unsigned char*)cf_alloc(total);
	CF_MEMCPY(wav, "RIFF", 4);
	write_le32(wav + 4, (uint32_t)(total - 8));
	CF_MEMCPY(wav + 8, "WAVE", 4);
	CF_MEMCPY(wav + 12, "fmt ", 4);
	write_le32(wav + 16, 16);
	write_le16(wav + 20, 1);  // PCM
	write_le16(wav + 22, 1);  // mono
	write_le32(wav + 24, (uint32_t)rate);
	write_le32(wav + 28, (uint32_t)(rate * 2));
	write_le16(wav + 32, 2);
	write_le16(wav + 34, 16);
	CF_MEMCPY(wav + 36, "data", 4);
	write_le32(wav + 40, (uint32_t)data_bytes);
	for (int i = 0; i < n; i++) {
		float v = s[i] * 0.8f;
		if (v > 1.0f) v = 1.0f;
		if (v < -1.0f) v = -1.0f;
		write_le16(wav + 44 + i * 2, (uint16_t)(int16_t)(v * 32767.0f));
	}
	cf_free(s);

	plink_wav = wav;
	return cf_audio_load_wav_from_memory(wav, total);
}

// Estimated water surface height near x: the highest settled particle nearby.
float water_surface_at(float x)
{
	float best = 190.0f;
	for (int i = 0; i < particle_count; i++) {
		if (fabsf(particles[i].p.x - x) > 30.0f) continue;
		if (cf_len(particles[i].v) > 120.0f) continue; // ignore droplets in flight
		if (particles[i].p.y > best) best = particles[i].p.y;
	}
	return best;
}

void drip_wait(CF_Coroutine co, float seconds)
{
	while (seconds > 0) {
		cf_coroutine_yield(co);
		seconds -= g_dt;
	}
}

void drip_script(CF_Coroutine co)
{
	for (;;) {
		drip.stage = 0;
		drip_wait(co, cf_rnd_range_float(&cave_rnd, 3.0f, 8.0f));

		CF_V2 tip = stalactite_tips[cf_rnd_range_int(&cave_rnd, 0, 4)];

		// Grow a droplet at the tip over half a second.
		drip.stage = 1;
		float t = 0;
		while (t < 0.5f) {
			drip.r = 1.2f + (t / 0.5f) * 2.6f;
			drip.pos = cf_v2(tip.x, tip.y - 3.0f - drip.r);
			cf_coroutine_yield(co);
			t += g_dt;
		}

		// Release: fall until it meets the water, glinting through the god ray.
		drip.stage = 2;
		drip.vel = cf_v2(0, 0);
		while (drip.pos.y > water_surface_at(drip.pos.x)) {
			drip.vel.y -= 900.0f * g_dt;
			drip.pos.y += drip.vel.y * g_dt;
			cf_coroutine_yield(co);
		}

		// Splash: impulse + ring of agitation + plink.
		water_impulse(drip.pos, 60.0f, 150.0f, 1.0f);
		CF_SoundParams sp = cf_sound_params_defaults();
		sp.volume = 0.6f;
		sp.pitch = cf_rnd_range_float(&cave_rnd, 0.85f, 1.2f);
		cf_play_sound(plink_audio, sp);
	}
}

// Draw the drip into a scene pass. channel: 0 = emissivity, 1 = absorption.
void draw_drip(int channel)
{
	if (drip.stage == 0) return;
	if (channel == 0) {
		// A faint cool glint; crossing the god ray it also catches c-1 light.
		cf_draw_push_color(cf_make_color_rgb_f(1.1f, 1.4f, 1.9f));
		cf_draw_circle_fill2(drip.pos, drip.r * 0.7f);
		cf_draw_pop_color();
	} else {
		// Watery absorber (blue-weighted like the pool).
		cf_draw_push_color(cf_make_color_rgb_f(0.5f, 0.25f, 0.2f));
		cf_draw_circle_fill2(drip.pos, drip.r);
		cf_draw_pop_color();
	}
}

//--------------------------------------------------------------------------------------------------
// Jellyfish: a custom SDF shape (runtime-compiled GLSL snippet) -- smooth union
// of two circles with a scooped underside forms the pulsing bell. Drifts in the
// pool, steers away from the mouse, and pulse-brightens rose-pink when startled.
// Its emission feeds the GI like every other emitter.

CF_CustomShape jelly_shape;

typedef struct Jelly
{
	CF_V2 pos;
	CF_V2 vel;
	float phase;    // pulse phase
	float startle;  // 0..1, spikes when the mouse gets close
	float t;        // wander clock
} Jelly;

Jelly jelly;

void jelly_init()
{
	// Registered BEFORE any draw shaders are created (they bake in the set of
	// custom shapes that exist at compile time).
	jelly_shape = cf_make_custom_shape(
		"// params: a = center, b.x = bell radius, b.y = smoothing k\n"
		"float sdf(vec2 p, ShapeParams s)\n"
		"{\n"
		"	float d1 = length(p - s.a) - s.b.x;\n"
		"	vec2 c2 = s.a + vec2(0.0, -s.b.x * 0.45);\n"
		"	float d2 = length(p - c2) - s.b.x * 0.78;\n"
		"	float k = max(s.b.y, 1e-3);\n"
		"	float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);\n"
		"	float d = mix(d2, d1, h) - k * h * (1.0 - h);\n"
		"	vec2 c3 = s.a + vec2(0.0, -s.b.x * 1.05);\n"
		"	float d3 = length(p - c3) - s.b.x * 0.9;\n"
		"	return max(d, -d3);\n"
		"}\n");

	jelly.pos = cf_v2(350, 210); // the deep bowl, west of the knuckle
	jelly.vel = cf_v2(0, 0);
	jelly.phase = 0;
	jelly.startle = 0;
	jelly.t = 0;
}

void jelly_update(float dt)
{
	jelly.t += dt;
	jelly.phase += dt * (2.2f + jelly.startle * 2.5f);

	float pulse = sinf(jelly.phase);

	// Contraction stroke = a little upward thrust.
	if (pulse > 0) jelly.vel.y += pulse * 30.0f * dt;

	// Slow wander across the deep bowl (the shelf east of the knuckle is too
	// shallow for a jelly).
	float target_x = 355.0f + sinf(jelly.t * 0.11f) * 110.0f;
	jelly.vel.x += (target_x - jelly.pos.x) * 0.05f * dt;

	// Stay submerged: gentle spring toward a comfortable depth band.
	float surface = water_surface_at(jelly.pos.x);
	float lo = 200.0f, hi = surface - 18.0f;
	if (hi < lo + 10.0f) hi = lo + 10.0f;
	if (jelly.pos.y < lo) jelly.vel.y += (lo - jelly.pos.y) * 0.8f * dt;
	if (jelly.pos.y > hi) jelly.vel.y -= (jelly.pos.y - hi) * 0.8f * dt;

	// Startle: steer away from a close mouse and flash brighter.
	CF_V2 d = cf_sub(jelly.pos, mouse_world);
	float len = cf_len(d);
	if (len < 120.0f && len > 1e-3f) {
		float w = 1.0f - len / 120.0f;
		jelly.vel = cf_add(jelly.vel, cf_mul_v2_f(cf_div_v2_f(d, len), 90.0f * w * dt));
		jelly.startle = cf_min(jelly.startle + 2.5f * w * dt, 1.0f);
	}
	jelly.startle *= expf(-dt / 1.5f);

	jelly.vel = cf_mul_v2_f(jelly.vel, 0.97f);
	jelly.pos = cf_add(jelly.pos, cf_mul_v2_f(jelly.vel, dt));

	// Keep clear of the rock (bell + tentacles need ~45 px), and softly herd
	// it back if a startle shove aims it at the shallow shelf.
	float gd = cave_ground_sdf(jelly.pos);
	if (gd < 45.0f) {
		CF_V2 n = cave_ground_grad(jelly.pos);
		jelly.pos = cf_add(jelly.pos, cf_mul_v2_f(n, 45.0f - gd));
	}
	if (jelly.pos.x > 470.0f) jelly.vel.x -= (jelly.pos.x - 470.0f) * 1.5f * dt;
	if (jelly.pos.x < 240.0f) jelly.vel.x += (240.0f - jelly.pos.x) * 1.5f * dt;
}

// Draw the jellyfish into a scene pass. channel: 0 = emissivity, 1 = absorption, 2 = diffuse.
void draw_jelly(int channel)
{
	float pulse = sinf(jelly.phase);
	float bell_r = 24.0f * (1.0f + 0.10f * pulse);

	CF_Color bell, tent;
	if (channel == 0) {
		// Rose-pink, up to ~8 HDR when startled.
		float glow = 1.6f * (0.65f + 0.35f * cf_max(pulse, 0.0f)) * (1.0f + 5.0f * jelly.startle);
		bell = cf_make_color_rgb_f(1.0f * glow, 0.35f * glow, 0.5f * glow);
		tent = cf_make_color_rgb_f(0.35f * glow, 0.12f * glow, 0.18f * glow);
	} else if (channel == 1) {
		// Must absorb to emit; translucent watery flesh.
		bell = cf_make_color_rgb_f(0.45f, 0.3f, 0.32f);
		tent = cf_make_color_rgb_f(0.3f, 0.2f, 0.22f);
	} else {
		bell = cf_make_color_rgb_f(0.2f, 0.09f, 0.12f);
		tent = cf_make_color_rgb_f(0.1f, 0.05f, 0.06f);
	}

	if (jelly_shape.id) {
		float params[4] = { jelly.pos.x, jelly.pos.y, bell_r, 8.0f };
		CF_Aabb bounds = cf_make_aabb(
			cf_v2(jelly.pos.x - bell_r * 2.0f, jelly.pos.y - bell_r * 2.2f),
			cf_v2(jelly.pos.x + bell_r * 2.0f, jelly.pos.y + bell_r * 1.4f));
		cf_draw_push_color(bell);
		cf_draw_custom_shape_fill(jelly_shape, bounds, params, 4);
		cf_draw_pop_color();
	} else {
		// No runtime shader compilation: plain circle stand-in.
		cf_draw_push_color(bell);
		cf_draw_circle_fill2(jelly.pos, bell_r);
		cf_draw_pop_color();
	}

	// Three tentacles trailing with phase lag.
	cf_draw_push_color(tent);
	for (int i = 0; i < 3; i++) {
		float dx = ((float)i - 1.0f) * bell_r * 0.5f;
		float sway = sinf(jelly.t * 2.0f + (float)i * 1.3f - 1.1f) * 6.0f;
		float len = 26.0f + (float)i * 4.0f;
		CF_V2 p0 = cf_v2(jelly.pos.x + dx, jelly.pos.y - bell_r * 0.5f);
		CF_V2 p1 = cf_v2(jelly.pos.x + dx + sway, jelly.pos.y - bell_r * 0.5f - len);
		cf_draw_capsule_fill2(p0, p1, 1.6f);
	}
	cf_draw_pop_color();
}

//--------------------------------------------------------------------------------------------------
// Water rendering: density canvas + threshold shader.

CF_Canvas density_canvas; // 512x512 rgba16f: r = density, g = agitation density
CF_Shader blob_shd;
CF_Shader water_shd;

void draw_density()
{
	begin_design_draw();
	push_additive_f16_render_state();
	cf_draw_push_shader(blob_shd);
	for (int i = 0; i < particle_count; i++) {
		Particle* pt = particles + i;
		if (pt->airborne) continue; // drawn individually as droplets instead
		cf_draw_push_vertex_attributes(pt->p.x, pt->p.y, P_SPLAT, 0);
		cf_draw_push_color(cf_make_color_rgb_f(1.0f, pt->agitation, 0.0f));
		cf_draw_circle_fill2(pt->p, P_SPLAT);
		cf_draw_pop_color();
		cf_draw_pop_vertex_attributes();
	}
	cf_draw_pop_shader();
	cf_draw_pop_render_state();
	cf_render_to(density_canvas, true);
	end_canvas_draw();
}

// Draw the water field with the threshold shader. Mode selects the output
// (0 screen, 1 absorption, 2 emissivity, 3 albedo); col_a/col_b are per-mode.
void draw_water_field(float mode, CF_Color col_a, CF_Color col_b)
{
	float ws = (float)CAVE_REF; // density canvas covers the design box; the scene
	                            // transform (below) maps it into the world.
	float threshold = 0.55f;
	float hf_params[4] = { hf_x0, hf_x1, WATERLINE, root_time };
	// The water body is authored in design space but samples the world-space
	// fluence: hand it the world dims and design->world transform so it can map
	// its design (wx,wy) to fluence uv.
	float world4[4] = { (float)world_w, (float)world_h, 0, 0 };
	float scene4[4] = { scene_s, scene_ox, scene_oy, (float)CAVE_REF };
	cf_draw_push_shader(water_shd);
	cf_draw_set_texture("u_heightfield", hf_tex);
	cf_draw_set_texture("u_fluence", cf_canvas_get_target(hrc.fluence));
	cf_draw_set_texture("u_ground_sdf", gsdf_tex);
	cf_draw_set_uniform("u_col_a", &col_a, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_draw_set_uniform("u_col_b", &col_b, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_draw_set_uniform("u_hf", hf_params, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_draw_set_uniform("u_world", world4, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_draw_set_uniform("u_scene", scene4, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_draw_set_uniform_float("u_mode", mode);
	cf_draw_set_uniform_float("u_threshold", threshold);
	cf_draw_push_alpha_discard(false);
	cf_draw_canvas(density_canvas, cf_v2(ws * 0.5f, ws * 0.5f), cf_v2(ws, ws));
	cf_draw_pop_alpha_discard();
	cf_draw_pop_shader();
}

//--------------------------------------------------------------------------------------------------
// Scene canvas passes.

// The moon-slit emitter, embedded at the top of the crack channel: a slanted
// quad aligned with the crack, inset from its walls, overrunning the frame top
// so the visible bright area is exactly the crack aperture.
void draw_moon_emitter()
{
	// A bright slab filling the crack channel near the ceiling. Its bottom edge
	// (y0) sits a little below the visible frame top so the aperture reads as a
	// clean glowing slit, and it runs up past the frame (y1 > design top) so the
	// rock above stays sealed and the beam has headroom to develop. Inset from
	// the crack walls so light reads as sky through the aperture, never a block.
	float y0 = 1020.0f, y1 = 1080.0f;
	float hw = CRACK_HALF_W - 4.0f;
	float xb = CRACK_X0 + (y0 - CRACK_Y0) * CRACK_SLOPE;
	float xt = CRACK_X0 + (y1 - CRACK_Y0) * CRACK_SLOPE;
	cf_draw_quad_fill2(
		cf_v2(xt - hw, y1), cf_v2(xt + hw, y1),
		cf_v2(xb + hw, y0), cf_v2(xb - hw, y0), 0
	);
}

// Shape draws and canvas blits mix freely within one cf_render_to (the old
// quirk where a mid-pass blit re-applied the pending clear is fixed).

void draw_emissivity()
{
	begin_canvas_draw();
	push_f16_render_state();

	// Bioluminescent water glow: agitation-driven HDR cyan, saturating at
	// (0.5, 2.5, 1.9); alpha is the agitation-density gain.
	if (!skip('w')) draw_water_field(2.0f,
		cf_make_color_rgba_f(0.2f * 2.5f, 1.0f * 2.5f, 0.75f * 2.5f, 0.45f),
		cf_make_color_rgb_f(0.004f, 0.016f, 0.013f));

	// Moon shaft emitter: an HDR slab embedded in the crack channel's top,
	// aligned with the crack's slant and inset from its walls so the bright
	// region reads as sky through the aperture, never a floating block. Cool
	// blue-white -- authored in plain physical units, no encode. Sits just above
	// the visible frame so only the crack aperture is in view and the shaft
	// carries the moonlight down into the cave.
	cf_draw_push_color(cf_make_color_rgb_f(60.0f, 74.0f, 88.0f));
	draw_moon_emitter();
	cf_draw_pop_color();

	draw_drip(0);
	if (!skip('j')) draw_jelly(0);

	cf_draw_pop_render_state();
	cf_render_to(hrc.emissivity, true);
	end_canvas_draw();
}

void draw_absorption()
{
	begin_canvas_draw();
	push_f16_render_state();

	if (fog_on) {
		// Faint uniform fog so the moon shaft reads volumetrically, denser in a
		// band above the waterline. Sits underneath the water blit -- mid-pass
		// canvas blits no longer re-trigger the clear, so one pass suffices.
		cf_draw_push_color(cf_make_color_rgb_f(0.004f, 0.004f, 0.004f));
		cf_draw_quad_fill(cf_make_aabb(cf_v2(0, 0), cf_v2(1024, 1024)), 0);
		cf_draw_pop_color();
		cf_draw_push_color(cf_make_color_rgb_f(0.008f, 0.008f, 0.008f));
		cf_draw_quad_fill(cf_make_aabb(cf_v2(0, 270), cf_v2(1024, 470)), 0);
		cf_draw_pop_color();
	}

	// Water body: translucent, blue-weighted (red dies first underwater).
	if (!skip('w')) draw_water_field(1.0f,
		cf_make_color_rgb_f(0.10f, 0.045f, 0.035f),
		cf_color_clear());

	// Rock (replaces fog/water where solid).
	if (!skip('k')) cf_draw_list(list_rock_absorption);

	// The moon slit must absorb to emit (radiance = emiss * (1 - T)).
	cf_draw_push_color(cf_make_color_rgb_f(0.5f, 0.5f, 0.5f));
	draw_moon_emitter();
	cf_draw_pop_color();

	draw_drip(1);
	if (!skip('j')) draw_jelly(1);

	// Roots hanging through the crack: thin absorbers slicing the god ray.
	if (!skip('r')) {
		cf_draw_push_color(cf_make_color_rgb_f(30.0f, 30.0f, 30.0f));
		draw_roots(2.0f);
		cf_draw_pop_color();
	}

	cf_draw_pop_render_state();
	cf_render_to(hrc.absorption, true);
	end_canvas_draw();
}

void draw_diffuse()
{
	begin_canvas_draw();
	push_f16_render_state();

	// Water albedo: dim teal (subsurface-ish bounce).
	if (!skip('w')) draw_water_field(3.0f,
		cf_make_color_rgb_f(0.05f, 0.15f, 0.13f),
		cf_color_clear());

	// Rock albedo.
	if (!skip('k')) cf_draw_list(list_rock_diffuse);

	if (!skip('j')) draw_jelly(2);

	cf_draw_pop_render_state();
	cf_render_to(hrc.diffuse, true);
	end_canvas_draw();
}

//--------------------------------------------------------------------------------------------------
// Isolated-light debug mode (HRC_CAVE_TESTLIGHT).
//
// Draws the HRC scene inputs DIRECTLY in world space (no design->world scale,
// no letterbox, no scene_scissor): the world canvas fills the window at its true
// aspect and the cascade runs genuinely rectangular over it. This lets the
// rectangular cascade math be validated in isolation from the busy cave scene.

// World-space canvas projection: world (0,0) at bottom-left, (world_w,world_h)
// at top-right, no design transform. Mirrors begin_canvas_draw minus scene_s.
void begin_world_draw()
{
	cf_draw_push();
	cf_draw_TSR_absolute(cf_v2(0, 0), cf_v2(1, 1), 0);
	cf_draw_projection(cf_ortho_2d(0, 0, (float)world_w, (float)world_h));
	cf_draw_translate(-world_w * 0.5f, -world_h * 0.5f);
}

// Centered filled emissive disc of radius testlight_r (HRC_CAVE_TESTLIGHT_R,
// default 8 world units). A finite disc -- rather than a near-point source --
// spreads the emission over many cells so the rectangular cascade's ±45° seam
// spokes should soften/round out; sub-8px sources are a known HRC limitation.
// Bright, neutral-white HDR emission -- authored in linear physical units (no
// encode), exactly like the cave's moon emitter.
void draw_testlight_emissivity()
{
	begin_world_draw();
	push_f16_render_state();
	float cx = world_w * (0.5f + testlight_ox), cy = world_h * (0.5f + testlight_oy);
	cf_draw_push_color(cf_make_color_rgb_f(testlight_e, testlight_e, testlight_e));
	cf_draw_circle_fill2(cf_v2(cx, cy), testlight_r);
	cf_draw_pop_color();
	cf_draw_pop_render_state();
	cf_render_to(hrc.emissivity, true);
	cf_draw_pop();
}

// Absorption: the emitter cell must absorb to emit (radiance = emiss*(1-T)), at
// the same coefficient as the cave moon (0.5/px). TESTLIGHT=2 adds two dense box
// occluders offset diagonally from center so their shadows can be checked for
// straightness/orientation independent of the light's radial symmetry.
void draw_testlight_absorption()
{
	begin_world_draw();
	push_f16_render_state();
	float cx = world_w * (0.5f + testlight_ox), cy = world_h * (0.5f + testlight_oy);

	cf_draw_push_color(cf_make_color_rgb_f(0.5f, 0.5f, 0.5f));
	cf_draw_circle_fill2(cf_v2(cx, cy), testlight_r);
	cf_draw_pop_color();

	if (testlight >= 2) {
		// Offsets in world pixels (kept modest so both boxes stay on-screen at any
		// aspect). One box to the upper-right, one to the lower-left of center.
		float off = 0.16f * (float)(world_w < world_h ? world_w : world_h);
		float hw = 18.0f;
		cf_draw_push_color(cf_make_color_rgb_f(30.0f, 30.0f, 30.0f));
		cf_draw_quad_fill(cf_make_aabb(cf_v2(cx + off - hw, cy + off - hw), cf_v2(cx + off + hw, cy + off + hw)), 0);
		cf_draw_quad_fill(cf_make_aabb(cf_v2(cx - off - hw, cy - off - hw), cf_v2(cx - off + hw, cy - off + hw)), 0);
		cf_draw_pop_color();
	}

	cf_draw_pop_render_state();
	cf_render_to(hrc.absorption, true);
	cf_draw_pop();
}

//--------------------------------------------------------------------------------------------------
// Screen composition.

const char* debug_view = NULL; // HRC_CAVE_VIEW=emissivity|absorption|diffuse|density

void draw_screen()
{
	CF_V2 wsz = cf_v2((float)world_w, (float)world_h);

	if (debug_view) {
		CF_Canvas c = hrc.fluence;
		if (!CF_STRCMP(debug_view, "emissivity")) c = hrc.emissivity;
		else if (!CF_STRCMP(debug_view, "absorption")) c = hrc.absorption;
		else if (!CF_STRCMP(debug_view, "diffuse")) c = hrc.diffuse;
		else if (!CF_STRCMP(debug_view, "density")) c = density_canvas;
		cf_draw_canvas(c, cf_v2(0, 0), wsz);
		return;
	}

	// Clip the whole composition to the design box: the world outside it is a
	// letterbox margin, kept black (the emitter's faint upward glow would
	// otherwise bleed into it). Reduces to the full window at a square world.
	cf_draw_push_scissor(scene_scissor());

	if (gi_on) {
		cf_draw_canvas(hrc.fluence, cf_v2(0, 0), wsz);
	} else {
		// GI off: flat dim ambient on the albedo so the scene stays legible.
		cf_draw_push_color(cf_make_color_rgb_f(0.55f, 0.55f, 0.6f));
		cf_draw_canvas(hrc.diffuse, cf_v2(0, 0), wsz);
		cf_draw_pop_color();
	}

	// World-space overlays: root silhouettes + water surface. Same design->world
	// transform as begin_canvas_draw, over the app's centered screen projection.
	cf_draw_push();
	cf_draw_translate(-world_w * 0.5f, -world_h * 0.5f);
	cf_draw_translate(scene_ox, scene_oy);
	cf_draw_scale(scene_s, scene_s);

	if (!skip('r')) {
		cf_draw_push_color(cf_make_color_rgb_f(0.02f, 0.02f, 0.025f));
		draw_roots(2.0f);
		cf_draw_pop_color();
	}

	if (water_sdf_on) {
		// col_a = interior ambient teal (a = body opacity), col_b = spec tint.
		draw_water_field(0.0f,
			cf_make_color_rgba_f(0.010f, 0.045f, 0.045f, 0.92f),
			cf_make_color_rgba_f(0.55f, 0.9f, 0.85f, 1.0f));
	} else {
		// Debug: plain circles per particle.
		cf_draw_push_color(cf_make_color_rgb_f(0.05f, 0.14f, 0.13f));
		for (int i = 0; i < particle_count; i++) {
			cf_draw_circle_fill2(particles[i].p, P_RADIUS);
		}
		cf_draw_pop_color();
	}

	// Airborne droplets: lone ejecta drawn as soft-cored beads (blob falloff)
	// with a tiny moonward glint, instead of merging into the metaball body.
	cf_draw_push_shader(blob_shd);
	for (int i = 0; i < particle_count; i++) {
		Particle* pt = particles + i;
		if (!pt->airborne) continue;
		cf_draw_push_vertex_attributes(pt->p.x, pt->p.y, 4.0f, 0);
		cf_draw_push_color(cf_make_color_rgba_f(0.05f, 0.17f, 0.16f, 0.8f));
		cf_draw_circle_fill2(pt->p, 4.0f);
		cf_draw_pop_color();
		cf_draw_pop_vertex_attributes();
		CF_V2 g = cf_v2(pt->p.x - 0.9f, pt->p.y + 1.0f);
		cf_draw_push_vertex_attributes(g.x, g.y, 1.4f, 0);
		cf_draw_push_color(cf_make_color_rgba_f(0.55f, 0.75f, 0.85f, 0));
		cf_draw_circle_fill2(g, 1.4f);
		cf_draw_pop_color();
		cf_draw_pop_vertex_attributes();
	}

	// Spray: fading additive glints.
	for (int i = 0; i < spray_count; i++) {
		Spray* s = sprays + i;
		float t = s->life / SPRAY_LIFE;
		cf_draw_push_vertex_attributes(s->p.x, s->p.y, 2.2f, 0);
		cf_draw_push_color(cf_make_color_rgba_f(0.65f * t, 0.95f * t, 0.9f * t, 0));
		cf_draw_circle_fill2(s->p, 2.2f);
		cf_draw_pop_color();
		cf_draw_pop_vertex_attributes();
	}
	cf_draw_pop_shader();

	cf_draw_pop();
	cf_draw_pop_scissor();
}

//--------------------------------------------------------------------------------------------------
// Overlay.

void draw_hud()
{
	float hx = (float)world_w * 0.5f;
	float hy = (float)world_h * 0.5f;
	smoothed_fps = smoothed_fps * 0.95f + (1.0f / cf_max(CF_DELTA_TIME, 1e-4f)) * 0.05f;

	cf_draw_push_color(cf_make_color_rgba_f(0.7f, 0.75f, 0.8f, 0.9f));
	cf_push_font_size(15.0f);
	if (show_overlay) {
		char buf[512];
		snprintf(buf, sizeof(buf),
			"BIOLUMINAL   %.0f fps\n"
			"mouse move: stir   click: splash\n"
			"[1] GI: %s\n"
			"[2] fog: %s\n"
			"[3] rim light: %s\n"
			"[4] SDF water: %s\n"
			"[5] bounce feedback: %s\n"
			"[6] physics: %s\n"
			"[F1] hide overlay",
			smoothed_fps,
			gi_on ? "on" : "off",
			fog_on ? "on" : "off",
			rim_on ? "on" : "off",
			water_sdf_on ? "on" : "circles",
			bounce_on ? "on" : "off",
			paused ? "paused" : "on");
		cf_draw_text(buf, cf_v2(-hx + 12.0f, hy - 14.0f), -1);
	} else {
		cf_draw_text("F1", cf_v2(-hx + 12.0f, hy - 14.0f), -1);
	}
	cf_pop_font_size();
	cf_draw_pop_color();
}

void handle_input()
{
	if (cf_key_just_pressed(CF_KEY_F1)) show_overlay = !show_overlay;
	if (cf_key_just_pressed(CF_KEY_1)) gi_on = !gi_on;
	if (cf_key_just_pressed(CF_KEY_2)) fog_on = !fog_on;
	if (cf_key_just_pressed(CF_KEY_3)) rim_on = !rim_on;
	if (cf_key_just_pressed(CF_KEY_4)) water_sdf_on = !water_sdf_on;
	if (cf_key_just_pressed(CF_KEY_5)) bounce_on = !bounce_on;
	if (cf_key_just_pressed(CF_KEY_6)) paused = !paused;

	float dt = cf_max(CF_DELTA_TIME, 1e-4f);
	CF_V2 prev = mouse_world;
	if (autostir) {
		// A hand swishing back and forth along the deep bowl's surface.
		autostir_t += dt;
		mouse_world = cf_v2(350.0f + sinf(autostir_t * 0.9f) * 150.0f, 262.0f);
	} else {
		// Screen pixel -> world (y up) -> design space (the sim's coordinates).
		mouse_world = world_to_design(cf_v2(cf_mouse_x(), (float)world_h - cf_mouse_y()));
	}
	mouse_vel = cf_div_v2_f(cf_sub(mouse_world, prev), dt);

	if (!autostir && cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)) {
		water_impulse(mouse_world, 120.0f, 220.0f, 1.0f);
	}
}

//--------------------------------------------------------------------------------------------------
// HDR draw-color smoke test: draw (4,2,1) to an rgba16f canvas and read it back.

float half_to_float(uint16_t h)
{
	uint32_t sign = (uint32_t)(h >> 15) & 1;
	uint32_t exp = (uint32_t)(h >> 10) & 0x1f;
	uint32_t man = (uint32_t)h & 0x3ff;
	if (exp == 0) return (sign ? -1.0f : 1.0f) * (float)man * (1.0f / 16777216.0f);
	if (exp == 31) return sign ? -1e30f : 1e30f;
	union { uint32_t u; float f; } v;
	v.u = (sign << 31) | ((exp + 112) << 23) | (man << 13);
	return v.f;
}

void hdr_smoke_test()
{
	int n = 64;
	CF_Canvas c = hrc_make_canvas(n, n, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	cf_draw_push();
	cf_draw_TSR_absolute(cf_v2(0, 0), cf_v2(1, 1), 0);
	cf_draw_projection(cf_ortho_2d(0, 0, (float)n, (float)n));
	cf_draw_translate((float)-n * 0.5f, (float)-n * 0.5f);
	push_f16_render_state();
	cf_draw_push_color(cf_make_color_rgb_f(4.0f, 2.0f, 1.0f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(0, 0), cf_v2((float)n, (float)n)), 0);
	cf_draw_pop_color();
	cf_draw_pop_render_state();
	cf_render_to(c, true);
	cf_draw_pop();

	// Submit the frame's GPU work so the readback sees the draw.
	cf_app_draw_onto_screen(false);

	CF_Readback rb = cf_canvas_readback(c);
	while (!cf_readback_ready(rb)) {}
	uint16_t* px = (uint16_t*)cf_alloc(n * n * 8);
	cf_readback_data(rb, px, n * n * 8);
	cf_destroy_readback(rb);
	int center = (n / 2 * n + n / 2) * 4;
	printf("HDR smoke test: drew (4,2,1), read back (%.3f, %.3f, %.3f) -- %s\n",
		half_to_float(px[center + 0]), half_to_float(px[center + 1]), half_to_float(px[center + 2]),
		half_to_float(px[center]) > 3.5f ? "PASS" : "FAIL");
	fflush(stdout);
	cf_free(px);
	cf_destroy_canvas(c);
}

//--------------------------------------------------------------------------------------------------
// Entry point.

int main(int argc, char* argv[])
{
	// Window / world resolution. HRC_CAVE_RES=WxH picks a non-square resolution
	// (default the square CAVE_REF). Rounded to a multiple of 2*CAVE_UPSCALE so
	// grid_w/grid_h = world/upscale stay integral and even.
	world_w = CAVE_REF;
	world_h = CAVE_REF;
	{
		const char* res = getenv("HRC_CAVE_RES");
		if (res) {
			int rw = 0, rh = 0;
			if (sscanf(res, "%dx%d", &rw, &rh) == 2 && rw >= 256 && rh >= 256 && rw <= 4096 && rh <= 4096) {
				int q = 2 * CAVE_UPSCALE;
				world_w = (rw / q) * q;
				world_h = (rh / q) * q;
			}
		}
	}
	grid_w = world_w / CAVE_UPSCALE;
	grid_h = world_h / CAVE_UPSCALE;

	// Design-space -> world: uniform scale to the smaller axis (never distort),
	// centered. A square world gives s=1, offset 0 (identical to the old layout).
	scene_s = (world_w < world_h ? world_w : world_h) / (float)CAVE_REF;
	scene_ox = (world_w - CAVE_REF * scene_s) * 0.5f;
	scene_oy = (world_h - CAVE_REF * scene_s) * 0.5f;

	cf_make_app("Bioluminal", 0, 0, 0, world_w, world_h, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	cf_clear_color(0, 0, 0, 1);

	int perf = getenv("HRC_CAVE_PERF") != NULL;
	autostir = getenv("HRC_CAVE_AUTOSTIR") != NULL;
	debug_view = getenv("HRC_CAVE_VIEW");
	{ const char* tl = getenv("HRC_CAVE_TESTLIGHT"); if (tl) testlight = atoi(tl); }
	{ const char* te = getenv("HRC_CAVE_TESTLIGHT_E"); if (te) testlight_e = (float)atof(te); }
	{ const char* tr = getenv("HRC_CAVE_TESTLIGHT_R"); if (tr) testlight_r = (float)atof(tr); }
	{ const char* to = getenv("HRC_CAVE_TESTLIGHT_OX"); if (to) testlight_ox = (float)atof(to); }
	{ const char* to = getenv("HRC_CAVE_TESTLIGHT_OY"); if (to) testlight_oy = (float)atof(to); }

	// Scripted-run overrides for the F1 toggles.
	{
		const char* env;
		if ((env = getenv("HRC_CAVE_GI")))     gi_on = atoi(env);
		if ((env = getenv("HRC_CAVE_FOG")))    fog_on = atoi(env);
		if ((env = getenv("HRC_CAVE_RIM")))    rim_on = atoi(env);
		if ((env = getenv("HRC_CAVE_WATER"))) water_sdf_on = atoi(env);
		if ((env = getenv("HRC_CAVE_BOUNCE"))) bounce_on = atoi(env);
		if ((env = getenv("HRC_CAVE_OVERLAY"))) show_overlay = atoi(env);
		if ((env = getenv("HRC_CAVE_SKIP"))) skip_flags = env;
	}

	// Custom SDF shapes must register first: registration recompiles the
	// renderer's internal shaders, and draw shaders/recorded geometry bake in
	// the shape set that exists when they are created.
	jelly_init();

	hrc_init();
	if (testlight) {
		printf("TESTLIGHT=%d world=%dx%d grid=%dx%d n_horiz=%d n_vert=%d upscale=%d\n",
			testlight, world_w, world_h, grid_w, grid_h, n_horiz, n_vert, CAVE_UPSCALE);
		fflush(stdout);
	}
	record_rock_lists();
	particles_init();

	// Fixed drip RNG for reproducible screenshots, otherwise time-seeded.
	cave_rnd = cf_rnd_seed(getenv("HRC_CAVE_SCREENSHOT_MODE") ? 12345 : (uint64_t)cf_get_ticks());
	spray_rnd = cf_rnd_seed(777); // separate stream: spray must not perturb the drip script's RNG
	plink_audio = make_plink_audio();
	drip_co = cf_make_coroutine(drip_script, 0, NULL);

	density_canvas = hrc_make_canvas(512, 512, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_LINEAR);
	ground_bake_sdf_tex();
	heightfield_init();
	blob_shd = load_draw_shader("/hrc_cave_data/cave_blob.shd");
	water_shd = load_draw_shader("/hrc_cave_data/cave_water.shd");

	int want_hdr_test = getenv("HRC_CAVE_HDRTEST") != NULL;

	int frame = 0;
	double frame_ms_acc = 0;

	while (cf_app_is_running()) {
		uint64_t t0 = cf_get_ticks();
		cf_app_update(NULL);

		if (want_hdr_test) {
			// Needs an active GPU command buffer, so runs after the first app
			// update; it submits the frame itself, so skip the rest of it.
			want_hdr_test = 0;
			hdr_smoke_test();
			continue;
		}

		// Isolated-light mode: skip the whole cave (sim + scene draws + letterbox)
		// and run ONLY the rectangular HRC cascade over a world-space test scene.
		if (testlight) {
			cf_draw_push_shape_aa(0);
			draw_testlight_emissivity();
			draw_testlight_absorption();
			cf_draw_pop_shape_aa();

			hrc_compute();

			if (debug_view) {
				CF_Canvas c = hrc.fluence;
				if (!CF_STRCMP(debug_view, "emissivity")) c = hrc.emissivity;
				else if (!CF_STRCMP(debug_view, "absorption")) c = hrc.absorption;
				cf_draw_canvas(c, cf_v2(0, 0), cf_v2((float)world_w, (float)world_h));
			} else {
				cf_draw_canvas(hrc.fluence, cf_v2(0, 0), cf_v2((float)world_w, (float)world_h));
			}
			cf_app_draw_onto_screen(true);

			frame_ms_acc += (double)(cf_get_ticks() - t0) / (double)cf_get_tick_frequency() * 1000.0;
			if (++frame % 60 == 0 && perf) {
				printf("testlight frame avg %.2f ms, grid %dx%d n_h=%d n_v=%d\n",
					frame_ms_acc / 60.0, grid_w, grid_h, n_horiz, n_vert);
				fflush(stdout);
				frame_ms_acc = 0;
			}
			continue;
		}

		handle_input();
		if (!paused) {
			g_dt = cf_min(CF_DELTA_TIME, 1.0f / 30.0f);
			root_time += g_dt;
			if (!skip('p')) particles_update(g_dt);
			heightfield_update(g_dt);
			spray_update(g_dt);
			jelly_update(g_dt);
			cf_coroutine_resume(drip_co);
		}

		// Canvas passes: no AA on the scene canvases (crisp binary occupancy for HRC).
		cf_draw_push_shape_aa(0);
		if (!skip('w')) draw_density();
		draw_emissivity();
		draw_absorption();
		draw_diffuse();
		cf_draw_pop_shape_aa();

		if (bounce_on && gi_on) hrc_feedback();
		if (gi_on) hrc_compute();

		draw_screen();
		draw_hud();
		cf_app_draw_onto_screen(true);

		frame_ms_acc += (double)(cf_get_ticks() - t0) / (double)cf_get_tick_frequency() * 1000.0;
		if (++frame % 60 == 0 && perf) {
			printf("frame avg %.2f ms over last 60, mean particle speed %.2f px/s\n", frame_ms_acc / 60.0, mean_speed);
			fflush(stdout);
			frame_ms_acc = 0;
		}
	}

	cf_destroy_coroutine(drip_co);
	cf_audio_destroy(plink_audio);
	cf_free(plink_wav);
	cf_destroy_draw_list(list_rock_absorption);
	cf_destroy_draw_list(list_rock_diffuse);
	cf_destroy_canvas(density_canvas);
	cf_destroy_texture(hf_tex);
	cf_destroy_texture(gsdf_tex);
	cf_destroy_shader(blob_shd);
	cf_destroy_shader(water_shd);
	hrc_shutdown();
	cf_destroy_app();
	return 0;
}
