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

#include <internal/cute_ecs_internal.h>

using namespace Cute;

// -------------------------------------------------------------------------------------------------

struct TestComponentTransform
{
	float x;
	float y;
};

struct TestComponentSprite
{
	uint64_t img_id;
};

struct TestComponentCollider
{
	uint64_t type;
	float radius;
};

struct TestComponentOctorok
{
	int ai_state;
	uint32_t pellet_count;
	Array<CF_Entity> pellets;
	int buddy_said_hi;
	CF_Entity buddy;
};

// -------------------------------------------------------------------------------------------------

void test_component_transform_initialize(CF_Entity entity, void* component, void* udata)
{
	TestComponentTransform* transform = (TestComponentTransform*)component;
	transform->x = 0;
	transform->y = 0;
}

void test_component_sprite_initialize(CF_Entity entity, void* component, void* udata)
{
	TestComponentSprite* sprite = (TestComponentSprite*)component;
	sprite->img_id = 7;
}

void test_component_collider_initialize(CF_Entity entity, void* component, void* udata)
{
	TestComponentCollider* collider = (TestComponentCollider*)component;
	collider->type = 3;
	collider->radius = 14.0f;
}

void test_component_octorok_initialize(CF_Entity entity, void* component, void* udata)
{
	TestComponentOctorok* octorok = (TestComponentOctorok*)component;
	octorok->ai_state = 0;
	octorok->pellet_count = 3;
	octorok->buddy_said_hi = 0;
}

// -------------------------------------------------------------------------------------------------

int s_octorok_system_ran_ok;
void update_test_octorok_system(CF_ComponentList component_list, int count, void* udata)
{
	TestComponentTransform* transforms = CF_GET_COMPONENTS(component_list, TestComponentTransform);
	TestComponentSprite* sprites = CF_GET_COMPONENTS(component_list, TestComponentSprite);
	TestComponentCollider* colliders = CF_GET_COMPONENTS(component_list, TestComponentCollider);
	TestComponentOctorok* octoroks = CF_GET_COMPONENTS(component_list, TestComponentOctorok);
	for (int i = 0; i < count; ++i) {
		TestComponentTransform* transform = transforms + i;
		TestComponentSprite* sprite = sprites + i;
		TestComponentCollider* collider = colliders + i;
		TestComponentOctorok* octorok = octoroks + i;

		CF_ASSERT(sprite->img_id == 7);
		CF_ASSERT(collider->type == 3);
		CF_ASSERT(collider->radius == 14.0f);
		CF_ASSERT(octorok->ai_state == 0);
		CF_ASSERT(octorok->pellet_count == 3);

		transform->x = 20.0f;
		transform->y = 20.0f;
		if (transform->x == 20.0f) s_octorok_system_ran_ok++;

		TestComponentOctorok* buddy = (TestComponentOctorok*)cf_entity_get_component(octorok->buddy, "TestComponentOctorok");
		buddy->buddy_said_hi = 1;
	}
}

// -------------------------------------------------------------------------------------------------

int s_octorok_buddy_said_hi_count;
void update_test_octorok_buddy_counter_system(CF_ComponentList component_list, int count, void* udata)
{
	TestComponentOctorok* octoroks = CF_GET_COMPONENTS(component_list, TestComponentOctorok);
	for (int i = 0; i < count; ++i) {
		TestComponentOctorok* octorok = octoroks + i;
		if (octorok->buddy_said_hi) {
			++s_octorok_buddy_said_hi_count;
		}
	}
}

// -------------------------------------------------------------------------------------------------

