#include <cute_app.h>
#include <cute_handle_table.h>
using namespace cute;

int main()
{
	app_t* app = app_make("Cute Framework Audio Demo", 50, 50, 400, 400, APP_OPTIONS_NO_GFX | APP_OPTIONS_RESIZABLE);

	while (is_running(app)) {
	}

	return 0;
}
