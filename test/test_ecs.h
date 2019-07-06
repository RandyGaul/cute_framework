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
};

// -------------------------------------------------------------------------------------------------

void test_component_transform_initialize(void* component)
{
	test_component_transform_t* transform = (test_component_transform_t*)component;
	transform->x = 0;
	transform->y = 0;
}

error_t test_component_transform_serialize(kv_t* kv, void* component)
{
	test_component_transform_t* transform = (test_component_transform_t*)component;
	kv_key(kv, "x"); kv_val(kv, &transform->x);
	kv_key(kv, "y"); kv_val(kv, &transform->y);
	return kv_error_state(kv);
}

void test_component_sprite_initialize(void* component)
{
	test_component_sprite_t* sprite = (test_component_sprite_t*)component;
	sprite->img_id = 0;
}

error_t test_component_sprite_serialize(kv_t* kv, void* component)
{
	test_component_sprite_t* sprite = (test_component_sprite_t*)component;
	kv_key(kv, "img_id"); kv_val(kv, &sprite->img_id);
	return kv_error_state(kv);
}

void test_component_collider_initialize(void* component)
{
	test_component_collider_t* collider = (test_component_collider_t*)component;
	collider->type = 0;
	collider->radius = 0;
}

error_t test_component_collider_serialize(kv_t* kv, void* component)
{
	test_component_collider_t* collider = (test_component_collider_t*)component;
	kv_key(kv, "type"); kv_val(kv, &collider->type);
	kv_key(kv, "radius"); kv_val(kv, &collider->radius);
	return kv_error_state(kv);
}

void test_component_octorok_initialize(void* component)
{
	test_component_octorok_t* octorok = (test_component_octorok_t*)component;
	octorok->ai_state = 0;
	octorok->pellet_count = 0;
}

error_t test_component_octorok_serialize(kv_t* kv, void* component)
{
	test_component_octorok_t* octorok = (test_component_octorok_t*)component;
	kv_key(kv, "ai_state"); kv_val(kv, &octorok->ai_state);
	kv_key(kv, "pellet_count"); kv_val(kv, &octorok->pellet_count);
	return kv_error_state(kv);
}

// -------------------------------------------------------------------------------------------------

void update_test_octorok_system(float dt, test_component_transform_t* transform, test_component_octorok_t* octorok)
{
	transform->x += 1.0f * dt;
	transform->y -= 1.0f * dt;
}

// -------------------------------------------------------------------------------------------------

CUTE_TEST_CASE(test_ecs_octorok, "Run ECS with a mock Octorok entity.");
int test_ecs_octorok()
{
	app_t* app = app_make(NULL, 0, 0, 0, 0, APP_OPTIONS_HEADLESS);

	// Register component types.
	component_config_t transform_config;
	transform_config.name = CUTE_STRINGIZE(test_component_transform_t);
	transform_config.type = test_component_transform_type;
	transform_config.initializer_fn = test_component_transform_initialize;
	transform_config.serializer_fn = test_component_transform_serialize;
	app_register_component_type(app, &transform_config);

	component_config_t sprite_config;
	sprite_config.name = CUTE_STRINGIZE(test_component_sprite_t);
	sprite_config.type = test_component_sprite_type;
	sprite_config.initializer_fn = test_component_sprite_initialize;
	sprite_config.serializer_fn = test_component_sprite_serialize;
	app_register_component_type(app, &sprite_config);

	component_config_t collider_config;
	collider_config.name = CUTE_STRINGIZE(test_component_collider_t);
	collider_config.type = test_component_collider_type;
	collider_config.initializer_fn = test_component_collider_initialize;
	collider_config.serializer_fn = test_component_collider_serialize;
	app_register_component_type(app, &collider_config);

	component_config_t octorok_config;
	octorok_config.name = CUTE_STRINGIZE(test_component_octorok_t);
	octorok_config.type = test_component_octorok_type;
	octorok_config.initializer_fn = test_component_octorok_initialize;
	octorok_config.serializer_fn = test_component_octorok_serialize;
	app_register_component_type(app, &octorok_config);

	// Register entity types (just one, the octorok).
	const char* octorok_schema = CUTE_STRINGIZE({
		entity_type = "octorok",
		inherits_from = "",
		test_component_transform_t = {
		}
		test_component_octorok_t = {
		}
		test_component_sprite_t = {
			image = "/data/images/octorok.png",
		}
		test_component_collider_t = {
			collider_type = "circle"
			radius = 3
		},
	});

	component_type_t octorok_entity_types[] = {
		test_component_transform_type,
		test_component_sprite_type,
		test_component_collider_type,
		test_component_octorok_type
	};

	entity_config_t entity_config;
	entity_config.name = "Octorok";
	entity_config.type = 0;
	entity_config.types_count = 4;
	entity_config.types = octorok_entity_types;
	entity_config.schema_size = CUTE_STRLEN(octorok_schema);
	entity_config.schema = octorok_schema;
	app_register_entity_type(app, &entity_config);

	// Register systems (just one, the Octorok system).
	component_type_t octorok_system_types[] = {
		test_component_transform_type,
		test_component_sprite_type,
		test_component_collider_type,
		test_component_octorok_type
	};
	app_register_system(app, update_test_octorok_system, octorok_system_types, 4);

	const char* serialized_entities = CUTE_STRINGIZE({
		{
			entity_type = "octorok",
			test_component_transform_t = {
				x = 10,
				y = 15,
			}
		},
		{
			entity_type = "octorok",
			test_component_transform_t = {
				x = 15,
				y = 15,
			}
		},
	});
	
	// WOKRING HERE.
	// TODO
	// [ ] Register component types.
	// [ ] Register systems.
	// [ ] Register entity types.
	// [ ] Register entity schema.
	// [ ] Load entity from string.
	// [ ] Call update on systems.

	return -1;
}
