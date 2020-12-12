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
using namespace cute;

// -------------------------------------------------------------------------- //

// -- GLOBALS -- //

app_t* app;

struct Input_map
{
	key_button_t up;		//i.e. row toggle, menu nav
	key_button_t down;		//i.e. row toggle, menu nav
	//
	key_button_t left;		//basic movement
	key_button_t right;		//basic movement
	//
	key_button_t action;	//i.e. launch ball, confirm, etc.
};

struct Player
{
	//@TODO:	player state
	int id;
	int lives;
	//
	Input_map inputs;
};

struct HUD
{
	//@TODO:	HUD display object
};

struct Paddle
{
	enum Buff {fast=1, big=2, stricky=3, bar=4};
	enum Debuff {slow=1, small=2, inverted=3, stun=4};
	//
	static float base_accel;
	static float base_max_speed;
	
	int id;				//match to player-ID
	//
	int x;
	int y;
	int width;
	int height;
	//
	float dx;
	float dy;
	float accel;
	float max_speed;
	//
	array<Buff> buffs;		//@CONSIDER:	using a bitfield
	array<Debuff> debuffs; 	//@CONSIDER:	using a bitfield
};

struct Ball
{
	static float base_speed;	// = 1.0f;		//baseline ball speed
	static float speed_mult;	// = 2.0f;		//multiplier (on speed-up)
	
	int x;
	int y;
	float speed;
	v2 dir;
};


// -------------------------------------------------------------------------- //

// -- FUNC DECLARATIONS -- //

void g_init();
void g_update(float dt);
void g_draw();
void g_destroy();
//
void make_entity(const char* name, array<const char*> component_types);
void make_component(const char* name, int size_of_component, void* udata, 
					component_serialize_fn* serializer_fn, 
					component_cleanup_fn* cleanup_fn
				   );
void make_component(component_config_t c_config);
void make_system(void* update_fn, array<const char*> component_type_tuple, void* udata, 
				 void (*pre_update_fn)(app_t* app, float dt, void* udata), 
				 void (*post_update_fn)(app_t* app, float dt, void* udata)
				);
void make_system(system_config_t s_config);


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
	c_config.serializer_fn = serializer_fn;
	c_config.cleanup_fn = cleanup_fn;
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
