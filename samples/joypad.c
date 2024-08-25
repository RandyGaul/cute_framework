#include <cute.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	cf_make_app("Joypad", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);

	CF_Joypad joypad = cf_joypad_open(0);

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_A)) {
			printf("Pressed A\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_B)) {
			printf("Pressed B\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_X)) {
			printf("Pressed X\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_Y)) {
			printf("Pressed Y\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_BACK)) {
			printf("Pressed BACK\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_GUIDE)) {
			printf("Pressed GUIDE\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_START)) {
			printf("Pressed START\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_LEFTSTICK)) {
			printf("Pressed LEFTSTICK\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_RIGHTSTICK)) {
			printf("Pressed RIGHTSTICK\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_LEFTSHOULDER)) {
			printf("Pressed LEFTSHOULDER\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_RIGHTSHOULDER)) {
			printf("Pressed RIGHTSHOULDER\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_DPAD_UP)) {
			printf("Pressed DPAD_UP\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_DPAD_DOWN)) {
			printf("Pressed DPAD_DOWN\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_DPAD_LEFT)) {
			printf("Pressed DPAD_LEFT\n");
		}

		if (cf_joypad_button_just_pressed(joypad, CF_JOYPAD_BUTTON_DPAD_RIGHT)) {
			printf("Pressed DPAD_RIGHT\n");
		}

		char* s = NULL;
		float x = -60.0f;
		float y = 35.0f;
		sfmt(s, "AXIS_LEFTX %d\n", cf_joypad_axis(joypad, CF_JOYPAD_AXIS_LEFTX));
		cf_draw_text(s, cf_v2(x,y), -1);
		sfmt(s, "AXIS_LEFTY %d\n", cf_joypad_axis(joypad, CF_JOYPAD_AXIS_LEFTY));
		cf_draw_text(s, cf_v2(x,y-15), -1);
		sfmt(s, "AXIS_RIGHTX %d\n", cf_joypad_axis(joypad, CF_JOYPAD_AXIS_RIGHTX));
		cf_draw_text(s, cf_v2(x,y-30), -1);
		sfmt(s, "AXIS_RIGHTY %d\n", cf_joypad_axis(joypad, CF_JOYPAD_AXIS_RIGHTY));
		cf_draw_text(s, cf_v2(x,y-45), -1);
		sfmt(s, "AXIS_TRIGGERLEFT %d\n", cf_joypad_axis(joypad, CF_JOYPAD_AXIS_TRIGGERLEFT));
		cf_draw_text(s, cf_v2(x,y-60), -1);
		sfmt(s, "AXIS_TRIGGERRIGHT %d\n", cf_joypad_axis(joypad, CF_JOYPAD_AXIS_TRIGGERRIGHT));
		cf_draw_text(s, cf_v2(x,y-75), -1);

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();

	return 0;
}
