/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_draw3d.h>
#include <cute_draw.h>
#include <cute_graphics.h>
#include <cute_alloc.h>
#include <cute_array.h>
#include <cute_map.h>
#include <cute_string.h>

#include <cute_sprite.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_draw_internal.h>
#include <internal/cute_graphics_internal.h>
#include <internal/cute_aseprite_cache_internal.h>

using namespace Cute;

// 3d mesh submission over the 2d command stream (see include/cute_draw3d.h for the full design
// and shader contract). Each coalescing group -- consecutive submissions of one mesh under
// identical shader/render-state/camera/uniform state -- becomes one CF_Command carrying a
// CF_MeshCmd3d. The 2d layer sort orders it against sprites/shapes/text, and s_process_command
// hands it back to cf_draw3d_process, which renders all of the group's instances in one
// instanced draw. Per-instance data (transform rows, uv rect, mesh attributes) rides reserved
// instance-rate vertex attributes appended onto the user's mesh on first contact.

// The reserved per-instance vertex attributes, packed in shader-contract order
// (locations 8-15: in_model0/1/2, in_uv_rect, in_nmat0/1/2, in_mesh_attributes).
struct CF_MeshInstance3d
{
	CF_V4 model0, model1, model2;
	CF_V4 uv_rect;
	CF_V4 nmat0, nmat1, nmat2;
	CF_V4 mesh_attributes;
};

// One name -> value binding captured from the live uniform state. Live entries own their data
// individually (updated in place by cf_draw3d_set_uniform); captured entries point into their
// command's single uniform_block allocation.
struct CF_Uniform3d
{
	const char* name; // Interned.
	CF_UniformType type;
	int array_length;
	int size;
	void* data;
};

struct CF_TextureBinding3d
{
	const char* name; // Interned.
	CF_Texture texture;
};

// One instance's sprite image, captured at submission (cf_draw3d_push_texture). The entry is
// the atlas-cache template (image id, dimensions, local uv sub-rect); resolution to a page
// texture + atlas uv rect happens per flush in cf_draw3d_process, which is also the usage
// signal the atlas compiler packs by.
struct CF_ImageRef3d
{
	atlas_cache_entry_t entry;
	bool bordered; // Atlased images carry a 1px border (inset one texel); premade rects don't.
};

// The per-command payload for a 3d mesh draw, owned by its CF_Command (freed via
// cf_draw3d_free_cmd when the command is destroyed). The command's own shader/render_state/
// layer/scissor/viewport fields carry the rest of the captured state.
struct CF_MeshCmd3d
{
	CF_Mesh mesh = { 0 };
	bool escape = false; // Mesh has its own instance buffer: drawn as-is, no reserved attributes.
	CF_M4x4 vp;          // projection * view, captured at submission (or composed live at replay).
	uint64_t state_version = 0; // s_draw3d->version at capture; cheap uniform/texture equality.
	Cute::Array<CF_MeshInstance3d> instances;
	// Draw list replay: instances borrowed from the baked list, exactly like CF_Command's
	// geoms_ref -- replays never deep-copy instance data. NULL for ordinary commands.
	const Cute::Array<CF_MeshInstance3d>* instances_ref = NULL;
	// Sprite texturing (cf_draw3d_push_texture): one image per instance, parallel to
	// `instances`. Presence splits coalescing (it changes what the shader samples); the
	// images themselves vary freely across instances.
	bool sprite_textured = false;
	Cute::Array<CF_ImageRef3d> image_refs;
	const Cute::Array<CF_ImageRef3d>* image_refs_ref = NULL;
	Cute::Array<CF_Uniform3d> uniforms; // data points into uniform_block.
	void* uniform_block = NULL;         // One allocation holding every captured uniform's bytes.
	                                    // NULL when the bytes are borrowed (replay payloads).
	Cute::Array<CF_TextureBinding3d> textures;
	// Baked lists: the instance data lives on the GPU permanently, uploaded once at bake, so
	// replays skip the per-flush CPU upload entirely. Replay payloads borrow the handle.
	uint64_t gpu_instances = 0;
	bool owns_gpu_instances = false;

	~CF_MeshCmd3d()
	{
		if (uniform_block) CF_FREE(uniform_block);
		if (owns_gpu_instances && gpu_instances) cf_destroy_instance_buffer(gpu_instances);
	}
};

struct CF_Draw3d
{
	// Camera and transform stacks (cf_draw3d_push_projection/push_view/push).
	Cute::Array<CF_M4x4> projections;
	Cute::Array<CF_M4x4> views;
	Cute::Array<CF_M4x4> transforms;

	// Pipeline state stacks.
	Cute::Array<CF_Shader> shaders;
	Cute::Array<CF_RenderState> render_states;
	Cute::Array<CF_V4> mesh_attributes;
	Cute::Array<const CF_Sprite*> sprites; // cf_draw3d_push_texture stack.

	// Live uniform/texture state (cf_draw3d_set_uniform/set_texture). `version` bumps on every
	// change that alters bytes or handles, so submissions compare one integer instead of
	// re-hashing the whole set when deciding whether to coalesce.
	Cute::Array<CF_Uniform3d> uniforms; // data individually owned.
	Cute::Array<CF_TextureBinding3d> textures;
	uint64_t version = 1;

	CF_Material material = { 0 };

	// The shared unit quad behind cf_draw3d_sprite, made on first use (pos + uv, centered,
	// uv (0,0) at the image's top-left like 2d sprites).
	CF_Mesh sprite_quad = { 0 };

	// Shape drawing (see the shapes section): color stack plus lazily-built stroke quad,
	// solid meshes, and built-in shaders, all created on first shape call.
	Cute::Array<CF_Color> colors;
	CF_Mesh stroke_quad = { 0 };
	CF_Shader stroke_shd = { 0 };
	CF_Shader solid_shd = { 0 };
	CF_RenderState stroke_rs;
	CF_Mesh cube_mesh = { 0 };
	CF_Mesh sphere_mesh = { 0 };
	CF_Mesh cone_mesh = { 0 };
	Cute::Map<CF_Mesh> torus_meshes; // Keyed by quantized tube/major ratio.

	// Sprite-texturing scratch, valid only inside cf_draw3d_process: the atlas report hook
	// routes uv results here (parallel to the command's instances) while `resolving` is set,
	// and `staged` holds uv-filled instance copies for the per-page uploads.
	bool resolving = false;
	Cute::Array<CF_PendingUV> resolve_uvs;
	Cute::Array<CF_MeshInstance3d> staged;
	Cute::Array<CF_MeshInstance3d> page;
};

static CF_Draw3d* s_draw3d;

void cf_make_draw3d()
{
	s_draw3d = CF_NEW(CF_Draw3d);
	s_draw3d->projections.add(cf_m4_identity());
	s_draw3d->views.add(cf_m4_identity());
	s_draw3d->transforms.add(cf_m4_identity());
	CF_Shader none = { 0 };
	s_draw3d->shaders.add(none); // No default 3d shader -- see cf_draw3d_push_shader.
	s_draw3d->render_states.add(cf_render_state_3d_defaults());
	s_draw3d->mesh_attributes.add(cf_v4(0));
	s_draw3d->sprites.add(NULL);
	s_draw3d->colors.add(cf_color_white());
	s_draw3d->material = cf_make_material();
}

void cf_draw3d_free_cmd(CF_Command* cmd)
{
	if (cmd->mesh3d) {
		cmd->mesh3d->~CF_MeshCmd3d();
		CF_FREE(cmd->mesh3d);
		cmd->mesh3d = NULL;
	}
}

void cf_destroy_draw3d()
{
	// Mesh commands still sitting in the stream (recorded but never rendered) own their payloads.
	for (int i = 0; i < s_draw->cmds.count(); ++i) {
		cf_draw3d_free_cmd(&s_draw->cmds[i]);
	}
	for (int i = 0; i < s_draw3d->uniforms.count(); ++i) {
		CF_FREE(s_draw3d->uniforms[i].data);
	}
	if (s_draw3d->sprite_quad.id) cf_destroy_mesh(s_draw3d->sprite_quad);
	if (s_draw3d->stroke_quad.id) {
		cf_destroy_mesh(s_draw3d->stroke_quad);
		cf_destroy_shader(s_draw3d->stroke_shd);
		cf_destroy_shader(s_draw3d->solid_shd);
	}
	if (s_draw3d->cube_mesh.id) cf_destroy_mesh(s_draw3d->cube_mesh);
	if (s_draw3d->sphere_mesh.id) cf_destroy_mesh(s_draw3d->sphere_mesh);
	if (s_draw3d->cone_mesh.id) cf_destroy_mesh(s_draw3d->cone_mesh);
	for (int i = 0; i < s_draw3d->torus_meshes.count(); ++i) {
		cf_destroy_mesh(s_draw3d->torus_meshes.items()[i]);
	}
	cf_destroy_material(s_draw3d->material);
	s_draw3d->~CF_Draw3d();
	CF_FREE(s_draw3d);
	s_draw3d = NULL;
}

//--------------------------------------------------------------------------------------------------
// Camera.

void cf_draw3d_push_projection(CF_M4x4 projection) { s_draw3d->projections.add(projection); }
CF_M4x4 cf_draw3d_pop_projection() { if (s_draw3d->projections.count() > 1) return s_draw3d->projections.pop(); return s_draw3d->projections.last(); }
CF_M4x4 cf_draw3d_peek_projection() { return s_draw3d->projections.last(); }
void cf_draw3d_push_view(CF_M4x4 view) { s_draw3d->views.add(view); }
CF_M4x4 cf_draw3d_pop_view() { if (s_draw3d->views.count() > 1) return s_draw3d->views.pop(); return s_draw3d->views.last(); }
CF_M4x4 cf_draw3d_peek_view() { return s_draw3d->views.last(); }

