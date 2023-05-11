#include <cute.h>
using namespace Cute;

#include <imgui.h>

int main(int argc, char* argv[])
{
	int w = 640/1;
	int h = 480/1;
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	Result result = make_app("Development Scratch", 0, 0, w, h, options, argv[0]);
	if (is_error(result)) return -1;

	app_init_imgui();

	make_font("sample_data/calibri.ttf", "Calibri");
	push_font("Calibri");

	camera_dimensions((float)w, (float)h);
	int draw_calls = 0;

	Sprite s = cf_make_sprite("test_data/girl.aseprite");
	s.scale.x = 2.0f;
	s.scale.y = 2.0f;
	s.play("spin");

	draw_push_antialias(false);
	Color c = color_white();
	c.a = 0.5f;
	draw_push_color(c);
	cf_clear_color(0, 0, 0, 0);

	float fps = 0;
	bool pause = false;
	float pause_t = 0;

	while (app_is_running()) {
		app_update();

		if (fps == 0) {
			fps = 1.0f / CF_DELTA_TIME;
		} else {
			fps = lerp(fps, 1.0f / CF_DELTA_TIME, 1.0f / 500.0f);
		}
		static float t = 0;

		static v2 v = V2(35,50);
		ImGui::Begin("what");
		ImGui::SliderFloat2("pt", &v.x, -200, 200);
		ImGui::End();

		if (0) {
			draw_push_antialias(true);
			draw_circle_fill(V2(sinf(t*0.25f) * 250.0f,0), (cosf(t) * 0.5f + 0.5f) * 50.0f + 50.0f);
			draw_circle(V2(sinf(t+2.0f) * 50.0f,0), (cosf(t) * 0.5f + 0.5f) * 50.0f + 50.0f, 5);
			draw_line(V2(0,0), V2(cosf(t*0.5f+CF_PI),sinf(t*0.5f+CF_PI))*100.0f,10);
			draw_pop_antialias();

			draw_circle_fill(V2(sinf(t*0.25f) * 250.0f,100), (cosf(t) * 0.5f + 0.5f) * 50.0f + 50.0f);
			draw_circle(V2(20,sinf(t+2.0f) * 50.0f), (cosf(t) * 0.5f + 0.5f) * 50.0f + 50.0f, 5);
			draw_line(V2(0,0), V2(cosf(t*0.5f),sinf(t*0.5f))*100.0f,10);
		}

		if (0) {
			v2 o = V2(sinf(t),cosf(t));
			draw_push_antialias(true);
			draw_quad(cf_make_aabb(V2(-20,-20)+o*25.0f, V2(20,20)+o*25.0f), 5);
			draw_quad_fill(cf_make_aabb(V2(-60,-60)+o*25.0f, V2(-40,-30)+o*25.0f));
			draw_pop_antialias();
			draw_quad(cf_make_aabb(V2(-20,-20)-o*25.0f, V2(20,20)-o*25.0f), 5);
			draw_quad_fill(cf_make_aabb(V2(-60,-60)-o*25.0f, V2(-40,-30)-o*25.0f));
		}

		if (0) {
			v2 box[4];
			Aabb bb = make_aabb(V2(-20,-30), V2(30,50));
			aabb_verts(box, bb);
			M3x2 m = make_rotation(mod(t*0.25f, CF_PI*2));
			for (int i = 0; i < 4; ++i) box[i] = mul(m, box[i]);
			draw_push_antialias(true);
			draw_quad(box[0], box[1], box[2], box[3], 5, 20);
			draw_pop_antialias();
			m = make_translation(-150,0);
			for (int i = 0; i < 4; ++i) box[i] = mul(m, box[i]);
			draw_quad(box[0], box[1], box[2], box[3], 5, 20);

			m = make_translation(0,150);
			for (int i = 0; i < 4; ++i) box[i] = mul(m, box[i]);
			draw_quad_fill(box[0], box[1], box[2], box[3], 20);

			m = make_translation(150,0);
			for (int i = 0; i < 4; ++i) box[i] = mul(m, box[i]);
			draw_push_antialias(true);
			draw_quad_fill(box[0], box[1], box[2], box[3], 20);
			draw_pop_antialias();
		}

		if (0) {
			draw_capsule(V2(-100,100), V2(100,150), 20, 3);
			draw_push_antialias(true);
			draw_capsule(V2(-100,0), V2(100,50), 20, 3);
			draw_pop_antialias();
		}

		if (0) {
			draw_tri_fill(V2(-100,-100), V2(-50,80), V2(120,15));
			draw_push_antialias(true);
			draw_tri_fill(V2(-50,-50-100), V2(-70,30-100), V2(150,25-100));
			draw_pop_antialias();

			draw_push_layer(1);
			s.update();
			s.draw();
			draw_push_color(color_red());
			draw_text("testing <wave>some</wave> text", V2(-100,0));
			draw_pop_color();
			draw_pop_layer();
		}

		if (0) {
			v2 pts[] = {
				V2(0,0),
				V2(100,0),
				V2(150,50),
				V2(150,100),
			};
			Rnd rnd = rnd_seed(0);
			for (int i = 0; i < CF_ARRAY_SIZE(pts); ++i) {
				auto f = [&](float s) { return rnd_next_range(rnd, 1.0f, 3.0f) * s; };
				if (i%2) {
					pts[i] += V2(cosf(t*f(0.5f)*0.5f + f(3)), sinf(t*f(0.5f)*0.5f + f(3))) * f(50);
				} else {
					pts[i] += V2(sinf(t*f(0.5f)*0.5f + f(3)), cosf(t*f(0.5f)*0.5f + f(3))) * f(50);
				}
			}
			draw_push_antialias(true);
			cf_draw_polyline(pts, CF_ARRAY_SIZE(pts), 25, true);
			draw_pop_antialias();

			if (key_just_pressed(KEY_SPACE)) {
				pause = !pause;
				printf("v2 pts[] = {\n");
				printf("\tV2(%ff,%ff),\n", pts[0].x, pts[0].y);
				printf("\tV2(%ff,%ff),\n", pts[1].x, pts[1].y);
				printf("\tV2(%ff,%ff),\n", pts[2].x, pts[2].y);
				printf("\tV2(%ff,%ff),\n", pts[3].x, pts[3].y);
				printf("};\n");
			}
		}

		if (1) {
			v2 pts[] = {
				V2(0,0),
				v,
				V2(0,100),
			};
			draw_push_antialias(true);
			cf_draw_polyline(pts, CF_ARRAY_SIZE(pts), 10, false);
			draw_pop_antialias();
		}

		String s = fps;
		draw_text(s.c_str(), V2(-w/2.0f,-h/2.0f)+V2(2,35));
		s = CF_DELTA_TIME;
		draw_text(s.c_str(), V2(-w/2.0f,-h/2.0f)+V2(2,20));

		if (!pause || key_just_pressed(KEY_F)) {
			t += CF_DELTA_TIME;
		}

		draw_calls = app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
