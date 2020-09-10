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

#include <cute_batch.h>
#include <cute_alloc.h>
#include <cute_array.h>
#include <cute_file_system.h>
#include <cute_lru_cache.h>
#include <cute_defer.h>
//#include <cute_debug_printf.h>

#include <internal/cute_app_internal.h>

#include <shaders/sprite_shader.h>

struct quad_udata_t
{
	float alpha;
};

#include <cute/cute_png.h>

#define SPRITEBATCH_SPRITE_USERDATA quad_udata_t
#define SPRITEBATCH_IMPLEMENTATION
//#define SPRITEBATCH_LOG CUTE_DEBUG_PRINTF
#include <cute/cute_spritebatch.h>

#define CUTE_PNG_IMPLEMENTATION
#include <cute/cute_png.h>

namespace cute
{

struct quad_vertex_t
{
	v2 pos;
	v2 uv;
	float alpha;
};

struct batch_t
{
	::spritebatch_t sb;

	array<quad_vertex_t> verts;
	triple_buffer_t sprite_buffer;
	sg_shader default_shader = { 0 };
	sg_shader outline_shader = { 0 };
	sg_shader tint_shader = { 0 };
	sg_shader active_shader = { 0 };
	batch_quad_shader_type_t shader_type = BATCH_QUAD_SHADER_TYPE_DEFAULT;
	bool pip_dirty = true;
	sg_pipeline pip = { 0 };
	sg_blend_state blend_state;
	sg_depth_stencil_state depth_stencil_state;
	bool scissor_enabled = false;
	int scissor_x, scissor_y;
	int scissor_w, scissor_h;
	float outline_use_border = 0;
	matrix_t mvp;

	get_pixels_fn* get_pixels = NULL;
	void* get_pixels_udata = NULL;