//--------------------------------------------------------------------------------------------------
// Model transform stack.

void cf_draw3d_push() { s_draw3d->transforms.add(s_draw3d->transforms.last()); }
void cf_draw3d_pop() { if (s_draw3d->transforms.count() > 1) s_draw3d->transforms.pop(); }
void cf_draw3d_transform(CF_M4x4 m) { s_draw3d->transforms.last() = cf_mul_m4(s_draw3d->transforms.last(), m); }
void cf_draw3d_translate(CF_V3 t) { cf_draw3d_transform(cf_m4_translate(t)); }
void cf_draw3d_rotate(CF_Quat q) { cf_draw3d_transform(cf_quat_to_m4(q)); }
void cf_draw3d_scale(CF_V3 s) { cf_draw3d_transform(cf_m4_scale(s)); }
CF_M4x4 cf_draw3d_peek_transform() { return s_draw3d->transforms.last(); }
CF_V3 cf_draw3d_mul(CF_V3 p) { return cf_m4_transform_point(s_draw3d->transforms.last(), p); }

//--------------------------------------------------------------------------------------------------
// Shader and pipeline state.

void cf_draw3d_push_shader(CF_Shader shader) { s_draw3d->shaders.add(shader); }
CF_Shader cf_draw3d_pop_shader() { if (s_draw3d->shaders.count() > 1) return s_draw3d->shaders.pop(); return s_draw3d->shaders.last(); }
CF_Shader cf_draw3d_peek_shader() { return s_draw3d->shaders.last(); }
void cf_draw3d_push_render_state(CF_RenderState render_state) { s_draw3d->render_states.add(render_state); }
CF_RenderState cf_draw3d_pop_render_state() { if (s_draw3d->render_states.count() > 1) return s_draw3d->render_states.pop(); return s_draw3d->render_states.last(); }
CF_RenderState cf_draw3d_peek_render_state() { return s_draw3d->render_states.last(); }

//--------------------------------------------------------------------------------------------------
// Uniforms and textures.

void cf_draw3d_set_uniform(const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_ASSERT(array_length > 0);
	name = sintern(name);
	// u_view_projection is fed from the camera stacks and cannot be overridden.
	CF_ASSERT(name != sintern("u_view_projection"));
	int size = s_uniform_size(type) * array_length;
	for (int i = 0; i < s_draw3d->uniforms.count(); ++i) {
		CF_Uniform3d& u = s_draw3d->uniforms[i];
		if (u.name == name) {
			if (u.type == type && u.array_length == array_length && !CF_MEMCMP(u.data, data, size)) {
				return; // Unchanged; keep coalescing alive.
			}
			if (u.size != size) {
				CF_FREE(u.data);
				u.data = CF_ALLOC(size);
			}
			u.type = type;
			u.array_length = array_length;
			u.size = size;
			CF_MEMCPY(u.data, data, size);
			s_draw3d->version++;
			return;
		}
	}
	CF_Uniform3d u;
	u.name = name;
	u.type = type;
	u.array_length = array_length;
	u.size = size;
	u.data = CF_ALLOC(size);
	CF_MEMCPY(u.data, data, size);
	s_draw3d->uniforms.add(u);
	s_draw3d->version++;
}

void cf_draw3d_set_uniform_int(const char* name, int val) { cf_draw3d_set_uniform(name, &val, CF_UNIFORM_TYPE_INT, 1); }
void cf_draw3d_set_uniform_float(const char* name, float val) { cf_draw3d_set_uniform(name, &val, CF_UNIFORM_TYPE_FLOAT, 1); }
void cf_draw3d_set_uniform_v2(const char* name, CF_V2 val) { cf_draw3d_set_uniform(name, &val, CF_UNIFORM_TYPE_FLOAT2, 1); }
void cf_draw3d_set_uniform_v3(const char* name, CF_V3 val) { cf_draw3d_set_uniform(name, &val, CF_UNIFORM_TYPE_FLOAT3, 1); }
void cf_draw3d_set_uniform_m4(const char* name, CF_M4x4 val) { cf_draw3d_set_uniform(name, &val, CF_UNIFORM_TYPE_MAT4, 1); }
void cf_draw3d_set_uniform_color(const char* name, CF_Color val) { cf_draw3d_set_uniform(name, &val, CF_UNIFORM_TYPE_FLOAT4, 1); }

void cf_draw3d_set_texture(const char* name, CF_Texture texture)
{
	name = sintern(name);
	for (int i = 0; i < s_draw3d->textures.count(); ++i) {
		CF_TextureBinding3d& t = s_draw3d->textures[i];
		if (t.name == name) {
			if (t.texture.id == texture.id) return;
			t.texture = texture;
			s_draw3d->version++;
			return;
		}
	}
	CF_TextureBinding3d t;
	t.name = name;
	t.texture = texture;
	s_draw3d->textures.add(t);
	s_draw3d->version++;
}

//--------------------------------------------------------------------------------------------------
// Sprite texturing. The stack lands here; the atlas plumbing (u_image binding and live uv_rect
// lanes) rides in with the draw-list bake work.

void cf_draw3d_push_texture(const CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	s_draw3d->sprites.add(sprite);
}

void cf_draw3d_pop_texture()
{
	if (s_draw3d->sprites.count() > 1) s_draw3d->sprites.pop();
}

const CF_Sprite* cf_draw3d_peek_texture() { return s_draw3d->sprites.last(); }

//--------------------------------------------------------------------------------------------------
// Mesh attributes.

void cf_draw3d_push_mesh_attributes(CF_V4 attributes) { s_draw3d->mesh_attributes.add(attributes); }
CF_V4 cf_draw3d_pop_mesh_attributes() { if (s_draw3d->mesh_attributes.count() > 1) return s_draw3d->mesh_attributes.pop(); return s_draw3d->mesh_attributes.last(); }
CF_V4 cf_draw3d_peek_mesh_attributes() { return s_draw3d->mesh_attributes.last(); }

//--------------------------------------------------------------------------------------------------
// Submission.

// Row r of a column-major 4x4 -- the shader contract delivers the model transform as the three
// rows of its affine 4x3 (world_pos.x = dot(in_model0, vec4(pos, 1)), etc).
static CF_INLINE CF_V4 s_row(const CF_M4x4& m, int r)
{
	return cf_v4(m.elements[0 + r], m.elements[4 + r], m.elements[8 + r], m.elements[12 + r]);
}

static CF_MeshInstance3d s_instance()
{
	CF_MeshInstance3d inst;
	const CF_M4x4& model = s_draw3d->transforms.last();
	inst.model0 = s_row(model, 0);
	inst.model1 = s_row(model, 1);
	inst.model2 = s_row(model, 2);
	inst.uv_rect = cf_v4(0, 0, 1, 1);
	// The immediate path reuses the model rows as the normal matrix (exact for rigid transforms
	// and uniform scale -- see the header); baked draw lists compute exact ones.
	inst.nmat0 = cf_v4(inst.model0.x, inst.model0.y, inst.model0.z, 0);
	inst.nmat1 = cf_v4(inst.model1.x, inst.model1.y, inst.model1.z, 0);
	inst.nmat2 = cf_v4(inst.model2.x, inst.model2.y, inst.model2.z, 0);
	inst.mesh_attributes = s_draw3d->mesh_attributes.last();
	return inst;
}

// Captures a sprite's current frame as an atlas entry template, mirroring cf_draw_sprite's
// image-id resolution (asset sprites, blend layers, easy sprites, premade sub-images).
static CF_ImageRef3d s_image_ref(const CF_Sprite* sprite)
{
	CF_ImageRef3d ref = { };
	atlas_cache_entry_t& s = ref.entry;
	s.minx = 0;
	s.miny = 0;
	s.maxx = 1;
	s.maxy = 1;
	ref.bordered = true;
	if (sprite->id != CF_SPRITE_ID_INVALID) {
		if (sprite->blend_index > 0) {
			CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
			const char* anim_name = sprite->animation_name;
			const CF_Animation* anim = anim_name ? map_get(asset->animations, anim_name) : NULL;
			int global_frame = sprite->frame_index + (anim ? anim->frame_offset : 0);
			s.image_id = asset->blend_frame_ids[sprite->blend_index][global_frame];
		} else {
			s.image_id = sprite->_image_id;
		}
	} else if (sprite->easy_sprite_id >= CF_PREMADE_ID_RANGE_LO && sprite->easy_sprite_id <= CF_PREMADE_ID_RANGE_HI) {
		CF_AtlasSubImage sub_image = s_draw->premade_sub_image_id_to_sub_image.find(sprite->easy_sprite_id);
		s.minx = sub_image.minx;
		s.maxx = sub_image.maxx;
		s.miny = sub_image.miny;
		s.maxy = sub_image.maxy;
		s.image_id = sprite->easy_sprite_id;
		s.texture_id = sub_image.image_id; // @JANK - Hijacked to store texture_id, matching cf_draw_sprite.
		ref.bordered = false;
	} else {
		s.image_id = sprite->easy_sprite_id;
	}
	s.w = sprite->w;
	s.h = sprite->h;
	return ref;
}

