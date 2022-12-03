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

#include "cute_defines.h"
#include "cute_concurrency.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CF_CircularBuffer
{
	int index0;
	int index1;
	CF_AtomicInt size_left;
	int capacity;
	uint8_t* data;
} CF_CircularBuffer;

CUTE_API CF_CircularBuffer CUTE_CALL cf_make_circular_buffer(int initial_size_in_bytes /*= NULL*/);
CUTE_API void CUTE_CALL cf_destroy_circular_buffer(CF_CircularBuffer* buffer);

CUTE_API void CUTE_CALL cf_circular_buffer_reset(CF_CircularBuffer* buffer);

CUTE_API int CUTE_CALL cf_circular_buffer_push(CF_CircularBuffer* buffer, const void* data, int size);
CUTE_API int CUTE_CALL cf_circular_buffer_pull(CF_CircularBuffer* buffer, void* data, int size);

CUTE_API int CUTE_CALL cf_circular_buffer_grow(CF_CircularBuffer* buffer, int new_size_in_bytes);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using CircularBuffer = CF_CircularBuffer;

CUTE_INLINE CircularBuffer make_circular_buffer(int initial_size_in_bytes = NULL) { return cf_make_circular_buffer(initial_size_in_bytes); }
CUTE_INLINE void destroy_circular_buffer(CircularBuffer* buffer) { cf_destroy_circular_buffer(buffer); }

CUTE_INLINE void circular_buffer_reset(CircularBuffer* buffer) { cf_circular_buffer_reset(buffer); }

CUTE_INLINE int circular_buffer_push(CircularBuffer* buffer, const void* data, int size) { return cf_circular_buffer_push(buffer, data, size); }
CUTE_INLINE int circular_buffer_pull(CircularBuffer* buffer, void* data, int size) { return cf_circular_buffer_pull(buffer, data, size); }

CUTE_INLINE int circular_buffer_grow(CircularBuffer* buffer, int new_size_in_bytes) { return cf_circular_buffer_grow(buffer, new_size_in_bytes); }

}

#endif // CUTE_CPP

#endif // CUTE_CIRCULAR_BUFFER_H
