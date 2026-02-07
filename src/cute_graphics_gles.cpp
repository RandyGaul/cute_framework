#include "internal/cute_graphics_internal.h"
#include "internal/cute_app_internal.h"
#include "internal/cute_alloc_internal.h"

#ifndef CF_RUNTIME_SHADER_COMPILATION
#	include "data/builtin_shaders_bytecode.h"
#endif

#include <cute_graphics.h>
#include <cute_time.h>

#ifdef CF_EMSCRIPTEN
#	include <GLES3/gl3.h>
#	include <GLES3/gl2ext.h>
#else
#	include <glad/glad.h>
#endif

#define CF_POLL_OPENGL_ERROR() \
	do { \
		if (g_ctx.debug) { s_poll_error(__FILE__, __LINE__); } \
	} while (0)

static inline GLenum s_wrap(CF_Filter f)
{
	switch (f) { default:
	case CF_FILTER_NEAREST: return GL_NEAREST;
	case CF_FILTER_LINEAR:  return GL_LINEAR;
	}
}

static inline GLenum s_wrap(CF_MipFilter m, bool has_mips)
{
	switch (m) { default:
	case CF_MIP_FILTER_NEAREST: return has_mips ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
	case CF_MIP_FILTER_LINEAR:  return has_mips ? GL_LINEAR_MIPMAP_LINEAR   : GL_LINEAR;
	}
}

static inline GLenum s_wrap(CF_WrapMode w)
{
	switch (w) { default:
	case CF_WRAP_MODE_CLAMP_TO_EDGE:   return GL_CLAMP_TO_EDGE;
	case CF_WRAP_MODE_REPEAT:          return GL_REPEAT;
	case CF_WRAP_MODE_MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
	}
}

static inline GLenum s_wrap(CF_PrimitiveType p)
{
	switch (p) { default:
	case CF_PRIMITIVE_TYPE_TRIANGLELIST:  return GL_TRIANGLES;
	case CF_PRIMITIVE_TYPE_TRIANGLESTRIP: return GL_TRIANGLE_STRIP;
	case CF_PRIMITIVE_TYPE_LINELIST:      return GL_LINES;
	case CF_PRIMITIVE_TYPE_LINESTRIP:     return GL_LINE_STRIP;
	}
}

static inline GLenum s_wrap(CF_CompareFunction c)
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

static inline GLenum s_wrap(CF_CullMode m)
{
	switch (m) { default:
	case CF_CULL_MODE_NONE:  return 0;
	case CF_CULL_MODE_FRONT: return GL_FRONT;
	case CF_CULL_MODE_BACK:  return GL_BACK;
	}
}

static inline GLenum s_wrap(CF_BlendOp op)
{
	switch (op) { default:
	case CF_BLEND_OP_ADD:              return GL_FUNC_ADD;
	case CF_BLEND_OP_SUBTRACT:         return GL_FUNC_SUBTRACT;
	case CF_BLEND_OP_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
	case CF_BLEND_OP_MIN:              return GL_MIN;
	case CF_BLEND_OP_MAX:              return GL_MAX;
	}
}

static inline GLenum s_wrap(CF_BlendFactor f)
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

static inline GLenum s_wrap(CF_StencilOp op)
{
	switch (op) { default:
	case CF_STENCIL_OP_KEEP:            return GL_KEEP;
	case CF_STENCIL_OP_ZERO:            return GL_ZERO;
	case CF_STENCIL_OP_REPLACE:         return GL_REPLACE;
	case CF_STENCIL_OP_INCREMENT_CLAMP: return GL_INCR;
	case CF_STENCIL_OP_DECREMENT_CLAMP: return GL_DECR;
	case CF_STENCIL_OP_INVERT:          return GL_INVERT;
	case CF_STENCIL_OP_INCREMENT_WRAP:  return GL_INCR_WRAP;
	case CF_STENCIL_OP_DECREMENT_WRAP:  return GL_DECR_WRAP;
	}
}

#define RING_BUFFER_CAPACITY 3

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

struct CF_GL_Slot
{
	GLuint handle = 0;
	GLsync fence = 0;
	uint32_t last_use_frame = 0;
	GLsizeiptr offset = 0;
	GLsizeiptr size = 0;
};

struct CF_GL_Ring
{
	CF_GL_Slot slots[RING_BUFFER_CAPACITY];
	int count = 0;
	int head = 0;
};

struct CF_GL_Texture
{
	int w = 0, h = 0;
	GLuint id = 0;
	GLenum internal_fmt = GL_NONE;
	GLenum upload_fmt   = GL_NONE;
	GLenum upload_type  = GL_NONE;
	bool has_mips = false;
	GLint min_filter = GL_LINEAR;
	GLint mag_filter = GL_LINEAR;
	GLint wrap_u = GL_REPEAT;
	GLint wrap_v = GL_REPEAT;
	CF_GL_Ring ring;
	int active_slot = -1;
};

struct CF_GL_Canvas
{
	int w, h;
	GLuint fbo;
	GLuint color;
	GLuint depth;
	bool has_depth;
	bool has_stencil;

	CF_Texture cf_color;
};

struct CF_GL_Buffer
{
	GLuint id = 0;
	GLenum target = GL_ARRAY_BUFFER;
	GLsizeiptr capacity = 0;
	GLintptr active_offset = 0;
	int stride = 0;
	int count = 0;
	int active_slot = -1;
	uint32_t version = 0;
	CF_GL_Ring ring;
};

struct CF_GL_Mesh
{
	CF_GL_Buffer vbo;
	CF_GL_Buffer ibo;
	CF_GL_Buffer instance;

	int attribute_count = 0;
	CF_VertexAttribute attributes[CF_MESH_MAX_VERTEX_ATTRIBUTES];
};

struct CF_GL_ShaderUniformBlock
{
	CF_ShaderUniformInfo info;
	CF_ShaderUniformMemberInfo* members;
};

struct CF_GL_ShaderInfo
{
	int num_uniform_blocks;
	GLuint ubo[CF_MAX_UNIFORM_BLOCK_COUNT];
	CF_GL_ShaderUniformBlock uniform_blocks[CF_MAX_UNIFORM_BLOCK_COUNT];
	CF_ShaderUniformMemberInfo* uniform_members;
};

struct CF_GL_TextureBinding
{
	const char* name;
	int location;
};

struct CF_GL_Shader
{
	GLuint program;

	int num_texture_bindings;
	CF_GL_TextureBinding* texture_bindings;
	int num_vs_texture_bindings;
	CF_GL_TextureBinding* vs_texture_bindings;
	CF_GL_ShaderInfo vs;
	CF_GL_ShaderInfo fs;
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
#	define GL_BGRA GL_BGRA_EXT
	// These are not available in WebGL
#	define GL_R16_SNORM GL_NONE
#	define GL_RG16_SNORM GL_NONE
#	define GL_RGBA16_SNORM GL_NONE
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
	uint32_t frame_index;

	CF_GL_RenderState target_state;
	CF_GL_RenderState current_state;
	GLuint fbo;
	CF_MaterialInternal* material;
	CF_GL_Mesh* mesh;
	CF_GL_Canvas* canvas;
	uint64_t enabled_vertex_attrib_mask;
	CF_Filter filter_override;
	bool has_filter_override;
} g_ctx = { };

