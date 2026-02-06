// Input Bindings Example -- Platformer Character
//
// Shows how to wire up keyboard + joypad inputs using Cute Framework's
// binding system. Inspired by a real platformer's player controller.

#include <cute.h>

// Tuning.
#define PRESS_BUFFER 0.1f   // Seconds to buffer press/release for forgiving input.
#define DEADZONE     0.25f  // Analog stick deadzone.
#define DASH_SPEED   350.0f
#define DASH_TIME    0.15f

int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app("Binding Example", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (cf_is_error(result)) return -1;

	// Movement stick: WASD + arrow keys + d-pad + left analog stick.
	CF_StickBinding move = cf_make_stick_binding(0);
	cf_stick_binding_add_wasd(move);
	cf_stick_binding_add_arrow_keys(move);
	cf_stick_binding_add_dpad(move);
	cf_stick_binding_add_left_stick(move, DEADZONE);

	// Jump button: spacebar + Z key + joypad A.
	CF_ButtonBinding jump = cf_make_button_binding(0, PRESS_BUFFER);
	cf_button_binding_add_key(jump, CF_KEY_SPACE);
	cf_button_binding_add_key(jump, CF_KEY_Z);
	cf_button_binding_add_joypad_button(jump, CF_JOYPAD_BUTTON_A);

	// Dash button: left shift + X key + joypad B.
	CF_ButtonBinding dash = cf_make_button_binding(0, PRESS_BUFFER);
	cf_button_binding_add_key(dash, CF_KEY_LSHIFT);
	cf_button_binding_add_key(dash, CF_KEY_X);
	cf_button_binding_add_joypad_button(dash, CF_JOYPAD_BUTTON_B);

	float x = 0, y = 0;
	float vx = 0, vy = 0;
	bool on_ground = true;
	float ground_y = -160.0f;
	float dash_timer = 0;
	float dash_dir = 1.0f;

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		float dt = CF_DELTA_TIME;

		// -- Dash --
		if (cf_binding_pressed(dash) && dash_timer <= 0) {
			cf_binding_consume_press(dash);
			dash_dir = cf_binding_sign(move).x;
			if (dash_dir == 0) dash_dir = 1.0f;
			dash_timer = DASH_TIME;
		}

		dash_timer -= dt;
		if (dash_timer < 0) dash_timer = 0;

		if (dash_timer > 0) {
			// Dash overrides normal movement.
			vx = DASH_SPEED * dash_dir;
		} else {
			// -- Normal movement --
			CF_V2 input = cf_binding_value(move);
			vx = input.x * 120.0f;
		}

		// -- Jump (buffered) --
		if (on_ground && cf_binding_pressed(jump)) {
			cf_binding_consume_press(jump);
			vy = 250.0f;
			on_ground = false;
		}

		// Variable jump height: cut upward velocity when button released.
		if (!on_ground && vy > 0 && !cf_button_binding_down(jump)) {
			vy *= 0.5f;
		}

		// Gravity + integration.
		if (dash_timer <= 0) {
			vy -= 800.0f * dt;
		}
		x += vx * dt;
		y += vy * dt;
		if (y <= ground_y) { y = ground_y; vy = 0; on_ground = true; }

		// Draw the player.
		cf_draw_push_color(cf_color_white());
		cf_draw_box_fill(cf_make_aabb(cf_v2(x - 8, y), cf_v2(x + 8, y + 16)), 0);
		cf_draw_pop_color();

		// Draw the ground line.
		cf_draw_push_color(cf_color_grey());
		cf_draw_line(cf_v2(-320, ground_y), cf_v2(320, ground_y), 0);
		cf_draw_pop_color();

		// Draw controls.
		cf_draw_push_color(cf_color_grey());
		float tx = -300.0f;
		float ty = 220.0f;
		float line = 18.0f;
		cf_draw_text("Controls:", cf_v2(tx, ty), -1);
		cf_draw_text("Move:  WASD / Arrow Keys / D-Pad / Left Stick", cf_v2(tx, ty - line), -1);
		cf_draw_text("Jump:  Space / Z / A Button", cf_v2(tx, ty - line*2), -1);
		cf_draw_text("Dash:  LShift / X / B Button", cf_v2(tx, ty - line*3), -1);
		cf_draw_pop_color();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_binding(move);
	cf_destroy_binding(jump);
	cf_destroy_binding(dash);
	cf_destroy_app();
	return 0;
}
