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

#ifndef CUTE_CIRCULAR_BUFFER_H
#define CUTE_CIRCULAR_BUFFER_H

#include <cute_defines.h>
#include <cute_concurrency.h>

namespace cute
{

struct circular_buffer_t
{
	int index0 = 0;
	int index1 = 0;
	atomic_int_t size_left = atomic_zero();
	int capacity = 0;
	uint8_t* data = NULL;
	void* user_allocator_context = NULL;
};

CUTE_API circular_buffer_t CUTE_CALL circular_buffer_make(int initial_size_in_bytes, void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL circular_buffer_free(circular_buffer_t* buffer);

CUTE_API void CUTE_CALL circular_buffer_reset(circular_buffer_t* buffer);

CUTE_API int CUTE_CALL circular_buffer_push(circular_buffer_t* buffer, const void* data, int size);
CUTE_API int CUTE_CALL circular_buffer_pull(circular_buffer_t* buffer, void* data, int size);

CUTE_API int CUTE_CALL circular_buffer_grow(circular_buffer_t* buffer, int new_size_in_bytes);

}

#endif // CUTE_CIRCULAR_BUFFER_H