CF_TEST_CASE(test_ecs_octorok, "Run ECS with a mock Octorok entity.");
int test_ecs_octorok()
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, APP_OPTIONS_HIDDEN, NULL))) {
		return -1;
	}

	// Register component types.
	cf_component_begin();
	cf_component_set_size(sizeof(TestComponentTransform));
	cf_component_set_name(CF_STRINGIZE(TestComponentTransform));
	cf_component_set_optional_initializer(test_component_transform_initialize, NULL);
	cf_component_end();

	cf_component_begin();
	cf_component_set_size(sizeof(TestComponentSprite));
	cf_component_set_name(CF_STRINGIZE(TestComponentSprite));
	cf_component_set_optional_initializer(test_component_sprite_initialize, NULL);
	cf_component_end();

	cf_component_begin();
	cf_component_set_size(sizeof(TestComponentCollider));
	cf_component_set_name(CF_STRINGIZE(TestComponentCollider));
	cf_component_set_optional_initializer(test_component_collider_initialize, NULL);
	cf_component_end();

	cf_component_begin();
	cf_component_set_size(sizeof(TestComponentOctorok));
	cf_component_set_name(CF_STRINGIZE(TestComponentOctorok));
	cf_component_set_optional_initializer(test_component_octorok_initialize, NULL);
	cf_component_end();

	// Register the entity type.
	cf_entity_begin();
	cf_entity_set_name("Octorok");
	cf_entity_add_component(CF_STRINGIZE(TestComponentTransform));
	cf_entity_add_component(CF_STRINGIZE(TestComponentOctorok));
	cf_entity_add_component(CF_STRINGIZE(TestComponentSprite));
	cf_entity_add_component(CF_STRINGIZE(TestComponentCollider));
	cf_entity_end();

	// Register systems.
	cf_system_begin();
	cf_system_set_update(update_test_octorok_system);
	cf_system_require_component(CF_STRINGIZE(TestComponentTransform));
	cf_system_require_component(CF_STRINGIZE(TestComponentOctorok));
	cf_system_require_component(CF_STRINGIZE(TestComponentSprite));
	cf_system_require_component(CF_STRINGIZE(TestComponentCollider));
	cf_system_end();

	cf_system_begin();
	cf_system_set_update(update_test_octorok_buddy_counter_system);
	cf_system_require_component(CF_STRINGIZE(TestComponentOctorok));
	cf_system_end();

	// Create the entities.
	Entity e0 = make_entity("Octorok");
	Entity e1 = make_entity("Octorok");

	TestComponentOctorok* octorok0 = (TestComponentOctorok*)entity_get_component(e0, CF_STRINGIZE(TestComponentOctorok));
	TestComponentOctorok* octorok1 = (TestComponentOctorok*)entity_get_component(e1, CF_STRINGIZE(TestComponentOctorok));
	octorok0->buddy = e1;
	octorok1->buddy = e0;

	// Update the systems.
	s_octorok_system_ran_ok = 0;
	s_octorok_buddy_said_hi_count = 0;
	cf_run_systems();

	// Assert outcomes (make sure the systems actually ran).
	CF_TEST_ASSERT(s_octorok_system_ran_ok == 2);
	CF_TEST_ASSERT(s_octorok_buddy_said_hi_count == 2);

	cf_destroy_app();

	return 0;
}

// -------------------------------------------------------------------------------------------------

struct DummyComponent
{
	int iters = 0;
};

void dummy_initialize(CF_Entity entity, void* component, void* udata)
{
	DummyComponent* dummy = (DummyComponent*)component;
	CF_PLACEMENT_NEW(dummy) DummyComponent;
}

void update_dummy_system(CF_ComponentList component_list, int count, void* udata)
{
	DummyComponent* dummies = CF_GET_COMPONENTS(component_list, DummyComponent);
	for (int i = 0; i < count; ++i) {
		DummyComponent* dummy = dummies + i;
		dummy->iters++;
	}
}

CF_TEST_CASE(test_ecs_basic, "Very rudimentary test of the ECS.");
int test_ecs_basic()
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, APP_OPTIONS_HIDDEN, NULL))) {
		return -1;
	}

	cf_component_begin();
	cf_component_set_name("DummyComponent");
	cf_component_set_size(sizeof(DummyComponent));
	cf_component_set_optional_initializer(dummy_initialize, NULL);
	cf_component_end();

	cf_system_begin();
	cf_system_set_update(update_dummy_system);
	cf_system_require_component("DummyComponent");
	cf_system_end();

	cf_entity_begin();
	cf_entity_set_name("Dummy_Entity");
	cf_entity_add_component("DummyComponent");
	cf_entity_end();

	CF_Entity e = cf_make_entity("Dummy_Entity");
	CF_TEST_ASSERT(e != CF_INVALID_ENTITY);
	cf_run_systems();

	DummyComponent* dummy = (DummyComponent*)cf_entity_get_component(e, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 1);

	cf_destroy_app();

	return 0;
}

