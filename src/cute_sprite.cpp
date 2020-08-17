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

#define SPRITEBATCH_IMPLEMENTATION
#include <cute/cute_spritebatch.h>

#define CUTE_PNG_IMPLEMENTATION
#include <cute/cute_png.h>

#include <cute_sprite.h>
#include <cute_alloc.h>
#include <cute_array.h>
#include <cute_file_system.h>
#include <cute_lru_cache.h>
#include <cute_defer.h>
#include <cute_file_index.h>

#include <internal/cute_app_internal.h>

namespace cute
{

struct sprite_vertex_t
{
	v2 pos;
	v2 uv;
};

struct vs_uniforms_t
{
	matrix_t mvp;
	v2 texel_size;
	float use_border;
};

enum sprite_batch_mode_t
{
	SPRITE_BATCH_MODE_UNINITIALIZED,
	SPRITE_BATCH_MODE_LRU,
	SPRITE_BATCH_MODE_CUSTOM,
};

struct cached_image_t
{
	uint64_t id;
	const char* path;
	size_t size;
	cp_image_t img;
};

using sprite_cache_t = lru_cache<uint64_t, cached_image_t>;

struct spritebatch_t
{
	::spritebatch_t sb;
	app_t* app;
	sprite_batch_mode_t mode = SPRITE_BATCH_MODE_UNINITIALIZED;
	float w = 0, h = 0;

	array<sprite_vertex_t> verts;
	triple_buffer_t sprite_buffer;
	sg_shader default_shader = { 0 };
	sg_shader outline_shader = { 0 };
	sg_shader tint_shader = { 0 };
	sg_shader active_shader = { 0 };
	sprite_shader_type_t shader_type = SPRITE_SHADER_TYPE_DEFAULT;
	sg_pipeline pip = { 0 };
	bool scissor_enabled = false;
	int scissor_x, scissor_y;
	int scissor_w, scissor_h;
	float outline_use_border = 0;
	matrix_t mvp;

	const char** image_paths = NULL;
	int image_paths_count = 0;
	size_t cache_capacity = 0;
	size_t cache_used = 0;
	sprite_cache_t* cache = NULL;

	file_index_t* easy_images = NULL;
	const char** easy_paths = NULL;

	void* mem_ctx = NULL;
};

//--------------------------------------------------------------------------------------------------
// Internal functions.

static sg_shader s_load_shader(spritebatch_t* sb, sprite_shader_type_t type)
{
	sg_shader_desc params = { 0 };

	// Default sprite shader.
	switch (type) {
	case SPRITE_SHADER_TYPE_DEFAULT:
		params.attrs[0].name = "pos";
		params.attrs[0].sem_name = "POSITION";
		params.attrs[0].sem_index = 0;
		params.attrs[1].name = "uv";
		params.attrs[1].sem_name = "TEXCOORD";
		params.attrs[1].sem_index = 0;
		params.vs.uniform_blocks[0].size = sizeof(vs_uniforms_t::mvp);
		params.vs.uniform_blocks[0].uniforms[0].name = "u_mvp";
		params.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_MAT4;
		params.fs.images[0].name = "u_image";
		params.fs.images[0].type = SG_IMAGETYPE_2D;
		params.vs.source = CUTE_STRINGIZE(
			cbuffer params : register(b0)
			{
				float4x4 u_mvp;
			};

			struct vs_in
			{
				float2 pos : POSITION;
				float2 uv  : TEXCOORD0;
			};

			struct vs_out
			{
				float4 posH : SV_Position;
				float2 uv   : TEXCOORD0;
			};

			vs_out main(vs_in vtx)
			{
				float4 posH = mul(float4(round(vtx.pos), 0, 1), u_mvp);

				vs_out interp;
				interp.posH = posH;
				interp.uv = vtx.uv;
				return interp;
			}
		);
		params.fs.source = CUTE_STRINGIZE(
			Texture2D<float4> u_image: register(t0);
			sampler smp: register(s0);

			float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target0
			{
				float4 color = u_image.Sample(smp, uv);
				return color;
			}
		);
		break;

	default:
		return { SG_INVALID_ID };
	}

	sg_shader shader = sg_make_shader(params);
	return shader;
}

#if 0
static gfx_shader_t* s_load_shader(spritebatch_t* sb, sprite_shader_type_t type)
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
	app_t* app = sb->app;
	gfx_shader_t* shader = gfx_shader_new(app, sb->sprite_buffer, vs, ps);
	gfx_shader_set_screen_wh(app, shader, sb->w, sb->h);

	return shader;
}
#endif

static void s_free_cached_image(spritebatch_t* sb, cached_image_t* cached_image)
{
	CUTE_FREE(cached_image->img.pix, NULL);
}

