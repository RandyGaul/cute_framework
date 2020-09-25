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

#include <components/lamp.h>

#include <systems/player_system.h>
#include <systems/light_system.h>

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
		ImGui::Begin("Dev Tool", &open);

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
			if (ImGui::IsWindowAppearing()) {
				sprintf(buf, "%s", world->level_name);
			}
			if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
				ImGui::SetKeyboardFocusHere(0);
			}
			ImGui::InputText("Level Name", buf, 1024, ImGuiInputTextFlags_AutoSelectAll);
			if (ImGui::Button("OK", ImVec2(120,0)) || key_was_pressed(app, KEY_RETURN)) {
				ImGui::CloseCurrentPopup();
				if (buf[0]) {
					reload_level(buf);
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
			if (ImGui::IsWindowAppearing()) {
				sprintf(buf, "%s", world->level_name);
			}
			if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
				ImGui::SetKeyboardFocusHere(0);
			}
			ImGui::InputText("Level Name", buf, 1024, ImGuiInputTextFlags_AutoSelectAll);
			if (ImGui::Button("OK", ImVec2(120,0)) || key_was_pressed(app, KEY_RETURN)) {
				ImGui::CloseCurrentPopup();
				if (buf[0]) {
					saved = true;
					file_t* fp = file_system_open_file_for_write(buf);
					for (int i = 0; i < World::LEVEL_H; ++i) {
						for (int j = 0; j < World::LEVEL_W; ++j) {
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
		if (ImGui::Checkbox("Eraser", &erase) || (key_was_pressed(app, KEY_E) && !ImGui::IsPopupOpen("Open") && !ImGui::IsPopupOpen("Save As"))) {
			selected = -1;
			erase = true;
		}
		ImGui::SameLine();
		ImGui::TextUnformatted("(E)");
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
			if (((i + 1) % 3) != 0 && i != schema_previews.count() - 1) {
				ImGui::SameLine();
			}
		}
		ImGui::Separator();
		ImGui::Text("Moves: %d", world->moves);
		ImGui::Text("Oil: %d out of %d", LAMP->oil_count, LAMP->oil_capacity);
		if (ImGui::Button("Add 5 Oil")) {
			LAMP->add_oil(5);
		}
		if (ImGui::Button("Subtract 5 Oil")) {
			LAMP->add_oil(-5);
		}
		static color_t tint = make_color(0.5f, 0.5f, 0.5f, 1.0f);
		ImGui::ColorPicker4("Tint", (float*)&tint);
		batch_set_tint_color(batch, tint);
		ImGui::End();

		if (erase) {
			// Draw erase selection.
			static sprite_t sprite;
			static bool erase_sprite_loaded = false;
			if (!erase_sprite_loaded) {
				aseprite_cache_load(cache, "editor_select.aseprite", &sprite);
			}

			transform_t tx = make_transform();
			v2 mpw = mouse_pos_in_world_space(app);
			int mx, my;
			world2tile(mpw, &mx, &my);
			tx.p = tile2world(mx, my);
			batch_push(batch, sprite.batch_sprite(tx));
			batch_flush(batch);

			// Delete entities on left-click.
			if (!ImGui::IsAnyWindowHovered() && !ImGui::IsAnyWindowFocused() && mouse_is_down(app, MOUSE_BUTTON_LEFT)) {
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
			tx.p = tile2world(mx, my);
			batch_push(batch, preview.sprite.batch_sprite(tx));
			batch_flush(batch);

			// Create entities on left-click.
			if (!ImGui::IsAnyWindowHovered() && !ImGui::IsAnyWindowFocused() && mouse_is_down(app, MOUSE_BUTTON_LEFT)) {
				make_entity_at(selected, mx, my);
			}
		}
	}
}

void draw_text(const char* text, int x, int y)
{
	const font_t* font = font_get_default(app);
	float w = (float)font_text_width(font, text);
	float h = (float)font_text_height(font, text);
	font_push_verts(app, font, text, x + -w / 2, y + h / 2, 0);
}

bool button_text(const char* text, int x, int y)
{
	struct state_t
	{
		bool clicked = false;
	};

	static dictionary<const char*, state_t> s_state;
	state_t* s = s_state.find(text);
	if (!s) {
		state_t state;
		s = s_state.insert(text, state);
	}

	const font_t* font = font_get_default(app);
	float w = (float)font_text_width(font, text);
	float h = (float)font_text_height(font, text);
	font_push_verts(app, font, text, x + -w / 2, y + h / 2, 0);
	int height_diff = font_line_height(font) - font_height(font) - 1;
	aabb_t bb = make_aabb(v2((float)x, (float)y - height_diff), w, h);
	bb = expand(bb, v2(2, 2));

	bool result = false;

	v2 mp = mouse_pos_in_world_space(app);
	if (contains(bb, mp)) {
		if (!s->clicked && mouse_was_pressed(app, MOUSE_BUTTON_LEFT)) {
			s->clicked = true;
		} else if (s->clicked && mouse_was_released(app, MOUSE_BUTTON_LEFT)) {
			s->clicked = false;
			result = true;
		}
		batch_quad_line(batch, bb, 1, color_white());
		if (mouse_is_down(app, MOUSE_BUTTON_LEFT)) {
			batch_quad(batch, expand(bb, -v2(2, 2)), color_white());
		}
	}

	return result;
}

void do_lose_screen_stuff(float dt)
{
	static coroutine_t s_co;
	coroutine_t* co = &s_co;
	static sprite_t player;
	static bool show_text = false;
	static bool show_girl = true;
	float t = 0;

	COROUTINE_START(co);
	Darkness::lerp_to(0);
	aseprite_cache_load(cache, "girl.aseprite", &player);
	player.play("idle");

	if (Darkness::radius != 0) {
		COROUTINE_EXIT(co);
	}

	destroy_all_entities();

	COROUTINE_SEQUENCE_POINT(co);
	t = co->elapsed / 2.0f;
	show_girl = mod(t, 0.25f) < 0.25f * 0.5f;
	COROUTINE_WAIT(co, 2.0f, dt);

	COROUTINE_CASE(co, idle);
	show_text = true;
	t = 1.0f;
	COROUTINE_YIELD(co);
	goto idle;

	COROUTINE_END(co);

	draw_text("YOU DIED", 0, 0);
	if (t == 1) {
		if (button_text("Retry?", -30, -20)) {
			world->lose_screen = false;
			reload_level(world->level_name);
			Darkness::reset();
			s_co = { 0 };
			show_text = false;
			show_girl = true;
			batch_no_tint(batch);
		}
		if (button_text("Nah.", 40, -20)) {
			printf("nah\n");
		}
	}

	color_t color = color_white();
	color.a = ease_in_sin(t * t * t);
	font_draw(app, font_get_default(app), matrix_ortho_2d(320, 240, 0, 0), color);
	batch_flush(batch);

	if (show_girl) {
		batch_outlines(batch, true);
		batch_outlines_color(batch, make_color(0.8f, 0.1f, 0.1f));
		batch_set_tint_color(batch, color_black());
		player.draw(batch, player_last_tx());
		batch_flush(batch);
		batch_outlines(batch, false);
		batch_outlines_color(batch, color_white());
	}
}

int main(int argc, const char** argv)
{
	init_world();
	select_level(0);

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);
		if (world->load_level_dirty_flag) select_level(world->level_index);
		app_update_systems(app, dt);
		if (world->lose_screen) {
			do_lose_screen_stuff(dt);
		}
		do_imgui_stuff(app, dt);
		app_present(app);
	}

	app_destroy(app);

	return 0;
}
