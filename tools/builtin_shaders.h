#ifndef CF_BUILTIN_SHADERS_H
#define CF_BUILTIN_SHADERS_H

#include "cute_shader.h"

#include <stdlib.h>
#include <string.h>


// Computes where the draw fragment shader's payload storage buffer binds for a given
// user shader() stub. SDL_GPU requires fragment set-2 bindings to be contiguous by
// resource class (samplers first, then storage buffers), so the buffer lands right
// after the stub's last declared sampler. The result is injected as the
// CF_PAYLOAD_BINDING preprocessor define (see the payload_buffer in s_draw_fs).
// Expects *preprocessed* source (see cute_shader_preprocess): comments are stripped
// and macros expanded, so a token scan over declaration statements is reliable.
static inline int cf_compute_payload_binding(const char* shd)
{
	#define CF_IS_IDENT_CHAR(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || ((c) >= '0' && (c) <= '9') || (c) == '_')
	int payload_binding = 1;
	const char* stmt = shd; // Start of the current declaration statement.
	const char* s = shd;
	while (s && *s) {
		char c = *s;
		if (c == ';' || c == '{' || c == '}') {
			stmt = ++s;
			continue;
		}
		if (CF_IS_IDENT_CHAR(c) && !(c >= '0' && c <= '9')) {
			const char* id = s;
			while (CF_IS_IDENT_CHAR(*s)) s++;
			size_t n = (size_t)(s - id);
			bool is_sampler = (n == 9 && strncmp(id, "sampler2D", 9) == 0)
			               || (n == 10 && strncmp(id, "usampler2D", 10) == 0);
			if (is_sampler) {
				// Find this declaration's `binding = N` within the statement so far.
				for (const char* b = stmt; b + 7 <= id; b++) {
					if (strncmp(b, "binding", 7) != 0) continue;
					if (b > stmt && CF_IS_IDENT_CHAR(b[-1])) continue;
					if (CF_IS_IDENT_CHAR(b[7])) continue;
					const char* v = b + 7;
					while (*v == ' ' || *v == '\t' || *v == '=') v++;
					int binding = atoi(v);
					if (binding + 1 > payload_binding) payload_binding = binding + 1;
				}
			}
			continue;
		}
		s++;
	}
	#undef CF_IS_IDENT_CHAR
	return payload_binding;
}

//--------------------------------------------------------------------------------------------------
// Builtin shaders. These get cross-compiled at runtime.

// Render as white -- for testing.
static const char* s_basic_vs = R"(
layout (location = 0) in vec2 in_posH;

void main()
{
	gl_Position = vec4(in_posH, 0, 1);
}
)";
static const char* s_basic_fs = R"(
layout(location = 0) out vec4 result;

void main()
{
	result = vec4(1);
}
)";

// Copy the app's offscreen canvas onto the swapchain texture (backfbuffer).
static const char* s_backbuffer_vs = R"(
layout (location = 0) in vec2 in_posH;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec2 uv;

void main()
{
	uv = in_uv;
	gl_Position = vec4(in_posH, 0, 1);
}
)";
static const char* s_backbuffer_fs = R"(
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

static const char* s_gamma = R"(
vec4 gamma(vec4 c)
{
	return vec4(pow(abs(c.rgb), vec3(1.0/2.2)), c.a);
}

vec4 de_gamma(vec4 c)
{
	return vec4(pow(abs(c.rgb), vec3(2.2)), c.a);
}
)";

static const char* s_blend = R"(
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

// Pure SDF helpers shared between fragment shaders (via distance.shd) and the tiled
// renderer's binning compute shaders. Must stay free of globals and derivatives.
static const char* s_sdf_core = R"(
// unpackUnorm4x8 is missing from GLSL ES 3.00; this manual unpack works everywhere.
vec4 cf_unpack_color(uint c)
{
	return vec4(float(c & 255u), float((c >> 8u) & 255u), float((c >> 16u) & 255u), float((c >> 24u) & 255u)) * (1.0 / 255.0);
}

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

// Referenced from: https://www.shadertoy.com/view/wdBXRW
float distance_polygon(vec2 p, vec2[8] v, int N)
{
	float d = dot(p-v[0], p-v[0]);
	float s = 1.0;
	for (int i=0, j=N-1; i<N; j=i, i++) {
		vec2 e = v[j] - v[i];
		vec2 w = p - v[i];
		vec2 b = w - e * clamp(dot(w,e)/dot(e,e), 0.0, 1.0);
		d = min(d, dot(b,b));

		bvec3 cond = bvec3(p.y     >= v[i].y,
		                   p.y     <  v[j].y,
		                   e.x*w.y >  e.y*w.x);
		if (all(cond) || all(not(cond))) {
			s =- s;
		}
	}

	return s * sqrt(d);
}

// Directed arrow as one SDF: union of a round-capped shaft (capsule) and a triangular
// head. A single command means translucent arrows never double-blend where the shaft
// meets the head. r = shaft radius, w = head length and half-width.
float distance_arrow(vec2 p, vec2 a, vec2 b, float r, float w)
{
	vec2 d = b - a;
	float l = safe_len(d);
	vec2 n = l == 0.0 ? vec2(0) : d / l;
	vec2 base = b - n * w;
	vec2 t = vec2(-n.y, n.x) * w;
	float ds = distance_segment(p, a, base) - r;
	float dt = distance_triangle(p, b, base + t, base - t);
	return min(ds, dt);
}
)";

// Default custom-shape include: no shapes registered. cf_make_custom_shape() swaps in a
// generated version stitching every registered `float sdf(vec2 p, ShapeParams s)` snippet
// plus a per-command dispatcher. User snippets must be true signed distance functions
// (Lipschitz <= 1): the binning compute shaders trust them for tile culling and
// opaque-cover occlusion.
static const char* s_custom_shapes_stub = R"(
struct ShapeParams
{
	vec2 a, b, c, d, e, f, g, h;
	vec4 attributes;
};

float custom_sdf(int shape_id, vec2 p, ShapeParams s)
{
	return 3.402823e38;
}
)";

// CSG shape groups (cf_draw_shape_group_begin/end): one composite command whose payload
// streams operand primitives folded left-to-right with min/max (smooth variants via k).
// Consumers must declare the `payload` storage buffer and include custom_shapes.shd
// before including this file.
static const char* s_csg = R"(
float csg_smin(float a, float b, float k)
{
	if (k <= 0.0) return min(a, b);
	float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
	return mix(b, a, h) - k * h * (1.0 - h);
}

float csg_distance(uint po, int n, vec2 p, vec4 attributes)
{
	// Payload: po+0 = pre-padded bounds (used by the vertex stage), then six vec4s per
	// operand: (prim, aux, op, k), (radius, 0, 0, 0), and 8 vec2s of shape params.
	float d = 3.402823e38;
	vec2 cpts[8];
	for (int i = 0; i < n; ++i) {
		uint o = po + 1u + uint(i) * 6u;
		vec4 h4 = cf_payload(o);
		float r = cf_payload(o + 1u).x;
		vec4 q0 = cf_payload(o + 2u);
		vec4 q1 = cf_payload(o + 3u);
		vec4 q2 = cf_payload(o + 4u);
		vec4 q3 = cf_payload(o + 5u);
		int prim = int(h4.x);
		float di;
		if (prim == 3) {
			di = min(distance_segment(p, q0.xy, q0.zw), distance_segment(p, q0.zw, q1.xy));
		} else if (prim == 2) {
			di = distance_box(p, q0.xy, q0.zw, q1.xy);
		} else if (prim == 5) {
			di = distance_triangle(p, q0.xy, q0.zw, q1.xy);
		} else if (prim == 8) {
			di = distance_arrow(p, q0.xy, q0.zw, q1.x, q1.y);
		} else if (prim == 9) {
			ShapeParams sp;
			sp.a = q0.xy; sp.b = q0.zw; sp.c = q1.xy; sp.d = q1.zw;
			sp.e = q2.xy; sp.f = q2.zw; sp.g = q3.xy; sp.h = q3.zw;
			sp.attributes = attributes;
			di = custom_sdf(int(h4.y), p, sp);
		} else {
			cpts[0] = q0.xy; cpts[1] = q0.zw; cpts[2] = q1.xy; cpts[3] = q1.zw;
			cpts[4] = q2.xy; cpts[5] = q2.zw; cpts[6] = q3.xy; cpts[7] = q3.zw;
			di = distance_polygon(p, cpts, int(h4.y));
		}
		di -= r;
		int op = int(h4.z);
		d = i == 0 ? di
		  : op == 0 ? csg_smin(d, di, h4.w)
		  : op == 1 ? -csg_smin(-d, di, h4.w)
		  : -csg_smin(-d, -di, h4.w);
	}
	return d;
}
)";