// Routes atlas uv results to the mesh command being resolved in cf_draw3d_process (the 2d
// batch callback calls this first and falls through to its own table otherwise).
bool cf_draw3d_atlas_report(atlas_cache_entry_t* entries, int count, int texture_w, int texture_h)
{
	if (!s_draw3d || !s_draw3d->resolving) return false;
	for (int i = 0; i < count; ++i) {
		const atlas_cache_entry_t* s = entries + i;
		CF_PendingUV& uv = s_draw3d->resolve_uvs[(int)s->udata];
		uv.texture_id = s->texture_id;
		uv.minx = s->minx;
		uv.miny = s->miny;
		uv.maxx = s->maxx;
		uv.maxy = s->maxy;
		uv.tex_w = texture_w;
		uv.tex_h = texture_h;
	}
	return true;
}

// A mesh submission leaves a fresh empty command on top of the stream so subsequent 2d drawing
// never lands on the mesh command (mirroring cf_draw_canvas). Coalescing therefore looks at the
// command *under* the top, provided the top is still untouched.
static CF_Command* s_coalesce_candidate()
{
	int n = s_draw->cmds.count();
	if (n < 2) return NULL;
	CF_Command& top = s_draw->cmds[n - 1];
	if (top.mesh3d || top.is_canvas || top.geoms.count() || top.items.count() || top.geoms_ref || top.u.name || top.u.is_texture) return NULL;
	CF_Command& under = s_draw->cmds[n - 2];
	return under.mesh3d ? &under : NULL;
}

// The shared submission tail behind cf_draw3d_mesh and the built-in shapes: coalesce or open
// a new command carrying `inst` (already built -- from the transform stack for user meshes,
// raw shape lanes for strokes).
static void s_submit(CF_Mesh mesh, const CF_MeshInstance3d& inst, bool escape, const CF_Sprite* sprite)
{
	CF_Shader shader = s_draw3d->shaders.last();
	CF_ASSERT(shader.id); // No default 3d shader -- push one first (see the shader contract in cute_draw3d.h).
	if (!shader.id) return;

	CF_M4x4 vp = cf_mul_m4(s_draw3d->projections.last(), s_draw3d->views.last());
	CF_RenderState rs = s_draw3d->render_states.last();
	bool sprite_textured = sprite && !escape;

	if (!escape) {
		CF_Command* under = s_coalesce_candidate();
		if (under) {
			CF_MeshCmd3d* mc = under->mesh3d;
			if (mc->mesh.id == mesh.id && !mc->escape && !mc->instances_ref
				&& mc->sprite_textured == sprite_textured
				&& mc->state_version == s_draw3d->version
				&& !CF_MEMCMP(&mc->vp, &vp, sizeof(vp))
				&& under->shader.id == shader.id
				&& under->render_state == rs
				&& under->layer == s_draw->layers.last()
				&& under->scissor == s_draw->scissors.last()
				&& under->viewport == s_draw->viewports.last()) {
				mc->instances.add(inst);
				if (sprite_textured) mc->image_refs.add(s_image_ref(sprite));
				return;
			}
		}
	}

	CF_Command& cmd = s_draw->add_cmd();
	cmd.shader = shader;
	cmd.render_state = rs;
	CF_MeshCmd3d* mc = CF_NEW(CF_MeshCmd3d);
	cmd.mesh3d = mc;
	mc->mesh = mesh;
	mc->escape = escape;
	mc->vp = vp;
	mc->state_version = s_draw3d->version;

	// Capture the live uniform values into one command-owned block.
	int total = 0;
	for (int i = 0; i < s_draw3d->uniforms.count(); ++i) total += s_draw3d->uniforms[i].size;
	if (total) {
		mc->uniform_block = CF_ALLOC(total);
		int offset = 0;
		for (int i = 0; i < s_draw3d->uniforms.count(); ++i) {
			CF_Uniform3d u = s_draw3d->uniforms[i];
			void* dst = (char*)mc->uniform_block + offset;
			CF_MEMCPY(dst, u.data, u.size);
			u.data = dst;
			mc->uniforms.add(u);
			offset += u.size;
		}
	}
	mc->textures = s_draw3d->textures;
	mc->sprite_textured = sprite_textured;

	if (!escape) {
		mc->instances.add(inst);
		if (sprite_textured) mc->image_refs.add(s_image_ref(sprite));
	}

	// Keep subsequent 2d drawing off this command.
	s_draw->add_cmd();
}

void cf_draw3d_mesh(CF_Mesh mesh)
{
	CF_ASSERT(mesh.id);
	bool ours = cf_mesh_has_vertex_attribute(mesh, "in_model0");
	bool escape = !ours && cf_mesh_instance_stride(mesh) != 0;
	CF_MeshInstance3d inst;
	if (!escape) inst = s_instance();
	else CF_MEMSET(&inst, 0, sizeof(inst));
	s_submit(mesh, inst, escape, s_draw3d->sprites.last());
}

// The shared quad + submission tail behind cf_draw3d_sprite and cf_draw3d_billboard. The
// `basis` orients the quad (NULL keeps the transform stack's orientation).
static void s_submit_sprite_quad(const CF_Sprite* sprite, CF_V3 position, const CF_M4x4* basis)
{
	CF_ASSERT(sprite);
	if (!s_draw3d->sprite_quad.id) {
		struct QuadVertex { CF_V3 pos; CF_V2 uv; };
		QuadVertex verts[6] = {
			{ { -0.5f, -0.5f, 0 }, { 0, 1 } }, { { 0.5f, -0.5f, 0 }, { 1, 1 } }, { { 0.5f, 0.5f, 0 }, { 1, 0 } },
			{ { -0.5f, -0.5f, 0 }, { 0, 1 } }, { { 0.5f, 0.5f, 0 }, { 1, 0 } }, { { -0.5f, 0.5f, 0 }, { 0, 0 } },
		};
		CF_VertexAttribute attrs[2] = { };
		attrs[0].name = "in_pos";
		attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
		attrs[0].offset = CF_OFFSET_OF(QuadVertex, pos);
		attrs[1].name = "in_uv";
		attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
		attrs[1].offset = CF_OFFSET_OF(QuadVertex, uv);
		s_draw3d->sprite_quad = cf_make_mesh((int)sizeof(verts), attrs, 2, (int)sizeof(QuadVertex));
		cf_mesh_update_vertex_data(s_draw3d->sprite_quad, verts, 6);
	}

	cf_draw3d_push();
	cf_draw3d_translate(position);
	if (basis) cf_draw3d_transform(*basis);
	cf_draw3d_scale(cf_v3((float)sprite->w * sprite->scale.x, (float)sprite->h * sprite->scale.y, 1.0f));
	cf_draw3d_push_texture(sprite);
	cf_draw3d_mesh(s_draw3d->sprite_quad);
	cf_draw3d_pop_texture();
	cf_draw3d_pop();
}

void cf_draw3d_sprite(const CF_Sprite* sprite, CF_V3 position)
{
	s_submit_sprite_quad(sprite, position, NULL);
}

void cf_draw3d_billboard(const CF_Sprite* sprite, CF_V3 position)
{
	// Aim the quad with the camera's own axes, straight out of the view matrix's rows.
	const CF_M4x4& view = s_draw3d->views.last();
	CF_M4x4 basis = cf_m4_identity();
	basis.elements[0] = view.elements[0]; basis.elements[1] = view.elements[4]; basis.elements[2]  = view.elements[8];
	basis.elements[4] = view.elements[1]; basis.elements[5] = view.elements[5]; basis.elements[6]  = view.elements[9];
	basis.elements[8] = view.elements[2]; basis.elements[9] = view.elements[6]; basis.elements[10] = view.elements[10];
	s_submit_sprite_quad(sprite, position, &basis);
}

//--------------------------------------------------------------------------------------------------
// Shapes: SDF strokes and solid primitives, the 3d analog of cute_draw.h's shape drawing.
//
// Strokes (lines, polylines, rings, arcs) are instances of one shared corner quad under a
// built-in shader: the vertex stage expands a camera-facing ribbon (or a flat disc quad)
// padded by an anti-aliasing band, and the fragment stage evaluates the shape's signed
// distance with a screen-space fade -- local AA that needs no MSAA, plus thin-line fading
// (sub-pixel strokes clamp to half a pixel of width and fade alpha instead of shimmering).
// Shape parameters ride the reserved instance lanes reinterpreted as raw data, so strokes
// coalesce, layer-sort, and record into draw lists exactly like meshes -- a wireframe level
// bakes into one persistent-buffer instanced draw.
//
// Solids (cube, sphere, cone, torus) are lazily-built unit meshes drawn through the ordinary
// submission path under a built-in hemisphere-lit shader, colored by the color stack.

