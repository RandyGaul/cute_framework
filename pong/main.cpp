// main.cpp
//

#include <cute.h>
using namespace cute;


// -------------------------------------------------------------------------- //

// -- MAIN -- //

int main(int argc, const char** argv)
{
	// Create a window with a resolution of 640 x 480, along with a DirectX 11 context.
	app_t* app = app_make("CUTE PONG", 50, 50, 640, 480, CUTE_APP_OPTIONS_D3D11_CONTEXT, argv[0]);

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
	//@STUB
}

// UPDATE
void update()
{
	//@STUB
}

// DRAW
void draw()
{
	//@STUB
}


// -------------------------------------------------------------------------- //




// -------------------------------------------------------------------------- //