static const char* s_distance = R"(
#include "sdf_core.shd"

float sdf_stroke(float d)
{
	return abs(d) - v_stroke;
}

float dd(float d)
{
	return length(vec2(dFdx(d), dFdy(d)));
}

// Given two colors a and b, and a distance to the isosurface of a shape,
// apply antialiasing, fill, and surface stroke FX.
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
)";

// Curve text glyphs: per-pixel coverage evaluated directly from the glyph's quadratic
// Bezier outline, fetched from an RGBA8 atlas strip (3 texels per curve, two 16-bit
// fixed-point coords per texel, fractions of the glyph's quantization box).
//
// Winding technique after E. Lengyel, "GPU-Centered Font Rendering Directly from Glyph
// Outlines" (JCGT 2017); the patent has been dedicated to the public domain. Which of a
// curve's two roots count as ray crossings folds into a 2-bit lookup indexed by the
// control points' y-sign pattern (the 0x2E74 table). Crossings accumulate fractionally
// within the pixel footprint instead of as hard +-1 steps -- analytic AA at any scale --
// and two perpendicular rays average down x/y aliasing. texelFetch carries no implicit
// derivatives, so everything here is safe inside divergent branches.
//
// Consumers must declare the `u_image` sampler and include distance.shd (for safe_div)
// before including this file.
static const char* s_glyph = R"(
// w is the block's content width in texels: single-row glyph strips never wrap, while
// multi-row path blocks wrap texel index i across rows.
vec2 cf_glyph_cp(ivec2 base, int i, int w)
{
	vec4 t = texelFetch(u_image, base + ivec2(i % w, i / w), 0) * 255.0;
	return vec2(t.r + t.g * 256.0, t.b + t.a * 256.0) * (1.0 / 65535.0);
}

// Fractional crossing count of the +x ray from the origin against one quadratic
// (control points given relative to the pixel). inv_px converts a crossing's x offset
// into pixel-footprint coverage.
float cf_glyph_ray(vec2 a, vec2 b, vec2 c, float inv_px)
{
	uint code = 0x2E74u >> (((a.y > 0.0) ? 2u : 0u) + ((b.y > 0.0) ? 4u : 0u) + ((c.y > 0.0) ? 8u : 0u));
	if ((code & 3u) == 0u) return 0.0;
	// y(t) = Ay t^2 - 2 By t + a.y. Nearly-straight-in-y curves (lines ride this path
	// as degenerate quads) fall back to the linear root.
	float Ay = a.y - 2.0 * b.y + c.y;
	float By = a.y - b.y;
	float Ax = a.x - 2.0 * b.x + c.x;
	float Bx = a.x - b.x;
	float t1, t2;
	if (abs(Ay) < 1e-4 * (abs(a.y) + abs(b.y) + abs(c.y))) {
		t1 = safe_div(0.5 * a.y, By);
		t2 = t1;
	} else {
		float q = sqrt(max(By * By - Ay * a.y, 0.0));
		t1 = (By - q) / Ay;
		t2 = (By + q) / Ay;
	}
	float cov = 0.0;
	if ((code & 1u) != 0u) {
		float x = (Ax * t1 - 2.0 * Bx) * t1 + a.x;
		cov += clamp(0.5 + x * inv_px, 0.0, 1.0);
	}
	if ((code & 2u) != 0u) {
		float x = (Ax * t2 - 2.0 * Bx) * t2 + a.x;
		cov -= clamp(0.5 + x * inv_px, 0.0, 1.0);
	}
	return cov;
}

// Exact nearest squared distance from the origin to one quadratic, via the closed-form
// cubic solve (Cardano), after I. Quilez's construction. The strip's line encoding
// {p1, p2, p2} keeps the t^2 coefficient nonzero for straight lines; the eps guard
// covers the measure-zero case of an off-curve point exactly at a chord midpoint.
float cf_glyph_dist_sq(vec2 A, vec2 B, vec2 C)
{
	vec2 a = B - A;
	vec2 b = A - 2.0 * B + C;
	vec2 c = a * 2.0;
	vec2 d = A;
	float kk = 1.0 / max(dot(b, b), 1e-12);
	float kx = kk * dot(a, b);
	float ky = kk * (2.0 * dot(a, a) + dot(d, b)) / 3.0;
	float kz = kk * dot(d, a);
	float p = ky - kx * kx;
	float p3 = p * p * p;
	float q = kx * (2.0 * kx * kx - 3.0 * ky) + kz;
	float h = q * q + 4.0 * p3;
	if (h >= 0.0) {
		h = sqrt(h);
		vec2 x = (vec2(h, -h) - q) / 2.0;
		vec2 uv = sign(x) * pow(abs(x), vec2(1.0 / 3.0));
		float t = clamp(uv.x + uv.y - kx, 0.0, 1.0);
		vec2 e = d + (c + b * t) * t;
		return dot(e, e);
	} else {
		float z = sqrt(-p);
		float v = acos(q / (p * z * 2.0)) / 3.0;
		float m = cos(v);
		float n = sin(v) * 1.732050808;
		vec2 t = clamp(vec2(m + m, -n - m) * z - kx, 0.0, 1.0);
		vec2 e0 = d + (c + b * t.x) * t.x;
		vec2 e1 = d + (c + b * t.y) * t.y;
		return min(dot(e0, e0), dot(e1, e1));
	}
}

// Coverage (0..1) plus optional unsigned distance for one glyph command. p, q0, e1, e2
// live in the command's record-time world space; px is that space's per-screen-pixel
// step along x and y. Sign never matters downstream: fills consume coverage directly
// and strokes consume abs distance, so the winding sum only needs magnitude.
float cf_glyph_eval(vec2 p, vec2 q0, vec2 e1, vec2 e2, ivec2 base, int strip_w, int n, vec2 px, bool want_dist, out float dist)
{
	float cov_x = 0.0;
	float cov_y = 0.0;
	float d_sq = 3.402823e38;
	float inv_px_x = 1.0 / max(px.x, 1e-12);
	float inv_px_y = 1.0 / max(px.y, 1e-12);
	for (int i = 0; i < n; ++i) {
		vec2 f0 = cf_glyph_cp(base, i * 3, strip_w);
		vec2 f1 = cf_glyph_cp(base, i * 3 + 1, strip_w);
		vec2 f2 = cf_glyph_cp(base, i * 3 + 2, strip_w);
		vec2 a = q0 + f0.x * e1 + f0.y * e2 - p;
		vec2 b = q0 + f1.x * e1 + f1.y * e2 - p;
		vec2 c = q0 + f2.x * e1 + f2.y * e2 - p;
		cov_x += cf_glyph_ray(a, b, c, inv_px_x);
		cov_y -= cf_glyph_ray(vec2(a.y, a.x), vec2(b.y, b.x), vec2(c.y, c.x), inv_px_y);
		if (want_dist) d_sq = min(d_sq, cf_glyph_dist_sq(a, b, c));
	}
	dist = sqrt(d_sq);
	return 0.5 * (clamp(abs(cov_x), 0.0, 1.0) + clamp(abs(cov_y), 0.0, 1.0));
}
)";

static const char* s_smooth_uv = R"(
// Variant taking a precomputed screen-space texel footprint (fwidth(uv * texture_size)),
// for contexts where implicit derivatives are unavailable (FXC forbids gradient
// instructions inside dynamic loops -- see the tiled walk shader).
vec2 smooth_uv_fw(vec2 uv, vec2 texture_size, vec2 fw)
{
	vec2 pixel = uv * texture_size;
	vec2 seam = floor(pixel + 0.5);
	pixel = seam + clamp((pixel - seam) / fw, -0.5, 0.5);
	return pixel / texture_size;
}

vec2 smooth_uv(vec2 uv, vec2 texture_size)
{
	vec2 pixel = uv * texture_size;
	vec2 seam = floor(pixel + 0.5);
	pixel = seam + clamp((pixel - seam) / fwidth(pixel), -0.5, 0.5);
	return pixel / texture_size;
}
)";

// Stub function. This gets replaced by injected user-shader code via #include.
static const char* s_shader_stub = R"(
vec4 shader(vec4 color, ShaderParams params)
{
	return color;
}
)";

//--------------------------------------------------------------------------------------------------
// Primary cute_draw.h shader.


static const char* s_draw_fs = R"(
layout (location = 0) in vec4 v_pos_uv;
layout (location = 1) in flat int v_n;
layout (location = 2) in vec4 v_ab;
layout (location = 3) in vec4 v_cd;
layout (location = 4) in vec4 v_ef;
layout (location = 5) in vec4 v_gh;
layout (location = 6) in vec4 v_col;
layout (location = 7) in vec4 v_shape;
layout (location = 8) in vec4 v_blend_posH;
layout (location = 9) in vec4 v_user;
layout (location = 10) in vec4 v_uv_bounds;
layout (location = 11) in flat int v_po;

