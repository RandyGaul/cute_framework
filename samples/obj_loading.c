/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Loading a model file into cute_draw3d.h. CF deliberately reads no model formats -- a mesh
// is vertices you assemble yourself -- so this sample shows how short the missing piece
// actually is: a ~90 line OBJ parser feeding cf_make_mesh, and everything downstream
// (instancing, transforms, texturing, lighting) is the same as every other draw3d sample.
//
// The parser handles the OBJ everyone actually exports: v / vt / vn records and faces in
// any of the f v, f v/vt, f v//vn, f v/vt/vn forms, with n-gons fan-triangulated (the
// bundled knot is all quads on purpose). Indices are resolved flat -- no vertex dedup --
// which trades memory for simplicity; a shipping importer would hash the index triples.
//
// Drag to orbit, mouse wheel to zoom.

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Vertex
{
	CF_V3 pos;
	CF_V3 n;
	CF_V2 uv;
} Vertex;

//--------------------------------------------------------------------------------------------------
// The OBJ parser. Returns a ready-to-draw mesh, or a zero handle on failure.

typedef struct ObjIndex { int v, vt, vn; } ObjIndex;

// Parses one "v[/vt][/vn]" reference. OBJ is 1-based and allows negative (from-the-end)
// indices; both resolve here to 0-based.
static ObjIndex s_parse_ref(const char** p, int nv, int nvt, int nvn)
{
	ObjIndex ix = { -1, -1, -1 };
	char* end;
	ix.v = (int)strtol(*p, &end, 10);
	*p = end;
	if (**p == '/') {
		(*p)++;
		if (**p != '/') {
			ix.vt = (int)strtol(*p, &end, 10);
			*p = end;
		}
		if (**p == '/') {
			(*p)++;
			ix.vn = (int)strtol(*p, &end, 10);
			*p = end;
		}
	}
	ix.v = ix.v > 0 ? ix.v - 1 : (ix.v < 0 ? nv + ix.v : -1);
	ix.vt = ix.vt > 0 ? ix.vt - 1 : (ix.vt < 0 ? nvt + ix.vt : -1);
	ix.vn = ix.vn > 0 ? ix.vn - 1 : (ix.vn < 0 ? nvn + ix.vn : -1);
	return ix;
}

