/*
	Custom SDF shapes (cf_make_custom_shape) + CSG shape groups (cf_draw_shape_group_begin).

	Registers two user-defined shapes -- a five-pointed star and a crescent moon -- as
	GLSL distance-function snippets, then draws them through the regular draw API.
	Custom shapes batch with builtin shapes/sprites/text, and antialiasing, stroked
	outlines, colors, and layers all apply automatically.

	Custom shapes need runtime shader compilation, which web/bytecode-only builds lack.
	There the sample falls back to CSG shape groups -- boolean ops over plain draw calls
	-- which are data-driven and work on every backend: the star becomes a union of five
	triangles and a pentagon, the moon a circle-minus-circle, each still one command
	with a composite outline.

	Run with `custom_shapes screenshot out.png` to render one frame to a png and exit,
	or `custom_shapes frames [count]` to dump a numbered png sequence.
*/

#include <cute.h>
#include <stdio.h>
using namespace Cute;

static bool s_has_custom;
static CF_CustomShape s_star;
static CF_CustomShape s_moon;

// Draws a five-pointed star: the registered sdf when available, otherwise a CSG group
// unioning five point-triangles with the inner pentagon -- one shape either way.
static void s_draw_star(v2 c, float r, float sharp, float stroke)
{
	if (s_has_custom) {
		float params[4] = { c.x, c.y, r, sharp };
		CF_Aabb bounds = make_aabb(V2(c.x - r, c.y - r), V2(c.x + r, c.y + r));
		if (stroke > 0) cf_draw_custom_shape(s_star, bounds, stroke, params, 4);
		else cf_draw_custom_shape_fill(s_star, bounds, params, 4);
		return;
	}
	v2 outer[5], inner[5];
	for (int i = 0; i < 5; ++i) {
		float a = CF_PI * 0.5f + (float)i * (2.0f * CF_PI / 5.0f);
		outer[i] = c + V2(cosf(a), sinf(a)) * r;
		float b = a + CF_PI / 5.0f;
		inner[i] = c + V2(cosf(b), sinf(b)) * (r * 0.45f);
	}
	draw_shape_group_begin();
	cf_draw_polygon_fill(inner, 5, 0);
	for (int i = 0; i < 5; ++i) {
		// Extend each tip triangle past its inner verts along its own silhouette
		// lines: exactly-touching operands would leave zero-distance seams that a
		// composite outline traces, so overlaps must be strict.
		v2 a = inner[i], b = inner[(i + 4) % 5];
		v2 ea = a + (a - outer[i]) * 0.35f;
		v2 eb = b + (b - outer[i]) * 0.35f;
		cf_draw_tri_fill(outer[i], ea, eb, 0);
	}
	if (stroke > 0) draw_shape_group_end_stroked(stroke);
	else draw_shape_group_end();
}

// Draws a crescent moon: the registered sdf when available, otherwise the same CSG
// subtract written as a shape group.
static void s_draw_moon(v2 c, float r, v2 bite_offset, float bite_r)
{
	if (s_has_custom) {
		float params[8] = { c.x, c.y, r, 0, bite_offset.x, bite_offset.y, bite_r, 0 };
		cf_draw_custom_shape_fill(s_moon, make_aabb(V2(c.x - r, c.y - r), V2(c.x + r, c.y + r)), params, 8);
		return;
	}
	draw_shape_group_begin();
	cf_draw_circle_fill2(c, r);
	draw_shape_group_op(CF_SHAPE_OP_SUBTRACT, 0);
	cf_draw_circle_fill2(c + bite_offset, bite_r);
	draw_shape_group_end();
}

