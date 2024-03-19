/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_CIRCULAR_BUFFER_H
#define CF_CIRCULAR_BUFFER_H

#include "cute_defines.h"
#include "cute_multithreading.h"

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

CF_API CF_CircularBuffer CF_CALL cf_make_circular_buffer(int initial_size_in_bytes /*= NULL*/);
CF_API void CF_CALL cf_destroy_circular_buffer(CF_CircularBuffer* buffer);

CF_API void CF_CALL cf_circular_buffer_reset(CF_CircularBuffer* buffer);

CF_API int CF_CALL cf_circular_buffer_push(CF_CircularBuffer* buffer, const void* data, int size);
CF_API int CF_CALL cf_circular_buffer_pull(CF_CircularBuffer* buffer, void* data, int size);

CF_API int CF_CALL cf_circular_buffer_grow(CF_CircularBuffer* buffer, int new_size_in_bytes);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using CircularBuffer = CF_CircularBuffer;

CF_INLINE CircularBuffer make_circular_buffer(int initial_size_in_bytes) { return cf_make_circular_buffer(initial_size_in_bytes); }
CF_INLINE void destroy_circular_buffer(CircularBuffer* buffer) { cf_destroy_circular_buffer(buffer); }

CF_INLINE void circular_buffer_reset(CircularBuffer* buffer) { cf_circular_buffer_reset(buffer); }

CF_INLINE int circular_buffer_push(CircularBuffer* buffer, const void* data, int size) { return cf_circular_buffer_push(buffer, data, size); }
CF_INLINE int circular_buffer_pull(CircularBuffer* buffer, void* data, int size) { return cf_circular_buffer_pull(buffer, data, size); }

CF_INLINE int circular_buffer_grow(CircularBuffer* buffer, int new_size_in_bytes) { return cf_circular_buffer_grow(buffer, new_size_in_bytes); }

}

#endif // CF_CPP

#endif // CF_CIRCULAR_BUFFER_H