static const char* s_stroke_vs =
"layout (location = 0) in vec2 in_corner;\n"
"layout (location = 8)  in vec4 in_model0;\n"          // Segment: a.xyz, half-thickness at a. Ring: center.xyz, radius.
"layout (location = 9)  in vec4 in_model1;\n"          // Segment: b.xyz, half-thickness at b. Ring: normal.xyz, half-thickness.
"layout (location = 11) in vec4 in_uv_rect;\n"         // Color (at a, for segments).
"layout (location = 12) in vec4 in_nmat0;\n"           // Color at b (segments).
"layout (location = 13) in vec4 in_nmat1;\n"           // x: kind (0 segment, 1 ring). y: angle0, z: sweep, w: fill flag.
"layout (location = 0) out vec2 v_local;\n"
"layout (location = 1) out vec4 v_seg;\n"              // Segment: len, ht0, ht1, ppw. Ring: radius, ht, ppw, fill.
"layout (location = 2) out vec4 v_c0;\n"
"layout (location = 3) out vec4 v_c1;\n"
"layout (location = 4) out vec4 v_arc;\n"              // cos/sin of arc edge directions (rings).
"layout (location = 5) out float v_kind;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"    vec4 u_shape_eye;\n"
"    vec4 u_shape_res;\n"
"};\n"
"void main() {\n"
"    float kind = in_nmat1.x;\n"
"    vec3 world;\n"
"    if (kind < 0.5) {\n"
"        vec3 a = in_model0.xyz;\n"
"        vec3 b = in_model1.xyz;\n"
"        vec3 d = b - a;\n"
"        float len = length(d);\n"
"        vec3 dir = len > 0.0001 ? d / len : vec3(1, 0, 0);\n"
"        vec3 mid = (a + b) * 0.5;\n"
"        vec3 across = normalize(cross(dir, u_shape_eye.xyz - mid));\n"
"        vec4 cc = u_view_projection * vec4(mid, 1.0);\n"
"        vec4 ca = u_view_projection * vec4(mid + across, 1.0);\n"
"        vec2 ppx = (ca.xy / max(ca.w, 0.0001) - cc.xy / max(cc.w, 0.0001)) * u_shape_res.xy * 0.5;\n"
"        float ppw = max(length(ppx), 0.0001);\n"
"        float aa = 3.0 / ppw;\n"
"        float htmax = max(in_model0.w, in_model1.w);\n"
"        float ext = max(htmax, 0.5 / ppw) + aa;\n"
"        float s = mix(-ext, len + ext, in_corner.x + 0.5);\n"
"        float t = ext * in_corner.y * 2.0;\n"
"        world = a + dir * s + across * t;\n"
"        v_local = vec2(s, t);\n"
"        v_seg = vec4(len, in_model0.w, in_model1.w, ppw);\n"
"    } else {\n"
"        vec3 center = in_model0.xyz;\n"
"        vec3 n = normalize(in_model1.xyz);\n"
"        vec3 h = abs(n.y) < 0.99 ? vec3(0, 1, 0) : vec3(1, 0, 0);\n"
"        vec3 bx = normalize(cross(h, n));\n"
"        vec3 by = cross(n, bx);\n"
"        vec4 cc = u_view_projection * vec4(center, 1.0);\n"
"        vec4 ca = u_view_projection * vec4(center + bx, 1.0);\n"
"        vec2 ppx = (ca.xy / max(ca.w, 0.0001) - cc.xy / max(cc.w, 0.0001)) * u_shape_res.xy * 0.5;\n"
"        float ppw = max(length(ppx), 0.0001);\n"
"        float aa = 3.0 / ppw;\n"
"        float ext = in_model0.w + max(in_model1.w, 0.5 / ppw) + aa;\n"
"        vec2 l = in_corner * 2.0 * ext;\n"
"        world = center + bx * l.x + by * l.y;\n"
"        v_local = l;\n"
"        v_seg = vec4(in_model0.w, in_model1.w, ppw, in_nmat1.w);\n"
"    }\n"
"    float a0 = in_nmat1.y;\n"
"    float a1 = in_nmat1.y + in_nmat1.z;\n"
"    v_arc = vec4(cos(a0), sin(a0), cos(a1), sin(a1));\n"
"    v_c0 = in_uv_rect;\n"
"    v_c1 = in_nmat0;\n"
"    v_kind = kind + (in_nmat1.z >= 6.28 ? 2.0 : 0.0);\n" // +2 marks a full ring (skip arc clipping).
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

static const char* s_stroke_fs =
"layout (location = 0) in vec2 v_local;\n"
"layout (location = 1) in vec4 v_seg;\n"
"layout (location = 2) in vec4 v_c0;\n"
"layout (location = 3) in vec4 v_c1;\n"
"layout (location = 4) in vec4 v_arc;\n"
"layout (location = 5) in float v_kind;\n"
"layout (location = 0) out vec4 result;\n"
"void main() {\n"
"    float raw;\n" // Signed distance to the shape's skeleton, in world units.
"    float ht;\n"  // Stroke half-thickness; negative marks a filled shape (edge AA only).
"    vec4 col;\n"
"    if (v_kind < 0.5) {\n"
"        float len = v_seg.x;\n"
"        float sc = clamp(v_local.x, 0.0, len);\n"
"        float f = len > 0.0001 ? sc / len : 0.0;\n"
"        ht = mix(v_seg.y, v_seg.z, f);\n"
"        raw = length(vec2(v_local.x - sc, v_local.y));\n"
"        col = mix(v_c0, v_c1, f);\n"
"    } else {\n"
"        float radius = v_seg.x;\n"
"        ht = v_seg.y;\n"
"        col = v_c0;\n"
"        if (v_seg.w > 0.5) {\n"
"            raw = length(v_local) - radius;\n" // Filled disc.
"            ht = -1.0;\n"
"        } else {\n"
"            bool inside = true;\n"
"            if (v_kind < 2.5) {\n"
"                // Arc: half-plane containment against the edge directions (no atan needed).\n"
"                vec2 e0 = v_arc.xy;\n"
"                vec2 e1 = v_arc.zw;\n"
"                float c0 = e0.x * v_local.y - e0.y * v_local.x;\n"
"                float c1 = e1.x * v_local.y - e1.y * v_local.x;\n"
"                bool wide = (e0.x * e1.y - e0.y * e1.x) < 0.0;\n"
"                inside = wide ? (c0 >= 0.0 || c1 <= 0.0) : (c0 >= 0.0 && c1 <= 0.0);\n"
"            }\n"
"            if (inside) {\n"
"                raw = abs(length(v_local) - radius);\n"
"            } else {\n"
"                vec2 p0 = v_arc.xy * radius;\n"
"                vec2 p1 = v_arc.zw * radius;\n"
"                raw = min(length(v_local - p0), length(v_local - p1));\n" // Round arc ends.
"            }\n"
"        }\n"
"    }\n"
"    // Per-fragment anti-aliasing: fwidth(raw) is how many world units one pixel spans HERE,\n"
"    // exact under perspective where any per-instance estimate goes soft near and hard far.\n"
"    // Strokes thinner than a pixel clamp to half-pixel width and fade alpha instead.\n"
"    float wpp = max(fwidth(raw), 0.000001);\n"
"    float fade = 1.0;\n"
"    float d;\n"
"    if (ht >= 0.0) {\n"
"        float ht_eff = max(ht, 0.5 * wpp);\n"
"        fade = clamp(ht / (0.5 * wpp), 0.0, 1.0);\n"
"        d = raw - ht_eff;\n"
"    } else {\n"
"        d = raw;\n"
"    }\n"
"    float alpha = clamp(0.5 - d / wpp, 0.0, 1.0) * fade * col.a;\n"
"    result = vec4(col.rgb * alpha, alpha); // Premultiplied.\n"
"}\n";

static const char* s_solid_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec3 in_normal;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 12) in vec4 in_nmat0;\n"
"layout (location = 13) in vec4 in_nmat1;\n"
"layout (location = 14) in vec4 in_nmat2;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec3 v_normal;\n"
"layout (location = 1) out vec4 v_color;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_normal = normalize(vec3(dot(in_nmat0.xyz, in_normal), dot(in_nmat1.xyz, in_normal), dot(in_nmat2.xyz, in_normal)));\n"
"    v_color = in_mesh_attributes;\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

static const char* s_solid_fs =
"layout (location = 0) in vec3 v_normal;\n"
"layout (location = 1) in vec4 v_color;\n"
"layout (location = 0) out vec4 result;\n"
"void main() {\n"
"    vec3 n = normalize(v_normal);\n"
"    float key = max(dot(n, normalize(vec3(0.4, 0.75, 0.5))), 0.0);\n"
"    float hemi = 0.5 + 0.5 * n.y;\n"
"    result = vec4(v_color.rgb * (0.35 + 0.15 * hemi + 0.5 * key), v_color.a);\n"
"}\n";

// Lazily builds the stroke quad, solid meshes, and built-in shaders on first shape use.
static void s_shapes_init()
{
	if (s_draw3d->stroke_quad.id) return;
	CF_V2 corners[6] = { { -0.5f, -0.5f }, { 0.5f, -0.5f }, { 0.5f, 0.5f }, { -0.5f, -0.5f }, { 0.5f, 0.5f }, { -0.5f, 0.5f } };
	CF_VertexAttribute cattr[1] = { };
	cattr[0].name = "in_corner";
	cattr[0].format = CF_VERTEX_FORMAT_FLOAT2;
	cattr[0].offset = 0;
	s_draw3d->stroke_quad = cf_make_mesh((int)sizeof(corners), cattr, 1, (int)sizeof(CF_V2));
	cf_mesh_update_vertex_data(s_draw3d->stroke_quad, corners, 6);
	s_draw3d->stroke_shd = cf_make_shader_from_source(s_stroke_vs, s_stroke_fs);
	s_draw3d->solid_shd = cf_make_shader_from_source(s_solid_vs, s_solid_fs);

	// Stroke render state: premultiplied blend, depth test but no writes, no culling. Strokes
	// are anti-aliased and translucent-edged by construction, so they never own the depth
	// buffer; solids use whatever render state the user has pushed (3d defaults work).
	s_draw3d->stroke_rs = cf_render_state_3d_defaults();
	s_draw3d->stroke_rs.cull_mode = CF_CULL_MODE_NONE;
	s_draw3d->stroke_rs.depth_write_enabled = false;
	s_draw3d->stroke_rs.blend.enabled = true;
	s_draw3d->stroke_rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	s_draw3d->stroke_rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	s_draw3d->stroke_rs.blend.rgb_op = CF_BLEND_OP_ADD;
	s_draw3d->stroke_rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	s_draw3d->stroke_rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	s_draw3d->stroke_rs.blend.alpha_op = CF_BLEND_OP_ADD;
}

