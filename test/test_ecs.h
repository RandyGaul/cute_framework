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
	array<cf_entity_t> pellets;
	int buddy_said_hi;
	cf_entity_t buddy;
};

// -------------------------------------------------------------------------------------------------

bool test_component_transform_serialize(cf_kv_t* kv, bool reading, cf_entity_t entity, void* component, void* udata)
{
	test_component_transform_t* transform = (test_component_transform_t*)component;
	if (reading) {
		transform->x = 0;
		transform->y = 0;
	}
	cf_kv_key(kv, "x", NULL); cf_kv_val_float(kv, &transform->x);
	cf_kv_key(kv, "y", NULL); cf_kv_val_float(kv, &transform->y);
	return !cf_is_error(cf_kv_error_state(kv));
}

bool test_component_sprite_serialize(cf_kv_t* kv, bool reading, cf_entity_t entity, void* component, void* udata)
{
	test_component_sprite_t* sprite = (test_component_sprite_t*)component;
	if (reading) {
		sprite->img_id = 7;
	}
	cf_kv_key(kv, "img_id", NULL); cf_kv_val_uint64(kv, &sprite->img_id);
	return !cf_is_error(cf_kv_error_state(kv));
}

bool test_component_collider_serialize(cf_kv_t* kv, bool reading, cf_entity_t entity, void* component, void* udata)
{
	test_component_collider_t* collider = (test_component_collider_t*)component;
	if (reading) {
		collider->type = 3;
		collider->radius = 14.0f;
	}
	cf_kv_key(kv, "type", NULL); cf_kv_val_uint64(kv, &collider->type);
	cf_kv_key(kv, "radius", NULL); cf_kv_val_float(kv, &collider->radius);
	return !cf_is_error(cf_kv_error_state(kv));
}

bool test_component_octorok_serialize(cf_kv_t* kv, bool reading, cf_entity_t entity, void* component, void* udata)
{
	test_component_octorok_t* octorok = (test_component_octorok_t*)component;
	if (reading) {
		octorok->ai_state = 0;
		octorok->pellet_count = 3;
		octorok->buddy_said_hi = 0;
	}
	cf_kv_key(kv, "ai_state", NULL); cf_kv_val_int32(kv, &octorok->ai_state);
	cf_kv_key(kv, "pellet_count", NULL); cf_kv_val_uint32(kv, &octorok->pellet_count);
	cf_kv_key(kv, "buddy", NULL); cf_kv_val_entity(kv, &octorok->buddy);
	return !cf_is_error(cf_kv_error_state(kv));
}

// -------------------------------------------------------------------------------------------------

#define FIND_COMPONENTS(T) T* T##s = (T*)cf_ecs_arrays_find_components(arrays, #T); CUTE_ASSERT(!count || T##s)

int s_octorok_system_ran_ok;
void update_test_octorok_system(float dt, cf_ecs_arrays_t* arrays, int count, void* udata)
{
	FIND_COMPONENTS(test_component_transform_t);
	FIND_COMPONENTS(test_component_sprite_t);
	FIND_COMPONENTS(test_component_collider_t);
	FIND_COMPONENTS(test_component_octorok_t);
	for (int i = 0; i < count; ++i) {
		test_component_transform_t* transform = test_component_transform_ts + i;
		test_component_sprite_t* sprite = test_component_sprite_ts + i;
		test_component_collider_t* collider = test_component_collider_ts + i;
		test_component_octorok_t* octorok = test_component_octorok_ts + i;

		CUTE_ASSERT(sprite->img_id == 2); // Overridden by schema, originally initialized to 7.
		CUTE_ASSERT(collider->type == 4); // Overridden by schema, originally initialized to 3.
		CUTE_ASSERT(collider->radius == 3.0f); // Overridden by schema, originally initialized to 14.0f.
		CUTE_ASSERT(octorok->ai_state == 0);
		CUTE_ASSERT(octorok->pellet_count == 3);

		transform->x = 20.0f;
		transform->y = 20.0f;
		if (transform->x == 20.0f) s_octorok_system_ran_ok++;

		test_component_octorok_t* buddy = (test_component_octorok_t*)cf_entity_get_component(octorok->buddy, "test_component_octorok_t");
		buddy->buddy_said_hi = 1;
	}
}

// -------------------------------------------------------------------------------------------------

int s_octorok_buddy_said_hi_count;
void update_test_octorok_buddy_counter_system(float dt, cf_ecs_arrays_t* arrays, int count, void* udata)
{
	FIND_COMPONENTS(test_component_octorok_t);
	for (int i = 0; i < count; ++i) {
		test_component_octorok_t* octorok = test_component_octorok_ts + i;
		if (octorok->buddy_said_hi) {
			++s_octorok_buddy_said_hi_count;
		}
	}
}

// -------------------------------------------------------------------------------------------------

