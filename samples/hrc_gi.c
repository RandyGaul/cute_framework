// HRC 2D global illumination -- a compute-driven radiance-cascade GI renderer,
// validated bit-exact against amitabha (Sannikov's reference implementation).
//
// The scene is authored as canvases holding physical units: linear EMISSION, an
// ABSORPTION coefficient, and a DIFFUSE albedo, per world pixel. Absorption is
// continuous, so a surface is just a cell with huge absorption and fog is a cell
// with a little -- the same code path gives hard shadows and volumetric
// transmission. Albedo feeds a multibounce pass so a surface reflects the light
// landing on it instead of reading as a black cutout.
//
// Pipeline: direct-trace the low cascade levels, merge_up the rest, merge down,
// then scatter-reconstruct per pixel. Output is directional (2D Fourier L1), so
// surfaces can be shaded from a normal rather than just a scalar fluence.
//
// Scenes, cycled with the LEFT/RIGHT arrow keys:
//   cornell   diffuse bounce and soft shadows in a closed box
//   glass     a translucent volume beside a solid occluder
//   pinhole   five coloured sources beaming through one gap
//   rectroom  a small emitter in a sealed room
//   circ      a single emitter in open space
//   showcase  coloured walls, haze, and a glowing volume
//   text      type as the only geometry: emissive headlines light a hazy void
//             while a non-emissive line silhouettes and casts shadows
//
// Env vars:
//   HRC_SCENE=name      scene to start on (default cornell)
//   HRC_RES=WxH         window / world resolution (default 1024x1024)
//   HRC_SCALE=1|2|4     run the probe lattice at 1/N res and upscale
//   HRC_EXPOSURE=f      display exposure bias
//   HRC_SAT=f           post-tonemap chroma (1.0 = untouched)
//   HRC_SHOT=1          headless: render the starting scene, dump it, exit
//
// The text scene exposes its own tuning knobs: HRC_TXT_E (emission), _OABS/_EABS
// (absorption), _OALB (albedo), _BIG/_SML (type size), _TRACK (letter-spacing),
// _FOG (haze), and HRC_OVL_E/_O/_TONE/_WHITE for the composited stamp.

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



// Runtime toggles. Only the ones that actually drive something live here; the
// cave sample this grew out of also declared gi/fog/water/overlay flags that were
// never wired to anything, so they are gone rather than sitting on a key that
// does nothing.
int bounce_on = 1;  // multibounce feedback pass
float smoothed_fps = 60.0f;

// Wall-clock for animated scenes. Advanced by the interactive loop only, so a
// headless capture always renders the same frame and stays reproducible.
float g_scene_time = 0.0f;

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
	CF_ComputeShader cs_trace, cs_extend, cs_merge, cs_finish, cs_display, cs_minmax, cs_feedback;
	CF_Material m_trace, m_extend, m_merge, m_finish, m_display, m_minmax, m_feedback;
} nsx;

const char* scene_name(void);

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

// Cascade extents for the CURRENT scale. Storage is allocated once for scale 1 --
// the largest configuration -- so changing scale only changes how much of it is
// used. Reallocating instead would mean destroying buffers the GPU may still be
// reading from, which is not worth it for a display toggle.
static void hrc_ns_apply_scale(void)
{
	int gw = hrc_w() / 2, gh = hrc_h() / 2;
	nsx.maxd = gw > gh ? gw : gh;
	nsx.nlev = hrc_log2_ceil(nsx.maxd) + 1;   // levels 0..nlev-1
}