CF_TEST_CASE(test_ecs_activation, "Tests out ECS the activate/deactivate API.");
int test_ecs_activation()
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, APP_OPTIONS_HIDDEN, NULL))) {
		return -1;
	}

	cf_component_begin();
	cf_component_set_name("DummyComponent");
	cf_component_set_size(sizeof(DummyComponent));
	cf_component_set_optional_initializer(dummy_initialize, NULL);
	cf_component_end();

	cf_system_begin();
	cf_system_set_update(update_dummy_system);
	cf_system_require_component("DummyComponent");
	cf_system_end();

	cf_entity_begin();
	cf_entity_set_name("Dummy_Entity");
	cf_entity_add_component("DummyComponent");
	cf_entity_end();
	
	CF_Entity e0 = cf_make_entity("Dummy_Entity");
	CF_TEST_ASSERT(e0 != CF_INVALID_ENTITY);
	CF_Entity e1 = cf_make_entity("Dummy_Entity");
	CF_TEST_ASSERT(e1 != CF_INVALID_ENTITY);
	CF_Entity e2 = cf_make_entity("Dummy_Entity");
	CF_TEST_ASSERT(e2 != CF_INVALID_ENTITY);
	cf_run_systems();

	DummyComponent* dummy = (DummyComponent*)cf_entity_get_component(e0, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 1);
	dummy = (DummyComponent*)cf_entity_get_component(e1, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 1);
	dummy = (DummyComponent*)cf_entity_get_component(e2, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 1);

	cf_entity_deactivate(e2);
	cf_run_systems();

	dummy = (DummyComponent*)cf_entity_get_component(e0, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 2);
	dummy = (DummyComponent*)cf_entity_get_component(e1, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 2);
	dummy = (DummyComponent*)cf_entity_get_component(e2, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 1);
	CF_TEST_ASSERT(cf_entity_is_active(e0));
	CF_TEST_ASSERT(cf_entity_is_active(e1));
	CF_TEST_ASSERT(!cf_entity_is_active(e2));

	cf_entity_deactivate(e1);
	cf_run_systems();

	dummy = (DummyComponent*)cf_entity_get_component(e0, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 3);
	dummy = (DummyComponent*)cf_entity_get_component(e1, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 2);
	dummy = (DummyComponent*)cf_entity_get_component(e2, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 1);
	CF_TEST_ASSERT(cf_entity_is_active(e0));
	CF_TEST_ASSERT(!cf_entity_is_active(e1));
	CF_TEST_ASSERT(!cf_entity_is_active(e2));

	cf_entity_deactivate(e0);
	cf_run_systems();

	dummy = (DummyComponent*)cf_entity_get_component(e0, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 3);
	dummy = (DummyComponent*)cf_entity_get_component(e1, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 2);
	dummy = (DummyComponent*)cf_entity_get_component(e2, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 1);
	CF_TEST_ASSERT(!cf_entity_is_active(e0));
	CF_TEST_ASSERT(!cf_entity_is_active(e1));
	CF_TEST_ASSERT(!cf_entity_is_active(e2));

	cf_entity_activate(e2);
	cf_run_systems();

	dummy = (DummyComponent*)cf_entity_get_component(e0, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 3);
	dummy = (DummyComponent*)cf_entity_get_component(e1, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 2);
	dummy = (DummyComponent*)cf_entity_get_component(e2, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 2);
	CF_TEST_ASSERT(!cf_entity_is_active(e0));
	CF_TEST_ASSERT(!cf_entity_is_active(e1));
	CF_TEST_ASSERT(cf_entity_is_active(e2));

	cf_entity_activate(e0);
	cf_run_systems();

	dummy = (DummyComponent*)cf_entity_get_component(e0, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 4);
	dummy = (DummyComponent*)cf_entity_get_component(e1, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 2);
	dummy = (DummyComponent*)cf_entity_get_component(e2, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 3);
	CF_TEST_ASSERT(cf_entity_is_active(e0));
	CF_TEST_ASSERT(!cf_entity_is_active(e1));
	CF_TEST_ASSERT(cf_entity_is_active(e2));

	cf_entity_activate(e1);
	cf_run_systems();

	dummy = (DummyComponent*)cf_entity_get_component(e0, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 5);
	dummy = (DummyComponent*)cf_entity_get_component(e1, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 3);
	dummy = (DummyComponent*)cf_entity_get_component(e2, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 4);
	CF_TEST_ASSERT(cf_entity_is_active(e0));
	CF_TEST_ASSERT(cf_entity_is_active(e1));
	CF_TEST_ASSERT(cf_entity_is_active(e2));

	cf_destroy_app();

	return 0;
}