// Camera info for the stroke vertex stage, refreshed whenever the view changes (set_uniform
// dedups identical bytes, so this only bumps the coalescing version on real camera motion).
static void s_shapes_set_eye()
{
	const CF_M4x4& v = s_draw3d->views.last();
	// view = [R | t]; the camera's world position satisfies R*eye + t = 0 -> eye = -R^T t.
	CF_V3 t = cf_v3(v.elements[12], v.elements[13], v.elements[14]);
	CF_V4 eye = cf_v4(
		-(v.elements[0] * t.x + v.elements[1] * t.y + v.elements[2] * t.z),
		-(v.elements[4] * t.x + v.elements[5] * t.y + v.elements[6] * t.z),
		-(v.elements[8] * t.x + v.elements[9] * t.y + v.elements[10] * t.z), 1.0f);
	cf_draw3d_set_uniform("u_shape_eye", &eye, CF_UNIFORM_TYPE_FLOAT4, 1);
}

// Submits one stroke instance under the built-in stroke shader and render state. The current
// transform stack has already been applied to the world-space inputs by the callers.
static void s_submit_stroke(const CF_MeshInstance3d& inst)
{
	s_shapes_init();
	s_shapes_set_eye();
	cf_draw3d_push_shader(s_draw3d->stroke_shd);
	cf_draw3d_push_render_state(s_draw3d->stroke_rs);
	s_submit(s_draw3d->stroke_quad, inst, false, NULL);
	cf_draw3d_pop_render_state();
	cf_draw3d_pop_shader();
}

void cf_draw3d_push_color(CF_Color c) { s_draw3d->colors.add(c); }
CF_Color cf_draw3d_pop_color() { if (s_draw3d->colors.count() > 1) return s_draw3d->colors.pop(); return s_draw3d->colors.last(); }
CF_Color cf_draw3d_peek_color() { return s_draw3d->colors.last(); }

static CF_INLINE CF_V4 s_color4() { CF_Color c = s_draw3d->colors.last(); return cf_v4(c.r, c.g, c.b, c.a); }

void cf_draw3d_line2(CF_V3 p0, CF_V3 p1, float thickness0, float thickness1)
{
	CF_MeshInstance3d inst = { };
	CF_V3 a = cf_draw3d_mul(p0);
	CF_V3 b = cf_draw3d_mul(p1);
	inst.model0 = cf_v4(a.x, a.y, a.z, thickness0 * 0.5f);
	inst.model1 = cf_v4(b.x, b.y, b.z, thickness1 * 0.5f);
	inst.uv_rect = s_color4();
	inst.nmat0 = inst.uv_rect;
	inst.nmat1 = cf_v4(0, 0, 0, 0);
	s_submit_stroke(inst);
}

void cf_draw3d_line(CF_V3 p0, CF_V3 p1, float thickness) { cf_draw3d_line2(p0, p1, thickness, thickness); }

void cf_draw3d_polyline(const CF_V3* points, int count, float thickness, bool loop)
{
	if (count < 2) return;
	for (int i = 0; i < count - 1; ++i) cf_draw3d_line(points[i], points[i + 1], thickness);
	if (loop) cf_draw3d_line(points[count - 1], points[0], thickness);
}

static void s_ring(CF_V3 center, CF_V3 normal, float radius, float half_thickness, float a0, float sweep, bool fill)
{
	CF_MeshInstance3d inst = { };
	CF_V3 c = cf_draw3d_mul(center);
	// Rotate the plane normal by the transform's upper 3x3 (no translation).
	const CF_M4x4& m = s_draw3d->transforms.last();
	CF_V3 n = cf_v3(
		m.elements[0] * normal.x + m.elements[4] * normal.y + m.elements[8] * normal.z,
		m.elements[1] * normal.x + m.elements[5] * normal.y + m.elements[9] * normal.z,
		m.elements[2] * normal.x + m.elements[6] * normal.y + m.elements[10] * normal.z);
	inst.model0 = cf_v4(c.x, c.y, c.z, radius);
	inst.model1 = cf_v4(n.x, n.y, n.z, half_thickness);
	inst.uv_rect = s_color4();
	inst.nmat0 = inst.uv_rect;
	inst.nmat1 = cf_v4(1.0f, a0, sweep, fill ? 1.0f : 0.0f);
	s_submit_stroke(inst);
}

void cf_draw3d_circle(CF_V3 center, CF_V3 normal, float radius, float thickness)
{
	s_ring(center, normal, radius, thickness * 0.5f, 0, 7.0f, false);
}

void cf_draw3d_circle_fill(CF_V3 center, CF_V3 normal, float radius)
{
	s_ring(center, normal, radius, 0, 0, 7.0f, true);
}

void cf_draw3d_arc(CF_V3 center, CF_V3 normal, float radius, float angle0, float sweep, float thickness)
{
	s_ring(center, normal, radius, thickness * 0.5f, angle0, sweep, false);
}

void cf_draw3d_box_wire(CF_V3 center, CF_V3 half_extents, float thickness)
{
	CF_V3 c = center, e = half_extents;
	CF_V3 v[8];
	for (int i = 0; i < 8; ++i) {
		v[i] = cf_v3(c.x + (i & 1 ? e.x : -e.x), c.y + (i & 2 ? e.y : -e.y), c.z + (i & 4 ? e.z : -e.z));
	}
	int edges[12][2] = { {0,1},{2,3},{4,5},{6,7}, {0,2},{1,3},{4,6},{5,7}, {0,4},{1,5},{2,6},{3,7} };
	for (int i = 0; i < 12; ++i) cf_draw3d_line(v[edges[i][0]], v[edges[i][1]], thickness);
}

void cf_draw3d_axes(float length, float thickness)
{
	CF_Color colors[3] = { cf_make_color_rgb_f(0.9f, 0.2f, 0.25f), cf_make_color_rgb_f(0.3f, 0.85f, 0.3f), cf_make_color_rgb_f(0.25f, 0.5f, 0.95f) };
	CF_V3 axes[3] = { cf_v3(length, 0, 0), cf_v3(0, length, 0), cf_v3(0, 0, length) };
	for (int i = 0; i < 3; ++i) {
		cf_draw3d_push_color(colors[i]);
		cf_draw3d_line(cf_v3(0, 0, 0), axes[i], thickness);
		cf_draw3d_pop_color();
	}
}

void cf_draw3d_arrow(CF_V3 a, CF_V3 b, float thickness, float arrow_width)
{
	CF_V3 d = cf_sub_v3(b, a);
	float len = cf_len_v3(d);
	if (len < 0.0001f) return;
	CF_V3 dir = cf_div_v3_f(d, len);
	float head = arrow_width * 1.8f;
	if (head > len) head = len;
	CF_V3 neck = cf_add_v3(a, cf_mul_v3_f(dir, len - head));
	cf_draw3d_line(a, neck, thickness);
	cf_draw3d_cone(neck, b, arrow_width);
}

//--------------------------------------------------------------------------------------------------
// Solid primitives: lazily-built unit meshes through the ordinary submission path.

struct CF_SolidVertex { CF_V3 pos; CF_V3 n; };

static CF_Mesh s_make_solid(const Cute::Array<CF_SolidVertex>& verts)
{
	CF_VertexAttribute attrs[2] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = CF_OFFSET_OF(CF_SolidVertex, pos);
	attrs[1].name = "in_normal";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[1].offset = CF_OFFSET_OF(CF_SolidVertex, n);
	CF_Mesh m = cf_make_mesh(verts.count() * (int)sizeof(CF_SolidVertex), attrs, 2, (int)sizeof(CF_SolidVertex));
	cf_mesh_update_vertex_data(m, (void*)verts.data(), verts.count());
	return m;
}

// Column-vector basis + translation as a CF_M4x4 (column-major storage).
static CF_M4x4 s_basis_m4(CF_V3 x, CF_V3 y, CF_V3 z, CF_V3 t)
{
	CF_M4x4 m = cf_m4_identity();
	m.elements[0] = x.x; m.elements[1] = x.y; m.elements[2]  = x.z;
	m.elements[4] = y.x; m.elements[5] = y.y; m.elements[6]  = y.z;
	m.elements[8] = z.x; m.elements[9] = z.y; m.elements[10] = z.z;
	m.elements[12] = t.x; m.elements[13] = t.y; m.elements[14] = t.z;
	return m;
}

// Any orthonormal frame perpendicular to n, right-handed as the columns (bx, n, bz) -- a
// negative-determinant frame would mirror the mesh and flip its winding under back-face culling.
static void s_perp_basis(CF_V3 n, CF_V3* bx, CF_V3* bz)
{
	CF_V3 h = CF_FABSF(n.y) < 0.99f ? cf_v3(0, 1, 0) : cf_v3(1, 0, 0);
	*bx = cf_norm_v3(cf_cross_v3(h, n));
	*bz = cf_cross_v3(*bx, n);
}

