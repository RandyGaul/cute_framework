/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// A skinned, animated glTF model through cute_model.h + cute_draw3d.h.
//
// cute_model.h (libraries/cute/) loads GLB/glTF into plain arrays: vertex streams,
// a node hierarchy, skins with inverse-bind matrices, and keyframe clips. The runtime
// contract is the one the skinning sample established: bone matrices arrive fully
// composed (posed * inverse-bind) in a mat4 array uniform, and the vertex shader only
// does the weighted blend. Everything downstream -- instancing, transforms, texturing,
// the shape API's strokes for the skeleton overlay -- is ordinary draw3d.
//
// Every frame: rest pose -> cm_animate (sample the clip) -> cm_world_transforms ->
// cm_skin_palette -> cf_draw3d_set_uniform("u_bones", ...). The fox cycles its three
// clips (Survey, Walk, Run); thin strokes trace the posed skeleton.
//
// Drag to orbit, mouse wheel to zoom, space to toggle the skeleton overlay.
//
// Fox model: CC0 by PixelMannen; rigging and animations CC-BY 4.0 by @tomkranis,
// via the Khronos glTF-Sample-Assets repository.

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>

#include <cute/ckit.h>
#define CUTE_MODEL_IMPLEMENTATION
#include <cute/cute_model.h>

typedef struct Vertex
{
	CF_V3 pos;
	CF_V3 n;
	CF_V2 uv;
	float joints[4];  // Bone indices as floats -- vertex attributes have no integer lanes.
	float weights[4];
} Vertex;

// The skinning blend happens upstream of the ordinary draw3d contract; u_bones holds
// this frame's posed * inverse-bind palette. The bone count is stamped in at runtime.
static const char* s_vs_fmt =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec3 in_normal;\n"
"layout (location = 2) in vec2 in_uv;\n"
"layout (location = 3) in vec4 in_joints;\n"
"layout (location = 4) in vec4 in_weights;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec3 v_normal;\n"
"layout (location = 1) out vec2 v_uv;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"    mat4 u_bones[%d];\n"
"};\n"
"void main() {\n"
"    vec4 p0 = vec4(in_pos, 1.0);\n"
"    vec4 n0 = vec4(in_normal, 0.0);\n"
"    vec3 pos = (u_bones[int(in_joints.x)] * p0 * in_weights.x + u_bones[int(in_joints.y)] * p0 * in_weights.y\n"
"              + u_bones[int(in_joints.z)] * p0 * in_weights.z + u_bones[int(in_joints.w)] * p0 * in_weights.w).xyz;\n"
"    vec3 nrm = normalize((u_bones[int(in_joints.x)] * n0 * in_weights.x + u_bones[int(in_joints.y)] * n0 * in_weights.y\n"
"                        + u_bones[int(in_joints.z)] * n0 * in_weights.z + u_bones[int(in_joints.w)] * n0 * in_weights.w).xyz);\n"
"    vec4 p = vec4(pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_normal = normalize(vec3(dot(in_model0.xyz, nrm), dot(in_model1.xyz, nrm), dot(in_model2.xyz, nrm)));\n"
"    v_uv = in_uv;\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

static const char* s_fs =
"layout (location = 0) in vec3 v_normal;\n"
"layout (location = 1) in vec2 v_uv;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_image;\n"
"void main() {\n"
"    vec3 n = normalize(v_normal);\n"
"    vec3 key_dir = normalize(vec3(0.5, 0.8, 0.4));\n"
"    float key = max(dot(n, key_dir), 0.0);\n"
"    float hemi = 0.5 + 0.5 * n.y;\n"
"    vec3 albedo = texture(u_image, v_uv).rgb;\n"
"    result = vec4(albedo * (0.28 + 0.22 * hemi + 0.6 * key), 1.0);\n"
"}\n";

