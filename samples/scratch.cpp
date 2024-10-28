#include <cute.h>
using namespace Cute;

// This isn't really a sample, but a scratch pad for the CF author to experiment.

int main(int argc, char* argv[])
{
	float w = 320, h = 240;
	float sx = 2, sy = 2;
	make_app("Development Scratch", 0, 0, 0, (int)(w * sx), (int)(h * sy), CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	CF_Canvas plain = make_canvas(canvas_defaults((int)w, (int)h));
	cf_clear_color(0,0,0,0);

	while (app_is_running()) {
		app_update();
		draw_push_antialias(false);
		draw_scale(sx, sy);
		draw_rotate(CF_PI * CF_SECONDS / 16.0f);

		draw_push_color(color_red());
		draw_arrow(V2(0, 0), V2(20, 0), 1, 5);

		draw_push_color(color_blue());
		draw_bezier_line(V2(8, -10), V2(10, 15), V2(20, 10), 16, 1);

		draw_push_color(color_yellow());
		draw_box(V2(0, 0), 10, 10);

		draw_push_color(color_green());
		draw_box_rounded(make_aabb(V2(0, 0), 50, 50), 1, 3);

		draw_push_color(color_orange());
		draw_capsule(V2(0, -20), V2(0, 20), 8, 1);

		draw_push_color(color_cyan());
		draw_circle(V2(0, 0), 10);

		draw_push_color(color_magenta());
		draw_line(V2(-35, 28), V2(100, 9), 1);

		Array<v2> points = { V2(0,0) };
		draw_push_color(color_purple());
		draw_polyline(points.data(), points.count());

		points.add(V2(12, 18));
		draw_push_color(color_grey());
		draw_polyline(points.data(), points.count());

		points.add(V2(-27, 35));
		points.add(V2(-40, 5));
		draw_push_color(color_white());
		draw_polyline(points.data(), points.count(), 1, true);

		draw_push_color(cf_make_color_hex(0xDEADBEEF));
		draw_quad(make_aabb(V2(0,0), 37, 42), 1.0f, 4);

		draw_push_color(cf_make_color_hex(0xDADB0D));
		draw_tri(V2(13, 48), V2(27, 16), V2(15, 30), 1.0f, 2.0f);

		render_to(plain, true);
		draw_canvas(plain, 0, 0, w, h);
		render_to(app_get_canvas(), true);

		app_draw_onto_screen();
	}

	destroy_canvas(plain);
	destroy_app();

	return 0;
}
