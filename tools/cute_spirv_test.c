/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info

	Unit tests for cute_spirv.h (the GLSL -> SPIR-V compiler).

	Every successfully compiled module is round-tripped through spirv-val
	(--target-env vulkan1.1) when the tool is available. Set CSPV_SPIRV_VAL to the
	spirv-val executable path, or have it on PATH; tests warn and skip validation
	otherwise (structural checks still run).
*/

#define CKIT_IMPLEMENTATION
#include "cute/ckit.h"

#define CUTE_SPIRV_IMPLEMENTATION
#include "cute/cute_spirv.h"

// Pull in the real builtin shader sources (and cute_shader.h, which is
// dependency-free).
#include "builtin_shaders.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--------------------------------------------------------------------------------------------------
// Tiny test harness.

static int g_checks = 0;
static int g_fails = 0;
static const char* g_current_test = "";

#define CHECK(cond) do { \
	g_checks++; \
	if (!(cond)) { \
		g_fails++; \
		printf("FAIL %s (%s:%d): %s\n", g_current_test, __FILE__, __LINE__, #cond); \
	} \
} while (0)

#define CHECK_MSG(cond, msg) do { \
	g_checks++; \
	if (!(cond)) { \
		g_fails++; \
		printf("FAIL %s (%s:%d): %s -- %s\n", g_current_test, __FILE__, __LINE__, #cond, (msg) ? (msg) : "(null)"); \
	} \
} while (0)

#define TEST(fn) do { g_current_test = #fn; fn(); } while (0)

//--------------------------------------------------------------------------------------------------
// spirv-val integration.

static bool g_has_spirv_val = false;
static char* g_spirv_val_path; // ckit strings.
static char* g_temp_spv;

static void detect_spirv_val(void)
{
	const char* env = getenv("CSPV_SPIRV_VAL");
	g_spirv_val_path = smake(env ? env : "spirv-val");
	// Double-quoted wrapping avoids cmd.exe's outer-quote stripping.
	char* cmd = sfmake("\"\"%s\" --version > nul 2>&1\"", g_spirv_val_path);
	g_has_spirv_val = system(cmd) == 0;
	sfree(cmd);
	if (!g_has_spirv_val) {
		printf("WARNING: spirv-val not found (set CSPV_SPIRV_VAL); skipping validation.\n");
	}
	const char* tmp = getenv("TEMP");
	g_temp_spv = sfmake("%s\\cute_spirv_test.spv", tmp ? tmp : ".");
}

// Returns true if the module passes spirv-val (or validation is unavailable).
static bool validate_spirv(const uint32_t* words, size_t count)
{
	if (!g_has_spirv_val) return true;
	FILE* f = fopen(g_temp_spv, "wb");
	if (!f) return false;
	fwrite(words, sizeof(uint32_t), count, f);
	fclose(f);
	char* cmd = sfmake("\"\"%s\" --target-env vulkan1.1 \"%s\"\"", g_spirv_val_path, g_temp_spv);
	bool ok = system(cmd) == 0;
	sfree(cmd);
	return ok;
}

//--------------------------------------------------------------------------------------------------
// Compile helpers.

static void expect_ok_ex(CSPV_Stage stage, const char* src, const CSPV_Options* opts)
{
	CSPV_Result r = cspv_compile_ex(src, stage, opts);
	CHECK_MSG(r.success, r.error_message);
	if (r.success) {
		CHECK(r.word_count > 5);
		CHECK(r.spirv[0] == 0x07230203u); // Magic.
		CHECK(r.spirv[1] == 0x00010300u); // SPIR-V 1.3.
		CHECK(r.spirv[3] > 0);            // Id bound.
		bool valid = validate_spirv(r.spirv, r.word_count);
		if (!valid) {
			// Dump the failing shader source for debugging.
			printf("--- spirv-val failed for shader: ---\n%s\n", src);
		}
		CHECK(valid);
	}
	cspv_free(&r);
}

static void expect_ok(CSPV_Stage stage, const char* src)
{
	expect_ok_ex(stage, src, NULL);
}

static void expect_err_ex(CSPV_Stage stage, const char* src, const char* expect_substr, const CSPV_Options* opts)
{
	CSPV_Result r = cspv_compile_ex(src, stage, opts);
	CHECK_MSG(!r.success, "expected a compile error but compilation succeeded");
	if (!r.success) {
		CHECK_MSG(r.error_message && strstr(r.error_message, expect_substr) != NULL, r.error_message);
	}
	cspv_free(&r);
}

static void expect_err(CSPV_Stage stage, const char* src, const char* expect_substr)
{
	expect_err_ex(stage, src, expect_substr, NULL);
}

//--------------------------------------------------------------------------------------------------
// Preprocessor tests.

static const char* test_include_resolve(const char* path, void* user)
{
	(void)user;
	if (sequ(path, "util.shd")) return "float util_one() { return 1.0; }\n";
	if (sequ(path, "nested.shd")) return "#include \"util.shd\"\nfloat two() { return util_one() + 1.0; }\n";
	if (sequ(path, "bad.shd")) return "float broken() {\n\treturn oops;\n}\n";
	return NULL;
}

