#include "internal/cute_graphics_internal.h"
#include "internal/cute_app_internal.h"
#include "internal/cute_alloc_internal.h"

#include "cute_shader/builtin_shaders.h"

#ifndef CF_RUNTIME_SHADER_COMPILATION
#	include "data/builtin_shaders_bytecode.h"
#endif

#include <cute_graphics.h>

#include <spirv_cross_c.h>

#ifdef CF_EMSCRIPTEN
#	include <GLES3/gl3.h>
#	include <GLES3/gl2ext.h>
#else
#	include <glad/glad.h>
#endif

#define CF_POLL_OPENGL_ERROR() \
	do { \
		if (g_ctx.debug) { cf_gles_poll_error(__FILE__, __LINE__); } \
	} while (0)

struct CF_GL_PixelFormatInfo
{
	CF_PixelFormat format;
	GLenum internal_fmt;
	GLenum upload_fmt;
	GLenum upload_type;
	uint32_t caps;
	bool has_alpha;
	bool is_depth;
	bool has_stencil;
	bool is_integer;
	const char* required_extension;
};

struct CF_GL_TextureInternal
{
	int w = 0, h = 0;
	GLuint id = 0;
	GLenum internal_fmt = GL_NONE;
	GLenum upload_fmt   = GL_NONE;
	GLenum upload_type  = GL_NONE;
	bool has_mips = false;
	GLint min_filter = GL_LINEAR;
	GLint mag_filter = GL_LINEAR;
	GLint wrap_u = GL_REPEAT, wrap_v = GL_REPEAT;
};

struct CF_GL_CanvasInternal
{
	int w, h;
	GLuint fbo;
	GLuint color;
	GLuint depth;

	CF_Texture cf_color;
};

struct CF_GL_Buffer
{
	GLuint id = 0;
	int size = 0;
	int stride = 0;
	int count = 0;
};

struct CF_GL_MeshInternal
{
	GLuint vao = 0;
	CF_GL_Buffer vbo;
	CF_GL_Buffer ibo;
	CF_GL_Buffer instance;
	int index_count = 0;

	int attribute_count = 0;
	CF_VertexAttribute attributes[CF_MESH_MAX_VERTEX_ATTRIBUTES];
};

struct CF_GL_ShaderInternal
{
	GLuint prog = 0;
	GLuint ubo = 0;
	GLuint ubo_index = GL_INVALID_INDEX;
	GLuint ubo_binding = 0; // binding point

	// lazy texture bindings by name
	struct TexBinding { const char* name; GLint loc; GLint unit; };
	Cute::Array<TexBinding> fs_textures;
};

struct Vertex
{
	float x, y;
	float u, v;
};

struct CF_GL_Rect
{
	int x, y, w, h;
};

struct CF_GL_RenderState
{
	CF_GL_Rect viewport;
	bool scissor_enabled;
	CF_GL_Rect scissor;
	CF_Color blend_constants;
	GLint stencil_reference;
};

enum
{
	CF_GL_FMT_CAP_SAMPLE = 0x1,
	CF_GL_FMT_CAP_LINEAR = 0x2,
	CF_GL_FMT_CAP_COLOR = 0x4,
	CF_GL_FMT_CAP_ALPHA = 0x8,
	CF_GL_FMT_CAP_MSAA = 0x10,
	CF_GL_FMT_CAP_DEPTH = 0x20,
	CF_GL_FMT_CAP_STENCIL = 0x40,
};

#ifdef CF_EMSCRIPTEN
#define GL_BGRA GL_BGRA_EXT
// These are not available in WebGL
#define GL_R16_SNORM GL_NONE
#define GL_RG16_SNORM GL_NONE
#define GL_RGBA16_SNORM GL_NONE
#endif

static CF_GL_PixelFormatInfo g_gl_pixel_formats[] =
{
	{ CF_PIXEL_FORMAT_A8_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8_UNORM, GL_R8, GL_RED, GL_UNSIGNED_BYTE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8G8_UNORM, GL_RG8, GL_RG, GL_UNSIGNED_BYTE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8G8B8A8_UNORM, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16B16A16_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R10G10B10A2_UNORM, GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_B5G6R5_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_B5G5R5A1_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_B4G4R4A4_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_B8G8R8A8_UNORM, GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE, 0, true, false, false, false, "GL_EXT_texture_format_BGRA8888" },
	{ CF_PIXEL_FORMAT_BC1_RGBA_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC2_RGBA_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC3_RGBA_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC4_R_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC5_RG_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC7_RGBA_UNORM, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC6H_RGB_FLOAT, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC6H_RGB_UFLOAT, GL_NONE, GL_NONE, GL_NONE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8_SNORM, GL_R8_SNORM, GL_RED, GL_BYTE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8G8_SNORM, GL_RG8_SNORM, GL_RG, GL_BYTE, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8G8B8A8_SNORM, GL_RGBA8_SNORM, GL_RGBA, GL_BYTE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16_SNORM, GL_R16_SNORM, GL_RED, GL_SHORT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16_SNORM, GL_RG16_SNORM, GL_RG, GL_SHORT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16B16A16_SNORM, GL_RGBA16_SNORM, GL_RGBA, GL_SHORT, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16_FLOAT, GL_R16F, GL_RED, GL_HALF_FLOAT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16_FLOAT, GL_RG16F, GL_RG, GL_HALF_FLOAT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R32_FLOAT, GL_R32F, GL_RED, GL_FLOAT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R32G32_FLOAT, GL_RG32F, GL_RG, GL_FLOAT, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R32G32B32A32_FLOAT, GL_RGBA32F, GL_RGBA, GL_FLOAT, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R11G11B10_UFLOAT, GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV, 0, false, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_R8_UINT, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8G8_UINT, GL_RG8UI, GL_RG_INTEGER, GL_UNSIGNED_BYTE, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8G8B8A8_UINT, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, 0, true, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16_UINT, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16G16_UINT, GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_SHORT, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16G16B16A16_UINT, GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, 0, true, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8_INT, GL_R8I, GL_RED_INTEGER, GL_BYTE, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8G8_INT, GL_RG8I, GL_RG_INTEGER, GL_BYTE, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8G8B8A8_INT, GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE, 0, true, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16_INT, GL_R16I, GL_RED_INTEGER, GL_SHORT, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16G16_INT, GL_RG16I, GL_RG_INTEGER, GL_SHORT, 0, false, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R16G16B16A16_INT, GL_RGBA16I, GL_RGBA_INTEGER, GL_SHORT, 0, true, false, false, true, NULL },
	{ CF_PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB, GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC1_RGBA_UNORM_SRGB, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC2_RGBA_UNORM_SRGB, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC3_RGBA_UNORM_SRGB, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_BC7_RGBA_UNORM_SRGB, GL_NONE, GL_NONE, GL_NONE, 0, true, false, false, false, NULL },
	{ CF_PIXEL_FORMAT_D16_UNORM, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0, false, true, false, false, NULL },
	{ CF_PIXEL_FORMAT_D24_UNORM, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0, false, true, false, false, NULL },
	{ CF_PIXEL_FORMAT_D32_FLOAT, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, 0, false, true, false, false, NULL },
	{ CF_PIXEL_FORMAT_D24_UNORM_S8_UINT, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0, false, true, true, false, NULL },
#if defined(GL_DEPTH32F_STENCIL8)
	{ CF_PIXEL_FORMAT_D32_FLOAT_S8_UINT, GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 0, false, true, true, false, NULL },
#else
	{ CF_PIXEL_FORMAT_D32_FLOAT_S8_UINT, GL_NONE, GL_NONE, GL_NONE, 0, false, true, true, false, NULL },
#endif
};

