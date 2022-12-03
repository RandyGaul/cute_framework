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
#include "cute_result.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * "Key-value", or KV for short, is a serialization API designed to be very similar to JSON. It's a
 * text-based format to store any kind of data in key-value format. This key-value style has some
 * great advantages for serialziation, such as versioning your data, simplicity (human readable),
 * and a low-friction API.
 * 
 * You start by opening a KV in either read or write format. For 95% of use cases you can then get
 * away with using the same code for reading and writing data, with only the difference of opening
 * in read/write mode with `cf_kv_read`/`cf_kv_write`.
 * 
 * Here's a quick example of some KV data.
 * 
 *     size = 10,
 *     name = "Clarence",
 *     data = {
 *         x = 5,
 *         z = 15.0,
 *     },
 *     blob = "SGVsbG8gdGhlcmUgOik=",
 *     codes = [3] {
 *         7, -3, 20,
 *     }
 * 
 * Here are the supported data types in KV:
 * 
 *     - integers
 *     - floats
 *     - strings
 *     - base64 blobs, as strings
 *     - arrays
 *     - objects
 * 
 * Reading data with KV is all about calling `cf_kv_key(kv, "key_name")` to lookup a serialized
 * field. If found, you can call the proper `kv_val_***` function to fetch the data for that key.
 * For example, let's say we have this struct in our game containing some important info.
 * 
 *     struct ImportantStuff
 *     {
 *         int state;
 *         float factor;
 *         int value_count;
 *         int values[MAX_ENTRIES];
 *     };
 * 
 * We could save the `ImportantStuff` to a KV text file with some code like this:
 * 
 *     bool serialize(CF_KeyValue* kv, ImportantStuff* stuff)
 *     {
 *         if (cf_kv_key("state"))       cf_kv_val_int32(kv, &stuff->state);
 *         if (cf_kv_key("factor"))      cf_kv_val_float(kv, &stuff->factor);
 *         if (cf_kv_key("value_count")) cf_kv_val_int32(kv, &stuff->value_count);
 * 
 *         if (cf_kv_object_begin(kv, &stuff->value_count, "values")) {
 *             for (int i = 0; i < stuff->value_count; ++i) {
 *                 cf_kv_val_int32(kv, stuff->values + i);
 *             }
 *             cf_kv_object_end(kv);
 *         }
 *         
 *         return !cf_is_error(cf_kv_last_error(kv));
 *     }
 * 
 *     bool save(ImportantStuff* stuff, const char* path)
 *     {
 *          CF_KeyValue* kv = kv_write();
 *          if (!serialize(kv, stuff)) {
 *              cf_kv_destroy(kv);
 *              return NULL;
 *          }
 *          bool result = !cf_is_error(cf_fs_write_entire_buffer_to_file(cf_kv_buffer(kv), cf_kv_buffer_size(kv)));
 *          cf_kv_destroy(kv);
 *          return result;
 *     }
 * 
 * The text file would then look something like this:
 * 
 *     state = 10
 *     factor = 1.3
 *     value_count = 3
 *     values = [3] {
 *         7, -3, 20,
 *     }
 * 
 * The bulk of the work happens in the `serialize` function from the example above. We can
 * reuse this entire function to also read back the KV data into an `ImportantStuff` struct
 * by making a similar function to `save`.
 * 
 *     bool load(ImportantStuff* stuff, const char* path)
 *     {
 *          void* data;
 *          size_t size;
 *          if (cf_is_error(cf_fs_read_entire_file_to_memory(path, &data, &size))) return false;
 *          CF_KeyValue* kv = kv_read(data, size);
 *          if (!kv) return false;
 *          bool result = serialize(kv, stuff);
 *          cf_kv_destroy(kv);
 *          return result;
 *     }
 * 
 * In the common case it's possible to reuse most seriaization code for both reading and
 * writing. However, sometimes it's necessary to have entirely different code for reading
 * and writing. Use `cf_kv_state` to see if the KV is currently in read vs write mode, and then
 * run different code accordingly.
 */

