/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <cute_defines.h>
#include <cute_c_runtime.h>
#include <cute_error.h>

namespace cute
{

/**
 * Represents the global state for all of cute graphics.
 */
struct gfx_t;

// -------------------------------------------------------------------------------------------------
// Vertex buffers.

enum gfx_vertex_buffer_type_t
{
	GFX_VERTEX_BUFFER_TYPE_STATIC,
	GFX_VERTEX_BUFFER_TYPE_DYNAMIC
};

enum gfx_index_buffer_type_t
{
	GFX_INDEX_BUFFER_TYPE_NONE,
	GFX_INDEX_BUFFER_TYPE_UINT16,
	GFX_INDEX_BUFFER_TYPE_UINT32,
};

struct gfx_vertex_buffer_params_t;
struct gfx_vertex_buffer_t;

CUTE_API void CUTE_CALL gfx_vertex_buffer_params_add_attribute(gfx_vertex_buffer_params_t* params, int num_components, int offset);
CUTE_API gfx_vertex_buffer_t* CUTE_CALL gfx_vertex_buffer_new(gfx_t* gfx, gfx_vertex_buffer_params_t* params);
CUTE_API void CUTE_CALL gfx_vertex_buffer_free(gfx_t* gfx, gfx_vertex_buffer_t* buffer);
CUTE_API error_t CUTE_CALL gfx_vertex_buffer_map(gfx_t* gfx, gfx_vertex_buffer_t* buffer, int vertex_count, void** vertices, int index_count = 0, void** indices = NULL);
CUTE_API error_t CUTE_CALL gfx_vertex_buffer_unmap(gfx_t* gfx, gfx_vertex_buffer_t* buffer);

// -------------------------------------------------------------------------------------------------
// 4x4 matrix for setting a projection matrix in the graphics pipeline.
// 
// This matrix is exposed here for advanced users just in case they need it. Generally the
// `matrix_ortho_2d` is all that's needed for most 2d use cases.

struct gfx_matrix_t
{
	float data[16];
};

CUTE_API void CUTE_CALL matrix_identity(gfx_matrix_t* m);
CUTE_API void CUTE_CALL matrix_ortho_2d(gfx_matrix_t* m, float w, float h, float x, float y);

// -------------------------------------------------------------------------------------------------
// Textures.

/**
 * A standard texture for storing data on the GPU, usually used for drawing sprites.
 */
struct gfx_texture_t;

/**
 * A special texture the GPU can render onto, instead of the to the screen.
 */
struct gfx_render_texture_t;

enum gfx_pixel_format_t
{
	GFX_PIXEL_FORMAT_R8B8G8A8
};

enum gfx_wrap_mode_t
{
	GFX_WRAP_MODE_REPEAT,
	GFX_WRAP_MODE_CLAMP_EDGE,
	GFX_WRAP_MODE_CLAMP_BORDER,
	GFX_WRAP_MODE_REPEAT_MIRRORED,
};

CUTE_API gfx_texture_t* CUTE_CALL gfx_texture_create(gfx_t* gfx, int w, int h, void* pixels, gfx_pixel_format_t pixel_format, gfx_wrap_mode_t wrap_mode);
CUTE_API void CUTE_CALL gfx_texture_clean_up(gfx_t* gfx, gfx_texture_t* tex);
CUTE_API gfx_render_texture_t* CUTE_CALL gfx_render_texture_new(gfx_t* gfx, int w, int h, gfx_pixel_format_t pixel_format, gfx_wrap_mode_t wrap_mode);
CUTE_API void CUTE_CALL gfx_render_texture_clean_up(gfx_t* gfx, gfx_render_texture_t* render_texture);

// -------------------------------------------------------------------------------------------------
// Shaders and uniforms.

/**
 * A shader is a small piece of code that can run on the GPU. In cute gfx a shader also can have
 * uniforms attached to it, which are small pieces of data used to fill in variables in the shader
 * program.
 */
struct gfx_shader_t;

enum gfx_uniform_type_t
{
	GFX_UNIFORM_TYPE_INVALID,
	GFX_UNIFORM_TYPE_BOOL,
	GFX_UNIFORM_TYPE_FLOAT,
	GFX_UNIFORM_TYPE_FLOAT2,
	GFX_UNIFORM_TYPE_FLOAT3,
	GFX_UNIFORM_TYPE_FLOAT4,
	GFX_UNIFORM_TYPE_INT,
	GFX_UNIFORM_TYPE_TEXTURE,
	GFX_UNIFORM_TYPE_MATRIX,
};

CUTE_API gfx_shader_t* CUTE_CALL gfx_shader_new(gfx_t* gfx, gfx_vertex_buffer_t* buffer, const char* vertex_shader, const char* pixel_shader);
CUTE_API void CUTE_CALL gfx_shader_free(gfx_t* gfx, gfx_shader_t* shader);
CUTE_API error_t CUTE_CALL gfx_shader_set_uniform(gfx_t* gfx, gfx_shader_t* shader, const char* uniform_name, void* value, gfx_uniform_type_t type);
CUTE_API error_t CUTE_CALL gfx_shader_set_mvp(gfx_t* gfx, gfx_shader_t* shader, gfx_matrix_t* mvp);
CUTE_API error_t CUTE_CALL gfx_shader_set_screen_wh(gfx_t* gfx, gfx_shader_t* shader, float w, float h);

// -------------------------------------------------------------------------------------------------
// Draw calls.

/**
 * A draw call collates all information needed to render something with the GPU. The `gfx_draw_call_t`
 * is just a POD struct and can be used on the stack. This is a fairly low-level construct, so only use
 * it if you know what you're doing!
 */
struct gfx_draw_call_t;

struct gfx_viewport_t
{
	float x, y;
	float w, h;
};

struct gfx_scissor_t
{
	int left;
	int top;
	int right;
	int bottom;
};

// TODO:
// Draw call and uniform can *not* store strings if rendering is moved off the main thread,
// otherwise the pointers will dangle. Copy strings into local buffers or something!

CUTE_API void CUTE_CALL gfx_draw_call_set_mvp(gfx_draw_call_t* call, gfx_matrix_t* mvp);
CUTE_API void CUTE_CALL gfx_draw_call_set_scissor_box(gfx_draw_call_t* call, gfx_scissor_t* scissor);
CUTE_API void CUTE_CALL gfx_draw_call_add_texture(gfx_draw_call_t* call, gfx_texture_t* texture, const char* uniform_name);
CUTE_API error_t CUTE_CALL gfx_draw_call_add_verts(gfx_t* gfx, gfx_draw_call_t* call, void* verts, int vert_count);
CUTE_API void CUTE_CALL gfx_draw_call_add_uniform(gfx_draw_call_t* call, const char* uniform_name, void* value, gfx_uniform_type_t type);

// -------------------------------------------------------------------------------------------------
// `gfx_t` creation and management functions.

enum gfx_type_t
{
	GFX_TYPE_D3D9,
	GFX_TYPE_GL,
	GFX_TYPE_GL_ES
};

enum gfx_upscale_maximum_t
{
	GFX_UPSCALE_MAXIMUM_ANY,
	GFX_UPSCALE_MAXIMUM_4X,
	GFX_UPSCALE_MAXIMUM_3X,
	GFX_UPSCALE_MAXIMUM_2X,
	GFX_UPSCALE_MAXIMUM_1X,
};

struct gfx_t;

CUTE_API gfx_t* CUTE_CALL gfx_new(gfx_type_t type, gfx_pixel_format_t pixel_format, int screen_w, int screen_h, int tex_w, int tex_h, gfx_upscale_maximum_t upscale, void* platform_handle = NULL, void* user_mem_ctx = NULL);
CUTE_API void CUTE_CALL gfx_clean_up(gfx_t* gfx);
CUTE_API void CUTE_CALL gfx_push_draw_call(gfx_t* gfx, gfx_draw_call_t* call);
CUTE_API error_t CUTE_CALL gfx_flush(gfx_t* gfx);
CUTE_API error_t CUTE_CALL gfx_flush_to_texture(gfx_t* gfx, gfx_texture_t* render_texture);
CUTE_API void CUTE_CALL gfx_set_alpha(gfx_t* gfx, int one_for_enabled);
CUTE_API gfx_type_t CUTE_CALL gfx_type(gfx_t* gfx);
CUTE_API void CUTE_CALL gfx_set_clear_color(gfx_t* gfx, int color);

CUTE_API void CUTE_CALL gfx_line_mvp(gfx_t* gfx, gfx_matrix_t* projection);
CUTE_API void CUTE_CALL gfx_line_color(gfx_t* gfx, float r, float g, float b);
CUTE_API void CUTE_CALL gfx_line(gfx_t* gfx, float ax, float ay, float bx, float by);
CUTE_API void CUTE_CALL gfx_line_width(gfx_t* gfx, float width);
CUTE_API void CUTE_CALL gfx_line_depth_test(gfx_t* gfx, int zero_for_off);
CUTE_API void CUTE_CALL gfx_line_submit_draw_call(gfx_t* gfx);

// make/clean up framebuffer

// -------------------------------------------------------------------------------------------------
// Inline definitions for inline and types intended to be used on C-runtime stack.

#define CUTE_GFX_VERTEX_BUFFER_MAX_ATTRIBUTES (16)
#define CUTE_GFX_DRAW_CALL_MAX_TEXTURES (8)
#define CUTE_GFX_DRAW_CALL_MAX_UNIFORMS (16)

struct gfx_vertex_buffer_params_t
{
	gfx_vertex_buffer_type_t type = GFX_VERTEX_BUFFER_TYPE_DYNAMIC;
	int attribute_count = 0;
	int num_components[CUTE_GFX_VERTEX_BUFFER_MAX_ATTRIBUTES];
	int offsets[CUTE_GFX_VERTEX_BUFFER_MAX_ATTRIBUTES];
	int stride = 0;