static struct
{
	bool debug;
	SDL_GLContext gl_ctx;
	SDL_Window* window;
	spvc_context spvc;
	CF_Mesh backbuffer_quad;
	CF_Shader backbuffer_shader;
	CF_Material backbuffer_material;

	CF_GL_RenderState target_state;
	CF_GL_RenderState current_state;
	GLuint fbo;
	CF_MaterialInternal* material;
	CF_GL_MeshInternal* mesh;
} g_ctx = { };

static void cf_gles_poll_error(const char* file, int line)
{
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		fprintf(stderr, "%s:%d: GL error: 0x%x\n", file, line, err);
	}
}

static CF_GL_RenderState s_default_state()
{
	CF_GL_RenderState state = { };
	state.viewport = { 0, 0, app->w, app->h };
	state.scissor_enabled = false;
	state.scissor = { 0, 0, app->w, app->h };
	state.stencil_reference = 0;
	state.blend_constants = { 0.0, 0.0, 0.0, 0.0 };
	return state;
}

static void s_apply_state()
{
	CF_GL_RenderState* target = &g_ctx.target_state;
	CF_GL_RenderState* current = &g_ctx.current_state;

	if (
		target->viewport.x != current->viewport.x
		||
		target->viewport.y != current->viewport.y
		||
		target->viewport.w != current->viewport.w
		||
		target->viewport.h != current->viewport.h
	) {
		glViewport(
			target->viewport.x,
			target->viewport.y,
			target->viewport.w,
			target->viewport.h
		);
	}

	if (target->scissor_enabled != current->scissor_enabled)
	{
		if (target->scissor_enabled) {
			glEnable(GL_SCISSOR_TEST);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	}

	if (
		target->scissor_enabled
		&& (
			target->scissor.x != current->scissor.x
			||
			target->scissor.y != current->scissor.y
			||
			target->scissor.w != current->scissor.w
			||
			target->scissor.h != current->scissor.h
		)
	) {
		glScissor(
			target->scissor.x,
			target->scissor.y,
			target->scissor.w,
			target->scissor.h
		);
	}

	if (target->stencil_reference != current->stencil_reference) {
		glStencilFuncSeparate(
			GL_FRONT_AND_BACK,
			GL_ALWAYS,
			target->stencil_reference,
			0xFF
		);
	}

	if (
		target->blend_constants.r != current->blend_constants.r
		||
		target->blend_constants.g != current->blend_constants.g
		||
		target->blend_constants.b != current->blend_constants.b
		||
		target->blend_constants.a != current->blend_constants.a
	) {
		glBlendColor(
			current->blend_constants.r,
			current->blend_constants.g,
			current->blend_constants.b,
			current->blend_constants.a
		);
	}

	*current = *target;

	CF_POLL_OPENGL_ERROR();
}

static void s_bind_framebuffer(GLuint fbo)
{
	if (g_ctx.fbo != fbo)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		g_ctx.fbo = fbo;
	}
}

static void s_quad(float x, float y, float sx, float sy, Vertex quad[6])
{
	quad[0].x = -0.5f; quad[0].y =  0.5f; quad[0].u = 0; quad[0].v = 0;
	quad[1].x =  0.5f; quad[1].y = -0.5f; quad[1].u = 1; quad[1].v = 1;
	quad[2].x =  0.5f; quad[2].y =  0.5f; quad[2].u = 1; quad[2].v = 0;

	quad[3].x = -0.5f; quad[3].y =  0.5f; quad[3].u = 0; quad[3].v = 0;
	quad[4].x = -0.5f; quad[4].y = -0.5f; quad[4].u = 0; quad[4].v = 1;
	quad[5].x =  0.5f; quad[5].y = -0.5f; quad[5].u = 1; quad[5].v = 1;

	for (int i = 0; i < 6; ++i) {
		quad[i].x = quad[i].x * sx + x;
		quad[i].y = quad[i].y * sy + y;
	}
}

static bool cf_gles_has_extension(const char* name)
{
	if (!name) return true;
	GLint count = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	for (GLint i = 0; i < count; ++i) {
		const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, (GLuint)i);
		if (ext && CF_STRCMP(ext, name) == 0) return true;
	}
	return false;
}

static CF_GL_PixelFormatInfo* cf_gles_find_pixel_format_info(CF_PixelFormat format)
{
	for (size_t i = 0; i < CF_ARRAY_SIZE(g_gl_pixel_formats); ++i) {
		if (g_gl_pixel_formats[i].format == format) return &g_gl_pixel_formats[i];
	}
	return NULL;
}

static void cf_gles_load_format_caps()
{
	static bool s_caps_initialized = false;
	if (s_caps_initialized) return;
	s_caps_initialized = true;

	for (size_t i = 0; i < CF_ARRAY_SIZE(g_gl_pixel_formats); ++i) {
		CF_GL_PixelFormatInfo& info = g_gl_pixel_formats[i];
		info.caps = 0;

		if (info.internal_fmt == GL_NONE) continue;
		if (!cf_gles_has_extension(info.required_extension)) continue;

		// GL 3.3 core does not support querying these properties.
		// Mark as unsupported by default.
		// You can hardcode a conservative set of assumptions here if desired.

		// Example conservative assumptions:
		if (!info.is_integer) info.caps |= CF_GL_FMT_CAP_SAMPLE; // can sample
		if (!info.is_integer) info.caps |= CF_GL_FMT_CAP_LINEAR; // linear filter on normalized formats
		if (!info.is_depth) info.caps |= CF_GL_FMT_CAP_COLOR; // color attachment support on non-depth formats

		if (info.has_alpha)   info.caps |= CF_GL_FMT_CAP_ALPHA;
		if (info.is_depth)    info.caps |= CF_GL_FMT_CAP_DEPTH;
		if (info.has_stencil) info.caps |= CF_GL_FMT_CAP_STENCIL;

		// Dont set MSAA capability (GL_SAMPLES query not available in 3.3 core).
		// If you want MSAA support, youll need to test by actually creating
		// a multisampled renderbuffer and checking for errors.
	}
}

