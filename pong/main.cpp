// main.cpp
//

#include <cute.h>
using namespace cute;

// -------------------------------------------------------------------------- //

// -- GLOBALS -- //

app_t* app;

struct HUD
{
	
};

struct paddle
{
	
};

struct ball
{
	
};


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

// -- UTILS -- //

// make_entity()
// --> formulates & registers entity w/ name
// --> convenience function
// --> basic: make_entity( name, {components-list} )
// --> reqs: components listed in ... must exist (and registered)
void make_entity(const char* name, array<const char*> component_types)
{
	app_register_entity_type(app, component_types, name);
}

// make_component()
// --> configures & registers component
// --> convenience function
// --> reqs: component-struct definition w/ same name (created separately)
// --> basic: make_component( name, size_of_component )
void make_component(const char* name, int size_of_component, void* udata = NULL, 
					component_serialize_fn* serializer_fn = NULL, 
					component_cleanup_fn* cleanup_fn = NULL
				   )
{
	component_config_t c_config;
	c_config.name = name;
	c_config.size_of_component = size_of_component;
	c_config.udata = udata;
	c_config.serializer_fn = serializer;
	c_config.cleanup_fn = cleanup;
	//
	app_register_component_type(app, c_config);
}
//
void make_component(component_config_t c_config)
{
	app_register_component_type(app, c_config);
}

// make_system()
// --> configures & registers system
// --> convenience function
// --> note: component_type_tuple = component-combo required for entity targeting
// --> basic: make_system( update_fn, {components-list} )
void make_system(void* update_fn, array<const char*> component_type_tuple, void* udata = NULL, 
				 void (*pre_update_fn)(app_t* app, float dt, void* udata) = NULL, 
				 void (*post_update_fn)(app_t* app, float dt, void* udata) = NULL
				)
{
	system_config_t s_config;
	s_config.update_fn = (void*)update_fn;
	s_config.component_type_tuple = component_type_tuple;
	s_config.pre_update_fn = pre_update_fn;
	s_config.post_update_fn = post_update_fn;
	//
	app_register_system(app, s_config);
}
//
void make_system(system_config_t s_config)
{
	app_register_system(app, s_config);
}

// -------------------------------------------------------------------------- //

// -- GAM -- //



// -------------------------------------------------------------------------- //

// -- HELPERS -- //



// -------------------------------------------------------------------------- //