static void s_free_all_images_in_cache(spritebatch_t* sb)
{
	list_t* list = sb->cache->list();
	for (list_node_t* n = list_begin(list); n != list_end(list); n = n->next) {
		cached_image_t cached_image = *sprite_cache_t::node_to_item(n);
		s_free_cached_image(sb, &cached_image);
	}
}

//--------------------------------------------------------------------------------------------------

spritebatch_t* sprite_batch_make(app_t* app)
{
	spritebatch_t* sb = CUTE_NEW(spritebatch_t, app->mem_ctx);
	if (!sb) return NULL;

	sb->w = (float)app->w;
	sb->h = (float)app->h;
	sb->app = app;
	sb->mem_ctx = app->mem_ctx;

	sg_pipeline_desc pip_params = { 0 };
	pip_params.layout.buffers[0].stride = sizeof(sprite_vertex_t);
	pip_params.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
	pip_params.layout.buffers[0].step_rate = 1;
	pip_params.layout.attrs[0].buffer_index = 0;
	pip_params.layout.attrs[0].offset = CUTE_OFFSET_OF(sprite_vertex_t, pos);
	pip_params.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
	pip_params.layout.attrs[1].buffer_index = 0;
	pip_params.layout.attrs[1].offset = CUTE_OFFSET_OF(sprite_vertex_t, uv);
	pip_params.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
	pip_params.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
	pip_params.shader = sb->default_shader = sb->active_shader = s_load_shader(sb, SPRITE_SHADER_TYPE_DEFAULT);
	pip_params.blend.enabled = true;
	pip_params.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
	pip_params.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	pip_params.blend.op_rgb = SG_BLENDOP_ADD;
	pip_params.blend.src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	pip_params.blend.dst_factor_alpha = SG_BLENDFACTOR_ONE;
	pip_params.blend.op_alpha = SG_BLENDOP_ADD;
	sb->pip = sg_make_pipeline(pip_params);

	sb->outline_shader = s_load_shader(sb, SPRITE_SHADER_TYPE_OUTLINE);
	sb->tint_shader = s_load_shader(sb, SPRITE_SHADER_TYPE_TINT);

	sb->sprite_buffer = triple_buffer_make(sizeof(sprite_vertex_t) * 1024 * 10, sizeof(sprite_vertex_t), 0);

	return sb;
}

void sprite_batch_destroy(spritebatch_t* sb)
{
	if (sb->easy_paths) {
		file_index_free_paths(sb->easy_images, sb->easy_paths);
		file_index_destroy(sb->easy_images);
	}

	spritebatch_term(&sb->sb);
	s_free_all_images_in_cache(sb);
	sb->cache->~sprite_cache_t();
	CUTE_FREE(sb->cache, sb->mem_ctx);
	sb->~spritebatch_t();
	CUTE_FREE(sb, sb->mem_ctx);
}

spritebatch_t* sprite_batch_easy_make(app_t* app, const char* path)
{
	spritebatch_t* sb = sprite_batch_make(app);

	int w = 0, h = 0;
	app_render_size(app, &w, &h);
	matrix_t mvp = matrix_ortho_2d((float)w, (float)h, 0, 0);
	sprite_batch_set_mvp(sb, mvp);

	sb->easy_images = file_index_make();
	file_index_search_directory(sb->easy_images, path, "png");
	int paths_count;
	const char** paths = file_index_get_paths(sb->easy_images, &paths_count);
	sprite_batch_enable_disk_LRU_cache(sb, paths, paths_count, CUTE_MB * 256);
	sb->easy_paths = paths;

	return sb;
}

error_t sprite_batch_easy_sprite(spritebatch_t* sb, const char* path, sprite_t* out)
{
	uint64_t id;
	error_t err = file_index_find(sb->easy_images, path, &id);
	if (err.is_error()) return err;

	sprite_batch_LRU_cache_prefetch(sb, id);

	sprite_t sprite;
	cached_image_t img;
	err = sb->cache->find(id, &img);
	if (err.is_error()) return err;

	sprite.id = id;
	sprite.w = img.img.w;
	sprite.h = img.img.h;
	sprite.scale_x = (float)img.img.w;
	sprite.scale_y = (float)img.img.h;
	sprite.transform.p = cute::v2(0, 0);
	sprite.transform.r = cute::make_rotation(0);
	sprite.sort_bits = 0;
	*out = sprite;

	return error_success();
}

