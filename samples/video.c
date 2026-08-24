// Records what the app draws into an .mp4, then plays that file back -- both halves of the video
// API in one program, with nothing but Cute Framework behind them.
//
// Space starts and stops recording. While recording, each finished frame is read back off the GPU
// and handed to the encoder. Stopping writes video_sample.mp4 next to the executable and opens it
// for playback; space again goes back to recording over it. D toggles playback between a 2d sprite
// and a quad in a 3d scene, which are the two ways a decoded frame reaches the screen.
//
// Readback is asynchronous, so the frame handed to the encoder is a frame or two behind what is on
// screen. That is fine for a recording, and it is why the request and the result are handled apart.

#include <cute.h>
#include <math.h>

#define W 640
#define H 360
#define FPS 30
#define FILENAME "/video_sample.mp4"

typedef enum Mode
{
	MODE_IDLE,
	MODE_RECORDING,
	MODE_PLAYING,
} Mode;

// The draw3d contract, plus uvs remapped by in_uv_rect. That lane carries a sprite's atlas
// sub-rect when one is pushed and the full rect (0,0,1,1) when none is, so the SAME shader takes
// its image from cf_draw3d_push_texture or from cf_draw3d_set_texture without knowing which.
static const char* s_video_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec2 in_uv;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 11) in vec4 in_uv_rect;\n"
"layout (location = 0) out vec2 v_uv;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_uv = mix(in_uv_rect.xy, in_uv_rect.zw, in_uv);\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

static const char* s_video_fs =
"layout (location = 0) in vec2 v_uv;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_image;\n"
"void main() {\n"
"    result = vec4(texture(u_image, v_uv).rgb, 1.0);\n"
"}\n";

typedef struct Vertex
{
	CF_V3 pos;
	CF_V2 uv;
} Vertex;

