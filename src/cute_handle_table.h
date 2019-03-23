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

#ifndef CUTE_HANDLE_TABLE_H
#define CUTE_HANDLE_TABLE_H

#include <cute_defines.h>

namespace cute
{

using handle_t = uint64_t;
#define CUTE_INVALID_HANDLE (~0ULL)

#define CUTE_HANDLE_INDEX(handle) (handle & 0xFFFF)

struct handle_table_t
{
	int size = 0;
	int capacity = 0;
	uint16_t freelist = 0;
	handle_t* handles = NULL;
	void* mem_ctx = NULL;
};

extern CUTE_API int CUTE_CALL handle_table_init(handle_table_t* table, int initial_capacity, void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL handle_table_clean_up(handle_table_t* table);

extern CUTE_API handle_t CUTE_CALL handle_table_alloc(handle_table_t* table, uint16_t index);
extern CUTE_API handle_t CUTE_CALL handle_table_update_index(handle_table_t* table, handle_t handle, uint16_t index);
extern CUTE_API void CUTE_CALL handle_table_free(handle_table_t* table, handle_t handle);

}

#endif // CUTE_HANDLE_TABLE_H