static CF_Mesh s_load_obj(const char* virtual_path, int* out_vertex_count)
{
	size_t size = 0;
	char* text = (char*)cf_fs_read_entire_file_to_memory_and_nul_terminate(virtual_path, &size);
	if (!text) return (CF_Mesh){ 0 };

	dyna CF_V3* positions = NULL;
	dyna CF_V3* normals = NULL;
	dyna CF_V2* uvs = NULL;
	dyna Vertex* verts = NULL;

	const char* p = text;
	while (*p) {
		const char* line_end = p;
		while (*line_end && *line_end != '\n') line_end++;
		if (p[0] == 'v' && p[1] == ' ') {
			CF_V3 v;
			sscanf(p + 2, "%f %f %f", &v.x, &v.y, &v.z);
			apush(positions, v);
		} else if (p[0] == 'v' && p[1] == 't' && p[2] == ' ') {
			CF_V2 v;
			sscanf(p + 3, "%f %f", &v.x, &v.y);
			v.y = 1.0f - v.y; // OBJ's v runs bottom-up; sampling here runs top-down.
			apush(uvs, v);
		} else if (p[0] == 'v' && p[1] == 'n' && p[2] == ' ') {
			CF_V3 v;
			sscanf(p + 3, "%f %f %f", &v.x, &v.y, &v.z);
			apush(normals, v);
		} else if (p[0] == 'f' && p[1] == ' ') {
			// Fan-triangulate: (0, i, i+1) for each corner past the second.
			const char* q = p + 2;
			ObjIndex corners[16];
			int n = 0;
			while (q < line_end && n < 16) {
				while (q < line_end && *q == ' ') q++;
				if (q >= line_end || *q == '\r') break;
				corners[n++] = s_parse_ref(&q, asize(positions), asize(uvs), asize(normals));
			}
			for (int i = 2; i < n; ++i) {
				ObjIndex tri[3] = { corners[0], corners[i - 1], corners[i] };
				for (int k = 0; k < 3; ++k) {
					Vertex vert = { 0 };
					if (tri[k].v >= 0 && tri[k].v < asize(positions)) vert.pos = positions[tri[k].v];
					if (tri[k].vn >= 0 && tri[k].vn < asize(normals)) vert.n = normals[tri[k].vn];
					if (tri[k].vt >= 0 && tri[k].vt < asize(uvs)) vert.uv = uvs[tri[k].vt];
					apush(verts, vert);
				}
			}
		}
		p = *line_end ? line_end + 1 : line_end;
	}

	CF_Mesh mesh = { 0 };
	int count = asize(verts);
	if (count) {
		CF_VertexAttribute attrs[3] = { 0 };
		attrs[0].name = "in_pos";
		attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
		attrs[0].offset = CF_OFFSET_OF(Vertex, pos);
		attrs[1].name = "in_normal";
		attrs[1].format = CF_VERTEX_FORMAT_FLOAT3;
		attrs[1].offset = CF_OFFSET_OF(Vertex, n);
		attrs[2].name = "in_uv";
		attrs[2].format = CF_VERTEX_FORMAT_FLOAT2;
		attrs[2].offset = CF_OFFSET_OF(Vertex, uv);
		mesh = cf_make_mesh(count * (int)sizeof(Vertex), attrs, 3, (int)sizeof(Vertex));
		cf_mesh_update_vertex_data(mesh, verts, count);
	}
	if (out_vertex_count) *out_vertex_count = count;

	afree(positions);
	afree(normals);
	afree(uvs);
	afree(verts);
	cf_free(text);
	return mesh;
}

//--------------------------------------------------------------------------------------------------
// Shaders: the draw3d contract with uvs, a checker texture, and two-tone lighting.

static const char* s_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec3 in_normal;\n"
"layout (location = 2) in vec2 in_uv;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 12) in vec4 in_nmat0;\n"
"layout (location = 13) in vec4 in_nmat1;\n"
"layout (location = 14) in vec4 in_nmat2;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec3 v_normal;\n"
"layout (location = 1) out vec2 v_uv;\n"
"layout (location = 2) out vec4 v_tint;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_normal = normalize(vec3(dot(in_nmat0.xyz, in_normal),\n"
"                              dot(in_nmat1.xyz, in_normal),\n"
"                              dot(in_nmat2.xyz, in_normal)));\n"
"    v_uv = in_uv;\n"
"    v_tint = in_mesh_attributes;\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

static const char* s_fs =
"layout (location = 0) in vec3 v_normal;\n"
"layout (location = 1) in vec2 v_uv;\n"
"layout (location = 2) in vec4 v_tint;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_image;\n"
"void main() {\n"
"    vec3 n = normalize(v_normal);\n"
"    vec3 key_dir = normalize(vec3(0.5, 0.7, 0.5));\n"
"    float key = max(dot(n, key_dir), 0.0);\n"
"    float fill = max(dot(n, -key_dir), 0.0) * 0.3;\n"
"    float hemi = 0.5 + 0.5 * n.y;\n"
"    vec3 ambient = mix(vec3(0.10, 0.09, 0.12), vec3(0.16, 0.18, 0.22), hemi);\n"
"    vec3 albedo = texture(u_image, v_uv).rgb * v_tint.rgb;\n"
"    result = vec4(albedo * (ambient + vec3(1.0, 0.94, 0.85) * key + vec3(0.4, 0.45, 0.6) * fill), 1.0);\n"
"}\n";