void sprite_batch_push(spritebatch_t* sb, sprite_t sprite)
{
	sprite_t s = sprite;
	spritebatch_push(&sb->sb, s.id, s.w, s.h, s.transform.p.x, s.transform.p.y, s.scale_x, s.scale_y, s.transform.r.c, s.transform.r.s, sprite.sort_bits);
}

error_t sprite_batch_flush(spritebatch_t* sb)
{
	// Start the pipeline.
	sg_apply_pipeline(sb->pip);

	if (sb->scissor_enabled) {
		sg_apply_scissor_rect(sb->scissor_x, sb->scissor_y, sb->scissor_w, sb->scissor_h, false);
	}

	// Construct batches.
	spritebatch_tick(&sb->sb);
	if (!spritebatch_defrag(&sb->sb)) {
		return error_failure("`spritebatch_defrag` failed.");
	}
	if (!spritebatch_flush(&sb->sb)) {
		return error_failure("`spritebatch_flush` failed.");
	}

	// Increment which vertex buffer to use -- triple buffering.
	sb->sprite_buffer.advance();

	return error_success();
}

//--------------------------------------------------------------------------------------------------
// spritebatch_t callbacks.

static void s_batch_report(spritebatch_sprite_t* sprites, int count, int texture_w, int texture_h, void* udata)
{
	spritebatch_t* sb = (spritebatch_t*)udata;

	// Build vertex buffer of all quads for each sprite.
	int vert_count = count * 6;
	sb->verts.ensure_count(vert_count);
	sprite_vertex_t* verts = sb->verts.data();

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
		sprite_vertex_t* out_verts = verts + i * 6;

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
	triple_buffer_append(&sb->sprite_buffer, vert_count, verts, 0, NULL);

	// Setup resource bindings.
	sg_bindings bind = { 0 };
	bind.vertex_buffers[0] = sb->sprite_buffer.get_vbuf();
	bind.vertex_buffer_offsets[0] = sb->sprite_buffer.vbuf.offset;
	bind.index_buffer = sb->sprite_buffer.get_ibuf();
	bind.index_buffer_offset = sb->sprite_buffer.ibuf.offset;
	bind.fs_images[0].id = (uint32_t)sprites->texture_id;
	sg_apply_bindings(bind);

	vs_uniforms_t uniforms = {
		sb->mvp,
		v2(1.0f / (float)texture_w, 1.0f / (float)texture_h),
		sb->outline_use_border
	};

	// Set shader-specific uniforms.
	switch (sb->shader_type)
	{
	case SPRITE_SHADER_TYPE_DEFAULT:
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &uniforms.mvp, sizeof(uniforms.mvp));
		break;

	case SPRITE_SHADER_TYPE_OUTLINE:
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &uniforms, sizeof(uniforms));
		break;

	case SPRITE_SHADER_TYPE_TINT:
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &uniforms.mvp, sizeof(uniforms.mvp));
		break;
	}

	// Kick off a draw call.
	sg_draw(0, vert_count, 1);
}

static void s_get_pixels(SPRITEBATCH_U64 image_id, void* buffer, int bytes_to_fill, void* udata)
{
	spritebatch_t* sb = (spritebatch_t*)udata;
	sprite_batch_LRU_cache_prefetch(sb, image_id);
	cached_image_t cached_image;
	if (!sb->cache->find(image_id, &cached_image).is_error()) {
		CUTE_ASSERT(cached_image.size == bytes_to_fill);
		CUTE_MEMCPY(buffer, cached_image.img.pix, bytes_to_fill);
	} else {
		CUTE_MEMSET(buffer, bytes_to_fill, 0);
	}
}

static SPRITEBATCH_U64 s_generate_texture_handle(void* pixels, int w, int h, void* udata)
{
	spritebatch_t* sb = (spritebatch_t*)udata;
	return texture_make((pixel_t*)pixels, w, h);
}

static void s_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata)
{
	spritebatch_t* sb = (spritebatch_t*)udata;
	texture_destroy(texture_id);
}

//--------------------------------------------------------------------------------------------------

void sprite_batch_set_shader_type(spritebatch_t* sb, sprite_shader_type_t type)
{
	sb->shader_type = type;

	// Load the shader, if needed, and set the active sprite system shader.
	switch (type)
	{
	case SPRITE_SHADER_TYPE_DEFAULT:
		if (sb->default_shader.id == SG_INVALID_ID) {
			sb->default_shader = s_load_shader(sb, type);
		}
		sb->active_shader = sb->default_shader;
		break;

	case SPRITE_SHADER_TYPE_OUTLINE:
		if (sb->outline_shader.id == SG_INVALID_ID) {
			sb->outline_shader = s_load_shader(sb, type);
		}
		sb->active_shader = sb->outline_shader;
		break;

	case SPRITE_SHADER_TYPE_TINT:
		if (sb->tint_shader.id == SG_INVALID_ID) {
			sb->tint_shader = s_load_shader(sb, type);
		}
		sb->active_shader = sb->tint_shader;
		break;
	}
}