static void hrc_ns_init_once(void)
{
	if (nsx.inited) { hrc_ns_apply_scale(); return; }
	// Allocate at scale 1 regardless of the starting scale, so the keys never need
	// to resize anything. MAXD/NLEV here are capacities, not the working extents.
	int gw = world_w / 2, gh = world_h / 2;
	int MAXD = gw > gh ? gw : gh;
	int NLEV = hrc_log2_ceil(MAXD) + 1;
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
		nsx.dir[i] = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	nsx.cs_trace   = load_compute_shader("/hrc_gi_data/ami_ns_trace.c_shd");
	nsx.cs_extend  = load_compute_shader("/hrc_gi_data/ami_ns_extend.c_shd");
	nsx.cs_merge   = load_compute_shader("/hrc_gi_data/ami_ns_merge.c_shd");
	nsx.cs_finish  = load_compute_shader("/hrc_gi_data/ami_ns_finish.c_shd");
	nsx.cs_display = load_compute_shader("/hrc_gi_data/ami_display.c_shd");
	nsx.cs_minmax  = load_compute_shader("/hrc_gi_data/ami_minmax.c_shd");
	nsx.cs_feedback= load_compute_shader("/hrc_gi_data/ami_feedback.c_shd");
	nsx.minmax = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16_FLOAT, CF_FILTER_NEAREST);
	nsx.m_trace = cf_make_material(); nsx.m_extend = cf_make_material(); nsx.m_merge = cf_make_material();
	nsx.m_finish = cf_make_material(); nsx.m_display = cf_make_material();
	nsx.m_minmax = cf_make_material();
	nsx.m_feedback = cf_make_material();
	nsx.inited = 1;
	hrc_ns_apply_scale();
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

	// Multibounce: fold last frame's fluence, tinted by albedo, back into emission
	// before tracing. Skipping it leaves absorbing surfaces unlit, which is what the
	// B key demonstrates. This is what lights the visible FACE of an opaque surface --
	// the cascade only reports light ARRIVING at a cell, so a pure absorber has
	// nothing to show and reads as a black cutout. Runs after the scene draw (which
	// rewrote emissivity) and before the cascade, so each frame adds one bounce.
	if (bounce_on) {
		int gather_r = 10;
		float surface_thresh = 0.5f; // above fog (~0.003), below any authored surface
		cf_material_set_uniform_cs(nsx.m_feedback, "u_world_w", &world_w, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_feedback, "u_world_h", &world_h, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_feedback, "u_gather_r", &gather_r, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_feedback, "u_surface_thresh", &surface_thresh, CF_UNIFORM_TYPE_FLOAT, 1);
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(world_w, 16), hrc_div_ceil(world_h, 16), 1);
		CF_Texture ro[3] = { cf_canvas_get_target(hrc.fluence_lin), cf_canvas_get_target(hrc.diffuse), opac_tex };
		d.ro_textures = ro; d.ro_texture_count = 3;
		CF_Texture rw[1] = { emis_tex };
		d.rw_textures = rw; d.rw_texture_count = 1;
		cf_dispatch_compute(nsx.cs_feedback, nsx.m_feedback, d);
	}

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
		cf_material_set_texture_cs(nsx.m_display, "u_a0", cf_canvas_get_target(nsx.dir[0]));
		cf_material_set_texture_cs(nsx.m_display, "u_a1", cf_canvas_get_target(nsx.dir[1]));
		cf_material_set_texture_cs(nsx.m_display, "u_b1", cf_canvas_get_target(nsx.dir[2]));
		cf_material_set_texture_cs(nsx.m_display, "u_emission", emis_tex);
		cf_material_set_texture_cs(nsx.m_display, "u_minmax", cf_canvas_get_target(nsx.minmax));
		cf_material_set_uniform_cs(nsx.m_display, "u_world_w",  &world_w,  CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_display, "u_world_h",  &world_h,  CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(nsx.m_display, "u_exposure", &exposure, CF_UNIFORM_TYPE_FLOAT, 1);
		{ int hw = hrc_w(), hh = hrc_h(); float abs_thresh = GI_ABS_THRESHOLD;
		  cf_material_set_uniform_cs(nsx.m_display, "u_scale", &g_hrc_scale, CF_UNIFORM_TYPE_INT, 1);
		  cf_material_set_uniform_cs(nsx.m_display, "u_hrc_w", &hw, CF_UNIFORM_TYPE_INT, 1);
		  cf_material_set_uniform_cs(nsx.m_display, "u_hrc_h", &hh, CF_UNIFORM_TYPE_INT, 1);
		  cf_material_set_uniform_cs(nsx.m_display, "u_abs_threshold", &abs_thresh, CF_UNIFORM_TYPE_FLOAT, 1); }
		{ const char* e = getenv("HRC_EMISSIVE");
		  float emissive = e ? (float)atof(e) : 1.0f;
		  cf_material_set_uniform_cs(nsx.m_display, "u_emissive", &emissive, CF_UNIFORM_TYPE_FLOAT, 1);
		  // Only the type scene asks for extra chroma; everything else stays at 1.0.
		  const char* sv = getenv("HRC_SAT");
		  float sat = sv ? (float)atof(sv) : (!CF_STRCMP(scene_name(), "text") ? 1.20f : 1.0f);
		  cf_material_set_uniform_cs(nsx.m_display, "u_saturation", &sat, CF_UNIFORM_TYPE_FLOAT, 1); }
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
// Every scene the sample can show, in cycle order. HRC_SCENE picks the starting
// one; the arrow keys move through them at runtime.
static const char* g_scene_list[] = {
	"cornell", "glass", "pinhole", "rectroom", "circ", "showcase", "orbit", "text",
};
static const int g_scene_count = (int)(sizeof(g_scene_list) / sizeof(g_scene_list[0]));
static int g_scene_index = 0;

const char* scene_name(void)
{
	return g_scene_list[g_scene_index];
}

// Point the sample at a scene by name. Unknown names leave the selection alone so
// a typo in HRC_SCENE shows the default rather than crashing.
static void scene_select(const char* name)
{
	if (!name) return;
	for (int i = 0; i < g_scene_count; i++) {
		if (!CF_STRCMP(name, g_scene_list[i])) { g_scene_index = i; return; }
	}
	printf("unknown HRC_SCENE '%s' -- showing %s\n", name, scene_name());
	fflush(stdout);
}

static void scene_cycle(int delta)
{
	g_scene_index = (g_scene_index + delta + g_scene_count) % g_scene_count;
}

// The scene is drawn with the ordinary cf_draw API into two canvases:
// EMISSION (linear light emitted per pixel) and ABSORPTION (extinction
// coefficient per pixel). Every primitive -- boxes, circles, and vector TEXT --
// goes through the same path, so a glowing sign is just text drawn into the
// emission canvas and it lights the room like any other emitter.
//
// channel: 0 = emission, 1 = absorption, 2 = diffuse albedo (feeds the bounce
// pass), 3 = the translucent stamp composited over the finished image.
// Alpha of the translucent overlay stamp (channel 3), per line kind.
static float g_overlay_emis = 0.45f;
static float g_overlay_occ  = 0.72f;
static float g_overlay_tone  = 0.70f;  // value of the opaque stamp
static float g_overlay_white = 0.20f;  // how far the emissive stamp pulls to white

// One centred line of the "text" scene, authored into whichever channel is being
// drawn. Emissive lines carry emission plus just enough absorption to couple it;
// occluding lines carry no emission, absorption dense enough to seal the stroke,
// and an albedo so the bounce pass lights their visible face.
static void text_line(int channel, const char* str, float y, float size,
                      float er, float eg, float eb, float absorb, float albedo, float track)
{
	cf_push_font_size(size);
	CF_V2 sz = cf_text_size(str, -1);
	// Untracked lines fit by shrinking the type: a long headline scales down rather
	// than running off the frame. Tracked lines fit further down by closing up their
	// letter-spacing instead, which preserves the size.
	float fit_w = (float)world_w * 0.92f;
	if (track <= 0 && sz.x > fit_w) {
		cf_pop_font_size();
		cf_push_font_size(size * (fit_w / sz.x));
		sz = cf_text_size(str, -1);
	}
	CF_Color c;
	if (channel == 0)      c = cf_make_color_rgb_f(er, eg, eb);
	else if (channel == 1) c = cf_make_color_rgb_f(absorb, absorb, absorb * 1.06f);
	else if (channel == 2) c = cf_make_color_rgb_f(albedo, albedo, albedo * 1.03f);
	else {
		// Channel 3: the translucent stamp composited over the finished HRC image.
		// The volumetric pass below already produced the glow and the cast shadows;
		// this only puts a crisp, flat letterform back on top so the type reads.
		// Emissive lines stamp their own hue, occluding lines stamp dark.
		if (er + eg + eb > 0) {
			// A hot emitter's core reads near-white, with the hue carried by the glow
			// around it rather than the letterform itself. Stamping the raw normalized
			// emission colour oversaturates, so pull it most of the way to white.
			float m = er > eg ? (er > eb ? er : eb) : (eg > eb ? eg : eb);
			float k = g_overlay_white;
			c = cf_make_color_rgba_f(er / m * (1 - k) + k, eg / m * (1 - k) + k,
			                         eb / m * (1 - k) + k, g_overlay_emis);
		} else {
			// Neutral and faintly warm. This line is a lit surface, not an emitter, so
			// a hued stamp would read as though it were glowing; its contrast against
			// the wash comes from value instead of colour.
			float t = g_overlay_tone;
			c = cf_make_color_rgba_f(t * 1.04f, t * 1.00f, t * 0.97f, g_overlay_occ);
		}
	}
	cf_draw_push_color(c);
	float baseline = y + sz.y * 0.5f;
	if (track <= 0) {
		// No tracking: one call, so the font's own kerning applies.
		cf_draw_text(str, cf_v2(-sz.x * 0.5f, baseline), -1);
	} else {
		// Letter-spacing, which cf_draw has no API for: step glyph by glyph and add
		// the extra advance by hand. Measure the tracked width first so the line
		// still centres. ASCII only, which is all these captions are.
		float glyphs = 0;
		int n = 0;
		for (const char* p = str; *p; ++p) { char ch[2] = { *p, 0 }; glyphs += cf_text_width(ch, -1); n++; }
		// Auto-fit: tracking that would push the line past the frame gets pulled back,
		// so a longer string just tracks tighter instead of running off the edge.
		float maxw = (float)world_w * 0.92f;
		if (n > 1 && glyphs + track * (n - 1) > maxw) {
			track = (maxw - glyphs) / (float)(n - 1);
			if (track < 0) track = 0;
		}
		float total = glyphs + track * (n > 0 ? n - 1 : 0);
		float x = -total * 0.5f;
		for (const char* p = str; *p; ++p) {
			char ch[2] = { *p, 0 };
			cf_draw_text(ch, cf_v2(x, baseline), -1);
			x += cf_text_width(ch, -1) + track;
		}
	}
	cf_draw_pop_color();
	cf_pop_font_size();
}

static void scene_shapes(int channel, float W, float H)
{
	const char* name = scene_name();
	int cornell = !CF_STRCMP(name, "cornell");
	int pinhole = !CF_STRCMP(name, "pinhole");
	int circ    = !CF_STRCMP(name, "circ");
	int glass   = !CF_STRCMP(name, "glass");
	int rectroom= !CF_STRCMP(name, "rectroom");
	int orbit   = !CF_STRCMP(name, "orbit");
	int emis = (channel == 0);
	const float SOLID = 30000.0f;
	// Absorption for anything that EMITS. Never SOLID: at infinite absorption a light
	// occludes itself, so only the cells on its surface can radiate outward and the
	// emission picks up the shape of that rasterized boundary -- brighter along
	// whichever side the march reaches first, and spoked on small round sources. A
	// finite coefficient still reads as a solid bright object but lets the whole
	// volume emit, which is both symmetric and what a real lamp does.
	const float EMIT_ABS = 0.5f;

	// "text": type is very nearly the whole scene, after entropylost/amida's
	// "thisisradiancecascades". Two emissive headlines light a hazy room and one
	// non-emissive line silhouettes against that wash while casting long shadows.
	// The room is a plain bordered box whose walls carry a diffuse albedo, so the
	// neon washes back off them -- pink pooling at the top, mint below. Press B to
	// drop the bounce and the borders go black.
	if (!CF_STRCMP(name, "text")) {
		float EM  = getenv("HRC_TXT_E")   ? (float)atof(getenv("HRC_TXT_E"))   : 3.2f;
		float OA  = getenv("HRC_TXT_OABS")? (float)atof(getenv("HRC_TXT_OABS")): 0.25f;
		float OB  = getenv("HRC_TXT_OALB")? (float)atof(getenv("HRC_TXT_OALB")): 0.85f;
		float EA  = getenv("HRC_TXT_EABS")? (float)atof(getenv("HRC_TXT_EABS")): 0.20f;
		float BIG = getenv("HRC_TXT_BIG") ? (float)atof(getenv("HRC_TXT_BIG")) : 0.190f;
		float SML = getenv("HRC_TXT_SML") ? (float)atof(getenv("HRC_TXT_SML")) : 0.125f;
		float TRK = getenv("HRC_TXT_TRACK")?(float)atof(getenv("HRC_TXT_TRACK")):0.22f;
		// A little haze over the whole frame. Without a participating medium the
		// coloured light has nothing to scatter off and the field reads as washed-out
		// grey -- it is fluence through vacuum. Fog gives the light something to
		// stain, which is what makes the pink and mint actually fill the frame.
		// Drawn first: brushes overwrite, so any solid laid down before it would be
		// erased back to fog.
		if (channel == 1) {
			float fog = getenv("HRC_TXT_FOG") ? (float)atof(getenv("HRC_TXT_FOG")) : 0.005f;
			cf_draw_push_color(cf_make_color_rgb_f(fog, fog, fog * 1.15f));
			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W * 0.5f, -H * 0.5f), cf_v2(W * 0.5f, H * 0.5f)), 0);
			cf_draw_pop_color();
		}
		// Borders. Solid to light transport, with an albedo so the bounce pass lights
		// their faces: without something to catch it the neon just falls off into an
		// empty void and there is no second bounce to see.
		{
			// Thin on purpose: the bounce pass lights a surface from a small
			// neighbourhood gather, so a thick wall only ever lights its inner few
			// pixels and the rest stays black. At this thickness the whole border
			// catches light and reads as a lit frame.
			float t = H * 0.009f;
			float a = getenv("HRC_TXT_WALL") ? (float)atof(getenv("HRC_TXT_WALL")) : 0.90f;
			CF_Color c = (channel == 1) ? cf_make_color_rgb_f(SOLID, SOLID, SOLID)
			                            : cf_make_color_rgb_f(a, a, a);
			if (channel == 1 || channel == 2) {
				cf_draw_push_color(c);
				cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, H*0.5f - t), cf_v2(W*0.5f, H*0.5f)), 0);
				cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, -H*0.5f), cf_v2(W*0.5f, -H*0.5f + t)), 0);
				cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, -H*0.5f), cf_v2(-W*0.5f + t, H*0.5f)), 0);
				cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(W*0.5f - t, -H*0.5f), cf_v2(W*0.5f, H*0.5f)), 0);
				cf_draw_pop_color();
			}
		}
		// Zero emission, dense enough to seal the stroke, and a plain [0,1] albedo so
		// the bounce pass lights its face. Tracked out wide so it spans the frame.
		text_line(channel, "lookin cute ^_^", -H * 0.265f, H * SML, 0, 0, 0, OA, OB, H * SML * TRK);
		// Cute Framework's own palette, normalized to hue and scaled by emission:
		// pink #fea3a8 and mint #96dec6 off docs/stylesheets/cute.css and the logo.
		text_line(channel, "CUTE",       H * 0.150f, H * BIG, EM * 1.000f, EM * 0.520f, EM * 0.550f, EA, 0, 0);
		text_line(channel, "FRAMEWORK", -H * 0.055f, H * BIG, EM * 0.500f, EM * 1.000f, EM * 0.840f, EA, 0, 0);
		return;
	}

	// Channel 3 (the overlay stamp) is the type scene's alone. Channel 2 is diffuse
	// albedo, which only orbit authors among the room scenes.
	if (channel == 3) return;
	if (channel == 2 && !orbit) return;

	#define WALL(cx, cy, hw, hh) do { 		if (channel == 1) { cf_draw_push_color(cf_make_color_rgb_f(SOLID, SOLID, SOLID)); 			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2((cx)-(hw), (cy)-(hh)), cf_v2((cx)+(hw), (cy)+(hh))), 0); 			cf_draw_pop_color(); } } while (0)
	#define LIGHT(cx, cy, hw, hh, r, g, b) do { 		if (channel > 1) break; 		cf_draw_push_color(emis ? cf_make_color_rgb_f(r, g, b) : cf_make_color_rgb_f(EMIT_ABS, EMIT_ABS, EMIT_ABS)); 		cf_draw_box_rounded_fill(cf_make_aabb(cf_v2((cx)-(hw), (cy)-(hh)), cf_v2((cx)+(hw), (cy)+(hh))), 0); 		cf_draw_pop_color(); } while (0)
	#define FOG(cx, cy, hw, hh, d) do { 		if (channel == 1) { cf_draw_push_color(cf_make_color_rgb_f(d, d, (d) * 1.15f)); 			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2((cx)-(hw), (cy)-(hh)), cf_v2((cx)+(hw), (cy)+(hh))), 0); 			cf_draw_pop_color(); } } while (0)

	// Room shell shared by the enclosed scenes.
	if (cornell || glass || rectroom) {
		WALL(0, H*0.5f - 10, W*0.5f, 10);   // ceiling
		WALL(0, -H*0.5f + 10, W*0.5f, 10);  // floor
		WALL(-W*0.5f + 10, 0, 10, H*0.5f);  // left
		WALL(W*0.5f - 10, 0, 10, H*0.5f);   // right
	}

	if (circ) {
		// Emitters must not be SOLID. At infinite absorption only the disc's rasterized
		// rim can emit outward -- its own body blocks the rest -- and at this radius
		// that rim is a coarse pixel staircase whose extent differs along the axes and
		// the diagonals. The result is a +-45 degree modulation of the emitted light: a
		// hard X across the frame. A finite coefficient lets the whole volume emit, and
		// the source reads as the round, radially symmetric disc it is meant to be.
		cf_draw_push_color(emis ? cf_make_color_rgb_f(3, 3, 3) : cf_make_color_rgb_f(EMIT_ABS, EMIT_ABS, EMIT_ABS));
		cf_draw_circle_fill(cf_make_circle(cf_v2(0, 0), 8));
		cf_draw_pop_color();
	} else if (rectroom) {
		// Finite absorption, same reason as circ: a SOLID emitter only radiates from
		// its staircased rim and throws +-45 degree spokes.
		cf_draw_push_color(emis ? cf_make_color_rgb_f(2.5f, 2.5f, 2.5f) : cf_make_color_rgb_f(EMIT_ABS, EMIT_ABS, EMIT_ABS));
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
	} else if (orbit) {
		// A small light on a circular path around a cluster of pillars, so the shadows
		// sweep as it travels. This is also the scene where the bounce pass earns its
		// keep: the walls carry a diffuse albedo, so their colour washes back into the
		// room. Press B to see the difference -- without it the walls are black and
		// only directly-lit surfaces show at all.
		WALL(0, H*0.5f - 12, W*0.5f, 12);
		WALL(0, -H*0.5f + 12, W*0.5f, 12);
		WALL(-W*0.5f + 12, 0, 12, H*0.5f);
		WALL(W*0.5f - 12, 0, 12, H*0.5f);
		if (channel == 2) {
			// Tinted walls: warm left, cool right, neutral floor/ceiling. Kept well
			// under 1 -- the bounce series converges to 1/(1-albedo), so values near
			// unity pile up over the settle frames and wash the room out.
			cf_draw_push_color(cf_make_color_rgb_f(0.90f, 0.16f, 0.10f));
			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, -H*0.5f), cf_v2(-W*0.5f + 26, H*0.5f)), 0);
			cf_draw_pop_color();
			cf_draw_push_color(cf_make_color_rgb_f(0.10f, 0.24f, 0.92f));
			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(W*0.5f - 26, -H*0.5f), cf_v2(W*0.5f, H*0.5f)), 0);
			cf_draw_pop_color();
			cf_draw_push_color(cf_make_color_rgb_f(0.80f, 0.80f, 0.78f));
			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, H*0.5f - 26), cf_v2(W*0.5f, H*0.5f)), 0);
			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, -H*0.5f), cf_v2(W*0.5f, -H*0.5f + 26)), 0);
			cf_draw_pop_color();
		}
		// Pillars sit INSIDE the orbit radius so the light never travels through them
		// -- an emitter buried in an occluder would just switch itself off.
		// A single small block near the middle: enough to throw a shadow that sweeps
		// as the light orbits, without carving the room into black wedges that hide
		// the bounced fill this scene exists to show.
		WALL(0, 0, W*0.045f, H*0.045f);
		// The travelling light: a clean circular orbit clear of both the pillars and
		// the walls. Radius 10 keeps it above the paper's minimum source size, and
		// EMIT_ABS keeps it from occluding itself.
		{
			float t = g_scene_time * 0.5f;
			float ox = cosf(t) * W * 0.30f;
			float oy = sinf(t) * H * 0.30f;
			cf_draw_push_color(emis ? cf_make_color_rgb_f(9.0f, 7.6f, 5.6f)
			                        : cf_make_color_rgb_f(EMIT_ABS, EMIT_ABS, EMIT_ABS));
			cf_draw_circle_fill(cf_make_circle(cf_v2(ox, oy), 10));
			cf_draw_pop_color();
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
	// The text scene is lit entirely by two large emitters filling the frame, so it
	// sits far higher on the exposure curve than the room scenes. Pulling it down
	// keeps the letter cores from clipping to white and lets the wash hold its hue
	// and fall off into dark corners.
	// Per-scene display scale. The bounce-lit room needs pulling down as well: a
	// closed box with reflective walls converges to roughly 1/(1-albedo) times the
	// direct light, which clips at the room scenes' default exposure.
	g_ns_exposure = 1.0f;
	if (!CF_STRCMP(scene_name(), "text"))  g_ns_exposure = 0.10f;
	if (!CF_STRCMP(scene_name(), "orbit")) g_ns_exposure = 2.0f;

	// 0 = emission, 1 = absorption, 2 = diffuse albedo (drives the bounce feedback).
	for (int channel = 0; channel < 3; channel++) {
		cf_draw_push();
		cf_draw_projection(cf_ortho_2d(0, 0, W, H));
		push_f16_render_state();
		scene_shapes(channel, W, H);
		cf_draw_pop_render_state();

		cf_render_to(channel == 0 ? hrc.emissivity : channel == 1 ? hrc.absorption : hrc.diffuse, true);
		cf_draw_pop();
	}
}