static CF_Texture s_make_checker(void)
{
	enum { N = 64 };
	CF_Pixel px[N * N];
	for (int y = 0; y < N; ++y) {
		for (int x = 0; x < N; ++x) {
			int on = ((x / 8) + (y / 8)) & 1;
			px[y * N + x] = on ? cf_make_pixel_rgb(235, 232, 225) : cf_make_pixel_rgb(160, 120, 90);
		}
	}
	CF_TextureParams params = cf_texture_defaults(N, N);
	params.filter = CF_FILTER_LINEAR;
	CF_Texture tex = cf_make_texture(params);
	cf_texture_update(tex, px, (int)sizeof(px));
	return tex;
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("cute_draw3d -- obj loading", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;

	// Mount this sample's data folder next to the executable.
	char* base = cf_path_normalize(cf_fs_get_base_directory());
	char* data_dir = cf_string_append(base, "/obj_loading_data");
	cf_fs_mount(data_dir, "/obj_loading_data", false);
	cf_string_free(data_dir);

	cf_canvas_set_clear_color(cf_app_get_canvas(), cf_make_color_rgb_f(0.11f, 0.11f, 0.14f));

	int vertex_count = 0;
	CF_Mesh knot = s_load_obj("/obj_loading_data/knot.obj", &vertex_count);
	if (!knot.id) {
		printf("Failed to load /obj_loading_data/knot.obj\n");
		cf_destroy_app();
		return -1;
	}
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);
	CF_Texture checker = s_make_checker();
	cf_draw3d_set_texture("u_image", checker);

	// Orbit camera state.
	float azimuth = 0.6f, elevation = 0.45f, distance = 8.0f;
	bool dragging = false;
	float last_mx = 0, last_my = 0;
	float t = 0;

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		// Drag to orbit, wheel to zoom.
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
		distance = cf_clamp(distance - cf_mouse_wheel_motion() * 0.6f, 3.5f, 20.0f);

		int w, h;
		cf_app_get_size(&w, &h);
		float ce = CF_COSF(elevation);
		CF_V3 eye = cf_v3(CF_SINF(azimuth) * ce * distance, CF_SINF(elevation) * distance, CF_COSF(azimuth) * ce * distance);
		cf_draw3d_push_projection(cf_perspective(CF_PI / 3.6f, (float)w / (float)h, 0.1f, 100.0f));
		cf_draw3d_push_view(cf_look_at(eye, cf_v3(0, 0, 0), cf_v3(0, 1, 0)));
		cf_draw3d_push_shader(shader);

		// The loaded mesh is a mesh like any other: submit it a few times and the copies
		// coalesce into one instanced draw.
		for (int i = 0; i < 3; ++i) {
			cf_draw3d_push();
			if (i) {
				float side = i == 1 ? -1.0f : 1.0f;
				cf_draw3d_translate(cf_v3(side * 5.2f, 0.6f * side, 0));
				cf_draw3d_scale(cf_v3(0.45f));
				cf_draw3d_rotate(cf_quat_from_axis_angle(cf_v3(0.3f, 1, 0), t * 0.7f * side));
			} else {
				cf_draw3d_rotate(cf_quat_from_axis_angle(cf_v3(0, 1, 0), t * 0.15f));
			}
			cf_draw3d_push_mesh_attributes(i == 0 ? cf_v4(1, 1, 1, 1) : cf_v4(0.8f, 0.9f, 1.1f, 1));
			cf_draw3d_mesh(knot);
			cf_draw3d_pop_mesh_attributes();
			cf_draw3d_pop();
		}

		cf_draw3d_pop_shader();
		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();

		// The HUD rides the same command stream.
		char hud[256];
		snprintf(hud, sizeof(hud),
			"knot.obj: %d vertices through a ~90 line parser into cf_make_mesh.\n"
			"drag orbit   wheel zoom", vertex_count);
		cf_draw_push_color(cf_make_color_rgb_f(0.85f, 0.87f, 0.92f));
		cf_draw_text(hud, cf_v2(-(float)w * 0.5f + 20.0f, (float)h * 0.5f - 20.0f), -1);
		cf_draw_pop_color();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_texture(checker);
	cf_destroy_shader(shader);
	cf_destroy_mesh(knot);
	cf_destroy_app();
	return 0;
}
