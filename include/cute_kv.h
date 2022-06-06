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

#include "cute_defines.h"
#include "cute_error.h"

namespace cute
{

/**
 * "Key-value", or kv. Used for serialization either to/from an in-memory buffer. The design of the
 * kv api is supposed to allow for *mostly* (but not completely) the same code to be used for both
 * serialization and deserialization.
 */
struct cf_kv_t;

CUTE_API cf_kv_t* CUTE_CALL cf_kv_make(void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL cf_kv_destroy(cf_kv_t* kv);

enum cf_kv_state_t
{
	CF_KV_STATE_UNITIALIZED,
	CF_KV_STATE_WRITE,
	CF_KV_STATE_READ,
};

CUTE_API cf_kv_state_t CUTE_CALL cf_kv_get_state(cf_kv_t* kv);

/**
 * Parses the text at `data` in a single-pass. Sets the `kv` to read mode `KV_STATE_READ`.
 */
CUTE_API cf_error_t CUTE_CALL cf_kv_parse(cf_kv_t* kv, const void* data, size_t size);

/**
 * Clears the `kv`'s internal state, but retains all previously parsed data. This can be useful
 * to quickly reset the `kv` at any point, especially while in the middle of reading objects/arrays.
 */
CUTE_API void CUTE_CALL cf_kv_reset_read_state(cf_kv_t* kv);

/**
 * Sets the `kv` to write mode `KV_STATE_WRITE`. Data will be serialized and written to an internal
 * write buffer.
 */
CUTE_API void CUTE_CALL cf_kv_write_mode(cf_kv_t* kv);

/**
 * Fetches the write buffer pointer containing any data serialized so far.
 */
CUTE_API void* CUTE_CALL cf_kv_get_buffer(cf_kv_t* kv);

/**
 * Returns the size written to the write buffer so far.
 */
CUTE_API size_t CUTE_CALL cf_kv_size_written(cf_kv_t* kv);

/**
 * If the `kv` is in write mode `KV_STATE_WRITE` a nul-terminator '\0' is added to the end of the
 * buffer. This function is for convenience, to be called when serializing is done, and the buffer is
 * ready to be treated as a nul-terminated c-string.
 */
CUTE_API void CUTE_CALL cf_kv_nul_terminate(cf_kv_t* kv);

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
CUTE_API void CUTE_CALL cf_kv_set_base(cf_kv_t* kv, cf_kv_t* base);

/**
 * Returns the error state of the kv instance.
 */
CUTE_API cf_error_t CUTE_CALL cf_kv_error_state(cf_kv_t* kv);

// -------------------------------------------------------------------------------------------------
// Key and Value functions.
// The pattern is to call `kv_key` first and then `kv_val` for the corresponding value.
// The behavior is different if the kv is set to read or write mode.
// For read mode, if a key is found and the correct type of cf_kv_val function is called, the value is
// loaded into the supplied `val` pointer.
// For write mode keys and values are written to the buffer set with `kv_set_write_bffer`.

enum cf_kv_type_t
{
	CF_KV_TYPE_NULL   = 0,
	CF_KV_TYPE_INT64  = 1,
	CF_KV_TYPE_DOUBLE = 2,
	CF_KV_TYPE_STRING = 3,
	CF_KV_TYPE_ARRAY  = 4,
	CF_KV_TYPE_OBJECT = 5,
};

CUTE_API cf_error_t CUTE_CALL cf_kv_key(cf_kv_t* kv, const char* key, cf_kv_type_t* type = NULL);

CUTE_API cf_error_t CUTE_CALL cf_kv_val(cf_kv_t* kv, uint8_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val(cf_kv_t* kv, uint16_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val(cf_kv_t* kv, uint32_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val(cf_kv_t* kv, uint64_t* val);

CUTE_API cf_error_t CUTE_CALL cf_kv_val(cf_kv_t* kv, int8_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val(cf_kv_t* kv, int16_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val(cf_kv_t* kv, int32_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val(cf_kv_t* kv, int64_t* val);

CUTE_API cf_error_t CUTE_CALL cf_kv_val(cf_kv_t* kv, float* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val(cf_kv_t* kv, double* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val(cf_kv_t* kv, bool* val);

CUTE_API cf_error_t CUTE_CALL cf_kv_val_string(cf_kv_t* kv, const char** str, size_t* size);
CUTE_API cf_error_t CUTE_CALL cf_kv_val_blob(cf_kv_t* kv, void* data, size_t data_capacity, size_t* data_len);

CUTE_API cf_error_t CUTE_CALL cf_kv_object_begin(cf_kv_t* kv, const char* key = NULL);
CUTE_API cf_error_t CUTE_CALL cf_kv_object_end(cf_kv_t* kv);

CUTE_API cf_error_t CUTE_CALL cf_kv_array_begin(cf_kv_t* kv, int* count, const char* key = NULL);
CUTE_API cf_error_t CUTE_CALL cf_kv_array_end(cf_kv_t* kv);

CUTE_API void CUTE_CALL cf_kv_print(cf_kv_t* kv);

}

#endif // CUTE_KV_H