typedef struct CF_KeyValue CF_KeyValue;

#define CF_KV_STATE_DEFS \
	CF_ENUM(KV_STATE_WRITE, 0) \
	CF_ENUM(KV_STATE_READ, 1) \

typedef enum CF_KeyValueState
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_KV_STATE_DEFS
	#undef CF_ENUM
} CF_KeyValueState;

CUTE_INLINE const char* cf_key_value_state_to_string(CF_KeyValueState state)
{
	switch (state) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_KV_STATE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * Parses a buffer of kv data. All data is loaded up into memory at once. You can fetch out values
 * as-needed by using `cf_kv_key` and `cf_kv_val_***` functions.
 * 
 * Example:
 * 
 *     const char* data =
 *         "a = 10,\n"
 *         "b = 13,\n"
 *     ;
 *     size_t len = CUTE_STRLEN(string);
 *     
 *     CF_KeyValue* kv = cf_kv_parse((void*)string, len, NULL);
 *     
 *     int val;
 *     if (cf_kv_key(kv, "a")) {
 *         cf_kv_val(kv, &val);
 *         printf("a was %d\n", val);
 *     }
 *     if (cf_kv_key(kv, "b")) {
 *         cf_kv_val(kv, &val);
 *         printf("b was %d\n", val);
 *     }
 *     
 *     cf_kv_destroy(kv);
 * 
 * Which prints:
 * 
 *     a was 10
 *     b was 13
 */
CUTE_API CF_KeyValue* CUTE_CALL cf_kv_read(const void* data, size_t size, CF_Result* result_out /* = NULL */);

/**
 * Creates a new kv ready for writing. You can write values as-needed by using `cf_kv_key` and
 * `cf_kv_val***` functions.
 * 
 * Example:
 * 
 *     int a = 10;
 *     int b = 12;
 *     
 *     CF_KeyValue* kv = kv_write();
 *     cf_kv_key(kv, "a");
 *     cf_kv_val(kv, &a);
 *     
 *     cf_kv_key(kv, "b");
 *     cf_kv_val(kv, &b);
 * 
 *     printf("%s", cf_kv_buffer());
 *     cf_kv_destroy(kv);
 * 
 * Which prints:
 * 
 *     a = 10,
 *     b = 12,
 */
CUTE_API CF_KeyValue* CUTE_CALL cf_kv_write();

/**
 * Frees up all resources using by the kv instance.
 */
CUTE_API void CUTE_CALL cf_kv_destroy(CF_KeyValue* kv);

/**
 * Returns the mode this KV was opened in. Either KV_STATE_READ or KV_STATE_WRITE. You can use this
 * function in your serialization routines to do specific things for reading vs writing, whereas for
 * most cases you can use the same code for both reading and writing.
 */
CUTE_API CF_KeyValueState CUTE_CALL cf_kv_state(CF_KeyValue* kv);

/**
 * Fetches the write buffer pointer containing any data serialized so far. This function will return
 * NULL if the kv is not opened for write mode.
 */
CUTE_API const char* CUTE_CALL cf_kv_buffer(CF_KeyValue* kv);

/**
 * Returns the size written to the write buffer so far. This size does not include the nul-terminator.
 */
CUTE_API size_t CUTE_CALL cf_kv_buffer_size(CF_KeyValue* kv);

/**
 * Resets the read position of `kv`. `kv` must be in read mode to use this function. After calling
 * `cf_kv_key` and various combinations of `cf_kv_object_begin` or `cf_kv_array_begin` you might want
 * to stop reading and reset back to the top-level object in your serialized file. This function does
 * not do any re-parsing, and merely clears/resets a few internal variables.
 */
CUTE_API void CUTE_CALL cf_read_reset(CF_KeyValue* kv);

/**
 * The base must be in read mode. This function is used to support data inheritence and delta encoding.
 * Depending on whether `kv` is in read or write mode this function behaves very differently. Read mode
 * is for data inheritence, while write mode is for delta encoding.
 * 
 * Data Inheritence
 * 
 *     If `kv` is in read mode any value missing will be fetched recursively from `base`.
 * 
 * Delta Encoding
 * 
 *     If `kv` is in write mode any value will first be recursively looked up in `base`. If found, it
 *     is only written if the new value is different from the value to be written.
 * 
 * For a more in-depth description on how to use this function see the tutotorial page from the Cute
 * Framework docs here: https://randygaul.github.io/cute_framework/#/serialization/
 */
CUTE_API void CUTE_CALL cf_kv_set_base(CF_KeyValue* kv, CF_KeyValue* base);

/**
 * Returns the error state of the kv instance. You can use this try and get a more useful description
 * of what may have went wrong. These errors are not fatal. For example if you search for a key with
 * `cf_kv_key` and it's non-existent a potentially useful error message may be generated, but you can
 * still keep going and look for other keys freely.
 */
CUTE_API CF_Result CUTE_CALL cf_kv_last_error(CF_KeyValue* kv);

#define CF_KV_TYPE_DEFS \
	CF_ENUM(KV_TYPE_NULL,   0) \
	CF_ENUM(KV_TYPE_INT64,  1) \
	CF_ENUM(KV_TYPE_DOUBLE, 2) \
	CF_ENUM(KV_TYPE_STRING, 3) \
	CF_ENUM(KV_TYPE_ARRAY,  4) \
	CF_ENUM(KV_TYPE_OBJECT, 5) \

typedef enum CF_KeyValueType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_KV_TYPE_DEFS
	#undef CF_ENUM
} CF_KeyValueType;

