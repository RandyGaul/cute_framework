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

cute::error_t test_component_transform_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	test_component_transform_t* transform = (test_component_transform_t*)component;
	if (reading) {
		transform->x = 0;
		transform->y = 0;
	}
	kv_key(kv, "x"); kv_val(kv, &transform->x);
	kv_key(kv, "y"); kv_val(kv, &transform->y);
	return kv_error_state(kv);
}

cute::error_t test_component_sprite_serialize(app_t* app,  kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	test_component_sprite_t* sprite = (test_component_sprite_t*)component;
	if (reading) {
		sprite->img_id = 7;
	}
	kv_key(kv, "img_id"); kv_val(kv, &sprite->img_id);
	return kv_error_state(kv);
}

cute::error_t test_component_collider_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	test_component_collider_t* collider = (test_component_collider_t*)component;
	if (reading) {
		collider->type = 3;
		collider->radius = 14.0f;
	}
	kv_key(kv, "type"); kv_val(kv, &collider->type);
	kv_key(kv, "radius"); kv_val(kv, &collider->radius);
	return kv_error_state(kv);
}

cute::error_t test_component_octorok_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	test_component_octorok_t* octorok = (test_component_octorok_t*)component;
	if (reading) {
		octorok->ai_state = 0;
		octorok->pellet_count = 3;
		octorok->buddy_said_hi = 0;
	}
	kv_key(kv, "ai_state"); kv_val(kv, &octorok->ai_state);
	kv_key(kv, "pellet_count"); kv_val(kv, &octorok->pellet_count);
	kv_key(kv, "buddy"); kv_val_entity(kv, app, &octorok->buddy);
	return kv_error_state(kv);
}

// -------------------------------------------------------------------------------------------------

int s_octorok_system_ran_ok;
void update_test_octorok_system(app_t* app, float dt, void* udata, test_component_transform_t* transforms, test_component_sprite_t* sprites, test_component_collider_t* colliders, test_component_octorok_t* octoroks, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		test_component_transform_t* transform = transforms + i;
		test_component_sprite_t* sprite = sprites + i;
		test_component_collider_t* collider = colliders + i;
		test_component_octorok_t* octorok = octoroks + i;

		CUTE_ASSERT(sprite->img_id == 2); // Overridden by schema, originally initialized to 7.
		CUTE_ASSERT(collider->type == 4); // Overridden by schema, originally initialized to 3.
		CUTE_ASSERT(collider->radius == 3.0f); // Overridden by schema, originally initialized to 14.0f.
		CUTE_ASSERT(octorok->ai_state == 0);
		CUTE_ASSERT(octorok->pellet_count == 3);

		transform->x = 20.0f;
		transform->y = 20.0f;
		if (transform->x == 20.0f) s_octorok_system_ran_ok++;

		test_component_octorok_t* buddy = (test_component_octorok_t*)ecs_get_component(app, octorok->buddy, "test_component_octorok_t");
		buddy->buddy_said_hi = 1;
	}
}

// -------------------------------------------------------------------------------------------------

int s_octorok_buddy_said_hi_count;
void update_test_octorok_buddy_counter_system(app_t* app, float dt, void* udata, test_component_octorok_t* octoroks, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		test_component_octorok_t* octorok = octoroks + i;
		if (octorok->buddy_said_hi) {
			++s_octorok_buddy_said_hi_count;
		}
	}
}

// -------------------------------------------------------------------------------------------------