// Submits one solid instance: local placement composed under the transform stack, colored by
// the color stack, under the built-in hemisphere-lit shader and the user's render state.
static void s_submit_solid(CF_Mesh mesh, const CF_M4x4& local)
{
	s_shapes_init();
	CF_M4x4 m = cf_mul_m4(s_draw3d->transforms.last(), local);
	CF_MeshInstance3d inst;
	inst.model0 = s_row(m, 0);
	inst.model1 = s_row(m, 1);
	inst.model2 = s_row(m, 2);
	inst.uv_rect = cf_v4(0, 0, 1, 1);
	// Normal matrix: the rotation with each column re-normalized, correct under per-axis scale
	// (which is all the shape placements below ever produce).
	CF_V3 c0 = cf_safe_norm_v3(cf_v3(m.elements[0], m.elements[1], m.elements[2]));
	CF_V3 c1 = cf_safe_norm_v3(cf_v3(m.elements[4], m.elements[5], m.elements[6]));
	CF_V3 c2 = cf_safe_norm_v3(cf_v3(m.elements[8], m.elements[9], m.elements[10]));
	inst.nmat0 = cf_v4(c0.x, c1.x, c2.x, 0);
	inst.nmat1 = cf_v4(c0.y, c1.y, c2.y, 0);
	inst.nmat2 = cf_v4(c0.z, c1.z, c2.z, 0);
	inst.mesh_attributes = s_color4();
	cf_draw3d_push_shader(s_draw3d->solid_shd);
	s_submit(mesh, inst, false, NULL);
	cf_draw3d_pop_shader();
}

void cf_draw3d_cube(CF_V3 center, CF_V3 half_extents)
{
	s_shapes_init();
	if (!s_draw3d->cube_mesh.id) {
		Cute::Array<CF_SolidVertex> v;
		// Unit cube, half-extent 1, CCW from outside.
		for (int f = 0; f < 6; ++f) {
			int axis = f >> 1;
			float sign = (f & 1) ? -1.0f : 1.0f;
			CF_V3 n = cf_v3(axis == 0 ? sign : 0, axis == 1 ? sign : 0, axis == 2 ? sign : 0);
			CF_V3 u = cf_v3(n.y, n.z, n.x); // Perpendicular by rotation of components.
			CF_V3 w = cf_cross_v3(n, u);
			CF_V3 q[4] = {
				cf_add_v3(n, cf_add_v3(cf_mul_v3_f(u, -1), cf_mul_v3_f(w, -1))),
				cf_add_v3(n, cf_add_v3(cf_mul_v3_f(u,  1), cf_mul_v3_f(w, -1))),
				cf_add_v3(n, cf_add_v3(cf_mul_v3_f(u,  1), cf_mul_v3_f(w,  1))),
				cf_add_v3(n, cf_add_v3(cf_mul_v3_f(u, -1), cf_mul_v3_f(w,  1))),
			};
			int idx[6] = { 0, 1, 2, 0, 2, 3 };
			for (int i = 0; i < 6; ++i) v.add({ q[idx[i]], n });
		}
		s_draw3d->cube_mesh = s_make_solid(v);
	}
	CF_M4x4 local = s_basis_m4(cf_v3(half_extents.x, 0, 0), cf_v3(0, half_extents.y, 0), cf_v3(0, 0, half_extents.z), center);
	s_submit_solid(s_draw3d->cube_mesh, local);
}

void cf_draw3d_sphere(CF_V3 center, float radius)
{
	s_shapes_init();
	if (!s_draw3d->sphere_mesh.id) {
		Cute::Array<CF_SolidVertex> v;
		const int STACKS = 16, SLICES = 24;
		for (int i = 0; i < STACKS; ++i) {
			float p0 = CF_PI * ((float)i / STACKS - 0.5f);
			float p1 = CF_PI * ((float)(i + 1) / STACKS - 0.5f);
			for (int j = 0; j < SLICES; ++j) {
				float t0 = 2.0f * CF_PI * (float)j / SLICES;
				float t1 = 2.0f * CF_PI * (float)(j + 1) / SLICES;
				CF_V3 a = cf_v3(CF_COSF(p0) * CF_COSF(t0), CF_SINF(p0), CF_COSF(p0) * CF_SINF(t0));
				CF_V3 b = cf_v3(CF_COSF(p0) * CF_COSF(t1), CF_SINF(p0), CF_COSF(p0) * CF_SINF(t1));
				CF_V3 c = cf_v3(CF_COSF(p1) * CF_COSF(t1), CF_SINF(p1), CF_COSF(p1) * CF_SINF(t1));
				CF_V3 d = cf_v3(CF_COSF(p1) * CF_COSF(t0), CF_SINF(p1), CF_COSF(p1) * CF_SINF(t0));
				v.add({ a, a }); v.add({ c, c }); v.add({ b, b });
				v.add({ a, a }); v.add({ d, d }); v.add({ c, c });
			}
		}
		s_draw3d->sphere_mesh = s_make_solid(v);
	}
	CF_M4x4 local = s_basis_m4(cf_v3(radius, 0, 0), cf_v3(0, radius, 0), cf_v3(0, 0, radius), center);
	s_submit_solid(s_draw3d->sphere_mesh, local);
}

void cf_draw3d_cone(CF_V3 base, CF_V3 tip, float radius)
{
	s_shapes_init();
	if (!s_draw3d->cone_mesh.id) {
		Cute::Array<CF_SolidVertex> v;
		// Unit cone: base circle radius 1 at y = 0, tip at y = 1, smooth slant normals.
		const int SLICES = 32;
		for (int j = 0; j < SLICES; ++j) {
			float t0 = 2.0f * CF_PI * (float)j / SLICES;
			float t1 = 2.0f * CF_PI * (float)(j + 1) / SLICES;
			float tm = (t0 + t1) * 0.5f;
			CF_V3 b0 = cf_v3(CF_COSF(t0), 0, CF_SINF(t0));
			CF_V3 b1 = cf_v3(CF_COSF(t1), 0, CF_SINF(t1));
			CF_V3 n0 = cf_norm_v3(cf_v3(CF_COSF(t0), 1, CF_SINF(t0)));
			CF_V3 n1 = cf_norm_v3(cf_v3(CF_COSF(t1), 1, CF_SINF(t1)));
			CF_V3 nm = cf_norm_v3(cf_v3(CF_COSF(tm), 1, CF_SINF(tm)));
			v.add({ b0, n0 }); v.add({ cf_v3(0, 1, 0), nm }); v.add({ b1, n1 });
			// Base cap.
			CF_V3 dn = cf_v3(0, -1, 0);
			v.add({ b0, dn }); v.add({ b1, dn }); v.add({ cf_v3(0, 0, 0), dn });
		}
		s_draw3d->cone_mesh = s_make_solid(v);
	}
	CF_V3 axis = cf_sub_v3(tip, base);
	if (cf_len_v3(axis) < 0.00001f) return;
	CF_V3 bx, bz;
	s_perp_basis(cf_norm_v3(axis), &bx, &bz);
	CF_M4x4 local = s_basis_m4(cf_mul_v3_f(bx, radius), axis, cf_mul_v3_f(bz, radius), base);
	s_submit_solid(s_draw3d->cone_mesh, local);
}

void cf_draw3d_torus(CF_V3 center, CF_V3 normal, float radius, float tube_radius)
{
	s_shapes_init();
	// Tube/major ratio bakes into the mesh; cache one mesh per quantized ratio.
	float ratio = cf_clamp(tube_radius / cf_max(radius, 0.00001f), 0.01f, 0.99f);
	uint64_t key = (uint64_t)(ratio * 256.0f);
	CF_Mesh* found = s_draw3d->torus_meshes.try_find(key);
	CF_Mesh mesh = found ? *found : CF_Mesh { 0 };
	if (!mesh.id) {
		float r = (float)key / 256.0f;
		Cute::Array<CF_SolidVertex> v;
		const int MAJ = 32, MIN = 16;
		for (int i = 0; i < MAJ; ++i) {
			float a0 = 2.0f * CF_PI * (float)i / MAJ;
			float a1 = 2.0f * CF_PI * (float)(i + 1) / MAJ;
			for (int j = 0; j < MIN; ++j) {
				float b0 = 2.0f * CF_PI * (float)j / MIN;
				float b1 = 2.0f * CF_PI * (float)(j + 1) / MIN;
				// Major angle around y, minor angle around the ring's local circle.
				auto P = [&](float a, float b, CF_V3* n) {
					CF_V3 ring = cf_v3(CF_COSF(a), 0, CF_SINF(a));
					*n = cf_add_v3(cf_mul_v3_f(ring, CF_COSF(b)), cf_v3(0, CF_SINF(b), 0));
					return cf_add_v3(ring, cf_mul_v3_f(*n, r));
				};
				CF_V3 na, nb, nc, nd;
				CF_V3 pa = P(a0, b0, &na), pb = P(a1, b0, &nb), pc = P(a1, b1, &nc), pd = P(a0, b1, &nd);
				v.add({ pa, na }); v.add({ pc, nc }); v.add({ pb, nb });
				v.add({ pa, na }); v.add({ pd, nd }); v.add({ pc, nc });
			}
		}
		mesh = s_make_solid(v);
		s_draw3d->torus_meshes.insert(key, mesh);
	}
	CF_V3 n = cf_safe_norm_v3(normal);
	if (cf_len_v3(n) == 0) n = cf_v3(0, 1, 0);
	CF_V3 bx, bz;
	s_perp_basis(n, &bx, &bz);
	CF_M4x4 local = s_basis_m4(cf_mul_v3_f(bx, radius), cf_mul_v3_f(n, radius), cf_mul_v3_f(bz, radius), center);
	s_submit_solid(mesh, local);
}

//--------------------------------------------------------------------------------------------------
// Projection helpers. Both route through the 2d camera's mvp rather than raw pixels, so they
// compose exactly with cf_draw_text / cf_screen_to_world no matter where the 2d camera sits:
// 3d ndc IS clip-space xy, which is precisely what the 2d mvp maps its own world into.

