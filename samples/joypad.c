#include <cute.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	cf_make_app("Joypad", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		for (int player_index = 0; player_index < CF_MAX_JOYPADS; ++player_index) {
			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_A)) {
				printf("Player index %d pressed A (applying a rumble)\n", player_index);
				cf_joypad_rumble(player_index, 0xFFFF/2, 0xFFFF/2, 10);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_B)) {
				printf("Player index %d pressed B\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_X)) {
				printf("Player index %d pressed X\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_Y)) {
				printf("Player index %d pressed Y\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_BACK)) {
				printf("Player index %d pressed BACK\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_GUIDE)) {
				printf("Player index %d pressed GUIDE\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_START)) {
				printf("Player index %d pressed START\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_LEFTSTICK)) {
				printf("Player index %d pressed LEFTSTICK\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_RIGHTSTICK)) {
				printf("Player index %d pressed RIGHTSTICK\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_LEFTSHOULDER)) {
				printf("Player index %d pressed LEFTSHOULDER\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_RIGHTSHOULDER)) {
				printf("Player index %d pressed RIGHTSHOULDER\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_DPAD_UP)) {
				printf("Player index %d pressed DPAD_UP\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_DPAD_DOWN)) {
				printf("Player index %d pressed DPAD_DOWN\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_DPAD_LEFT)) {
				printf("Player index %d pressed DPAD_LEFT\n", player_index);
			}

			if (cf_joypad_button_just_pressed(player_index, CF_JOYPAD_BUTTON_DPAD_RIGHT)) {
				printf("Player index %d pressed DPAD_RIGHT\n", player_index);
			}

			char* s = NULL;
			float x = -60.0f;
			float y = 35.0f;
			sfmt(s, "Player index 0 AXIS_LEFTX %d\n", cf_joypad_axis(0, CF_JOYPAD_AXIS_LEFTX));
			cf_draw_text(s, cf_v2(x,y), -1);
			sfmt(s, "Player index 0 AXIS_LEFTY %d\n", cf_joypad_axis(0, CF_JOYPAD_AXIS_LEFTY));
			cf_draw_text(s, cf_v2(x,y-15), -1);
			sfmt(s, "Player index 0 AXIS_RIGHTX %d\n", cf_joypad_axis(0, CF_JOYPAD_AXIS_RIGHTX));
			cf_draw_text(s, cf_v2(x,y-30), -1);
			sfmt(s, "Player index 0 AXIS_RIGHTY %d\n", cf_joypad_axis(0, CF_JOYPAD_AXIS_RIGHTY));
			cf_draw_text(s, cf_v2(x,y-45), -1);
			sfmt(s, "Player index 0 AXIS_TRIGGERLEFT %d\n", cf_joypad_axis(0, CF_JOYPAD_AXIS_TRIGGERLEFT));
			cf_draw_text(s, cf_v2(x,y-60), -1);
			sfmt(s, "Player index 0 AXIS_TRIGGERRIGHT %d\n", cf_joypad_axis(0, CF_JOYPAD_AXIS_TRIGGERRIGHT));
			cf_draw_text(s, cf_v2(x,y-75), -1);
		}

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();

	return 0;
}