static inline void s_poll_error(const char* file, int line)
{
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		fprintf(stderr, "%s:%d: GL error: 0x%x\n", file, line, err);
	}
}

static inline GLsizeiptr s_align_up(GLsizeiptr value, GLsizeiptr alignment)
{
	GLsizeiptr mask = alignment - 1;
	return (value + mask) & ~mask;
}

static inline GLenum s_poll_fence(GLsync fence)
{
	if (!fence) return GL_ALREADY_SIGNALED;
	return glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
}

static inline CF_GL_Slot* s_acquire_slot(CF_GL_Ring* ring, uint32_t frame, int* out_index)
{
	int count = ring->count;
	for (int tries = 0; tries < count; ++tries) {
		int index = (ring->head + tries) % count;
		CF_GL_Slot& slot = ring->slots[index];
		GLenum status = s_poll_fence(slot.fence);
		if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
			if (slot.fence) {
				glDeleteSync(slot.fence);
				slot.fence = 0;
			}
			ring->head = (index + 1) % count;
			slot.last_use_frame = frame;
			if (out_index) *out_index = index;
			return &slot;
		}
	}
	if (count < RING_BUFFER_CAPACITY) {
		int index = count;
		CF_GL_Slot& new_slot = ring->slots[index];
		new_slot = CF_GL_Slot();
		ring->count = count + 1;
		ring->head = (index + 1) % ring->count;
		new_slot.last_use_frame = frame;
		if (out_index) *out_index = index;
		return &new_slot;
	}
	return NULL;
}

static inline CF_GL_Slot* s_force_slot(CF_GL_Ring* ring, uint32_t frame, int* out_index)
{
	if (!ring->count) return NULL;
	int index = ring->head;
	CF_GL_Slot& slot = ring->slots[index];
	if (slot.fence) {
		// Block the CPU until the GPU is done with this slot.
		// If you're seeing this on the hot-path of a profile or flame-graph it means you're GPU bound.
#ifdef CF_EMSCRIPTEN
		while (slot.fence) {
			GLenum status = s_poll_fence(slot.fence);
			if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
				glDeleteSync(slot.fence);
				slot.fence = 0;
				break;
			}
			if (status == GL_WAIT_FAILED) {
				glDeleteSync(slot.fence);
				slot.fence = 0;
				break;
			}
			// We can't call glClientWaitSync on WebGL, so we'll try and block to simulate sort what it
			// would otherwise achieve.
			cf_sleep(0);
		}
#else
		glClientWaitSync(slot.fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
		glDeleteSync(slot.fence);
		slot.fence = 0;
#endif
	}
	slot.last_use_frame = frame;
	ring->head = (index + 1) % ring->count;
	if (out_index) *out_index = index;
	return &slot;
}

static inline CF_GL_Slot* s_acquire_or_wait(CF_GL_Ring* ring, uint32_t frame, int* out_index)
{
	CF_GL_Slot* slot = s_acquire_slot(ring, frame, out_index);
	if (slot) return slot;
	return s_force_slot(ring, frame, out_index);
}

static inline void s_set_slot_fence(CF_GL_Slot& slot)
{
	if (slot.fence) {
		glDeleteSync(slot.fence);
	}
	slot.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
#ifdef CF_EMSCRIPTEN
	glFlush();
#endif
}

static inline CF_GL_RenderState s_default_state(CF_GL_Canvas* canvas)
{
	CF_GL_RenderState state = { };
	state.viewport = { 0, 0, canvas->w, canvas->h };
	state.scissor_enabled = false;
	state.scissor = { 0, 0, canvas->w, canvas->h };
	state.stencil_reference = 0;
	state.blend_constants = { 0.0, 0.0, 0.0, 0.0 };
	return state;
}

static inline bool s_canvas_has_depth(const CF_GL_Canvas* canvas)
{
	return canvas && canvas->has_depth;
}

static inline bool s_canvas_has_stencil(const CF_GL_Canvas* canvas)
{
	return canvas && canvas->has_stencil;
}

static inline void s_apply_state()
{
	CF_GL_RenderState* target = &g_ctx.target_state;
	CF_GL_RenderState* current = &g_ctx.current_state;

	if (target->viewport.x != current->viewport.x ||
		target->viewport.y != current->viewport.y ||
		target->viewport.w != current->viewport.w ||
		target->viewport.h != current->viewport.h
	) {
		// Flip viewport vertically since OpenGL puts (0, 0) at the bottom left.
		CF_GL_Canvas* canvas = g_ctx.canvas;
		glViewport(target->viewport.x, canvas->h - target->viewport.y - target->viewport.h, target->viewport.w, target->viewport.h);
	}

	if (target->scissor_enabled != current->scissor_enabled) {
		if (target->scissor_enabled) {
			glEnable(GL_SCISSOR_TEST);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	}

	if (target->scissor_enabled &&
	   (target->scissor.x != current->scissor.x ||
		target->scissor.y != current->scissor.y ||
		target->scissor.w != current->scissor.w ||
		target->scissor.h != current->scissor.h)
	) {
		// Flip the scissor rect vertically since OpenGL puts (0, 0) at the bottom left.
		CF_GL_Canvas* canvas = g_ctx.canvas;
		glScissor(target->scissor.x, canvas->h - target->scissor.y - target->scissor.h, target->scissor.w, target->scissor.h);
	}

	if (target->stencil_reference != current->stencil_reference) {
		GLenum front_compare = GL_ALWAYS;
		GLenum back_compare = GL_ALWAYS;
		GLuint mask = 0xFF;
		if (g_ctx.material && g_ctx.material->state.stencil.enabled && s_canvas_has_stencil(g_ctx.canvas)) {
			front_compare = s_wrap(g_ctx.material->state.stencil.front.compare);
			back_compare = s_wrap(g_ctx.material->state.stencil.back.compare);
			mask = g_ctx.material->state.stencil.read_mask;
		}
		glStencilFuncSeparate(GL_FRONT, front_compare, target->stencil_reference, mask);
		glStencilFuncSeparate(GL_BACK, back_compare, target->stencil_reference, mask);
	}

	if (target->blend_constants.r != current->blend_constants.r ||
	target->blend_constants.g != current->blend_constants.g ||
	target->blend_constants.b != current->blend_constants.b ||
		target->blend_constants.a != current->blend_constants.a
	) {
		glBlendColor(
			target->blend_constants.r,
			target->blend_constants.g,
			target->blend_constants.b,
			target->blend_constants.a
		);
	}

	*current = *target;
	CF_POLL_OPENGL_ERROR();
}

static inline void s_bind_framebuffer(GLuint fbo)
{
	if (g_ctx.fbo != fbo) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		g_ctx.fbo = fbo;
	}
}

static inline bool s_has_extension(const char* name)
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

static inline CF_GL_PixelFormatInfo* s_find_pixel_format_info(CF_PixelFormat format)
{
	for (size_t i = 0; i < CF_ARRAY_SIZE(g_gl_pixel_formats); ++i) {
		if (g_gl_pixel_formats[i].format == format) return &g_gl_pixel_formats[i];
	}
	return NULL;
}