// Composite the type back over the finished HRC image with the ordinary draw
// path: default shader, default blend, partial alpha. The cascade pass already
// did the lighting -- glow, colour bleed, cast shadows -- but reconstructs it
// through a light field, which is low-frequency by nature and leaves thin
// letterforms soft. Stamping a flat translucent copy on top restores the crisp
// edge while the volumetric result still shows through it.
static void hrc_text_overlay(void)
{
	if (CF_STRCMP(scene_name(), "text")) return; // only the type scene stamps
	{ const char* e = getenv("HRC_OVL_E"); if (e) g_overlay_emis = (float)atof(e); }
	{ const char* o = getenv("HRC_OVL_O"); if (o) g_overlay_occ  = (float)atof(o); }
	{ const char* t = getenv("HRC_OVL_TONE"); if (t) g_overlay_tone = (float)atof(t); }
	{ const char* w = getenv("HRC_OVL_WHITE"); if (w) g_overlay_white = (float)atof(w); }
	if (g_overlay_emis <= 0 && g_overlay_occ <= 0) return;

	cf_draw_push();
	cf_draw_projection(cf_ortho_2d(0, 0, (float)world_w, (float)world_h));
	scene_shapes(3, (float)world_w, (float)world_h); // same layout, overlay colours
	cf_render_to(hrc.fluence, false);                // composite, do not clear
	cf_draw_pop();
}

