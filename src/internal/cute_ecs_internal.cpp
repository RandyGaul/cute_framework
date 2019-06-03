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

#include <cute_alloc.h>
#include <cute_c_runtime.h>

#include <internal/cute_ecs_internal.h>

namespace cute
{

struct ecs_allocator_t
{
	int max_objects;
	handle_table_t table;
	int object_size;
	int object_count;
	int object_capacity;
	void* objects;
	void* mem_ctx;
};

static void s_ensure_capacity(ecs_allocator_t* ecs_alloc, int num_objects)
{
	if (num_objects > ecs_alloc->object_capacity) {
		int new_capacity = ecs_alloc->object_capacity ? ecs_alloc->object_capacity * 2 : 256;
		while (new_capacity < num_objects)
		{
			new_capacity <<= 1;
			CUTE_ASSERT(new_capacity); // Detect overflow.
		}

		size_t new_size = ecs_alloc->object_size * new_capacity;
		void* new_items = CUTE_ALLOC(new_size, ecs_alloc->mem_ctx);
		CUTE_ASSERT(new_items);
		CUTE_MEMCPY(new_items, ecs_alloc->objects, ecs_alloc->object_size * ecs_alloc->object_count);
		CUTE_FREE(ecs_alloc->objects, ecs_alloc->mem_ctx);
		ecs_alloc->objects = new_items;
		ecs_alloc->object_capacity = new_capacity;
	}
}

static void* s_get(ecs_allocator_t* ecs_alloc, int index)
{
	CUTE_ASSERT(index >= 0 && index < ecs_alloc->object_size);
	return (void*)((uintptr_t)ecs_alloc->objects + index * ecs_alloc->object_size);
}

static void* s_add(ecs_allocator_t* ecs_alloc, const void* object)
{
	s_ensure_capacity(ecs_alloc, ecs_alloc->object_count + 1);
	return s_get(ecs_alloc, ecs_alloc->object_count++);
}

static void* s_unordered_remove(ecs_allocator_t* ecs_alloc, int index)
{
	void* slot = s_get(ecs_alloc, index);
	const void* last = s_get(ecs_alloc, --ecs_alloc->object_count);
	CUTE_MEMCPY(slot, last, ecs_alloc->object_size);
	return slot;
}

ecs_allocator_t* ecs_allocator_make(int object_size, int max_objects, int reserve_count, void* user_allocator_context)
{
	ecs_allocator_t* ecs_alloc = (ecs_allocator_t*)CUTE_ALLOC(sizeof(ecs_allocator_t), user_allocator_context);
	ecs_alloc->max_objects = max_objects;
	handle_table_init(&ecs_alloc->table, max_objects, user_allocator_context);
	ecs_alloc->object_size = object_size;
	ecs_alloc->object_count = 0;
	ecs_alloc->object_capacity = 0;
	ecs_alloc->objects = NULL;
	ecs_alloc->mem_ctx = user_allocator_context;
	s_ensure_capacity(ecs_alloc, reserve_count);
	return NULL;
}

void ecs_allocator_destroy(ecs_allocator_t* ecs_alloc)
{
	CUTE_FREE(ecs_alloc->objects, ecs_alloc->mem_ctx);
	CUTE_FREE(ecs_alloc, ecs_alloc->mem_ctx);
}

handle_t ecs_allocator_allocate(ecs_allocator_t* ecs_alloc, const void* object)
{
	int index = ecs_alloc->object_count;
	void* slot = s_add(ecs_alloc, object);
	CUTE_MEMCPY(slot, object, ecs_alloc->object_size);
	handle_t handle = handle_table_alloc(&ecs_alloc->table, index);
	CUTE_ASSERT(handle != CUTE_INVALID_HANDLE);
	return handle;
}

error_t ecs_allocator_get_object(ecs_allocator_t* ecs_alloc, handle_t id, void* object)
{
	if (!handle_table_is_valid(&ecs_alloc->table, id)) {
		return error_failure("Tried to get object with invalid handle.");
	} else {
		int index = (int)handle_table_get_index(&ecs_alloc->table, id);
		const void* slot = s_get(ecs_alloc, index);
		CUTE_MEMCPY(object, slot, ecs_alloc->object_size);
		return error_success();
	}
}

void* ecs_allocator_remove_object(ecs_allocator_t* ecs_alloc, handle_t id, int* moved_index)
{
	if (handle_table_is_valid(&ecs_alloc->table, id)) {
		handle_table_free(&ecs_alloc->table, id);
		int index = (int)handle_table_get_index(&ecs_alloc->table, id);
		if (moved_index) *moved_index = index;
		return s_unordered_remove(ecs_alloc, index);
	} else {
		return NULL;
	}
}

void* ecs_allocator_remove_object(ecs_allocator_t* ecs_alloc, int index)
{
	void* object = s_unordered_remove(ecs_alloc, index);
	return object;
}

bool ecs_allocator_has_object(ecs_allocator_t* ecs_alloc, handle_t id)
{
	return handle_table_is_valid(&ecs_alloc->table, id) ? true : false;
}

void ecs_allocator_update_handle(ecs_allocator_t* ecs_alloc, handle_t moved_handle, int moved_index)
{
	handle_table_update_index(&ecs_alloc->table, moved_handle, moved_index);
}

void* ecs_allocator_get_objects(ecs_allocator_t* ecs_alloc)
{
	return ecs_alloc->objects;
}

int ecs_allocator_get_object_count(ecs_allocator_t* ecs_alloc)
{
	return ecs_alloc->object_count;
}

}