layout(location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_image;

layout (set = 3, binding = 0) uniform uniform_block {
	vec2 u_texture_size;
	int u_alpha_discard;
	int u_use_smooth_uv;
};

// Storage access: real SSBOs by default; on GLES3/WebGL2 (define CF_GLES) storage
// buffers are emulated as RGBA32UI texel fetches (see cf_gles_make_storage_buffer).
#ifdef CF_GLES
layout (set = 2, binding = 14) uniform highp usampler2D u_fs_storage_0; // payload
vec4 cf_payload(uint i)
{
	uvec4 u = texelFetch(u_fs_storage_0, ivec2(int(i) & 1023, int(i) >> 10), 0);
	return vec4(uintBitsToFloat(u.x), uintBitsToFloat(u.y), uintBitsToFloat(u.z), uintBitsToFloat(u.w));
}
#else
#ifndef CF_PAYLOAD_BINDING
#define CF_PAYLOAD_BINDING 1
#endif
layout (std430, set = 2, binding = CF_PAYLOAD_BINDING) readonly buffer payload_buffer { vec4 payload[]; };
vec4 cf_payload(uint i) { return payload[i]; }
#endif


// Used only for polygon SDF.
vec2 pts[8];

// Backwards-compat globals.
// Here for convenience for any legacy user draw shaders that reference them.
vec2 v_pos;
vec2 v_uv;
vec2 v_posH;
float v_radius;
float v_stroke;
float v_aa;
float v_type;
float v_alpha;
float v_fill;
vec2 pos;
vec2 screen_uv;

#include "blend.shd"
#include "gamma.shd"
#include "smooth_uv.shd"
#include "distance.shd"
#include "glyph.shd"
#include "custom_shapes.shd"
#include "csg.shd"

struct ShaderParams
{
	vec2 view_pos;
	vec2 uv;
	vec2 uv_min;
	vec2 uv_max;
	vec2 screen_uv;
	vec4 attributes;
};

#include "shader_stub.shd"

void main()
{
	// Unpack into backwards-compat globals.
	v_pos    = v_pos_uv.xy;
	v_uv     = v_pos_uv.zw;
	v_radius = v_shape.x;
	v_stroke = v_shape.y;
	v_aa     = v_shape.z;
	v_type   = v_shape.w;
	v_alpha  = v_blend_posH.x;
	v_fill   = v_blend_posH.y;
	v_posH   = v_blend_posH.zw;

	bool is_sprite  = v_type >= 0.0 && v_type < 0.5;
	bool is_text    = v_type >  0.5 && v_type < 1.5;
	bool is_box     = v_type >  1.5 && v_type < 2.5;
	bool is_seg     = v_type >  2.5 && v_type < 3.5;
	bool is_tri     = v_type >  3.5 && v_type < 4.5;
	bool is_tri_sdf = v_type >  4.5 && v_type < 5.5;
	bool is_poly    = v_type >  5.5 && v_type < 6.5;
	bool is_seg_clip = v_type > 6.5 && v_type < 7.5;
	bool is_arrow   = v_type >  7.5 && v_type < 8.5;
	bool is_custom  = v_type >  8.5 && v_type < 9.5;
	bool is_csg     = v_type >  9.5 && v_type < 10.5;
	bool is_glyph   = v_type > 10.5 && v_type < 11.5;

	// Traditional sprite/text/tri cases.
	vec4 c = vec4(0);
	vec2 uv = u_use_smooth_uv == 0 ? smooth_uv(v_uv, u_texture_size) : v_uv;
	vec4 tex_c = de_gamma(texture(u_image, uv));
	c = is_sprite ? gamma(tex_c) : c;
	c = is_text ? v_col * tex_c.a : c;
	c = is_tri ? v_col : c;

	// SDF cases.
	float d = 0;
	if (is_box) {
		d = distance_box(v_pos, v_ab.xy, v_ab.zw, v_cd.xy);
	} else if (is_seg) {
		d = distance_segment(v_pos, v_ab.xy, v_ab.zw);
		d = min(d, distance_segment(v_pos, v_ab.zw, v_cd.xy));
	} else if (is_tri_sdf) {
		d = distance_triangle(v_pos, v_ab.xy, v_ab.zw, v_cd.xy);
	} else if (is_poly) {
		pts[0] = v_ab.xy;
		pts[1] = v_ab.zw;
		pts[2] = v_cd.xy;
		pts[3] = v_cd.zw;
		pts[4] = v_ef.xy;
		pts[5] = v_ef.zw;
		pts[6] = v_gh.xy;
		pts[7] = v_gh.zw;
		d = distance_polygon(v_pos, pts, v_n);
	} else if (is_seg_clip) {
		d = distance_segment(v_pos, v_ab.xy, v_ab.zw);
	} else if (is_arrow) {
		d = distance_arrow(v_pos, v_ab.xy, v_ab.zw, v_cd.x, v_cd.y);
	} else if (is_custom) {
		ShapeParams sp_custom;
		sp_custom.a = v_ab.xy; sp_custom.b = v_ab.zw;
		sp_custom.c = v_cd.xy; sp_custom.d = v_cd.zw;
		sp_custom.e = v_ef.xy; sp_custom.f = v_ef.zw;
		sp_custom.g = v_gh.xy; sp_custom.h = v_gh.zw;
		sp_custom.attributes = v_user;
		d = custom_sdf(v_n, v_pos, sp_custom);
	} else if (is_csg) {
		d = csg_distance(uint(v_po), v_n, v_pos, v_user);
	} else if (is_glyph) {
		// Curve glyph: winding coverage from the outline's quadratics. Derivatives are
		// taken before any pixel-divergent math and the branch is primitive-uniform
		// (one instance per command), so they stay well-defined.
		vec4 gp0 = cf_payload(uint(v_po));
		vec4 gp1 = cf_payload(uint(v_po) + 1u);
		vec4 gp2 = cf_payload(uint(v_po) + 2u);
		vec2 gpx = vec2(length(vec2(dFdx(v_pos.x), dFdy(v_pos.x))), length(vec2(dFdx(v_pos.y), dFdy(v_pos.y))));
		float gd;
		float gcov = cf_glyph_eval(v_pos, gp0.xy, gp0.zw, gp1.xy, ivec2(gp2.xy), int(gp2.z), v_n, gpx, v_stroke > 0.0, gd);
		// Per-corner colors (TL,TR,BR,BL) bilerped by box fraction, matching the
		// rasterized text path's text-effect gradients.
		vec4 gc = cf_payload(uint(v_po) + 3u);
		float gs = clamp(dot(v_pos - gp0.xy, gp0.zw) * gp1.z, 0.0, 1.0);
		float gt = clamp(dot(v_pos - gp0.xy, gp1.xy) * gp1.w, 0.0, 1.0);
		vec4 gvcol = mix(mix(cf_unpack_color(floatBitsToUint(gc.w)), cf_unpack_color(floatBitsToUint(gc.z)), gs),
		                 mix(cf_unpack_color(floatBitsToUint(gc.x)), cf_unpack_color(floatBitsToUint(gc.y)), gs), gt);
		c = v_stroke > 0.0 ? sdf(vec4(0), gvcol, gd) : gvcol * gcov;
	}
	c = (!is_sprite && !is_text && !is_tri && !is_glyph) ? sdf(c, v_col, d - v_radius) : c;
	// Polyline bodies: hard bisector-plane clip. Strict on plane0 and inclusive on
	// plane1 so the shared boundary between neighboring bodies belongs to exactly one.
	if (is_seg_clip) {
		bool keep = dot(v_cd.xy, v_pos) - v_ef.x < 0.0 && dot(v_cd.zw, v_pos) - v_ef.y <= 0.0;
		c *= keep ? 1.0 : 0.0;
	}

	c *= v_alpha;
	pos = v_pos;
	screen_uv = (v_posH + vec2(1,-1)) * 0.5 * vec2(1,-1);
	ShaderParams sp;
	sp.view_pos = pos;
	sp.uv = v_uv;
	sp.uv_min = v_uv_bounds.xy;
	sp.uv_max = v_uv_bounds.zw;
	sp.screen_uv = screen_uv;
	sp.attributes = v_user;
	c = shader(c, sp);
	if (u_alpha_discard != 0 && c.a == 0) discard;

	// D3D12 links VS->PS by hardware register per semantic: the PS input signature must
	// mirror the VS output registers, but the compiler packs PS inputs by *consumed*
	// varyings, so a user shader() stub referencing a sparse subset would shift register
	// assignment and fail pipeline creation. Keep every varying live unconditionally
	// (the uniform can never hold this value, and the compiler cannot prove that).
	if (u_alpha_discard == -2147483647) {
		c += v_pos_uv + v_ab + v_cd + v_ef + v_gh + v_col + v_shape + v_blend_posH + v_user + v_uv_bounds + vec4(float(v_n), float(v_po), 0, 0);
	}
	result = c;
}
)";

