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

#include <internal/cute_alloc_internal.h>
#include <internal/cute_draw_internal.h>
#include <internal/cute_graphics_internal.h>

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

// The per-command payload for a 3d mesh draw, owned by its CF_Command (freed via
// cf_draw3d_free_cmd when the command is destroyed). The command's own shader/render_state/
// layer/scissor/viewport fields carry the rest of the captured state.
struct CF_MeshCmd3d
{
	CF_Mesh mesh = { 0 };
	bool escape = false; // Mesh has its own instance buffer: drawn as-is, no reserved attributes.
	CF_M4x4 vp;          // projection * view, captured at submission.
	uint64_t state_version = 0; // s_draw3d->version at capture; cheap uniform/texture equality.
	Cute::Array<CF_MeshInstance3d> instances;
	Cute::Array<CF_Uniform3d> uniforms; // data points into uniform_block.
	void* uniform_block = NULL;         // One allocation holding every captured uniform's bytes.
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
	// Draw-list recording of 3d submissions lands with the bake work.
	CF_ASSERT(!s_draw->recording_list);

	bool ours = cf_mesh_has_vertex_attribute(mesh, "in_model0");
	bool escape = !ours && cf_mesh_instance_stride(mesh) != 0;
	CF_M4x4 vp = cf_mul_m4(s_draw3d->projections.last(), s_draw3d->views.last());
	CF_RenderState rs = s_draw3d->render_states.last();

	if (!escape) {
		CF_Command* under = s_coalesce_candidate();
		if (under) {
			CF_MeshCmd3d* mc = under->mesh3d;
			if (mc->mesh.id == mesh.id && !mc->escape
				&& mc->state_version == s_draw3d->version
				&& !CF_MEMCMP(&mc->vp, &vp, sizeof(vp))
				&& under->shader.id == shader.id
				&& under->render_state == rs
				&& under->layer == s_draw->layers.last()
				&& under->scissor == s_draw->scissors.last()
				&& under->viewport == s_draw->viewports.last()) {
				mc->instances.add(s_instance());
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

	if (!escape) mc->instances.add(s_instance());

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

	if (!mc->escape && mc->instances.count()) {
		if (!cf_mesh_has_vertex_attribute(mc->mesh, "in_model0")) {
			s_augment_mesh(mc->mesh);
		}
		cf_mesh_update_instance_data(mc->mesh, mc->instances.data(), mc->instances.count());
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
	cf_apply_shader(cmd->shader, material);

	CF_Rect viewport = cmd->viewport;
	if (viewport.w >= 0 && viewport.h >= 0) {
		cf_apply_viewport(viewport.x, viewport.y, viewport.w, viewport.h);
	}
	CF_Rect scissor = cmd->scissor;
	if (scissor.w >= 0 && scissor.h >= 0) {
		cf_apply_scissor(scissor.x, scissor.y, scissor.w, scissor.h);
	}

	cf_draw_elements();
}