static char* s_transpile(const CF_ShaderBytecode* bytecode)
{
	if (!bytecode->content || !bytecode->size) return NULL;

	spvc_context context = g_ctx.spvc;

	spvc_parsed_ir ir = NULL;
	spvc_result result = spvc_context_parse_spirv(
		context,
		(const uint32_t*)bytecode->content,
		(size_t)(bytecode->size / sizeof(uint32_t)),
		&ir
	);
	if (result != SPVC_SUCCESS) {
		spvc_context_release_allocations(context);
		return NULL;
	}

	spvc_compiler compiler = NULL;
	if (spvc_context_create_compiler(context, SPVC_BACKEND_GLSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler) != SPVC_SUCCESS) {
		spvc_context_release_allocations(context);
		return NULL;
	}

	// --- Ensure integer varyings are flat (required by GLSL ES) -----------------
	{
		// Determine execution model (Vertex vs Fragment) of this module's entry point.
		const spvc_entry_point* eps = NULL;
		size_t ep_count = 0;
		SpvExecutionModel_ exec_model = SpvExecutionModelVertex; // default safe guess
		if (spvc_compiler_get_entry_points(compiler, &eps, &ep_count) == SPVC_SUCCESS && ep_count > 0) {
			exec_model = eps[0].execution_model;
		}

		spvc_resources res = NULL;
		if (spvc_compiler_create_shader_resources(compiler, &res) == SPVC_SUCCESS) {
			// Helper: mark variables flat if base type is (u)int (covers ivec*/uvec* as well).
			auto decorate_list_flat_if_integer = [&](const spvc_reflected_resource* list, size_t count) {
				for (size_t i = 0; i < count; ++i) {
					spvc_type type = spvc_compiler_get_type_handle(compiler, list[i].type_id);
					spvc_basetype bt = spvc_type_get_basetype(type);
					if (bt == SPVC_BASETYPE_INT32 || bt == SPVC_BASETYPE_UINT32) {
						// Avoid re-marking if already flat (optional).
						if (!spvc_compiler_has_decoration(compiler, list[i].id, SpvDecorationFlat)) {
							spvc_compiler_set_decoration(compiler, list[i].id, SpvDecorationFlat, 1);
						}
					}
				}
			};

			// 1) Stage outputs (VS varyings): always ok to set flat here for integer types.
			const spvc_reflected_resource* outs = NULL; size_t out_count = 0;
			if (spvc_resources_get_resource_list_for_type(res, SPVC_RESOURCE_TYPE_STAGE_OUTPUT, &outs, &out_count) == SPVC_SUCCESS) {
				decorate_list_flat_if_integer(outs, out_count);
			}

			// 2) Stage inputs:
			//    - Fragment shader inputs are varyings -> must be 'flat' for integer types.
			//    - Vertex shader inputs are *attributes* -> DO NOT decorate.
			if (exec_model == SpvExecutionModelFragment) {
				const spvc_reflected_resource* ins = NULL; size_t in_count = 0;
				if (spvc_resources_get_resource_list_for_type(res, SPVC_RESOURCE_TYPE_STAGE_INPUT, &ins, &in_count) == SPVC_SUCCESS) {
					decorate_list_flat_if_integer(ins, in_count);
				}
			}
		}
	}
	// ---------------------------------------------------------------------------

	// Options for GLES 3.00
	spvc_compiler_options options = NULL;
	if (spvc_compiler_create_compiler_options(compiler, &options) == SPVC_SUCCESS) {
		spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_GLSL_VERSION, 300);
		spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_TRUE);
		spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ENABLE_420PACK_EXTENSION, SPVC_FALSE);
		spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_SEPARATE_SHADER_OBJECTS, SPVC_TRUE);
		spvc_compiler_install_compiler_options(compiler, options);
	}

	const char* source = NULL;
	result = spvc_compiler_compile(compiler, &source);

	char* output = NULL;
	if (result == SPVC_SUCCESS && source) {
		size_t len = CF_STRLEN(source);
		output = (char*)CF_ALLOC(len + 1);
		CF_MEMCPY(output, source, len + 1);
	}

	spvc_context_release_allocations(context);
	return output;
}

static GLuint s_compile_shader(GLenum stage, const char* src)
{
	GLuint s = glCreateShader(stage);
	glShaderSource(s, 1, &src, NULL);
	glCompileShader(s);
	GLint ok = GL_FALSE;
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[4096]; GLsizei len=0;
		glGetShaderInfoLog(s, sizeof(log), &len, log);
		fprintf(stderr, "GLSL compile error:\n%.*s\n", (int)len, log);
	}
	return s;
}

static GLuint s_link_program(GLuint vs, GLuint fs)
{
	GLuint p = glCreateProgram();
	glAttachShader(p, vs);
	glAttachShader(p, fs);
	glLinkProgram(p);
	GLint ok = GL_FALSE;
	glGetProgramiv(p, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[4096]; GLsizei len=0;
		glGetProgramInfoLog(p, sizeof(log), &len, log);
		fprintf(stderr, "GLSL link error:\n%.*s\n", (int)len, log);
	}
	glDetachShader(p, vs); glDetachShader(p, fs);
	glDeleteShader(vs); glDeleteShader(fs);
	CF_POLL_OPENGL_ERROR();
	return p;
}

