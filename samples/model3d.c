/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// A skinned, animated glTF model through cute_model.h + cute_draw3d.h.
//
// cute_model.h (libraries/cute/) loads GLB/glTF into plain arrays: vertex streams,
// a node hierarchy, skins with inverse-bind matrices, and keyframe clips. Skinning at
// scale runs the pull pattern: every character's bone palette lives in ONE storage
// buffer (cf_draw3d_set_vs_storage_buffers), each submission carries its palette's base
// offset in a mesh-attribute lane, and the vertex shader does the weighted blend -- so a
// whole pack of independently-animated foxes coalesces into a single instanced draw
// (the HUD prints the receipts via cf_draw3d_stats). Everything downstream --
// transforms, texturing, the shape API's strokes for the skeleton overlay -- is
// ordinary draw3d.
//
// Every frame, per fox: rest pose -> cm_animate (each fox its own clip + phase) ->
// cm_world_transforms -> cm_skin_palette into that fox's region of the shared buffer,
// then one cf_update_storage_buffer for the lot. Thin strokes trace the last fox's
// posed skeleton.
//
// Drag to orbit, mouse wheel to zoom, space to toggle the skeleton overlay.
//
// Fox model: CC0 by PixelMannen; rigging and animations CC-BY 4.0 by @tomkranis,
// via the Khronos glTF-Sample-Assets repository.

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>

// CF's library compiles the loader's implementation (src/cute_model.cpp); consumers just
// include the headers for the CM_* data and animation API.
#include <cute/ckit.h>
#include <cute/cute_model.h>

typedef struct Vertex
{
	CF_V3 pos;
	CF_V3 n;
	CF_V2 uv;
	uint16_t joints[4]; // Bone indices, straight from the loader: USHORT4 feeds a uvec4 input.
	float weights[4];
} Vertex;

// The skinning blend happens upstream of the ordinary draw3d contract; u_bones holds
// this frame's posed * inverse-bind palettes, one region per fox in a single SSBO.
static const char* s_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec3 in_normal;\n"
"layout (location = 2) in vec2 in_uv;\n"
"layout (location = 3) in uvec4 in_joints;\n"
"layout (location = 4) in vec4 in_weights;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec3 v_normal;\n"
"layout (location = 1) out vec2 v_uv;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"// Every fox's palette lives in ONE storage buffer; in_mesh_attributes.x carries each\n"
"// submission's base offset, so the whole pack coalesces into a single instanced draw.\n"
"layout (std430, set = 0, binding = 0) readonly buffer bones_buffer {\n"
"    vec4 u_bones[];\n"
"};\n"
"mat4 bone(uint j) {\n"
"    uint i = j * 4u;\n"
"    return mat4(u_bones[i], u_bones[i + 1u], u_bones[i + 2u], u_bones[i + 3u]);\n"
"}\n"
"void main() {\n"
"    uint base = uint(in_mesh_attributes.x);\n"
"    mat4 skin = bone(base + in_joints.x) * in_weights.x + bone(base + in_joints.y) * in_weights.y\n"
"              + bone(base + in_joints.z) * in_weights.z + bone(base + in_joints.w) * in_weights.w;\n"
"    vec3 pos = (skin * vec4(in_pos, 1.0)).xyz;\n"
"    vec3 nrm = normalize((skin * vec4(in_normal, 0.0)).xyz);\n"
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

// Interleaves the loader's separate streams into one indexed draw3d mesh via cm_interleave:
// one CM_VertexAttribute per CF_VertexAttribute, same offsets. The loader hands over
// ready-to-use u32 indices, so keep them: vertex_count interleaved vertices instead of
// index_count de-indexed ones (3-4x less vertex memory on typical meshes), and joints ride
// as USHORT4 into a uvec4 shader input with no widening.
static CF_Mesh s_make_mesh(const CM_Primitive* prim, int* out_count)
{
	int n = prim->vertex_count;
	Vertex* verts = (Vertex*)cf_alloc(sizeof(Vertex) * (size_t)n);
	CM_VertexAttribute streams[5] = {
		{ CM_STREAM_POSITION, CF_OFFSET_OF(Vertex, pos) },
		{ CM_STREAM_NORMAL,   CF_OFFSET_OF(Vertex, n) },
		{ CM_STREAM_UV,       CF_OFFSET_OF(Vertex, uv) },
		{ CM_STREAM_JOINTS,   CF_OFFSET_OF(Vertex, joints) },
		{ CM_STREAM_WEIGHTS,  CF_OFFSET_OF(Vertex, weights) },
	};
	cm_interleave(prim, streams, 5, (int)sizeof(Vertex), verts);
	CF_VertexAttribute attrs[5] = { 0 };
	attrs[0].name = "in_pos";     attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;  attrs[0].offset = CF_OFFSET_OF(Vertex, pos);
	attrs[1].name = "in_normal";  attrs[1].format = CF_VERTEX_FORMAT_FLOAT3;  attrs[1].offset = CF_OFFSET_OF(Vertex, n);
	attrs[2].name = "in_uv";      attrs[2].format = CF_VERTEX_FORMAT_FLOAT2;  attrs[2].offset = CF_OFFSET_OF(Vertex, uv);
	attrs[3].name = "in_joints";  attrs[3].format = CF_VERTEX_FORMAT_USHORT4; attrs[3].offset = CF_OFFSET_OF(Vertex, joints);
	attrs[4].name = "in_weights"; attrs[4].format = CF_VERTEX_FORMAT_FLOAT4;  attrs[4].offset = CF_OFFSET_OF(Vertex, weights);
	CF_Mesh mesh = cf_make_mesh(n * (int)sizeof(Vertex), attrs, 5, (int)sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, n);
	cf_mesh_set_index_buffer(mesh, prim->index_count * (int)sizeof(uint32_t), 32);
	cf_mesh_update_index_data(mesh, prim->indices, prim->index_count);
	cf_free(verts);
	*out_count = n;
	return mesh;
}

