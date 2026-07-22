/*
	Tiled renderer perf comparison.

	Renders a deliberately overdraw-heavy scene of overlapping shapes, lines, and text.
	Press SPACE to toggle between the instanced path and the tiled path; watch the frame time.
	Press 1/2 to halve/double the object count. Vsync is off so frame time reflects load.
*/

#include <cute.h>
#include <internal/cute_draw_internal.h>
#include <stdio.h>

using namespace Cute;

int main(int argc, char* argv[])
{
	// `tile_perf bench [count] [radius_scale] [alpha]` runs a fixed number of frames on
	// each path (instanced, tiled, auto) and prints average frame times.
	// Large radius_scale values make the scene GPU/overdraw-bound instead of
	// CPU/record-bound; alpha 1 makes shapes opaque (exercises opaque-cover culling).
	bool bench = argc > 1 && CF_STRCMP(argv[1], "bench") == 0;
	int bench_count = argc > 2 ? atoi(argv[2]) : 4000;
	float bench_rscale = argc > 3 ? (float)atof(argv[3]) : 1.0f;
	float bench_alpha = argc > 4 ? (float)atof(argv[4]) : 0.35f;
	int bench_mode = argc > 5 ? atoi(argv[5]) : -1; // -1 = all passes; 0/1/2 = instanced/tiled/auto only (for profilers).

	// Profiler-friendly fallback: some capture tools cannot pass command lines. When
	// launched bare, read bench settings from tile_perf_args.txt next to the exe:
	// "bench <count> <rscale> <alpha> <mode>".
	if (argc <= 1) {
		FILE* fp = fopen("tile_perf_args.txt", "r");
		if (fp) {
			char word[16] = { 0 };
			if (fscanf(fp, "%15s %d %f %f %d", word, &bench_count, &bench_rscale, &bench_alpha, &bench_mode) >= 1) {
				bench = CF_STRCMP(word, "bench") == 0;
			}
			fclose(fp);
		}
	}

	int app_options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | (bench ? CF_APP_OPTIONS_HIDDEN_BIT : CF_APP_OPTIONS_RESIZABLE_BIT);
	if (getenv("CF_GLES")) app_options |= CF_APP_OPTIONS_GFX_OPENGL_BIT; // Bench the GLES3 backend.
	CF_Result result = cf_make_app("tile_perf -- SPACE toggles tiled/mesh, 1/2 halves/doubles count", 0, 0, 0, 1280, 720, app_options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_app_set_present_mode(CF_PRESENT_MODE_IMMEDIATE); // vsync off; frame time reflects load.

	int object_count = bench ? bench_count : 4000;
	bool use_tiled = cf_draw_tiled_available();
	CF_Rnd rnd = cf_rnd_seed(42);

	if (bench) {
		if (!cf_draw_tiled_available()) {
			printf("tiled path unavailable on this backend; benching the instanced path only\n");
		}
		const int warmup = 30;
		const int frames = 200;
		int pass_count = cf_draw_tiled_available() ? 3 : 1;
		int pass_first = 0;
		if (bench_mode >= 0 && bench_mode < pass_count) {
			pass_first = bench_mode;
			pass_count = bench_mode + 1;
		}
		for (int pass = pass_first; pass < pass_count; ++pass) {
			if (pass == 2) {
				cf_draw_set_tiled_auto(); // Default heuristics pick the path per batch.
			} else {
				cf_draw_set_tiled_enabled(pass == 1);
			}
			double total_ms = 0;
			double record_ms = 0;
			for (int f = 0; f < warmup + frames; ++f) {
				cf_app_update(NULL);
				CF_Rnd frame_rnd = cf_rnd_seed(42);
				uint64_t t0 = cf_get_ticks();
				for (int i = 0; i < object_count; ++i) {
					float x = cf_rnd_range_float(&frame_rnd, -600, 600);
					float y = cf_rnd_range_float(&frame_rnd, -320, 320);
					float r = cf_rnd_range_float(&frame_rnd, 10, 60) * bench_rscale;
					float hue = cf_rnd_range_float(&frame_rnd, 0, 1);
					CF_Color c = cf_hsv_to_rgb(cf_make_color_rgb_f(hue, 0.8f, 0.9f));
					c.a = bench_alpha;
					cf_draw_push_color(c);
					switch (i % 6) {
					case 0: cf_draw_circle_fill2(cf_v2(x, y), r); break;
					case 1: cf_draw_quad_fill(cf_make_aabb(cf_v2(x - r, y - r), cf_v2(x + r, y + r)), r * 0.25f); break;
					case 2: cf_draw_capsule_fill2(cf_v2(x, y - r), cf_v2(x, y + r), r * 0.4f); break;
					case 3: cf_draw_line(cf_v2(x - 200, y - 150), cf_v2(x + 200, y + 150), 3.0f); break;
					case 4: cf_draw_tri_fill(cf_v2(x, y + r), cf_v2(x - r, y - r), cf_v2(x + r, y - r), 4.0f); break;
					case 5: {
						// Zig-zag polyline (translucent -- exercises the exact-partition path).
						CF_V2 pts[6];
						for (int j = 0; j < 6; ++j) {
							pts[j] = cf_v2(x + (float)j * 20.0f, y + ((j & 1) ? 30.0f : -30.0f));
						}
						cf_draw_polyline(pts, 6, 5.0f, false);
					}	break;
					}
					cf_draw_pop_color();
				}
				uint64_t t1 = cf_get_ticks(); // Record phase ends; flush/submit/present next.
				cf_app_draw_onto_screen(true);
				double ms = (double)(cf_get_ticks() - t0) / (double)cf_get_tick_frequency() * 1000.0;
				double rms = (double)(t1 - t0) / (double)cf_get_tick_frequency() * 1000.0;
				if (f >= warmup) {
					total_ms += ms;
					record_ms += rms;
				}
			}
			const char* label = pass == 0 ? "INSTANCED" : (pass == 1 ? "TILED    " : "AUTO     ");
			printf("%s: %d objects, %.3f ms/frame avg over %d frames (record %.3f ms; record+submit+present, vsync off)\n",
				label, object_count, total_ms / frames, frames, record_ms / frames);
		}
		cf_destroy_app();
		return 0;
	}

	// Rolling frame-time average.
	double accum_ms = 0;
	int accum_frames = 0;
	double shown_ms = 0;
	int shown_tiled_batches = 0, shown_instanced_batches = 0;
	double shown_upload_kb = 0;

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		if (cf_key_just_pressed(CF_KEY_SPACE)) {
			use_tiled = !use_tiled;
		}
		if (cf_key_just_pressed(CF_KEY_1)) object_count = cf_max(object_count / 2, 125);
		if (cf_key_just_pressed(CF_KEY_2)) object_count = cf_min(object_count * 2, 128000);
		cf_draw_set_tiled_enabled(use_tiled);

		// Deterministic scene each frame (same rng seed) so both paths draw identical content.
		CF_Rnd frame_rnd = rnd;
		float t = (float)CF_SECONDS;

		for (int i = 0; i < object_count; ++i) {
			float x = cf_rnd_range_float(&frame_rnd, -600, 600);
			float y = cf_rnd_range_float(&frame_rnd, -320, 320);
			float r = cf_rnd_range_float(&frame_rnd, 10, 60);
			float hue = cf_rnd_range_float(&frame_rnd, 0, 1);
			float wob = cf_sin(t + (float)i * 0.37f) * 20.0f;
			CF_Color c = cf_hsv_to_rgb(cf_make_color_rgb_f(hue, 0.8f, 0.9f));
			c.a = 0.35f; // Translucent: forces blending, maximizes overdraw cost.
			cf_draw_push_color(c);
			switch (i % 5) {
			case 0: cf_draw_circle_fill2(cf_v2(x + wob, y), r); break;
			case 1: cf_draw_quad_fill(cf_make_aabb(cf_v2(x - r, y - r), cf_v2(x + r, y + r)), r * 0.25f); break;
			case 2: cf_draw_capsule_fill2(cf_v2(x, y - r), cf_v2(x + wob, y + r), r * 0.4f); break;
			case 3: cf_draw_line(cf_v2(x - 200, y - 150), cf_v2(x + 200 + wob, y + 150), 3.0f); break;
			case 4: cf_draw_tri_fill(cf_v2(x, y + r), cf_v2(x - r, y - r), cf_v2(x + r, y - r + wob), 4.0f); break;
			}
			cf_draw_pop_color();
		}

		// Stats overlay (drawn after -- note it batches with everything above).
		accum_ms += (double)CF_DELTA_TIME * 1000.0;
		accum_frames++;
		if (accum_frames >= 30) {
			shown_ms = accum_ms / accum_frames;
			accum_ms = 0;
			accum_frames = 0;
			uint64_t upload_bytes;
			cf_draw_tiled_stats(&shown_tiled_batches, &shown_instanced_batches, &upload_bytes);
			shown_upload_kb = upload_bytes / 1024.0;
		}

		char buf[256];
		snprintf(buf, sizeof(buf), "%s  |  %d objects  |  %.2f ms/frame (%.0f fps)\ntiled batches: %d  instanced batches: %d  tiled upload: %.0f KB",
			use_tiled ? "TILED" : "INSTANCED",
			object_count,
			shown_ms,
			shown_ms > 0 ? 1000.0 / shown_ms : 0,
			shown_tiled_batches,
			shown_instanced_batches,
			shown_upload_kb);
		cf_draw_push_color(cf_color_white());
		cf_push_font_size(20);
		cf_draw_text(buf, cf_v2(-620, 340), -1);
		cf_pop_font_size();
		cf_draw_pop_color();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();
	return 0;
}
