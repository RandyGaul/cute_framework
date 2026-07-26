// Inspired by (and closely following) three.js's pixelation example:
//   https://threejs.org/examples/webgl_postprocessing_pixel.html
//
// A port of it onto CF's low-level graphics API: a 3d scene rendered into a low-resolution canvas
// and composited back with a pixel-art edge pass.
//
// Four passes, none of which needed anything CF did not already have:
//   1. shadow   -- scene depth from the light's point of view, into a 1024x1024 float canvas
//   2. colour   -- Phong shading with shadows and a warm spot light, into a 320x240 canvas
//   3. g-buffer -- world normal + depth, into a second 320x240 float canvas
//   4. composite -- upscale the colour buffer and shade its edges using the g-buffer
//
// The edge pass is the part worth reading: depth edges darken and normal edges *brighten*, as a
// multiplier on the texel rather than a blend toward an ink colour. See pixel_post.shd.
//
// The camera is orthographic, as three.js's is. That is not just a look: with an orthographic
// projection gl_FragCoord.z is linear, which is what lets the edge thresholds ported from
// three.js work without retuning.
//
// Geometry is built procedurally -- CF reads no model format, and a sample should not need an
// asset pipeline.
//
// Mouse: drag to orbit, wheel to zoom. R toggles an idle spin (off by default).
// Keys:  TAB opens the controls panel; SPACE cycles edge modes; N and D show the normal and depth
//        buffers; S toggles shadows; 1/2 change the pixel size.

#include <cute.h>
#include <dcimgui.h>

#include "proggy.h"

#define WINDOW_W 960
#define WINDOW_H 720
#define SHADOW_SIZE 2048       // three.js: directionalLight.shadow.mapSize.set(2048, 2048)
#define SPOT_SHADOW_SIZE 1024  // The spot covers a much smaller area, so it needs less.
#define SPOT_ANGLE (CF_PI / 16.0f)
#define SPOT_RANGE 10.0f

// An orthographic camera frames by its view height, so the distance only has to keep the scene
// between the near and far planes.
#define CAM_DISTANCE 2.3094f   // length of three.js's (0, 2*tan(PI/6), 2)
#define ZNEAR 0.1f
#define ZFAR 10.0f

#define MAX_OBJECTS 8

typedef struct Object
{
	CF_Mesh mesh;
	CF_V3 position;
	float spin;        // Radians per second about y; 0 for static objects.
	float spin_phase;
	CF_Color color;
	CF_Color emissive;
	float textured;    // 1 to sample the checker texture, 0 for a flat colour.
	float shininess;
	float specular;    // three.js's MeshPhongMaterial.specular, as a linear grey.
} Object;

// ---------------------------------------------------------------------------------------------
// Procedural geometry. Every face gets its own vertices so normals stay flat -- sharing corner
// vertices between faces averages the normals and rounds off exactly the creases the edge pass
// is looking for.