void sprite_batch_set_mvp(spritebatch_t* sb, matrix_t mvp)
{
	sb->mvp = mvp;
}

void sprite_batch_set_scissor_box(spritebatch_t* sb, int x, int y, int w, int h)
{
	sb->scissor_enabled = true;
	sb->scissor_x = x;
	sb->scissor_y = y;
	sb->scissor_w = w;
	sb->scissor_h = h;
}

void sprite_batch_no_scissor_box(spritebatch_t* sb)
{
	sb->scissor_enabled = false;
}

void sprite_batch_outlines_use_border(spritebatch_t* sb, bool use_border)
{
	sb->outline_use_border = use_border ? 1.0f : 0;
}

error_t sprite_batch_enable_disk_LRU_cache(spritebatch_t* sb, const char** image_paths, int image_paths_count, size_t cache_size_in_bytes)
{
	if (sb->mode != SPRITE_BATCH_MODE_UNINITIALIZED) {
		return error_failure("The sprite batch is already initialized.");
	}

	spritebatch_config_t config;
	spritebatch_set_default_config(&config);
	config.atlas_use_border_pixels = 1;
	config.batch_callback = s_batch_report;
	config.get_pixels_callback = s_get_pixels;
	config.generate_texture_callback = s_generate_texture_handle;
	config.delete_texture_callback = s_destroy_texture_handle;
	config.allocator_context = sb->mem_ctx;

	if (spritebatch_init(&sb->sb, &config, sb)) {
		return error_failure("Unable to initialize `spritebatch_t`.");
	}

	sb->mode = SPRITE_BATCH_MODE_LRU;
	sb->image_paths = image_paths;
	sb->image_paths_count = image_paths_count;
	sb->cache_capacity = cache_size_in_bytes;
	sb->cache = CUTE_NEW(sprite_cache_t, sb->mem_ctx)(256, sb->mem_ctx);

	return error_success();
}

error_t sprite_batch_LRU_cache_prefetch(spritebatch_t* sb, uint64_t id)
{
	if (sb->mode != SPRITE_BATCH_MODE_LRU) {
		return error_failure("Sprite batch is not in `SPRITE_BATCH_MODE_LRU` mode -- please call `sprite_batch_enable_disk_LRU_cache` to use this function.");
	}

	if (sb->cache->find(id)) {
		return error_success();
	}

	const char* path = sb->image_paths[id];
	void* data;
	size_t sz;
	error_t err = file_system_read_entire_file_to_memory(path, &data, &sz, sb->mem_ctx);
	CUTE_DEFER(CUTE_FREE(data, sb->mem_ctx));
	if (err.is_error()) return err;

	cp_image_t img = cp_load_png_mem(data, (int)sz);

	if (sz > sb->cache_capacity) {
		CUTE_FREE(img.pix, NULL);
		return error_failure("The entire image is larger than the cache.");
	}

	while (sz + sb->cache_used > sb->cache_capacity) {
		cached_image_t cached_image = *sb->cache->lru();
		sb->cache->remove(cached_image.id);
		sb->cache_used -= cached_image.size;
		s_free_cached_image(sb, &cached_image);
	}

	cached_image_t cached_image;
	cached_image.id = id;
	cached_image.path = path;
	cached_image.size = sizeof(cp_pixel_t) * img.w * img.h;
	cached_image.img = img;
	sb->cache->insert(cached_image.id, cached_image);

	return error_success();
}

void sprite_batch_LRU_cache_clear(spritebatch_t* sb)
{
	s_free_all_images_in_cache(sb);
	sb->cache->clear();
}

error_t sprite_batch_enable_custom_pixel_loader(spritebatch_t* sb, void (*get_pixels_fn)(uint64_t image_id, void* buffer, int bytes_to_fill, void* udata), void* udata)
{
	spritebatch_config_t config;
	spritebatch_set_default_config(&config);
	config.atlas_use_border_pixels = 1;
	config.batch_callback = s_batch_report;
	config.get_pixels_callback = get_pixels_fn;
	config.generate_texture_callback = s_generate_texture_handle;
	config.delete_texture_callback = s_destroy_texture_handle;
	config.allocator_context = sb->mem_ctx;

	if (spritebatch_init(&sb->sb, &config, udata)) {
		return error_failure("Unable to initialize `spritebatch_t`.");
	}

	sb->mode = SPRITE_BATCH_MODE_CUSTOM;

	return error_success();
}

}
