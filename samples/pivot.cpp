#include <cute.h>
#include <imgui/imgui.h>

using namespace Cute;

void mount_content_directory_as(const char* dir)
{
	CF_Path path = fs_get_base_directory();
	path.normalize();
	path += "/pivot_data";
	fs_mount(path.c_str(), dir);
}

int main(int argc, char* argv[])
{
	make_app("Pivot", 0, 0, 0, 640, 480, APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	mount_content_directory_as("/");

	// This sprite has a slice on each frame with `pivot` checked in aseprite.
	// These pivots offset the sprite each frame when drawn.
	Sprite animated_box_with_pivots = cf_make_sprite("/pivot.ase");

	while (app_is_running()) {
		app_update();
		draw_scale(4,4);

		animated_box_with_pivots.update();
		animated_box_with_pivots.draw();
		CF_Aabb pivot = cf_sprite_get_slice(&animated_box_with_pivots, "pivot");
		draw_box(pivot);

		app_draw_onto_screen(true);
	}

	destroy_app();

	return 0;
}