	void* mem_ctx = NULL;
};

//--------------------------------------------------------------------------------------------------
// Internal functions.

static sg_shader s_load_shader(batch_t* b, batch_quad_shader_type_t type)
{
	sg_shader_desc params = { 0 };

	// Default sprite shader.
	switch (type) {
	case BATCH_QUAD_SHADER_TYPE_DEFAULT:
		params = *default_sprite_shader_desc();
		break;

	default:
		return { SG_INVALID_ID };
	}

	sg_shader shader = sg_make_shader(params);
	return shader;
}

#if 0
static gfx_shader_t* s_load_shader(batch_t* b, sprite_shader_type_t type)
{
	const char* default_vs = CUTE_STRINGIZE(
		struct vertex_t
		{
			float2 pos : POSITION0;
			float2 uv  : TEXCOORD0;
		};

		struct interp_t
		{
			float4 posH : POSITION0;
			float2 uv   : TEXCOORD0;
		};

		float4x4 u_mvp;
		float2 u_inv_screen_wh;

		interp_t main(vertex_t vtx)
		{
			float4 posH = mul(float4(round(vtx.pos), 0, 1), u_mvp);

			posH.xy += u_inv_screen_wh;
			interp_t interp;
			interp.posH = posH;
			interp.uv = vtx.uv;
			return interp;
		}
	);

	const char* default_ps = CUTE_STRINGIZE(
		struct interp_t
		{
			float4 posH : POSITION;
			float2 uv   : TEXCOORD0;
		};

		sampler2D u_image;

		float4 main(interp_t interp) : COLOR
		{
			float2 uv = interp.uv;
			float4 color = tex2D(u_image, uv);
			return color;
		}
	);

	const char* outline_vs = CUTE_STRINGIZE(
		struct vertex_t
		{
			float2 pos : POSITION0;
			float2 uv  : TEXCOORD0;
		};

		struct interp_t
		{
			float4 posH : POSITION0;
			float2 uv   : TEXCOORD0;
		};

		float4x4 u_mvp;
		float2 u_inv_screen_wh;

		interp_t main(vertex_t vtx)
		{
			float4 posH = mul(float4(round(vtx.pos), 0, 1), u_mvp);

			posH.xy += u_inv_screen_wh;
			interp_t interp;
			interp.posH = posH;
			interp.uv = vtx.uv;
			return interp;
		}
	);

	const char* outline_ps = CUTE_STRINGIZE(
		struct interp_t
		{
			float4 posH : POSITION;
			float2 uv   : TEXCOORD0;
		};

		sampler2D u_image;
		float2 u_texel_size;
		float u_use_border;

		float4 main(interp_t interp) : COLOR
		{
			float2 uv = interp.uv;

			// Border detection for pixel outlines.
			float a = (float)any(tex2D(u_image, uv + float2(0,  u_texel_size.y)).rgb);
			float b = (float)any(tex2D(u_image, uv + float2(0, -u_texel_size.y)).rgb);
			float c = (float)any(tex2D(u_image, uv + float2(u_texel_size.x,  0)).rgb);
			float d = (float)any(tex2D(u_image, uv + float2(-u_texel_size.x, 0)).rgb);
			float e = (float)any(tex2D(u_image, uv + float2(-u_texel_size.x, -u_texel_size.y)).rgb);
			float f = (float)any(tex2D(u_image, uv + float2(-u_texel_size.x,  u_texel_size.y)).rgb);
			float g = (float)any(tex2D(u_image, uv + float2( u_texel_size.x, -u_texel_size.y)).rgb);
			float h = (float)any(tex2D(u_image, uv + float2( u_texel_size.x,  u_texel_size.y)).rgb);

			float4 image_color = tex2D(u_image, uv);
			float image_mask = (float)any(image_color.rgb);

			float border = max(a, max(b, max(c, max(d, max(e, max(f, max(g, h))))))) * (1 - image_mask);
			border *= u_use_border;

			// Pick between white border or sprite color.
			float4 border_color = float4(1, 1, 1, 1);
			float4 color = lerp(image_color * image_mask, border_color, border);

			return color;
		}
	);

	const char* tint_vs = CUTE_STRINGIZE(
		struct vertex_t
		{
			float2 pos : POSITION0;
			float2 uv  : TEXCOORD0;
		};

		struct interp_t
		{
			float4 posH : POSITION0;
			float2 uv   : TEXCOORD0;
		};

		float4x4 u_mvp;
		float2 u_inv_screen_wh;

		interp_t main(vertex_t vtx)
		{
			float4 posH = mul(float4(round(vtx.pos), 0, 1), u_mvp);

			posH.xy += u_inv_screen_wh;
			interp_t interp;
			interp.posH = posH;
			interp.uv = vtx.uv;
			return interp;
		}
	);

	const char* tint_ps = CUTE_STRINGIZE(
		struct interp_t
		{
			float4 posH : POSITION;
			float2 uv   : TEXCOORD0;
		};

		sampler2D u_image;

		float4 main(interp_t interp) : COLOR
		{
			float2 uv = interp.uv;
			float4 color = tex2D(u_image, uv);
			return color;
		}
	);

	// Grab the shader strings.
	const char* vs = NULL;
	const char* ps = NULL;

	switch (type)
	{
	case SPRITE_SHADER_TYPE_DEFAULT:
		vs = default_vs;
		ps = default_ps;
		break;

	case SPRITE_SHADER_TYPE_OUTLINE:
		vs = outline_vs;
		ps = outline_ps;
		break;

	case SPRITE_SHADER_TYPE_TINT:
		vs = tint_vs;
		ps = tint_ps;
		break;
	}

	// Set common uniforms.
	app_t* app = b->app;
	gfx_shader_t* shader = gfx_shader_new(app, b->sprite_buffer, vs, ps);
	gfx_shader_set_screen_wh(app, shader, b->w, b->h);

	return shader;
}
#endif

//--------------------------------------------------------------------------------------------------
// spritebatch_t callbacks.

static void s_batch_report(spritebatch_sprite_t* sprites, int count, int texture_w, int texture_h, void* udata)
{
	batch_t* b = (batch_t*)udata;

	// Build vertex buffer of all quads for each sprite.
	int vert_count = count * 6;
	b->verts.ensure_count(vert_count);
	quad_vertex_t* verts = b->verts.data();

	for (int i = 0; i < count; ++i)
	{
		spritebatch_sprite_t* s = sprites + i;

		v2 quad[] = {
			{ -0.5f,  0.5f },
			{  0.5f,  0.5f },
			{  0.5f, -0.5f },
			{ -0.5f, -0.5f },
		};

		for (int j = 0; j < 4; ++j)
		{
			float x = quad[j].x;
			float y = quad[j].y;

			// rotate sprite about origin
			float x0 = s->c * x - s->s * y;
			float y0 = s->s * x + s->c * y;
			x = x0;
			y = y0;

			// scale sprite about origin
			x *= s->sx;
			y *= s->sy;

			// translate sprite into the world
			x += s->x;
			y += s->y;

			quad[j].x = x;
			quad[j].y = y;
		}

		// output transformed quad into CPU buffer
		quad_vertex_t* out_verts = verts + i * 6;

		for (int i = 0; i < 6; ++i) {
			out_verts[i].alpha = s->udata.alpha;
		}

		out_verts[0].pos.x = quad[0].x;
		out_verts[0].pos.y = quad[0].y;
		out_verts[0].uv.x = s->minx;
		out_verts[0].uv.y = s->maxy;

		out_verts[1].pos.x = quad[3].x;
		out_verts[1].pos.y = quad[3].y;
		out_verts[1].uv.x = s->minx;
		out_verts[1].uv.y = s->miny;

		out_verts[2].pos.x = quad[1].x;
		out_verts[2].pos.y = quad[1].y;
		out_verts[2].uv.x = s->maxx;
		out_verts[2].uv.y = s->maxy;

		out_verts[3].pos.x = quad[1].x;
		out_verts[3].pos.y = quad[1].y;
		out_verts[3].uv.x = s->maxx;
		out_verts[3].uv.y = s->maxy;

		out_verts[4].pos.x = quad[3].x;
		out_verts[4].pos.y = quad[3].y;
		out_verts[4].uv.x = s->minx;
		out_verts[4].uv.y = s->miny;

		out_verts[5].pos.x = quad[2].x;
		out_verts[5].pos.y = quad[2].y;
		out_verts[5].uv.x = s->maxx;
		out_verts[5].uv.y = s->miny;
	}

	// Map the vertex buffer with sprite vertex data.
	error_t err = triple_buffer_append(&b->sprite_buffer, vert_count, verts);
	CUTE_ASSERT(!err.is_error());

	// Setup resource bindings.
	sg_bindings bind = b->sprite_buffer.bind();
	bind.fs_images[0].id = (uint32_t)sprites->texture_id;
	sg_apply_bindings(bind);

	// Apply uniforms.
	// TODO - Move MVP to the spritebatch_flush function as an optimization.

	// Set shader-specific uniforms.
	switch (b->shader_type)
	{
	case BATCH_QUAD_SHADER_TYPE_DEFAULT:
	{
		default_vs_params_t params = {
			b->mvp
		};
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &params, sizeof(params));
	}	break;

	case BATCH_QUAD_SHADER_TYPE_OUTLINE:
	{
		//vs_uniforms_t uniforms = {
		//	b->mvp,
		//	v2(1.0f / (float)texture_w, 1.0f / (float)texture_h),
		//	b->outline_use_border
		//};
		//sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &params, sizeof(params));
	}	break;

	case BATCH_QUAD_SHADER_TYPE_TINT:
	{
		//sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &params, sizeof(params));
	}	break;
	}