CF_V3 cf_draw3d_project(CF_V3 world_position)
{
	CF_M4x4 vp = cf_mul_m4(s_draw3d->projections.last(), s_draw3d->views.last());
	CF_V4 clip = cf_mul_m4_v4(vp, cf_v4(world_position.x, world_position.y, world_position.z, 1.0f));
	if (clip.w <= 0) {
		return cf_v3(0, 0, -1.0f); // Behind the camera; a negative z marks xy unusable.
	}
	CF_V2 ndc = cf_v2(clip.x / clip.w, clip.y / clip.w);
	CF_V2 out = cf_mul_m32_v2(cf_invert(s_draw->mvp), ndc);
	return cf_v3(out.x, out.y, clip.z / clip.w);
}

void cf_draw3d_unproject(CF_V2 point, CF_V3* origin_out, CF_V3* direction_out)
{
	CF_V2 ndc = cf_mul_m32_v2(s_draw->mvp, point);
	CF_M4x4 vp = cf_mul_m4(s_draw3d->projections.last(), s_draw3d->views.last());
	CF_M4x4 inv = cf_m4_invert(vp);
	CF_V4 p0 = cf_mul_m4_v4(inv, cf_v4(ndc.x, ndc.y, 0.0f, 1.0f)); // Near plane (clip z = 0).
	CF_V4 p1 = cf_mul_m4_v4(inv, cf_v4(ndc.x, ndc.y, 1.0f, 1.0f)); // Far plane (clip z = 1).
	CF_V3 a = cf_v3(p0.x / p0.w, p0.y / p0.w, p0.z / p0.w);
	CF_V3 b = cf_v3(p1.x / p1.w, p1.y / p1.w, p1.z / p1.w);
	if (origin_out) *origin_out = a;
	if (direction_out) *direction_out = cf_norm_v3(cf_sub_v3(b, a));
}

//--------------------------------------------------------------------------------------------------
// Flush-time rendering, called from s_process_command in layer-sorted order (accumulated 2d
// geometry has already been flushed, so paint order holds).

// Appends the reserved instance-rate attributes to a user mesh the first time it flows through
// this layer. Offsets follow CF_MeshInstance3d; the buffer auto-grows on update.
static void s_augment_mesh(CF_Mesh mesh)
{
	const char* names[8] = {
		"in_model0", "in_model1", "in_model2",
		"in_uv_rect",
		"in_nmat0", "in_nmat1", "in_nmat2",
		"in_mesh_attributes",
	};
	CF_VertexAttribute attrs[8] = { };
	for (int i = 0; i < 8; ++i) {
		attrs[i].name = names[i];
		attrs[i].format = CF_VERTEX_FORMAT_FLOAT4;
		attrs[i].offset = i * (int)sizeof(CF_V4);
		attrs[i].per_instance = true;
	}
	cf_mesh_append_attributes(mesh, attrs, 8);
	// Appending past CF_MESH_MAX_VERTEX_ATTRIBUTES silently drops the extras (matching
	// cf_make_mesh); a mesh that dense would render garbage with no error, so say so here.
	CF_ASSERT(cf_mesh_has_vertex_attribute(mesh, "in_mesh_attributes"));
	cf_mesh_set_instance_buffer(mesh, (int)sizeof(CF_MeshInstance3d) * 16, (int)sizeof(CF_MeshInstance3d));
}

void cf_draw3d_process(CF_Command* cmd, CF_Canvas canvas, bool clear)
{
	CF_MeshCmd3d* mc = cmd->mesh3d;
	cf_apply_canvas(canvas, clear);

	const Cute::Array<CF_MeshInstance3d>& instances = mc->instances_ref ? *mc->instances_ref : mc->instances;
	bool textured = mc->sprite_textured && !mc->escape && instances.count();
	if (textured) {
		// Resolve every instance's image through the atlas. Pushing entries per flush is both
		// the uv lookup and the usage signal: images drawn together pack into shared pages,
		// so page splits (extra draws below) converge away as the atlas learns the scene.
		const Cute::Array<CF_ImageRef3d>& image_refs = mc->image_refs_ref ? *mc->image_refs_ref : mc->image_refs;
		CF_ASSERT(image_refs.count() == instances.count());
		s_draw3d->resolve_uvs.clear();
		s_draw3d->resolve_uvs.set_count(instances.count());
		for (int i = 0; i < instances.count(); ++i) {
			atlas_cache_entry_t e = image_refs[i].entry;
			e.udata = (ATLAS_CACHE_U64)i;
			atlas_cache_push(&s_draw->atlas_cache, e);
		}
		s_draw3d->resolving = true;
		if (!s_draw->delay_defrag) {
			atlas_cache_defrag(&s_draw->atlas_cache);
		}
		atlas_cache_flush(&s_draw->atlas_cache);
		s_draw3d->resolving = false;

		// Stage uv-filled instance copies. The rect packs as (minx, maxy, maxx, miny) so mesh
		// uv (0, 0) samples the image's top-left, matching 2d sprites; atlased images inset
		// one texel to skip the 1px border ring.
		s_draw3d->staged.clear();
		for (int i = 0; i < instances.count(); ++i) {
			const CF_PendingUV& uv = s_draw3d->resolve_uvs[i];
			CF_MeshInstance3d inst = instances[i];
			float du = image_refs[i].bordered && uv.tex_w ? 1.0f / (float)uv.tex_w : 0;
			float dv = image_refs[i].bordered && uv.tex_h ? 1.0f / (float)uv.tex_h : 0;
			inst.uv_rect = cf_v4(uv.minx + du, uv.maxy - dv, uv.maxx - du, uv.miny + dv);
			s_draw3d->staged.add(inst);
		}
	}
	if (!mc->escape && instances.count()) {
		if (!cf_mesh_has_vertex_attribute(mc->mesh, "in_model0")) {
			s_augment_mesh(mc->mesh);
		}
		if (!textured && !mc->gpu_instances) {
			cf_mesh_update_instance_data(mc->mesh, (void*)instances.data(), instances.count());
		}
	}
	cf_apply_mesh(mc->mesh);

	CF_Material material = s_draw3d->material;
	cf_material_set_render_state(material, cmd->render_state);
	// u_image is always bound: the atlas page for sprite-textured draws (below), and the 1x1
	// white texture otherwise -- shaders sampling u_image degrade gracefully with no sprite
	// pushed, matching the 2d layer's shape-only draws.
	cf_material_set_texture_vs(material, "u_image", s_draw->white_texture);
	cf_material_set_texture_fs(material, "u_image", s_draw->white_texture);
	for (int i = 0; i < mc->textures.count(); ++i) {
		// Both stages: unmatched names are ignored at bind, and vertex texture fetch matters
		// for 3d (vertex animation textures and friends).
		cf_material_set_texture_vs(material, mc->textures[i].name, mc->textures[i].texture);
		cf_material_set_texture_fs(material, mc->textures[i].name, mc->textures[i].texture);
	}
	for (int i = 0; i < mc->uniforms.count(); ++i) {
		CF_Uniform3d& u = mc->uniforms[i];
		cf_material_set_uniform_vs(material, u.name, u.data, u.type, u.array_length);
		cf_material_set_uniform_fs(material, u.name, u.data, u.type, u.array_length);
	}
	cf_material_set_uniform_vs(material, "u_view_projection", &mc->vp, CF_UNIFORM_TYPE_MAT4, 1);
	// Canvas dimensions for the built-in stroke shader's pixel-density math; unmatched
	// uniform names are ignored at bind time, so this is free for every other shader.
	int canvas_w, canvas_h;
	cf_current_canvas_size(&canvas_w, &canvas_h);
	CF_V4 shape_res = cf_v4((float)canvas_w, (float)canvas_h, 0, 0);
	cf_material_set_uniform_vs(material, "u_shape_res", &shape_res, CF_UNIFORM_TYPE_FLOAT4, 1);

	// 3d textures sample with their own sampler settings, not the 2d filter-mode override.
	cf_set_sampler_override(NULL);

	CF_Rect viewport = cmd->viewport;
	CF_Rect scissor = cmd->scissor;

	if (!textured) {
		// Baked groups draw straight from their persistent buffer -- no upload happened.
		if (mc->gpu_instances) cf_apply_instance_buffer_override(mc->gpu_instances, instances.count());
		cf_apply_shader(cmd->shader, material);
		if (viewport.w >= 0 && viewport.h >= 0) cf_apply_viewport(viewport.x, viewport.y, viewport.w, viewport.h);
		if (scissor.w >= 0 && scissor.h >= 0) cf_apply_scissor(scissor.x, scissor.y, scissor.w, scissor.h);
		cf_draw_elements();
		return;
	}

	// One instanced draw per atlas page the images resolved to, first-appearance order.
	// Consuming entries by zeroing their texture_id keeps this allocation-free.
	for (;;) {
		uint64_t page = 0;
		for (int i = 0; i < s_draw3d->resolve_uvs.count(); ++i) {
			if (s_draw3d->resolve_uvs[i].texture_id) {
				page = s_draw3d->resolve_uvs[i].texture_id;
				break;
			}
		}
		if (!page) break;
		s_draw3d->page.clear();
		for (int i = 0; i < s_draw3d->resolve_uvs.count(); ++i) {
			if (s_draw3d->resolve_uvs[i].texture_id == page) {
				s_draw3d->page.add(s_draw3d->staged[i]);
				s_draw3d->resolve_uvs[i].texture_id = 0;
			}
		}
		CF_Texture atlas;
		atlas.id = page;
		cf_material_set_texture_vs(material, "u_image", atlas);
		cf_material_set_texture_fs(material, "u_image", atlas);
		cf_mesh_update_instance_data(mc->mesh, s_draw3d->page.data(), s_draw3d->page.count());
		cf_apply_shader(cmd->shader, material);
		if (viewport.w >= 0 && viewport.h >= 0) cf_apply_viewport(viewport.x, viewport.y, viewport.w, viewport.h);
		if (scissor.w >= 0 && scissor.h >= 0) cf_apply_scissor(scissor.x, scissor.y, scissor.w, scissor.h);
		cf_draw_elements();
	}
}