CUTE_TEST_CASE(test_ecs_octorok, "Run ECS with a mock Octorok entity.");
int test_ecs_octorok()
{
	app_t* app = app_make(NULL, 0, 0, 0, 0, CUTE_APP_OPTIONS_HIDDEN);

	// Register component types.
	ecs_component_begin(app);
	ecs_component_set_size(app, sizeof(test_component_transform_t));
	ecs_component_set_name(app, CUTE_STRINGIZE(test_component_transform_t));
	ecs_component_set_optional_serializer(app, test_component_transform_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_size(app, sizeof(test_component_sprite_t));
	ecs_component_set_name(app, CUTE_STRINGIZE(test_component_sprite_t));
	ecs_component_set_optional_serializer(app, test_component_sprite_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_size(app, sizeof(test_component_collider_t));
	ecs_component_set_name(app, CUTE_STRINGIZE(test_component_collider_t));
	ecs_component_set_optional_serializer(app, test_component_collider_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_size(app, sizeof(test_component_octorok_t));
	ecs_component_set_name(app, CUTE_STRINGIZE(test_component_octorok_t));
	ecs_component_set_optional_serializer(app, test_component_octorok_serialize);
	ecs_component_end(app);

	// Register entity types (just one, the octorok).
	const char* octorok_schema_string = CUTE_STRINGIZE(
		entity_type = "Octorok",
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

	ecs_entity_type_begin(app);
	ecs_entity_type_set_optional_schema(app, octorok_schema_string);
	ecs_entity_type_end(app);

	// Register systems.
	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)update_test_octorok_system);
	ecs_system_add_component(app, "test_component_transform_t");
	ecs_system_add_component(app, "test_component_sprite_t");
	ecs_system_add_component(app, "test_component_collider_t");
	ecs_system_add_component(app, "test_component_octorok_t");
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)update_test_octorok_buddy_counter_system);
	ecs_system_add_component(app, "test_component_octorok_t");
	ecs_system_end(app);

	// Load up serialized entities.
	const char* serialized_entities = CUTE_STRINGIZE(
		entities = [2] {
			{
				entity_type = "Octorok",
				test_component_transform_t = {
					x = 10.0,
					y = 15.0,
				},
				test_component_octorok_t = {
					buddy = 1,
				},
			},
			{
				entity_type = "Octorok",
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

	kv_t* parsed_entities = kv_make();
	cute::error_t err = kv_parse(parsed_entities, serialized_entities, CUTE_STRLEN(serialized_entities));
	if (err.is_error()) return -1;

	array<entity_t> entities;
	err = ecs_load_entities(app, parsed_entities, &entities);
	if (err.is_error()) return -1;
	kv_destroy(parsed_entities);

	// Assert that saving the entities has matching values to what's in RAM currently.
	kv_t* saved_entities = kv_make();
	char entity_buffer[1024];
	kv_set_write_buffer(saved_entities, entity_buffer, 1024);
	err = ecs_save_entities(app, entities, saved_entities);
	if (err.is_error()) return -1;
	size_t entity_buffer_size = kv_size_written(saved_entities);
	entity_buffer[entity_buffer_size] = 0;
	entity_buffer_size += 1;
	kv_destroy(saved_entities);

	saved_entities = kv_make();
	kv_parse(saved_entities, entity_buffer, entity_buffer_size);

	kv_key(saved_entities, "entities");
	int c;
	kv_array_begin(saved_entities, &c);
		kv_object_begin(saved_entities);
			const char* serialized_entity_type;
			size_t len;
			kv_key(saved_entities, "entity_type");
			kv_val_string(saved_entities, &serialized_entity_type, &len);
			CUTE_TEST_ASSERT(!CUTE_STRNCMP("Octorok", serialized_entity_type, len));
		kv_object_end(saved_entities);
	kv_array_end(saved_entities);

	kv_destroy(saved_entities);

	// Update the systems.
	s_octorok_system_ran_ok = 0;
	s_octorok_buddy_said_hi_count = 0;
	ecs_run_systems(app, 0);

	// Assert outcomes (make sure the systems actually ran).
	CUTE_TEST_ASSERT(s_octorok_system_ran_ok == 2);
	CUTE_TEST_ASSERT(s_octorok_buddy_said_hi_count == 2);

	app_destroy(app);

	return 0;
}

// -------------------------------------------------------------------------------------------------

struct dummy_component_t
{
	int iters = 0;
};

cute::error_t dummy_serialize(app_t* app, kv_t* kv, bool reading, entity_t entity, void* component, void* udata)
{
	dummy_component_t* dummy = (dummy_component_t*)component;
	if (reading) {
		CUTE_PLACEMENT_NEW(dummy) dummy_component_t;
	}
	return error_success();
}

void update_dummy_system(app_t* app, float dt, void* udata, dummy_component_t* dummies, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		dummy_component_t* dummy = dummies + i;
		dummy->iters++;
	}
}

CUTE_TEST_CASE(test_ecs_no_kv, "Run ECS without kv at all.");
int test_ecs_no_kv()
{
	app_t* app = app_make(NULL, 0, 0, 0, 0, CUTE_APP_OPTIONS_HIDDEN);

	ecs_component_begin(app);
	ecs_component_set_name(app, "Dummy");
	ecs_component_set_size(app, sizeof(dummy_component_t));
	ecs_component_set_optional_serializer(app, dummy_serialize);
	ecs_component_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)update_dummy_system);
	ecs_system_add_component(app, "Dummy");
	ecs_system_end(app);

	ecs_entity_type_begin(app);
	ecs_entity_type_set_name(app, "Dummy_Entity");
	ecs_entity_type_add_component(app, "Dummy");
	ecs_entity_type_end(app);

	entity_t e = ecs_make_entity(app, "Dummy_Entity");
	CUTE_TEST_ASSERT(e != INVALID_ENTITY);
	ecs_run_systems(app, 0);

	dummy_component_t* dummy = (dummy_component_t*)ecs_get_component(app, e, "Dummy");
	CUTE_TEST_ASSERT(dummy->iters == 1);

	app_destroy(app);

	return 0;
}