// Payload access, needed for CSG shape groups (variable-length operand lists cannot
// ride in varyings). SSBO flavor: SDL_GPU requires set-2 bindings to be contiguous by
// resource class (samplers first, then storage buffers), so the binding lands right
// after the last sampler -- computed per compile from the user shader's samplers and
// injected as a define (see cf_compile_shader_to_bytecode_internal).

// GLES flavor: texel fetches from the emulated storage-buffer texture. Binding 14 is
// out of the way of user shader samplers (the GLES backend binds by name, not slot).



//--------------------------------------------------------------------------------------------------
// Tiled draw path shaders (see cute_draw.cpp, s_draw_report_tiled).
//
// The screen is cut into fixed-size square tiles. The CPU bins draw commands into per-tile
// lists and uploads them via storage buffers. A single fullscreen triangle walks each
// pixel's tile list, evaluating SDFs / sprite samples per command and compositing
// in-register (premultiplied src-over), writing the framebuffer exactly once.
//
// Derivative discipline: the tile size is even, so a 2x2 fragment quad never straddles a
// tile boundary -- every pixel in a quad walks the identical command list in lockstep.
// Branches on per-command data are therefore quad-uniform and implicit derivatives
// (texture, fwidth in smooth_uv) inside them are well-defined. Pixel-dependent conditions
// (coverage) must only ever mask a command's contribution, never branch around a sample.

static const char* s_tile_vs = R"(
layout (location = 0) in vec2 in_posH;

void main()
{
	gl_Position = vec4(in_posH, 0, 1);
}
)";