//--------------------------------------------------------------------------------------------------
// Draw lists. Recording runs through the ordinary submission path (commands accumulate past
// recording_mark and move into the list at cf_draw_list_end); the hooks below add the 3d
// semantics: list-local transforms while recording, the bake (grouping + exact normal
// matrices), and replay payloads that borrow the baked list's instances under a live camera.

void cf_draw3d_list_begin()
{
	// Record in list-local space: replay composes the then-current transform stack on top.
	s_draw3d->transforms.add(cf_m4_identity());
}

// True when two mesh commands render identically except for their per-instance data -- the
// bake grouping predicate. Uniform/texture captures compare by content (not version) so
// A-B-A submission orders still fold both A's together.
static bool s_bake_group_match(const CF_Command* a, const CF_Command* b)
{
	const CF_MeshCmd3d* ma = a->mesh3d;
	const CF_MeshCmd3d* mb = b->mesh3d;
	if (ma->mesh.id != mb->mesh.id) return false;
	if (ma->escape || mb->escape) return false;
	if (ma->sprite_textured != mb->sprite_textured) return false;
	if (a->shader.id != b->shader.id) return false;
	if (!(a->render_state == b->render_state)) return false;
	if (a->layer != b->layer) return false;
	if (!(a->scissor == b->scissor)) return false;
	if (!(a->viewport == b->viewport)) return false;
	if (ma->uniforms.count() != mb->uniforms.count()) return false;
	for (int i = 0; i < ma->uniforms.count(); ++i) {
		const CF_Uniform3d& ua = ma->uniforms[i];
		const CF_Uniform3d& ub = mb->uniforms[i];
		if (ua.name != ub.name || ua.type != ub.type || ua.array_length != ub.array_length || ua.size != ub.size) return false;
		if (CF_MEMCMP(ua.data, ub.data, ua.size)) return false;
	}
	if (ma->textures.count() != mb->textures.count()) return false;
	for (int i = 0; i < ma->textures.count(); ++i) {
		if (ma->textures[i].name != mb->textures[i].name) return false;
		if (ma->textures[i].texture.id != mb->textures[i].texture.id) return false;
	}
	return true;
}

// Resolves a payload's borrowed data to owned copies -- a nested replay recorded into this
// list must never reference another list's storage (mirrors the geoms_ref resolution in
// cf_draw_list_end).
static void s_own_payload(CF_MeshCmd3d* mc)
{
	if (mc->instances_ref) {
		mc->instances = *mc->instances_ref;
		mc->instances_ref = NULL;
	}
	mc->gpu_instances = 0; // Borrowed from another list, if set; this list bakes its own.
	mc->owns_gpu_instances = false;
	if (mc->image_refs_ref) {
		mc->image_refs = *mc->image_refs_ref;
		mc->image_refs_ref = NULL;
	}
	if (!mc->uniform_block && mc->uniforms.count()) {
		int total = 0;
		for (int i = 0; i < mc->uniforms.count(); ++i) total += mc->uniforms[i].size;
		mc->uniform_block = CF_ALLOC(total);
		int offset = 0;
		for (int i = 0; i < mc->uniforms.count(); ++i) {
			CF_Uniform3d& u = mc->uniforms[i];
			void* dst = (char*)mc->uniform_block + offset;
			CF_MEMCPY(dst, u.data, u.size);
			u.data = dst;
			offset += u.size;
		}
	}
}

// Rebuilds the affine 4x4 from an instance's three baked model rows.
static CF_M4x4 s_model_from_rows(const CF_MeshInstance3d* inst)
{
	CF_M4x4 m = cf_m4_identity();
	const CF_V4* rows[3] = { &inst->model0, &inst->model1, &inst->model2 };
	for (int r = 0; r < 3; ++r) {
		m.elements[0 * 4 + r] = rows[r]->x;
		m.elements[1 * 4 + r] = rows[r]->y;
		m.elements[2 * 4 + r] = rows[r]->z;
		m.elements[3 * 4 + r] = rows[r]->w;
	}
	return m;
}

void cf_draw3d_list_end(CF_DrawListData* data)
{
	// Restore the pre-recording transform stack.
	cf_draw3d_pop();

	// Own any borrowed payload data, then group: every later command whose full state matches
	// an earlier one folds its instances into that command, so replay issues one instanced
	// draw per unique state regardless of submission order. Depth testing owns the ordering
	// this discards -- the documented bake semantic.
	int count = data->cmds.count();
	for (int i = 0; i < count; ++i) {
		if (!data->cmds[i].mesh3d) continue;
		s_own_payload(data->cmds[i].mesh3d);
	}
	for (int i = 0; i < count; ++i) {
		CF_MeshCmd3d* group = data->cmds[i].mesh3d;
		if (!group || group->escape) continue;
		for (int j = i + 1; j < count; ++j) {
			CF_MeshCmd3d* mc = data->cmds[j].mesh3d;
			if (!mc || mc->escape) continue;
			if (!s_bake_group_match(&data->cmds[i], &data->cmds[j])) continue;
			for (int k = 0; k < mc->instances.count(); ++k) {
				group->instances.add(mc->instances[k]);
			}
			for (int k = 0; k < mc->image_refs.count(); ++k) {
				group->image_refs.add(mc->image_refs[k]);
			}
			cf_draw3d_free_cmd(&data->cmds[j]);
		}
	}
	// Compact out the merged-away commands (their payloads are freed; order is preserved).
	int w = 0;
	for (int r = 0; r < count; ++r) {
		CF_Command& c = data->cmds[r];
		if (!c.mesh3d && c.geoms.count() == 0 && !c.geoms_ref && c.items.count() == 0 && !c.u.data && !c.u.is_texture) continue;
		if (w != r) data->cmds[w] = data->cmds[r];
		++w;
	}
	data->cmds.set_count(w);

	// Exact per-instance normal matrices, computed once at bake (the immediate path derives
	// them from the model rows instead -- see the header).
	for (int i = 0; i < data->cmds.count(); ++i) {
		CF_MeshCmd3d* mc = data->cmds[i].mesh3d;
		if (!mc) continue;
		for (int k = 0; k < mc->instances.count(); ++k) {
			CF_MeshInstance3d& inst = mc->instances[k];
			CF_M4x4 n = cf_m4_normal_matrix(s_model_from_rows(&inst));
			inst.nmat0 = cf_v4(n.elements[0], n.elements[4], n.elements[8], 0);
			inst.nmat1 = cf_v4(n.elements[1], n.elements[5], n.elements[9], 0);
			inst.nmat2 = cf_v4(n.elements[2], n.elements[6], n.elements[10], 0);
		}
	}

	// The instance data is final now (sprite-textured groups excepted -- their uv lanes
	// refresh against the atlas every flush): upload each group once to a persistent GPU
	// buffer, so replays cost zero instance bandwidth.
	for (int i = 0; i < data->cmds.count(); ++i) {
		CF_MeshCmd3d* mc = data->cmds[i].mesh3d;
		if (!mc || mc->escape || mc->sprite_textured || !mc->instances.count()) continue;
		int bytes = mc->instances.count() * (int)sizeof(CF_MeshInstance3d);
		mc->gpu_instances = cf_make_instance_buffer(bytes, (int)sizeof(CF_MeshInstance3d));
		cf_update_instance_buffer(mc->gpu_instances, mc->instances.data(), mc->instances.count());
		mc->owns_gpu_instances = true;
	}
}

void cf_draw3d_replay_cmd(CF_Command* dst, const CF_Command* src)
{
	const CF_MeshCmd3d* smc = src->mesh3d;
	CF_MeshCmd3d* mc = CF_NEW(CF_MeshCmd3d);
	mc->mesh = smc->mesh;
	mc->escape = smc->escape;
	// Cameras are live at replay, and the current 3d transform stack moves the whole list:
	// final position = P * V * T_now * M_baked. Note a rotation in T_now does not reach the
	// baked in_nmat lanes -- lighting under whole-list rotation is the shader's business.
	CF_M4x4 vp = cf_mul_m4(s_draw3d->projections.last(), s_draw3d->views.last());
	mc->vp = cf_mul_m4(vp, s_draw3d->transforms.last());
	mc->instances_ref = &smc->instances;
	mc->gpu_instances = smc->gpu_instances; // Borrowed; the list's payload owns it.
	mc->sprite_textured = smc->sprite_textured;
	mc->image_refs_ref = smc->sprite_textured ? &smc->image_refs : NULL;
	mc->uniforms = smc->uniforms; // Bytes stay in the list's uniform_block (borrowed).
	mc->textures = smc->textures;
	dst->mesh3d = mc;
}

void cf_draw3d_free_list_cmds(CF_DrawListData* data)
{
	for (int i = 0; i < data->cmds.count(); ++i) {
		cf_draw3d_free_cmd(&data->cmds[i]);
	}
}
