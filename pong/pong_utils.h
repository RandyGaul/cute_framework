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
void make_component(const char* name, int size_of_component,
					component_serialize_fn* serializer_fn = NULL, void* serializer_udata = NULL,
					component_cleanup_fn* cleanup_fn = NULL, void* cleanup_udata = NULL
				   );
void make_system(void* update_fn, array<const char*> component_type_tuple, void* udata = NULL, 
				 void (*pre_update_fn)(app_t* app, float dt, void* udata) = NULL, 
				 void (*post_update_fn)(app_t* app, float dt, void* udata) = NULL
				);
//


// -------------------------------------------------------------------------- //

// -- FUNC DEFINITIONS -- //

// make_entity()
// --> configures & registers entity w/ name
// --> convenience function
// --> basic: make_entity( name, {components-list} )
// --> reqs: components listed in ... must exist (and registered)
void make_entity(const char* name, array<const char*> component_types)
{
	ecs_entity_begin(app);
	ecs_entity_set_name(app, name);
	for (int i = 0; i < component_types.size(); ++i) {
		ecs_entity_add_component(app, component_types[i]);
	}
	ecs_entity_end(app);
}

// make_component()
// --> configures & registers component
// --> convenience function
// --> reqs: component-struct definition w/ same name (created separately)
// --> basic: make_component( name, size_of_component )
void make_component(const char* name, int size_of_component, 
					component_serialize_fn* serializer_fn, void* serializer_udata,
					component_cleanup_fn* cleanup_fn, void* cleanup_udata
				   )
{
	ecs_component_begin(app);
	ecs_component_set_name(app, name);
	ecs_component_set_size(app, size_of_component);
	ecs_component_set_optional_serializer(app, serializer_fn, serializer_udata);
	ecs_component_set_optional_cleanup(app, cleanup_fn, cleanup_udata);
	ecs_component_end(app);
}
//

// make_system()
// --> configures & registers system
// --> convenience function
// --> note: component_type_tuple = component-combo required for entity targeting
// --> basic: make_system( update_fn, {components-list} )
void make_system(void* update_fn, array<const char*> component_type_tuple, void* udata, 
				 void (*pre_update_fn)(app_t* app, float dt, void* udata), 
				 void (*post_update_fn)(app_t* app, float dt, void* udata)
				)
{
	ecs_system_begin(app);
	ecs_system_set_update(app, update_fn);
	for (int i = 0; i < component_type_tuple.size(); ++i) {
		ecs_system_require_component(app, component_type_tuple[i]);
	}
	ecs_system_set_optional_pre_update(app, pre_update_fn);
	ecs_system_set_optional_post_update(app, post_update_fn);
	ecs_system_set_optional_update_udata(app, udata);
	ecs_system_end(app);
}
//

// -------------------------------------------------------------------------- //

#endif	// PONG_UTILS
