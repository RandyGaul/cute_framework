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

	~CF_MeshCmd3d() { if (uniform_block) CF_FREE(uniform_block); }
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

void cf_draw3d_mesh(CF_Mesh mesh)
{
	CF_ASSERT(mesh.id);
	CF_Shader shader = s_draw3d->shaders.last();
	CF_ASSERT(shader.id); // No default 3d shader -- push one first (see the shader contract in cute_draw3d.h).
	if (!shader.id) return;

	bool ours = cf_mesh_has_vertex_attribute(mesh, "in_model0");
	bool escape = !ours && cf_mesh_instance_stride(mesh) != 0;
	CF_M4x4 vp = cf_mul_m4(s_draw3d->projections.last(), s_draw3d->views.last());
	CF_RenderState rs = s_draw3d->render_states.last();
	const CF_Sprite* sprite = s_draw3d->sprites.last();
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
				mc->instances.add(s_instance());
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
		mc->instances.add(s_instance());
		if (sprite_textured) mc->image_refs.add(s_image_ref(sprite));
	}

	// Keep subsequent 2d drawing off this command.
	s_draw->add_cmd();
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
		if (!textured) {
			cf_mesh_update_instance_data(mc->mesh, (void*)instances.data(), instances.count());
		}
	}
	cf_apply_mesh(mc->mesh);

	CF_Material material = s_draw3d->material;
	cf_material_set_render_state(material, cmd->render_state);
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

	// 3d textures sample with their own sampler settings, not the 2d filter-mode override.
	cf_set_sampler_override(NULL);

	CF_Rect viewport = cmd->viewport;
	CF_Rect scissor = cmd->scissor;

	if (!textured) {
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