static void test_preprocessor(void)
{
	// Object-like and function-like macros.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"#define COLOR vec4(1, 0, 0, 1)\n"
		"#define SCALE(x, s) ((x) * (s))\n"
		"layout(location = 0) out vec4 result;\n"
		"void main() { result = SCALE(COLOR, 0.5); }\n");
	// Macro in macro + self-reference guard.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"#define A B\n"
		"#define B 2.0\n"
		"#define REC REC\n"
		"layout(location = 0) out vec4 result;\n"
		"void main() { float REC = 3.0; result = vec4(A + REC); }\n");
	// #undef: X is usable as a plain identifier afterwards.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"#define X vec4\n"
		"#undef X\n"
		"layout(location = 0) out vec4 result;\n"
		"void main() { float X = 2.0; result = vec4(X); }\n");
	// Conditionals.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"#define FEATURE 1\n"
		"layout(location = 0) out vec4 result;\n"
		"#if FEATURE && defined(FEATURE) && !defined(MISSING)\n"
		"void main() { result = vec4(1); }\n"
		"#else\n"
		"this is not even glsl\n"
		"#endif\n");
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"#ifdef MISSING\n"
		"garbage {\n"
		"#elif 2 + 2 == 4\n"
		"void main() { result = vec4(1); }\n"
		"#else\n"
		"more garbage\n"
		"#endif\n");
	// Nested conditionals inside an inactive region stay skipped.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"#ifdef MISSING\n"
		"#ifdef ALSO_MISSING\n"
		"junk\n"
		"#endif\n"
		"junk\n"
		"#endif\n"
		"void main() { result = vec4(1); }\n");
	// Line continuation in a #define.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"#define LONG(x) \\\n"
		"	((x) + 1.0)\n"
		"layout(location = 0) out vec4 result;\n"
		"void main() { result = vec4(LONG(2.0)); }\n");
	// #version/#extension/#pragma ignored.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"#version 450\n"
		"#extension GL_ARB_whatever : enable\n"
		"#pragma something\n"
		"layout(location = 0) out vec4 result;\n"
		"void main() { result = vec4(1); }\n");

	// Includes with automatic include guards (double-include would redefine).
	CSPV_Options opts;
	memset(&opts, 0, sizeof(opts));
	opts.include_resolve = test_include_resolve;
	expect_ok_ex(CSPV_STAGE_FRAGMENT,
		"#include \"util.shd\"\n"
		"#include \"nested.shd\"\n"
		"#include \"util.shd\"\n"
		"layout(location = 0) out vec4 result;\n"
		"void main() { result = vec4(two()); }\n", &opts);

	// Builtin defines from options.
	CSPV_Define defines[2];
	defines[0].name = "CF_FRAGMENT";
	defines[0].value = NULL;
	defines[1].name = "HALF";
	defines[1].value = "0.5";
	opts.num_defines = 2;
	opts.defines = defines;
	expect_ok_ex(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"#ifdef CF_FRAGMENT\n"
		"void main() { result = vec4(HALF); }\n"
		"#endif\n", &opts);
	opts.num_defines = 0;

	// Errors: location reporting inside an include names the included file.
	expect_err_ex(CSPV_STAGE_FRAGMENT,
		"#include \"bad.shd\"\n"
		"void main() { }\n", "bad.shd:2", &opts);
	// The display_name hook renames a file in error messages (the mechanism CF
	// uses to report draw-shader-stub errors under the user's shader path).
	{
		CSPV_Options rename_opts = opts;
		struct local {
			static const char* rename(const char* path, void* user) {
				(void)user;
				return sequ(path, "bad.shd") ? "my_actual_shader.shd" : NULL;
			}
		};
		rename_opts.display_name = local::rename;
		expect_err_ex(CSPV_STAGE_FRAGMENT,
			"#include \"bad.shd\"\n"
			"void main() { }\n", "my_actual_shader.shd:2", &rename_opts);
	}
	expect_err_ex(CSPV_STAGE_FRAGMENT, "#include \"missing.shd\"\nvoid main() { }\n", "cannot open include file", &opts);
	expect_err(CSPV_STAGE_FRAGMENT, "#include \"x.shd\"\nvoid main() { }\n", "no include resolver");
	expect_err(CSPV_STAGE_FRAGMENT, "#garbage\nvoid main() { }\n", "unknown preprocessor directive");
	expect_err(CSPV_STAGE_FRAGMENT, "#ifdef X\nvoid main() { }\n", "missing #endif");
	expect_err(CSPV_STAGE_FRAGMENT, "#endif\nvoid main() { }\n", "#endif without #if");
	expect_err(CSPV_STAGE_FRAGMENT, "#ifdef X\n#else\n#else\n#endif\nvoid main() { }\n", "duplicate #else");
	expect_err(CSPV_STAGE_FRAGMENT,
		"#define M(a, b) a + b\n"
		"void main() { float x = M(1.0); }\n", "expects 2 argument");
}

//--------------------------------------------------------------------------------------------------
// Standalone test shaders exercising the builtin utility-module code (gamma/blend
// bodies wrapped with a main; the real modules compile verbatim via the corpus test).

static const char* s_gamma_fs =
"layout(location = 0) out vec4 result;\n"
"vec4 gamma(vec4 c)\n"
"{\n"
"	return vec4(pow(abs(c.rgb), vec3(1.0/2.2)), c.a);\n"
"}\n"
"vec4 de_gamma(vec4 c)\n"
"{\n"
"	return vec4(pow(abs(c.rgb), vec3(2.2)), c.a);\n"
"}\n"
"void main()\n"
"{\n"
"	result = gamma(de_gamma(vec4(0.5)));\n"
"}\n";

// blend.shd (rgb<->hsv + overload pair), exercises ternaries with vector arms,
// mod-with-splat, user-function overloads.
static const char* s_blend_fs =
"layout(location = 0) out vec4 result;\n"
"vec3 rgb_to_hsv(vec3 c)\n"
"{\n"
"	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);\n"
"	vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);\n"
"	vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);\n"
"	float d = q.x - min(q.w, q.y);\n"
"	float e = 1.0e-10;\n"
"	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);\n"
"}\n"
"vec3 hsv_to_rgb(vec3 c)\n"
"{\n"
"	vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0);\n"
"	rgb = rgb*rgb*(3.0-2.0*rgb);\n"
"	return c.z * mix(vec3(1.0), rgb, c.y);\n"
"}\n"
"vec3 hue(vec3 base, vec3 tint)\n"
"{\n"
"	base = rgb_to_hsv(base);\n"
"	tint = rgb_to_hsv(tint);\n"
"	return hsv_to_rgb(vec3(tint.r, base.gb));\n"
"}\n"
"vec4 hue(vec4 base, vec4 tint)\n"
"{\n"
"	return vec4(hue(base.rgb, tint.rgb), base.a);\n"
"}\n"
"void main()\n"
"{\n"
"	result = hue(vec4(0.25), vec4(0.75));\n"
"}\n";

//--------------------------------------------------------------------------------------------------
// Full builtin-shader corpus, mirroring how cute_graphics.cpp invokes the compiler:
// no defines, the builtin include modules, and shader_stub.shd resolving to either
// the default stub or an injected user shader (passed via opts.user).

static const char* corpus_resolve(const char* path, void* user)
{
	if (user && sequ(path, "shader_stub.shd")) return (const char*)user;
	for (int i = 0; i < (int)(sizeof(s_builtin_includes) / sizeof(s_builtin_includes[0])); i++) {
		if (sequ(path, s_builtin_includes[i].name)) return s_builtin_includes[i].content;
	}
	if (sequ(path, "shader_stub.shd")) return s_shader_stub;
	return NULL;
}

static void test_corpus_builtin_full(void)
{
	CSPV_Options opts;
	memset(&opts, 0, sizeof(opts));
	opts.include_resolve = corpus_resolve;

	// cute_graphics injects the payload storage buffer's binding slot (see
	// cf_compute_payload_binding); mirror the default here.
	CSPV_Define payload_define;
	payload_define.name = "CF_PAYLOAD_BINDING";
	payload_define.value = "1";
	opts.num_defines = 1;
	opts.defines = &payload_define;

	// Options for the GLES texel-fetch flavor (single-source: CF_GLES selects it).
	CSPV_Define gles_defines[2];
	gles_defines[0] = payload_define;
	gles_defines[1].name = "CF_GLES";
	gles_defines[1].value = "1";
	CSPV_Options gles_opts = opts;
	gles_opts.num_defines = 2;
	gles_opts.defines = gles_defines;

	// Every builtin vertex/fragment pair, verbatim, with the default stub -- and the
	// CF_GLES flavor for the pairs that have one.
	for (int i = 0; i < (int)(sizeof(s_builtin_shader_sources) / sizeof(s_builtin_shader_sources[0])); i++) {
		expect_ok_ex(CSPV_STAGE_VERTEX, s_builtin_shader_sources[i].vertex, &opts);
		expect_ok_ex(CSPV_STAGE_FRAGMENT, s_builtin_shader_sources[i].fragment, &opts);
		if (s_builtin_shader_sources[i].has_gles_flavor) {
			expect_ok_ex(CSPV_STAGE_VERTEX, s_builtin_shader_sources[i].vertex, &gles_opts);
			expect_ok_ex(CSPV_STAGE_FRAGMENT, s_builtin_shader_sources[i].fragment, &gles_opts);
		}
	}

	// Every builtin compute shader (the tiled path's binning pipeline).
	for (int i = 0; i < (int)(sizeof(s_builtin_compute_shader_sources) / sizeof(s_builtin_compute_shader_sources[0])); i++) {
		expect_ok_ex(CSPV_STAGE_COMPUTE, s_builtin_compute_shader_sources[i].source, &opts);
	}

	// A custom draw shader spliced in through the stub, like cf_make_draw_shader.
	opts.user = (void*)
		"vec4 shader(vec4 color, ShaderParams params)\n"
		"{\n"
		"	vec4 hued = vec4(hue(color.rgb, params.attributes.rgb), color.a);\n"
		"	float d = distance_aabb(params.uv - 0.5, vec2(0.25));\n"
		"	return mix(gamma(hued), color, smoothstep(0.0, params.attributes.a, d));\n"
		"}\n";
	expect_ok_ex(CSPV_STAGE_FRAGMENT, s_draw_fs, &opts);

	// Blit shaders only include smooth_uv.shd, so their user stubs are simpler.
	opts.user = (void*)
		"vec4 shader(vec4 color, ShaderParams params)\n"
		"{\n"
		"	return color * params.attributes;\n"
		"}\n";
	expect_ok_ex(CSPV_STAGE_FRAGMENT, s_blit_fs, &opts);
}