	// Kick off a draw call.
	sg_draw(0, vert_count, 1);
}

static void s_get_pixels(SPRITEBATCH_U64 image_id, void* buffer, int bytes_to_fill, void* udata)
{
	batch_t* b = (batch_t*)udata;
	b->get_pixels(image_id, buffer, bytes_to_fill, b->get_pixels_udata);
}

static SPRITEBATCH_U64 s_generate_texture_handle(void* pixels, int w, int h, void* udata)
{
	batch_t* b = (batch_t*)udata;
	return texture_make((pixel_t*)pixels, w, h);
}

static void s_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata)
{
	batch_t* b = (batch_t*)udata;
	texture_destroy(texture_id);
}

//--------------------------------------------------------------------------------------------------

static void s_sync_pip(batch_t* b)
{
	if (b->pip_dirty) {
		if (b->pip.id != SG_INVALID_ID) sg_destroy_pipeline(b->pip);
		sg_pipeline_desc params = { 0 };
		params.layout.buffers[0].stride = sizeof(quad_vertex_t);
		params.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
		params.layout.buffers[0].step_rate = 1;
		params.layout.attrs[0].buffer_index = 0;
		params.layout.attrs[0].offset = CUTE_OFFSET_OF(quad_vertex_t, pos);
		params.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
		params.layout.attrs[1].buffer_index = 0;
		params.layout.attrs[1].offset = CUTE_OFFSET_OF(quad_vertex_t, uv);
		params.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
		params.layout.attrs[1].buffer_index = 0;
		params.layout.attrs[2].offset = CUTE_OFFSET_OF(quad_vertex_t, alpha);
		params.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT;
		params.layout.attrs[2].buffer_index = 0;
		params.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
		params.shader = b->active_shader;
		params.depth_stencil = b->depth_stencil_state;
		params.blend = b->blend_state;
		b->pip = sg_make_pipeline(params);
		b->pip_dirty = false;
	}
}