static CF_Shader s_make_shader(const char* vs_src, const char* fs_src)
{
	CF_GL_ShaderInternal* sh = CF_NEW(CF_GL_ShaderInternal);
	GLuint vs = s_compile_shader(GL_VERTEX_SHADER,   vs_src);
	GLuint fs = s_compile_shader(GL_FRAGMENT_SHADER, fs_src);
	sh->prog = s_link_program(vs, fs);

	// Optional UBO named "uniform_block"
	sh->ubo_index = glGetUniformBlockIndex(sh->prog, "uniform_block");
	if (sh->ubo_index != GL_INVALID_INDEX) {
		glGenBuffers(1, &sh->ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, sh->ubo);
		glBufferData(GL_UNIFORM_BUFFER, 4 * 1024, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		sh->ubo_binding = 0;
		glUniformBlockBinding(sh->prog, sh->ubo_index, sh->ubo_binding);
	}

	CF_POLL_OPENGL_ERROR();
	return CF_Shader{ (uint64_t)(uintptr_t)sh };
}

CF_INLINE GLenum s_wrap(CF_Filter f)
{
	switch (f) { default:
	case CF_FILTER_NEAREST: return GL_NEAREST;
	case CF_FILTER_LINEAR:  return GL_LINEAR;
	}
}

CF_INLINE GLenum s_wrap(CF_MipFilter m, bool has_mips)
{
	if (!has_mips) return GL_NEAREST; // min filter without mips
	switch (m) { default:
	case CF_MIP_FILTER_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
	case CF_MIP_FILTER_LINEAR:  return GL_LINEAR_MIPMAP_LINEAR;
	}
}

CF_INLINE GLenum s_wrap(CF_WrapMode w)
{
	switch (w) { default:
	case CF_WRAP_MODE_CLAMP_TO_EDGE:   return GL_CLAMP_TO_EDGE;
	case CF_WRAP_MODE_REPEAT:          return GL_REPEAT;
	case CF_WRAP_MODE_MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
	}
}

CF_INLINE GLenum s_wrap(CF_PrimitiveType p)
{
	switch (p) { default:
	case CF_PRIMITIVE_TYPE_TRIANGLELIST:  return GL_TRIANGLES;
	case CF_PRIMITIVE_TYPE_TRIANGLESTRIP: return GL_TRIANGLE_STRIP;
	case CF_PRIMITIVE_TYPE_LINELIST:      return GL_LINES;
	case CF_PRIMITIVE_TYPE_LINESTRIP:     return GL_LINE_STRIP;
	}
}

CF_INLINE GLenum s_wrap(CF_CompareFunction c)
{
	switch (c) { default:
	case CF_COMPARE_FUNCTION_ALWAYS:                return GL_ALWAYS;
	case CF_COMPARE_FUNCTION_NEVER:                 return GL_NEVER;
	case CF_COMPARE_FUNCTION_LESS_THAN:             return GL_LESS;
	case CF_COMPARE_FUNCTION_EQUAL:                 return GL_EQUAL;
	case CF_COMPARE_FUNCTION_NOT_EQUAL:             return GL_NOTEQUAL;
	case CF_COMPARE_FUNCTION_LESS_THAN_OR_EQUAL:    return GL_LEQUAL;
	case CF_COMPARE_FUNCTION_GREATER_THAN:          return GL_GREATER;
	case CF_COMPARE_FUNCTION_GREATER_THAN_OR_EQUAL: return GL_GEQUAL;
	}
}

CF_INLINE GLenum s_wrap(CF_CullMode m)
{
	switch (m) { default:
	case CF_CULL_MODE_NONE:  return 0;
	case CF_CULL_MODE_FRONT: return GL_FRONT;
	case CF_CULL_MODE_BACK:  return GL_BACK;
	}
}

CF_INLINE GLenum s_wrap(CF_BlendOp op)
{
	switch (op) { default:
	case CF_BLEND_OP_ADD:              return GL_FUNC_ADD;
	case CF_BLEND_OP_SUBTRACT:         return GL_FUNC_SUBTRACT;
	case CF_BLEND_OP_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
	case CF_BLEND_OP_MIN:              return GL_MIN;
	case CF_BLEND_OP_MAX:              return GL_MAX;
	}
}

CF_INLINE GLenum s_wrap(CF_BlendFactor f)
{
	switch (f) { default:
	case CF_BLENDFACTOR_ZERO:                     return GL_ZERO;
	case CF_BLENDFACTOR_ONE:                      return GL_ONE;
	case CF_BLENDFACTOR_SRC_COLOR:                return GL_SRC_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_SRC_COLOR:      return GL_ONE_MINUS_SRC_COLOR;
	case CF_BLENDFACTOR_DST_COLOR:                return GL_DST_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_DST_COLOR:      return GL_ONE_MINUS_DST_COLOR;
	case CF_BLENDFACTOR_SRC_ALPHA:                return GL_SRC_ALPHA;
	case CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:      return GL_ONE_MINUS_SRC_ALPHA;
	case CF_BLENDFACTOR_DST_ALPHA:                return GL_DST_ALPHA;
	case CF_BLENDFACTOR_ONE_MINUS_DST_ALPHA:      return GL_ONE_MINUS_DST_ALPHA;
	case CF_BLENDFACTOR_CONSTANT_COLOR:           return GL_CONSTANT_COLOR;
	case CF_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
	case CF_BLENDFACTOR_SRC_ALPHA_SATURATE:       return GL_SRC_ALPHA_SATURATE;
	}
}

CF_Result cf_gles_init(bool debug)
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if (debug) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	}
	g_ctx.debug = debug;

	if (spvc_context_create(&g_ctx.spvc) != SPVC_SUCCESS) {
		return cf_result_error("Could not create Spirv-Cross context");
	}

	return cf_result_success();
}


void cf_gles_destroy_shader_internal(CF_Shader sh);

void cf_gles_cleanup()
{
	cf_destroy_material(g_ctx.backbuffer_material);
	cf_gles_destroy_shader_internal(g_ctx.backbuffer_shader);
	cf_destroy_mesh(g_ctx.backbuffer_quad);
	spvc_context_destroy(g_ctx.spvc);
	SDL_GL_DestroyContext(g_ctx.gl_ctx);
}

SDL_GLContext cf_gles_get_gl_context()
{
	return g_ctx.gl_ctx;
}

void cf_load_gles();

void cf_gles_attach(SDL_Window* window)
{
	g_ctx.gl_ctx = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, g_ctx.gl_ctx);
	cf_load_gles();

	g_ctx.window = window;

	// Setup the backbuffer fullscreen mesh and canvas.
	CF_VertexAttribute attrs[2] = { };
	attrs[0].name = "in_posH";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(Vertex, x);
	attrs[1].name = "in_uv";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CF_OFFSET_OF(Vertex, u);
	g_ctx.backbuffer_quad = cf_make_mesh(sizeof(Vertex) * 6, attrs, CF_ARRAY_SIZE(attrs), sizeof(Vertex));
	Vertex quad[6];
	s_quad(0, 0, 2, 2, quad);
	cf_mesh_update_vertex_data(g_ctx.backbuffer_quad, quad, 6);

#ifdef CF_RUNTIME_SHADER_COMPILATION
	g_ctx.backbuffer_shader = cf_make_shader_from_source_internal(s_backbuffer_vs, s_backbuffer_fs, NULL);
#else
	g_ctx.backbuffer_shader = cf_make_shader_from_bytecode(s_backbuffer_vs_bytecode, s_backbuffer_fs_bytecode);
#endif

	g_ctx.backbuffer_material = cf_make_material();
}

bool cf_gles_supports_msaa(int sample_count)
{
	CF_ASSERT(!"This is only implemented on SDL_Gpu backends (Metal/DX11/DX12/Vulkan).");
	return false;
}

void cf_gles_flush()
{
	glFlush();
}

void cf_gles_set_vsync(bool true_turn_on_vsync)
{
	SDL_GL_SetSwapInterval(true_turn_on_vsync ? 1 : 0);
}

void cf_gles_begin_frame()
{
}

void cf_gles_end_frame()
{
	SDL_GL_SwapWindow(g_ctx.window);
}

void cf_gles_blit_canvas(CF_Canvas canvas)
{
	// Blit onto the default framebuffer.
	s_bind_framebuffer(0);
	cf_apply_mesh(g_ctx.backbuffer_quad);
	CF_V2 u_texture_size = V2((float)app->w, (float)app->h);

	cf_material_set_texture_fs(g_ctx.backbuffer_material, "u_image", cf_canvas_get_target(canvas));
	cf_material_set_uniform_fs(g_ctx.backbuffer_material, "u_texture_size", &u_texture_size, CF_UNIFORM_TYPE_FLOAT2, 1);
	cf_apply_shader(g_ctx.backbuffer_shader, g_ctx.backbuffer_material);
	cf_draw_elements();
	cf_commit();
}

