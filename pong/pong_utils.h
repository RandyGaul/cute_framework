// pong_utils.h
// 

#ifndef PONG_UTILS
#define PONG_UTILS

// -------------------------------------------------------------------------- //

#include <cute.h>
#include <pong_global.h>
using namespace cute;


// -------------------------------------------------------------------------- //

// -- GLOBAlS -- //

//@STUB:	constants, enums, global-vars


// -------------------------------------------------------------------------- //

// -- STRUCTS -- //

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


// -------------------------------------------------------------------------- //

// -- FUNC SIGS -- //

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
//


// -------------------------------------------------------------------------- //

// -- FUNC DEFINITIONS -- //

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

#endif	// PONG_UTILS