static const char* s_tile_fs = R"(
layout (location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_image;

struct Cmd
{
	vec4 aabb;    // Pixel-space bounds, top-left origin: min.xy, max.xy.
	uvec4 meta;   // x: type, y: rgba8 color, z: payload offset (vec4 units), w: inverse-mvp offset (vec4 units).
	vec4 shape;   // radius, stroke (pre-halved), aa, alpha.
	vec4 misc;    // x: fill (0 or 1), y: polygon vert count, z, w: unused.
	vec4 user;    // User params (ShaderParams.attributes).
};

layout (std430, set = 2, binding = 1) readonly buffer cmd_buffer { Cmd cmds[]; };
layout (std430, set = 2, binding = 2) readonly buffer payload_buffer { vec4 payload[]; };
layout (std430, set = 2, binding = 3) readonly buffer tile_buffer { uvec2 tiles[]; };
layout (std430, set = 2, binding = 4) readonly buffer list_buffer { uint tile_list[]; };

layout (set = 3, binding = 0) uniform uniform_block {
	vec2 u_texture_size;
	vec2 u_canvas_wh;
	int u_alpha_discard;
	int u_use_smooth_uv;
	int u_tiles_x;
	int u_tile_px;
	int u_blend; // CF_DrawBlend for this run; runs split at mode changes CPU-side.
};

// Globals consumed by the sdf() helpers in distance.shd, set per command in the walk loop.
float v_stroke;
float v_aa;
float v_fill;

vec4 cf_payload(uint i) { return payload[i]; }

#include "gamma.shd"
#include "smooth_uv.shd"
#include "distance.shd"
#include "glyph.shd"
#include "custom_shapes.shd"
#include "csg.shd"

#define CMD_TYPE_SPRITE   0u
#define CMD_TYPE_TEXT     1u
#define CMD_TYPE_BOX      2u
#define CMD_TYPE_SEGMENT  3u
#define CMD_TYPE_TRI      4u
#define CMD_TYPE_TRI_SDF  5u
#define CMD_TYPE_POLYGON  6u
#define CMD_TYPE_SEG_CLIP 7u
#define CMD_TYPE_ARROW    8u
#define CMD_TYPE_CUSTOM   9u
#define CMD_TYPE_CSG      10u
#define CMD_TYPE_GLYPH    11u

vec2 pts[8];

void main()
{
	vec2 frag = gl_FragCoord.xy; // Pixel centers, top-left origin.
	ivec2 tile = ivec2(frag) / u_tile_px;
	uvec2 range = tiles[uint(tile.y * u_tiles_x + tile.x)]; // offset, count
	vec2 ndc = vec2(frag.x * (2.0 / u_canvas_wh.x) - 1.0, 1.0 - frag.y * (2.0 / u_canvas_wh.y));

	// Walk the list top-down with premultiplied "under" compositing (equivalent to
	// bottom-up src-over). Once a pixel's accumulated alpha saturates to exactly 1.0,
	// everything deeper contributes zero -- SDF/tri commands then skip evaluation
	// entirely (a pixel-divergent skip is legal there: no derivatives). Sprite/text
	// commands still evaluate to keep texture fetches quad-uniform; their contribution
	// multiplies to zero. Non-normal blend runs swap the accumulate operator: additive
	// sums, multiply accumulates per-channel gain in `trans`, screen under-composites
	// per channel.
	vec4 acc = vec4(0);
	vec3 trans = vec3(1.0);
	for (uint i = range.y; i > 0u;) {
		--i;
		Cmd cmd = cmds[tile_list[range.x + i]];
		uint type = cmd.meta.x;
		vec4 col = unpackUnorm4x8(cmd.meta.y);
		uint po = cmd.meta.z;

		// Pixel-space AABB coverage. Mask only -- never branch on this around a sample.
		vec2 cov2 = step(cmd.aabb.xy, frag) * step(frag, cmd.aabb.zw);
		float coverage = cov2.x * cov2.y;

		// Saturation early-out per blend mode: src-over saturates on alpha, multiply
		// when all gain is spent, screen when every channel hits white. Additive never
		// saturates.
		float done = u_blend == 0 ? acc.a
		           : u_blend == 2 ? 1.0 - max(trans.x, max(trans.y, trans.z))
		           : u_blend == 3 ? min(acc.r, min(acc.g, acc.b))
		           : 0.0;

		// Recover the command's record-time world space (inverse mvp at palette + 2).
		vec4 im0 = payload[cmd.meta.w + 2u];
		vec4 im1 = payload[cmd.meta.w + 3u];
		vec2 p = im0.xy * ndc.x + im0.zw * ndc.y + im1.xy;

		v_stroke = cmd.shape.y;
		v_aa = cmd.shape.z;
		v_fill = cmd.misc.x;

		vec4 c = vec4(0);
		if (type <= CMD_TYPE_TEXT) {
			// Sprite/text: quad corners are recorded post-mvp, so map NDC into the quad,
			// then to atlas uv. q0 = quad top-left corner, e1 = top edge, e2 = left edge.
			vec4 P0 = payload[po];
			vec4 P1 = payload[po + 1u];
			vec4 uvb = payload[po + 2u]; // minx, maxy, maxx, miny
			vec2 q0 = P0.xy;
			vec2 e1 = P0.zw;
			vec2 e2 = P1.xy;
			float s = dot(p - q0, e1) * P1.z;
			float t = dot(p - q0, e2) * P1.w;
			vec2 uv = vec2(mix(uvb.x, uvb.z, s), mix(uvb.y, uvb.w, t));
			// Sample unconditionally (quad-uniform), clamp into the glyph's uv rect to
			// avoid bleeding neighboring atlas entries, mask outside the quad after.
			uv = clamp(uv, min(uvb.xy, uvb.zw), max(uvb.xy, uvb.zw));
			// The mesh path computes gamma(de_gamma(tex)) for sprites and de_gamma(tex).a
			// for text; both are identities (de_gamma leaves alpha untouched), so skip
			// the two pow() round-trips here.
#ifdef CF_NO_IMPLICIT_GRADIENTS
			// FXC (DXBC SM 5.1) forbids gradient instructions inside the dynamic tile
			// loop. The quad's uv mapping is affine per command, so the screen-space
			// texel footprint smooth_uv needs is computable analytically, and sampling
			// goes explicit-lod (atlas textures are single-mip, so lod 0 is exact).
			vec2 gdx = im0.xy * (2.0 / u_canvas_wh.x);
			vec2 gdy = im0.zw * (2.0 / u_canvas_wh.y);
			float gds = (abs(dot(gdx, e1)) + abs(dot(gdy, e1))) * P1.z;
			float gdt = (abs(dot(gdx, e2)) + abs(dot(gdy, e2))) * P1.w;
			vec2 fw = vec2(gds * abs(uvb.z - uvb.x), gdt * abs(uvb.w - uvb.y)) * u_texture_size;
			vec2 uv_final = u_use_smooth_uv == 0 ? smooth_uv_fw(uv, u_texture_size, fw) : uv;
			vec4 tex_c = textureLod(u_image, uv_final, 0.0);
#else
			vec2 uv_final = u_use_smooth_uv == 0 ? smooth_uv(uv, u_texture_size) : uv;
			vec4 tex_c = texture(u_image, uv_final);
#endif
			float quad_cov = step(0.0, s) * step(s, 1.0) * step(0.0, t) * step(t, 1.0);
			if (type == CMD_TYPE_SPRITE) {
				c = tex_c;
			} else {
				vec4 tl = unpackUnorm4x8(floatBitsToUint(payload[po + 3u].x));
				vec4 tr = unpackUnorm4x8(floatBitsToUint(payload[po + 3u].y));
				vec4 br = unpackUnorm4x8(floatBitsToUint(payload[po + 3u].z));
				vec4 bl = unpackUnorm4x8(floatBitsToUint(payload[po + 3u].w));
				vec4 vcol = mix(mix(tl, tr, s), mix(bl, br, s), t);
				c = vcol * tex_c.a;
			}
			c *= quad_cov;
		} else if (type == CMD_TYPE_TRI) {
			if (done >= 1.0 || coverage == 0.0) continue; // Contribution would be exactly zero.
			// Raw triangle, evaluated directly in NDC with barycentric color interpolation.
			vec4 P0 = payload[po];
			vec4 P1 = payload[po + 1u];
			vec4 P2 = payload[po + 2u];
			vec2 a = P0.xy, b = P0.zw, cc = P1.xy;
			float denom = det2(b - a, cc - a);
			float inv_denom = safe_div(1.0, denom);
			float w0 = det2(b - p, cc - p) * inv_denom;
			float w1 = det2(cc - p, a - p) * inv_denom;
			float w2 = 1.0 - w0 - w1;
			float inside = step(0.0, w0) * step(0.0, w1) * step(0.0, w2);
			uint flags = floatBitsToUint(P2.w);
			vec4 c0 = unpackUnorm4x8(floatBitsToUint(P2.x));
			vec4 c1 = unpackUnorm4x8(floatBitsToUint(P2.y));
			vec4 c2 = unpackUnorm4x8(floatBitsToUint(P2.z));
			vec4 vcol = (flags & 1u) != 0u ? c0 * w0 + c1 * w1 + c2 * w2 : col;
			c = vcol * inside;
		} else if (type == CMD_TYPE_GLYPH) {
			// Curve glyph: winding coverage straight from the outline's quadratics
			// (texelFetch has no implicit derivatives -- a divergent skip is safe here).
			if (done >= 1.0 || coverage == 0.0) continue;
			vec4 P0 = payload[po];
			vec4 P1 = payload[po + 1u];
			vec4 P2 = payload[po + 2u];
			// World-per-screen-pixel from the inverse mvp columns (same construction as
			// the binning cull's r_tile, split per axis for the two AA rays).
			vec2 dwx = im0.xy * (2.0 / u_canvas_wh.x);
			vec2 dwy = im0.zw * (2.0 / u_canvas_wh.y);
			vec2 gpx = vec2(length(vec2(dwx.x, dwy.x)), length(vec2(dwx.y, dwy.y)));
			float gd;
			float gcov = cf_glyph_eval(p, P0.xy, P0.zw, P1.xy, ivec2(P2.xy), int(P2.z), int(cmd.misc.y), gpx, v_stroke > 0.0, gd);
			// Per-corner colors (TL,TR,BR,BL) bilerped by box fraction, matching the
			// rasterized text path's text-effect gradients.
			vec4 gc = payload[po + 3u];
			float gs = clamp(dot(p - P0.xy, P0.zw) * P1.z, 0.0, 1.0);
			float gt = clamp(dot(p - P0.xy, P1.xy) * P1.w, 0.0, 1.0);
			vec4 gvcol = mix(mix(unpackUnorm4x8(floatBitsToUint(gc.w)), unpackUnorm4x8(floatBitsToUint(gc.z)), gs),
			                 mix(unpackUnorm4x8(floatBitsToUint(gc.x)), unpackUnorm4x8(floatBitsToUint(gc.y)), gs), gt);
			c = v_stroke > 0.0 ? sdf(vec4(0), gvcol, gd) : gvcol * gcov;
		} else {
			if (done >= 1.0 || coverage == 0.0) continue; // Contribution would be exactly zero.
			// SDF shapes. Params live in record-time world space; recover it from NDC
			// via the command's inverse mvp. Contributions fade out naturally past the
			// AA fringe, so the AABB mask only trims work the mesh path's quad would
			// have clipped anyway.
			float d = 0.0;
			vec4 P0 = payload[po];
			vec4 P1 = payload[po + 1u];
			float clip = 1.0;
			if (type == CMD_TYPE_BOX) {
				d = distance_box(p, P0.xy, P0.zw, P1.xy);
			} else if (type == CMD_TYPE_SEGMENT) {
				d = distance_segment(p, P0.xy, P0.zw);
				d = min(d, distance_segment(p, P0.zw, P1.xy));
			} else if (type == CMD_TYPE_TRI_SDF) {
				d = distance_triangle(p, P0.xy, P0.zw, P1.xy);
			} else if (type == CMD_TYPE_SEG_CLIP) {
				// Polyline body: capsule SDF, hard bisector-plane clip (strict plane0,
				// inclusive plane1 so shared boundaries belong to exactly one body).
				d = distance_segment(p, P0.xy, P0.zw);
				vec4 P2 = payload[po + 2u];
				bool keep = dot(P1.xy, p) - P2.x < 0.0 && dot(P1.zw, p) - P2.y <= 0.0;
				clip = keep ? 1.0 : 0.0;
			} else if (type == CMD_TYPE_ARROW) {
				d = distance_arrow(p, P0.xy, P0.zw, P1.x, P1.y);
			} else if (type == CMD_TYPE_CUSTOM) {
				vec4 P2 = payload[po + 2u];
				vec4 P3 = payload[po + 3u];
				ShapeParams sp;
				sp.a = P0.xy; sp.b = P0.zw; sp.c = P1.xy; sp.d = P1.zw;
				sp.e = P2.xy; sp.f = P2.zw; sp.g = P3.xy; sp.h = P3.zw;
				sp.attributes = cmd.user;
				d = custom_sdf(int(cmd.misc.y), p, sp);
			} else if (type == CMD_TYPE_CSG) {
				d = csg_distance(po, int(cmd.misc.y), p, cmd.user);
			} else {
				vec4 P2 = payload[po + 2u];
				vec4 P3 = payload[po + 3u];
				pts[0] = P0.xy;
				pts[1] = P0.zw;
				pts[2] = P1.xy;
				pts[3] = P1.zw;
				pts[4] = P2.xy;
				pts[5] = P2.zw;
				pts[6] = P3.xy;
				pts[7] = P3.zw;
				d = distance_polygon(p, pts, int(cmd.misc.y));
			}
			c = sdf(vec4(0), col, d - cmd.shape.x) * clip;
		}

		c *= cmd.shape.w * coverage; // alpha modulation + AABB mask
		if (u_blend == 0) {
			acc = acc + c * (1.0 - acc.a); // Premultiplied "under" (top-down), in-register.
		} else if (u_blend == 1) {
			acc.rgb += c.rgb; // Additive: order-independent sum.
		} else if (u_blend == 2) {
			trans *= (1.0 - c.a) + c.rgb; // Multiply: accumulate per-channel gain.
		} else {
			acc.rgb += (1.0 - acc.rgb) * c.rgb; // Screen: per-channel under-composite.
		}
	}

	// Non-normal runs write alpha 0 (canvas alpha preserved by the run's blend state);
	// multiply outputs its accumulated gain for the DST_COLOR/ZERO composite.
	if (u_blend == 2) acc = vec4(trans, 0.0);
	else if (u_blend != 0) acc.a = 0.0;
	if (u_alpha_discard != 0 && u_blend == 0 && acc.a == 0.0) discard;
	result = acc;
}
)";

//--------------------------------------------------------------------------------------------------
// Instanced command-fed vertex shader. Pairs with s_draw_fs.
//
// The mesh path's counterpart to the tile walk: instead of the CPU expanding each shape
// into six fat pre-transformed vertices, one instance per command reads the same
// cmds/payload storage buffers the tiled path uploads, derives the coverage quad from
// the shape params right here (this is where the old CPU-side OBB fitting moved), and
// produces all of s_draw_fs's varyings. Camera transform comes from the matrix palette
// (forward mvp at the entry, inverse at +2 for the tile walk).