bool cf_gles_texture_supports_format(CF_PixelFormat format, CF_TextureUsageBits usage)
{
	cf_gles_load_format_caps();
	CF_GL_PixelFormatInfo* info = cf_gles_find_pixel_format_info(format);
	if (!info || info->internal_fmt == GL_NONE) return false;
		uint32_t caps = info->caps;
		if (!caps) return false;
		if (usage & (CF_TEXTURE_USAGE_GRAPHICS_STORAGE_READ_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_READ_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT)) return false;
		if ((usage & CF_TEXTURE_USAGE_SAMPLER_BIT) && !(caps & CF_GL_FMT_CAP_SAMPLE)) return false;
		if ((usage & CF_TEXTURE_USAGE_COLOR_TARGET_BIT) && !(caps & CF_GL_FMT_CAP_COLOR)) return false;
		if (usage & CF_TEXTURE_USAGE_DEPTH_STENCIL_TARGET_BIT) {
		if (!info->is_depth) return false;
		if (!(caps & CF_GL_FMT_CAP_DEPTH)) return false;
		if (info->has_stencil && !(caps & CF_GL_FMT_CAP_STENCIL)) return false;
	}
	return true;
}

bool cf_gles_query_pixel_format(CF_PixelFormat format, CF_PixelFormatOp op)
{
	cf_gles_load_format_caps();
	CF_GL_PixelFormatInfo* info = cf_gles_find_pixel_format_info(format);
	if (!info || info->internal_fmt == GL_NONE) return false;
	uint32_t caps = info->caps;
	if (!caps) return false;

	switch (op) {
	case CF_PIXELFORMAT_OP_NEAREST_FILTER:
		return (caps & CF_GL_FMT_CAP_SAMPLE) != 0;

	case CF_PIXELFORMAT_OP_BILINEAR_FILTER:
		return (caps & CF_GL_FMT_CAP_SAMPLE) && (caps & CF_GL_FMT_CAP_LINEAR);

	case CF_PIXELFORMAT_OP_RENDER_TARGET:
		return (caps & CF_GL_FMT_CAP_COLOR) != 0;

	case CF_PIXELFORMAT_OP_ALPHA_BLENDING:
		if (!(caps & CF_GL_FMT_CAP_COLOR)) return false;
		if (!(caps & CF_GL_FMT_CAP_ALPHA)) return false;
		return !info->is_integer;

	case CF_PIXELFORMAT_OP_MSAA:
		if (info->is_depth) return (caps & CF_GL_FMT_CAP_MSAA) && (caps & CF_GL_FMT_CAP_DEPTH);
		return (caps & CF_GL_FMT_CAP_MSAA) && (caps & CF_GL_FMT_CAP_COLOR);

	case CF_PIXELFORMAT_OP_DEPTH:
		return (caps & CF_GL_FMT_CAP_DEPTH) != 0;

	default:
		return false;
	}
}

static void cf_gles_apply_sampler_params(CF_GL_TextureInternal* t, const CF_TextureParams& p)
{
	cf_gles_load_format_caps();
	CF_GL_PixelFormatInfo* info = cf_gles_find_pixel_format_info(p.pixel_format);
	uint32_t caps = info ? info->caps : 0;
	t->has_mips = p.generate_mipmaps || p.mip_count > 1;
	GLenum min_filter = s_wrap(p.mip_filter, t->has_mips);
	GLenum mag_filter = s_wrap(p.filter);
	if (!(caps & CF_GL_FMT_CAP_LINEAR)) {
		min_filter = t->has_mips ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
		mag_filter = GL_NEAREST;
	}
	t->min_filter = min_filter;
	t->mag_filter = mag_filter;
	t->wrap_u = s_wrap(p.wrap_u);
	t->wrap_v = s_wrap(p.wrap_v);

	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, t->min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, t->mag_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, t->wrap_u);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t->wrap_v);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static GLuint cf_gles_make_depth_renderbuffer(const CF_TextureParams& p)
{
	cf_gles_load_format_caps();
	CF_GL_PixelFormatInfo* info = cf_gles_find_pixel_format_info(p.pixel_format);
	if (!info || info->internal_fmt == GL_NONE || !info->is_depth) return 0;
	if (!(info->caps & CF_GL_FMT_CAP_DEPTH)) return 0;
	if (info->has_stencil && !(info->caps & CF_GL_FMT_CAP_STENCIL)) return 0;

	GLuint rbo = 0;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, info->internal_fmt, p.width, p.height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	CF_POLL_OPENGL_ERROR();
	return rbo;
}

CF_Texture cf_gles_make_texture(CF_TextureParams params)
{
	if (!cf_gles_texture_supports_format(params.pixel_format, (CF_TextureUsageBits)params.usage)) {
		CF_ASSERT(!"Unsupported pixel format for GLES backend.");
		return CF_Texture{};
	}

	CF_GL_PixelFormatInfo* info = cf_gles_find_pixel_format_info(params.pixel_format);
	if (!info || info->internal_fmt == GL_NONE) return CF_Texture{};

	CF_GL_TextureInternal* t = CF_NEW(CF_GL_TextureInternal);
	t->w = params.width;
	t->h = params.height;
	t->internal_fmt = info->internal_fmt;
	t->upload_fmt   = info->upload_fmt;
	t->upload_type  = info->upload_type;
	if (!info->is_depth && (t->upload_fmt == GL_NONE || t->upload_type == GL_NONE)) {
		CF_FREE(t);
		return CF_Texture{};
	}

	glGenTextures(1, &t->id);
	if (!t->id) {
		CF_FREE(t);
		return CF_Texture{};
	}

	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexImage2D(GL_TEXTURE_2D, 0, t->internal_fmt, t->w, t->h, 0, t->upload_fmt, t->upload_type, NULL);
	cf_gles_apply_sampler_params(t, params);
	if (params.generate_mipmaps) glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	CF_POLL_OPENGL_ERROR();

	return CF_Texture{ (uint64_t)(uintptr_t)t };
}

void cf_gles_destroy_texture(CF_Texture tex)
{
	if (!tex.id) return;
	CF_GL_TextureInternal* t = (CF_GL_TextureInternal*)(uintptr_t)tex.id;
	if (t->id) glDeleteTextures(1, &t->id);
	CF_POLL_OPENGL_ERROR();
	CF_FREE(t);
}