//--------------------------------------------------------------------------------------------------
// Corpus tests.

static void test_corpus_basic(void)
{
	expect_ok(CSPV_STAGE_FRAGMENT, s_gamma_fs);
	expect_ok(CSPV_STAGE_FRAGMENT, s_blend_fs);
}

//--------------------------------------------------------------------------------------------------
// Feature tests (positive).

#define FS_MAIN(body) \
	"layout(location = 0) out vec4 result;\n" \
	"void main()\n{\n" body "\n}\n"

static void test_expressions(void)
{
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("result = vec4(1.0 + 2.0 * 3.0 - 4.0 / 2.0);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("float x = 5.0; x += 1.0; x -= 2.0; x *= 3.0; x /= 4.0; result = vec4(x);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("int i = 7 % 3; uint u = 8u >> 2u; int j = i << 1; result = vec4(float(i + j), float(u), 0, 1);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("int i = 6 & 3; int j = 6 | 3; int k = 6 ^ 3; int m = ~6; result = vec4(float(i + j + k + m));"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("float x = 1.0; x++; ++x; x--; --x; result = vec4(x);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("vec2 v = vec2(1, 2); v.x = 3.0; v.y += 1.0; result = vec4(v, 0, 1);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("vec4 v = vec4(1, 2, 3, 4); v.xy = v.zw; v.wzyx = v; result = v;"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("vec3 v = vec3(1); float f = v.z; result = vec4(v.xy, f, v.r);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("bool b = 1.0 < 2.0 && 3 > 2 || !(4u >= 5u); result = vec4(b ? 1.0 : 0.0);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("float x = 2.0; float y = x == 2.0 ? 1.0 : 0.0; result = vec4(y != 0.0 ? y : x);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("vec3 a = vec3(1); vec3 b = vec3(2); vec3 c = true ? a : b; result = vec4(c, 1);"));
	// Implicit conversions: int -> float, int -> uint, scalar-vector ops.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("float x = 1; vec2 v = vec2(1, 2) * 2.0; vec2 w = 3.0 * v; result = vec4(v + w, x, 1);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("uint u = 5; float f = u + 1.5; result = vec4(f);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("result = vec4(-1.0); result = -result; int i = -3; result.x = float(-i);"));
}

static void test_constructors(void)
{
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("result = vec4(1);"));                      // Splat with int -> float.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("result = vec4(1.0, 2.0, 3.0, 4.0);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("result = vec4(vec2(1), vec2(2));"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("result = vec4(vec3(1), 1.0);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("vec4 v = vec4(1, 2, 3, 4); vec3 s = vec3(v); vec2 t = vec2(v); result = vec4(s, t.x);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("float f = float(3); int i = int(3.7); uint u = uint(5); bool b = i > 2; result = vec4(f, float(i), float(u), b ? 1.0 : 0.0);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("ivec2 iv = ivec2(1, 2); uvec3 uv = uvec3(1u); result = vec4(float(iv.x), float(uv.y), 0, 1);"));
}

static void test_intrinsics(void)
{
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"vec3 v = vec3(0.5);\n"
		"v = pow(abs(v), vec3(2.2));\n"
		"v = mix(v, vec3(1.0), 0.5);\n"
		"v = clamp(v, 0.0, 1.0);\n"
		"v = min(max(v, vec3(0.1)), 0.9);\n"
		"float s = smoothstep(0.0, 1.0, v.x) + step(0.5, v.y) + fract(v.z) + floor(v.x) + ceil(v.y) + round(v.z) + trunc(v.x);\n"
		"s += sqrt(v.x) + inversesqrt(v.y) + exp(v.z) + log(v.x) + exp2(v.y) + log2(v.z) + sign(v.x);\n"
		"s += sin(v.x) + cos(v.y) + tan(v.z) + asin(0.5) + acos(0.5) + atan(1.0);\n"
		"s += length(v) + distance(v, vec3(0)) + dot(v, v);\n"
		"vec3 n = normalize(v);\n"
		"vec3 c = cross(n, vec3(0, 1, 0));\n"
		"vec3 r = reflect(n, c);\n"
		"s += mod(7.5, 2.0);\n"
		"result = vec4(r, s);"));
	// Derivatives (fragment only).
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("vec2 v = vec2(0.5); result = vec4(dFdx(v), dFdy(v)) + vec4(fwidth(v), 0, 0);"));
}

static void test_control_flow(void)
{
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("float x = 0.0; if (x < 1.0) { x = 2.0; } result = vec4(x);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("float x = 0.0; if (x < 1.0) x = 2.0; else x = 3.0; result = vec4(x);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"float acc = 0.0;\n"
		"for (int i = 0; i < 10; ++i) {\n"
		"	if (i == 3) continue;\n"
		"	if (i == 8) break;\n"
		"	acc += float(i);\n"
		"}\n"
		"result = vec4(acc);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"int i = 0;\n"
		"while (i < 5) { i++; }\n"
		"result = vec4(float(i));"));
	// Nested loops with break/continue targeting the correct loop.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"float acc = 0.0;\n"
		"for (int i = 0; i < 4; ++i) {\n"
		"	for (int j = 0; j < 4; ++j) {\n"
		"		if (j == 2) break;\n"
		"		acc += 1.0;\n"
		"	}\n"
		"}\n"
		"result = vec4(acc);"));
	// Statements after return go into an unreachable block.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("result = vec4(1); return; result = vec4(2);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("if (true) { discard; } result = vec4(1);"));
	// For loop with no condition (infinite + break).
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("int i = 0; for (;;) { i++; if (i > 3) break; } result = vec4(float(i));"));
	// Empty for clauses combinations.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("int i = 0; for (; i < 3;) { i++; } result = vec4(float(i));"));
}