// The instanced VS (and s_draw_fs below) assemble from head + access + body pieces so
// storage-buffer reads can swap per backend: real SSBOs on compute-capable backends,
// RGBA32UI texel fetches on GLES3 (no SSBOs there -- cf_gles emulates CF_StorageBuffer
// as a 1024-wide texture, one texel per vec4, row-major).

static const char* s_inst_vs = R"(
layout (location = 0) in float in_corner;

struct Cmd
{
	vec4 aabb;
	uvec4 meta;
	vec4 shape;
	vec4 misc;
	vec4 user;
};

// Storage access: real SSBOs by default; on GLES3/WebGL2 (define CF_GLES) storage
// buffers are emulated as RGBA32UI texel fetches (see cf_gles_make_storage_buffer).
#ifdef CF_GLES
layout (set = 0, binding = 0) uniform highp usampler2D u_vs_storage_0; // cmds: 5 texels each
layout (set = 0, binding = 1) uniform highp usampler2D u_vs_storage_1; // payload
uvec4 cf_fetch0(uint i) { return texelFetch(u_vs_storage_0, ivec2(int(i) & 1023, int(i) >> 10), 0); }
uvec4 cf_fetch1(uint i) { return texelFetch(u_vs_storage_1, ivec2(int(i) & 1023, int(i) >> 10), 0); }
vec4 cf_bits4(uvec4 u) { return vec4(uintBitsToFloat(u.x), uintBitsToFloat(u.y), uintBitsToFloat(u.z), uintBitsToFloat(u.w)); }
Cmd cf_cmd(uint i)
{
	uint base = i * 5u;
	Cmd c;
	c.aabb = cf_bits4(cf_fetch0(base));
	c.meta = cf_fetch0(base + 1u);
	c.shape = cf_bits4(cf_fetch0(base + 2u));
	c.misc = cf_bits4(cf_fetch0(base + 3u));
	c.user = cf_bits4(cf_fetch0(base + 4u));
	return c;
}
vec4 cf_payload(uint i) { return cf_bits4(cf_fetch1(i)); }
#else
layout (std430, set = 0, binding = 0) readonly buffer cmd_buffer { Cmd cmds[]; };
layout (std430, set = 0, binding = 1) readonly buffer payload_buffer { vec4 payload[]; };
Cmd cf_cmd(uint i) { return cmds[i]; }
vec4 cf_payload(uint i) { return payload[i]; }
#endif

layout (location = 0) out vec4 v_pos_uv;
layout (location = 1) out int v_n;
layout (location = 2) out vec4 v_ab;
layout (location = 3) out vec4 v_cd;
layout (location = 4) out vec4 v_ef;
layout (location = 5) out vec4 v_gh;
layout (location = 6) out vec4 v_col;
layout (location = 7) out vec4 v_shape;
layout (location = 8) out vec4 v_blend_posH;
layout (location = 9) out vec4 v_user;
layout (location = 10) out vec4 v_uv_bounds;
layout (location = 11) out flat int v_po;

#include "sdf_core.shd"

void main()
{
	Cmd cmd = cf_cmd(uint(gl_InstanceIndex));
	vec4 user_out = cmd.user;
	uint type = cmd.meta.x;
	uint po = cmd.meta.z;
	int corner = int(in_corner + 0.5);
	vec4 P0 = cf_payload(po);
	vec4 P1 = cf_payload(po + 1u);

	// Conservative coverage inflation: radius + full stroke + aa (shape.y is the
	// pre-halved stroke).
	float pad = cmd.shape.x + cmd.shape.y * 2.0 + cmd.shape.z;

	vec2 pos = vec2(0);
	vec2 uv = vec2(0);
	vec4 col = cf_unpack_color(cmd.meta.y);
	vec4 ab = P0;
	vec4 cd = P1;
	vec4 ef = vec4(0);
	vec4 gh = vec4(0);
	vec4 uvb = vec4(0);
	float cx = (corner == 1 || corner == 2) ? 1.0 : 0.0;
	float cy = (corner == 2 || corner == 3) ? 1.0 : 0.0;

	if (type <= 1u) {
		// Sprite/text: parallelogram q0 + e1/e2, uv by corner.
		vec2 q0 = P0.xy;
		vec2 e1 = P0.zw;
		vec2 e2 = P1.xy;
		pos = q0 + e1 * cx + e2 * cy;
		uvb = cf_payload(po + 2u);
		uv = vec2(mix(uvb.x, uvb.z, cx), mix(uvb.y, uvb.w, cy));
		if (type == 1u) {
			vec4 tcol = cf_payload(po + 3u);
			uint cc = corner == 0 ? floatBitsToUint(tcol.x) : (corner == 1 ? floatBitsToUint(tcol.y) : (corner == 2 ? floatBitsToUint(tcol.z) : floatBitsToUint(tcol.w)));
			col = cf_unpack_color(cc);
		}
	} else if (type == 4u) {
		// Raw triangle: corners 0,1 map to verts, 2 and 3 both map to the third vert
		// (the quad's second triangle degenerates to zero area).
		vec2 t0 = P0.xy;
		vec2 t1 = P0.zw;
		vec2 t2 = P1.xy;
		pos = corner == 0 ? t0 : (corner == 1 ? t1 : t2);
		vec4 tcol = cf_payload(po + 2u);
		uint cc = corner == 0 ? floatBitsToUint(tcol.x) : (corner == 1 ? floatBitsToUint(tcol.y) : floatBitsToUint(tcol.z));
		col = cf_unpack_color(cc);
		user_out = corner == 0 ? cf_payload(po + 3u) : (corner == 1 ? cf_payload(po + 4u) : cf_payload(po + 5u));
	} else if (type == 2u) {
		// Box: center + rotated half-extents.
		vec2 c = P0.xy;
		vec2 he = P0.zw + vec2(pad);
		vec2 u = P1.xy;
		vec2 sx = u * he.x;
		vec2 sy = skew(u) * he.y;
		pos = c + sx * (cx * 2.0 - 1.0) + sy * (cy * 2.0 - 1.0);
	} else if (type == 3u || type == 7u) {
		// Capsule/segment (and clipped polyline body): exact OBB along the segment.
		// Degenerate (circle) becomes an axis-aligned square.
		vec2 a = P0.xy;
		vec2 b = P0.zw;
		vec2 d = b - a;
		float l = safe_len(d);
		vec2 n0 = l == 0.0 ? vec2(pad, 0.0) : d * (pad / l);
		vec2 t = skew(n0);
		pos = corner == 0 ? a - n0 + t : (corner == 1 ? b + n0 + t : (corner == 2 ? b + n0 - t : a - n0 - t));
		if (type == 7u) ef = cf_payload(po + 2u);
	} else if (type == 5u) {
		// SDF triangle: padded AABB of the three verts.
		vec2 mn = min(P0.xy, min(P0.zw, P1.xy)) - vec2(pad);
		vec2 mx = max(P0.xy, max(P0.zw, P1.xy)) + vec2(pad);
		pos = vec2(mix(mn.x, mx.x, cx), mix(mn.y, mx.y, cy));
	} else if (type == 8u) {
		// Arrow: padded AABB of the endpoints (head fits within max(r, w) inflation).
		float apad = max(P1.x, P1.y) + pad;
		vec2 mn = min(P0.xy, P0.zw) - vec2(apad);
		vec2 mx = max(P0.xy, P0.zw) + vec2(apad);
		pos = vec2(mix(mn.x, mx.x, cx), mix(mn.y, mx.y, cy));
	} else if (type == 9u) {
		// Custom shape: CPU-supplied pre-padded bounds ride in the payload; the 16
		// shape params pass through untouched via ab/cd/ef/gh.
		vec4 P4 = cf_payload(po + 4u);
		pos = vec2(mix(P4.x, P4.z, cx), mix(P4.y, P4.w, cy));
		ef = cf_payload(po + 2u);
		gh = cf_payload(po + 3u);
	} else if (type == 10u) {
		// CSG shape group: pre-padded composite bounds are the payload's first vec4;
		// the fragment stage reads the operand list from the payload directly.
		pos = vec2(mix(P0.x, P0.z, cx), mix(P0.y, P0.w, cy));
	} else if (type == 11u) {
		// Curve glyph: outline-box parallelogram (BL origin + x/y edges), inflated by
		// stroke + aa along the edge directions.
		vec2 q0g = P0.xy;
		vec2 e1g = P0.zw;
		vec2 e2g = P1.xy;
		float gpad = cmd.shape.y * 2.0 + cmd.shape.z;
		vec2 gn1 = safe_norm(e1g, safe_len(e1g)) * gpad;
		vec2 gn2 = safe_norm(e2g, safe_len(e2g)) * gpad;
		pos = q0g + e1g * cx + e2g * cy + gn1 * (cx * 2.0 - 1.0) + gn2 * (cy * 2.0 - 1.0);
	} else {
		// Polygon: padded AABB of n verts.
		vec4 P2 = cf_payload(po + 2u);
		vec4 P3 = cf_payload(po + 3u);
		int n = int(cmd.misc.y);
		vec2 pts_[8];
		pts_[0] = P0.xy; pts_[1] = P0.zw; pts_[2] = P1.xy; pts_[3] = P1.zw;
		pts_[4] = P2.xy; pts_[5] = P2.zw; pts_[6] = P3.xy; pts_[7] = P3.zw;
		vec2 mn = pts_[0];
		vec2 mx = pts_[0];
		for (int i = 1; i < n; ++i) {
			mn = min(mn, pts_[i]);
			mx = max(mx, pts_[i]);
		}
		mn -= vec2(pad);
		mx += vec2(pad);
		pos = vec2(mix(mn.x, mx.x, cx), mix(mn.y, mx.y, cy));
		ef = P2;
		gh = P3;
	}

	// Camera from the matrix palette (forward mvp).
	vec4 f0 = cf_payload(cmd.meta.w);
	vec4 f1 = cf_payload(cmd.meta.w + 1u);
	vec2 posH = f0.xy * pos.x + f0.zw * pos.y + f1.xy;

	v_pos_uv = vec4(pos, uv);
	v_n = int(cmd.misc.y);
	v_ab = ab;
	v_cd = cd;
	v_ef = ef;
	v_gh = gh;
	v_col = col;
	v_shape = vec4(cmd.shape.x, cmd.shape.y, cmd.shape.z, float(type));
	v_blend_posH = vec4(cmd.shape.w, cmd.misc.x, posH);
	v_user = user_out;
	v_uv_bounds = uvb;
	v_po = int(po);
	gl_Position = vec4(posH, 0, 1);
}
)";