batch_t* batch_make(get_pixels_fn* get_pixels, void* get_pixels_udata, void* mem_ctx)
{
	batch_t* b = CUTE_NEW(batch_t, app->mem_ctx);
	if (!b) return NULL;

	b->mvp = matrix_identity();
	b->get_pixels = get_pixels;
	b->get_pixels_udata = get_pixels_udata;
	b->mem_ctx = mem_ctx;

	batch_set_depth_stencil_defaults(b);
	batch_set_blend_defaults(b);
	b->default_shader = b->active_shader = s_load_shader(b, BATCH_QUAD_SHADER_TYPE_DEFAULT);
	b->outline_shader = s_load_shader(b, BATCH_QUAD_SHADER_TYPE_OUTLINE);
	b->tint_shader = s_load_shader(b, BATCH_QUAD_SHADER_TYPE_TINT);
	b->sprite_buffer = triple_buffer_make(sizeof(quad_vertex_t) * 1024 * 10, sizeof(quad_vertex_t));

	spritebatch_config_t config;
	spritebatch_set_default_config(&config);
	config.atlas_use_border_pixels = 1;
	config.ticks_to_decay_texture = 100000;
	config.batch_callback = s_batch_report;
	config.get_pixels_callback = s_get_pixels;
	config.generate_texture_callback = s_generate_texture_handle;
	config.delete_texture_callback = s_destroy_texture_handle;
	config.allocator_context = b->mem_ctx;
	config.lonely_buffer_count_till_flush = 0;

	if (spritebatch_init(&b->sb, &config, b)) {
		CUTE_FREE(b, app->mem_ctx);
		if (!b) return NULL;
	}

	return b;
}

void batch_destroy(batch_t* b)
{
	spritebatch_term(&b->sb);
	b->~batch_t();
	CUTE_FREE(b, b->mem_ctx);
}