static void test_functions(void)
{
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("result = vec4(0);") );
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"float add(float a, float b) { return a + b; }\n"
		"void main() { result = vec4(add(1.0, 2.0)); }\n");
	// Overloads by arity and by type.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"float pick(float a) { return a; }\n"
		"float pick(float a, float b) { return a + b; }\n"
		"vec2 pick(vec2 a) { return a; }\n"
		"void main() { result = vec4(pick(1.0), pick(1.0, 2.0), pick(vec2(3.0))); }\n");
	// Implicit conversion at the call site (int literal -> float param).
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"float f(float x) { return x; }\n"
		"void main() { result = vec4(f(1)); }\n");
	// Parameters are mutable.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"float f(float x) { x += 1.0; return x; }\n"
		"void main() { result = vec4(f(1.0)); }\n");
	// void function with early return.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"void nop(float x) { if (x > 0.0) return; }\n"
		"void main() { nop(1.0); result = vec4(1); }\n");
	// out/inout parameters (copy-in/copy-out), including swizzle lvalue args.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"void split(float v, out float lo, out float hi) { lo = fract(v); hi = floor(v); }\n"
		"void accum(inout vec3 sum, inout float weight, vec3 v, float w) { sum += v * w; weight += w; }\n"
		"void main() {\n"
		"	float lo; float hi;\n"
		"	split(3.75, lo, hi);\n"
		"	vec3 sum = vec3(0); float weight = 0.0;\n"
		"	accum(sum, weight, vec3(1, 2, 3), 0.5);\n"
		"	vec4 v = vec4(0);\n"
		"	split(2.5, v.x, v.y);\n"
		"	result = vec4(sum, lo + hi + weight) + v;\n"
		"}\n");
	expect_err(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"void f(out float x) { x = 1.0; }\n"
		"void main() { f(2.0); result = vec4(1); }\n", "must be assignable");
	// Constant-expression array sizes: #define arithmetic and const globals.
	expect_ok(CSPV_STAGE_COMPUTE,
		"#define TILE_W 8\n"
		"const int PAD = 2;\n"
		"layout(local_size_x = 64) in;\n"
		"layout(std430, set = 1, binding = 0) buffer b { float data[]; };\n"
		"shared vec3 s_tile[TILE_W * TILE_W];\n"
		"shared float s_edge[TILE_W + PAD];\n"
		"void main() {\n"
		"	uint i = gl_LocalInvocationIndex;\n"
		"	s_tile[i % 64u] = vec3(1);\n"
		"	s_edge[i % 10u] = 2.0;\n"
		"	barrier();\n"
		"	data[i] = s_tile[0].x + s_edge[0] + float(s_tile.length() + s_edge.length());\n"
		"}\n");
	// 'in' param qualifier and (void) param list.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"float f(in float x) { return x; }\n"
		"float g(void) { return 2.0; }\n"
		"void main() { result = vec4(f(1.0) + g()); }\n");
}

static void test_io_and_uniforms(void)
{
	// Vertex inputs/outputs + gl_Position.
	expect_ok(CSPV_STAGE_VERTEX,
		"layout(location = 0) in vec2 pos;\n"
		"layout(location = 1) in vec4 col;\n"
		"layout(location = 0) out vec4 v_col;\n"
		"void main() { v_col = col; gl_Position = vec4(pos, 0, 1); }\n");
	// Flat interpolation.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) flat in int v_n;\n"
		"layout(location = 0) out vec4 result;\n"
		"void main() { result = vec4(float(v_n)); }\n");
	// Integer fragment input is implicitly flat even without the qualifier.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) in flat int v_n;\n"
		"layout(location = 0) out vec4 result;\n"
		"void main() { result = vec4(float(v_n)); }\n");
	// Multi-member uniform block with std140 offsets.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"layout(set = 3, binding = 0) uniform uniform_block {\n"
		"	float u_a;\n"
		"	vec2 u_b;\n"
		"	vec3 u_c;\n"
		"	vec4 u_d;\n"
		"	int u_e;\n"
		"};\n"
		"void main() { result = vec4(u_a) + vec4(u_b, 0, 0) + vec4(u_c, 0) + u_d + vec4(float(u_e)); }\n");
	// Two samplers and a block.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) in vec2 uv;\n"
		"layout(location = 0) out vec4 result;\n"
		"layout(set = 2, binding = 0) uniform sampler2D u_a;\n"
		"layout(set = 2, binding = 1) uniform sampler2D u_b;\n"
		"void main() { result = texture(u_a, uv) + texture(u_b, uv); }\n");
	// texture() in a vertex shader uses an explicit LOD.
	expect_ok(CSPV_STAGE_VERTEX,
		"layout(set = 0, binding = 0) uniform sampler2D u_tex;\n"
		"void main() { gl_Position = texture(u_tex, vec2(0.5)); }\n");
}

static void test_matrices(void)
{
	// Constructors: diagonal, column vectors, flat scalars (column-major).
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN("mat4 m = mat4(1.0); result = m * vec4(1);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"mat2 m = mat2(vec2(1, 0), vec2(0, 1));\n"
		"vec2 v = m * vec2(0.5);\n"
		"result = vec4(v, 0, 1);"));
	// The sdf_core rotation idiom: mat2 from 4 scalars.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"float c = cos(0.5); float s = sin(0.5);\n"
		"mat2 rot = mat2(c, -s, s, c);\n"
		"vec2 v = rot * vec2(1, 0);\n"
		"result = vec4(v, 0, 1);"));
	// mat*mat, vec*mat, mat*scalar, scalar*mat, negate.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"mat3 a = mat3(1.0); mat3 b = mat3(2.0);\n"
		"mat3 c = a * b; c = c * 0.5; c = 2.0 * c; c = -c;\n"
		"vec3 v = vec3(1) * c; v = c * v;\n"
		"result = vec4(v, 1);"));
	// Matrix indexing: read and write columns, chained scalar access.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"mat4 m = mat4(1.0);\n"
		"m[3] = vec4(1, 2, 3, 4);\n"
		"vec4 col = m[3];\n"
		"float f = m[0][0] + m[3].y;\n"
		"result = col * f;"));
	// Dynamic index.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"mat4 m = mat4(2.0);\n"
		"vec4 acc = vec4(0);\n"
		"for (int i = 0; i < 4; ++i) acc += m[i];\n"
		"result = acc;"));
	// transpose/inverse/determinant.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"mat3 m = mat3(2.0);\n"
		"mat3 t = transpose(m);\n"
		"mat3 inv = inverse(m);\n"
		"float d = determinant(m);\n"
		"result = vec4((t * inv * vec3(1)), d);"));
	// mat4 uniform block member (std140: ColMajor + MatrixStride 16).
	expect_ok(CSPV_STAGE_VERTEX,
		"layout(location = 0) in vec2 pos;\n"
		"layout(set = 1, binding = 0) uniform uniform_block {\n"
		"	mat4 u_mvp;\n"
		"	vec2 u_offset;\n"
		"};\n"
		"void main() { gl_Position = u_mvp * vec4(pos + u_offset, 0, 1); }\n");
	// Vector indexing (read + write, dynamic).
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"vec4 v = vec4(1, 2, 3, 4);\n"
		"v[0] = 5.0;\n"
		"float sum = 0.0;\n"
		"for (int i = 0; i < 4; ++i) sum += v[i];\n"
		"int j = 2;\n"
		"v[j] = sum;\n"
		"result = v;"));
}