	int vertex_count = 0;
	gfx_index_buffer_type_t index_type = GFX_INDEX_BUFFER_TYPE_NONE;
	int index_count = 0;
};

struct gfx_uniform_t
{
	int dirty;
	const char* name;
	uint64_t h;
	gfx_uniform_type_t type;
	union
	{
		float data[16];
		void* texture_handle;
	} u;
};

struct gfx_buffer_indices_t
{
	int index0 = 0;
	int index1 = 0;
	int element_count = 0;
	int stride = 0;
	int buffer_number = 0;
};

struct gfx_draw_call_t
{
	int use_mvp = 0;
	gfx_matrix_t mvp;
	gfx_shader_t* shader = NULL;
	gfx_viewport_t* veiwport = NULL;
	int use_scissor = 0;
	gfx_scissor_t scissor;
	gfx_vertex_buffer_t* buffer = NULL;
	gfx_buffer_indices_t vertex_indices;
	gfx_buffer_indices_t index_indices;
	int texture_count = 0;
	gfx_texture_t* textures[CUTE_GFX_DRAW_CALL_MAX_TEXTURES];
	const char* texture_uniform_names[CUTE_GFX_DRAW_CALL_MAX_TEXTURES];
	int uniform_count = 0;
	gfx_uniform_t uniforms[CUTE_GFX_DRAW_CALL_MAX_UNIFORMS];
};

}