void cf_gles_texture_update(CF_Texture tex, void* data, int /*size*/)
{
	CF_GL_TextureInternal* t = (CF_GL_TextureInternal*)(uintptr_t)tex.id;
	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t->w, t->h, t->upload_fmt, t->upload_type, data);
	if (t->has_mips) glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_texture_update_mip(CF_Texture tex, void* data, int /*size*/, int mip)
{
	CF_GL_TextureInternal* t = (CF_GL_TextureInternal*)(uintptr_t)tex.id;
	int w = cf_max(t->w >> mip, 1);
	int h = cf_max(t->h >> mip, 1);
	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexSubImage2D(GL_TEXTURE_2D, mip, 0, 0, w, h, t->upload_fmt, t->upload_type, data);
	glBindTexture(GL_TEXTURE_2D, 0);
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_generate_mipmaps(CF_Texture tex)
{
	CF_GL_TextureInternal* t = (CF_GL_TextureInternal*)(uintptr_t)tex.id;
	glBindTexture(GL_TEXTURE_2D, t->id);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	CF_POLL_OPENGL_ERROR();
}

uint64_t cf_gles_texture_handle(CF_Texture t)
{
	return ((CF_GL_TextureInternal*)t.id)->id;
}

uint64_t cf_gles_texture_binding_handle(CF_Texture t)
{
	return ((CF_GL_TextureInternal*)t.id)->id;
}

CF_Canvas cf_gles_make_canvas(CF_CanvasParams params)
{
	if (!cf_gles_texture_supports_format(params.target.pixel_format, (CF_TextureUsageBits)params.target.usage)) {
		CF_ASSERT(!"Unsupported color target format for GLES backend.");
		return CF_Canvas{};
	}
	if (params.depth_stencil_enable) {
		if (!cf_gles_texture_supports_format(params.depth_stencil_target.pixel_format, (CF_TextureUsageBits)params.depth_stencil_target.usage)) {
			CF_ASSERT(!"Unsupported depth/stencil format for GLES backend.");
			return CF_Canvas{};
		}
	}

	CF_GL_CanvasInternal* c = CF_NEW(CF_GL_CanvasInternal);
	c->w = params.target.width;
	c->h = params.target.height;

	// color
	CF_Texture color = cf_gles_make_texture(params.target);
	c->cf_color = color;
	if (!color.id) {
		CF_FREE(c);
		return CF_Canvas{};
	}
	c->color = ((CF_GL_TextureInternal*)(uintptr_t)color.id)->id;

	// depth/stencil (renderbuffer)
	if (params.depth_stencil_enable) {
		c->depth = cf_gles_make_depth_renderbuffer(params.depth_stencil_target);
		if (!c->depth) {
			cf_gles_destroy_texture(color);
			CF_FREE(c);
			return CF_Canvas{};
		}
	}

	glGenFramebuffers(1, &c->fbo);
	s_bind_framebuffer(c->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, c->color, 0);
	if (c->depth) {
		if (params.depth_stencil_target.pixel_format == CF_PIXEL_FORMAT_D24_UNORM_S8_UINT)
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, c->depth);
		else
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, c->depth);
	}
	CF_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	CF_POLL_OPENGL_ERROR();

	return CF_Canvas{ (uint64_t)(uintptr_t)c };
}

void cf_gles_destroy_canvas(CF_Canvas ch)
{
	if (!ch.id) return;
	CF_GL_CanvasInternal* c = (CF_GL_CanvasInternal*)(uintptr_t)ch.id;
	if (c->depth) glDeleteRenderbuffers(1, &c->depth);
	if (c->fbo) glDeleteFramebuffers(1, &c->fbo);
	cf_gles_destroy_texture(c->cf_color);
	CF_POLL_OPENGL_ERROR();
	CF_FREE(c);
}

void cf_gles_canvas_get_size(CF_Canvas canvas_handle, int* w, int* h)
{
	CF_GL_CanvasInternal* canvas = (CF_GL_CanvasInternal*)(uintptr_t)canvas_handle.id;
	if (canvas) {
		if (w) { *w = canvas->w; }
		if (h) { *h = canvas->h; }
	}
}

CF_Texture cf_gles_canvas_get_target(CF_Canvas ch)
{
	CF_GL_CanvasInternal* c = (CF_GL_CanvasInternal*)(uintptr_t)ch.id;
	return c->cf_color;
}

CF_Texture cf_gles_canvas_get_depth_stencil_target(CF_Canvas)
{
	// TODO: Implement
	return CF_Texture{};
}

static void s_clear_canvas()
{
	GLbitfield bits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	glClearColor(app->clear_color.r, app->clear_color.g, app->clear_color.b, app->clear_color.a);
	glClearDepthf(app->clear_depth);
	glClearStencil((GLint)app->clear_stencil);
	glClear(bits);
}

void cf_gles_clear_canvas(CF_Canvas canvas_handle)
{
	CF_GL_CanvasInternal* canvas = (CF_GL_CanvasInternal*)(uintptr_t)canvas_handle.id;
	s_bind_framebuffer(canvas->fbo);
	s_clear_canvas();
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_apply_canvas(CF_Canvas canvas_handle, bool clear)
{
	CF_GL_CanvasInternal* canvas = (CF_GL_CanvasInternal*)(uintptr_t)canvas_handle.id;
	s_bind_framebuffer(canvas->fbo);
	if (clear) { s_clear_canvas(); }

	g_ctx.target_state = s_default_state();
}

CF_Mesh cf_gles_make_mesh(int vertex_buffer_size, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride)
{
	auto* m = CF_NEW(CF_GL_MeshInternal);
	glGenVertexArrays(1, &m->vao);
	glGenBuffers(1, &m->vbo.id);

	m->vbo.size = vertex_buffer_size;
	m->vbo.stride = vertex_stride;
	m->attribute_count = cf_min(attribute_count, CF_MESH_MAX_VERTEX_ATTRIBUTES);
	for (int i = 0; i < m->attribute_count; ++i) {
		m->attributes[i] = attributes[i];
		m->attributes[i].name = sintern(attributes[i].name);
	}

	glBindVertexArray(m->vao);
	glBindBuffer(GL_ARRAY_BUFFER, m->vbo.id);
	glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, NULL, GL_DYNAMIC_DRAW);
	glBindVertexArray(0);
	CF_POLL_OPENGL_ERROR();

	return CF_Mesh{ (uint64_t)(uintptr_t)m };
}

