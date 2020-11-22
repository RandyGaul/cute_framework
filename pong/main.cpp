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
		//@CUTE:	app display window
		//
		//			DirectX 11	(Win10)		= CUTE_APP_OPTIONS_D3D11_CONTEXT
		//			OpenGL		(UNIX)		= CUTE_APP_OPTIONS_OPENGL_CONTEXT
		//			OpenGL ES	(mobile)	= CUTE_APP_OPTIONS_OPENGLES_CONTEXT
		//			...ref: doc > app > app_options
	
	// -- USER CODE -- //
	g_init();			//@GAM:	init
	//
	
	// App main-loop
	while (app_is_running(app))	//@CUTE:	main loop (app-scope)
	{
		float dt = calc_dt();	//@CUTE:	dt (time-elapsed since last frame)
		app_update(app, dt);	//@CUTE:	update (app-scope)	...overhead utils, not game
		
		// -- USER CODE -- //
		g_update();		//@GAM:	update
		g_draw();		//@GAM:	draw
		//
		
		app_present(app);		//@CUTE:	draw (app-scope)	...final render, to screen
	}
	
	
	app_destroy(app);			//@CUTE:	terminate (app-scope)
	
	return 0;
}


// -------------------------------------------------------------------------- //

// -- BASICS -- //

// INIT
void g_init()
{
	//@STUB
}

// UPDATE
void g_update()
{
	//@STUB
}

// DRAW
void g_draw()
{
	//@STUB
}


// -------------------------------------------------------------------------- //




// -------------------------------------------------------------------------- //
