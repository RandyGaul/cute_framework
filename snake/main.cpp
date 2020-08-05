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
#include <imgui/imgui.h>
#include <cute.h>
using namespace cute;

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_RESIZABLE;
	app_t* app = app_make("Cute ImGui", 0, 0, 640, 480, options);

	file_system_mount(file_system_get_base_dir(), "", 1);

	gfx_init(app);
	spritebatch_t* sb = sprite_batch_easy_make(app, "data");

	ImGuiContext* imgui_context = app_init_imgui(app);
	if (!imgui_context) {
		printf("Unable to initialize ImGui.\n");
		return -1;
	}
	ImGui::SetCurrentContext(imgui_context);

	sprite_t cloud;
	error_t err = sprite_batch_easy_sprite(sb, "data/cloud.png", &cloud);
	if (err.is_error()) {
		printf("%s\n", err.details);
		return -1;
	}
	float t = 0;

	threadpool_t* pool = threadpool_create(3);
	threadpool_destroy(pool);

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);

		if (key_is_down(app, KEY_SPACE)) {
			t += dt * 1.5f;
		}
		cloud.transform.p.x = cos(t) * 20.0f;
		cloud.transform.p.y = sin(t) * 20.0f;
		sprite_batch_push(sb, cloud);

		sprite_batch_flush(sb);

		static bool hello_open = true;
		if (hello_open) {
			ImGui::Begin("Hello", &hello_open);
			static bool push_me;
			ImGui::Checkbox("Push Me", &push_me);
			if (push_me) {
				ImGui::Separator();
				ImGui::Indent();
				ImGui::Text("This is a Dear ImGui Window!");
			}
			ImGui::End();
		}

		gfx_flush(app);
	}

	app_destroy(app);

	return 0;
}