CUTE_INLINE const char* cf_key_value_type_to_string(CF_KeyValueType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_KV_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_API bool CUTE_CALL cf_kv_key(CF_KeyValue* kv, const char* key, CF_KeyValueType* type /*= NULL*/);

CUTE_API bool CUTE_CALL cf_kv_val_uint8(CF_KeyValue* kv, uint8_t* val);
CUTE_API bool CUTE_CALL cf_kv_val_uint16(CF_KeyValue* kv, uint16_t* val);
CUTE_API bool CUTE_CALL cf_kv_val_uint32(CF_KeyValue* kv, uint32_t* val);
CUTE_API bool CUTE_CALL cf_kv_val_uint64(CF_KeyValue* kv, uint64_t* val);

CUTE_API bool CUTE_CALL cf_kv_val_int8(CF_KeyValue* kv, int8_t* val);
CUTE_API bool CUTE_CALL cf_kv_val_int16(CF_KeyValue* kv, int16_t* val);
CUTE_API bool CUTE_CALL cf_kv_val_int32(CF_KeyValue* kv, int32_t* val);
CUTE_API bool CUTE_CALL cf_kv_val_int64(CF_KeyValue* kv, int64_t* val);

CUTE_API bool CUTE_CALL cf_kv_val_float(CF_KeyValue* kv, float* val);
CUTE_API bool CUTE_CALL cf_kv_val_double(CF_KeyValue* kv, double* val);
CUTE_API bool CUTE_CALL cf_kv_val_bool(CF_KeyValue* kv, bool* val);

CUTE_API bool CUTE_CALL cf_kv_val_string(CF_KeyValue* kv, const char** str, size_t* size);
CUTE_API bool CUTE_CALL cf_kv_val_blob(CF_KeyValue* kv, void* data, size_t data_capacity, size_t* data_len);

CUTE_API bool CUTE_CALL cf_kv_object_begin(CF_KeyValue* kv, const char* key /*= NULL*/);
CUTE_API bool CUTE_CALL cf_kv_object_end(CF_KeyValue* kv);

CUTE_API bool CUTE_CALL cf_kv_array_begin(CF_KeyValue* kv, int* count, const char* key /*= NULL*/);
CUTE_API bool CUTE_CALL cf_kv_array_end(CF_KeyValue* kv);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using KeyValue = CF_KeyValue;

using KeyValueState = CF_KeyValueState;
#define CF_ENUM(K, V) CUTE_INLINE constexpr KeyValueState K = CF_##K;
CF_KV_STATE_DEFS
#undef CF_ENUM

CUTE_INLINE const char* to_string(KeyValueState state)
{
	switch (state) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_KV_STATE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using KeyValueType = CF_KeyValueType;
#define CF_ENUM(K, V) CUTE_INLINE constexpr KeyValueType K = CF_##K;
CF_KV_TYPE_DEFS
#undef CF_ENUM

CUTE_INLINE const char* to_string(KeyValueType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_KV_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_INLINE KeyValue* kv_read(const void* data, size_t size, Result* result_out = NULL) { return cf_kv_read(data, size, result_out); }
CUTE_INLINE KeyValue* kv_write(KeyValue* kv) { return cf_kv_write(); }
CUTE_INLINE void kv_destroy(KeyValue* kv) { cf_kv_destroy(kv); }
CUTE_INLINE KeyValueState kv_state(KeyValue* kv) { return (KeyValueState)cf_kv_state(kv); }
CUTE_INLINE const char* kv_buffer(KeyValue* kv) { return cf_kv_buffer(kv); }
CUTE_INLINE size_t kv_buffer_size(KeyValue* kv) { return cf_kv_buffer_size(kv); }
CUTE_INLINE void kv_set_base(KeyValue* kv, KeyValue* base) { cf_kv_set_base(kv, base); }
CUTE_INLINE Result kv_error_state(KeyValue* kv) { return cf_kv_last_error(kv); }
CUTE_INLINE bool kv_key(KeyValue* kv, const char* key, KeyValueType* type = NULL) { return cf_kv_key(kv, key, type); }
CUTE_INLINE bool kv_val(KeyValue* kv, uint8_t* val) { return cf_kv_val_uint8(kv, val); }
CUTE_INLINE bool kv_val(KeyValue* kv, uint16_t* val) { return cf_kv_val_uint16(kv, val); }
CUTE_INLINE bool kv_val(KeyValue* kv, uint32_t* val) { return cf_kv_val_uint32(kv, val); }
CUTE_INLINE bool kv_val(KeyValue* kv, uint64_t* val) { return cf_kv_val_uint64(kv, val); }
CUTE_INLINE bool kv_val(KeyValue* kv, int8_t* val) { return cf_kv_val_int8(kv, val); }
CUTE_INLINE bool kv_val(KeyValue* kv, int16_t* val) { return cf_kv_val_int16(kv, val); }
CUTE_INLINE bool kv_val(KeyValue* kv, int32_t* val) { return cf_kv_val_int32(kv, val); }
CUTE_INLINE bool kv_val(KeyValue* kv, int64_t* val) { return cf_kv_val_int64(kv, val); }
CUTE_INLINE bool kv_val(KeyValue* kv, float* val) { return cf_kv_val_float(kv, val); }
CUTE_INLINE bool kv_val(KeyValue* kv, double* val) { return cf_kv_val_double(kv, val); }
CUTE_INLINE bool kv_val(KeyValue* kv, bool* val) { return cf_kv_val_bool(kv, val); }
CUTE_INLINE bool kv_val_string(KeyValue* kv, const char** str, size_t* size) { return cf_kv_val_string(kv, str, size); }
CUTE_INLINE bool kv_val_blob(KeyValue* kv, void* data, size_t data_capacity, size_t* data_len) { return cf_kv_val_blob(kv, data, data_capacity, data_len); }
CUTE_INLINE bool kv_object_begin(KeyValue* kv, const char* key = NULL) { return cf_kv_object_begin(kv, key); }
CUTE_INLINE bool kv_object_end(KeyValue* kv) { return cf_kv_object_end(kv); }
CUTE_INLINE bool kv_array_begin(KeyValue* kv, int* count, const char* key = NULL) { return cf_kv_array_begin(kv, count, key); }
CUTE_INLINE bool kv_array_end(KeyValue* kv) { return cf_kv_array_end(kv); }

}

#endif // CUTE_CPP

#endif // CUTE_KV_H