// Headless capture: render the scene, settle, dump display + linear, exit.
void hrc_ns_testscene(void)
{
	// Settle over several frames before reading back. A single frame is not safe:
	// the readback can report ready before the GPU has finished the dispatches, and
	// we capture zeros (reproducible on back-to-back runs). Re-running the compute
	// each frame is idempotent here -- the scene textures don't change.
	//
	// hrc_ns_scene_draw opens a frame (cf_app_update), so each pass pairs it with one
	// cf_app_draw_onto_screen to submit. Leaving a frame open no longer loses its GPU
	// work, but pairing keeps the frame accounting obvious.
	for (int f = 0; f < 6; f++) {
		hrc_ns_scene_draw();   // opens the frame (cf_app_update)
		hrc_ns_compute();
		hrc_text_overlay();    // translucent stamp over the finished image
		cf_app_draw_onto_screen(true);   // submits it
	}

	CF_Readback rb = cf_canvas_readback(hrc.fluence);
	CF_Readback rbl = cf_canvas_readback(hrc.fluence_lin);
	for (int t = 0; t < 240 && (!cf_readback_ready(rb) || !cf_readback_ready(rbl)); t++) { cf_app_update(NULL); cf_app_draw_onto_screen(true); }
	// Dumps land in the working directory. HRC_OUT can point them elsewhere; it is
	// used as a prefix, so a trailing slash gives a directory and anything else
	// prefixes the filename.
	const char* out = getenv("HRC_OUT");
	char path[512];
	if (!out) out = "";
	if (cf_readback_ready(rb)) {
		int sz = cf_readback_size(rb);
		unsigned char* buf = (unsigned char*)cf_alloc(sz);
		cf_readback_data(rb, buf, sz);
		CF_SNPRINTF(path, sizeof(path), "%shrc_shot.raw", out);
		FILE* f = fopen(path, "wb");
		if (f) { fwrite(buf, 1, sz, f); fclose(f); printf("SCENE %s: %dx%d rgba8 -> %s\n", scene_name(), world_w, world_h, path); }
		else    { printf("SCENE %s: failed to open %s for writing\n", scene_name(), path); }
		fflush(stdout);
		cf_free(buf);
	}
	if (cf_readback_ready(rbl)) {
		int sz = cf_readback_size(rbl), N = world_w * world_h;
		unsigned char* buf = (unsigned char*)cf_alloc(sz); cf_readback_data(rbl, buf, sz);
		uint16_t* h = (uint16_t*)buf;
		CF_SNPRINTF(path, sizeof(path), "%shrc_lin.raw", out);
		FILE* f = fopen(path, "wb");
		if (f) {
			for (int i = 0; i < N; i++) { float rgb[3] = { half_to_float(h[i*4+0]), half_to_float(h[i*4+1]), half_to_float(h[i*4+2]) }; fwrite(rgb, 4, 3, f); }
			fclose(f);
			printf("SCENE lin -> %s\n", path);
		} else {
			printf("SCENE lin: failed to open %s for writing\n", path);
		}
		fflush(stdout);
		cf_free(buf);
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
	scene_select(getenv("HRC_SCENE"));

	// Headless capture is its own switch now that scenes cycle at runtime: HRC_SCENE
	// only chooses where to start, in both modes.
	if (getenv("HRC_SHOT")) hrc_ns_testscene();

	// Interactive: draw the selected scene and light it every frame.
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		g_scene_time += CF_DELTA_TIME;
		if (cf_key_just_pressed(CF_KEY_RIGHT) || cf_key_just_pressed(CF_KEY_SPACE)) scene_cycle(1);
		if (cf_key_just_pressed(CF_KEY_LEFT)) scene_cycle(-1);
		// Cascade scale. Storage is sized for scale 1, so this only changes the
		// working extents and the dispatch sizes -- nothing is reallocated.
		if (cf_key_just_pressed(CF_KEY_1)) { g_hrc_scale = 1; hrc_ns_apply_scale(); }
		if (cf_key_just_pressed(CF_KEY_2)) { g_hrc_scale = 2; hrc_ns_apply_scale(); }
		if (cf_key_just_pressed(CF_KEY_4)) { g_hrc_scale = 4; hrc_ns_apply_scale(); }
		if (cf_key_just_pressed(CF_KEY_B)) bounce_on = !bounce_on;
		hrc_ns_scene_draw();
		hrc_ns_compute();
		hrc_text_overlay();
		cf_draw_canvas(hrc.fluence, cf_v2(0, 0), cf_v2((float)world_w, (float)world_h));

		// Controls, drawn like any other shape. No scene title here: every scene
		// captions itself inside the world, so a HUD copy would just duplicate it.
		// Push/pop are balanced -- these stacks are global and leak across frames
		// otherwise.
		cf_push_font(cf_sintern("gi"));
		cf_push_font_size(world_h * 0.022f);
		cf_draw_push_color(cf_make_color_rgb_f(0.62f, 0.72f, 0.80f));
		char hud[256];
		CF_SNPRINTF(hud, sizeof(hud), "%s   (%d/%d)      %dx%d  cascade 1/%d",
			scene_name(), g_scene_index + 1, g_scene_count, world_w, world_h, g_hrc_scale);
		cf_draw_text(hud, cf_v2(-world_w * 0.46f, -world_h * 0.415f), -1);
		CF_SNPRINTF(hud, sizeof(hud), "left/right scene    1/2/4 cascade scale    B bounce %s",
			bounce_on ? "on" : "off");
		cf_draw_text(hud, cf_v2(-world_w * 0.46f, -world_h * 0.45f), -1);
		cf_draw_pop_color();
		cf_pop_font_size();
		cf_pop_font();

		cf_app_draw_onto_screen(true);
	}

	hrc_shutdown();
	cf_destroy_app();
	return 0;
}
