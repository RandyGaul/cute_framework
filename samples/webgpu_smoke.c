/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// CI smoke page for the web build with CF_WEBGPU=ON (see tools/web_smoke.py). Draws a red box
// to an offscreen canvas, presents it, reads the canvas back, and reports the backend, the center
// pixel, and every console error to the page title and to POST /result.

#include <cute.h>
#include <emscripten/emscripten.h>

EM_JS(void, s_hook_console, (void), {
	window.cf_smoke_errors = [];
	const error = console.error.bind(console);
	console.error = (...args) => { window.cf_smoke_errors.push(args.join(" ")); error(...args); };
	window.addEventListener("error", (e) => window.cf_smoke_errors.push("uncaught: " + e.message));
	window.addEventListener("unhandledrejection", (e) => window.cf_smoke_errors.push("unhandled rejection: " + e.reason));
});

EM_JS(void, s_report, (const char* backend, int r, int g, int b), {
	const report = { backend: UTF8ToString(backend), pixel: [r, g, b], errors: window.cf_smoke_errors };
	document.title = "CF_SMOKE " + report.backend + " " + report.pixel.join(",") + " errors=" + report.errors.length;
	console.log(document.title);
	fetch("/result", { method: "POST", body: JSON.stringify(report) });
});

int main(int argc, char* argv[])
{
	s_hook_console();

	int w = 64, h = 64;
	const char* backend = "INVALID";
	int r = -1, g = -1, b = -1;
	if (!cf_is_error(cf_make_app("webgpu smoke", 0, 0, 0, w, h, CF_APP_OPTIONS_NO_AUDIO_BIT, argv[0]))) {
		backend = cf_backend_type_to_string(cf_query_backend());
		CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(w, h));
		CF_Readback readback = { 0 };
		for (int frame = 0; frame < 120 && cf_app_is_running(); ++frame) {
			cf_app_update(NULL);
			cf_draw_push_color(cf_color_red());
			cf_draw_box_fill(cf_make_aabb(cf_v2(-16, -16), cf_v2(16, 16)), 0);
			cf_draw_pop_color();
			cf_render_to(canvas, true);
			cf_draw_canvas(canvas, cf_v2(0, 0), cf_v2((float)w, (float)h));
			if (frame == 4) readback = cf_canvas_readback(canvas);
			if (readback.id && cf_readback_ready(readback)) {
				CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
				cf_readback_data(readback, px, w * h * (int)sizeof(CF_Pixel));
				CF_Pixel c = px[(h / 2) * w + w / 2];
				r = c.colors.r; g = c.colors.g; b = c.colors.b;
				cf_free(px);
				cf_destroy_readback(readback);
				readback.id = 0;
			}
			cf_app_draw_onto_screen(true);
			if (r >= 0) break;
		}
		cf_destroy_canvas(canvas);
		cf_destroy_app();
	}

	s_report(backend, r, g, b);
	return 0;
}
