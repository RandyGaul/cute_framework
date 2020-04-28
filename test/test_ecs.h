/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <cute_ecs.h>
#include <cute_kv_utils.h>

#include <internal/cute_ecs_internal.h>

using namespace cute;

// -------------------------------------------------------------------------------------------------

enum test_component_types_t : uint32_t
{
	test_component_transform_type,
	test_component_sprite_type,
	test_component_collider_type,
	test_component_octorok_type,
};

struct test_component_transform_t
{
	float x;
	float y;
};

struct test_component_sprite_t
{
	uint64_t img_id;
};

struct test_component_collider_t
{
	uint64_t type;
	float radius;
};

struct test_component_octorok_t
{
	int ai_state;
	uint32_t pellet_count;
	array<entity_t> pellets;
	int buddy_said_hi;
	entity_t buddy;
};

// -------------------------------------------------------------------------------------------------

void test_component_transform_initialize(app_t* app, void* component, void* udata)
{
	test_component_transform_t* transform = (test_component_transform_t*)component;
	transform->x = 0;
	transform->y = 0;
}

error_t test_component_transform_serialize(app_t* app, kv_t* kv, void* component, void* udata)
{
	test_component_transform_t* transform = (test_component_transform_t*)component;
	kv_key(kv, "x"); kv_val(kv, &transform->x);
	kv_key(kv, "y"); kv_val(kv, &transform->y);
	return kv_error_state(kv);
}

void test_component_sprite_initialize(app_t* app, void* component, void* udata)
{
	test_component_sprite_t* sprite = (test_component_sprite_t*)component;
	sprite->img_id = 7;
}

error_t test_component_sprite_serialize(app_t* app, kv_t* kv, void* component, void* udata)
{
	test_component_sprite_t* sprite = (test_component_sprite_t*)component;
	kv_key(kv, "img_id"); kv_val(kv, &sprite->img_id);
	return kv_error_state(kv);
}

void test_component_collider_initialize(app_t* app, void* component, void* udata)
{
	test_component_collider_t* collider = (test_component_collider_t*)component;
	collider->type = 3;
	collider->radius = 14.0f;
}

error_t test_component_collider_serialize(app_t* app, kv_t* kv, void* component, void* udata)
{
	test_component_collider_t* collider = (test_component_collider_t*)component;
	kv_key(kv, "type"); kv_val(kv, &collider->type);
	kv_key(kv, "radius"); kv_val(kv, &collider->radius);
	return kv_error_state(kv);
}

void test_component_octorok_initialize(app_t* app, void* component, void* udata)
{
	test_component_octorok_t* octorok = (test_component_octorok_t*)component;
	octorok->ai_state = 0;
	octorok->pellet_count = 3;
	octorok->buddy_said_hi = 0;
}

error_t test_component_octorok_serialize(app_t* app, kv_t* kv, void* component, void* udata)
{
	test_component_octorok_t* octorok = (test_component_octorok_t*)component;
	kv_key(kv, "ai_state"); kv_val(kv, &octorok->ai_state);
	kv_key(kv, "pellet_count"); kv_val(kv, &octorok->pellet_count);
	kv_key(kv, "buddy"); kv_val_entity(kv, app, &octorok->buddy);
	return kv_error_state(kv);
}

// -------------------------------------------------------------------------------------------------

int s_octorok_system_ran_ok;
void update_test_octorok_system(float dt, test_component_transform_t* transform, test_component_sprite_t* sprite, test_component_collider_t* collider, test_component_octorok_t* octorok)
{
	CUTE_ASSERT(sprite->img_id == 2); // Overridden by schema, originally initialized to 7.
	CUTE_ASSERT(collider->type == 4); // Overridden by schema, originally initialized to 3.
	CUTE_ASSERT(collider->radius == 3.0f); // Overridden by schema, originally initialized to 14.0f.
	CUTE_ASSERT(octorok->ai_state == 0);
	CUTE_ASSERT(octorok->pellet_count == 3);
	transform->x = 20.0f;
	transform->y = 20.0f;
	if (transform->x == 20.0f) s_octorok_system_ran_ok++;

	// WORKING HERE
	// Want to use buddy said hi.
	// Need a way to retrieve component from an entity and mutate it.
}

// -------------------------------------------------------------------------------------------------

