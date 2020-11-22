#include <cute.h>
using namespace cute;


// -------------------------------------------------------------------------- //

// -- MAIN -- //

int main(int argc, const char** argv)
{
	// Create a window with a resolution of 640 x 480, along with a DirectX 11 context.
	uint32_t options = CUTE_APP_OPTIONS_D3D11_CONTEXT | CUTE_APP_OPTIONS_RESIZABLE | CUTE_APP_OPTIONS_WINDOW_POS_CENTERED;
	app_t* app = app_make("Fancy Window Title", 0, 0, 640, 480, options, argv[0]);

	while (app_is_running(app))
	{
		float dt = calc_dt();
		app_update(app, dt);
		app_present(app);
	}
	
	app_destroy(app);
	
	return 0;
}


// -------------------------------------------------------------------------- //

// -- BASICS -- //

// INIT
void init()
{
	
}

// UPDATE
void update()
{
	
}

// DRAW
void draw()
{
	
}


// -------------------------------------------------------------------------- //