// Interleaves the loader's separate streams into one draw3d mesh. De-indexes flat for
// simplicity, exactly like the OBJ sample.
static CF_Mesh s_make_mesh(const CM_Primitive* prim, int* out_count)
{
	int n = prim->index_count;
	Vertex* verts = (Vertex*)cf_alloc(sizeof(Vertex) * (size_t)n);
	for (int i = 0; i < n; ++i) {
		uint32_t v = prim->indices[i];
		Vertex* out = verts + i;
		out->pos = cf_v3(prim->positions[v * 3], prim->positions[v * 3 + 1], prim->positions[v * 3 + 2]);
		out->n = cf_v3(prim->normals[v * 3], prim->normals[v * 3 + 1], prim->normals[v * 3 + 2]);
		out->uv = prim->uvs ? cf_v2(prim->uvs[v * 2], prim->uvs[v * 2 + 1]) : cf_v2(0, 0);
		for (int k = 0; k < 4; ++k) {
			out->joints[k] = prim->joints ? (float)prim->joints[v * 4 + k] : 0.0f;
			out->weights[k] = prim->weights ? prim->weights[v * 4 + k] : (k == 0 ? 1.0f : 0.0f);
		}
	}
	CF_VertexAttribute attrs[5] = { 0 };
	attrs[0].name = "in_pos";     attrs[0].format = CF_VERTEX_FORMAT_FLOAT3; attrs[0].offset = CF_OFFSET_OF(Vertex, pos);
	attrs[1].name = "in_normal";  attrs[1].format = CF_VERTEX_FORMAT_FLOAT3; attrs[1].offset = CF_OFFSET_OF(Vertex, n);
	attrs[2].name = "in_uv";      attrs[2].format = CF_VERTEX_FORMAT_FLOAT2; attrs[2].offset = CF_OFFSET_OF(Vertex, uv);
	attrs[3].name = "in_joints";  attrs[3].format = CF_VERTEX_FORMAT_FLOAT4; attrs[3].offset = CF_OFFSET_OF(Vertex, joints);
	attrs[4].name = "in_weights"; attrs[4].format = CF_VERTEX_FORMAT_FLOAT4; attrs[4].offset = CF_OFFSET_OF(Vertex, weights);
	CF_Mesh mesh = cf_make_mesh(n * (int)sizeof(Vertex), attrs, 5, (int)sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, n);
	cf_free(verts);
	*out_count = n;
	return mesh;
}

