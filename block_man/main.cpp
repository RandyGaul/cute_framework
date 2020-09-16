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

void do_imgui_stuff(app_t* app, float dt)
{
	static bool open = true;
	static int selected = -1;
	static bool erase = false;
	if (key_was_pressed(app, KEY_E)) {
		open = true;
	}

	if (!open) {
		selected = -1;
	}

	if (open) {
		// Editor UI.
		ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_FirstUseEver);
		ImGui::Begin("Level Editor", &open);

		if (mouse_was_pressed(app, MOUSE_BUTTON_RIGHT)) {
			selected = -1;
			erase = false;
		}
		static bool saved = false;

		bool ctrl_s = key_mod_bit_flags(app) & CUTE_KEY_MOD_CTRL && key_was_pressed(app, KEY_S);
		if (ImGui::Button("Save As") || ctrl_s) {
			ImGui::OpenPopup("Save As");
		}
		ImGui::SameLine();
		ImGui::TextUnformatted("(Ctrl+S)");

		bool ctrl_o = key_mod_bit_flags(app) & CUTE_KEY_MOD_CTRL && key_was_pressed(app, KEY_O);
		if (ImGui::Button("Open") || ctrl_o) {
			ImGui::OpenPopup("Open");
		}
		ImGui::SameLine();
		ImGui::TextUnformatted("(Ctrl+O)");

		ImGui::Separator();

		if (ImGui::BeginPopupModal("Open")) {
			static char buf[1024];
			if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
				ImGui::SetKeyboardFocusHere(0);
			}
			ImGui::InputText("Level Name", buf, 1024, ImGuiInputTextFlags_AutoSelectAll);
			if (ImGui::Button("OK", ImVec2(120,0)) || key_was_pressed(app, KEY_RETURN)) {
				ImGui::CloseCurrentPopup();
				if (buf[0]) {
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120,0)) || key_was_pressed(app, KEY_ESCAPE)) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Save As")) {
			static char buf[1024];
			if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
				ImGui::SetKeyboardFocusHere(0);
			}
			ImGui::InputText("Level Name", buf, 1024, ImGuiInputTextFlags_AutoSelectAll);
			if (ImGui::Button("OK", ImVec2(120,0)) || key_was_pressed(app, KEY_RETURN)) {
				ImGui::CloseCurrentPopup();
				if (buf[0]) {
					saved = true;
					file_t* fp = file_system_open_file_for_write(buf);
					for (int i = 0; i < world->board.h; ++i) {
						for (int j = 0; j < world->board.w; ++j) {
							char c = world->board.data[i][j].code;
							file_system_write(fp, &c, 1);
						}
						char c = '\n';
						file_system_write(fp, &c, 1);
					}
					file_system_close(fp);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120,0)) || key_was_pressed(app, KEY_ESCAPE)) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (saved) {
			// Fade out "Saved!" tooltip.
			static coroutine_t s_co;
			coroutine_t* co = &s_co;

			COROUTINE_START(co);
			const float delay = 1.0f;
			float t = co->elapsed / delay;
			ImGui::SetNextWindowBgAlpha(ease_out_sin(1.0f - t));
			ImGui::BeginTooltip();
			ImGui::Text("Saved!");
			ImGui::EndTooltip();
			COROUTINE_WAIT(co, delay, dt);
			saved = false;
			COROUTINE_END(co);
		}
		if (ImGui::Checkbox("Erase", &erase)) {
			selected = -1;
		}
		for (int i = 0; i < schema_previews.count(); ++i) {
			float w = schema_previews[i].w;
			float h = schema_previews[i].h;
			ImVec2 uv0 = ImVec2(0, 0);
			ImVec2 uv1 = ImVec2(1, 1);
			int frame_padding = -1;
			ImVec4 bg_color = i == selected ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive) : ImVec4(0, 0, 0, 0);
			if (ImGui::ImageButton((ImTextureID)schema_previews[i].tex, ImVec2(w, h), uv0, uv1, frame_padding, bg_color)) {
				selected = i;
				erase = false;
			}
			if (((i + 1) % 3) != 0) {
				ImGui::SameLine();
			}
		}
		ImGui::End();

		if (erase) {
			// Draw erase selection.
			static sprite_t sprite;
			static bool erase_sprite_loaded = false;
			if (!erase_sprite_loaded) {
				aseprite_cache_load(cache, "data/editor_select.aseprite", &sprite);
			}

			transform_t tx = make_transform();
			v2 mpw = mouse_pos_in_world_space(app);
			int mx, my;
			world2tile(mpw, &mx, &my);
			tx.p = tile2world(mx, my) + sprite.local_offset;
			batch_push(batch, sprite.quad(tx));
			batch_flush(batch);

			// Delete entities on left-click.
			if (!ImGui::IsAnyWindowHovered() && mouse_was_pressed(app, MOUSE_BUTTON_LEFT)) {
				v2 mpw = mouse_pos_in_world_space(app);
				int mx, my;
				world2tile(mpw, &mx, &my);
				destroy_entity_at(mx, my);
			}
		}


		if (selected != -1) {
			schema_preview_t preview = schema_previews[selected];

			// Draw selected entity on mouse.
			transform_t tx = make_transform();
			v2 mpw = mouse_pos_in_world_space(app);
			int mx, my;
			world2tile(mpw, &mx, &my);
			tx.p = tile2world(mx, my) + preview.sprite.local_offset;
			batch_push(batch, preview.sprite.quad(tx));
			batch_flush(batch);

			// Create entities on left-click.
			if (!ImGui::IsAnyWindowHovered() && mouse_was_pressed(app, MOUSE_BUTTON_LEFT)) {
				make_entity_at(selected, mx, my);
			}
		}
	}
}

int main(int argc, const char** argv)
{
	init_world();
	load_level();

	char write_dir_path[1024];
	sprintf(write_dir_path, "%s%s", file_system_get_base_dir(), "../../block_man/data");
	file_system_set_write_dir(write_dir_path);

	matrix_t mvp = matrix_ortho_2d(320, 240, 0, -100);
	const font_t* font = font_get_default(app);
	float w = (float)font_text_width(font, "0000");
	float h = (float)font_text_height(font, "0000");

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);
		if (world->load_level_dirty_flag) load_level();
		app_update_systems(app, dt);
		do_imgui_stuff(app, dt);
		app_present(app);
	}

	app_destroy(app);

	return 0;
}
