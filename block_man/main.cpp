/*
	Cute Framework
	Copyright (C) 2020 Randy Gaul https://randygaul.net

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

#include <cute.h>
using namespace cute;

#include <stdio.h>
#include <imgui/imgui.h>

#include <cute/cute_coroutine.h>

#include <world.h>
#include <serialize.h>

#if 0
void LoadLevelIntoEditor(char* buf)
{
	int index = 0;
	for (int i = 0; i < level.data.count(); ++i)
	{
		for (int j = 0; j < level.data[i].count(); ++j)
		{
			CUTE_ASSERT(index < 1024 * 10);
			char c = level.data[i][j];
			buf[index++] = c;
		}
		buf[index++] = '\n';
		CUTE_ASSERT(index < 1024 * 10);
	}
	buf[index++] = 0;
}

void DoImguiStuff(app_t* app, float dt)
{
	static bool open = true;
	if (key_was_pressed(app, KEY_E)) {
		open = true;
	}
	if (open) {
		ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_FirstUseEver);
		ImGui::Begin("Level Editor", &open);
		static char editor_buf[1024 * 10];
		if (!loaded_level_into_editor) {
			LoadLevelIntoEditor(editor_buf);
			loaded_level_into_editor = true;
		}
		int flags = ImGuiInputTextFlags_AllowTabInput;
		ImGui::Text("Level %d", level_index + 1);
		ImGui::InputTextMultiline("", editor_buf, 1024 * 10, ImVec2(0, 200), ImGuiInputTextFlags_AllowTabInput);
		if (ImGui::Button("Sync Editor Text")) {
			LoadLevelIntoEditor(editor_buf);
		}
		if (ImGui::Button("Commit")) {
			array<char> buf;
			levels[level_index].clear();
			int i = 0;
			char c;
			while ((c = editor_buf[i++])) {
				if (c == '\n') {
					buf.add(0);
					levels[level_index].add(buf.data());
					buf.clear();
				} else {
					buf.add(c);
				}
			}
			LoadLevel(levels[level_index]);
		}
		static bool copied = false;
		if (ImGui::Button("Copy to Clipboard")) {
			array<char> buf;
			int index = 0;
			char c;
			buf.add('\t');
			buf.add('{');
			buf.add('\n');
			buf.add('\t');
			buf.add('\t');
			buf.add('\"');
			while ((c = editor_buf[index++])) {
				if (c == '\n') {
					buf.add('\"');
					buf.add(',');
					buf.add('\n');
					buf.add('\t');
					buf.add('\t');
					buf.add('\"');
				} else {
					buf.add(c);
				}
			}
			buf.pop();
			buf.pop();
			buf.add('}');
			buf.add(',');
			buf.add('\n');
			buf.add(0);
			clipboard_set(buf.data());
			copied = true;
		}
		if (copied) {
			static coroutine_t s_co;
			coroutine_t* co = &s_co;

			COROUTINE_START(co);
			float delay = 0.5f;
			ImGui::SetNextWindowBgAlpha(1.0f - co->elapsed / delay);
			ImGui::BeginTooltip();
			ImGui::Text("Copied!");
			ImGui::EndTooltip();
			COROUTINE_WAIT(co, delay, dt);
			copied = false;
			COROUTINE_END(co);
		}
		ImGui::End();
	}
}
#endif

int main(int argc, const char** argv)
{
	init_world();
	load_level(0);

	matrix_t mvp = matrix_ortho_2d(320, 240, 0, -100);
	const font_t* font = font_get_default(app);
	float w = (float)font_text_width(font, "0000");
	float h = (float)font_text_height(font, "0000");

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);

		//if (key_was_pressed(app, KEY_Q)) {
		//	// Cheat. For testing.
		//	level_index = (level_index + 1) % levels.count();
		//	LoadLevel(levels[level_index]);
		//}

		//DrawBackgroundBricks();

		app_update_systems(app, dt);

		//char buffer[4];
		//itoa(hero.moves, buffer, 10);
		//font_push_verts(app, font, buffer, -w / 2, h / 2, 0);
		//font_draw(app, font, mvp);
		//
		//DoImguiStuff(app, dt);

		app_present(app);
	}

	app_destroy(app);

	return 0;
}