CUTE_TEST_CASE(test_ecs_octorok, "Run ECS with a mock Octorok entity.");
int test_ecs_octorok()
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, APP_OPTIONS_HIDDEN, NULL))) {
		return -1;
	}

	// Register component types.
	cf_ecs_component_begin();
	cf_ecs_component_set_size(sizeof(test_component_transform_t));
	cf_ecs_component_set_name(CUTE_STRINGIZE(test_component_transform_t));
	cf_ecs_component_set_optional_serializer(test_component_transform_serialize, NULL);
	cf_ecs_component_end();

	cf_ecs_component_begin();
	cf_ecs_component_set_size(sizeof(test_component_sprite_t));
	cf_ecs_component_set_name(CUTE_STRINGIZE(test_component_sprite_t));
	cf_ecs_component_set_optional_serializer(test_component_sprite_serialize, NULL);
	cf_ecs_component_end();

	cf_ecs_component_begin();
	cf_ecs_component_set_size(sizeof(test_component_collider_t));
	cf_ecs_component_set_name(CUTE_STRINGIZE(test_component_collider_t));
	cf_ecs_component_set_optional_serializer(test_component_collider_serialize, NULL);
	cf_ecs_component_end();

	cf_ecs_component_begin();
	cf_ecs_component_set_size(sizeof(test_component_octorok_t));
	cf_ecs_component_set_name(CUTE_STRINGIZE(test_component_octorok_t));
	cf_ecs_component_set_optional_serializer(test_component_octorok_serialize, NULL);
	cf_ecs_component_end();

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

	cf_ecs_entity_begin();
	cf_ecs_entity_set_optional_schema(octorok_schema_string);
	cf_ecs_entity_end();

	// Register systems.
	cf_ecs_system_begin();
	cf_ecs_system_set_update(update_test_octorok_system);
	cf_ecs_system_require_component("test_component_transform_t");
	cf_ecs_system_require_component("test_component_sprite_t");
	cf_ecs_system_require_component("test_component_collider_t");
	cf_ecs_system_require_component("test_component_octorok_t");
	cf_ecs_system_end();

	cf_ecs_system_begin();
	cf_ecs_system_set_update(update_test_octorok_buddy_counter_system);
	cf_ecs_system_require_component("test_component_octorok_t");
	cf_ecs_system_end();

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

	cf_kv_t* parsed_entities = cf_kv_read(serialized_entities, CUTE_STRLEN(serialized_entities), NULL);
	if (!parsed_entities) return -1;

	array<cf_entity_t> entities;
	result_t err = cute::ecs_load_entities(parsed_entities, &entities);
	if (cf_is_error(err)) return -1;
	cf_kv_destroy(parsed_entities);

	// Assert that saving the entities has matching values to what's in RAM currently.
	cf_kv_t* saved_entities = cf_kv_write();
	err = cute::ecs_save_entities(entities, saved_entities);
	if (cf_is_error(err)) return -1;

	parsed_entities = cf_kv_read(cf_kv_buffer(saved_entities), cf_kv_buffer_size(saved_entities), NULL);

	cf_kv_key(parsed_entities, "entities", NULL);
	int c;
	cf_kv_array_begin(parsed_entities, &c, NULL);
		cf_kv_object_begin(parsed_entities, NULL);
			const char* serialized_entity_type;
			size_t len;
			cf_kv_key(parsed_entities, "entity_type", NULL);
			cf_kv_val_string(parsed_entities, &serialized_entity_type, &len);
			CUTE_TEST_ASSERT(!CUTE_STRNCMP("Octorok", serialized_entity_type, len));
		cf_kv_object_end(parsed_entities);
	cf_kv_array_end(parsed_entities);
	
	cf_kv_destroy(parsed_entities);
	cf_kv_destroy(saved_entities);

	// Update the systems.
	s_octorok_system_ran_ok = 0;
	s_octorok_buddy_said_hi_count = 0;
	cf_ecs_run_systems(0);

	// Assert outcomes (make sure the systems actually ran).
	CUTE_TEST_ASSERT(s_octorok_system_ran_ok == 2);
	CUTE_TEST_ASSERT(s_octorok_buddy_said_hi_count == 2);

	cf_destroy_app();

	return 0;
}

// -------------------------------------------------------------------------------------------------

struct dummy_component_t
{
	int iters = 0;
};

bool dummy_serialize(cf_kv_t* kv, bool reading, cf_entity_t entity, void* component, void* udata)
{
	dummy_component_t* dummy = (dummy_component_t*)component;
	if (reading) {
		CUTE_PLACEMENT_NEW(dummy) dummy_component_t;
	}
	return true;
}

void update_dummy_system(float dt, cf_ecs_arrays_t* arrays, int count, void* udata)
{
	FIND_COMPONENTS(dummy_component_t);
	for (int i = 0; i < count; ++i) {
		dummy_component_t* dummy = dummy_component_ts + i;
		dummy->iters++;
	}
}

CUTE_TEST_CASE(test_ecs_no_kv, "Run ECS without kv at all.");
int test_ecs_no_kv()
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, APP_OPTIONS_HIDDEN, NULL))) {
		return -1;
	}

	cf_ecs_component_begin();
	cf_ecs_component_set_name("dummy_component_t");
	cf_ecs_component_set_size(sizeof(dummy_component_t));
	cf_ecs_component_set_optional_serializer(dummy_serialize, NULL);
	cf_ecs_component_end();

	cf_ecs_system_begin();
	cf_ecs_system_set_update(update_dummy_system);
	cf_ecs_system_require_component("dummy_component_t");
	cf_ecs_system_end();

	cf_ecs_entity_begin();
	cf_ecs_entity_set_name("Dummy_Entity");
	cf_ecs_entity_add_component("dummy_component_t");
	cf_ecs_entity_end();

	cf_entity_t e = cf_make_entity("Dummy_Entity", NULL);
	CUTE_TEST_ASSERT(e != CF_INVALID_ENTITY);
	cf_ecs_run_systems(0);

	dummy_component_t* dummy = (dummy_component_t*)cf_entity_get_component(e, "dummy_component_t");
	CUTE_TEST_ASSERT(dummy->iters == 1);

	cf_destroy_app();

	return 0;
}