struct DummyComponent2
{
	int number = 10;
};

void dummy2_initialize(CF_Entity entity, void* component, void* udata)
{
	DummyComponent2* dummy = (DummyComponent2*)component;
	CF_PLACEMENT_NEW(dummy) DummyComponent2;
	(*(int*)udata)++;
}

CF_TEST_CASE(test_ecs_change_entity_type, "Change an entity from one type to another.");
int test_ecs_change_entity_type()
{
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, APP_OPTIONS_HIDDEN, NULL))) {
		return -1;
	}

	cf_component_begin();
	cf_component_set_name("DummyComponent");
	cf_component_set_size(sizeof(DummyComponent));
	cf_component_set_optional_initializer(dummy_initialize, NULL);
	cf_component_end();

	int dummy2_init_count = 0;

	cf_component_begin();
	cf_component_set_name("DummyComponent2");
	cf_component_set_size(sizeof(DummyComponent2));
	cf_component_set_optional_initializer(dummy2_initialize, (void*)&dummy2_init_count);
	cf_component_end();

	cf_entity_begin();
	cf_entity_set_name("Dummy_Entity");
	cf_entity_add_component("DummyComponent");
	cf_entity_end();

	cf_entity_begin();
	cf_entity_set_name("Dummy_Entity2");
	cf_entity_add_component("DummyComponent");
	cf_entity_add_component("DummyComponent2");
	cf_entity_end();

	cf_entity_begin();
	cf_entity_set_name("Dummy_Entity3");
	cf_entity_add_component("DummyComponent2");
	cf_entity_end();

	CF_Entity e = cf_make_entity("Dummy_Entity");
	DummyComponent* dummy = (DummyComponent*)cf_entity_get_component(e, "DummyComponent");
	CF_TEST_ASSERT(dummy->iters == 0);
	CF_TEST_ASSERT(dummy2_init_count == 0);

	cf_entity_change_type(e, "Dummy_Entity2");
	DummyComponent2* dummy2 = (DummyComponent2*)cf_entity_get_component(e, "DummyComponent2");
	CF_TEST_ASSERT(dummy2->number == 10);
	CF_TEST_ASSERT(dummy2_init_count == 1);

	cf_entity_change_type(e, "Dummy_Entity");
	dummy2 = (DummyComponent2*)cf_entity_get_component(e, "DummyComponent2");
	CF_TEST_ASSERT(dummy2 == NULL);

	cf_entity_change_type(e, "Dummy_Entity3");
	dummy2 = (DummyComponent2*)cf_entity_get_component(e, "DummyComponent2");
	CF_TEST_ASSERT(dummy2->number == 10);
	CF_TEST_ASSERT(dummy2_init_count == 2);
	dummy = (DummyComponent*)cf_entity_get_component(e, "DummyComponent");
	CF_TEST_ASSERT(dummy == NULL);

	cf_destroy_app();

	return 0;
}