static void test_switch_and_do(void)
{
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"int t = 2; float x = 0.0;\n"
		"switch (t) {\n"
		"case 0: x = 1.0; break;\n"
		"case 1:\n"
		"case 2: x = 2.0; break;\n"
		"case -1: x = 3.0; break;\n"
		"default: x = 4.0; break;\n"
		"}\n"
		"result = vec4(x);"));
	// Fallthrough into the next group and no default.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"int t = 1; float x = 0.0;\n"
		"switch (t) {\n"
		"case 0: x += 1.0;\n"
		"case 1: x += 2.0;\n"
		"}\n"
		"result = vec4(x);"));
	// switch nested in a loop: continue targets the loop, break the switch.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"float x = 0.0;\n"
		"for (int i = 0; i < 4; ++i) {\n"
		"	switch (i) {\n"
		"	case 0: continue;\n"
		"	case 1: x += 1.0; break;\n"
		"	default: x += 2.0; break;\n"
		"	}\n"
		"	x += 0.5;\n"
		"}\n"
		"result = vec4(x);"));
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"int i = 0;\n"
		"do { i++; } while (i < 5);\n"
		"do { i++; if (i > 8) break; } while (true);\n"
		"result = vec4(float(i));"));
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("switch (1) { float x = 1.0; }"), "statement before first 'case'");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("switch (1.0) { case 0: break; }"), "selector must be int or uint");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("switch (1) { case x: break; }"), "integer literal");
}

static void test_intrinsics_extended(void)
{
	// Bit casts.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"uint u = floatBitsToUint(1.5);\n"
		"int i = floatBitsToInt(2.5);\n"
		"float a = uintBitsToFloat(u) + intBitsToFloat(i);\n"
		"uvec2 uv = floatBitsToUint(vec2(1, 2));\n"
		"result = vec4(a, uintBitsToFloat(uv), 1);"));
	// Pack/unpack.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"uint p = packUnorm4x8(vec4(0.5));\n"
		"vec4 v = unpackUnorm4x8(p);\n"
		"uint h = packHalf2x16(vec2(1.5, 2.5));\n"
		"vec2 hv = unpackHalf2x16(h);\n"
		"uint n = packUnorm2x16(vec2(0.25));\n"
		"vec2 nv = unpackUnorm2x16(n) + unpackSnorm2x16(packSnorm2x16(vec2(0.5)));\n"
		"result = v + vec4(hv, nv);"));
	// Integer min/max/clamp/abs/sign.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"int a = min(3, 5); int b = max(-2, 7); int c = clamp(10, 0, 4); int d = abs(-9); int e = sign(-3);\n"
		"uint ua = min(3u, 5u); uint ub = clamp(9u, 0u, 4u);\n"
		"ivec2 vi = clamp(ivec2(5), ivec2(0), ivec2(3));\n"
		"result = vec4(float(a + b + c + d + e), float(ua + ub), float(vi.x), 1);"));
	// Vector relational + all/any/not + mix-select + isnan/isinf.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"vec3 a = vec3(1, 2, 3); vec3 b = vec3(2);\n"
		"bvec3 lt = lessThan(a, b);\n"
		"bvec3 ge = greaterThanEqual(a, b);\n"
		"bvec3 eq = equal(ivec3(1), ivec3(1));\n"
		"bvec3 ne = notEqual(uvec3(1u), uvec3(2u));\n"
		"bool every = all(lt); bool some = any(ge);\n"
		"bvec3 inv = not(lt);\n"
		"vec3 m = mix(a, b, lt);\n"
		"bool nan = any(isnan(a)) || any(isinf(b));\n"
		"result = vec4(m, (every || some || inv.x || nan || eq.x || ne.x) ? 1.0 : 0.0);"));
	// atan2, textureLod, texelFetch, textureSize.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) in vec2 uv;\n"
		"layout(location = 0) out vec4 result;\n"
		"layout(set = 2, binding = 0) uniform sampler2D u_tex;\n"
		"void main() {\n"
		"	float ang = atan(uv.y, uv.x) + atan(0.5);\n"
		"	vec4 a = textureLod(u_tex, uv, 0.0);\n"
		"	vec4 b = texelFetch(u_tex, ivec2(int(uv.x) & 1023, int(uv.y) >> 10), 0);\n"
		"	ivec2 size = textureSize(u_tex, 0);\n"
		"	result = a + b + vec4(ang, float(size.x), float(size.y), 1);\n"
		"}\n");
	// texelFetch in a vertex shader (the GLES storage-buffer-emulation idiom).
	expect_ok(CSPV_STAGE_VERTEX,
		"layout(set = 0, binding = 0) uniform sampler2D u_data;\n"
		"void main() { gl_Position = texelFetch(u_data, ivec2(0, 0), 0); }\n");
}

static void test_structs_arrays_globals(void)
{
	// Struct declaration, constructor, field access (read/write), nesting in arrays.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"struct ShapeParams {\n"
		"	vec2 center;\n"
		"	float radius;\n"
		"	vec4 attributes;\n"
		"};\n"
		"float eval(ShapeParams p) { return p.radius + p.center.x; }\n"
		"void main() {\n"
		"	ShapeParams p = ShapeParams(vec2(1, 2), 3.0, vec4(4));\n"
		"	p.radius = 5.0;\n"
		"	p.center.y += 1.0;\n"
		"	result = p.attributes * eval(p);\n"
		"}\n");
	// Local arrays: declaration, constructor init, indexing, .length().
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"float weights[3] = float[](0.25, 0.5, 0.25);\n"
		"vec2 offsets[2] = vec2[2](vec2(-1, 0), vec2(1, 0));\n"
		"float sum = 0.0;\n"
		"for (int i = 0; i < weights.length(); ++i) sum += weights[i];\n"
		"offsets[1] = offsets[0] * -1.0;\n"
		"result = vec4(offsets[0] + offsets[1], sum, 1);"));
	// Struct array + dynamic index + field chain.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"struct Cmd { vec2 pos; float w; };\n"
		"void main() {\n"
		"	Cmd cmds[2] = Cmd[](Cmd(vec2(1), 2.0), Cmd(vec2(3), 4.0));\n"
		"	float acc = 0.0;\n"
		"	for (int i = 0; i < 2; ++i) acc += cmds[i].w + cmds[i].pos.x;\n"
		"	result = vec4(acc);\n"
		"}\n");
	// Global const scalars/vectors/arrays with folded initializers, plain globals.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"const float PI = 3.14159;\n"
		"const float HALF_PI = 3.14159 / 2.0;\n"
		"const int N = 4;\n"
		"const vec2 UP = vec2(0.0, 1.0);\n"
		"const float KERNEL[3] = float[](1.0 / 4.0, 0.5, 0.25);\n"
		"float g_acc;\n"
		"void main() {\n"
		"	g_acc = PI + HALF_PI + float(N) + KERNEL[0] + KERNEL[2];\n"
		"	result = vec4(UP * g_acc, 0, 1);\n"
		"}\n");
	// Errors.
	expect_err(CSPV_STAGE_FRAGMENT,
		"struct S { float x; };\nstruct S { float y; };\nvoid main() { }", "redefinition of type 'S'");
	expect_err(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"struct S { float x; };\n"
		"void main() { S s = S(1.0); result = vec4(s.y); }", "no field 'y'");
	expect_err(CSPV_STAGE_FRAGMENT, "float g = someFunc();\nvoid main() { }", "constant expression");
	expect_err(CSPV_STAGE_FRAGMENT, "const float g;\nvoid main() { }", "requires an initializer");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("float a[2][2];"), "multi-dimensional");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("float a[0];"), "must be positive");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("float a[2] = float[](1.0, 2.0, 3.0);"), "cannot convert");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("vec2 v = vec2(1); float f = v.length();"), ".length() requires an array");
}

