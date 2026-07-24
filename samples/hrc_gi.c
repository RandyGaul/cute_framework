// HRC 2D global illumination -- a compute-driven radiance-cascade GI renderer,
// validated bit-exact against amitabha (Sannikov's reference implementation).
//
// The scene is authored as two canvases holding physical units: linear EMISSION
// and an ABSORPTION coefficient per world pixel. Absorption is continuous, so a
// surface is just a cell with huge absorption and fog is a cell with a little --
// the same code path gives hard shadows and volumetric transmission.
//
// Pipeline: direct-trace the low cascade levels, merge_up the rest, merge down,
// then scatter-reconstruct per pixel. Output is directional (2D Fourier L1), so
// surfaces can be shaded from a normal rather than just a scalar fluence.
//
// Scenes (HRC_SCENE=): cornell | glass | pinhole | rectroom | circ | showcase
//
// Env vars:
//   HRC_SCENE=name      pick a scene (default cornell)
//   HRC_RES=WxH         window / world resolution (default 1024x1024)
//   HRC_SCALE=1|2|4     run the probe lattice at 1/N res and upscale
//   HRC_EXPOSURE=f      display exposure bias
//   HRC_SHOT=1          headless: render, dump, exit

#include <cute.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

//--------------------------------------------------------------------------------------------------
// Configuration.
//
// The cave is authored in a fixed 1024x1024 DESIGN space (GI_REF). The runtime
// WORLD (scene canvases + GI grid + window) can be any non-square resolution: the
// design scene is uniformly scaled by scene_s and centered into the world, so
// shapes never distort and a square world reduces exactly to the old 1x layout.
// The HRC cascade runs genuinely rectangular over the full world grid.

#define GI_REF      1024      // design-space resolution the scene is authored in
#define GI_UPSCALE  2         // world pixels per cascade grid cell (per axis)
#define GI_N_MAX    11        // array sizing: log2_ceil of the largest grid axis
#define GI_TRACE_LEVELS 3  // direct-trace levels 0..2, merge_up (extend) above (amitabha uses 2)
#define GI_WG    16
#define GI_ABS_THRESHOLD 0.1f

// Runtime world / grid dimensions (set in main() from the window size).
int world_w = GI_REF, world_h = GI_REF;   // scene canvas + display resolution
int grid_w = GI_REF / GI_UPSCALE;         // cascade probe lattice (world / upscale)
int grid_h = GI_REF / GI_UPSCALE;
int n_horiz = 9, n_vert = 9, n_max = 9;       // per-axis cascade depth (log2_ceil grid)

// Design-space -> world transform: uniform scale + centering offset.
float scene_s = 1.0f;
float scene_ox = 0.0f, scene_oy = 0.0f;




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
	const char* err = cf_shader_compile_error();
	if (err && err[0]) { printf("SHADER COMPILE ERROR in %s:\n%s\n", path, err); fflush(stdout); }
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



void hrc_init()
{
	CF_MEMSET(&hrc, 0, sizeof(hrc));

	// Per-axis cascade depth. Horizontal rotations (0,2) cascade along the width
	// (grid_w); vertical rotations (1,3) along the height (grid_h).
	n_horiz = hrc_log2_ceil(grid_w);
	n_vert = hrc_log2_ceil(grid_h);
	n_max = n_horiz > n_vert ? n_horiz : n_vert;


	hrc.emissivity = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	hrc.absorption = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	hrc.diffuse = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	hrc.fluence = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R8G8B8A8_UNORM, CF_FILTER_LINEAR);
	hrc.fluence_lin = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);

}

void hrc_shutdown()
{
	cf_destroy_canvas(hrc.emissivity);
	cf_destroy_canvas(hrc.absorption);
	cf_destroy_canvas(hrc.diffuse);
	cf_destroy_canvas(hrc.fluence);
	cf_destroy_canvas(hrc.fluence_lin);
}

//--------------------------------------------------------------------------------------------------
// Feature toggles (F1 overlay).



int show_overlay = 0;
int gi_on = 1;
int fog_on = 1;
int rim_on = 1;
int water_sdf_on = 1;
int bounce_on = 1;
int paused = 0;
float smoothed_fps = 60.0f;