//--------------------------------------------------------------------------------------------------
// Tiled renderer GPU binning compute shaders (phase 3).
//
// Five dispatches per batch: zero -> count -> scan -> scatter -> sort. The CPU only
// uploads compact commands + payload; the GPU walks each command's pixel AABB at tile
// granularity, with an SDF distance cull at tile centers for shape types (this is where
// the old CPU-side tight OBB fitting effectively moved to). The sort pass restores
// painter's order within each tile (atomic scatter is nondeterministic, but typically
// nearly-sorted, where insertion sort is ~linear) and then applies opaque-cover culling:
// the latest opaque command whose interior covers the whole tile becomes the tile's new
// list start, dropping everything painted beneath it.

// Shared text for the binning shaders: command/readonly-buffer declarations, uniform
// block, and conservative tile coverage tests.
#define CF_TILE_BIN_COMMON \
"struct Cmd\n" \
"{\n" \
"	vec4 aabb;\n" \
"	uvec4 meta;\n" \
"	vec4 shape;\n" \
"	vec4 misc;\n" \
"	vec4 user;\n" \
"};\n" \
"layout (std430, set = 0, binding = 0) readonly buffer cmd_buffer { Cmd cmds[]; };\n" \
"layout (std430, set = 0, binding = 1) readonly buffer payload_buffer { vec4 payload[]; };\n" \
"layout (set = 2, binding = 0) uniform uniform_block {\n" \
"	vec2 u_canvas_wh;\n" \
"	int u_cmd_count;\n" \
"	int u_tiles_x;\n" \
"	int u_tiles_y;\n" \
"	int u_tile_px;\n" \
"};\n" \
"vec4 cf_payload(uint i) { return payload[i]; }\n" \
"#include \"sdf_core.shd\"\n" \
"#include \"custom_shapes.shd\"\n" \
"#include \"csg.shd\"\n" \
"vec2 pts[8];\n" \
"float cmd_distance_at(Cmd cmd, int tx, int ty, out float r_tile)\n" \
"{\n" \
"	vec2 center_px = vec2((float(tx) + 0.5) * float(u_tile_px), (float(ty) + 0.5) * float(u_tile_px));\n" \
"	vec2 ndc = vec2(center_px.x * (2.0 / u_canvas_wh.x) - 1.0, 1.0 - center_px.y * (2.0 / u_canvas_wh.y));\n" \
"	vec4 im0 = payload[cmd.meta.w + 2u];\n" \
"	vec4 im1 = payload[cmd.meta.w + 3u];\n" \
"	vec2 p = im0.xy * ndc.x + im0.zw * ndc.y + im1.xy;\n" \
"	vec2 dwx = im0.xy * (2.0 / u_canvas_wh.x);\n" \
"	vec2 dwy = im0.zw * (2.0 / u_canvas_wh.y);\n" \
"	r_tile = 0.5 * float(u_tile_px) * (length(dwx) + length(dwy));\n" \
"	uint po = cmd.meta.z;\n" \
"	vec4 P0 = payload[po];\n" \
"	vec4 P1 = payload[po + 1u];\n" \
"	uint type = cmd.meta.x;\n" \
"	float d;\n" \
"	if (type == 2u) {\n" \
"		d = distance_box(p, P0.xy, P0.zw, P1.xy);\n" \
"	} else if (type == 3u || type == 7u) {\n" \
"		d = distance_segment(p, P0.xy, P0.zw);\n" \
"		d = min(d, distance_segment(p, P0.zw, type == 7u ? P0.xy : P1.xy));\n" \
"	} else if (type == 5u) {\n" \
"		d = distance_triangle(p, P0.xy, P0.zw, P1.xy);\n" \
"	} else if (type == 8u) {\n" \
"		d = distance_arrow(p, P0.xy, P0.zw, P1.x, P1.y);\n" \
"	} else if (type == 9u) {\n" \
"		vec4 P2 = payload[po + 2u];\n" \
"		vec4 P3 = payload[po + 3u];\n" \
"		ShapeParams sp;\n" \
"		sp.a = P0.xy; sp.b = P0.zw; sp.c = P1.xy; sp.d = P1.zw;\n" \
"		sp.e = P2.xy; sp.f = P2.zw; sp.g = P3.xy; sp.h = P3.zw;\n" \
"		sp.attributes = cmd.user;\n" \
"		d = custom_sdf(int(cmd.misc.y), p, sp);\n" \
"	} else if (type == 10u) {\n" \
"		d = csg_distance(po, int(cmd.misc.y), p, cmd.user);\n" \
"	} else {\n" \
"		vec4 P2 = payload[po + 2u];\n" \
"		vec4 P3 = payload[po + 3u];\n" \
"		pts[0] = P0.xy; pts[1] = P0.zw; pts[2] = P1.xy; pts[3] = P1.zw;\n" \
"		pts[4] = P2.xy; pts[5] = P2.zw; pts[6] = P3.xy; pts[7] = P3.zw;\n" \
"		d = distance_polygon(p, pts, int(cmd.misc.y));\n" \
"	}\n" \
"	return d;\n" \
"}\n" \
"bool tile_touched(Cmd cmd, int tx, int ty)\n" \
"{\n" \
"	uint type = cmd.meta.x;\n" \
"	if (type <= 1u || type == 4u || type == 11u) return true; /* Sprites/text/tris/glyphs: AABB only. */\n" \
"	float r_tile;\n" \
"	float d = cmd_distance_at(cmd, tx, ty, r_tile);\n" \
"	return d - cmd.shape.x - cmd.shape.y - cmd.shape.z - r_tile <= 0.0;\n" \
"}\n"

static const char* s_tile_zero_cs = R"(
layout (std430, set = 1, binding = 0) buffer headers_buffer { uvec2 tiles[]; };
layout (set = 2, binding = 0) uniform uniform_block {
	int u_tile_count;
};
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main()
{
	uint i = gl_GlobalInvocationID.x;
	if (i < uint(u_tile_count)) {
		tiles[i] = uvec2(0u);
	}
}
)";

static const char* s_tile_count_cs =
CF_TILE_BIN_COMMON
R"(
layout (std430, set = 1, binding = 0) buffer headers_buffer { uvec2 tiles[]; };
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

