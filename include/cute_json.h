/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_JSON_H
#define CF_JSON_H

#include "cute_defines.h"
#include "cute_string.h"
#include "cute_result.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @enum     CF_JDoc
 * @category json
 * @brief    Represents a single json document.
 * @related  CF_JDoc CF_JVal cf_make_json
 */
typedef struct CF_JDoc { uint64_t id; } CF_JDoc;
/* @end */

/**
 * @enum     CF_JVal
 * @category json
 * @brief    Represents a single json value, such an integer or array.
 * @related  CF_JDoc CF_JVal cf_make_json CF_JType
 */
typedef struct CF_JVal { uint64_t id; } CF_JVal;
/* @end */

/**
 * @enum     CF_JType
 * @category json
 * @brief    Describes the type of a `CF_JVal`.
 * @related  CF_JDoc CF_JVal cf_make_json
 */
#define CF_JTYPE_DEFS \
	/* @entry TODO */                 \
	CF_ENUM(JTYPE_NONE,           -1) \
	/* @entry TODO */                 \
	CF_ENUM(JTYPE_NULL,            0) \
	/* @entry TODO */                 \
	CF_ENUM(JTYPE_INT,             1) \
	/* @entry TODO */                 \
	CF_ENUM(JTYPE_FLOAT,           2) \
	/* @entry TODO */                 \
	CF_ENUM(JTYPE_BOOL,            3) \
	/* @entry TODO */                 \
	CF_ENUM(JTYPE_STRING,          4) \
	/* @entry TODO */                 \
	CF_ENUM(JTYPE_ARRAY,           5) \
	/* @entry TODO */                 \
	CF_ENUM(JTYPE_OBJECT,          6) \
	/* @end */

typedef enum CF_JType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_JTYPE_DEFS
	#undef CF_ENUM
} CF_JType;

/**
 * @function cf_json_type_to_string
 * @category json
 * @brief    Convert an enum `CF_JType` to a c-style string.
 * @param    state        The type to convert to a string.
 * @related  CF_JVal CF_JType
 */