static void test_ssbo(void)
{
	// The CF draw-shader payload idiom: readonly SSBO with a runtime vec4 array.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"layout(std430, set = 2, binding = 1) readonly buffer payload_buffer { vec4 payload[]; };\n"
		"void main() {\n"
		"	vec4 acc = vec4(0);\n"
		"	for (int i = 0; i < payload.length(); ++i) acc += payload[i];\n"
		"	result = acc;\n"
		"}\n");
	// SSBO in a vertex shader with scalar members + runtime tail, dynamic index.
	expect_ok(CSPV_STAGE_VERTEX,
		"layout(std430, set = 0, binding = 0) readonly buffer data_buffer {\n"
		"	int u_count;\n"
		"	vec2 u_scale;\n"
		"	float values[];\n"
		"};\n"
		"void main() {\n"
		"	float v = u_count > 0 ? values[u_count - 1] : 0.0;\n"
		"	gl_Position = vec4(u_scale * v, 0, 1);\n"
		"}\n");
	// Writable SSBO.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) out vec4 result;\n"
		"layout(std430, set = 2, binding = 0) buffer out_buffer { uint counts[]; };\n"
		"void main() { counts[0] = 5u; result = vec4(1); }\n");
	// Errors.
	expect_err(CSPV_STAGE_FRAGMENT,
		"layout(std430, set = 0, binding = 0) readonly buffer b { vec4 data[]; int after; };\n"
		"void main() { }", "must be the last member");
	expect_err(CSPV_STAGE_FRAGMENT,
		"layout(set = 0, binding = 0) uniform u { vec4 data[]; };\n"
		"void main() { }", "only allowed in buffer blocks");
	// OpArrayLength on a runtime array member.
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(std430, set = 0, binding = 0) readonly buffer b { vec4 data[]; };\n"
		"layout(location = 0) out vec4 result;\n"
		"void main() { result = vec4(float(data.length())); }");
	// Instance-named blocks (the hrc idiom): same member name across blocks,
	// accessed as instance.member, including runtime arrays and .length().
	expect_ok(CSPV_STAGE_COMPUTE,
		"layout(local_size_x = 64) in;\n"
		"layout(std430, set = 1, binding = 0) writeonly buffer OutRad { uvec2 data[]; } u_out_rad;\n"
		"layout(std430, set = 1, binding = 1) writeonly buffer OutTrn { uvec2 data[]; } u_out_trn;\n"
		"layout(set = 2, binding = 0) uniform Params { int u_count; float u_scale; } u_params;\n"
		"void main() {\n"
		"	uint i = gl_GlobalInvocationID.x;\n"
		"	if (int(i) >= u_params.u_count || int(i) >= u_out_rad.data.length()) return;\n"
		"	u_out_rad.data[i] = uvec2(i, uint(u_params.u_scale));\n"
		"	u_out_trn.data[i] = u_out_rad.data[i] + uvec2(1u);\n"
		"}\n");
	// textureOffset with constant offsets (the shallow_water idiom).
	expect_ok(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) in vec2 uv;\n"
		"layout(location = 0) out vec4 result;\n"
		"layout(set = 2, binding = 0) uniform sampler2D tex;\n"
		"void main() {\n"
		"	float ha = textureOffset(tex, uv, ivec2(-1, 1)).r;\n"
		"	float hb = textureOffset(tex, uv, ivec2(1, 1)).r;\n"
		"	result = vec4(ha, hb, 0, 1);\n"
		"}\n");
	expect_err(CSPV_STAGE_FRAGMENT,
		"layout(location = 0) in vec2 uv;\n"
		"layout(location = 0) out vec4 result;\n"
		"layout(set = 2, binding = 0) uniform sampler2D tex;\n"
		"void main() { ivec2 o = ivec2(1); result = textureOffset(tex, uv, o); }\n",
		"constant");
}

static void test_compute(void)
{
	// Minimal compute shader with builtins and an SSBO.
	expect_ok(CSPV_STAGE_COMPUTE,
		"layout(local_size_x = 64) in;\n"
		"layout(std430, set = 1, binding = 0) buffer out_buffer { float results[]; };\n"
		"void main() {\n"
		"	uint i = gl_GlobalInvocationID.x;\n"
		"	uint li = gl_LocalInvocationIndex + gl_LocalInvocationID.y + gl_WorkGroupID.z + gl_NumWorkGroups.x;\n"
		"	results[i] = float(i + li);\n"
		"}\n");
	// Shared memory + barrier + atomics (the prefix-scan shape).
	expect_ok(CSPV_STAGE_COMPUTE,
		"layout(local_size_x = 256) in;\n"
		"layout(std430, set = 1, binding = 0) buffer data_buffer { uint data[]; };\n"
		"shared uint temp[256];\n"
		"shared uint total;\n"
		"void main() {\n"
		"	uint i = gl_LocalInvocationIndex;\n"
		"	temp[i] = data[i];\n"
		"	barrier();\n"
		"	for (uint off = 1u; off < 256u; off <<= 1u) {\n"
		"		uint v = 0u;\n"
		"		if (i >= off) v = temp[i - off];\n"
		"		barrier();\n"
		"		temp[i] += v;\n"
		"		barrier();\n"
		"	}\n"
		"	memoryBarrierShared();\n"
		"	groupMemoryBarrier();\n"
		"	memoryBarrier();\n"
		"	memoryBarrierBuffer();\n"
		"	atomicAdd(total, temp[i]);\n"
		"	uint old = atomicMax(data[0], temp[i]);\n"
		"	old += atomicMin(data[1], 4u) + atomicExchange(data[2], 7u);\n"
		"	old += atomicCompSwap(data[3], 0u, 1u) + atomicAnd(data[4], 3u) + atomicOr(data[5], 1u) + atomicXor(data[6], 2u);\n"
		"	data[i] = temp[i] + old;\n"
		"}\n");
	// Storage images: load/store/size, float + uint formats.
	expect_ok(CSPV_STAGE_COMPUTE,
		"layout(local_size_x = 16, local_size_y = 16) in;\n"
		"layout(set = 0, binding = 0) uniform sampler2D u_input;\n"
		"layout(set = 1, binding = 0, rgba8) uniform writeonly image2D u_output;\n"
		"layout(set = 1, binding = 1, rgba32f) uniform image2D u_scratch;\n"
		"layout(set = 1, binding = 2, r32ui) uniform image2D u_flags;\n"
		"void main() {\n"
		"	ivec2 p = ivec2(gl_GlobalInvocationID.xy);\n"
		"	ivec2 size = imageSize(u_scratch);\n"
		"	if (p.x >= size.x || p.y >= size.y) return;\n"
		"	vec4 c = texelFetch(u_input, p, 0);\n"
		"	vec4 s = imageLoad(u_scratch, p);\n"
		"	uvec4 f = imageLoad(u_flags, p);\n"
		"	imageStore(u_output, p, c + s);\n"
		"	imageStore(u_flags, p, f + uvec4(1u));\n"
		"}\n");
	// Errors.
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("barrier();"), "requires a compute shader");
	expect_err(CSPV_STAGE_FRAGMENT, "shared float x;\nvoid main() { }", "require a compute shader");
	expect_err(CSPV_STAGE_COMPUTE,
		"layout(local_size_x = 8) in;\n"
		"layout(set = 0, binding = 0) uniform image2D img;\n"
		"void main() { }", "requires a format");
	expect_err(CSPV_STAGE_COMPUTE,
		"layout(local_size_x = 8) in;\n"
		"void main() { float x = 1.0; atomicAdd(x, 1.0); }", "int or uint");
}