int main(int argc, char* argv[])
{
	bool screenshot = argc > 1 && CF_STRCMP(argv[1], "screenshot") == 0;
	const char* screenshot_path = argc > 2 ? argv[2] : "custom_shapes.png";
	// `custom_shapes frames [count]` dumps a numbered png sequence (for video/gif assembly).
	bool frames_mode = argc > 1 && CF_STRCMP(argv[1], "frames") == 0;
	int frame_count = frames_mode && argc > 2 ? atoi(argv[2]) : 360;
	bool headless = screenshot || frames_mode;
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | (headless ? CF_APP_OPTIONS_HIDDEN_BIT : CF_APP_OPTIONS_RESIZABLE_BIT);
	if (getenv("CF_GLES")) options |= CF_APP_OPTIONS_GFX_OPENGL_BIT; // Handy for eyeballing the GLES3 backend.
	CF_Result result = make_app("Custom SDF Shapes", 0, 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	// A five-pointed star. Registered once at init; the snippet compiles into the
	// renderer's shape pipelines and dispatches per draw call.
	s_star = cf_make_custom_shape(R"(
		// params: a = center, b.x = radius, b.y = point sharpness [2..5]
		float sdf(vec2 p, ShapeParams s)
		{
			p -= s.a;
			float r = s.b.x;
			float an = 3.141593 / 5.0;
			float en = 3.141593 / s.b.y;
			vec2 acs = vec2(cos(an), sin(an));
			vec2 ecs = vec2(cos(en), sin(en));
			float bn = mod(atan(p.x, p.y), 2.0 * an) - an;
			p = length(p) * vec2(cos(bn), abs(sin(bn)));
			p -= r * acs;
			p += ecs * clamp(-dot(p, ecs), 0.0, r * acs.y / ecs.y);
			return length(p) * sign(p.x);
		}
	)");

	// A crescent moon: one circle bitten out of another with a CSG subtract.
	s_moon = cf_make_custom_shape(R"(
		// params: a = center, b.x = radius, c = bite offset, d.x = bite radius
		float sdf(vec2 p, ShapeParams s)
		{
			float body = length(p - s.a) - s.b.x;
			float bite = length(p - s.a - s.c) - s.d.x;
			return max(body, -bite);
		}
	)");

	// Registration fails without runtime shader compilation (web/bytecode builds); the
	// draw helpers above fall back to CSG shape groups there. CF_FORCE_FALLBACK=1
	// exercises the fallback on desktop.
	s_has_custom = s_star.id && s_moon.id && !getenv("CF_FORCE_FALLBACK");

	// A gradient via the regular draw-shader stub: the sdf supplies coverage in color.a,
	// the stub tints it. Created AFTER the custom shapes register (draw shaders bake in
	// the custom shape set that exists when they compile).
	CF_Shader gradient_shd = { 0 };
#ifdef CF_RUNTIME_SHADER_COMPILATION
	gradient_shd = cf_make_draw_shader_from_source(R"(
		vec4 shader(vec4 color, ShaderParams params)
		{
			float t = clamp(params.attributes.x + params.view_pos.y * params.attributes.y, 0.0, 1.0);
			vec3 g = mix(vec3(1.00, 0.28, 0.43), vec3(1.00, 0.85, 0.40), t);
			return vec4(g, 1.0) * color.a;
		}
	)");
#endif

	float t = screenshot ? 2.2f : 0;
	int frame_index = 0;
	do {
		app_update();
		t += frames_mode ? 1.0f / 60.0f : CF_DELTA_TIME;

		// Night sky backdrop.
		draw_push_color(make_color(0x10142b));
		draw_box_fill(make_aabb(V2(-320, -240), V2(320, 240)));
		draw_pop_color();

		// Twinkling pips.
		CF_Rnd rnd = rnd_seed(7);
		for (int i = 0; i < 24; ++i) {
			float x = rnd_range(rnd, -300.0f, 300.0f);
			float y = rnd_range(rnd, -220.0f, 220.0f);
			float r = rnd_range(rnd, 3.0f, 9.0f) * (0.8f + 0.2f * sinf(t * 2.0f + (float)i));
			draw_push_color(make_color(0.9f, 0.9f, 1.0f, rnd_range(rnd, 0.3f, 0.9f)));
			s_draw_star(V2(x, y), r, 2.2f, 0);
			draw_pop_color();
		}

		// The moon, upper right, its bite gently waxing and waning.
		draw_push_color(make_color(0xf6f1d5));
		s_draw_moon(V2(170, 120), 70, V2(-32 + sinf(t * 0.6f) * 12.0f, 18), 62);
		draw_pop_color();

		// Boolean ops on plain draw calls: an animated blob -- three circles smoothly
		// unioned, with a fourth orbiting circle carved out -- rendered as ONE shape,
		// then outlined as one continuous stroke.
		v2 blob = V2(170, -130);
		v2 b0 = blob + V2(cosf(t) * 24.0f, sinf(t * 1.3f) * 16.0f);
		v2 b1 = blob + V2(cosf(t * 0.7f + 2.1f) * 30.0f, sinf(t * 0.9f) * 20.0f);
		v2 b2 = blob + V2(cosf(t * 1.6f + 4.0f) * 26.0f, sinf(t * 0.5f + 1.0f) * 22.0f);
		v2 bite = blob + V2(cosf(-t * 1.2f) * 34.0f, sinf(-t * 0.8f) * 24.0f);
		for (int pass = 0; pass < 2; ++pass) {
			draw_push_color(pass == 0 ? make_color(0x06d6a0) : make_color(0xf6f1d5));
			draw_shape_group_begin();
			cf_draw_circle_fill2(b0, 26);
			draw_shape_group_op(CF_SHAPE_OP_UNION, 30.0f);
			cf_draw_circle_fill2(b1, 20);
			cf_draw_circle_fill2(b2, 16);
			draw_shape_group_op(CF_SHAPE_OP_SUBTRACT, 10.0f);
			cf_draw_circle_fill2(bite, 10);
			if (pass == 0) draw_shape_group_end();
			else draw_shape_group_end_stroked(2.5f);
			draw_pop_color();
		}

		// The hero star: gradient-filled (when a draw shader is available) with a
		// stroked outline, gently spinning.
		draw_push();
		draw_translate(-110, -50);
		draw_rotate(t * 0.3f);
		if (gradient_shd.id) {
			draw_push_shader(gradient_shd);
			draw_push_vertex_attributes(0.5f, 1.0f / 220.0f, 0, 0);
			draw_push_color(color_white());
			s_draw_star(V2(0, 0), 110, 3.2f, 0);
			draw_pop_color();
			draw_pop_vertex_attributes();
			draw_pop_shader();
		} else {
			draw_push_color(make_color(0xffd166));
			s_draw_star(V2(0, 0), 110, 3.2f, 0);
			draw_pop_color();
		}
		draw_push_color(make_color(0xef476f));
		s_draw_star(V2(0, 0), 110, 3.2f, 5.0f);
		draw_pop_color();
		draw_pop();

		draw_push_color(color_white());
		push_font_size(24);
		draw_text(s_has_custom ? "custom SDF shapes" : "CSG shape groups (no runtime shaders)", V2(-110, -190), -1);
		pop_font_size();
		draw_pop_color();

		if (headless) {
			CF_Canvas canvas = make_canvas(canvas_defaults(640, 480));
			render_to(canvas, true);
			app_draw_onto_screen(false);
			CF_Readback rb = cf_canvas_readback(canvas);
			while (!cf_readback_ready(rb)) {}
			CF_Image img;
			img.w = 640;
			img.h = 480;
			img.pix = (CF_Pixel*)cf_alloc(640 * 480 * sizeof(CF_Pixel));
			cf_readback_data(rb, img.pix, 640 * 480 * (int)sizeof(CF_Pixel));
			cf_destroy_readback(rb);
			cf_destroy_canvas(canvas);
			fs_set_write_directory(fs_get_base_directory());
			char path[256];
			if (frames_mode) {
				cf_fs_create_directory("/frames");
				snprintf(path, sizeof(path), "frames/frame_%04d.png", frame_index);
			} else {
				snprintf(path, sizeof(path), "%s", screenshot_path);
			}
			cf_image_save_png(path, &img);
			cf_free(img.pix);
			frame_index++;
			if (!frames_mode || frame_index >= frame_count) break;
			continue;
		}

		app_draw_onto_screen();
	} while (app_is_running());

	destroy_app();
	return 0;
}