// A screen one unit tall and as wide as the video's aspect ratio, centred on the origin. Texture
// v runs top-down, so the top edge of the quad takes v = 0.
static CF_Mesh s_make_screen(float aspect)
{
	float x = aspect * 0.5f;
	Vertex verts[6] = {
		{ { -x, -0.5f, 0 }, { 0, 1 } }, { { x, -0.5f, 0 }, { 1, 1 } }, { { x, 0.5f, 0 }, { 1, 0 } },
		{ { -x, -0.5f, 0 }, { 0, 1 } }, { { x,  0.5f, 0 }, { 1, 0 } }, { { -x, 0.5f, 0 }, { 0, 0 } },
	};
	CF_VertexAttribute attrs[2] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = CF_OFFSET_OF(Vertex, pos);
	attrs[1].name = "in_uv";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CF_OFFSET_OF(Vertex, uv);
	CF_Mesh mesh = cf_make_mesh((int)sizeof(verts), attrs, 2, (int)sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	return mesh;
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT;
	CF_Result result = cf_make_app("Video", 0, 0, 0, W, H, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_fs_set_write_directory(cf_fs_get_base_directory()); // The recording lands beside the exe.

	CF_Canvas offscreen = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_Shader video_shd = cf_make_shader_from_source(s_video_vs, s_video_fs);
	CF_Mesh screen = s_make_screen((float)W / (float)H);
	CF_VideoEncoder* encoder = NULL;
	CF_Video* video = NULL;
	Mode mode = MODE_IDLE;
	bool in_3d = false;

	// One readback in flight at a time. Asking for another before the first arrives would only
	// queue up work the encoder cannot keep up with anyway.
	CF_Readback readback = { 0 };
	bool readback_pending = false;

	float t = 0;
	const char* message = "space to record";

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		if (mode == MODE_PLAYING) {
			// Playback is one call to advance the clock, then whichever way of drawing suits.
			cf_video_update(video, CF_DELTA_TIME);
			if (in_3d) {
				// The frames go straight to a texture of their own rather than through the
				// atlas, which is what dynamic content wants: only that one image is
				// re-uploaded when the frame changes.
				int w, h;
				cf_app_get_size(&w, &h);
				float a = t * 0.6f;
				CF_V3 eye = cf_v3(sinf(a) * 2.6f, 0.9f, cosf(a) * 2.6f);
				cf_draw3d_push_projection(cf_perspective(CF_PI / 3.2f, (float)w / (float)h, 0.1f, 100.0f));
				cf_draw3d_push_view(cf_look_at(eye, cf_v3(0, 0, 0), cf_v3(0, 1, 0)));
				cf_draw3d_push_shader(video_shd);
				cf_draw3d_set_texture("u_image", cf_video_texture(video));
				cf_draw3d_mesh(screen);
				cf_draw3d_pop_shader();
				cf_draw3d_pop_view();
				cf_draw3d_pop_projection();
			} else {
				CF_Sprite sprite = cf_video_sprite(video);
				cf_draw_sprite(&sprite);
			}
			cf_draw_text(message, cf_v2(-W * 0.5f + 12, H * 0.5f - 12), -1);
		} else {
			// Something worth compressing: moving shapes over a moving background, so the motion
			// search has something to find and the picture is not a flat colour.
			for (int i = 0; i < 40; ++i) {
				float a = t * 0.7f + i * 0.157f;
				float r = 40.0f + i * 3.0f;
				CF_V2 p = cf_v2(cosf(a) * r * 2.2f, sinf(a * 1.3f) * r);
				cf_draw_push_color(cf_make_color_rgb_f(0.2f + (i % 7) * 0.1f, 0.5f, 0.9f - (i % 5) * 0.1f));
				cf_draw_circle_fill(cf_make_circle(p, 6.0f + (i % 5) * 2.0f));
				cf_draw_pop_color();
			}
			cf_draw_text(message, cf_v2(-W * 0.5f + 12, H * 0.5f - 12), -1);
			cf_render_to(offscreen, true);
			cf_draw_canvas(offscreen, cf_v2(0, 0), cf_v2((float)W, (float)H));
		}

		if (mode == MODE_RECORDING) {
			if (!readback_pending) {
				readback = cf_canvas_readback(offscreen);
				readback_pending = true;
			} else if (cf_readback_ready(readback)) {
				int size = cf_readback_size(readback);
				void* pixels = cf_alloc(size);
				cf_readback_data(readback, pixels, size);
				cf_destroy_readback(readback);
				readback_pending = false;

				CF_Image frame;
				frame.w = W;
				frame.h = H;
				frame.pix = (CF_Pixel*)pixels;
				cf_video_encoder_add_frame(encoder, frame);
				cf_free(pixels);
			}
		}

		if (cf_key_just_pressed(CF_KEY_D) && mode == MODE_PLAYING) in_3d = !in_3d;

		if (cf_key_just_pressed(CF_KEY_SPACE)) {
			if (mode == MODE_RECORDING) {
				if (readback_pending) { cf_destroy_readback(readback); readback_pending = false; }
				CF_Result saved = cf_video_encoder_save(encoder, FILENAME);
				cf_destroy_video_encoder(encoder);
				encoder = NULL;
				mode = MODE_IDLE;
				message = "space to record";
				if (cf_is_error(saved)) {
					message = "could not write the file";
				} else {
					video = cf_make_video(FILENAME);
					if (!video) {
						message = cf_video_error();
					} else {
						cf_video_set_looped(video, true);
						message = "space to record again, d for 3d";
						mode = MODE_PLAYING;
					}
				}
			} else {
				if (video) { cf_destroy_video(video); video = NULL; }
				encoder = cf_make_video_encoder(W, H, FPS);
				message = encoder ? "recording -- space to stop" : cf_video_error();
				mode = encoder ? MODE_RECORDING : MODE_IDLE;
			}
		}

		cf_app_draw_onto_screen(true);
	}

	if (readback_pending) cf_destroy_readback(readback);
	if (encoder) cf_destroy_video_encoder(encoder);
	if (video) cf_destroy_video(video);
	cf_destroy_mesh(screen);
	cf_destroy_shader(video_shd);
	cf_destroy_canvas(offscreen);
	cf_destroy_app();
	return 0;
}