void batch_push(batch_t* b, batch_quad_t q)
{
	spritebatch_sprite_t s;
	s.image_id = q.id;
	s.w = q.w;
	s.h = q.h;
	s.x = q.transform.p.x;
	s.y = q.transform.p.y;
	s.sx = q.scale_x;
	s.sy = q.scale_y;
	s.s = q.transform.r.s;
	s.c = q.transform.r.c;
	s.sort_bits = (uint64_t)q.sort_bits << 32;
	s.udata.alpha = q.alpha;
	spritebatch_push(&b->sb, s);
}

error_t batch_flush(batch_t* b)
{
	// Start the pipeline.
	s_sync_pip(b);
	sg_apply_pipeline(b->pip);

	if (b->scissor_enabled) {
		sg_apply_scissor_rect(b->scissor_x, b->scissor_y, b->scissor_w, b->scissor_h, false);
	}

	// Construct batches.
	spritebatch_tick(&b->sb);
	if (!spritebatch_defrag(&b->sb)) {
		return error_failure("`spritebatch_defrag` failed.");
	}
	if (!spritebatch_flush(&b->sb)) {
		return error_failure("`spritebatch_flush` failed.");
	}

	// Increment which vertex buffer to use -- triple buffering.
	b->sprite_buffer.advance();

	return error_success();
}

//--------------------------------------------------------------------------------------------------

void batch_set_shader_type(batch_t* b, batch_quad_shader_type_t type)
{
	b->shader_type = type;

	// Load the shader, if needed, and set the active sprite system shader.
	switch (type)
	{
	case BATCH_QUAD_SHADER_TYPE_DEFAULT:
		if (b->default_shader.id == SG_INVALID_ID) {
			b->default_shader = s_load_shader(b, type);
		}
		b->active_shader = b->default_shader;
		break;

	case BATCH_QUAD_SHADER_TYPE_OUTLINE:
		if (b->outline_shader.id == SG_INVALID_ID) {
			b->outline_shader = s_load_shader(b, type);
		}
		b->active_shader = b->outline_shader;
		break;

	case BATCH_QUAD_SHADER_TYPE_TINT:
		if (b->tint_shader.id == SG_INVALID_ID) {
			b->tint_shader = s_load_shader(b, type);
		}
		b->active_shader = b->tint_shader;
		break;
	}
}

void batch_set_mvp(batch_t* b, matrix_t mvp)
{
	b->mvp = mvp;
}

void batch_set_scissor_box(batch_t* b, int x, int y, int w, int h)
{
	b->scissor_enabled = true;
	b->scissor_x = x;
	b->scissor_y = y;
	b->scissor_w = w;
	b->scissor_h = h;
}

void batch_no_scissor_box(batch_t* b)
{
	b->scissor_enabled = false;
}

void batch_outlines_use_border(batch_t* b, bool use_border)
{
	b->outline_use_border = use_border ? 1.0f : 0;
}

void batch_set_depth_stencil_state(batch_t* b, const sg_depth_stencil_state& depth_stencil_state)
{
	b->depth_stencil_state = depth_stencil_state;
	b->pip_dirty = true;
}

void batch_set_depth_stencil_defaults(batch_t* b)
{
	CUTE_MEMSET(&b->depth_stencil_state, 0, sizeof(b->depth_stencil_state));
	b->pip_dirty = true;
}

void batch_set_blend_state(batch_t* b, const sg_blend_state& blend_state)
{
	b->blend_state = blend_state;
	b->pip_dirty = true;
}

void batch_set_blend_defaults(batch_t* b)
{
	CUTE_MEMSET(&b->blend_state, 0, sizeof(b->blend_state));
	b->blend_state.enabled = true;
	b->blend_state.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
	b->blend_state.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	b->blend_state.op_rgb = SG_BLENDOP_ADD;
	b->blend_state.src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	b->blend_state.dst_factor_alpha = SG_BLENDFACTOR_ONE;
	b->blend_state.op_alpha = SG_BLENDOP_ADD;
	b->pip_dirty = true;
}

}
