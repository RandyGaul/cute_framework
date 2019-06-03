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

struct test_component_transform_t : public component_t
{
	float x;
	float y;
};

struct test_component_sprite_t : public component_t
{
	uint64_t sprite_id;
};

struct test_component_collider_t : public component_t
{
	uint64_t collider_type;
	float radius;
};

struct test_octorok_component_t : public component_t
{
	int ai_state;
	uint32_t octorok_pellet_count;
};

enum test_component_types_t : uint32_t
{
	test_component_transform_type,
	test_component_sprite_type,
	test_component_collider_type,
	test_octorok_component_type,
};

void test_component_transform_initialize(component_t* component)
{
	test_component_transform_t* transform = (test_component_transform_t*)component;
	transform->x = 0;
	transform->y = 0;
}

error_t test_component_transform_serialize(kv_t* kv, component_t* component)
{
	test_component_transform_t* transform = (test_component_transform_t*)component;
	kv_key(kv, "x"); kv_val(kv, &transform->x);
	kv_key(kv, "y"); kv_val(kv, &transform->y);
	return kv_error_state(kv);
}

// -------------------------------------------------------------------------------------------------

struct test_system_transform_t : public system_t
{
	test_system_transform_t()
		: system_t(
			CUTE_STRINGIZE(test_system_transform_t),
			CUTE_STRINGIZE(test_component_transform_t),
			test_component_transform_type,
			sizeof(test_component_transform_t),
			1024,
			1024
		)
	{
	}

	virtual void update(float dt) override
	{
		int component_count = get_components_count();
		test_component_transform_t* components = (test_component_transform_t*)get_components();
		for (int i = 0; i < component_count; ++i)
		{
			test_component_transform_t* transform = components + i;
			transform->x += 1.0f * dt;
			transform->y -= 1.0f * dt;
		}
	}
};

struct test_system_sprite_t : public system_t
{
	void push_sprite(uint64_t sprite_id)
	{
		// WORKING HERE
		// Gotta make getting other components trivial.
		// Lookup entity by id, then fetch components from systems.
		// Needs to be streamlined.
		// app_get_entity(app, entity_id)
		CUTE_UNUSED(sprite_id);
	}

	virtual void update(float dt) override
	{
		int component_count = get_components_count();
		test_component_sprite_t* components = (test_component_sprite_t*)get_components();
		for (int i = 0; i < component_count; ++i)
		{
			test_component_sprite_t* sprite = components + i;
			push_sprite(sprite->sprite_id);
		}
	}
};

struct test_system_collider_t : public system_t
{
	void debug_draw(test_component_collider_t* collider)
	{
		CUTE_UNUSED(collider);
	}

	virtual void update(float dt) override
	{
		int component_count = get_components_count();
		test_component_collider_t* components = (test_component_collider_t*)get_components();
		for (int i = 0; i < component_count; ++i)
		{
			test_component_collider_t* collider = components + i;
			debug_draw(collider);
		}
	}
};

struct test_system_octorok_t : public system_t
{
	void run_ai(test_octorok_component_t* octorok)
	{
		CUTE_UNUSED(octorok);
	}

	virtual void update(float dt) override
	{
		int component_count = get_components_count();
		test_octorok_component_t* components = (test_octorok_component_t*)get_components();
		for (int i = 0; i < component_count; ++i)
		{
			test_octorok_component_t* octorok = components + i;
			run_ai(octorok);
		}
	}
};

// -------------------------------------------------------------------------------------------------

CUTE_TEST_CASE(test_ecs_octorok, "Run ECS with a mock Octorok entity.");
int test_ecs_octorok()
{
	app_t* app = app_make(NULL, 0, 0, 0, 0, APP_OPTIONS_HEADLESS);

	const char* octorok_schema = CUTE_STRINGIZE({
		entity_type = "octorok",
		test_component_transform_t = {
		}
		test_octorok_component_t = {
		}
		test_component_sprite_t = {
			image = "/data/images/octorok.png",
		}
		test_component_collider_t = {
			collider_type = "circle"
			radius = 3
		},
	});

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

	component_config_t transform_config;
	transform_config.component_name = CUTE_STRINGIZE(test_component_transform_t);
	transform_config.component_type = test_component_transform_type;
	transform_config.component_initializer = test_component_transform_initialize;
	transform_config.component_serializer = test_component_transform_serialize;
	app_register_component(app, &transform_config);

	test_system_transform_t transform_system;

	app_add_system(app, &transform_system);

	error_t err = app_register_entity_schema(app, "octorok", 0, octorok_schema, (int)CUTE_STRLEN(octorok_schema));
	if (err.is_error()) return -1;

	return -1;
}