void cf_gles_mesh_set_index_buffer(CF_Mesh mh, int index_buffer_size_in_bytes, int index_bit_count)
{
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	if (!m->ibo.id) glGenBuffers(1, &m->ibo.id);
	CF_ASSERT(index_bit_count == 16 || index_bit_count == 32);
	m->ibo.size = index_buffer_size_in_bytes;
	m->ibo.stride = index_bit_count / 8;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo.id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer_size_in_bytes, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_mesh_set_instance_buffer(CF_Mesh mh, int instance_buffer_size_in_bytes, int instance_stride)
{
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	if (!m->instance.id) glGenBuffers(1, &m->instance.id);
	m->instance.size = instance_buffer_size_in_bytes;
	m->instance.stride = instance_stride;
	glBindBuffer(GL_ARRAY_BUFFER, m->instance.id);
	glBufferData(GL_ARRAY_BUFFER, instance_buffer_size_in_bytes, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_mesh_update_vertex_data(CF_Mesh mh, void* verts, int vertex_count)
{
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	GLsizeiptr bytes = vertex_count * m->vbo.stride;
	glBindBuffer(GL_ARRAY_BUFFER, m->vbo.id);
	if (bytes > m->vbo.size) {
		glBufferData(GL_ARRAY_BUFFER, bytes, verts, GL_DYNAMIC_DRAW);
		m->vbo.size = (int)bytes;
	} else {
		glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, verts);
	}
	m->vbo.count = vertex_count;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_mesh_update_index_data(CF_Mesh mh, void* indices, int index_count)
{
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	m->index_count = index_count;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo.id);
	int stride = m->ibo.stride ? m->ibo.stride : sizeof(uint16_t);
	GLsizeiptr bytes = index_count * stride;
	if (bytes > m->ibo.size) {
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, bytes, indices, GL_DYNAMIC_DRAW);
		m->ibo.size = (int)bytes;
	} else {
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, bytes, indices);
	}
	m->ibo.count = index_count;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_mesh_update_instance_data(CF_Mesh mh, void* instances, int instance_count)
{
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	if (!m->instance.id) return;
	glBindBuffer(GL_ARRAY_BUFFER, m->instance.id);
	GLsizeiptr bytes = instance_count * m->instance.stride;
	if (bytes > m->instance.size) {
		glBufferData(GL_ARRAY_BUFFER, bytes, instances, GL_DYNAMIC_DRAW);
		m->instance.size = (int)bytes;
	} else {
		glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, instances);
	}
	m->instance.count = instance_count;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_destroy_mesh(CF_Mesh mh)
{
	if (!mh.id) return;
	auto* m = (CF_GL_MeshInternal*)(uintptr_t)mh.id;
	if (m->ibo.id) glDeleteBuffers(1, &m->ibo.id);
	if (m->vbo.id) glDeleteBuffers(1, &m->vbo.id);
	if (m->instance.id) glDeleteBuffers(1, &m->instance.id);
	if (m->vao)	glDeleteVertexArrays(1, &m->vao);
	CF_POLL_OPENGL_ERROR();
	CF_FREE(m);
}

void cf_gles_apply_mesh(CF_Mesh mesh_handle)
{
	CF_GL_MeshInternal* mesh = (CF_GL_MeshInternal*)(uintptr_t)mesh_handle.id;
	g_ctx.mesh = mesh;
}

CF_Shader cf_gles_make_shader_from_bytecode(CF_ShaderBytecode vertex_bytecode, CF_ShaderBytecode fragment_bytecode)
{
	char* vs_src = s_transpile(&vertex_bytecode);
	char* fs_src = s_transpile(&fragment_bytecode);
	if (!vs_src || !fs_src) {
		CF_FREE(vs_src);
		CF_FREE(fs_src);
		return CF_Shader{};
	}
	CF_Shader shader = s_make_shader(vs_src, fs_src);
	CF_FREE(vs_src);
	CF_FREE(fs_src);
	return shader;
}

void cf_gles_destroy_shader_internal(CF_Shader sh)
{
	if (!sh.id) return;
	auto* s = (CF_GL_ShaderInternal*)(uintptr_t)sh.id;
	if (s->ubo) glDeleteBuffers(1, &s->ubo);
	if (s->prog) glDeleteProgram(s->prog);
	CF_POLL_OPENGL_ERROR();
	CF_FREE(s);
}

static void s_upload_uniforms(CF_GL_ShaderInternal* sh, CF_MaterialInternal* mi)
{
	if (sh->ubo_index == GL_INVALID_INDEX) return;

	// Compute total size (naive pack: VS then FS, in the order they were added).
	size_t total = 0;
	for (int pass = 0; pass < 2; ++pass) {
		const auto& list = (pass == 0) ? mi->vs.uniforms : mi->fs.uniforms;
		for (int i = 0; i < list.count(); ++i) total += (size_t)list[i].size;
	}
	if (total == 0) {
		// Nothing to upload; still ensure UBO is bound to the binding point.
		glBindBufferBase(GL_UNIFORM_BUFFER, sh->ubo_binding, sh->ubo);
		return;
	}

	// Single temporary blob; manual writes.
	uint8_t* blob = (uint8_t*)CF_ALLOC(total);
	uint8_t* write = blob;
	for (int pass = 0; pass < 2; ++pass) {
		const auto& list = (pass == 0) ? mi->vs.uniforms : mi->fs.uniforms;
		for (int i = 0; i < list.count(); ++i) {
			const CF_Uniform& u = list[i];
			CF_MEMCPY(write, u.data, u.size);
			write += u.size;
		}
	}

	// Upload to UBO.
	glBindBuffer(GL_UNIFORM_BUFFER, sh->ubo);
	GLint cur = 0;
	glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &cur);
	if ((GLint)total > cur) {
		glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)total, NULL, GL_DYNAMIC_DRAW);
	}
	glBufferSubData(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)total, blob);
	glBindBufferBase(GL_UNIFORM_BUFFER, sh->ubo_binding, sh->ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	CF_POLL_OPENGL_ERROR();
	CF_FREE(blob);
}

