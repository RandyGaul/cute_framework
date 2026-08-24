// Records what the app draws into an .mp4, then plays that file back -- both halves of the video
// API in one program, with nothing but Cute Framework behind them.
//
// Space starts and stops recording. While recording, each finished frame is read back off the GPU
// and handed to the encoder. Stopping writes video_sample.mp4 next to the executable and opens it
// for playback; space again goes back to recording over it.
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

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT;
	CF_Result result = cf_make_app("Video", 0, 0, 0, W, H, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_fs_set_write_directory(cf_fs_get_base_directory()); // The recording lands beside the exe.

	CF_Canvas offscreen = cf_make_canvas(cf_canvas_defaults(W, H));
	CF_VideoEncoder* encoder = NULL;
	CF_Video* video = NULL;
	Mode mode = MODE_IDLE;

	// One readback in flight at a time. Asking for another before the first arrives would only
	// queue up work the encoder cannot keep up with anyway.
	CF_Readback readback = { 0 };
	bool readback_pending = false;

	float t = 0;
	int recorded = 0;
	const char* message = "space to record";

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		if (mode == MODE_PLAYING) {
			// Playback is two calls: advance the clock, draw what it leaves behind.
			cf_video_update(video, CF_DELTA_TIME);
			CF_Sprite sprite = cf_video_sprite(video);
			cf_draw_sprite(&sprite);
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
				++recorded;
			}
		}

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
						message = "playing back -- space to record again";
						mode = MODE_PLAYING;
					}
				}
			} else {
				if (video) { cf_destroy_video(video); video = NULL; }
				encoder = cf_make_video_encoder(W, H, FPS);
				recorded = 0;
				message = encoder ? "recording -- space to stop" : cf_video_error();
				mode = encoder ? MODE_RECORDING : MODE_IDLE;
			}
		}

		CF_UNUSED(recorded);
		cf_app_draw_onto_screen(true);
	}

	if (readback_pending) cf_destroy_readback(readback);
	if (encoder) cf_destroy_video_encoder(encoder);
	if (video) cf_destroy_video(video);
	cf_destroy_canvas(offscreen);
	cf_destroy_app();
	return 0;
}