static void test_reflection(void)
{
	// Vertex: inputs + a uniform block with std140 offsets.
	CSPV_Result r = cspv_compile(
		"layout(location = 0) in vec2 in_pos;\n"
		"layout(location = 1) in vec4 in_col;\n"
		"layout(location = 2) in ivec2 in_idx;\n"
		"layout(set = 1, binding = 0) uniform uniform_block {\n"
		"	float u_a;\n"
		"	vec2 u_b;\n"
		"	vec3 u_c;\n"
		"	mat4 u_mvp;\n"
		"	int u_d;\n"
		"};\n"
		"void main() { gl_Position = u_mvp * vec4(in_pos + u_b + vec2(in_idx), u_a + u_c.x + float(u_d), 1) * in_col; }\n",
		CSPV_STAGE_VERTEX);
	CHECK_MSG(r.success, r.error_message);
	if (r.success) {
		CSPV_Reflection* rf = &r.reflection;
		CHECK(asize(rf->inputs) == 3);
		CHECK(sequ(rf->inputs[0].name, "in_pos") && rf->inputs[0].location == 0 && rf->inputs[0].type == CSPV_TYPE_FLOAT2);
		CHECK(sequ(rf->inputs[1].name, "in_col") && rf->inputs[1].location == 1 && rf->inputs[1].type == CSPV_TYPE_FLOAT4);
		CHECK(sequ(rf->inputs[2].name, "in_idx") && rf->inputs[2].location == 2 && rf->inputs[2].type == CSPV_TYPE_SINT2);
		CHECK(asize(rf->uniform_blocks) == 1);
		CSPV_ReflectionBlock* b = &rf->uniform_blocks[0];
		CHECK(sequ(b->name, "uniform_block") && b->set == 1 && b->binding == 0);
		CHECK(b->num_members == 5 && b->first_member == 0);
		CSPV_ReflectionMember* m = rf->uniform_members;
		CHECK(sequ(m[0].name, "u_a") && m[0].type == CSPV_TYPE_FLOAT && m[0].offset == 0);
		CHECK(sequ(m[1].name, "u_b") && m[1].type == CSPV_TYPE_FLOAT2 && m[1].offset == 8);
		CHECK(sequ(m[2].name, "u_c") && m[2].type == CSPV_TYPE_FLOAT3 && m[2].offset == 16);
		CHECK(sequ(m[3].name, "u_mvp") && m[3].type == CSPV_TYPE_MAT4 && m[3].offset == 32);
		CHECK(sequ(m[4].name, "u_d") && m[4].type == CSPV_TYPE_SINT && m[4].offset == 96);
		CHECK(b->size == 112); // 96 + 4 rounded up to 16.
	}
	cspv_free(&r);

	// Fragment: samplers + SSBO. Compute: storage images.
	r = cspv_compile(
		"layout(location = 0) out vec4 result;\n"
		"layout(set = 2, binding = 0) uniform sampler2D u_image;\n"
		"layout(set = 2, binding = 1) uniform sampler2D u_lut;\n"
		"layout(std430, set = 2, binding = 2) readonly buffer payload_buffer { vec4 payload[]; };\n"
		"void main() { result = texture(u_image, payload[0].xy) + texture(u_lut, vec2(0.5)); }\n",
		CSPV_STAGE_FRAGMENT);
	CHECK_MSG(r.success, r.error_message);
	if (r.success) {
		CSPV_Reflection* rf = &r.reflection;
		CHECK(asize(rf->samplers) == 2);
		CHECK(sequ(rf->samplers[0].name, "u_image") && rf->samplers[0].set == 2 && rf->samplers[0].binding == 0);
		CHECK(sequ(rf->samplers[1].name, "u_lut") && rf->samplers[1].binding == 1);
		CHECK(asize(rf->storage_buffers) == 1);
		CHECK(sequ(rf->storage_buffers[0].name, "payload_buffer") && rf->storage_buffers[0].readonly);
		CHECK(asize(rf->uniform_blocks) == 0);
		CHECK(asize(rf->inputs) == 0); // Inputs are reflected for the vertex stage only.
	}
	cspv_free(&r);

	r = cspv_compile(
		"layout(local_size_x = 8) in;\n"
		"layout(set = 1, binding = 0, rgba8) uniform writeonly image2D u_out;\n"
		"layout(set = 0, binding = 0, rgba32f) uniform readonly image2D u_in;\n"
		"layout(std430, set = 1, binding = 1) buffer counts_buffer { uint counts[]; };\n"
		"void main() { imageStore(u_out, ivec2(0), imageLoad(u_in, ivec2(0))); counts[0] = 1u; }\n",
		CSPV_STAGE_COMPUTE);
	CHECK_MSG(r.success, r.error_message);
	if (r.success) {
		CSPV_Reflection* rf = &r.reflection;
		CHECK(asize(rf->storage_images) == 2);
		CHECK(sequ(rf->storage_images[0].name, "u_out") && !rf->storage_images[0].readonly);
		CHECK(sequ(rf->storage_images[1].name, "u_in") && rf->storage_images[1].readonly);
		CHECK(asize(rf->storage_buffers) == 1 && !rf->storage_buffers[0].readonly);
	}
	cspv_free(&r);
}

static void test_short_circuit(void)
{
	// Only the taken side of && / || / ?: evaluates. Division guards are the
	// classic pattern that breaks under eager evaluation at runtime; here we just
	// verify the compiled control flow validates (branches, not LogicalAnd).
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"float d = 0.0;\n"
		"bool safe = d != 0.0 && (1.0 / d) > 2.0;\n"
		"bool other = d == 0.0 || (1.0 / d) > 2.0;\n"
		"float v = d != 0.0 ? 1.0 / d : 0.0;\n"
		"result = vec4(safe || other ? v : 0.0);"));
	// Ternary arms with side effects: exactly one runs.
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"float x = 0.0;\n"
		"float y = true ? (x += 1.0) : (x += 2.0);\n"
		"result = vec4(x, y, 0, 1);"));
	// Type unification still works across arms (int literal vs float).
	expect_ok(CSPV_STAGE_FRAGMENT, FS_MAIN(
		"float f = true ? 1 : 2.5;\n"
		"vec3 v = false ? vec3(1) : vec3(0.5);\n"
		"result = vec4(v, f);"));
}