CUTE_TEST_CASE(test_ecs_octorok, "Run ECS with a mock Octorok entity.");
int test_ecs_octorok()
{
	app_t* app = app_make(NULL, 0, 0, 0, 0, APP_OPTIONS_HEADLESS);

	// Register component types.
	component_config_t transform_config;
	transform_config.size_of_component = sizeof(test_component_transform_t);
	transform_config.name = CUTE_STRINGIZE(test_component_transform_t);
	transform_config.type = test_component_transform_type;
	transform_config.initializer_fn = test_component_transform_initialize;
	transform_config.serializer_fn = test_component_transform_serialize;
	error_t err = app_register_component_type(app, &transform_config);
	if (err.is_error()) return -1;

	component_config_t sprite_config;
	sprite_config.size_of_component = sizeof(test_component_sprite_t);
	sprite_config.name = CUTE_STRINGIZE(test_component_sprite_t);
	sprite_config.type = test_component_sprite_type;
	sprite_config.initializer_fn = test_component_sprite_initialize;
	sprite_config.serializer_fn = test_component_sprite_serialize;
	err = app_register_component_type(app, &sprite_config);
	if (err.is_error()) return -1;

	component_config_t collider_config;
	collider_config.size_of_component = sizeof(test_component_collider_t);
	collider_config.name = CUTE_STRINGIZE(test_component_collider_t);
	collider_config.type = test_component_collider_type;
	collider_config.initializer_fn = test_component_collider_initialize;
	collider_config.serializer_fn = test_component_collider_serialize;
	err = app_register_component_type(app, &collider_config);
	if (err.is_error()) return -1;

	component_config_t octorok_config;
	octorok_config.size_of_component = sizeof(test_component_octorok_t);
	octorok_config.name = CUTE_STRINGIZE(test_component_octorok_t);
	octorok_config.type = test_component_octorok_type;
	octorok_config.initializer_fn = test_component_octorok_initialize;
	octorok_config.serializer_fn = test_component_octorok_serialize;
	err = app_register_component_type(app, &octorok_config);
	if (err.is_error()) return -1;

	// Register entity types (just one, the octorok).
	const char* octorok_schema_string = CUTE_STRINGIZE(
		entity_name = "Octorok",
		entity_type = 0,
		test_component_transform_t = { },
		test_component_octorok_t = { },
		test_component_sprite_t = {
			img_id = 2,
		},
		test_component_collider_t = {
			type = 4,
			radius = 3
		},
	);

	kv_t* entity_schema = kv_make();
	err = kv_parse(entity_schema, octorok_schema_string, CUTE_STRLEN(octorok_schema_string));
	if (err.is_error()) return -1;
	err = app_register_entity_type(app, entity_schema);
	if (err.is_error()) return -1;

	// Register systems (just one, the Octorok system).
	component_type_t octorok_system_types[] = {
		test_component_transform_type,
		test_component_sprite_type,
		test_component_collider_type,
		test_component_octorok_type
	};
	app_register_system(app, update_test_octorok_system, octorok_system_types, 4);

	// Load up serialized entities.
	const char* serialized_entities = CUTE_STRINGIZE(
		entities = [2] {
			{
				entity_type = 0,
				test_component_transform_t = {
					x = 10.0,
					y = 15.0,
				},
				test_component_octorok_t = {
					buddy = 1,
				},
			},
			{
				entity_type = 0,
				test_component_transform_t = {
					x = 30,
					y = 40,
				},
				test_component_octorok_t = {
					buddy = 0,
				},
			},
		}
	);

	// This crashes since id_table isn't hooked up yet.
	kv_t* parsed_entities = kv_make();
	err = kv_parse(parsed_entities, serialized_entities, CUTE_STRLEN(serialized_entities));
	if (err.is_error()) return -1;

	array<entity_t> entities;
	err = app_load_entities(app, parsed_entities, &entities);
	if (err.is_error()) return -1;

	kv_t* saved_entities = kv_make();
	char entity_buffer[1024];
	kv_set_write_buffer(saved_entities, entity_buffer, 1024);
	err = app_save_entities(app, entities, saved_entities);
	entity_buffer[kv_size_written(saved_entities)] = 0;
	//printf("%s", entity_buffer);

	// Update the systems.
	s_octorok_system_ran_ok = 0;
	app_update_systems(app);

	// Assert outcomes (make sure the systems actually ran).
	CUTE_TEST_ASSERT(s_octorok_system_ran_ok == 2);

	app_destroy(app);

	return 0;
}