// One thread per (command, tile row): dispatch y covers the batch's tallest command so
// screen-sized commands don't serialize a single thread over their whole tile rect.
void main()
{
	uint ci = gl_GlobalInvocationID.x;
	if (ci >= uint(u_cmd_count)) return;
	Cmd cmd = cmds[ci];
	if (cmd.aabb.z < 0.0 || cmd.aabb.w < 0.0 || cmd.aabb.x >= float(u_tiles_x * u_tile_px) || cmd.aabb.y >= float(u_tiles_y * u_tile_px)) return;
	int tx0 = clamp(int(floor(cmd.aabb.x / float(u_tile_px))), 0, u_tiles_x - 1);
	int ty0 = clamp(int(floor(cmd.aabb.y / float(u_tile_px))), 0, u_tiles_y - 1);
	int tx1 = clamp(int(floor(cmd.aabb.z / float(u_tile_px))), 0, u_tiles_x - 1);
	int ty1 = clamp(int(floor(cmd.aabb.w / float(u_tile_px))), 0, u_tiles_y - 1);
	int ty = ty0 + int(gl_GlobalInvocationID.y);
	if (ty > ty1) return;
	// No SDF cull here: count only reserves space, and the list buffer is already
	// sized for full AABB rects. The gather writes the true per-tile counts, so an
	// overestimate costs nothing but slack in the list segments.
	for (int tx = tx0; tx <= tx1; ++tx) {
		atomicAdd(tiles[ty * u_tiles_x + tx].y, 1u);
	}
}
)";

static const char* s_tile_scan_cs = R"(
layout (std430, set = 1, binding = 0) buffer headers_buffer { uvec2 tiles[]; };
layout (set = 2, binding = 0) uniform uniform_block {
	int u_tile_count;
};
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

shared uint sh[256];

// Single-workgroup chunked exclusive scan. A one-lane serial loop here costs thousands
// of dependent memory round-trips; 256 lanes with a shared-memory Hillis-Steele scan
// per chunk keep it in the microseconds. All threads run the full loop structure
// uniformly (barriers require it); only the stores are guarded by bounds checks.
void main()
{
	uint tid = gl_LocalInvocationID.x;
	uint count = uint(u_tile_count);
	uint chunks = (count + 255u) / 256u;
	uint run = 0u;
	for (uint ch = 0u; ch < chunks; ++ch) {
		uint idx = ch * 256u + tid;
		uint v = idx < count ? tiles[idx].y : 0u;
		sh[tid] = v;
		barrier();
		for (uint off = 1u; off < 256u; off <<= 1u) {
			uint mine = sh[tid];
			uint add = tid >= off ? sh[tid - off] : 0u;
			barrier();
			sh[tid] = mine + add;
			barrier();
		}
		uint inclusive = sh[tid];
		if (idx < count) tiles[idx].x = run + inclusive - v;
		run += sh[255];
		barrier();
	}
}
)";

static const char* s_tile_gather_cs =
CF_TILE_BIN_COMMON
R"(
layout (std430, set = 1, binding = 0) buffer headers_buffer { uvec2 tiles[]; };
layout (std430, set = 1, binding = 1) buffer list_buffer { uint tile_list[]; };
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

// One thread per tile walks all commands in submission order and writes its own list
// segment: painter's order for free -- no atomics, no cursors, no sort (an atomic
// scatter would need a per-tile sort afterward, which goes quadratic on long lists).
// The opaque-cover cull folds into the same walk: the latest opaque SDF command whose
// filled interior covers the whole tile becomes the list's new start.
void main()
{
	uint t = gl_GlobalInvocationID.x;
	if (t >= uint(u_tiles_x * u_tiles_y)) return;
	int tx = int(t) % u_tiles_x;
	int ty = int(t) / u_tiles_x;
	float x0 = float(tx * u_tile_px);
	float y0 = float(ty * u_tile_px);
	float x1 = x0 + float(u_tile_px);
	float y1 = y0 + float(u_tile_px);
	uint base = tiles[t].x;
	uint reserved = tiles[t].y; // From the count pass; never write past our segment.
	uint n = 0u;
	// Walk commands top-down, filling the segment back-to-front (list stays in
	// ascending command order). The first opaque SDF command whose filled interior
	// covers the whole tile hides everything deeper -- stop immediately instead of
	// binning commands that would only be culled.
	for (uint k = 0u; k < uint(u_cmd_count) && n < reserved; ++k) {
		uint ci = uint(u_cmd_count) - 1u - k;
		Cmd cmd = cmds[ci];
		if (cmd.aabb.x >= x1 || cmd.aabb.y >= y1 || cmd.aabb.z < x0 || cmd.aabb.w < y0) continue;
		if (!tile_touched(cmd, tx, ty)) continue;
		tile_list[base + reserved - 1u - n] = ci;
		++n;
		if (cmd.misc.z != 0.0 && cmd.aabb.x <= x0 && cmd.aabb.y <= y0 && cmd.aabb.z >= x1 && cmd.aabb.w >= y1) {
			float r_tile;
			float d = cmd_distance_at(cmd, tx, ty, r_tile);
			// Fully inside the filled interior across the whole tile.
			if (d - cmd.shape.x + r_tile <= 0.0) break;
		}
	}
	tiles[t] = uvec2(base + reserved - n, n);
}
)";

//--------------------------------------------------------------------------------------------------
// Primary blit shader.

static const char* s_blit_vs = R"(
layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_posH;
layout (location = 2) in vec2 in_uv;
layout (location = 3) in vec4 in_params;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec2 v_pos;
layout (location = 2) out vec2 v_posH;
layout (location = 3) out vec4 v_params;

void main() {
	v_uv = in_uv;
	v_pos = in_pos;
	v_posH = in_posH;
	v_params = in_params;
	gl_Position = vec4(in_posH, 0, 1);
}
)";

static const char* s_blit_fs = R"(
layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec2 v_pos;
layout (location = 2) in vec2 v_posH;
layout (location = 3) in vec4 v_params;

layout (location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_image;

layout (set = 3, binding = 0) uniform uniform_block {
	vec2 u_texture_size;
	int u_alpha_discard;
};

#include "smooth_uv.shd"

vec2 pos;
vec2 screen_uv;

struct ShaderParams
{
	vec2 view_pos;
	vec2 uv;
	vec2 uv_min;
	vec2 uv_max;
	vec2 screen_uv;
	vec4 attributes;
};

#include "shader_stub.shd"

void main() {
	vec4 color = texture(u_image, smooth_uv(v_uv, u_texture_size));
	pos = v_pos;
	screen_uv = (v_posH + vec2(1,-1)) * 0.5 * vec2(1,-1);
	ShaderParams sp;
	sp.view_pos = pos;
	sp.uv = v_uv;
	sp.uv_min = vec2(0);
	sp.uv_max = vec2(0);
	sp.screen_uv = screen_uv;
	sp.attributes = v_params;
	vec4 c = shader(color, sp);
	if (u_alpha_discard != 0 && c.a == 0) discard;
	// Keep every varying live regardless of the user stub (see s_draw_fs -- D3D12 links
	// VS->PS by hardware register, so a sparse input signature breaks pipeline creation).
	if (u_alpha_discard == -2147483647) {
		c += vec4(v_uv, v_pos) + vec4(v_posH, 0, 0) + v_params;
	}
	result = c;
}
)";

typedef struct CF_BuiltinShaderSource {
	const char* name;
	const char* vertex;
	const char* fragment;
	// The default flavor uses features GLES3/WebGL2 can't express (SSBOs); compile
	// with the CF_GLES define for the texel-fetch flavor, and skip the GLSL 300
	// transpile for the default flavor.
	bool has_gles_flavor;
} CF_BuiltinShaderSource;

static CF_ShaderCompilerFile s_builtin_includes[] = {
	{ "gamma.shd", s_gamma },
	{ "sdf_core.shd", s_sdf_core },
	{ "distance.shd", s_distance },
	{ "glyph.shd", s_glyph },
	{ "smooth_uv.shd", s_smooth_uv },
	{ "blend.shd", s_blend },
	// Overridden at runtime with generated content once cf_make_custom_shape() has
	// registered user shapes (see cf_compile_shader_to_bytecode_internal).
	{ "custom_shapes.shd", s_custom_shapes_stub },
	{ "csg.shd", s_csg },
};

static CF_BuiltinShaderSource s_builtin_shader_sources[] = {
	{ "s_draw", s_inst_vs, s_draw_fs, true },
	{ "s_basic", s_basic_vs, s_basic_fs },
	{ "s_backbuffer", s_backbuffer_vs, s_backbuffer_fs },
	{ "s_blit", s_blit_vs, s_blit_fs },
	{ "s_tile", s_tile_vs, s_tile_fs, true },
};

typedef struct CF_BuiltinComputeShaderSource {
	const char* name;
	const char* source;
} CF_BuiltinComputeShaderSource;

// Tiled renderer GPU binning shaders. Compute shaders never target GLES.
static CF_BuiltinComputeShaderSource s_builtin_compute_shader_sources[] = {
	{ "s_tile_zero", s_tile_zero_cs },
	{ "s_tile_count", s_tile_count_cs },
	{ "s_tile_scan", s_tile_scan_cs },
	{ "s_tile_gather", s_tile_gather_cs },
};

#endif