//--------------------------------------------------------------------------------------------------
// Fuzz smoke: mutate real shader sources and require the compiler to error
// gracefully or succeed -- never crash. Deterministic (fixed seed) so failures
// reproduce; a crash here shows up as a crashed test process.

static uint32_t g_fuzz_state = 0x12345678u;
static uint32_t fuzz_rand(void)
{
	uint32_t x = g_fuzz_state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return g_fuzz_state = x;
}

static void test_fuzz_smoke(void)
{
	const char* seeds[] = { s_draw_fs, s_inst_vs, s_tile_fs, s_blit_fs, s_gamma, s_blend, s_distance };
	CSPV_Options opts;
	memset(&opts, 0, sizeof(opts));
	opts.include_resolve = corpus_resolve;

	int compiles = 0;
	for (int iter = 0; iter < 500; ++iter) {
		const char* seed = seeds[fuzz_rand() % (sizeof(seeds) / sizeof(seeds[0]))];
		char* src = smake(seed);
		int len = (int)slen(src);
		if (len == 0) { sfree(src); continue; }
		// Apply 1-8 random single-byte mutations (printable ASCII + newline).
		int mutations = 1 + (int)(fuzz_rand() % 8);
		for (int m = 0; m < mutations; ++m) {
			int at = (int)(fuzz_rand() % (uint32_t)len);
			uint32_t r = fuzz_rand() % 100;
			if (r < 70) src[at] = (char)(32 + fuzz_rand() % 95);
			else if (r < 85) src[at] = '\n';
			else src[at] = (char)(fuzz_rand() % 256); // Occasionally arbitrary bytes.
		}
		CSPV_Result r = cspv_compile_ex(src, (CSPV_Stage)(fuzz_rand() % 2), &opts);
		if (r.success) ++compiles;
		cspv_free(&r);
		sfree(src);
	}
	CHECK(true); // Reaching here without crashing is the assertion.
	printf("fuzz smoke: 500 mutated compiles survived (%d still compiled successfully)\n", compiles);
}

//--------------------------------------------------------------------------------------------------
// Error tests.

static void test_errors_syntax(void)
{
	expect_err(CSPV_STAGE_FRAGMENT, "void main() {", "unexpected end of file");
	expect_err(CSPV_STAGE_FRAGMENT, "void main() { float x = ; }", "unexpected token");
	expect_err(CSPV_STAGE_FRAGMENT, "garbage;", "unexpected token at global scope");
	expect_err(CSPV_STAGE_FRAGMENT, "void main() { float x = 1.0 }", "expected ';'");
}

static void test_errors_semantic(void)
{
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("x = 1.0;"), "undeclared identifier 'x'");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("vec2 v = vec3(1);"), "cannot convert");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("float x = 1.0; vec2 v = vec2(1); v = x + v.xyx;"), "cannot convert");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("1.0 = 2.0;"), "not assignable");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("vec2 v = vec2(1); float f = v.z;"), "out of range");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("vec2 v = vec2(1); float f = v.q;"), "out of range");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("vec2 v = vec2(1); float f = v.hello;"), "invalid swizzle");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("float x = 1.0; float x = 2.0;"), "redefinition of 'x'");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("vec2 a = vec2(1); vec2 b = vec2(2); bool c = a < b;"), "scalar operands");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("float x = 1.0 & 2.0;"), "integer operands");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("result = bad(1.0);"), "unknown function 'bad'");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("result = vec4(texture(1.0, vec2(0)));"), "sampler2D");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("float x = clamp(1.0);"), "expects 3 argument");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("result = vec4(1, 2, 3, 4, 5);"), "components");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("result = vec4(vec2(1));"), "cannot construct");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("break;"), "break outside of a loop");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("continue;"), "continue outside of a loop");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("if (1.0) { }"), "must be bool");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("return 1.0;"), "void function cannot return a value");
}

static void test_errors_stage_and_globals(void)
{
	expect_err(CSPV_STAGE_VERTEX, "void main() { discard; }", "only valid in fragment");
	expect_err(CSPV_STAGE_FRAGMENT, "in vec2 uv;\nvoid main() { }", "layout(location");
	expect_err(CSPV_STAGE_FRAGMENT, "uniform sampler2D s;\nvoid main() { }", "layout(set");
	expect_err(CSPV_STAGE_FRAGMENT,
		"uniform uniform_block { float x; };\nvoid main() { }", "layout(set");
	expect_err(CSPV_STAGE_FRAGMENT,
		"layout(set = 0, binding = 0) uniform uniform_block { };\nvoid main() { }", "empty");
	expect_err(CSPV_STAGE_FRAGMENT, "layout(set = 0, binding = 0) uniform float u_x;\nvoid main() { }", "uniform block");
	expect_err(CSPV_STAGE_FRAGMENT, "void main(float x) { }", "main must be 'void main()'");
	expect_err(CSPV_STAGE_FRAGMENT, "float main() { return 1.0; }", "main must be 'void main()'");
	expect_err(CSPV_STAGE_FRAGMENT, "layout(location = 0) out vec4 result;\nvoid render() { result = vec4(1); }", "no 'void main()'");
	expect_err(CSPV_STAGE_FRAGMENT,
		"float f(float x) { return x; }\nfloat f(float y) { return y; }\nvoid main() { }", "redefinition of 'f'");
	expect_err(CSPV_STAGE_FRAGMENT,
		"float f(float x) { }\nvoid main() { }", "missing a return");
	expect_err(CSPV_STAGE_COMPUTE, "void main() { }", "must declare layout(local_size");
	expect_err(CSPV_STAGE_FRAGMENT, FS_MAIN("mat2 m = mat2(1); mat2 n = m + m;"), "only '*' is supported");
	expect_err(CSPV_STAGE_FRAGMENT, "layout(location = 0) out vec4 result;\nvoid main() { result.x = gl_Position.x; }", "undeclared identifier 'gl_Position'");
}

static void test_errors_recursion(void)
{
	// GLSL forbids recursion; declare-before-use makes self reference an unknown symbol.
	expect_err(CSPV_STAGE_FRAGMENT,
		"float f(float x) { return f(x - 1.0); }\nvoid main() { }", "unknown function 'f'");
}

//--------------------------------------------------------------------------------------------------

int main(void)
{
	detect_spirv_val();

	TEST(test_preprocessor);
	TEST(test_corpus_basic);
	TEST(test_corpus_builtin_full);
	TEST(test_expressions);
	TEST(test_constructors);
	TEST(test_intrinsics);
	TEST(test_control_flow);
	TEST(test_matrices);
	TEST(test_switch_and_do);
	TEST(test_intrinsics_extended);
	TEST(test_structs_arrays_globals);
	TEST(test_ssbo);
	TEST(test_compute);
	TEST(test_reflection);
	TEST(test_short_circuit);
	TEST(test_fuzz_smoke);
	TEST(test_functions);
	TEST(test_io_and_uniforms);
	TEST(test_errors_syntax);
	TEST(test_errors_semantic);
	TEST(test_errors_stage_and_globals);
	TEST(test_errors_recursion);

	printf("\n%d checks, %d failures.\n", g_checks, g_fails);
	return g_fails ? 1 : 0;
}