// HRC_TESTLIGHT: isolated-light debug mode that exercises the rectangular
// cascade in isolation (no water/rock/jelly/drip/runes/roots, no design-space
// letterbox). The scene is authored DIRECTLY in world space filling the window.
//   1 = a single centered 8x8 emissive square (the paper's minimum supported
//       emitter, matching samples/hrc.c's known-good static test light). Pure
//       radial-symmetry test: a centered point light must read the same at any
//       world aspect (circle, no directional bias, no spokes).
//   2 = the 8x8 light plus two box occluders offset from center (shadow-shape
//       check: shadows must be straight and correctly oriented).
int testlight = 0;
float testlight_e = 2.0f; // HRC_TESTLIGHT_E: emitter linear emission (per channel)
float testlight_r = 8.0f; // HRC_TESTLIGHT_R: emitter radius in world units. Default 8,
                          // the paper's minimum supported source size -- a filled disc of this
                          // radius should spread/soften the ±45° seam spokes that a near-point
                          // source maximizes. Sweep smaller (1/2) to reproduce the point-source X.
float testlight_ox = 0.0f, testlight_oy = 0.0f; // HRC_TESTLIGHT_OX/OY: light offset from
                                                // world center, as a fraction of world_w/world_h

//--------------------------------------------------------------------------------------------------
// Cascade compute pipeline (fixed config).


// Multibounce feedback: inject last frame's fluence, tinted by albedo, into emissivity.

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
	int box = (int)(GI_REF * scene_s + 0.5f);
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
	float ws = (float)GI_REF;
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

static void ap_hsv(float h, float s, float v, float* r, float* g, float* b)
{
	float c = v * s, x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f)), m = v - c;
	float rr = 0, gg = 0, bb = 0;
	if (h < 60)      { rr = c; gg = x; }
	else if (h < 120){ rr = x; gg = c; }
	else if (h < 180){ gg = c; bb = x; }
	else if (h < 240){ gg = x; bb = c; }
	else if (h < 300){ rr = x; bb = c; }
	else             { rr = c; bb = x; }
	*r = rr + m; *g = gg + m; *b = bb + m;
}

static void ap_rect(float cx, float cy, float hw, float hh)
{
	cf_draw_quad_fill(cf_make_aabb(cf_v2(cx - hw, cy - hh), cf_v2(cx + hw, cy + hh)), 0);
}

