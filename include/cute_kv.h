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

/**
 * "Key-value", or kv. Used for serialization either to/from an in-memory buffer. The design of the
 * kv api is supposed to allow for *mostly* (but not completely) the same code to be used for both
 * serialization and deserialization.
 */
typedef struct cf_kv_t cf_kv_t;

CUTE_API cf_kv_t* CUTE_CALL cf_kv_make(void* user_allocator_context /*= NULL*/);
CUTE_API void CUTE_CALL cf_kv_destroy(cf_kv_t* kv);

typedef enum cf_kv_state_t
{
	CF_KV_STATE_UNITIALIZED,
	CF_KV_STATE_WRITE,
	CF_KV_STATE_READ,
} cf_kv_state_t;

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

typedef enum cf_kv_type_t
{
	CF_KV_TYPE_NULL   = 0,
	CF_KV_TYPE_INT64  = 1,
	CF_KV_TYPE_DOUBLE = 2,
	CF_KV_TYPE_STRING = 3,
	CF_KV_TYPE_ARRAY  = 4,
	CF_KV_TYPE_OBJECT = 5,
} cf_kv_type_t;

CUTE_API cf_error_t CUTE_CALL cf_kv_key(cf_kv_t* kv, const char* key, cf_kv_type_t* type /*= NULL*/);

CUTE_API cf_error_t CUTE_CALL cf_kv_val_uint8(cf_kv_t* kv, uint8_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val_uint16(cf_kv_t* kv, uint16_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val_uint32(cf_kv_t* kv, uint32_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val_uint64(cf_kv_t* kv, uint64_t* val);

CUTE_API cf_error_t CUTE_CALL cf_kv_val_int8(cf_kv_t* kv, int8_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val_int16(cf_kv_t* kv, int16_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val_int32(cf_kv_t* kv, int32_t* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val_int64(cf_kv_t* kv, int64_t* val);

CUTE_API cf_error_t CUTE_CALL cf_kv_val_float(cf_kv_t* kv, float* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val_double(cf_kv_t* kv, double* val);
CUTE_API cf_error_t CUTE_CALL cf_kv_val_bool(cf_kv_t* kv, bool* val);

CUTE_API cf_error_t CUTE_CALL cf_kv_val_string(cf_kv_t* kv, const char** str, size_t* size);
CUTE_API cf_error_t CUTE_CALL cf_kv_val_blob(cf_kv_t* kv, void* data, size_t data_capacity, size_t* data_len);

CUTE_API cf_error_t CUTE_CALL cf_kv_object_begin(cf_kv_t* kv, const char* key /*= NULL*/);
CUTE_API cf_error_t CUTE_CALL cf_kv_object_end(cf_kv_t* kv);

CUTE_API cf_error_t CUTE_CALL cf_kv_array_begin(cf_kv_t* kv, int* count, const char* key /*= NULL*/);
CUTE_API cf_error_t CUTE_CALL cf_kv_array_end(cf_kv_t* kv);

CUTE_API void CUTE_CALL cf_kv_print(cf_kv_t* kv);

#ifdef CUTE_CPP

namespace cute
{
using kv_t = cf_kv_t;
using kv_state_t = cf_kv_state_t;
using kv_type_t = cf_kv_type_t;

CUTE_INLINE kv_t* kv_make(void* user_allocator_context = NULL) { return cf_kv_make(user_allocator_context); }
CUTE_INLINE void kv_destroy(kv_t* kv) { cf_kv_destroy(kv); }
CUTE_INLINE kv_state_t kv_get_state(kv_t* kv) { return cf_kv_get_state(kv); }
CUTE_INLINE error_t kv_parse(kv_t* kv, const void* data, size_t size) { return cf_kv_parse(kv,data,size); }
CUTE_INLINE void kv_reset_read_state(kv_t* kv) { cf_kv_reset_read_state(kv); }
CUTE_INLINE void kv_write_mode(kv_t* kv) { cf_kv_write_mode(kv); }
CUTE_INLINE void* kv_get_buffer(kv_t* kv) { return cf_kv_get_buffer(kv); }
CUTE_INLINE size_t kv_size_written(kv_t* kv) { return cf_kv_size_written(kv); }
CUTE_INLINE void kv_nul_terminate(kv_t* kv) { cf_kv_nul_terminate(kv); }
CUTE_INLINE void kv_set_base(kv_t* kv, kv_t* base) { cf_kv_set_base(kv,base); }
CUTE_INLINE error_t kv_error_state(kv_t* kv) { return cf_kv_error_state(kv); }
CUTE_INLINE error_t kv_key(kv_t* kv, const char* key, kv_type_t* type = NULL) { return cf_kv_key(kv,key,type); }
CUTE_INLINE error_t kv_val(kv_t* kv, uint8_t* val) { return cf_kv_val_uint8(kv,val); }
CUTE_INLINE error_t kv_val(kv_t* kv, uint16_t* val) { return cf_kv_val_uint16(kv,val); }
CUTE_INLINE error_t kv_val(kv_t* kv, uint32_t* val) { return cf_kv_val_uint32(kv,val); }
CUTE_INLINE error_t kv_val(kv_t* kv, uint64_t* val) { return cf_kv_val_uint64(kv,val); }
CUTE_INLINE error_t kv_val(kv_t* kv, int8_t* val) { return cf_kv_val_int8(kv,val); }
CUTE_INLINE error_t kv_val(kv_t* kv, int16_t* val) { return cf_kv_val_int16(kv,val); }
CUTE_INLINE error_t kv_val(kv_t* kv, int32_t* val) { return cf_kv_val_int32(kv,val); }
CUTE_INLINE error_t kv_val(kv_t* kv, int64_t* val) { return cf_kv_val_int64(kv,val); }
CUTE_INLINE error_t kv_val(kv_t* kv, float* val) { return cf_kv_val_float(kv,val); }
CUTE_INLINE error_t kv_val(kv_t* kv, double* val) { return cf_kv_val_double(kv,val); }
CUTE_INLINE error_t kv_val(kv_t* kv, bool* val) { return cf_kv_val_bool(kv,val); }
CUTE_INLINE error_t kv_val_string(kv_t* kv, const char** str, size_t* size) { return cf_kv_val_string(kv,str,size); }
CUTE_INLINE error_t kv_val_blob(kv_t* kv, void* data, size_t data_capacity, size_t* data_len) { return cf_kv_val_blob(kv,data,data_capacity,data_len); }
CUTE_INLINE error_t kv_object_begin(kv_t* kv, const char* key = NULL) { return cf_kv_object_begin(kv,key); }
CUTE_INLINE error_t kv_object_end(kv_t* kv) { return cf_kv_object_end(kv); }
CUTE_INLINE error_t kv_array_begin(kv_t* kv, int* count, const char* key = NULL) { return cf_kv_array_begin(kv,count,key); }
CUTE_INLINE error_t kv_array_end(kv_t* kv) { return cf_kv_array_end(kv); }
CUTE_INLINE void kv_print(kv_t* kv) { cf_kv_print(kv); }

}

#endif // CUTE_CPP

#endif // CUTE_KV_H