// Screenshot/exit harness, same contract as the fireflies sample: --shot <t> saves a
// numbered png of the app canvas, --exit-at <t> quits cleanly. Smoke-testable in CI.
static int s_shot_count;
static void s_screenshot(void)
{
	CF_Canvas canvas = cf_app_get_canvas();
	int w = 0, h = 0;
	cf_canvas_get_size(canvas, &w, &h);
	CF_Pixel* px = (CF_Pixel*)cf_alloc((size_t)w * h * sizeof(CF_Pixel));
	CF_Readback rb = cf_canvas_readback(canvas);
	if (rb.id) {
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, px, w * h * (int)sizeof(CF_Pixel));
		cf_destroy_readback(rb);
		CF_Image img;
		img.w = w;
		img.h = h;
		img.pix = px;
		char path[256];
		snprintf(path, sizeof(path), "/model3d_shot_%02d.png", s_shot_count++);
		cf_image_save_png(path, &img);
		printf("saved %s\n", path);
	}
	cf_free(px);
}

int main(int argc, char* argv[])
{
	float shot_times[16];
	int shot_n = 0;
	float exit_at = -1.0f;
	bool gles = false;
	for (int i = 1; i < argc; ++i) {
		if (!CF_STRCMP(argv[i], "--shot") && i + 1 < argc) { if (shot_n < 16) shot_times[shot_n++] = (float)atof(argv[++i]); }
		else if (!CF_STRCMP(argv[i], "--exit-at") && i + 1 < argc) exit_at = (float)atof(argv[++i]);
		else if (!CF_STRCMP(argv[i], "--gles")) gles = true; // Exercises the emulated-storage skinning path.
	}

	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	if (gles) options |= CF_APP_OPTIONS_GFX_OPENGL_BIT;
	CF_Result result = cf_make_app("cute_model -- skinned glTF", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC);
	cf_fs_set_write_directory(cf_fs_get_base_directory());

	// Mount this sample's data folder next to the executable.
	char* base = cf_path_normalize(cf_fs_get_base_directory());
	char* data_dir = cf_string_append(base, "/model3d_data");
	cf_fs_mount(data_dir, "/model3d_data", false);
	cf_string_free(data_dir);

	cf_canvas_set_clear_color(cf_app_get_canvas(), cf_make_color_rgb_f(0.10f, 0.11f, 0.13f));

	// One call: VFS read, GLB parse, and external URI resolution (had this been a .gltf
	// with sidecar files) all happen inside CF.
	CM_Model* model = cf_make_model("/model3d_data/Fox.glb");
	if (!model) {
		printf("cf_make_model failed: %s\n", cf_model_error());
		cf_destroy_app();
		return -1;
	}

	// The fox is one mesh, one primitive, one skin. The base color texture ships inside
	// the GLB as encoded PNG or JPEG bytes; CF decodes, mips, and uploads it (a -1 or
	// absent slot would yield a 1x1 white texture).
	int vertex_count = 0;
	CF_Mesh mesh = s_make_mesh(&model->meshes[0].primitives[0], &vertex_count);
	CF_Texture texture = cf_make_texture_from_model_image(model,
		model->material_count ? model->materials[0].base_color_texture.image : -1);
	cf_draw3d_set_texture("u_image", texture);

	const CM_Skin* skin = &model->skins[0];
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);

	// A little pack of foxes, each running its own clip at its own phase. All their
	// palettes live in one storage buffer; each submission carries its palette's base
	// offset in a mesh-attribute lane, so the whole pack renders as ONE instanced draw.
	#define FOX_COUNT 5
	CM_Transform* locals = (CM_Transform*)cf_alloc(sizeof(CM_Transform) * (size_t)model->node_count);
	float* world = (float*)cf_alloc(sizeof(float) * 16 * (size_t)model->node_count);
	float* palettes = (float*)cf_alloc(sizeof(float) * 16 * (size_t)skin->joint_count * FOX_COUNT);
	CF_StorageBufferParams sb_params = cf_storage_buffer_defaults((int)(sizeof(float) * 16 * (size_t)skin->joint_count * FOX_COUNT));
	sb_params.graphics_readable = true;
	CF_StorageBuffer bones_buf = cf_make_storage_buffer(sb_params);

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

		// Every fox samples its own clip at its own phase; all palettes land in one
		// buffer with one upload. `world` keeps the LAST fox's pose for the skeleton
		// overlay below.
		for (int fox = 0; fox < FOX_COUNT; ++fox) {
			const CM_Animation* clip = model->animations + (fox % model->animation_count);
			float ft = t + fox * 0.7f;
			cm_rest_pose(model, locals);
			cm_animate(model, clip, fmodf(ft, clip->duration), locals);
			cm_world_transforms(model, locals, world);
			cm_skin_palette(model, 0, world, palettes + (size_t)fox * 16 * skin->joint_count);
		}
		cf_update_storage_buffer(bones_buf, palettes, (int)(sizeof(float) * 16 * (size_t)skin->joint_count * FOX_COUNT));

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
		cf_draw3d_set_vs_storage_buffers(&bones_buf, 1);
		for (int fox = 0; fox < FOX_COUNT; ++fox) {
			cf_draw3d_push();
			cf_draw3d_translate(cf_v3((fox - (FOX_COUNT - 1) * 0.5f) * 90.0f, 0, (fox & 1) ? -40.0f : 0));
			cf_draw3d_push_mesh_attributes(cf_v4((float)(fox * skin->joint_count), 0, 0, 0));
			cf_draw3d_mesh(mesh);
			cf_draw3d_pop_mesh_attributes();
			cf_draw3d_pop();
		}
		cf_draw3d_pop_shader();

		// Posed skeleton overlay through the shape API: a stroke per bone, drawn under
		// the same transform stack as the mesh. Strokes compose their render state on
		// top of the pushed one, so an always-pass depth test makes them x-ray. `world`
		// holds the LAST fox's pose, so draw it where that fox stands.
		if (show_skeleton) {
			CF_RenderState xray = cf_render_state_3d_defaults();
			xray.depth_compare = CF_COMPARE_FUNCTION_ALWAYS;
			cf_draw3d_push_render_state(xray);
			cf_draw3d_push();
			cf_draw3d_translate(cf_v3(((FOX_COUNT - 1) - (FOX_COUNT - 1) * 0.5f) * 90.0f, 0, ((FOX_COUNT - 1) & 1) ? -40.0f : 0));
			cf_draw3d_push_color(cf_make_color_rgba_f(1.0f, 1.0f, 1.0f, 0.6f));
			for (int i = 0; i < skin->joint_count; ++i) {
				int node = skin->joints[i];
				int parent = model->nodes[node].parent;
				if (parent < 0) continue;
				CF_V3 a = cf_v3(world[node * 16 + 12], world[node * 16 + 13], world[node * 16 + 14]);
				CF_V3 b = cf_v3(world[parent * 16 + 12], world[parent * 16 + 13], world[parent * 16 + 14]);
				// Thickness scales with the transform stack like the geometry it annotates,
				// so this is in the fox's own (centimeter-scale) model space.
				cf_draw3d_line(a, b, 0.8f);
			}
			cf_draw3d_pop_color();
			cf_draw3d_pop();
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

		// The receipts, through the ordinary 2d layer: the pack coalesces into ONE skinned
		// draw, so the whole frame is that plus the skeleton strokes and the ground grid.
		CF_DrawStats3d stats = cf_draw3d_stats();
		char hud[128];
		snprintf(hud, sizeof(hud), "%d foxes -- %d instances in %d total 3d draws (SSBO palettes)",
			FOX_COUNT, stats.instances, stats.commands);
		cf_push_font_size(20);
		cf_draw_push_color(cf_color_white());
		cf_draw_text(hud, cf_v2(-(float)w * 0.5f + 20, (float)h * 0.5f - 20), -1);
		cf_draw_pop_color();
		cf_pop_font_size();

		cf_app_draw_onto_screen(true);

		for (int i = 0; i < shot_n; ++i) {
			if (shot_times[i] >= 0 && t >= shot_times[i]) {
				s_screenshot();
				shot_times[i] = -1.0f;
			}
		}
		if (exit_at > 0 && t >= exit_at) break;
	}

	cf_free(locals);
	cf_free(world);
	cf_free(palettes);
	cf_destroy_storage_buffer(bones_buf);
	cf_destroy_model(model);
	cf_destroy_mesh(mesh);
	cf_destroy_texture(texture);
	cf_destroy_shader(shader);
	cf_destroy_app();
	return 0;
}