static inline void s_load_format_caps()
{
	static bool s_caps_initialized = false;
	if (s_caps_initialized) return;
	s_caps_initialized = true;

	for (size_t i = 0; i < CF_ARRAY_SIZE(g_gl_pixel_formats); ++i) {
		CF_GL_PixelFormatInfo& info = g_gl_pixel_formats[i];
		info.caps = 0;

		if (info.internal_fmt == GL_NONE) continue;
		if (!s_has_extension(info.required_extension)) continue;

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

static inline GLuint s_compile_shader(GLenum stage, const char* src, int src_len)
{
	GLuint s = glCreateShader(stage);
	glShaderSource(s, 1, &src, &src_len);
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

static inline GLuint s_make_program(CF_ShaderBytecode vs_bytecode, CF_ShaderBytecode fs_bytecode)
{
	GLuint vs = s_compile_shader(GL_VERTEX_SHADER,   vs_bytecode.glsl300_src, (int)vs_bytecode.glsl300_src_size);
	GLuint fs = s_compile_shader(GL_FRAGMENT_SHADER, fs_bytecode.glsl300_src, (int)fs_bytecode.glsl300_src_size);

	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	GLint ok = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[4096]; GLsizei len=0;
		glGetProgramInfoLog(program, sizeof(log), &len, log);
		fprintf(stderr, "GLSL link error:\n%.*s\n", (int)len, log);
	}
	glDetachShader(program, vs); glDetachShader(program, fs);
	glDeleteShader(vs); glDeleteShader(fs);

	return program;
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

	return cf_result_success();
}

void cf_gles_destroy_shader_internal(CF_Shader sh);

void cf_gles_cleanup()
{
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
}

bool cf_gles_supports_msaa(int sample_count)
{
	// GLES backend does not support MSAA.
	CF_UNUSED(sample_count);
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
	++g_ctx.frame_index;
}

void cf_gles_end_frame()
{
	SDL_GL_SwapWindow(g_ctx.window);
}

void cf_gles_blit_canvas(CF_Canvas canvas_handle)
{
	int window_width, window_height;
	SDL_GetWindowSizeInPixels(g_ctx.window, &window_width, &window_height);
	g_ctx.current_state.scissor_enabled = false;
	CF_POLL_OPENGL_ERROR();
	glDisable(GL_SCISSOR_TEST);
	CF_POLL_OPENGL_ERROR();

	CF_GL_Canvas* canvas = (CF_GL_Canvas*)(uintptr_t)canvas_handle.id;
	CF_POLL_OPENGL_ERROR();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, canvas->fbo);
	CF_POLL_OPENGL_ERROR();
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	CF_POLL_OPENGL_ERROR();
	glBlitFramebuffer(
		0, canvas->h, canvas->w, 0,
		0, 0, window_width, window_height,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR
	);
	CF_POLL_OPENGL_ERROR();
	g_ctx.fbo = 0;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool cf_gles_texture_supports_format(CF_PixelFormat format, CF_TextureUsageBits usage)
{
	s_load_format_caps();
	CF_GL_PixelFormatInfo* info = s_find_pixel_format_info(format);
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
	s_load_format_caps();
	CF_GL_PixelFormatInfo* info = s_find_pixel_format_info(format);
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

static inline void s_apply_sampler_state_to_handle(const CF_GL_Texture* t, GLuint handle)
{
	glBindTexture(GL_TEXTURE_2D, handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, t->min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, t->mag_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, t->wrap_u);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t->wrap_v);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static inline void s_apply_sampler_params(CF_GL_Texture* t, const CF_TextureParams& p)
{
	s_load_format_caps();
	CF_GL_PixelFormatInfo* info = s_find_pixel_format_info(p.pixel_format);
	uint32_t caps = info ? info->caps : 0;
	t->has_mips = p.allocate_mipmaps || p.mip_count > 1;
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

	s_apply_sampler_state_to_handle(t, t->id);
}

static inline bool s_texture_allocate_storage(CF_GL_Texture* t, CF_GL_Slot* slot)
{
	if (!slot->handle) {
		glGenTextures(1, &slot->handle);
	}
	if (!slot->handle) return false;
	glBindTexture(GL_TEXTURE_2D, slot->handle);
	glTexImage2D(GL_TEXTURE_2D, 0, t->internal_fmt, t->w, t->h, 0, t->upload_fmt, t->upload_type, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
	CF_POLL_OPENGL_ERROR();
	return true;
}

static inline CF_GL_Slot* s_prepare_buffer_slot(CF_GL_Buffer* buffer, GLsizeiptr required_bytes, int* out_index)
{
	CF_GL_Slot* slot = s_acquire_or_wait(&buffer->ring, g_ctx.frame_index, out_index);
	if (!slot) return NULL;
	if (!slot->handle) {
		glGenBuffers(1, &slot->handle);
		slot->size = 0;
		slot->offset = 0;
	}
	GLsizeiptr capacity = cf_max((int64_t)buffer->capacity, (int64_t)required_bytes);
	glBindBuffer(buffer->target, slot->handle);
	if (slot->size < capacity) {
		glBufferData(buffer->target, capacity, NULL, GL_DYNAMIC_DRAW);
		slot->size = capacity;
		slot->offset = 0;
		buffer->capacity = capacity;
	} else if (required_bytes > 0 && slot->offset + required_bytes > slot->size) {
		glBufferData(buffer->target, slot->size, NULL, GL_DYNAMIC_DRAW);
		slot->offset = 0;
	}
	return slot;
}

static inline void s_destroy_buffer(CF_GL_Buffer* buffer)
{
	for (int i = 0; i < buffer->ring.count; ++i) {
		CF_GL_Slot& slot = buffer->ring.slots[i];
		if (slot.fence) {
			glDeleteSync(slot.fence);
			slot.fence = 0;
		}
		if (slot.handle) {
			glDeleteBuffers(1, &slot.handle);
			slot.handle = 0;
		}
	}
}

static inline GLuint s_make_depth_renderbuffer(const CF_TextureParams& p)
{
	s_load_format_caps();
	CF_GL_PixelFormatInfo* info = s_find_pixel_format_info(p.pixel_format);
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
		return CF_Texture { 0ULL };
	}

	CF_GL_PixelFormatInfo* info = s_find_pixel_format_info(params.pixel_format);
	if (!info || info->internal_fmt == GL_NONE) return CF_Texture{};

	CF_GL_Texture* t = (CF_GL_Texture*)CF_CALLOC(sizeof(CF_GL_Texture));
	t->w = params.width;
	t->h = params.height;
	t->internal_fmt = info->internal_fmt;
	t->upload_fmt   = info->upload_fmt;
	t->upload_type  = info->upload_type;
	if (!info->is_depth && (t->upload_fmt == GL_NONE || t->upload_type == GL_NONE)) {
		CF_FREE(t);
		return CF_Texture { 0ULL };
	}

	int slot_index = -1;
	CF_GL_Slot* slot = s_acquire_or_wait(&t->ring, g_ctx.frame_index, &slot_index);
	if (!slot) {
		CF_FREE(t);
		return CF_Texture { 0ULL };
	}
	if (!s_texture_allocate_storage(t, slot)) {
		CF_FREE(t);
		return CF_Texture { 0ULL };
	}
	t->id = slot->handle;
	t->active_slot = slot_index;
	s_apply_sampler_params(t, params);
	if (params.allocate_mipmaps) {
		glBindTexture(GL_TEXTURE_2D, t->id);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	CF_POLL_OPENGL_ERROR();

	return CF_Texture{ (uint64_t)(uintptr_t)t };
}

void cf_gles_destroy_texture(CF_Texture tex)
{
	if (!tex.id) return;
	CF_GL_Texture* t = (CF_GL_Texture*)(uintptr_t)tex.id;
	for (int i = 0; i < t->ring.count; ++i) {
		CF_GL_Slot& slot = t->ring.slots[i];
		if (slot.fence) {
			glDeleteSync(slot.fence);
			slot.fence = 0;
		}
		if (slot.handle) {
			glDeleteTextures(1, &slot.handle);
			slot.handle = 0;
		}
	}
	CF_POLL_OPENGL_ERROR();
	CF_FREE(t);
}

void cf_gles_texture_update(CF_Texture tex, void* data, int /*size*/)
{
	CF_GL_Texture* t = (CF_GL_Texture*)(uintptr_t)tex.id;
	int slot_index = -1;
	CF_GL_Slot* slot = s_acquire_or_wait(&t->ring, g_ctx.frame_index, &slot_index);
	if (!slot) return;
	if (!s_texture_allocate_storage(t, slot)) return;
	t->id = slot->handle;
	t->active_slot = slot_index;
	s_apply_sampler_state_to_handle(t, t->id);
	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t->w, t->h, t->upload_fmt, t->upload_type, data);
	if (t->has_mips) glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_texture_update_mip(CF_Texture tex, void* data, int /*size*/, int mip)
{
	CF_GL_Texture* t = (CF_GL_Texture*)(uintptr_t)tex.id;
	int slot_index = -1;
	CF_GL_Slot* slot = s_acquire_or_wait(&t->ring, g_ctx.frame_index, &slot_index);
	if (!slot) return;
	if (!s_texture_allocate_storage(t, slot)) return;
	t->id = slot->handle;
	t->active_slot = slot_index;
	s_apply_sampler_state_to_handle(t, t->id);
	int w = cf_max(t->w >> mip, 1);
	int h = cf_max(t->h >> mip, 1);
	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexSubImage2D(GL_TEXTURE_2D, mip, 0, 0, w, h, t->upload_fmt, t->upload_type, data);
	glBindTexture(GL_TEXTURE_2D, 0);
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_generate_mipmaps(CF_Texture tex)
{
	CF_GL_Texture* t = (CF_GL_Texture*)(uintptr_t)tex.id;
	glBindTexture(GL_TEXTURE_2D, t->id);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	CF_POLL_OPENGL_ERROR();
}

uint64_t cf_gles_texture_handle(CF_Texture t)
{
	return ((CF_GL_Texture*)t.id)->id;
}

uint64_t cf_gles_texture_binding_handle(CF_Texture t)
{
	return ((CF_GL_Texture*)t.id)->id;
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

	CF_GL_Canvas* c = (CF_GL_Canvas*)CF_CALLOC(sizeof(CF_GL_Canvas));
	c->w = params.target.width;
	c->h = params.target.height;
	c->has_depth = false;
	c->has_stencil = false;

	// Color.
	CF_Texture color = cf_gles_make_texture(params.target);
	c->cf_color = color;
	if (!color.id) {
	CF_FREE(c);
	return CF_Canvas{};
	}
	c->color = ((CF_GL_Texture*)(uintptr_t)color.id)->id;

	// Septh/stencil (renderbuffer).
	if (params.depth_stencil_enable) {
	CF_GL_PixelFormatInfo* depth_info = s_find_pixel_format_info(params.depth_stencil_target.pixel_format);
	c->has_depth = depth_info && (depth_info->caps & CF_GL_FMT_CAP_DEPTH);
	c->has_stencil = depth_info && depth_info->has_stencil && (depth_info->caps & CF_GL_FMT_CAP_STENCIL);
	c->depth = s_make_depth_renderbuffer(params.depth_stencil_target);
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
		GLenum attachment = c->has_stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, c->depth);
	}
	CF_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	CF_POLL_OPENGL_ERROR();

	return CF_Canvas{ (uint64_t)(uintptr_t)c };
}

void cf_gles_destroy_canvas(CF_Canvas ch)
{
	if (!ch.id) return;
	CF_GL_Canvas* c = (CF_GL_Canvas*)(uintptr_t)ch.id;
	if (c->depth) glDeleteRenderbuffers(1, &c->depth);
	if (c->fbo) glDeleteFramebuffers(1, &c->fbo);
	cf_gles_destroy_texture(c->cf_color);
	CF_POLL_OPENGL_ERROR();
	CF_FREE(c);
}

void cf_gles_canvas_get_size(CF_Canvas canvas_handle, int* w, int* h)
{
	CF_GL_Canvas* canvas = (CF_GL_Canvas*)(uintptr_t)canvas_handle.id;
	if (canvas) {
		if (w) { *w = canvas->w; }
		if (h) { *h = canvas->h; }
	}
}

CF_Texture cf_gles_canvas_get_target(CF_Canvas ch)
{
	CF_GL_Canvas* c = (CF_GL_Canvas*)(uintptr_t)ch.id;
	return c->cf_color;
}

CF_Texture cf_gles_canvas_get_depth_stencil_target(CF_Canvas)
{
	// GLES uses renderbuffers for depth/stencil, which are not sampleable as textures.
	return CF_Texture{};
}

static inline void s_clear_canvas(const CF_GL_Canvas* canvas)
{
	GLbitfield bits = GL_COLOR_BUFFER_BIT;
	glClearColor(app->clear_color.r, app->clear_color.g, app->clear_color.b, app->clear_color.a);
	if (s_canvas_has_depth(canvas)) {
		glClearDepthf(app->clear_depth);
		bits |= GL_DEPTH_BUFFER_BIT;
	}
	if (s_canvas_has_stencil(canvas)) {
		glClearStencil((GLint)app->clear_stencil);
		bits |= GL_STENCIL_BUFFER_BIT;
	}
	g_ctx.current_state.scissor_enabled = false;
	glDisable(GL_SCISSOR_TEST);
	glClear(bits);
}

void cf_gles_clear_canvas(CF_Canvas canvas_handle)
{
	CF_GL_Canvas* canvas = (CF_GL_Canvas*)(uintptr_t)canvas_handle.id;

	GLuint fbo = g_ctx.fbo;
	s_bind_framebuffer(canvas->fbo);
	s_clear_canvas(canvas);
	s_bind_framebuffer(fbo);

	CF_POLL_OPENGL_ERROR();
}

void cf_gles_apply_canvas(CF_Canvas canvas_handle, bool clear)
{
	CF_GL_Canvas* canvas = (CF_GL_Canvas*)(uintptr_t)canvas_handle.id;
	s_bind_framebuffer(canvas->fbo);
	g_ctx.canvas = canvas;
	if (clear) { s_clear_canvas(canvas); }

	g_ctx.target_state = s_default_state(canvas);
}

CF_Mesh cf_gles_make_mesh(int vertex_buffer_size, const CF_VertexAttribute* attributes, int attribute_count, int vertex_stride)
{
	auto* m = (CF_GL_Mesh*)CF_CALLOC(sizeof(CF_GL_Mesh));
	m->vbo.target = GL_ARRAY_BUFFER;
	m->vbo.capacity = vertex_buffer_size;
	m->vbo.stride = vertex_stride;
	int slot_index = -1;
	CF_GL_Slot* slot = s_prepare_buffer_slot(&m->vbo, vertex_buffer_size, &slot_index);
	if (slot) {
		m->vbo.id = slot->handle;
		m->vbo.active_slot = slot_index;
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	m->attribute_count = cf_min(attribute_count, CF_MESH_MAX_VERTEX_ATTRIBUTES);
	for (int i = 0; i < m->attribute_count; ++i) {
		m->attributes[i] = attributes[i];
		m->attributes[i].name = sintern(attributes[i].name);
	}
	CF_POLL_OPENGL_ERROR();
	return CF_Mesh{ (uint64_t)(uintptr_t)m };
}

void cf_gles_mesh_set_index_buffer(CF_Mesh mh, int index_buffer_size_in_bytes, int index_bit_count)
{
	auto* m = (CF_GL_Mesh*)(uintptr_t)mh.id;
	CF_ASSERT(index_bit_count == 16 || index_bit_count == 32);
	m->ibo.target = GL_ELEMENT_ARRAY_BUFFER;
	m->ibo.capacity = index_buffer_size_in_bytes;
	m->ibo.stride = index_bit_count / 8;
	int slot_index = -1;
	CF_GL_Slot* slot = s_prepare_buffer_slot(&m->ibo, index_buffer_size_in_bytes, &slot_index);
	if (slot) {
		m->ibo.id = slot->handle;
		m->ibo.active_slot = slot_index;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_mesh_set_instance_buffer(CF_Mesh mh, int instance_buffer_size_in_bytes, int instance_stride)
{
	auto* m = (CF_GL_Mesh*)(uintptr_t)mh.id;
	m->instance.target = GL_ARRAY_BUFFER;
	m->instance.capacity = instance_buffer_size_in_bytes;
	m->instance.stride = instance_stride;
	int slot_index = -1;
	CF_GL_Slot* slot = s_prepare_buffer_slot(&m->instance, instance_buffer_size_in_bytes, &slot_index);
	if (slot) {
		m->instance.id = slot->handle;
		m->instance.active_slot = slot_index;
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_mesh_update_vertex_data(CF_Mesh mh, void* verts, int vertex_count)
{
	auto* m = (CF_GL_Mesh*)(uintptr_t)mh.id;
	if (!m->vbo.stride) return;
	GLsizeiptr bytes = (GLsizeiptr)vertex_count * m->vbo.stride;
	int slot_index = -1;
	CF_GL_Slot* slot = s_prepare_buffer_slot(&m->vbo, bytes, &slot_index);
	if (!slot) return;
	GLintptr upload_offset = slot->offset;
	if (bytes > 0) {
		glBufferSubData(GL_ARRAY_BUFFER, upload_offset, bytes, verts);
		slot->offset = s_align_up(upload_offset + bytes, 256);
	} else {
		slot->offset = 0;
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	m->vbo.id = slot->handle;
	m->vbo.active_slot = slot_index;
	m->vbo.active_offset = upload_offset;
	m->vbo.count = vertex_count;
	m->vbo.version++;
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_mesh_update_index_data(CF_Mesh mh, void* indices, int index_count)
{
	auto* m = (CF_GL_Mesh*)(uintptr_t)mh.id;
	int stride = m->ibo.stride ? m->ibo.stride : (int)sizeof(uint16_t);
	GLsizeiptr bytes = (GLsizeiptr)index_count * stride;
	int slot_index = -1;
	CF_GL_Slot* slot = s_prepare_buffer_slot(&m->ibo, bytes, &slot_index);
	if (!slot) return;
	GLintptr upload_offset = slot->offset;
	if (bytes > 0) {
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, upload_offset, bytes, indices);
		slot->offset = s_align_up(upload_offset + bytes, 256);
	} else {
		slot->offset = 0;
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	m->ibo.id = slot->handle;
	m->ibo.active_slot = slot_index;
	m->ibo.active_offset = upload_offset;
	m->ibo.count = index_count;
	m->ibo.version++;
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_mesh_update_instance_data(CF_Mesh mh, void* instances, int instance_count)
{
	auto* m = (CF_GL_Mesh*)(uintptr_t)mh.id;
	if (!m->instance.stride) return;
	GLsizeiptr bytes = (GLsizeiptr)instance_count * m->instance.stride;
	int slot_index = -1;
	CF_GL_Slot* slot = s_prepare_buffer_slot(&m->instance, bytes, &slot_index);
	if (!slot) return;
	GLintptr upload_offset = slot->offset;
	if (bytes > 0) {
		glBufferSubData(GL_ARRAY_BUFFER, upload_offset, bytes, instances);
		slot->offset = s_align_up(upload_offset + bytes, 256);
	} else {
		slot->offset = 0;
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	m->instance.id = slot->handle;
	m->instance.active_slot = slot_index;
	m->instance.active_offset = upload_offset;
	m->instance.count = instance_count;
	m->instance.version++;
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_destroy_mesh(CF_Mesh mh)
{
	if (!mh.id) return;
	auto* m = (CF_GL_Mesh*)(uintptr_t)mh.id;
	s_destroy_buffer(&m->vbo);
	s_destroy_buffer(&m->ibo);
	s_destroy_buffer(&m->instance);
	CF_POLL_OPENGL_ERROR();
	CF_FREE(m);
}

void cf_gles_apply_mesh(CF_Mesh mesh_handle)
{
	CF_GL_Mesh* mesh = (CF_GL_Mesh*)(uintptr_t)mesh_handle.id;
	g_ctx.mesh = mesh;
}

static void s_build_uniforms(GLuint program, CF_GL_ShaderInfo* shader_info, const CF_ShaderInfo* uniform_info, GLuint* binding_point)
{
	shader_info->uniform_members = (CF_ShaderUniformMemberInfo*)CF_ALLOC(sizeof(CF_ShaderUniformMemberInfo) * uniform_info->num_uniform_members);
	for (int member_index = 0; member_index < uniform_info->num_uniform_members; ++member_index) {
		shader_info->uniform_members[member_index] = uniform_info->uniform_members[member_index];
		shader_info->uniform_members[member_index].name = cf_sintern(shader_info->uniform_members[member_index].name);
	}

	CF_ShaderUniformMemberInfo* members = shader_info->uniform_members;
	shader_info->num_uniform_blocks = uniform_info->num_uniforms;
	for (int block_index = 0; block_index < uniform_info->num_uniforms; ++block_index) {
		CF_GL_ShaderUniformBlock* block = &shader_info->uniform_blocks[block_index];
		block->info = uniform_info->uniforms[block_index];
		block->info.block_name = cf_sintern(block->info.block_name);
		block->members = members;
		// Members of consecutive blocks are tightly packed into a single array
		members += block->info.num_members;
	}

	glGenBuffers(shader_info->num_uniform_blocks, shader_info->ubo);
	for (int block_index = 0; block_index < uniform_info->num_uniforms; ++block_index) {
		CF_GL_ShaderUniformBlock* block = &shader_info->uniform_blocks[block_index];
		glBindBuffer(GL_UNIFORM_BUFFER, shader_info->ubo[block_index]);
		glBufferData(GL_UNIFORM_BUFFER, shader_info->uniform_blocks[block_index].info.block_size, NULL, GL_DYNAMIC_DRAW);
		// GLES 3.00 and WebGL do not support explicit binding (i.e: layout(binding = x))
		// Thus, we have to query this after linking.
		GLuint index = glGetUniformBlockIndex(program, block->info.block_name);
		if (index == GL_INVALID_INDEX) {
			// The block could be optimized out
			block->info.block_index = -1;
			continue;
		}
		glUniformBlockBinding(program, index, *binding_point);
		block->info.block_index = *binding_point;
		++*binding_point;
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	CF_POLL_OPENGL_ERROR();
}

CF_Shader cf_gles_make_shader_from_bytecode(CF_ShaderBytecode vertex_bytecode, CF_ShaderBytecode fragment_bytecode)
{
	GLuint program = s_make_program(vertex_bytecode, fragment_bytecode);

	// Copy refelection data
	CF_GL_Shader* shader = (CF_GL_Shader*)CF_CALLOC(sizeof(CF_GL_Shader));
	shader->program = program;

	// FS texture bindings.
	shader->texture_bindings = (CF_GL_TextureBinding*)CF_ALLOC(sizeof(CF_GL_TextureBinding) * fragment_bytecode.shader_info.num_images);
	for (int image_index = 0; image_index < fragment_bytecode.shader_info.num_images; ++image_index) {
		// GLES 3.00 and WebGL do not support explicit binding (i.e: layout(binding = x))
		// Thus, we have to query this after linking.
		const char* image_name = fragment_bytecode.shader_info.image_names[image_index];
		GLint location = glGetUniformLocation(program, image_name);
		if (location < 0) { continue; }  // The image might be optimized out

		shader->texture_bindings[shader->num_texture_bindings].location = location;
		shader->texture_bindings[shader->num_texture_bindings].name = cf_sintern(image_name);
		++shader->num_texture_bindings;
	}

	// VS texture bindings.
	shader->vs_texture_bindings = (CF_GL_TextureBinding*)CF_ALLOC(sizeof(CF_GL_TextureBinding) * vertex_bytecode.shader_info.num_images);
	for (int image_index = 0; image_index < vertex_bytecode.shader_info.num_images; ++image_index) {
		const char* image_name = vertex_bytecode.shader_info.image_names[image_index];
		GLint location = glGetUniformLocation(program, image_name);
		if (location < 0) { continue; }

		shader->vs_texture_bindings[shader->num_vs_texture_bindings].location = location;
		shader->vs_texture_bindings[shader->num_vs_texture_bindings].name = cf_sintern(image_name);
		++shader->num_vs_texture_bindings;
	}

	GLuint binding_point = 0;
	s_build_uniforms(program, &shader->vs, &vertex_bytecode.shader_info, &binding_point);
	s_build_uniforms(program, &shader->fs, &fragment_bytecode.shader_info, &binding_point);

	return { (uintptr_t)(uintptr_t)shader };
}

void cf_gles_destroy_shader_internal(CF_Shader shader_handle)
{
	if (!shader_handle.id) return;

	CF_GL_Shader* shader = (CF_GL_Shader*)(uintptr_t)shader_handle.id;

	glDeleteBuffers(shader->vs.num_uniform_blocks, shader->vs.ubo);
	glDeleteBuffers(shader->fs.num_uniform_blocks, shader->fs.ubo);

	CF_POLL_OPENGL_ERROR();

	CF_FREE(shader->vs.uniform_members);
	CF_FREE(shader->fs.uniform_members);
	CF_FREE(shader->texture_bindings);
	CF_FREE(shader->vs_texture_bindings);
	glDeleteProgram(shader->program);
	shader->~CF_GL_Shader();
	CF_FREE(shader);
}

static inline const CF_GL_ShaderUniformBlock* s_find_block_info(const CF_GL_ShaderInfo* shader_info, const char* name)
{
	for (int i = 0; i < shader_info->num_uniform_blocks; ++i) {
		const CF_GL_ShaderUniformBlock* block = &shader_info->uniform_blocks[i];
		if (block->info.block_name == name) {
			return block;
		}
	}

	return NULL;
}

static inline const CF_ShaderUniformMemberInfo* s_find_member_info(const CF_GL_ShaderUniformBlock* block, const char* name)
{
	for (int i = 0; i < block->info.num_members; ++i) {
		const CF_ShaderUniformMemberInfo* member = &block->members[i];
		if (member->name == name) {
			return member;
		}
	}

	return NULL;
}

static inline void s_upload_uniforms(CF_GL_ShaderInfo* shader_info, const CF_MaterialState* material, CF_Arena* arena)
{
	void* uniform_data_ptrs[CF_MAX_UNIFORM_BLOCK_COUNT];
	CF_MEMSET(uniform_data_ptrs, 0, sizeof(uniform_data_ptrs));

	// Copy data into uniform blocks.
	for (int uniform_index = 0; uniform_index < material->uniforms.count(); ++uniform_index) {
		const CF_Uniform* uniform = &material->uniforms[uniform_index];
		const CF_GL_ShaderUniformBlock* block = s_find_block_info(shader_info, uniform->block_name);
		if (block == NULL) { continue; }
		if (block->info.block_index < 0) { continue; }

		const CF_ShaderUniformMemberInfo* member = s_find_member_info(block, uniform->name);
		if (member == NULL) { continue; }

		CF_UniformType uniform_type = s_uniform_type(member->type);
		CF_ASSERT(uniform_type == uniform->type);
		CF_ASSERT(s_uniform_size(uniform_type) * member->array_length == uniform->size);

		int block_index = (int)(block - shader_info->uniform_blocks);
		void* uniform_data = uniform_data_ptrs[block_index];
		if (uniform_data == NULL) {
			uniform_data = cf_arena_alloc(arena, block->info.block_size);
			CF_MEMSET(uniform_data, 0, block->info.block_size);
			uniform_data_ptrs[block_index] = uniform_data;
		}

		CF_MEMCPY((void*)((uintptr_t)uniform_data + member->offset), uniform->data, uniform->size);
	}

	// Upload to GPU.
	for (int block_index = 0; block_index < shader_info->num_uniform_blocks; ++block_index) {
		void* block_data = uniform_data_ptrs[block_index];
		if (block_data == NULL) { continue; }

		const CF_GL_ShaderUniformBlock* block = &shader_info->uniform_blocks[block_index];
		glBindBuffer(GL_UNIFORM_BUFFER, shader_info->ubo[block_index]);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, block->info.block_size, block_data);
		glBindBufferBase(GL_UNIFORM_BUFFER, block->info.block_index, shader_info->ubo[block_index]);
	}
	CF_POLL_OPENGL_ERROR();

	cf_arena_reset(arena);
}

static inline void s_apply_vertex_attributes(CF_GL_Shader* shader, CF_GL_Mesh* mesh)
{
	if (mesh->ibo.id) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo.id);
	} else {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	uint64_t attribute_mask = 0;

	for (int i = 0; i < mesh->attribute_count; ++i) {
		const CF_VertexAttribute* attrib = &mesh->attributes[i];
		GLint loc = glGetAttribLocation(shader->program, attrib->name);
		if (loc < 0 || loc >= 64) continue;

		const bool per_instance = attrib->per_instance;
		CF_GL_Buffer& buf = per_instance ? mesh->instance : mesh->vbo;
		if (!buf.id) continue;
		GLintptr base_offset = per_instance ? mesh->instance.active_offset : mesh->vbo.active_offset;

		GLenum type = GL_FLOAT;
		GLint comps = 4;
		GLboolean norm = GL_FALSE;
		switch (attrib->format) {
			case CF_VERTEX_FORMAT_FLOAT:	type = GL_FLOAT; comps = 1; break;
			case CF_VERTEX_FORMAT_FLOAT2:	type = GL_FLOAT; comps = 2; break;
			case CF_VERTEX_FORMAT_FLOAT3:	type = GL_FLOAT; comps = 3; break;
			case CF_VERTEX_FORMAT_FLOAT4:	type = GL_FLOAT; comps = 4; break;
			case CF_VERTEX_FORMAT_INT:	type = GL_INT; comps = 1; break;
			case CF_VERTEX_FORMAT_INT2:	type = GL_INT; comps = 2; break;
			case CF_VERTEX_FORMAT_INT3:	type = GL_INT; comps = 3; break;
			case CF_VERTEX_FORMAT_INT4:	type = GL_INT; comps = 4; break;
			case CF_VERTEX_FORMAT_UINT:	type = GL_UNSIGNED_INT; comps = 1; break;
			case CF_VERTEX_FORMAT_UINT2:	type = GL_UNSIGNED_INT; comps = 2; break;
			case CF_VERTEX_FORMAT_UINT3:	type = GL_UNSIGNED_INT; comps = 3; break;
			case CF_VERTEX_FORMAT_UINT4:	type = GL_UNSIGNED_INT; comps = 4; break;
			case CF_VERTEX_FORMAT_BYTE4_NORM:	type = GL_BYTE; comps = 4; norm = GL_TRUE; break;
			case CF_VERTEX_FORMAT_UBYTE4_NORM:	type = GL_UNSIGNED_BYTE; comps = 4; norm = GL_TRUE; break;
			case CF_VERTEX_FORMAT_SHORT2:	type = GL_SHORT; comps = 2; break;
			case CF_VERTEX_FORMAT_SHORT2_NORM:	type = GL_SHORT; comps = 2; norm = GL_TRUE; break;
			case CF_VERTEX_FORMAT_SHORT4:	type = GL_SHORT; comps = 4; break;
			case CF_VERTEX_FORMAT_SHORT4_NORM:	type = GL_SHORT; comps = 4; norm = GL_TRUE; break;
			case CF_VERTEX_FORMAT_USHORT2:	type = GL_UNSIGNED_SHORT; comps = 2; break;
			case CF_VERTEX_FORMAT_USHORT2_NORM:	type = GL_UNSIGNED_SHORT; comps = 2; norm = GL_TRUE; break;
			case CF_VERTEX_FORMAT_USHORT4:	type = GL_UNSIGNED_SHORT; comps = 4; break;
			case CF_VERTEX_FORMAT_USHORT4_NORM:	type = GL_UNSIGNED_SHORT; comps = 4; norm = GL_TRUE; break;
			case CF_VERTEX_FORMAT_HALF2:	type = GL_HALF_FLOAT; comps = 2; break;
			case CF_VERTEX_FORMAT_HALF4:	type = GL_HALF_FLOAT; comps = 4; break;
			default: break;
		}
		glBindBuffer(GL_ARRAY_BUFFER, buf.id);
		if (!(g_ctx.enabled_vertex_attrib_mask & (1ULL << loc))) {
			glEnableVertexAttribArray((GLuint)loc);
		}
		const void* pointer = (const void*)(intptr_t)(attrib->offset + base_offset);
		if (type == GL_INT) {
			glVertexAttribIPointer((GLuint)loc, comps, type, buf.stride, pointer);
		} else {
			glVertexAttribPointer((GLuint)loc, comps, type, norm, buf.stride, pointer);
		}
		glVertexAttribDivisor((GLuint)loc, per_instance ? 1 : 0);

		attribute_mask |= 1ULL << loc;
	}

	uint64_t disable_mask = g_ctx.enabled_vertex_attrib_mask & ~attribute_mask;
	if (disable_mask) {
		uint64_t mask = disable_mask;
		for (GLuint loc = 0; mask; ++loc) {
			if (mask & 1ULL) {
				glDisableVertexAttribArray(loc);
			}
			mask >>= 1;
		}
	}

	g_ctx.enabled_vertex_attrib_mask = attribute_mask;
	CF_POLL_OPENGL_ERROR();
}

void cf_gles_apply_shader(CF_Shader shader_handle, CF_Material material_handle)
{
	CF_GL_Shader* shader = (CF_GL_Shader*)(uintptr_t)shader_handle.id;
	CF_MaterialInternal* material = (CF_MaterialInternal*)(uintptr_t)material_handle.id;
	g_ctx.material = material;

	// Render state.
	CF_RenderState render_state = material->state;

	// Cull.
	if (render_state.cull_mode == CF_CULL_MODE_NONE) {
		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
		glCullFace(s_wrap(render_state.cull_mode));
	}

	// Depth.
	const bool canvas_has_depth = s_canvas_has_depth(g_ctx.canvas);
	const bool depth_test_requested = render_state.depth_write_enabled || render_state.depth_compare != CF_COMPARE_FUNCTION_ALWAYS;
	if (canvas_has_depth && depth_test_requested) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(s_wrap(render_state.depth_compare));
		glDepthMask(render_state.depth_write_enabled ? GL_TRUE : GL_FALSE);
	} else {
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}

	// Stencil.
	const bool canvas_has_stencil = s_canvas_has_stencil(g_ctx.canvas);
	g_ctx.target_state.stencil_reference = (GLint)render_state.stencil.reference;
	g_ctx.current_state.stencil_reference = (GLint)render_state.stencil.reference;
	if (canvas_has_stencil && render_state.stencil.enabled) {
		glEnable(GL_STENCIL_TEST);
		glStencilMask(render_state.stencil.write_mask);
		glStencilFuncSeparate(GL_FRONT, s_wrap(render_state.stencil.front.compare), render_state.stencil.reference, render_state.stencil.read_mask);
		glStencilFuncSeparate(GL_BACK, s_wrap(render_state.stencil.back.compare), render_state.stencil.reference, render_state.stencil.read_mask);
		glStencilOpSeparate(GL_FRONT, s_wrap(render_state.stencil.front.fail_op), s_wrap(render_state.stencil.front.depth_fail_op), s_wrap(render_state.stencil.front.pass_op));
		glStencilOpSeparate(GL_BACK, s_wrap(render_state.stencil.back.fail_op), s_wrap(render_state.stencil.back.depth_fail_op), s_wrap(render_state.stencil.back.pass_op));
	} else {
		glDisable(GL_STENCIL_TEST);
		glStencilMask(0xFF);
	}

	// Blend.
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

	glUseProgram(shader->program);

	// Uniforms.
	s_upload_uniforms(&shader->vs, &material->vs, &material->block_arena);
	s_upload_uniforms(&shader->fs, &material->fs, &material->block_arena);

	// Textures (FS).
	GLuint texture_unit = 0;
	for (int texture_index = 0; texture_index < material->fs.textures.count(); ++texture_index) {
		CF_MaterialTex material_tex = material->fs.textures[texture_index];
		CF_GL_Texture* texture = (CF_GL_Texture*)(uintptr_t)material_tex.handle.id;
		for (int image_index = 0; image_index < shader->num_texture_bindings; ++image_index) {
			const CF_GL_TextureBinding* binding = &shader->texture_bindings[image_index];
			if (binding->name == material_tex.name) {
				glActiveTexture(GL_TEXTURE0 + texture_unit);
				glBindTexture(GL_TEXTURE_2D, texture->id);
				if (g_ctx.has_filter_override) {
					GLenum gl_filter = (g_ctx.filter_override == CF_FILTER_NEAREST) ? GL_NEAREST : GL_LINEAR;
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter);
				}
				glUniform1i(binding->location, texture_unit);
				++texture_unit;
				break;
			}
		}
	}

	// Textures (VS).
	for (int texture_index = 0; texture_index < material->vs.textures.count(); ++texture_index) {
		CF_MaterialTex material_tex = material->vs.textures[texture_index];
		CF_GL_Texture* texture = (CF_GL_Texture*)(uintptr_t)material_tex.handle.id;
		for (int image_index = 0; image_index < shader->num_vs_texture_bindings; ++image_index) {
			const CF_GL_TextureBinding* binding = &shader->vs_texture_bindings[image_index];
			if (binding->name == material_tex.name) {
				glActiveTexture(GL_TEXTURE0 + texture_unit);
				glBindTexture(GL_TEXTURE_2D, texture->id);
				if (g_ctx.has_filter_override) {
					GLenum gl_filter = (g_ctx.filter_override == CF_FILTER_NEAREST) ? GL_NEAREST : GL_LINEAR;
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter);
				}
				glUniform1i(binding->location, texture_unit);
				++texture_unit;
				break;
			}
		}
	}

	g_ctx.has_filter_override = false;
	CF_POLL_OPENGL_ERROR();

	CF_GL_Mesh* mesh = g_ctx.mesh;
	CF_ASSERT(mesh != NULL);

	s_apply_vertex_attributes(shader, mesh);
}

void cf_gles_draw_elements()
{
	CF_GL_Mesh* mesh = g_ctx.mesh;
	CF_MaterialInternal* material = g_ctx.material;
	CF_ASSERT(mesh != NULL);
	CF_ASSERT(material != NULL);

	s_apply_state();

	GLenum prim = s_wrap(material->state.primitive_type);
	int instance_count = (mesh->instance.id && mesh->instance.count > 0) ? mesh->instance.count : 0;

	if (mesh->ibo.id && mesh->ibo.count > 0) {
		GLenum elem = (mesh->ibo.stride == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
		if (instance_count > 0) {
			glDrawElementsInstanced(prim, mesh->ibo.count, elem, (const void*)(intptr_t)mesh->ibo.active_offset, instance_count);
		} else {
			glDrawElements(prim, mesh->ibo.count, elem, (const void*)(intptr_t)mesh->ibo.active_offset);
		}
	} else if (mesh->vbo.count > 0) {
		if (instance_count > 0) {
			glDrawArraysInstanced(prim, 0, mesh->vbo.count, instance_count);
		} else {
			glDrawArrays(prim, 0, mesh->vbo.count);
		}
	}

	if (mesh->vbo.active_slot >= 0 && mesh->vbo.active_slot < mesh->vbo.ring.count) {
		s_set_slot_fence(mesh->vbo.ring.slots[mesh->vbo.active_slot]);
	}
	if (mesh->ibo.active_slot >= 0 && mesh->ibo.active_slot < mesh->ibo.ring.count) {
		s_set_slot_fence(mesh->ibo.ring.slots[mesh->ibo.active_slot]);
	}
	if (mesh->instance.active_slot >= 0 && mesh->instance.active_slot < mesh->instance.ring.count) {
		s_set_slot_fence(mesh->instance.ring.slots[mesh->instance.active_slot]);
	}
	for (int texture_index = 0; texture_index < material->fs.textures.count(); ++texture_index) {
		CF_GL_Texture* texture = (CF_GL_Texture*)(uintptr_t)material->fs.textures[texture_index].handle.id;
		if (!texture) continue;
		if (texture->active_slot >= 0 && texture->active_slot < texture->ring.count) {
			s_set_slot_fence(texture->ring.slots[texture->active_slot]);
		}
	}
	for (int texture_index = 0; texture_index < material->vs.textures.count(); ++texture_index) {
		CF_GL_Texture* texture = (CF_GL_Texture*)(uintptr_t)material->vs.textures[texture_index].handle.id;
		if (!texture) continue;
		if (texture->active_slot >= 0 && texture->active_slot < texture->ring.count) {
			s_set_slot_fence(texture->ring.slots[texture->active_slot]);
		}
	}

	CF_POLL_OPENGL_ERROR();
	++app->draw_call_count;
	g_ctx.target_state = s_default_state(g_ctx.canvas);
}

void cf_gles_apply_viewport(int x, int y, int w, int h)
{
	g_ctx.target_state.viewport = { x, y, w, h };
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

// For GLES, samplers are just stored filter values since GLES sets filter mode directly on textures.
void* cf_gles_create_draw_sampler(CF_Filter filter)
{
	// Store the filter value as a pointer (we only need NEAREST=0 or LINEAR=1).
	return (void*)(uintptr_t)filter;
}

void cf_gles_destroy_draw_sampler(void* sampler)
{
	// Nothing to do for GLES - filter values don't need cleanup.
	CF_UNUSED(sampler);
}

void cf_gles_set_sampler_override(void* sampler)
{
	if (sampler) {
		g_ctx.filter_override = (CF_Filter)(uintptr_t)sampler;
		g_ctx.has_filter_override = true;
	} else {
		g_ctx.has_filter_override = false;
	}
}

//--------------------------------------------------------------------------------------------------
// Compute stubs (not supported on GLES3).

CF_ComputeShader cf_gles_make_compute_shader_from_bytecode(CF_ShaderBytecode bytecode)
{
	CF_UNUSED(bytecode);
	CF_ComputeShader result = { 0 };
	return result;
}

void cf_gles_destroy_compute_shader(CF_ComputeShader shader)
{
	CF_UNUSED(shader);
}

CF_StorageBuffer cf_gles_make_storage_buffer(CF_StorageBufferParams params)
{
	CF_UNUSED(params);
	CF_StorageBuffer result = { 0 };
	return result;
}

void cf_gles_update_storage_buffer(CF_StorageBuffer buffer, const void* data, int size)
{
	CF_UNUSED(buffer);
	CF_UNUSED(data);
	CF_UNUSED(size);
}

void cf_gles_destroy_storage_buffer(CF_StorageBuffer buffer)
{
	CF_UNUSED(buffer);
}

void cf_gles_dispatch_compute(CF_ComputeShader shader, CF_Material material, CF_ComputeDispatch dispatch)
{
	CF_UNUSED(shader);
	CF_UNUSED(material);
	CF_UNUSED(dispatch);
}