CF_INLINE const char* cf_json_type_to_string(CF_JType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_JTYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_make_json
 * @category json
 * @brief    Loads a json blob.
 * @param    data       A pointer to the raw json blob data.
 * @param    size       The number of bytes in the `data` pointer.
 * @remarks  You should call `cf_json_get_root` on this document to begin fetching values out of it.
 * @related  cf_draw_quad cf_draw_quad2 cf_draw_quad_fill cf_draw_quad_fill2
 */
CF_API CF_JDoc CF_CALL cf_make_json(const void* data, size_t size);
CF_API CF_JDoc CF_CALL cf_make_json_from_file(const char* virtual_path);
CF_API void CF_CALL cf_destroy_json(CF_JDoc doc);
CF_API CF_JVal CF_CALL cf_json_get_root(CF_JDoc doc);
CF_API void CF_CALL cf_json_set_root(CF_JDoc doc, CF_JVal val);

//--------------------------------------------------------------------------------------------------
// Extracting values.

CF_API CF_JType CF_CALL cf_json_type(CF_JVal val);
CF_API bool CF_CALL cf_json_is_null(CF_JVal val);
CF_API bool CF_CALL cf_json_is_int(CF_JVal val);
CF_API bool CF_CALL cf_json_is_float(CF_JVal val);
CF_API bool CF_CALL cf_json_is_bool(CF_JVal val);
CF_API bool CF_CALL cf_json_is_string(CF_JVal val);
CF_API bool CF_CALL cf_json_is_array(CF_JVal val);
CF_API bool CF_CALL cf_json_is_object(CF_JVal val);

CF_API int CF_CALL cf_json_get_int(CF_JVal val);
CF_API int64_t CF_CALL cf_json_get_i64(CF_JVal val);
CF_API uint64_t CF_CALL cf_json_get_u64(CF_JVal val);
CF_API float CF_CALL cf_json_get_float(CF_JVal val);
CF_API double CF_CALL cf_json_get_double(CF_JVal val);
CF_API bool CF_CALL cf_json_get_bool(CF_JVal val);
CF_API const char* CF_CALL cf_json_get_string(CF_JVal val);
CF_API int CF_CALL cf_json_get_len(CF_JVal val);

//--------------------------------------------------------------------------------------------------
// Traversals.

CF_API CF_JVal CF_CALL cf_json_get(CF_JVal val, const char* key);
CF_API CF_JVal CF_CALL cf_json_array_at(CF_JVal val, int index);
CF_API CF_JVal CF_CALL cf_json_array_get(CF_JVal val, int index);

typedef struct CF_JIter
{
	size_t index;
	size_t count;
	CF_JVal val;
	CF_JVal prev;
	CF_JVal parent;
} CF_JIter;

CF_API CF_JIter CF_CALL cf_json_iter(CF_JVal val);
#define cf_json_iter_done(iter) ((iter).index >= (iter).count)
CF_API CF_JIter CF_CALL cf_json_iter_next(CF_JIter iter);
CF_API CF_JVal CF_CALL cf_json_iter_next_by_name(CF_JIter* iter, const char* key);
CF_API CF_JVal CF_CALL cf_json_iter_remove(CF_JIter* iter);
CF_API const char* CF_CALL cf_json_iter_key(CF_JIter iter);
CF_API CF_JVal CF_CALL cf_json_iter_val(CF_JIter iter);

//--------------------------------------------------------------------------------------------------
// Creating individual values.

CF_API CF_JVal CF_CALL cf_json_from_null(CF_JDoc doc);
CF_API CF_JVal CF_CALL cf_json_from_int(CF_JDoc doc, int val);
CF_API CF_JVal CF_CALL cf_json_from_i64(CF_JDoc doc, int64_t val);
CF_API CF_JVal CF_CALL cf_json_from_u64(CF_JDoc doc, uint64_t val);
CF_API CF_JVal CF_CALL cf_json_from_float(CF_JDoc doc, float val);
CF_API CF_JVal CF_CALL cf_json_from_double(CF_JDoc doc, double val);
CF_API CF_JVal CF_CALL cf_json_from_bool(CF_JDoc doc, bool val);
CF_API CF_JVal CF_CALL cf_json_from_string(CF_JDoc doc, const char* val);
CF_API CF_JVal CF_CALL cf_json_from_string_range(CF_JDoc doc, const char* begin, const char* end);

//--------------------------------------------------------------------------------------------------
// Setting values.

CF_API void CF_CALL cf_json_set_null(CF_JVal jval);
CF_API void CF_CALL cf_json_set_int(CF_JVal jval, int val);
CF_API void CF_CALL cf_json_set_i64(CF_JVal jval, int64_t val);
CF_API void CF_CALL cf_json_set_u64(CF_JVal jval, uint64_t val);
CF_API void CF_CALL cf_json_set_float(CF_JVal jval, float val);
CF_API void CF_CALL cf_json_set_double(CF_JVal jval, double val);
CF_API void CF_CALL cf_json_set_bool(CF_JVal jval, bool val);
CF_API void CF_CALL cf_json_set_string(CF_JVal jval, const char* val);
CF_API void CF_CALL cf_json_set_string_range(CF_JVal jval, const char* begin, const char* end);

//--------------------------------------------------------------------------------------------------
// Creating arrays.

CF_API CF_JVal CF_CALL cf_json_array(CF_JDoc doc);
CF_API CF_JVal CF_CALL cf_json_array_from_int(CF_JDoc doc, int* vals, int count);
CF_API CF_JVal CF_CALL cf_json_array_from_i64(CF_JDoc doc, int64_t* vals, int count);
CF_API CF_JVal CF_CALL cf_json_array_from_u64(CF_JDoc doc, uint64_t* vals, int count);
CF_API CF_JVal CF_CALL cf_json_array_from_float(CF_JDoc doc, float* vals, int count);
CF_API CF_JVal CF_CALL cf_json_array_from_double(CF_JDoc doc, double* vals, int count);
CF_API CF_JVal CF_CALL cf_json_array_from_bool(CF_JDoc doc, bool* vals, int count);
CF_API CF_JVal CF_CALL cf_json_array_from_string(CF_JDoc doc, const char** vals, int count);

//--------------------------------------------------------------------------------------------------
// Adding elements onto arrays.

CF_API void CF_CALL cf_json_array_add(CF_JVal arr, CF_JVal val);
CF_API void CF_CALL cf_json_array_add_null(CF_JDoc doc, CF_JVal arr);
CF_API void CF_CALL cf_json_array_add_int(CF_JDoc doc, CF_JVal arr, int val);
CF_API void CF_CALL cf_json_array_add_i64(CF_JDoc doc, CF_JVal arr, int64_t val);
CF_API void CF_CALL cf_json_array_add_u64(CF_JDoc doc, CF_JVal arr, uint64_t val);
CF_API void CF_CALL cf_json_array_add_float(CF_JDoc doc, CF_JVal arr, float val);
CF_API void CF_CALL cf_json_array_add_double(CF_JDoc doc, CF_JVal arr, double val);
CF_API void CF_CALL cf_json_array_add_bool(CF_JDoc doc, CF_JVal arr, bool val);
CF_API void CF_CALL cf_json_array_add_string(CF_JDoc doc, CF_JVal arr, const char* val);
CF_API void CF_CALL cf_json_array_add_string_range(CF_JDoc doc, CF_JVal arr, const char* begin, const char* end);
CF_API CF_JVal CF_CALL cf_json_array_add_array(CF_JDoc doc, CF_JVal arr);
CF_API CF_JVal CF_CALL cf_json_array_add_object(CF_JDoc doc, CF_JVal arr);
CF_API CF_JVal CF_CALL cf_json_array_pop(CF_JVal arr);

//--------------------------------------------------------------------------------------------------
// Creating objects.

CF_API CF_JVal CF_CALL cf_json_object(CF_JDoc doc);
CF_API CF_JVal CF_CALL cf_json_object_from_strings(CF_JDoc doc, const char** keys, const char** vals, int count);
CF_API CF_JVal CF_CALL cf_json_object_from_string_pairs(CF_JDoc doc, const char** kv_pairs, int pair_count);

//--------------------------------------------------------------------------------------------------
// Adding properties onto objects.

CF_API void CF_CALL cf_json_object_add(CF_JDoc doc, CF_JVal obj, const char* key, CF_JVal val);
CF_API void CF_CALL cf_json_object_add_null(CF_JDoc doc, CF_JVal obj, const char* key);
CF_API void CF_CALL cf_json_object_add_int(CF_JDoc doc, CF_JVal obj, const char* key, int val);
CF_API void CF_CALL cf_json_object_add_i64(CF_JDoc doc, CF_JVal obj, const char* key, int64_t val);
CF_API void CF_CALL cf_json_object_add_u64(CF_JDoc doc, CF_JVal obj, const char* key, uint64_t val);
CF_API void CF_CALL cf_json_object_add_float(CF_JDoc doc, CF_JVal obj, const char* key, float val);
CF_API void CF_CALL cf_json_object_add_double(CF_JDoc doc, CF_JVal obj, const char* key, double val);
CF_API void CF_CALL cf_json_object_add_bool(CF_JDoc doc, CF_JVal obj, const char* key, bool val);
CF_API void CF_CALL cf_json_object_add_string(CF_JDoc doc, CF_JVal obj, const char* key, const char* val);
CF_API void CF_CALL cf_json_object_add_string_range(CF_JDoc doc, CF_JVal obj, const char* key, const char* begin, const char* end);

//--------------------------------------------------------------------------------------------------
// Object manipulation.

CF_API void CF_CALL cf_json_object_remove_key(CF_JVal obj, const char* key);
CF_API void CF_CALL cf_json_object_remove_key_range(CF_JVal obj, const char* key_begin, const char* key_end);
CF_API void CF_CALL cf_json_object_rename_key(CF_JDoc doc, CF_JVal obj, const char* key, const char* rename);
CF_API void CF_CALL cf_json_object_rename_key_range(CF_JDoc doc, CF_JVal obj, const char* key_begin, const char* key_end, const char* rename_begin, const char* rename_end);

//--------------------------------------------------------------------------------------------------
// Write as string.

CF_API dyna char* CF_CALL cf_json_to_string(CF_JDoc doc);
CF_API dyna char* CF_CALL cf_json_to_string_minimal(CF_JDoc doc);
CF_API CF_Result CF_CALL cf_json_to_file(CF_JDoc doc, const char* virtual_path);
CF_API CF_Result CF_CALL cf_json_to_file_minimal(CF_JDoc doc, const char* virtual_path);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{
}

#endif // CF_CPP

#endif // CF_JSON_H