static void push_face(CF_Vertex3D* verts, uint32_t* indices, int* vc, int* ic,
                      CF_V3 center, CF_V3 tangent, CF_V3 bitangent, CF_V3 normal, float uv_tiles)
{
	int base = *vc;
	const float signs[4][2] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
	const float uvs[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
	for (int i = 0; i < 4; ++i) {
		verts[base + i].pos = cf_add(center, cf_add(cf_mul(tangent, signs[i][0]), cf_mul(bitangent, signs[i][1])));
		verts[base + i].normal = normal;
		verts[base + i].uv = cf_v2(uvs[i][0] * uv_tiles, uvs[i][1] * uv_tiles);
	}
	*vc += 4;
	uint32_t quad[6] = { 0, 1, 2, 0, 2, 3 };
	for (int i = 0; i < 6; ++i) indices[(*ic)++] = (uint32_t)base + quad[i];
}

static CF_Mesh build_box(CF_V3 half, float uv_tiles)
{
	CF_Vertex3D verts[24];
	uint32_t indices[36];
	int vc = 0, ic = 0;

	// Tangent/bitangent per face chosen so each quad winds counter-clockwise seen from outside,
	// which is what CF_CULL_MODE_BACK expects.
	const float n[6][3] = { {0,0,1}, {0,0,-1}, {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0} };
	const float t[6][3] = { {1,0,0}, {-1,0,0}, {0,0,-1}, {0,0,1}, {1,0,0}, {1,0,0} };
	const float b[6][3] = { {0,1,0}, {0,1,0}, {0,1,0}, {0,1,0}, {0,0,-1}, {0,0,1} };

	for (int f = 0; f < 6; ++f) {
		CF_V3 nv = cf_v3(n[f][0], n[f][1], n[f][2]);
		CF_V3 tv = cf_mul(cf_v3(t[f][0], t[f][1], t[f][2]), half);
		CF_V3 bv = cf_mul(cf_v3(b[f][0], b[f][1], b[f][2]), half);
		push_face(verts, indices, &vc, &ic, cf_mul(nv, half), tv, bv, nv, uv_tiles);
	}
	return cf_make_mesh_3d(verts, vc, indices, ic);
}

// Faceted icosahedron, standing in for three.js's IcosahedronGeometry crystal. Flat per-face
// normals are the whole point here -- the edge pass turns each facet boundary into a highlight.
static CF_Mesh build_icosahedron(float radius)
{
	const float t = 1.618034f; // Golden ratio.
	const float raw[12][3] = {
		{ -1,  t,  0 }, {  1,  t,  0 }, { -1, -t,  0 }, {  1, -t,  0 },
		{  0, -1,  t }, {  0,  1,  t }, {  0, -1, -t }, {  0,  1, -t },
		{  t,  0, -1 }, {  t,  0,  1 }, { -t,  0, -1 }, { -t,  0,  1 },
	};
	const int faces[20][3] = {
		{ 0,11, 5 }, { 0, 5, 1 }, { 0, 1, 7 }, { 0, 7,10 }, { 0,10,11 },
		{ 1, 5, 9 }, { 5,11, 4 }, {11,10, 2 }, {10, 7, 6 }, { 7, 1, 8 },
		{ 3, 9, 4 }, { 3, 4, 2 }, { 3, 2, 6 }, { 3, 6, 8 }, { 3, 8, 9 },
		{ 4, 9, 5 }, { 2, 4,11 }, { 6, 2,10 }, { 8, 6, 7 }, { 9, 8, 1 },
	};

	CF_V3 p[12];
	for (int i = 0; i < 12; ++i) p[i] = cf_mul(cf_norm(cf_v3(raw[i][0], raw[i][1], raw[i][2])), radius);

	CF_Vertex3D verts[60];
	uint32_t indices[60];
	int vc = 0, ic = 0;
	for (int f = 0; f < 20; ++f) {
		CF_V3 a = p[faces[f][0]], b = p[faces[f][1]], c = p[faces[f][2]];
		CF_V3 nv = cf_norm(cf_cross(cf_sub(b, a), cf_sub(c, a)));
		// The face list's winding is not uniformly outward; flip any face whose normal points
		// back at the origin so back-face culling keeps the right half.
		if (cf_dot(nv, a) < 0) { CF_V3 tmp = b; b = c; c = tmp; nv = cf_neg(nv); }

		int base = vc;
		CF_V3 tri[3] = { a, b, c };
		const float uvs[3][2] = { { 0, 0 }, { 1, 0 }, { 0.5f, 1 } };
		for (int i = 0; i < 3; ++i) {
			verts[base + i].pos = tri[i];
			verts[base + i].normal = nv;
			verts[base + i].uv = cf_v2(uvs[i][0], uvs[i][1]);
		}
		vc += 3;
		for (int i = 0; i < 3; ++i) indices[ic++] = (uint32_t)(base + i);
	}
	return cf_make_mesh_3d(verts, vc, indices, ic);
}

static CF_Texture make_checker_texture(void)
{
	enum { N = 32 };
	CF_Pixel px[N * N];
	for (int y = 0; y < N; ++y) {
		for (int x = 0; x < N; ++x) {
			bool on = ((x / 16) + (y / 16)) % 2 == 0;
			px[y * N + x] = on ? cf_make_pixel_rgb(222, 220, 212) : cf_make_pixel_rgb(150, 152, 160);
		}
	}
	CF_TextureParams params = cf_texture_defaults(N, N);
	params.filter = CF_FILTER_NEAREST;
	CF_Texture tex = cf_make_texture(params);
	cf_texture_update(tex, px, (int)sizeof(px));
	return tex;
}

static CF_Canvas make_canvas_ex(int w, int h, CF_PixelFormat format)
{
	CF_CanvasParams params = cf_canvas_defaults(w, h);
	params.target.pixel_format = format;
	params.target.filter = CF_FILTER_NEAREST; // Upscaling must not blur the pixels.
	params.depth_stencil_enable = true;       // Without this all depth render state is ignored.
	return cf_make_canvas(params);
}

// `normal_matrix` is passed in rather than derived from `model` because the passes want normals
// in different spaces: the lit pass shades in world space, while the g-buffer needs VIEW space.
// three.js's normal buffer is view-space (MeshNormalMaterial), and the edge shader's
// vec3(1,1,1) bias is written against that -- feeding it world normals makes which crease
// brightens depend on world orientation, so highlights swim as the camera orbits.
// Uploads to both stages' copies of the shared block. On desktop the vertex and fragment blocks
// are separate descriptor sets and each stage would only need its own half. GLES links both
// stages into one program where `uniform_block` resolves to a single GL block, so only one
// stage's buffer binding survives -- writing identical contents to both means it does not matter
// which one wins. See pixel_3d_data/uniform_members.shd.
static void set_uniform_both(CF_Material m, const char* name, void* value, CF_UniformType type)
{
	cf_material_set_uniform_vs(m, name, value, type, 1);
	cf_material_set_uniform_fs(m, name, value, type, 1);
}

static void set_transform_uniforms(CF_Material m, CF_M4x4 mvp, CF_M4x4 model, CF_M4x4 normal_matrix, CF_M4x4 light_mvp, CF_M4x4 spot_mvp)
{
	set_uniform_both(m, "u_mvp", &mvp, CF_UNIFORM_TYPE_MAT4);
	set_uniform_both(m, "u_model", &model, CF_UNIFORM_TYPE_MAT4);
	set_uniform_both(m, "u_normal_matrix", &normal_matrix, CF_UNIFORM_TYPE_MAT4);
	set_uniform_both(m, "u_light_mvp", &light_mvp, CF_UNIFORM_TYPE_MAT4);
	set_uniform_both(m, "u_spot_mvp", &spot_mvp, CF_UNIFORM_TYPE_MAT4);
}

// three.js's crystal spin easing: hold still for `downtime` seconds, then ease through a full
// turn over the rest of `period`. Reproduced so the motion reads the same.
static float ease_in_out_cubic(float x) { return x * x * 3.0f - x * x * x * 2.0f; }

static float linear_step(float x, float edge0, float edge1)
{
	float m = 1.0f / (edge1 - edge0);
	return cf_clamp(-m * edge0 + m * x, 0.0f, 1.0f);
}

static float stop_go_eased(float x, float downtime, float period)
{
	float cycle = CF_FLOORF(x / period);
	float tween = x - cycle * period;
	return cycle + ease_in_out_cubic(linear_step(tween, downtime, period));
}

// three.js's pixelAlignFrustum. Without it the low-resolution grid is fixed in screen space while
// the scene slides underneath it, so every edge crawls and shimmers as the camera moves. Nudging
// the frustum by the camera's sub-pixel offset pins the grid to the world instead.
static CF_M4x4 pixel_aligned_ortho(CF_M4x4 view, CF_V3 eye, float world_w, float world_h,
                                   int lowres_w, int lowres_h, bool align)
{
	float half_w = world_w * 0.5f, half_h = world_h * 0.5f;
	if (!align) return cf_ortho(-half_w, half_w, -half_h, half_h, ZNEAR, ZFAR);

	float pixel_w = world_w / (float)lowres_w;
	float pixel_h = world_h / (float)lowres_h;

	// A view matrix is the inverse of the camera's transform, so its first two rows are the
	// camera's right and up axes in world space.
	CF_V3 cam_right = cf_v3(view.elements[0], view.elements[4], view.elements[8]);
	CF_V3 cam_up = cf_v3(view.elements[1], view.elements[5], view.elements[9]);

	// How far along those axes the camera sits, measured in pixels, and the leftover fraction.
	float along_right = cf_dot(eye, cam_right) / pixel_w;
	float along_up = cf_dot(eye, cam_up) / pixel_h;
	float fract_x = (along_right - CF_ROUNDF(along_right)) * pixel_w;
	float fract_y = (along_up - CF_ROUNDF(along_up)) * pixel_h;

	return cf_ortho(-half_w - fract_x, half_w - fract_x,
	                -half_h - fract_y, half_h - fract_y, ZNEAR, ZFAR);
}

static CF_M4x4 object_model(const Object* o, float time)
{
	CF_M4x4 translate = cf_m4_translate(o->position);
	float angle = o->spin_phase + time * o->spin;
	if (angle == 0) return translate;
	return cf_mul(translate, cf_m4_rotate_y(angle));
}

int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app("Pixel 3D", 0, 0, 0, WINDOW_W, WINDOW_H,
		CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);
	if (cf_is_error(result)) return -1;

	// Mount this sample's data folder, then point the shader directory at it so the shaders can
	// be found and hot-reloaded while the sample runs. The mount has to come first, or
	// cf_shader_directory walks a path that does not exist yet.
	char* base = cf_path_normalize(cf_fs_get_base_directory());
	char* data_dir = cf_string_append(base, "/pixel_3d_data");
	cf_fs_mount(data_dir, "/pixel_3d_data", false);
	cf_string_free(data_dir);
	cf_shader_directory("/pixel_3d_data");
	cf_app_init_imgui();
	cf_make_font_from_memory(proggy_data, proggy_sz, "ProggyClean");

	// One vertex stage, three fragment stages. This is what the two-file cf_make_shader path is
	// for: a draw shader has no hook for a custom vertex stage.
	CF_Shader lit_shader = cf_make_shader("/pixel_3d_data/mesh.vs", "/pixel_3d_data/mesh_lit.fs");
	CF_Shader normal_shader = cf_make_shader("/pixel_3d_data/mesh.vs", "/pixel_3d_data/mesh_normal.fs");
	CF_Shader shadow_shader = cf_make_shader("/pixel_3d_data/mesh.vs", "/pixel_3d_data/mesh_shadow.fs");
	CF_Shader post_shader = cf_make_draw_shader("pixel_post.shd");

	CF_Texture albedo = make_checker_texture();

	// three.js uses a flat PlaneGeometry here. A closed slab instead, because a single-sided quad
	// disappears under back-face culling on the GLES backend while surviving on SDL_GPU -- the two
	// disagree on its winding. Closed geometry is correct on both and keeps culling on.
	CF_Mesh ground_mesh = build_box(cf_v3(1.0f, 0.02f, 1.0f), 3.0f);
	CF_Mesh box_mesh = build_box(cf_v3(0.2f, 0.2f, 0.2f), 1.5f);     // three.js: addBox(.4, 0, 0, PI/4)
	CF_Mesh tall_mesh = build_box(cf_v3(0.25f, 0.25f, 0.25f), 1.5f); // three.js: addBox(.5, -.5, -.5, PI/4)
	CF_Mesh crystal_mesh = build_icosahedron(0.2f);

	// Colours lifted from the three.js example: 0x151729 background, a warm 0xfffecd key light,
	// a cool 0x757f8e ambient, a 0xffc100 spot, and a 0x68b7e9 crystal with 0x4f7e8b emissive.
	Object objects[MAX_OBJECTS];
	int object_count = 0;
	objects[object_count++] = (Object){ ground_mesh, cf_v3(0, -0.02f, 0), 0, 0,
		cf_make_color_rgb_f(1.0f, 1.0f, 1.0f), cf_make_color_rgb_f(0, 0, 0), 1.0f, 30.0f, 0.03f };
	objects[object_count++] = (Object){ box_mesh, cf_v3(0, 0.2f, 0), 0, CF_PI / 4.0f,
		cf_make_color_rgb_f(1.0f, 1.0f, 1.0f), cf_make_color_rgb_f(0, 0, 0), 1.0f, 30.0f, 0.03f };
	objects[object_count++] = (Object){ tall_mesh, cf_v3(-0.5f, 0.25f, -0.5f), 0, CF_PI / 4.0f,
		cf_make_color_rgb_f(1.0f, 1.0f, 1.0f), cf_make_color_rgb_f(0, 0, 0), 1.0f, 30.0f, 0.03f };
	// The crystal hovers above the small box; its transform is animated per frame below.
	int crystal = object_count;
	objects[object_count++] = (Object){ crystal_mesh, cf_v3(0, 0.7f, 0), 0, 0,
		cf_make_color_rgb_f(0.408f, 0.718f, 0.914f), cf_make_color_rgb_f(0.310f, 0.494f, 0.545f), 0, 10.0f, 0.6f };

	CF_Material lit_material = cf_make_material();
	CF_Material normal_material = cf_make_material();
	CF_Material shadow_material = cf_make_material();
	CF_RenderState rs = cf_render_state_3d_defaults();
	cf_material_set_render_state(lit_material, rs);
	cf_material_set_render_state(normal_material, rs);
	// The ground is a single-sided quad, so culling it away would punch a hole in the shadow map.
	CF_RenderState shadow_rs = rs;
	shadow_rs.cull_mode = CF_CULL_MODE_NONE;
	cf_material_set_render_state(shadow_material, shadow_rs);

	int pixel_size = 3;
	int lowres_w = WINDOW_W / pixel_size, lowres_h = WINDOW_H / pixel_size;
	CF_Canvas scene_canvas = make_canvas_ex(lowres_w, lowres_h, CF_PIXEL_FORMAT_R8G8B8A8_UNORM);
	// Normals want a signed range and the depth in alpha wants more than 8 bits, or the edge
	// thresholds land on banding instead of geometry.
	CF_Canvas normal_canvas = make_canvas_ex(lowres_w, lowres_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT);
	CF_Canvas shadow_canvas = make_canvas_ex(SHADOW_SIZE, SHADOW_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT);
	CF_Canvas spot_shadow_canvas = make_canvas_ex(SPOT_SHADOW_SIZE, SPOT_SHADOW_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT);

	// Each canvas keeps its own clear values, so no pass has to save and restore a global.
	cf_canvas_set_clear_color(scene_canvas, cf_make_color_rgb_f(0.082f, 0.090f, 0.161f));
	cf_canvas_set_clear_color(normal_canvas, cf_make_color_rgb_f(0, 0, 0));
	cf_canvas_set_clear_color(shadow_canvas, cf_make_color_rgb_f(1.0f, 1.0f, 1.0f)); // Nothing occludes by default.
	cf_canvas_set_clear_color(spot_shadow_canvas, cf_make_color_rgb_f(1.0f, 1.0f, 1.0f));

	CF_V3 light_dir = cf_norm(cf_v3(-1.0f, -1.0f, -1.0f));   // three.js puts the key light at (100,100,100).
	CF_V3 spot_pos = cf_v3(2.0f, 2.0f, 0); // three.js: SpotLight at (2, 2, 0)
	// Colours are linear here and gamma-encoded at the end of mesh_lit.fs, as three.js renders
	// linear and converts on output. Intensities are three.js's divided by PI, which is the
	// Lambert BRDF's 1/PI folded in -- that is what keeps shadowed ground at a readable mid grey
	// instead of crushing it to black.
	CF_V4 light_color = cf_v4(0.477f, 0.474f, 0.298f, 1.0f); // 0xfffecd at intensity 1.5
	CF_V4 ambient = cf_v4(0.175f, 0.208f, 0.264f, 1.0f);     // 0x757f8e at intensity 3
	CF_V4 spot_color = cf_v4(1.0f, 0.535f, 0, 3.2f);         // 0xffc100 at intensity 10
	CF_V4 spot_params = cf_v4(CF_COSF(SPOT_ANGLE), 0.02f, SPOT_RANGE, 2.0f);

	// Orbit camera. Distance only has to clear the near plane -- an orthographic projection
	// takes its framing from view_height, not from how far away the camera sits.
	float cam_azimuth = 0, cam_elevation = CF_PI / 6.0f; // three.js's starting angle.
	float view_height = 2.0f;             // top = 1, bottom = -1
	bool auto_rotate = false;
	bool dragging = false;
	float last_mx = 0, last_my = 0;

	float normal_edge_strength = 0.3f; // three.js defaults.
	float depth_edge_strength = 0.4f;
	bool shadows_on = true;
	bool pixel_aligned_panning = true; // three.js defaults this on.
	bool show_controls = false; // Off by default: the scene is the point, and TAB brings it up.
	int view_mode = 0;
	int edge_mode = 0;
	float t = 0;

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		// Dear ImGui gets first refusal on input: without this, dragging a slider also drags the
		// camera, and a keyboard shortcut fires while typing a value into a widget.
		ImGuiIO* io = ImGui_GetIO();
		bool ui_wants_mouse = io->WantCaptureMouse;
		bool ui_wants_keys = io->WantCaptureKeyboard;

		if (!ui_wants_keys) {
			if (cf_key_just_pressed(CF_KEY_N)) view_mode = (view_mode == 1) ? 0 : 1;
			if (cf_key_just_pressed(CF_KEY_D)) view_mode = (view_mode == 2) ? 0 : 2;
			if (cf_key_just_pressed(CF_KEY_S)) shadows_on = !shadows_on;
			if (cf_key_just_pressed(CF_KEY_SPACE)) edge_mode = (edge_mode + 1) % 4;
			// Same range as the slider, so the two ways of changing it agree.
			if (cf_key_just_pressed(CF_KEY_1) && pixel_size > 1) pixel_size--;
			if (cf_key_just_pressed(CF_KEY_2) && pixel_size < 16) pixel_size++;
			if (cf_key_just_pressed(CF_KEY_R)) auto_rotate = !auto_rotate;
			if (cf_key_just_pressed(CF_KEY_TAB)) show_controls = !show_controls;
		}

		// Drag to orbit. The idle spin is off by default so it never fights the mouse; grabbing
		// the camera also switches it off if R turned it on.
		float mx = cf_mouse_x(), my = cf_mouse_y();
		if (ui_wants_mouse) {
			// Drop any in-progress orbit, so letting go over a widget cannot resume it later
			// with a stale anchor and snap the camera.
			dragging = false;
		} else if (cf_mouse_down(CF_MOUSE_BUTTON_LEFT)) {
			if (dragging) {
				cam_azimuth -= (mx - last_mx) * 0.01f;
				cam_elevation += (my - last_my) * 0.01f;
				// Stop just short of the poles, where the up vector becomes degenerate and
				// cf_look_at's cross product collapses.
				cam_elevation = cf_clamp(cam_elevation, -1.45f, 1.45f);
				auto_rotate = false;
			}
			dragging = true;
		} else {
			dragging = false;
		}
		last_mx = mx;
		last_my = my;

		float wheel = cf_mouse_wheel_motion();
		if (wheel != 0 && !ui_wants_mouse) view_height = cf_clamp(view_height - wheel * 0.25f, 0.8f, 8.0f);

		if (auto_rotate) cam_azimuth += CF_DELTA_TIME * 0.25f;

		// Resizing the low-resolution buffers means rebuilding them.
		int want_w = cf_app_get_width() / pixel_size, want_h = cf_app_get_height() / pixel_size;
		if (want_w != lowres_w || want_h != lowres_h) {
			lowres_w = want_w > 1 ? want_w : 1;
			lowres_h = want_h > 1 ? want_h : 1;
			cf_destroy_canvas(scene_canvas);
			cf_destroy_canvas(normal_canvas);
			scene_canvas = make_canvas_ex(lowres_w, lowres_h, CF_PIXEL_FORMAT_R8G8B8A8_UNORM);
			normal_canvas = make_canvas_ex(lowres_w, lowres_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT);
			cf_canvas_set_clear_color(scene_canvas, cf_make_color_rgb_f(0.082f, 0.090f, 0.161f));
			cf_canvas_set_clear_color(normal_canvas, cf_make_color_rgb_f(0, 0, 0));
		}

		float ns = (edge_mode == 0 || edge_mode == 1) ? normal_edge_strength : 0.0f;
		float ds = (edge_mode == 0 || edge_mode == 2) ? depth_edge_strength : 0.0f;

		// --- Camera: orthographic, slowly orbiting, exactly as three.js frames it. ---
		// --- Crystal: hovers, spins in bursts, and pulses its glow through six discrete levels
		//     so the brightness steps land on the pixel-art aesthetic rather than sliding. ---
		objects[crystal].position = cf_v3(0, 0.7f + CF_SINF(t * 2.0f) * 0.05f, 0);
		objects[crystal].spin_phase = stop_go_eased(t, 6.0f, 8.0f) * 2.0f * CF_PI;
		// three.js quantizes this to six discrete levels; a smooth pulse reads better here.
		float emissive_intensity = CF_SINF(t * 3.0f) * 0.5f + 0.5f;

		float aspect = (float)lowres_w / (float)lowres_h;
		float ce = CF_COSF(cam_elevation);
		CF_V3 eye = cf_mul(cf_v3(CF_SINF(cam_azimuth) * ce, CF_SINF(cam_elevation), CF_COSF(cam_azimuth) * ce), CAM_DISTANCE);
		CF_M4x4 view = cf_look_at(eye, cf_v3(0, 0, 0), cf_v3(0, 1.0f, 0));
		CF_M4x4 proj = pixel_aligned_ortho(view, eye, aspect * view_height, view_height,
		                                   lowres_w, lowres_h, pixel_aligned_panning);
		CF_M4x4 view_proj = cf_mul(proj, view);

		// --- Light: an orthographic view from the sun, tight enough to keep texel density up. ---
		CF_V3 light_eye = cf_mul(light_dir, -4.0f);
		CF_M4x4 light_view = cf_look_at(light_eye, cf_v3(0, 0, 0), cf_v3(0, 1.0f, 0));
		CF_M4x4 light_proj = cf_ortho(-1.5f, 1.5f, -1.5f, 1.5f, 0.1f, 10.0f);
		CF_M4x4 light_vp = cf_mul(light_proj, light_view);

		// The spot is a cone, so its shadow needs a perspective frustum rather than a box. The
		// field of view is the full cone angle plus margin, so the penumbra does not run off the
		// edge of the map.
		CF_M4x4 spot_view = cf_look_at(spot_pos, cf_v3(0, 0, 0), cf_v3(0, 1.0f, 0));
		CF_M4x4 spot_proj = cf_perspective(SPOT_ANGLE * 2.6f, 1.0f, 0.1f, SPOT_RANGE);
		CF_M4x4 spot_vp = cf_mul(spot_proj, spot_view);

		// --- Pass 1: shadow map. ---
		cf_apply_canvas(shadow_canvas, true);
		if (shadows_on) {
			for (int i = 0; i < object_count; ++i) {
				CF_M4x4 model = object_model(&objects[i], t);
				CF_M4x4 light_mvp = cf_mul(light_vp, model);
				set_transform_uniforms(shadow_material, light_mvp, model, cf_m4_normal_matrix(model), light_mvp, light_mvp);
				cf_apply_mesh(objects[i].mesh);
				cf_apply_shader(shadow_shader, shadow_material);
				cf_draw_elements();
			}
		}

		// --- Pass 1b: the spot's shadow map, so the boxes block its light too. ---
		cf_apply_canvas(spot_shadow_canvas, true);
		if (shadows_on) {
			for (int i = 0; i < object_count; ++i) {
				CF_M4x4 model = object_model(&objects[i], t);
				CF_M4x4 spot_mvp = cf_mul(spot_vp, model);
				set_transform_uniforms(shadow_material, spot_mvp, model, cf_m4_normal_matrix(model), spot_mvp, spot_mvp);
				cf_apply_mesh(objects[i].mesh);
				cf_apply_shader(shadow_shader, shadow_material);
				cf_draw_elements();
			}
		}

		// --- Pass 2: lit colour. ---
		CF_V4 light_dir4 = cf_v4_from_v3(light_dir, 0);
		CF_V4 spot_pos4 = cf_v4_from_v3(spot_pos, 1.0f);
		// three.js's SpotLight target defaults to the origin; aiming below the floor drags the pool
		// toward the light and flattens it.
		CF_V3 spot_target = cf_v3(0, 0, 0);
		CF_V4 spot_dir4 = cf_v4_from_v3(cf_norm(cf_sub(spot_target, spot_pos)), 0);
		CF_V4 eye4 = cf_v4_from_v3(eye, 1.0f);

		cf_material_set_texture_fs(lit_material, "u_albedo", albedo);
		cf_material_set_texture_fs(lit_material, "u_shadow_map", cf_canvas_get_target(shadow_canvas));
		cf_material_set_texture_fs(lit_material, "u_spot_shadow_map", cf_canvas_get_target(spot_shadow_canvas));
		set_uniform_both(lit_material, "u_light_direction", &light_dir4, CF_UNIFORM_TYPE_FLOAT4);
		set_uniform_both(lit_material, "u_light_color", &light_color, CF_UNIFORM_TYPE_FLOAT4);
		set_uniform_both(lit_material, "u_ambient", &ambient, CF_UNIFORM_TYPE_FLOAT4);
		set_uniform_both(lit_material, "u_spot_pos", &spot_pos4, CF_UNIFORM_TYPE_FLOAT4);
		set_uniform_both(lit_material, "u_spot_dir", &spot_dir4, CF_UNIFORM_TYPE_FLOAT4);
		set_uniform_both(lit_material, "u_spot_color", &spot_color, CF_UNIFORM_TYPE_FLOAT4);
		set_uniform_both(lit_material, "u_spot_params", &spot_params, CF_UNIFORM_TYPE_FLOAT4);
		set_uniform_both(lit_material, "u_eye", &eye4, CF_UNIFORM_TYPE_FLOAT4);

		cf_apply_canvas(scene_canvas, true);
		for (int i = 0; i < object_count; ++i) {
			CF_M4x4 model = object_model(&objects[i], t);
			set_transform_uniforms(lit_material, cf_mul(view_proj, model), model,
				cf_m4_normal_matrix(model), cf_mul(light_vp, model), cf_mul(spot_vp, model));

			CF_V4 base = cf_v4(objects[i].color.r, objects[i].color.g, objects[i].color.b, 1.0f);
			float glow = (i == crystal) ? emissive_intensity : 1.0f;
			CF_V4 emissive = cf_v4(objects[i].emissive.r * glow, objects[i].emissive.g * glow, objects[i].emissive.b * glow, 1.0f);
			// z carries one shadow texel, so the fragment shader's PCF kernel does not have to
			// know the shadow map's size.
			// z and w carry one texel of each shadow map, so the fragment shader's PCF kernels do
			// not have to know either map's size. Zero means "no shadowing".
			CF_V4 material = cf_v4(objects[i].textured, objects[i].shininess,
			                       shadows_on ? 1.0f / (float)SHADOW_SIZE : 0.0f,
			                       shadows_on ? 1.0f / (float)SPOT_SHADOW_SIZE : 0.0f);
			set_uniform_both(lit_material, "u_base_color", &base, CF_UNIFORM_TYPE_FLOAT4);
			set_uniform_both(lit_material, "u_emissive", &emissive, CF_UNIFORM_TYPE_FLOAT4);
			CF_V4 specular = cf_v4(objects[i].specular, objects[i].specular, objects[i].specular, 1.0f);
			set_uniform_both(lit_material, "u_specular", &specular, CF_UNIFORM_TYPE_FLOAT4);
			set_uniform_both(lit_material, "u_material", &material, CF_UNIFORM_TYPE_FLOAT4);

			cf_apply_mesh(objects[i].mesh);
			cf_apply_shader(lit_shader, lit_material);
			cf_draw_elements();
		}

		// --- Pass 3: normal and depth g-buffer. ---
		cf_apply_canvas(normal_canvas, true);
		for (int i = 0; i < object_count; ++i) {
			CF_M4x4 model = object_model(&objects[i], t);
			set_transform_uniforms(normal_material, cf_mul(view_proj, model), model,
				cf_m4_normal_matrix(cf_mul(view, model)), cf_mul(light_vp, model), cf_mul(spot_vp, model));
			cf_apply_mesh(objects[i].mesh);
			cf_apply_shader(normal_shader, normal_material);
			cf_draw_elements();
		}

		// --- Pass 4: composite, upscaled to the window. ---
		float w = (float)cf_app_get_width(), h = (float)cf_app_get_height();
		// The draw API overrides every bound texture's own sampler with one chosen from the
		// current filter mode, which defaults to CF_DRAW_FILTER_SMOOTH -- so creating the canvas
		// with CF_FILTER_NEAREST is not enough on its own. Without this the low-resolution buffer
		// is smoothly interpolated on the way up and the pixels stop being pixels.
		cf_draw_push_filter(CF_DRAW_FILTER_NEAREST);
		cf_draw_push_shader(post_shader);
		cf_draw_set_texture("scene_tex", cf_canvas_get_target(scene_canvas));
		cf_draw_set_texture("normal_tex", cf_canvas_get_target(normal_canvas));
		cf_draw_set_uniform_v2("u_texel", cf_v2(1.0f / (float)lowres_w, 1.0f / (float)lowres_h));
		cf_draw_set_uniform_float("u_normal_edge_strength", ns);
		cf_draw_set_uniform_float("u_depth_edge_strength", ds);
		cf_draw_set_uniform_float("u_view_mode", (float)view_mode);
		cf_draw_push_shape_aa(0);
		cf_draw_box_fill(cf_make_aabb(cf_v2(-w * 0.5f, -h * 0.5f), cf_v2(w * 0.5f, h * 0.5f)), 0);
		cf_draw_pop_shape_aa();
		cf_draw_pop_shader();
		cf_draw_pop_filter();

		// --- Shortcut hints along the bottom. Drawn after the composite and outside the
		//     nearest-filter push, so the text stays crisp rather than being pixelated. ---
		{
			const char* hints = "drag orbit   wheel zoom   TAB controls   SPACE edges   N normals   D depth   S shadows   1/2 pixel size   R spin";
			cf_push_font("ProggyClean");
			cf_push_font_size(13.0f);
			CF_V2 size = cf_text_size(hints, -1);
			CF_V2 at = cf_v2(-size.x * 0.5f, -h * 0.5f + size.y + 10.0f);
			// A dark pass offset by a pixel keeps the text legible over the bright ground.
			cf_draw_push_color(cf_make_color_rgba_f(0, 0, 0, 0.65f));
			cf_draw_text(hints, cf_v2(at.x + 1.0f, at.y - 1.0f), -1);
			cf_draw_pop_color();
			cf_draw_push_color(cf_make_color_rgb_f(0.88f, 0.90f, 0.95f));
			cf_draw_text(hints, at, -1);
			cf_draw_pop_color();
			cf_pop_font_size();
			cf_pop_font();
		}

		// --- Controls, mirroring the dat.GUI panel in the three.js example. ---
		if (show_controls) {
			ImGui_Begin("Controls", &show_controls, 0);
			ImGui_SliderInt("pixelSize", &pixel_size, 1, 16);
			ImGui_SliderFloat("normalEdgeStrength", &normal_edge_strength, 0.0f, 2.0f);
			ImGui_SliderFloat("depthEdgeStrength", &depth_edge_strength, 0.0f, 1.0f);
			ImGui_Checkbox("pixelAlignedPanning", &pixel_aligned_panning);
			ImGui_End();
		}

		cf_app_draw_onto_screen(true);

	}

	cf_destroy_shader(lit_shader);
	cf_destroy_shader(normal_shader);
	cf_destroy_shader(shadow_shader);
	cf_destroy_shader(post_shader);
	cf_destroy_material(lit_material);
	cf_destroy_material(normal_material);
	cf_destroy_material(shadow_material);
	cf_destroy_canvas(scene_canvas);
	cf_destroy_canvas(normal_canvas);
	cf_destroy_canvas(shadow_canvas);
	cf_destroy_canvas(spot_shadow_canvas);
	cf_destroy_texture(albedo);
	cf_destroy_mesh(ground_mesh);
	cf_destroy_mesh(box_mesh);
	cf_destroy_mesh(tall_mesh);
	cf_destroy_mesh(crystal_mesh);
	cf_destroy_app();
	return 0;
}