void cf_gles_apply_shader(CF_Shader shader_handle, CF_Material material_handle)
{
	CF_GL_ShaderInternal* shader = (CF_GL_ShaderInternal*)(uintptr_t)shader_handle.id;
	CF_MaterialInternal* material = (CF_MaterialInternal*)(uintptr_t)material_handle.id;
	g_ctx.material = material;

	glUseProgram(shader->prog);

	// render state
	CF_RenderState render_state = material->state;

	// cull
	if (render_state.cull_mode == CF_CULL_MODE_NONE) {
		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
		glCullFace(s_wrap(render_state.cull_mode));
	}

	// depth
	if (render_state.depth_write_enabled || render_state.depth_compare != CF_COMPARE_FUNCTION_ALWAYS) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(s_wrap(render_state.depth_compare));
		glDepthMask(render_state.depth_write_enabled ? GL_TRUE : GL_FALSE);
	} else {
		glDisable(GL_DEPTH_TEST);
	}

	// blend
	if (render_state.blend.enabled) {
		glEnable(GL_BLEND);
		glColorMask(
			render_state.blend.write_R_enabled,
			render_state.blend.write_G_enabled,
			render_state.blend.write_B_enabled,
			render_state.blend.write_A_enabled
		);
		glBlendEquationSeparate(
			s_wrap(render_state.blend.rgb_op),
			s_wrap(render_state.blend.alpha_op)
		);
		glBlendFuncSeparate(
			s_wrap(render_state.blend.rgb_src_blend_factor),
			s_wrap(render_state.blend.rgb_dst_blend_factor),
			s_wrap(render_state.blend.alpha_src_blend_factor),
			s_wrap(render_state.blend.alpha_dst_blend_factor)
		);
	} else {
		glDisable(GL_BLEND);
		glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	}
	CF_POLL_OPENGL_ERROR();

	// uniforms
	s_upload_uniforms(shader, material);

	// textures (FS)
	GLint unit = 0;
	for (int i = 0; i < material->fs.textures.count(); ++i) {
		const char* name = material->fs.textures[i].name;
		auto* tex = (CF_GL_TextureInternal*)(uintptr_t)material->fs.textures[i].handle.id;
		if (!tex) continue;

		// TODO: cache the location
		GLint loc = glGetUniformLocation(shader->prog, name);
		if (loc >= 0) {
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_2D, tex->id);
			glUniform1i(loc, unit);
			++unit;
		}
	}
	CF_POLL_OPENGL_ERROR();

	// vertex attribs (match by name)
	CF_GL_MeshInternal* mesh = g_ctx.mesh;
	CF_ASSERT(mesh != NULL);

	// TODO: Check what is saved by the VAO
	glBindVertexArray(mesh->vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo.id);
	if (mesh->ibo.id) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo.id);
	CF_POLL_OPENGL_ERROR();

	for (int i = 0; i < mesh->attribute_count; ++i) {
		const CF_VertexAttribute* attrib = &mesh->attributes[i];
		// TODO: cache the locations
		GLint loc = glGetAttribLocation(shader->prog, attrib->name);
		if (loc < 0) continue;

		const bool per_instance = attrib->per_instance;
		CF_GL_Buffer& buf = per_instance ? mesh->instance : mesh->vbo;
		if (!buf.id) continue;

		GLenum type = GL_FLOAT; GLint comps = 4; GLboolean norm = GL_FALSE;
		switch (attrib->format) {
			case CF_VERTEX_FORMAT_FLOAT:  type=GL_FLOAT; comps=1; break;
			case CF_VERTEX_FORMAT_FLOAT2: type=GL_FLOAT; comps=2; break;
			case CF_VERTEX_FORMAT_FLOAT3: type=GL_FLOAT; comps=3; break;
			case CF_VERTEX_FORMAT_FLOAT4: type=GL_FLOAT; comps=4; break;
			case CF_VERTEX_FORMAT_INT:  type=GL_INT; comps=1; break;
			case CF_VERTEX_FORMAT_INT2: type=GL_INT; comps=2; break;
			case CF_VERTEX_FORMAT_INT3: type=GL_INT; comps=3; break;
			case CF_VERTEX_FORMAT_INT4: type=GL_INT; comps=4; break;
			case CF_VERTEX_FORMAT_UINT:  type=GL_UNSIGNED_INT; comps=1; break;
			case CF_VERTEX_FORMAT_UINT2: type=GL_UNSIGNED_INT; comps=2; break;
			case CF_VERTEX_FORMAT_UINT3: type=GL_UNSIGNED_INT; comps=3; break;
			case CF_VERTEX_FORMAT_UINT4: type=GL_UNSIGNED_INT; comps=4; break;
			case CF_VERTEX_FORMAT_BYTE4_NORM: type=GL_BYTE; comps=4; norm=GL_TRUE; break;
			case CF_VERTEX_FORMAT_UBYTE4_NORM: type=GL_UNSIGNED_BYTE; comps=4; norm=GL_TRUE; break;
			case CF_VERTEX_FORMAT_SHORT2: type=GL_SHORT; comps=2; break;
			case CF_VERTEX_FORMAT_SHORT2_NORM: type=GL_SHORT; comps=2; norm=GL_TRUE; break;
			case CF_VERTEX_FORMAT_SHORT4: type=GL_SHORT; comps=4; break;
			case CF_VERTEX_FORMAT_SHORT4_NORM: type=GL_SHORT; comps=4; norm=GL_TRUE; break;
			case CF_VERTEX_FORMAT_USHORT2: type=GL_UNSIGNED_SHORT; comps=2; break;
			case CF_VERTEX_FORMAT_USHORT2_NORM: type=GL_UNSIGNED_SHORT; comps=2; norm=GL_TRUE; break;
			case CF_VERTEX_FORMAT_USHORT4: type=GL_UNSIGNED_SHORT; comps=4; break;
			case CF_VERTEX_FORMAT_USHORT4_NORM: type=GL_UNSIGNED_SHORT; comps=4; norm=GL_TRUE; break;
			case CF_VERTEX_FORMAT_HALF2: type=GL_HALF_FLOAT; comps=2; break;
			case CF_VERTEX_FORMAT_HALF4: type=GL_HALF_FLOAT; comps=4; break;
			default: break;
		}
		glBindBuffer(GL_ARRAY_BUFFER, buf.id);
		glEnableVertexAttribArray((GLuint)loc);
		if (type == GL_INT) {
			glVertexAttribIPointer((GLuint)loc, comps, type, buf.stride, (const void*)(intptr_t)attrib->offset);
		} else {
			glVertexAttribPointer((GLuint)loc, comps, type, norm, buf.stride, (const void*)(intptr_t)attrib->offset);
		}
		glVertexAttribDivisor((GLuint)loc, per_instance ? 1 : 0);
	}
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_draw_elements()
{
	CF_GL_MeshInternal* mesh = g_ctx.mesh;
	CF_MaterialInternal* material = g_ctx.material;
	CF_ASSERT(mesh != NULL);
	CF_ASSERT(material != NULL);

	s_apply_state();

	GLenum prim = s_wrap(material->state.primitive_type);
	int instance_count = (mesh->instance.id && mesh->instance.count > 0) ? mesh->instance.count : 0;

	if (mesh->ibo.id && mesh->index_count > 0) {
		GLenum elem = (mesh->ibo.stride == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
		if (instance_count > 0) {
			glDrawElementsInstanced(prim, mesh->index_count, elem, NULL, instance_count);
		} else {
			glDrawElements(prim, mesh->index_count, elem, NULL);
		}
	} else if (mesh->vbo.count > 0) {
		if (instance_count > 0) {
			glDrawArraysInstanced(prim, 0, mesh->vbo.count, instance_count);
		} else {
			glDrawArrays(prim, 0, mesh->vbo.count);
		}
	}

	CF_POLL_OPENGL_ERROR();
	++app->draw_call_count;
}

void cf_gles_commit()
{
	g_ctx.target_state = s_default_state();
}

void cf_gles_apply_viewport(int x, int y, int w, int h)
{
	g_ctx.target_state = { x, y, w, h };
}

void cf_gles_apply_scissor(int x, int y, int w, int h)
{
	g_ctx.target_state.scissor_enabled = true;
	g_ctx.target_state.scissor = { x, y, w, h };
}

void cf_gles_apply_stencil_reference(int reference)
{
	g_ctx.target_state.stencil_reference = reference;
}

void cf_gles_apply_blend_constants(float r, float g, float b, float a)
{
	g_ctx.target_state.blend_constants = { r, g, b, a };
}
