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

#ifndef CUTE_KV_H
#define CUTE_KV_H

#include <cute_defines.h>
#include <cute_error.h>

namespace cute
{

/**
 * "Key-value", or kv. Used for serialization either to/from an in-memory buffer. The design of the
 * kv api is supposed to allow for *mostly* (but not completely) the same code to be used for both
 * serialization and deserialization.
 */
struct kv_t;

CUTE_API kv_t* CUTE_CALL kv_make(void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL kv_destroy(kv_t* kv);

enum kv_state_t
{
	KV_STATE_UNITIALIZED,
	KV_STATE_WRITE,
	KV_STATE_READ,
};

CUTE_API kv_state_t CUTE_CALL kv_get_state(kv_t* kv);

/**
 * Parses the text at `data` in a single-pass. Sets the `kv` to read mode `KV_STATE_READ`.
 */
CUTE_API error_t CUTE_CALL kv_parse(kv_t* kv, const void* data, size_t size);

/**
 * Clears the `kv`'s internal state, but retains all previously parsed data. This can be useful
 * to quickly reset the `kv` at any point, especially while in the middle of reading objects/arrays.
 */
CUTE_API void CUTE_CALL kv_reset_read_state(kv_t* kv);

/**
 * Sets the `kv` to write mode `KV_STATE_WRITE`, ready to serialize data to `buffer`.
 */
CUTE_API void CUTE_CALL kv_set_write_buffer(kv_t* kv, void* buffer, size_t size);

/**
 * The base must be in read mode. This function is used to support data inheritence and delta encoding.
 * 
 * Data Inheritence
 * 
 *     If a kv is in read mode any value missing from a kv will be fetched recursively from the base.
 * 
 * Delta Encoding
 * 
 *     If the kv is in write mode any value will first be recursively looked up in base. If found, it
 *     is only written if the new value is different from the value to be written.
 */
CUTE_API void CUTE_CALL kv_set_base(kv_t* kv, kv_t* base);

/**
 * Returns the size written to the buffer from `kv_set_write_buffer` so far.
 */
CUTE_API size_t CUTE_CALL kv_size_written(kv_t* kv);

/**
 * Returns the error state of the kv instance.
 */
CUTE_API error_t CUTE_CALL kv_error_state(kv_t* kv);

// -------------------------------------------------------------------------------------------------
// Key and Value functions.
// The pattern is to call `kv_key` first and then `kv_val` for the corresponding value.
// The behavior is different if the kv is set to read or write mode.
// For read mode, if a key is found and the correct type of kv_val function is called, the value is
// loaded into the supplied `val` pointer.
// For write mode keys and values are written to the buffer set with `kv_set_write_bffer`.

enum kv_type_t
{
	KV_TYPE_NULL   = 0,
	KV_TYPE_INT64  = 1,
	KV_TYPE_DOUBLE = 2,
	KV_TYPE_STRING = 3,
	KV_TYPE_ARRAY  = 4,
	KV_TYPE_OBJECT = 5,
};

CUTE_API error_t CUTE_CALL kv_key(kv_t* kv, const char* key, kv_type_t* type = NULL);

CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, uint8_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, uint16_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, uint32_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, uint64_t* val);

CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, int8_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, int16_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, int32_t* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, int64_t* val);

CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, float* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, double* val);
CUTE_API error_t CUTE_CALL kv_val(kv_t* kv, bool* val);

CUTE_API error_t CUTE_CALL kv_val_string(kv_t* kv, const char** str, size_t* size);
CUTE_API error_t CUTE_CALL kv_val_blob(kv_t* kv, void* data, size_t data_capacity, size_t* data_len);

CUTE_API error_t CUTE_CALL kv_object_begin(kv_t* kv, const char* key = NULL);
CUTE_API error_t CUTE_CALL kv_object_end(kv_t* kv);

CUTE_API error_t CUTE_CALL kv_array_begin(kv_t* kv, int* count, const char* key = NULL);
CUTE_API error_t CUTE_CALL kv_array_end(kv_t* kv);

CUTE_API void CUTE_CALL kv_print(kv_t* kv);

}

#endif // CUTE_KV_H
