/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// CI smoke page for the web build with CF_WEBGPU=ON, loaded by web_smoke.py. Draws a red box
// to an offscreen canvas, presents it, reads the canvas back, and reports the backend, the center
// pixel, and every console error to the page title and to POST /result.

#include <cute.h>
#include <emscripten/emscripten.h>

// Software adapters on CI can take seconds to compile the first pipelines, and a readback lands
// only after them.
#define READBACK_TIMEOUT_MS 10000.0

// Reports once. An uncaught exception kills main, so the page then reports on its own: a broken
// run fails with its error instead of timing out.
EM_JS(void, s_hook_console, (void), {
	window.cf_smoke_errors = [];
	window.cf_smoke_backend = "INVALID";
	window.cf_smoke_report = (pixel) => {
		if (window.cf_smoke_reported) return;
		window.cf_smoke_reported = true;
		const report = { backend: window.cf_smoke_backend, pixel: pixel, errors: window.cf_smoke_errors };
		document.title = "CF_SMOKE " + report.backend + " " + report.pixel.join(",") + " errors=" + report.errors.length;
		console.log(document.title);
		fetch("/result", { method: "POST", body: JSON.stringify(report) });
	};
	const fail = (msg) => {
		window.cf_smoke_errors.push(msg);
		setTimeout(() => window.cf_smoke_report([-1, -1, -1]), 0);
	};
	const error = console.error.bind(console);
	console.error = (...args) => { window.cf_smoke_errors.push(args.join(" ")); error(...args); };
	window.addEventListener("error", (e) => fail("uncaught: " + e.message));
	window.addEventListener("unhandledrejection", (e) => fail("unhandled rejection: " + e.reason));
});

EM_JS(void, s_set_backend, (const char* backend), {
	window.cf_smoke_backend = UTF8ToString(backend);
});

EM_JS(void, s_error, (const char* message), {
	window.cf_smoke_errors.push(UTF8ToString(message));
});

EM_JS(void, s_report, (int r, int g, int b), {
	window.cf_smoke_report([r, g, b]);
});

int main(int argc, char* argv[])
{
	s_hook_console();

	int w = 64, h = 64;
	int r = -1, g = -1, b = -1;
	if (!cf_is_error(cf_make_app("webgpu smoke", 0, 0, 0, w, h, CF_APP_OPTIONS_NO_AUDIO_BIT, argv[0]))) {
		s_set_backend(cf_backend_type_to_string(cf_query_backend()));
		CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(w, h));
		CF_Readback readback = { 0 };
		bool done = false;
		double start = emscripten_get_now();
		for (int frame = 0; !done && cf_app_is_running(); ++frame) {
			cf_app_update(NULL);
			cf_draw_push_color(cf_color_red());
			cf_draw_box_fill(cf_make_aabb(cf_v2(-16, -16), cf_v2(16, 16)), 0);
			cf_draw_pop_color();
			cf_render_to(canvas, true);
			cf_draw_canvas(canvas, cf_v2(0, 0), cf_v2((float)w, (float)h));
			if (frame == 4) readback = cf_canvas_readback(canvas);
			if (readback.id && cf_readback_ready(readback)) {
				CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * sizeof(CF_Pixel));
				if (cf_readback_data(readback, px, w * h * (int)sizeof(CF_Pixel))) {
					CF_Pixel c = px[(h / 2) * w + w / 2];
					r = c.colors.r; g = c.colors.g; b = c.colors.b;
				} else {
					s_error("smoke: the readback finished without data");
				}
				cf_free(px);
				cf_destroy_readback(readback);
				readback.id = 0;
				done = true;
			} else if (emscripten_get_now() - start > READBACK_TIMEOUT_MS) {
				s_error("smoke: the readback never finished");
				done = true;
			}
			cf_app_draw_onto_screen(true);
		}
		// A readback still in flight belongs to the app's teardown.
		cf_destroy_canvas(canvas);
		cf_destroy_app();
	} else {
		s_error("smoke: cf_make_app failed");
	}

	s_report(r, g, b);
	return 0;
}