// The fox's base color texture ships inside the GLB as a PNG.
static CF_Texture s_make_texture(const CM_Model* model)
{
	if (model->material_count && model->materials[0].base_color_texture.image >= 0) {
		CM_Image* image = model->images + model->materials[0].base_color_texture.image;
		CF_Image img;
		if (!cf_is_error(cf_image_load_png_from_memory(image->data, image->size, &img))) {
			CF_TextureParams params = cf_texture_defaults(img.w, img.h);
			params.filter = CF_FILTER_LINEAR;
			CF_Texture tex = cf_make_texture(params);
			cf_texture_update(tex, img.pix, img.w * img.h * (int)sizeof(CF_Pixel));
			cf_image_free(&img);
			return tex;
		}
	}
	// Fallback: 1x1 white.
	CF_Pixel white = cf_make_pixel_rgb(255, 255, 255);
	CF_Texture tex = cf_make_texture(cf_texture_defaults(1, 1));
	cf_texture_update(tex, &white, (int)sizeof(white));
	return tex;
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("cute_model -- skinned glTF", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC);

	// Mount this sample's data folder next to the executable.
	char* base = cf_path_normalize(cf_fs_get_base_directory());
	char* data_dir = cf_string_append(base, "/model3d_data");
	cf_fs_mount(data_dir, "/model3d_data", false);
	cf_string_free(data_dir);

	cf_canvas_set_clear_color(cf_app_get_canvas(), cf_make_color_rgb_f(0.10f, 0.11f, 0.13f));

	size_t size = 0;
	void* data = cf_fs_read_entire_file_to_memory("/model3d_data/Fox.glb", &size);
	if (!data) {
		printf("Failed to read /model3d_data/Fox.glb\n");
		cf_destroy_app();
		return -1;
	}
	CM_Model* model = cm_load(data, (int)size);
	cf_free(data);
	if (!model) {
		printf("cm_load failed: %s\n", cm_error());
		cf_destroy_app();
		return -1;
	}

	// The fox is one mesh, one primitive, one skin.
	int vertex_count = 0;
	CF_Mesh mesh = s_make_mesh(&model->meshes[0].primitives[0], &vertex_count);
	CF_Texture texture = s_make_texture(model);
	cf_draw3d_set_texture("u_image", texture);

	const CM_Skin* skin = &model->skins[0];
	char vs[4096];
	snprintf(vs, sizeof(vs), s_vs_fmt, skin->joint_count);
	CF_Shader shader = cf_make_shader_from_source(vs, s_fs);

	CM_Transform* locals = (CM_Transform*)cf_alloc(sizeof(CM_Transform) * (size_t)model->node_count);
	float* world = (float*)cf_alloc(sizeof(float) * 16 * (size_t)model->node_count);
	float* palette = (float*)cf_alloc(sizeof(float) * 16 * (size_t)skin->joint_count);

	// Orbit camera state.
	float azimuth = 0.7f, elevation = 0.35f, distance = 6.5f;
	bool dragging = false;
	bool show_skeleton = true;
	float last_mx = 0, last_my = 0;
	float t = 0;

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		float mx = cf_mouse_x(), my = cf_mouse_y();
		if (cf_mouse_down(CF_MOUSE_BUTTON_LEFT)) {
			if (dragging) {
				azimuth -= (mx - last_mx) * 0.008f;
				elevation += (my - last_my) * 0.008f;
				elevation = cf_clamp(elevation, -1.4f, 1.4f);
			}
			dragging = true;
		} else {
			dragging = false;
		}
		last_mx = mx;
		last_my = my;
		distance = cf_clamp(distance - cf_mouse_wheel_motion() * 0.6f, 3.0f, 16.0f);
		if (cf_key_just_pressed(CF_KEY_SPACE)) show_skeleton = !show_skeleton;

		// Cycle clips every few seconds, looping each with fmod.
		int clip_index = ((int)(t / 4.0f)) % model->animation_count;
		const CM_Animation* clip = model->animations + clip_index;
		cm_rest_pose(model, locals);
		cm_animate(model, clip, fmodf(t, clip->duration), locals);
		cm_world_transforms(model, locals, world);
		cm_skin_palette(model, 0, world, palette);

		int w, h;
		cf_app_get_size(&w, &h);
		float ce = CF_COSF(elevation);
		CF_V3 eye = cf_v3(CF_SINF(azimuth) * ce * distance, 1.2f + CF_SINF(elevation) * distance, CF_COSF(azimuth) * ce * distance);
		cf_draw3d_push_projection(cf_perspective(CF_PI / 3.6f, (float)w / (float)h, 0.1f, 100.0f));
		cf_draw3d_push_view(cf_look_at(eye, cf_v3(0, 1.0f, 0), cf_v3(0, 1, 0)));

		// The fox is authored in centimeters; scale to a couple of world units.
		cf_draw3d_push();
		cf_draw3d_scale(cf_v3(0.025f));

		cf_draw3d_push_shader(shader);
		cf_draw3d_set_uniform("u_bones", palette, CF_UNIFORM_TYPE_MAT4, skin->joint_count);
		cf_draw3d_mesh(mesh);
		cf_draw3d_pop_shader();

		// Posed skeleton overlay through the shape API: a stroke per bone, drawn under
		// the same transform stack as the mesh. Strokes compose their render state on
		// top of the pushed one, so an always-pass depth test makes them x-ray.
		if (show_skeleton) {
			CF_RenderState xray = cf_render_state_3d_defaults();
			xray.depth_compare = CF_COMPARE_FUNCTION_ALWAYS;
			cf_draw3d_push_render_state(xray);
			cf_draw3d_push_color(cf_make_color_rgba_f(1.0f, 1.0f, 1.0f, 0.6f));
			for (int i = 0; i < skin->joint_count; ++i) {
				int node = skin->joints[i];
				int parent = model->nodes[node].parent;
				if (parent < 0) continue;
				CF_V3 a = cf_v3(world[node * 16 + 12], world[node * 16 + 13], world[node * 16 + 14]);
				CF_V3 b = cf_v3(world[parent * 16 + 12], world[parent * 16 + 13], world[parent * 16 + 14]);
				// Stroke thickness is in world units and doesn't scale with the transform
				// stack, so size it for the final on-screen scale, not the fox's
				// centimeter-scale model space.
				cf_draw3d_line(a, b, 0.02f);
			}
			cf_draw3d_pop_color();
			cf_draw3d_pop_render_state();
		}
		cf_draw3d_pop();

		// Ground grid and a soft contact shadow.
		cf_draw3d_push_color(cf_make_color_rgb_f(0.22f, 0.24f, 0.28f));
		for (int i = -8; i <= 8; ++i) {
			cf_draw3d_line(cf_v3((float)i * 0.5f, 0, -4), cf_v3((float)i * 0.5f, 0, 4), 0.008f);
			cf_draw3d_line(cf_v3(-4, 0, (float)i * 0.5f), cf_v3(4, 0, (float)i * 0.5f), 0.008f);
		}
		cf_draw3d_pop_color();
		cf_draw3d_push_color(cf_make_color_rgba_f(0, 0, 0, 0.4f));
		cf_draw3d_circle_fill(cf_v3(0, 0.005f, 0), cf_v3(0, 1, 0), 1.5f);
		cf_draw3d_pop_color();

		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();

		// Clip name, through the ordinary 2d layer.
		cf_push_font_size(20);
		cf_draw_push_color(cf_color_white());
		cf_draw_text(clip->name, cf_v2(-(float)w * 0.5f + 20, (float)h * 0.5f - 20), -1);
		cf_draw_pop_color();
		cf_pop_font_size();

		cf_app_draw_onto_screen(true);
	}

	cf_free(locals);
	cf_free(world);
	cf_free(palette);
	cm_free(model);
	cf_destroy_mesh(mesh);
	cf_destroy_texture(texture);
	cf_destroy_shader(shader);
	cf_destroy_app();
	return 0;
}
