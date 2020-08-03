/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>

#include <cute_app.h>
#include <cute_timer.h>
#include <cute_input.h>
#include <cute_window.h>
#include <cute_file_system.h>
#include <cute_gfx.h>
#include <cute_font.h>
#include <cute_sprite.h>
#include <cute_array.h>

#define CUTE_PATH_IMPLEMENTATION
#include <cute/cute_path.h>

static const char** s_image_paths_ptr;
cute::array<const char*> s_paths;

static void s_image_paths()
{
	const char** paths = cute::file_system_enumerate_directory("data");
	int i = 0;
	while (paths[i]) {
		char ext[CUTE_PATH_MAX_EXT];
		path_pop_ext(paths[i], NULL, ext);
		if (!strcmp(ext, "png")) {
			s_paths.add(paths[i]);
		}
		++i;
	}
	s_image_paths_ptr = paths;
}

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_RESIZABLE;
	cute::app_t* app = cute::app_make("Cute Snake", 0, 0, 640, 480, options);

	const char* base_dir = cute::file_system_get_base_dir();
	cute::file_system_mount(base_dir, "", 1);

	cute::gfx_init(app);
	cute::gfx_init_upscale(app, 320, 240, cute::GFX_UPSCALE_MAXIMUM_ANY);
	cute::gfx_set_clear_color(app, 0xFF7095A4);
	cute::gfx_set_alpha(app, 1);

	cute::gfx_matrix_t mvp;
	cute::matrix_ortho_2d(&mvp, 320, 240, 0, 0);

	const cute::font_t* font = cute::font_get_default(app);

	cute::sprite_batch_t* sb = cute::sprite_batch_make(app);
	cute::sprite_batch_set_mvp(sb, mvp);

	s_image_paths();
	cute::sprite_batch_enable_disk_LRU_cache(sb, s_paths.data(), s_paths.count(), CUTE_MB * 256);

	while (cute::app_is_running(app)) {
		float dt = cute::calc_dt();
		cute::app_update(app, dt);
		
		cute::font_push_verts(app, font, "Boo! ~", -100, -75, 0);
		cute::font_push_verts(app, font, "o_O", -100, 75, 0);
		cute::font_push_verts(app, font, "Hi there :)\nWhat's your name?", 0, 0, 0);
		cute::font_submit_draw_call(app, font, mvp);

		cute::gfx_flush(app);
	}

	cute::file_system_free_enumerated_directory(s_image_paths_ptr);

	return 0;
}
