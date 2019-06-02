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

#include <internal/cute_ecs_internal.h>

namespace cute
{

ecs_allocator_t* ecs_allocator_make(int object_size, int max_objects, int reserve_count, void* user_allocator_context)
{
	return NULL;
}

void ecs_allocator_destroy(ecs_allocator_t* ecs_alloc)
{
}

handle_t ecs_allocator_allocate(ecs_allocator_t* ecs_alloc, const void* object)
{
	return CUTE_INVALID_HANDLE;
}

error_t ecs_allocator_get_object(ecs_allocator_t* ecs_alloc, handle_t id, void* object)
{
	return error_failure(NULL);
}

void ecs_allocator_remove_object(ecs_allocator_t* ecs_alloc, handle_t id)
{
}

void ecs_allocator_remove_object(ecs_allocator_t* ecs_alloc, int index)
{
}

bool ecs_allocator_has_object(ecs_allocator_t* ecs_alloc, handle_t id)
{
	return false;
}

void* ecs_allocator_get_objects(ecs_allocator_t* ecs_alloc)
{
	return NULL;
}

int ecs_allocator_get_object_count(ecs_allocator_t* ecs_alloc)
{
	return 0;
}

}