// Compute-shader rasterizer: exact replica of amitabha's rect_brush coverage
// (|pos+0.5 - center| < size). One dispatch per rect, later overwrites earlier.
static CF_ComputeShader g_cs_raster;
static CF_Material      g_m_raster;
static int              g_raster_inited = 0;
static float            g_write_emission = 1.0f;
static void ami_raster_shape(CF_Texture emis_tex, CF_Texture opac_tex, int D,
                             float is_circle, float radius,
                             float cx, float cy, float sx, float sy,
                             float er, float eg, float eb,
                             float or_, float og, float ob)
{
	float rect[4]  = { cx, cy, sx, sy };
	float em[4]    = { er, eg, eb, 0.0f };
	float op[4]    = { or_, og, ob, 0.0f };
	float shape[4] = { is_circle, radius, g_write_emission, 0.0f };
	cf_material_set_uniform_cs(g_m_raster, "u_rect",       rect,  CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_cs(g_m_raster, "u_emission_c", em,    CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_cs(g_m_raster, "u_opacity_c",  op,    CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_cs(g_m_raster, "u_shape",      shape, CF_UNIFORM_TYPE_FLOAT4, 1);
	CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(D, 8), hrc_div_ceil(D, 8), 1);
	CF_Texture rw[2] = { emis_tex, opac_tex };
	d.rw_textures = rw; d.rw_texture_count = 2;
	cf_dispatch_compute(g_cs_raster, g_m_raster, d);
}
static void ami_raster_rect(CF_Texture et, CF_Texture ot, int D,
                            float cx, float cy, float sx, float sy,
                            float er, float eg, float eb, float or_, float og, float ob)
{
	ami_raster_shape(et, ot, D, 0.0f, 0.0f, cx, cy, sx, sy, er, eg, eb, or_, og, ob);
}


//--------------------------------------------------------------------------------------------------
// Non-square exact HRC (unified 2-group model): horizontal group (rot 0,2;
// cascade=width) + vertical group (rot 1,3; cascade=height), each 4 segments.
// Handles square and non-square uniformly. ami_ns_trace/merge/finish + ami_display.
static struct {
	int inited, maxd, nlev;
	CF_StorageBuffer Trad[12], Ttrn[12], Rbuf[2], Rzero, R0h, R0v;
	CF_Canvas dir[3];   // directional radiance, 2D Fourier L1: a0, a1, b1
	CF_Canvas minmax;   // prefiltered box min/max absorption at HRC res
	CF_ComputeShader cs_trace, cs_extend, cs_merge, cs_finish, cs_display, cs_minmax;
	CF_Material m_trace, m_extend, m_merge, m_finish, m_display, m_minmax;
} nsx;

// Display scale for the exact path's raw-AgX. The cave's authored emissivity sits
// ~6x below amitabha's raw-AgX range; test scenes use amitabha-scale emission (1.0).
#define GI_DISPLAY_SCALE 6.0f
float g_ns_exposure = GI_DISPLAY_SCALE;

// HRC_SCALE: run the cascade at 1/N resolution and upscale on display.
// The DDA still marches the FULL-RES scene, so occlusion stays sharp; only the
// probe lattice and radiance buffers shrink (N^2 fewer probes).
int g_hrc_scale = 1;
int hrc_w(void) { return world_w / g_hrc_scale; }
int hrc_h(void) { return world_h / g_hrc_scale; }

static void hrc_ns_init_once(void)
{
	if (nsx.inited) return;
	int gw = hrc_w() / 2, gh = hrc_h() / 2;
	int MAXD = gw > gh ? gw : gh;
	int NLEV = hrc_log2_ceil(MAXD) + 1;   // levels 0..NLEV-1
	nsx.maxd = MAXD; nsx.nlev = NLEV;
	for (int n = 0; n < NLEV; n++) {
		int sx = hrc_div_ceil(MAXD, 1 << n), dirs = 2 << n, nrays = dirs + 1;
		int elems = sx * (4 * MAXD) * nrays;
		nsx.Trad[n] = hrc_make_buf(elems, 1);
		nsx.Ttrn[n] = hrc_make_buf(elems, 1);
	}
	int maxR = 0;
	for (int n = 0; n < NLEV; n++) { int e = hrc_div_ceil(MAXD, 1 << n) * (4 * MAXD) * (2 << n); if (e > maxR) maxR = e; }
	nsx.Rbuf[0] = hrc_make_buf(maxR, 1);
	nsx.Rbuf[1] = hrc_make_buf(maxR, 1);
	nsx.Rzero   = hrc_make_buf(maxR, 1);
	{ void* z = cf_calloc((size_t)maxR * 8, 1); cf_update_storage_buffer(nsx.Rzero, z, (size_t)maxR * 8); cf_free(z); }
	int r0sz = MAXD * 2 * (4 * MAXD); // = 8*MAXD*MAXD
	nsx.R0h = hrc_make_buf(r0sz, 1);
	nsx.R0v = hrc_make_buf(r0sz, 1);
	for (int i = 0; i < 3; i++)
		nsx.dir[i] = hrc_make_canvas(hrc_w(), hrc_h(), CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	nsx.cs_trace   = load_compute_shader("/hrc_gi_data/ami_ns_trace.c_shd");
	nsx.cs_extend  = load_compute_shader("/hrc_gi_data/ami_ns_extend.c_shd");
	nsx.cs_merge   = load_compute_shader("/hrc_gi_data/ami_ns_merge.c_shd");
	nsx.cs_finish  = load_compute_shader("/hrc_gi_data/ami_ns_finish.c_shd");
	nsx.cs_display = load_compute_shader("/hrc_gi_data/ami_display.c_shd");
	nsx.cs_minmax  = load_compute_shader("/hrc_gi_data/ami_minmax.c_shd");
	nsx.minmax = hrc_make_canvas(hrc_w(), hrc_h(), CF_PIXEL_FORMAT_R16G16_FLOAT, CF_FILTER_NEAREST);
	nsx.m_trace = cf_make_material(); nsx.m_extend = cf_make_material(); nsx.m_merge = cf_make_material();
	nsx.m_finish = cf_make_material(); nsx.m_display = cf_make_material();
	nsx.m_minmax = cf_make_material();
	nsx.inited = 1;
}

// Run one rotation group (trace -> merge -> R0_group).
static void hrc_ns_group(CF_Texture emis_tex, CF_Texture opac_tex,
                         int rot0, int rot1, int casc_grid, int cross_grid, CF_StorageBuffer R0out)
{
	int NLEV = hrc_log2_ceil(casc_grid) + 1;
	// Direct-trace only the lowest GI_TRACE_LEVELS; build the rest with merge_up
	// (ami_ns_extend), which composites two half-length intervals from the level
	// below instead of re-marching the scene. Direct tracing costs O(2^n) per probe
	// at level n, so the deep levels dominate -- amitabha traces 2 by default.
	for (int n = 0; n < NLEV && n < GI_TRACE_LEVELS; n++) {
		int sx = hrc_div_ceil(casc_grid, 1 << n), dirs = 2 << n, nrays = dirs + 1;
		int P[10] = { n, hrc_w()/2, hrc_h()/2, hrc_w(), hrc_h(), rot0, rot1, g_hrc_scale, world_w, world_h };
		cf_material_set_uniform_cs(nsx.m_trace, "u_level",   P + 0, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_trace, "u_grid_w",  P + 1, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_trace, "u_grid_h",  P + 2, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_trace, "u_world_w", P + 3, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_trace, "u_world_h", P + 4, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_trace, "u_rot0",    P + 5, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_trace, "u_rot1",    P + 6, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_trace, "u_scale",   P + 7, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_trace, "u_full_w",  P + 8, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_trace, "u_full_h",  P + 9, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_texture_cs(nsx.m_trace, "u_emission", emis_tex);
		cf_material_set_texture_cs(nsx.m_trace, "u_opacity",  opac_tex);
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(sx * nrays, 16), hrc_div_ceil(4 * cross_grid, 16), 1);
		CF_StorageBuffer rw[2] = { nsx.Trad[n], nsx.Ttrn[n] }; d.rw_buffers = rw; d.rw_buffer_count = 2;
		cf_dispatch_compute(nsx.cs_trace, nsx.m_trace, d);
	}
	// Extend remaining levels from the level below (merge_up).
	for (int n = GI_TRACE_LEVELS; n < NLEV; n++) {
		int sx = hrc_div_ceil(casc_grid, 1 << n), dirs = 2 << n, nrays = dirs + 1;
		int P[3] = { n, casc_grid, cross_grid };
		cf_material_set_uniform_cs(nsx.m_extend, "u_level",      P + 0, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_extend, "u_casc_grid",  P + 1, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_extend, "u_cross_grid", P + 2, CF_UNIFORM_TYPE_INT, 1);
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(sx * nrays, 16), hrc_div_ceil(4 * cross_grid, 16), 1);
		CF_StorageBuffer ro[2] = { nsx.Trad[n - 1], nsx.Ttrn[n - 1] }; d.ro_buffers = ro; d.ro_buffer_count = 2;
		CF_StorageBuffer rw[2] = { nsx.Trad[n], nsx.Ttrn[n] }; d.rw_buffers = rw; d.rw_buffer_count = 2;
		cf_dispatch_compute(nsx.cs_extend, nsx.m_extend, d);
	}
	// merge down (NLEV-2)..0, final level 0 -> R0out
	int r_out = 0;
	for (int i = NLEV - 2; i >= 0; i--) {
		int sx = hrc_div_ceil(casc_grid, 1 << i), dirs = 2 << i;
		int P[3] = { i, casc_grid, cross_grid };
		cf_material_set_uniform_cs(nsx.m_merge, "u_level",     P + 0, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_merge, "u_casc_grid", P + 1, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_merge, "u_cross_grid",P + 2, CF_UNIFORM_TYPE_INT, 1);
		CF_StorageBuffer Rprev = (i == NLEV - 2) ? nsx.Rzero : nsx.Rbuf[1 - r_out];
		CF_StorageBuffer Rdst  = (i == 0) ? R0out : nsx.Rbuf[r_out];
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(sx, 8), hrc_div_ceil(dirs, 8), 4 * cross_grid);
		CF_StorageBuffer ro[5] = { nsx.Trad[i], nsx.Ttrn[i], nsx.Trad[i + 1], nsx.Ttrn[i + 1], Rprev };
		d.ro_buffers = ro; d.ro_buffer_count = 5;
		CF_StorageBuffer rw[1] = { Rdst }; d.rw_buffers = rw; d.rw_buffer_count = 1;
		cf_dispatch_compute(nsx.cs_merge, nsx.m_merge, d);
		if (i > 0) r_out ^= 1;
	}
}

void hrc_ns_compute(void)
{
	hrc_ns_init_once();
	CF_Texture emis_tex = cf_canvas_get_target(hrc.emissivity);
	CF_Texture opac_tex = cf_canvas_get_target(hrc.absorption);

	// horizontal group: rot 0,2  cascade=width;  vertical group: rot 1,3  cascade=height.
	int gw = hrc_w() / 2, gh = hrc_h() / 2;
	hrc_ns_group(emis_tex, opac_tex, 0, 2, gw, gh, nsx.R0h);
	hrc_ns_group(emis_tex, opac_tex, 1, 3, gh, gw, nsx.R0v);

	// finish: each rotation scatters into its OWN texture.  rot0->(R0h,local0)
	//         rot2->(R0h,local1)  rot1->(R0v,local0)  rot3->(R0v,local1).
	for (int i = 0; i < 3; i++) cf_render_to(nsx.dir[i], true); // clear (scatter skips edges)
	int rots[4]      = { 0, 2, 1, 3 };
	int local[4]     = { 0, 1, 0, 1 };
	CF_StorageBuffer grp[4] = { nsx.R0h, nsx.R0h, nsx.R0v, nsx.R0v };
	for (int k = 0; k < 4; k++) {
		int rot = rots[k];
		int horiz = (rot == 0 || rot == 2);
		int world_c = horiz ? hrc_w() : hrc_h();
		int world_x = horiz ? hrc_h() : hrc_w();
		int P[9] = { gw, gh, hrc_w(), hrc_h(), rot, local[k], g_hrc_scale, world_w, world_h };
		cf_material_set_uniform_cs(nsx.m_finish, "u_grid_w",   P + 0, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_finish, "u_grid_h",   P + 1, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_finish, "u_world_w",  P + 2, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_finish, "u_world_h",  P + 3, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_finish, "u_rot",      P + 4, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_finish, "u_local_rot",P + 5, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_finish, "u_scale",    P + 6, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_finish, "u_full_w",   P + 7, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_finish, "u_full_h",   P + 8, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_texture_cs(nsx.m_finish, "u_emission", emis_tex);
		cf_material_set_texture_cs(nsx.m_finish, "u_opacity",  opac_tex);
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(world_c, 8), hrc_div_ceil(world_x, 8), 1);
		CF_StorageBuffer ro[1] = { grp[k] }; d.ro_buffers = ro; d.ro_buffer_count = 1;
		CF_Texture rw[3] = { cf_canvas_get_target(nsx.dir[0]), cf_canvas_get_target(nsx.dir[1]),
		                     cf_canvas_get_target(nsx.dir[2]) };
		d.rw_textures = rw; d.rw_texture_count = 3;
		cf_dispatch_compute(nsx.cs_finish, nsx.m_finish, d);
	}

	// prefilter: box min/max absorption per HRC cell (gates the upscale).
	if (g_hrc_scale > 1) {
		int hw = hrc_w(), hh = hrc_h();
		int P[5] = { hw, hh, world_w, world_h, g_hrc_scale };
		cf_material_set_texture_cs(nsx.m_minmax, "u_absorption", opac_tex);
		cf_material_set_uniform_cs(nsx.m_minmax, "u_hrc_w",  P + 0, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_minmax, "u_hrc_h",  P + 1, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_minmax, "u_full_w", P + 2, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_minmax, "u_full_h", P + 3, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_minmax, "u_scale",  P + 4, CF_UNIFORM_TYPE_INT, 1);
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(hw, 16), hrc_div_ceil(hh, 16), 1);
		CF_Texture rw[1] = { cf_canvas_get_target(nsx.minmax) }; d.rw_textures = rw; d.rw_texture_count = 1;
		cf_dispatch_compute(nsx.cs_minmax, nsx.m_minmax, d);
	}

	// display: AgX(radiance * scale) -> fluence, linear -> fluence_lin.
	// g_ns_exposure is the scene's display scale: 1.0 for amitabha-scale emission,
	// ~6 for the cave (whose authored emissivity sits ~6x below amitabha's range).
	{
		const char* ev = getenv("HRC_EXPOSURE");
		float exposure = ev ? (float)atof(ev) : g_ns_exposure;
		float rim_strength = 0.3f, rock_thresh = 5.0f;
		cf_material_set_texture_cs(nsx.m_display, "u_a0", cf_canvas_get_target(nsx.dir[0]));
		cf_material_set_texture_cs(nsx.m_display, "u_a1", cf_canvas_get_target(nsx.dir[1]));
		cf_material_set_texture_cs(nsx.m_display, "u_b1", cf_canvas_get_target(nsx.dir[2]));
		cf_material_set_texture_cs(nsx.m_display, "u_unused", cf_canvas_get_target(nsx.dir[0]));
		cf_material_set_texture_cs(nsx.m_display, "u_absorption", opac_tex);
		cf_material_set_texture_cs(nsx.m_display, "u_minmax", cf_canvas_get_target(nsx.minmax));
		cf_material_set_uniform_cs(nsx.m_display, "u_world_w",  &world_w,  CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_display, "u_world_h",  &world_h,  CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_display, "u_exposure", &exposure, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_uniform_cs(nsx.m_display, "u_rim",           &rim_on,        CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_display, "u_rim_strength",  &rim_strength,  CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_uniform_cs(nsx.m_display, "u_rock_thresh",   &rock_thresh,   CF_UNIFORM_TYPE_FLOAT, 1);
		{ int hw = hrc_w(), hh = hrc_h(); float abs_thresh = GI_ABS_THRESHOLD;
		  cf_material_set_uniform_cs(nsx.m_display, "u_scale", &g_hrc_scale, CF_UNIFORM_TYPE_INT, 1);
		  cf_material_set_uniform_cs(nsx.m_display, "u_hrc_w", &hw, CF_UNIFORM_TYPE_INT, 1);
		  cf_material_set_uniform_cs(nsx.m_display, "u_hrc_h", &hh, CF_UNIFORM_TYPE_INT, 1);
		  cf_material_set_uniform_cs(nsx.m_display, "u_abs_threshold", &abs_thresh, CF_UNIFORM_TYPE_FLOAT, 1); }
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(world_w, 16), hrc_div_ceil(world_h, 16), 1);
		CF_Texture rw[2] = { cf_canvas_get_target(hrc.fluence), cf_canvas_get_target(hrc.fluence_lin) };
		d.rw_textures = rw; d.rw_texture_count = 2;
		cf_dispatch_compute(nsx.cs_display, nsx.m_display, d);
	}
}

// Headless test scenes for the non-square exact HRC: rasterize a scene into
// emissivity/absorption, render via hrc_ns_compute, capture -> cave_shot.raw, exit.
// HRC_SCENE = rectroom | cornell | pinhole  (use with HRC_RES).
// Rasterize the selected scene into emissivity/absorption. Shared by the
// interactive loop and the headless capture so both light the same thing.
const char* scene_name(void)
{
	const char* n = getenv("HRC_SCENE");
	return n ? n : "cornell";
}

// The scene is drawn with the ordinary cf_draw API into two canvases:
// EMISSION (linear light emitted per pixel) and ABSORPTION (extinction
// coefficient per pixel). Every primitive -- boxes, circles, and vector TEXT --
// goes through the same path, so a glowing sign is just text drawn into the
// emission canvas and it lights the room like any other emitter.
//
// channel: 0 = emission, 1 = absorption.
static void scene_shapes(int channel, float W, float H)
{
	const char* name = scene_name();
	int cornell = !CF_STRCMP(name, "cornell");
	int pinhole = !CF_STRCMP(name, "pinhole");
	int circ    = !CF_STRCMP(name, "circ");
	int glass   = !CF_STRCMP(name, "glass");
	int rectroom= !CF_STRCMP(name, "rectroom");
	int emis = (channel == 0);
	const float SOLID = 30000.0f;

	#define WALL(cx, cy, hw, hh) do { 		if (!emis) { cf_draw_push_color(cf_make_color_rgb_f(SOLID, SOLID, SOLID)); 			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2((cx)-(hw), (cy)-(hh)), cf_v2((cx)+(hw), (cy)+(hh))), 0); 			cf_draw_pop_color(); } } while (0)
	#define LIGHT(cx, cy, hw, hh, r, g, b) do { 		cf_draw_push_color(emis ? cf_make_color_rgb_f(r, g, b) : cf_make_color_rgb_f(SOLID, SOLID, SOLID)); 		cf_draw_box_rounded_fill(cf_make_aabb(cf_v2((cx)-(hw), (cy)-(hh)), cf_v2((cx)+(hw), (cy)+(hh))), 0); 		cf_draw_pop_color(); } while (0)
	#define FOG(cx, cy, hw, hh, d) do { 		if (!emis) { cf_draw_push_color(cf_make_color_rgb_f(d, d, (d) * 1.15f)); 			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2((cx)-(hw), (cy)-(hh)), cf_v2((cx)+(hw), (cy)+(hh))), 0); 			cf_draw_pop_color(); } } while (0)

	// Room shell shared by the enclosed scenes.
	if (cornell || glass || rectroom) {
		WALL(0, H*0.5f - 10, W*0.5f, 10);   // ceiling
		WALL(0, -H*0.5f + 10, W*0.5f, 10);  // floor
		WALL(-W*0.5f + 10, 0, 10, H*0.5f);  // left
		WALL(W*0.5f - 10, 0, 10, H*0.5f);   // right
	}

	if (circ) {
		cf_draw_push_color(emis ? cf_make_color_rgb_f(3, 3, 3) : cf_make_color_rgb_f(SOLID, SOLID, SOLID));
		cf_draw_circle_fill(cf_make_circle(cf_v2(0, 0), 8));
		cf_draw_pop_color();
	} else if (rectroom) {
		cf_draw_push_color(emis ? cf_make_color_rgb_f(2.5f, 2.5f, 2.5f) : cf_make_color_rgb_f(SOLID, SOLID, SOLID));
		cf_draw_circle_fill(cf_make_circle(cf_v2(0, 0), 24));
		cf_draw_pop_color();
	} else if (cornell) {
		LIGHT(0, H*0.5f - 26, W*0.09f, 5, 4.0f, 3.7f, 3.2f);
		WALL(-W*0.10f, -H*0.06f, W*0.028f, H*0.16f);
		WALL(W*0.12f, -H*0.14f, W*0.031f, H*0.10f);
	} else if (glass) {
		LIGHT(0, H*0.5f - 26, W*0.13f, 5, 4.0f, 3.7f, 3.2f);
		// see-through volume: finite absorption, so light is attenuated THROUGH it
		FOG(-W*0.12f, -H*0.06f, W*0.070f, H*0.20f, 0.017f);
		// solid circle occluder alongside: crisp umbra for contrast
		if (!emis) {
			cf_draw_push_color(cf_make_color_rgb_f(SOLID, SOLID, SOLID));
			cf_draw_circle_fill(cf_make_circle(cf_v2(W*0.14f, -H*0.06f), H*0.135f));
			cf_draw_pop_color();
		}
	} else if (pinhole) {
		float gap = H * 0.022f;
		WALL(0, H*0.25f + gap*0.5f, 3, H*0.25f - gap*0.5f);
		WALL(0, -H*0.25f - gap*0.5f, 3, H*0.25f - gap*0.5f);
		float cols[5][3] = { {4.0f,0.8f,0.3f},{0.3f,1.4f,3.2f},{0.5f,3.0f,0.4f},{2.6f,0.5f,3.4f},{3.2f,1.9f,0.3f} };
		for (int i = 0; i < 5; i++) {
			float y = (float)(i - 2) * (H * 0.14f);
			LIGHT(-W*0.47f + 30, y, 4, 16, cols[i][0], cols[i][1], cols[i][2]);
		}
	} else { // showcase: wide room, coloured walls, haze, glowing volume
		WALL(0, H*0.5f - 14, W*0.5f, 14);
		WALL(0, -H*0.5f + 14, W*0.5f, 14);
		WALL(-W*0.5f + 14, 0, 14, H*0.5f);
		WALL(W*0.5f - 14, 0, 14, H*0.5f);
		FOG(0, 0, W*0.5f - 14, H*0.5f - 14, 0.0030f);
		LIGHT(-W*0.47f + 31, 0, 7, H*0.42f, 6.0f, 0.40f, 0.12f);
		LIGHT(W*0.47f - 31, 0, 7, H*0.42f, 0.14f, 1.1f, 6.0f);
		LIGHT(0, H*0.5f - 34, W*0.09f, 5, 3.2f, 3.0f, 2.6f);
		WALL(-W*0.16f, H*0.04f, W*0.030f, H*0.16f);
		cf_draw_push_color(emis ? cf_make_color_rgb_f(1.3f, 0.5f, 2.0f) : cf_make_color_rgb_f(0.020f, 0.016f, 0.024f));
		cf_draw_circle_fill(cf_make_circle(cf_v2(W*0.13f, -H*0.16f), H*0.13f));
		cf_draw_pop_color();
	}

	#undef WALL
	#undef LIGHT
	#undef FOG
}

// Rasterize the selected scene into emission + absorption. Shared by the
// interactive loop and the headless capture so both light the same thing.
void hrc_ns_scene_draw(void)
{
	float W = (float)world_w, H = (float)world_h;
	cf_app_update(NULL); // acquire a command buffer
	hrc_ns_init_once();
	g_ns_exposure = 1.0f;

	for (int channel = 0; channel < 2; channel++) {
		cf_draw_push();
		cf_draw_projection(cf_ortho_2d(0, 0, W, H));
		push_f16_render_state();
		scene_shapes(channel, W, H);
		cf_draw_pop_render_state();

		cf_render_to(channel == 0 ? hrc.emissivity : hrc.absorption, true);
		cf_draw_pop();
	}
}

// Headless capture: render the scene, settle, dump display + linear, exit.
void hrc_ns_testscene(void)
{
	hrc_ns_scene_draw();

	// Settle over several frames before reading back. A single frame is not safe:
	// the readback can report ready before the GPU has finished the dispatches, and
	// we capture zeros (reproducible on back-to-back runs). Re-running the compute
	// each frame is idempotent here -- the scene textures don't change.
	for (int f = 0; f < 6; f++) {
		hrc_ns_scene_draw();   // redraw: the glyph atlas may not be built on frame 0
		hrc_ns_compute();
		cf_app_draw_onto_screen(true);
		cf_app_update(NULL);
	}
	hrc_ns_compute();
	cf_app_draw_onto_screen(true);

	CF_Readback rb = cf_canvas_readback(hrc.fluence);
	CF_Readback rbl = cf_canvas_readback(hrc.fluence_lin);
	for (int t = 0; t < 240 && (!cf_readback_ready(rb) || !cf_readback_ready(rbl)); t++) { cf_app_update(NULL); cf_app_draw_onto_screen(true); }
	if (cf_readback_ready(rb)) {
		int sz = cf_readback_size(rb);
		unsigned char* buf = (unsigned char*)cf_alloc(sz);
		cf_readback_data(rb, buf, sz);
		FILE* f = fopen("C:/randy/hrc_shot.raw", "wb"); fwrite(buf, 1, sz, f); fclose(f); cf_free(buf);
		printf("SCENE %s: %dx%d rgba8 -> C:/randy/hrc_shot.raw\n", getenv("HRC_SCENE"), world_w, world_h); fflush(stdout);
	}
	if (cf_readback_ready(rbl)) {
		int sz = cf_readback_size(rbl), N = world_w * world_h;
		unsigned char* buf = (unsigned char*)cf_alloc(sz); cf_readback_data(rbl, buf, sz);
		uint16_t* h = (uint16_t*)buf;
		FILE* f = fopen("C:/randy/hrc_lin.raw", "wb");
		for (int i = 0; i < N; i++) { float rgb[3] = { half_to_float(h[i*4+0]), half_to_float(h[i*4+1]), half_to_float(h[i*4+2]) }; fwrite(rgb, 4, 3, f); }
		fclose(f); cf_free(buf);
		printf("SCENE lin -> C:/randy/hrc_lin.raw\n"); fflush(stdout);
	}
	exit(0);
}


int main(int argc, char* argv[])
{
	// Window / world resolution. HRC_RES=WxH picks a non-square resolution
	// (rounded to a multiple of 2*GI_UPSCALE so the probe lattice stays integral).
	world_w = GI_REF;
	world_h = GI_REF;
	{
		const char* res = getenv("HRC_RES");
		if (res) {
			int rw = 0, rh = 0;
			if (sscanf(res, "%dx%d", &rw, &rh) == 2 && rw >= 256 && rh >= 256 && rw <= 4096 && rh <= 4096) {
				int q = 2 * GI_UPSCALE;
				world_w = (rw / q) * q;
				world_h = (rh / q) * q;
			}
		}
	}
	grid_w = world_w / GI_UPSCALE;
	grid_h = world_h / GI_UPSCALE;
	scene_s = (world_w < world_h ? world_w : world_h) / (float)GI_REF;
	scene_ox = (world_w - GI_REF * scene_s) * 0.5f;
	scene_oy = (world_h - GI_REF * scene_s) * 0.5f;

	cf_make_app("HRC GI", 0, 0, 0, world_w, world_h, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	cf_clear_color(0, 0, 0, 1);

	{ const char* hs = getenv("HRC_SCALE"); if (hs) { int v = atoi(hs);
		if (v == 1 || v == 2 || v == 4) g_hrc_scale = v; } }

	hrc_init();
	cf_make_font("/hrc_gi_data/calibri.ttf", cf_sintern("gi"));

	// Headless: render one scene, dump it, exit. This is what regress.py drives.
	if (getenv("HRC_SCENE")) hrc_ns_testscene();

	// Interactive: draw the selected scene and light it every frame.
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		hrc_ns_scene_draw();
		hrc_ns_compute();
		cf_draw_canvas(hrc.fluence, cf_v2(0, 0), cf_v2((float)world_w, (float)world_h));

		// Scene title + controls, drawn like any other shape.
		cf_push_font(cf_sintern("gi"));
		cf_push_font(cf_sintern("gi"));
		cf_push_font_size(world_h * 0.045f);
		cf_draw_push_color(cf_make_color_rgb_f(1.0f, 0.93f, 0.78f));
		cf_draw_text(scene_name(), cf_v2(-world_w * 0.46f, world_h * 0.42f), -1);
		cf_draw_pop_color();
		cf_push_font_size(world_h * 0.022f);
		cf_draw_push_color(cf_make_color_rgb_f(0.62f, 0.72f, 0.80f));
		cf_draw_text("HRC 2D global illumination", cf_v2(-world_w * 0.46f, world_h * 0.375f), -1);
		cf_draw_text("HRC_SCENE=cornell|glass|pinhole|rectroom|circ|showcase    HRC_SCALE=1|2|4",
			cf_v2(-world_w * 0.46f, -world_h * 0.45f), -1);
		cf_draw_pop_color();

		cf_app_draw_onto_screen(true);
	}

	hrc_shutdown();
	cf_destroy_app();
	return 0;
}
