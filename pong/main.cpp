// main.cpp
//
// NOTES:	Cute Pong is a Pong-ish game.
//			+ upward facing layout
//			+ hit the ceiling to break it
//			+ buffs from achieving long combos (opt)
//			+ debuffs from stage interference (opt)
//			+ perception warping effects on stage (opt)
//

#include <cute.h>
#include <pong_global.h>
#include <pong_utils.h>
#include <pong_game.h>
using namespace cute;

// -------------------------------------------------------------------------- //

// -- GLOBALS -- //

app_t* app;

// -------------------------------------------------------------------------- //

// -- FUNC SIGS -- //

void g_init();
void g_update(float dt);
void g_draw();
void g_destroy();
//


// -------------------------------------------------------------------------- //

// -- MAIN -- //

int main(int argc, const char** argv)
{
	// Create a window with a resolution of 640 x 480, along with a DirectX 11 context.
	app = app_make("CUTE PONG", 50, 50, 640, 480, CUTE_APP_OPTIONS_D3D11_CONTEXT, argv[0]);
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
		g_update(dt);		//@GAM:	update
		g_draw();		//@GAM:	draw
		//
		
		app_present(app);		//@CUTE:	draw (app-scope)	...final render, to screen
	}
	
	// -- USER CODE -- //
	g_destroy();		//@GAM:	destroy
	//
	
	app_destroy(app);			//@CUTE:	terminate (app-scope)
	
	return 0;
}


// -------------------------------------------------------------------------- //

// -- BASICS -- //

// INIT
void g_init()
{
	//@STUB
	
	//init states
	//init HUD
	//init paddle
	//init ball
	//
	//init controls
	//init ...
}

// UPDATE
void g_update(float dt)
{
	//@STUB
	
	//update input
	//update paddle
	//update ball
	//update states
	//update HUD
	//
	//update vfx
	//update ...
}

// DRAW
void g_draw()
{
	//@STUB
	
	//draw BG
	//draw paddle
	//draw ball
	//draw particles
	//draw HUD
	//
	//draw vfx
	//draw ...
}

// DESTROY
void g_destroy()
{
	//@STUB
}



// -------------------------------------------------------------------------- //

// -- GAM -- //



// -------------------------------------------------------------------------- //

// -- HELPERS -- //



// -------------------------------------------------------------------------- //
