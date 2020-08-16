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

#ifndef CUTE_GFX_H
#define CUTE_GFX_H

namespace cute
{

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

extern CUTE_API void CUTE_CALL gfx_vertex_buffer_params_add_attribute(gfx_vertex_buffer_params_t* params, int num_components, int offset);
extern CUTE_API gfx_vertex_buffer_t* CUTE_CALL gfx_vertex_buffer_new(app_t* app, gfx_vertex_buffer_params_t* params);
extern CUTE_API void CUTE_CALL gfx_vertex_buffer_free(app_t* app, gfx_vertex_buffer_t* buffer);
extern CUTE_API error_t CUTE_CALL gfx_vertex_buffer_map(app_t* app, gfx_vertex_buffer_t* buffer, int vertex_count, void** vertices, int index_count = 0, void** indices = NULL);
extern CUTE_API error_t CUTE_CALL gfx_vertex_buffer_unmap(app_t* app, gfx_vertex_buffer_t* buffer);

// -------------------------------------------------------------------------------------------------
// 4x4 matrix for setting a projection matrix in the graphics pipeline.
// 
// This matrix is exposed here for advanced users just in case they need it. Generally the
// `matrix_ortho_2d` is all that's needed for most 2d use cases.

struct gfx_matrix_t
{
	float data[16];
};

extern CUTE_API gfx_matrix_t CUTE_CALL matrix_identity();
extern CUTE_API gfx_matrix_t CUTE_CALL matrix_ortho_2d(float w, float h, float x, float y);

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

extern CUTE_API gfx_texture_t* CUTE_CALL gfx_texture_create(app_t* app, int w, int h, void* pixels, gfx_pixel_format_t pixel_format, gfx_wrap_mode_t wrap_mode);
extern CUTE_API void CUTE_CALL gfx_texture_clean_up(app_t* app, gfx_texture_t* tex);
extern CUTE_API gfx_render_texture_t* CUTE_CALL gfx_render_texture_new(app_t* app, int w, int h, gfx_pixel_format_t pixel_format, gfx_wrap_mode_t wrap_mode);
extern CUTE_API void CUTE_CALL gfx_render_texture_clean_up(app_t* app, gfx_render_texture_t* render_texture);

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

extern CUTE_API gfx_shader_t* CUTE_CALL gfx_shader_new(app_t* app, gfx_vertex_buffer_t* buffer, const char* vertex_shader, const char* pixel_shader);
extern CUTE_API void CUTE_CALL gfx_shader_free(app_t* app, gfx_shader_t* shader);
extern CUTE_API error_t CUTE_CALL gfx_shader_set_uniform(app_t* app, gfx_shader_t* shader, const char* uniform_name, void* value, gfx_uniform_type_t type);
extern CUTE_API error_t CUTE_CALL gfx_shader_set_mvp(app_t* app, gfx_shader_t* shader, gfx_matrix_t* mvp);
extern CUTE_API error_t CUTE_CALL gfx_shader_set_screen_wh(app_t* app, gfx_shader_t* shader, float w, float h);

// -------------------------------------------------------------------------------------------------
// Blend state.

enum blend_factor_t
{
	BLEND_FACTOR_ZERO,
	BLEND_FACTOR_ONE,
	BLEND_FACTOR_SRC_COLOR,
	BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	BLEND_FACTOR_SRC_ALPHA,
	BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	BLEND_FACTOR_DST_COLOR,
	BLEND_FACTOR_ONE_MINUS_DST_COLOR,
	BLEND_FACTOR_DST_ALPHA,
	BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
	BLEND_FACTOR_SRC_ALPHA_SATURATED,
	BLEND_FACTOR_BLEND_COLOR,
	BLEND_FACTOR_ONE_MINUS_BLEND_COLOR,
	BLEND_FACTOR_BLEND_ALPHA,
	BLEND_FACTOR_ONE_MINUS_BLEND_ALPHA
};

enum blend_op_t
{
	BLEND_OP_ADD,
	BLEND_OP_SUBTRACT,
	BLEND_OP_REVERSE_SUBTRACT,
	BLEND_OP_MIN,
	BLEND_OP_MAX
};

struct blend_state_t
{
	bool enabled = true;
	blend_op_t op_rgb = BLEND_OP_ADD;
	blend_factor_t src_factor_rgb = BLEND_FACTOR_SRC_ALPHA;
	blend_factor_t dst_factor_rgb = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_op_t op_alpha = BLEND_OP_ADD;
	blend_factor_t src_factor_alpha = BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	blend_factor_t dst_factor_alpha = BLEND_FACTOR_ONE;
};

// -------------------------------------------------------------------------------------------------
// Stenciling.

enum stencil_cmp_t
{
	STENCIL_CMP_ALWAYS,
	STENCIL_CMP_NEVER,
	STENCIL_CMP_EQUAL,
	STENCIL_CMP_NOT_EQUAL,
	STENCIL_CMP_LESS,
	STENCIL_CMP_LESS_EQUAL,
	STENCIL_CMP_GREATER,
	STENCIL_CMP_GREATER_EQUAL
};

enum stencil_op_t
{
	STENCIL_OP_KEEP,
	STENCIL_OP_ZERO,
	STENCIL_OP_REPLACE,
	STENCIL_OP_INC_CLAMP,
	STENCIL_OP_DEC_CLAMP,
	STENCIL_OP_INC_WRAP,
	STENCIL_OP_DEC_WRAP,
	STENCIL_OP_INVERT
};

struct stencil_depth_state_t
{
	bool depth_write_enabled = true;
	bool stencil_enabled = false;
	int reference;
	int read_mask;
	int write_mask;
	stencil_op_t depth_fail;
	stencil_op_t stencil_fail;
	stencil_op_t stencil_and_depth_both_pass;
	stencil_cmp_t compare;
};

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

extern CUTE_API void CUTE_CALL gfx_draw_call_set_mvp(gfx_draw_call_t* call, gfx_matrix_t mvp);
extern CUTE_API void CUTE_CALL gfx_draw_call_set_scissor_box(gfx_draw_call_t* call, gfx_scissor_t* scissor);
extern CUTE_API void CUTE_CALL gfx_draw_call_add_texture(gfx_draw_call_t* call, gfx_texture_t* texture, const char* uniform_name);
extern CUTE_API error_t CUTE_CALL gfx_draw_call_add_verts(app_t* app, gfx_draw_call_t* call, void* verts, int vert_count);
extern CUTE_API void CUTE_CALL gfx_draw_call_add_uniform(gfx_draw_call_t* call, const char* uniform_name, void* value, gfx_uniform_type_t type);

// -------------------------------------------------------------------------------------------------
// Graphics initialization and management functions.

enum gfx_type_t
{
	GFX_TYPE_NONE,
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

extern CUTE_API error_t CUTE_CALL gfx_init(app_t* app);
extern CUTE_API error_t CUTE_CALL gfx_init_upscale(app_t* app, int render_w, int render_h, gfx_upscale_maximum_t upscale_max);
extern CUTE_API void CUTE_CALL gfx_render_size(app_t* app, int* render_w, int* render_h);

extern CUTE_API void CUTE_CALL gfx_push_draw_call(app_t* app, gfx_draw_call_t* call);
extern CUTE_API error_t CUTE_CALL gfx_flush(app_t* app);
extern CUTE_API error_t CUTE_CALL gfx_flush_to_texture(app_t* app, gfx_texture_t* render_texture);
extern CUTE_API gfx_type_t CUTE_CALL gfx_type(app_t* app);
extern CUTE_API void CUTE_CALL gfx_set_clear_color(app_t* app, int color);

extern CUTE_API error_t CUTE_CALL gfx_line_mvp(app_t* app, gfx_matrix_t* projection);
extern CUTE_API void CUTE_CALL gfx_line_color(app_t* app, float r, float g, float b);
extern CUTE_API void CUTE_CALL gfx_line(app_t* app, float ax, float ay, float bx, float by);
extern CUTE_API void CUTE_CALL gfx_line_width(app_t* app, float width);
extern CUTE_API void CUTE_CALL gfx_line_depth_test(app_t* app, int zero_for_off);
extern CUTE_API error_t CUTE_CALL gfx_line_submit_draw_call(app_t* app);

extern CUTE_API void* CUTE_CALL gfx_get_device(app_t* app);

// TODO
// make/clean up render to texture
// post FX
// stencil
// viewport
// resizing based on window size
// texture wrap modes
// blend states

// -------------------------------------------------------------------------------------------------
// Definitions for inline and types intended to be used on C-runtime stack.

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
	gfx_viewport_t viewport;
	int use_scissor = 0;
	gfx_scissor_t scissor;
	blend_state_t blend_state;
	stencil_depth_state_t stencil;
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

#endif // CUTE_GFX_H
