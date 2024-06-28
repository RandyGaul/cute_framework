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
 * @struct   CF_JDoc
 * @category json
 * @brief    Represents a single json document.
 * @related  CF_JDoc CF_JVal cf_make_json
 */
typedef struct CF_JDoc { uint64_t id; } CF_JDoc;
/* @end */

/**
 * @struct   CF_JVal
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
	/* @entry Empty value, representing an uninitialized state. This is not the same as `CF_JTYPE_NULL`. */ \
	CF_ENUM(JTYPE_NONE,           -1)                                                                       \
	/* @entry Null. */                                                                                      \
	CF_ENUM(JTYPE_NULL,            0)                                                                       \
	/* @entry Integer. */                                                                                   \
	CF_ENUM(JTYPE_INT,             1)                                                                       \
	/* @entry Float. */                                                                                     \
	CF_ENUM(JTYPE_FLOAT,           2)                                                                       \
	/* @entry Boolean. */                                                                                   \
	CF_ENUM(JTYPE_BOOL,            3)                                                                       \
	/* @entry String. */                                                                                    \
	CF_ENUM(JTYPE_STRING,          4)                                                                       \
	/* @entry An Array. */                                                                                  \
	CF_ENUM(JTYPE_ARRAY,           5)                                                                       \
	/* @entry A JSON Object. */                                                                             \
	CF_ENUM(JTYPE_OBJECT,          6)                                                                       \
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
 * @return   Returns a `CF_JDoc`.
 * @remarks  You should call `cf_json_get_root` on this document to begin fetching values out of it.
 * @related  CF_JDoc cf_make_json cf_make_json_from_file cf_json_get_root cf_destroy_json cf_json_get_root cf_json_to_string cf_json_to_file
 */
CF_API CF_JDoc CF_CALL cf_make_json(const void* data, size_t size);

/**
 * @function cf_make_json_from_file
 * @category json
 * @brief    Loads a json blob from a file.
 * @param    virtual_path  A virtual path to the json file. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @return   Returns a `CF_JDoc`.
 * @remarks  You should call `cf_json_get_root` on this document to begin fetching values out of it.
 * @related  CF_JDoc cf_make_json cf_make_json_from_file cf_json_get_root cf_destroy_json cf_json_get_root cf_json_to_string cf_json_to_file
 */
CF_API CF_JDoc CF_CALL cf_make_json_from_file(const char* virtual_path);

/**
 * @function cf_destroy_json
 * @category json
 * @brief    Destroys a json blob `CF_JDoc`.
 * @related  CF_JDoc cf_make_json cf_make_json_from_file cf_json_get_root cf_destroy_json cf_json_get_root cf_json_to_string cf_json_to_file
 */
CF_API void CF_CALL cf_destroy_json(CF_JDoc doc);

/**
 * @function cf_json_get_root
 * @category json
 * @brief    Fetches the root of the document.
 * @return   Returns a `CF_JVal`, the root of the document.
 * @related  CF_JDoc cf_make_json cf_make_json_from_file cf_json_get_root cf_destroy_json cf_json_get_root cf_json_set_root
 */
CF_API CF_JVal CF_CALL cf_json_get_root(CF_JDoc doc);

/**
 * @function cf_json_set_root
 * @category json
 * @brief    Sets the root of the document.
 * @related  CF_JDoc cf_make_json cf_make_json_from_file cf_json_get_root cf_destroy_json cf_json_get_root cf_json_set_root
 */
CF_API void CF_CALL cf_json_set_root(CF_JDoc doc, CF_JVal val);

//--------------------------------------------------------------------------------------------------
// Extracting values.

/**
 * @function cf_json_type
 * @category json
 * @brief    Returns the type of the `CF_JVal` via enum `CF_JType`.
 * @related  CF_JType CF_JVal cf_json_type cf_json_is_null cf_json_is_int cf_json_is_float cf_json_is_bool cf_json_is_string cf_json_is_array cf_json_is_object
 */
CF_API CF_JType CF_CALL cf_json_type(CF_JVal val);

/**
 * @function cf_json_is_null
 * @category json
 * @brief    Returns true if the `CF_JType` is `CF_JTYPE_NULL`.
 * @related  CF_JVal cf_json_type cf_json_is_null cf_json_is_int cf_json_is_float cf_json_is_bool cf_json_is_string cf_json_is_array cf_json_is_object
 */
CF_API bool CF_CALL cf_json_is_null(CF_JVal val);

/**
 * @function cf_json_is_int
 * @category json
 * @brief    Returns true if the `CF_JType` is `CF_JTYPE_INT`.
 * @related  CF_JVal cf_json_type cf_json_is_null cf_json_is_int cf_json_is_float cf_json_is_bool cf_json_is_string cf_json_is_array cf_json_is_object
 */
CF_API bool CF_CALL cf_json_is_int(CF_JVal val);

/**
 * @function cf_json_is_float
 * @category json
 * @brief    Returns true if the `CF_JType` is `CF_JTYPE_FLOAT`.
 * @related  CF_JVal cf_json_type cf_json_is_null cf_json_is_int cf_json_is_float cf_json_is_bool cf_json_is_string cf_json_is_array cf_json_is_object
 */
CF_API bool CF_CALL cf_json_is_float(CF_JVal val);

/**
 * @function cf_json_is_bool
 * @category json
 * @brief    Returns true if the `CF_JType` is `CF_JTYPE_BOOL`.
 * @related  CF_JVal cf_json_type cf_json_is_null cf_json_is_int cf_json_is_float cf_json_is_bool cf_json_is_string cf_json_is_array cf_json_is_object
 */
CF_API bool CF_CALL cf_json_is_bool(CF_JVal val);

/**
 * @function cf_json_is_string
 * @category json
 * @brief    Returns true if the `CF_JType` is `CF_JTYPE_STRING`.
 * @related  CF_JVal cf_json_type cf_json_is_null cf_json_is_int cf_json_is_float cf_json_is_bool cf_json_is_string cf_json_is_array cf_json_is_object
 */
CF_API bool CF_CALL cf_json_is_string(CF_JVal val);

/**
 * @function cf_json_is_array
 * @category json
 * @brief    Returns true if the `CF_JType` is `CF_JTYPE_ARRAY`.
 * @related  CF_JVal cf_json_type cf_json_is_null cf_json_is_int cf_json_is_float cf_json_is_bool cf_json_is_string cf_json_is_array cf_json_is_object
 */
CF_API bool CF_CALL cf_json_is_array(CF_JVal val);

/**
 * @function cf_json_is_object
 * @category json
 * @brief    Returns true if the `CF_JType` is `CF_JTYPE_OBJECT`.
 * @related  CF_JVal cf_json_type cf_json_is_null cf_json_is_int cf_json_is_float cf_json_is_bool cf_json_is_string cf_json_is_array cf_json_is_object
 */
CF_API bool CF_CALL cf_json_is_object(CF_JVal val);

/**
 * @function cf_json_get_int
 * @category json
 * @brief    Interprets the `CF_JVal` as an integer.
 * @related  CF_JVal cf_json_get_int cf_json_get_i64 cf_json_get_u64 cf_json_get_float cf_json_get_double cf_json_get_bool cf_json_get_string cf_json_get_len
 */
CF_API int CF_CALL cf_json_get_int(CF_JVal val);

/**
 * @function cf_json_get_i64
 * @category json
 * @brief    Interprets the `CF_JVal` as a 64-bit signed integer.
 * @related  CF_JVal cf_json_get_int cf_json_get_i64 cf_json_get_u64 cf_json_get_float cf_json_get_double cf_json_get_bool cf_json_get_string cf_json_get_len
 */
CF_API int64_t CF_CALL cf_json_get_i64(CF_JVal val);

/**
 * @function cf_json_get_u64
 * @category json
 * @brief    Interprets the `CF_JVal` as a 64-bit unsigned integer.
 * @related  CF_JVal cf_json_get_int cf_json_get_i64 cf_json_get_u64 cf_json_get_float cf_json_get_double cf_json_get_bool cf_json_get_string cf_json_get_len
 */
CF_API uint64_t CF_CALL cf_json_get_u64(CF_JVal val);

/**
 * @function cf_json_get_float
 * @category json
 * @brief    Interprets the `CF_JVal` as a 32-bit float.
 * @related  CF_JVal cf_json_get_int cf_json_get_i64 cf_json_get_u64 cf_json_get_float cf_json_get_double cf_json_get_bool cf_json_get_string cf_json_get_len
 */
CF_API float CF_CALL cf_json_get_float(CF_JVal val);

/**
 * @function cf_json_get_double
 * @category json
 * @brief    Interprets the `CF_JVal` as a 64-bit float.
 * @related  CF_JVal cf_json_get_int cf_json_get_i64 cf_json_get_u64 cf_json_get_float cf_json_get_double cf_json_get_bool cf_json_get_string cf_json_get_len
 */
CF_API double CF_CALL cf_json_get_double(CF_JVal val);

/**
 * @function cf_json_get_bool
 * @category json
 * @brief    Interprets the `CF_JVal` as a boolean.
 * @related  CF_JVal cf_json_get_int cf_json_get_i64 cf_json_get_u64 cf_json_get_float cf_json_get_double cf_json_get_bool cf_json_get_string cf_json_get_len
 */
CF_API bool CF_CALL cf_json_get_bool(CF_JVal val);

/**
 * @function cf_json_get_string
 * @category json
 * @brief    Interprets the `CF_JVal` as a plain C-string.
 * @related  CF_JVal cf_json_get_int cf_json_get_i64 cf_json_get_u64 cf_json_get_float cf_json_get_double cf_json_get_bool cf_json_get_string cf_json_get_len
 */
CF_API const char* CF_CALL cf_json_get_string(CF_JVal val);

/**
 * @function cf_json_get_len
 * @category json
 * @brief    Returns the length of the `CF_JVal`'s string, if it is a valid string.
 * @related  CF_JVal cf_json_get_int cf_json_get_i64 cf_json_get_u64 cf_json_get_float cf_json_get_double cf_json_get_bool cf_json_get_string cf_json_get_len
 */
CF_API int CF_CALL cf_json_get_len(CF_JVal val);

//--------------------------------------------------------------------------------------------------
// Traversals.

/**
 * @function cf_json_get
 * @category json
 * @brief    Looks up a value for a given key.
 * @param    val       The JSON value to search for `key` within.
 * @param    key       The search key.
 * @return   Returns the `CF_JVal` associated with `key` on the object `val`.
 * @related  CF_JVal cf_json_get cf_json_array_at cf_json_array_get cf_json_iter
 */
CF_API CF_JVal CF_CALL cf_json_get(CF_JVal val, const char* key);

/**
 * @function cf_json_array_at
 * @category json
 * @brief    Fetches a value in the given array.
 * @param    val       The JSON value to search for `key` within.
 * @param    index     The index of the value to return.
 * @return   Returns the `CF_JVal` associated with `index` on the object `val`.
 * @remarks  This function does the same thing as `cf_json_array_get`.
 * @related  CF_JVal cf_json_get cf_json_array_at cf_json_array_get cf_json_iter
 */
CF_API CF_JVal CF_CALL cf_json_array_at(CF_JVal val, int index);

/**
 * @function cf_json_array_get
 * @category json
 * @brief    Fetches a value in the given array.
 * @param    val       The JSON value to search for `key` within.
 * @param    index     The index of the value to return.
 * @return   Returns the `CF_JVal` associated with `index` on the object `val`.
 * @remarks  This function does the same thing as `cf_json_array_at`.
 * @related  CF_JVal cf_json_get cf_json_array_at cf_json_array_get cf_json_iter
 */
CF_API CF_JVal CF_CALL cf_json_array_get(CF_JVal val, int index);

/**
 * @struct   CF_JIter
 * @category json
 * @brief    An iterator for looping over arrays or key-value pairs.
 * @related  CF_JIter cf_json_iter cf_json_iter_next cf_json_iter_next_by_name cf_json_iter_remove cf_json_iter_key cf_json_iter_val
 */
typedef struct CF_JIter
{
	/* @member For internal use. Don't touch. Used for iterating arrays. */
	size_t index;
	/* @member For internal use. Don't touch. */
	size_t count;
	/* @member For internal use. Don't touch. The current `CF_JVal`. */
	CF_JVal val;
	/* @member For internal use. Don't touch. */
	CF_JVal prev;
	/* @member For internal use. Don't touch. */
	CF_JVal parent;
} CF_JIter;
// @end

/**
 * @function cf_json_iter
 * @category json
 * @brief    Creates an iterator for a given JSON value.
 * @param    val       The JSON value to iterate upon.
 * @return   Returns a `CF_JIter` for iterating.
 * @example > Traversing arrays/objects.
 *           // Traverse an array of strings:
 *           for (CF_JIter i = cf_json_iter(v); !cf_json_iter_done(i); i = cf_json_iter_next(i)) {
 *               const char* val = cf_json_get_string(cf_json_iter_val(i));
 *               printf("%s\n", val);
 *           }
 *           
 *           // Traverse key/val pairs on an objects:
 *           for (CF_JIter i = cf_json_iter(v); !cf_json_iter_done(i); iter = cf_json_iter_next(i)) {
 *               const char* val = cf_json_get_string(cf_json_iter_val(i));
 *               printf("%s\n", val);
 *           }
 * @remarks  The `CF_JIter` can be used in foor loops, and can traverse both JSON arrays and objects. When
 *           traversing arrays do not call `cf_json_iter_key`.
 * @related  CF_JVal cf_json_get cf_json_array_at cf_json_array_get cf_json_iter cf_json_iter_remove
 */
CF_API CF_JIter CF_CALL cf_json_iter(CF_JVal val);

/**
 * @function cf_json_iter_done
 * @category json
 * @brief    Returns true if the `CF_JIter` has finished iterating over all elements.
 * @remarks  See `cf_json_iter`.
 * @related  CF_JVal cf_json_get cf_json_array_at cf_json_array_get cf_json_iter cf_json_iter_remove
 */
#define cf_json_iter_done(iter) ((iter).index >= (iter).count)

/**
 * @function cf_json_iter_next
 * @category json
 * @brief    Proceeds to the next element.
 * @remarks  See `cf_json_iter`.
 * @related  CF_JVal cf_json_get cf_json_array_at cf_json_array_get cf_json_iter cf_json_iter_remove
 */
CF_API CF_JIter CF_CALL cf_json_iter_next(CF_JIter iter);

/**
 * @function cf_json_iter_next_by_name
 * @category json
 * @brief    Proceeds to the next element with a matching name.
 * @remarks  You should know the ordering of your key/val pairs before calling this function, as it only searches forwards. See `cf_json_iter`.
 * @related  CF_JVal cf_json_get cf_json_array_at cf_json_array_get cf_json_iter cf_json_iter_remove
 */
CF_API CF_JVal CF_CALL cf_json_iter_next_by_name(CF_JIter* iter, const char* key);

/**
 * @function cf_json_iter_remove
 * @category json
 * @brief    Removes the element currently referenced by the iterator.
 * @remarks  See `cf_json_iter`.
 * @related  CF_JVal cf_json_get cf_json_array_at cf_json_array_get cf_json_iter cf_json_iter_remove
 */
CF_API CF_JVal CF_CALL cf_json_iter_remove(CF_JIter* iter);

/**
 * @function cf_json_iter_key
 * @category json
 * @brief    Returns the key currently referenced by the iterator.
 * @remarks  You should not call this function when iterating over an array. See `cf_json_iter`.
 * @related  CF_JVal cf_json_get cf_json_array_at cf_json_array_get cf_json_iter cf_json_iter_remove
 */
CF_API const char* CF_CALL cf_json_iter_key(CF_JIter iter);

/**
 * @function cf_json_iter_val
 * @category json
 * @brief    Returns the value currently referenced by the iterator.
 * @remarks  See `cf_json_iter`.
 * @related  CF_JVal cf_json_get cf_json_array_at cf_json_array_get cf_json_iter cf_json_iter_remove
 */
CF_API CF_JVal CF_CALL cf_json_iter_val(CF_JIter iter);

//--------------------------------------------------------------------------------------------------
// Creating individual values.

/**
 * @function cf_json_from_null
 * @category json
 * @brief    Creates and returns a new NULL json value.
 * @remarks  The value can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_from_null cf_json_from_int cf_json_from_float cf_json_from_bool cf_json_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_from_null(CF_JDoc doc);

/**
 * @function cf_json_from_int
 * @category json
 * @brief    Creates and returns a new 32-bit int json value.
 * @remarks  The value can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_from_null cf_json_from_int cf_json_from_i64 cf_json_from_u64 cf_json_from_float cf_json_from_bool cf_json_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_from_int(CF_JDoc doc, int val);

/**
 * @function cf_json_from_i64
 * @category json
 * @brief    Creates and returns a new 64-bit signed int json value.
 * @remarks  The value can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_from_null cf_json_from_int cf_json_from_i64 cf_json_from_u64 cf_json_from_float cf_json_from_bool cf_json_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_from_i64(CF_JDoc doc, int64_t val);

/**
 * @function cf_json_from_u64
 * @category json
 * @brief    Creates and returns a new 64-bit unsigned int json value.
 * @remarks  The value can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_from_null cf_json_from_int cf_json_from_i64 cf_json_from_u64 cf_json_from_float cf_json_from_bool cf_json_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_from_u64(CF_JDoc doc, uint64_t val);

/**
 * @function cf_json_from_float
 * @category json
 * @brief    Creates and returns a new 32-bit float json value.
 * @remarks  The value can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_from_null cf_json_from_int cf_json_from_float cf_json_from_double cf_json_from_bool cf_json_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_from_float(CF_JDoc doc, float val);

/**
 * @function cf_json_from_double
 * @category json
 * @brief    Creates and returns a new 64-bit float json value.
 * @remarks  The value can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_from_null cf_json_from_int cf_json_from_float cf_json_from_double cf_json_from_bool cf_json_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_from_double(CF_JDoc doc, double val);

/**
 * @function cf_json_from_bool
 * @category json
 * @brief    Creates and returns a new boolean json value.
 * @remarks  The value can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_from_null cf_json_from_int cf_json_from_float cf_json_from_bool cf_json_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_from_bool(CF_JDoc doc, bool val);

/**
 * @function cf_json_from_string
 * @category json
 * @brief    Creates and returns a new string json value.
 * @remarks  The value can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_from_null cf_json_from_int cf_json_from_float cf_json_from_bool cf_json_from_string cf_json_from_string_range cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_from_string(CF_JDoc doc, const char* val);

/**
 * @function cf_json_from_string_range
 * @category json
 * @brief    Creates and returns a new string json value.
 * @remarks  The value can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_from_null cf_json_from_int cf_json_from_float cf_json_from_bool cf_json_from_string cf_json_from_string_range cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_from_string_range(CF_JDoc doc, const char* begin, const char* end);

//--------------------------------------------------------------------------------------------------
// Setting values.

/**
 * @function cf_json_set_null
 * @category json
 * @brief    Sets the `CF_JVal` to null.
 * @related  CF_JVal cf_json_set_null cf_json_set_int cf_json_set_i64 cf_json_set_u64 cf_json_set_float cf_json_set_double cf_json_set_bool cf_json_set_string cf_json_set_string_range
 */
CF_API void CF_CALL cf_json_set_null(CF_JVal jval);

/**
 * @function cf_json_set_int
 * @category json
 * @brief    Sets the `CF_JVal` to a 32-bit signed int.
 * @related  CF_JVal cf_json_set_null cf_json_set_int cf_json_set_i64 cf_json_set_u64 cf_json_set_float cf_json_set_double cf_json_set_bool cf_json_set_string cf_json_set_string_range
 */
CF_API void CF_CALL cf_json_set_int(CF_JVal jval, int val);

/**
 * @function cf_json_set_i64
 * @category json
 * @brief    Sets the `CF_JVal` to a 64-bit signed int.
 * @related  CF_JVal cf_json_set_null cf_json_set_int cf_json_set_i64 cf_json_set_u64 cf_json_set_float cf_json_set_double cf_json_set_bool cf_json_set_string cf_json_set_string_range
 */
CF_API void CF_CALL cf_json_set_i64(CF_JVal jval, int64_t val);

/**
 * @function cf_json_set_u64
 * @category json
 * @brief    Sets the `CF_JVal` to an unsigned 64-bit int.
 * @related  CF_JVal cf_json_set_null cf_json_set_int cf_json_set_i64 cf_json_set_u64 cf_json_set_float cf_json_set_double cf_json_set_bool cf_json_set_string cf_json_set_string_range
 */
CF_API void CF_CALL cf_json_set_u64(CF_JVal jval, uint64_t val);

/**
 * @function cf_json_set_float
 * @category json
 * @brief    Sets the `CF_JVal` to a 32-bit float.
 * @related  CF_JVal cf_json_set_null cf_json_set_int cf_json_set_i64 cf_json_set_u64 cf_json_set_float cf_json_set_double cf_json_set_bool cf_json_set_string cf_json_set_string_range
 */
CF_API void CF_CALL cf_json_set_float(CF_JVal jval, float val);

/**
 * @function cf_json_set_double
 * @category json
 * @brief    Sets the `CF_JVal` to a 64-bit float.
 * @related  CF_JVal cf_json_set_null cf_json_set_int cf_json_set_i64 cf_json_set_u64 cf_json_set_float cf_json_set_double cf_json_set_bool cf_json_set_string cf_json_set_string_range
 */
CF_API void CF_CALL cf_json_set_double(CF_JVal jval, double val);

/**
 * @function cf_json_set_bool
 * @category json
 * @brief    Sets the `CF_JVal` to a boolean.
 * @related  CF_JVal cf_json_set_null cf_json_set_int cf_json_set_i64 cf_json_set_u64 cf_json_set_float cf_json_set_double cf_json_set_bool cf_json_set_string cf_json_set_string_range
 */
CF_API void CF_CALL cf_json_set_bool(CF_JVal jval, bool val);

/**
 * @function cf_json_set_string
 * @category json
 * @brief    Sets the `CF_JVal` to a string.
 * @remarks  The string must be retained in memory while the `CF_JDoc` persists.
 * @related  CF_JVal cf_json_set_null cf_json_set_int cf_json_set_i64 cf_json_set_u64 cf_json_set_float cf_json_set_double cf_json_set_bool cf_json_set_string cf_json_set_string_range
 */
CF_API void CF_CALL cf_json_set_string(CF_JVal jval, const char* val);

/**
 * @function cf_json_set_string_range
 * @category json
 * @brief    Sets the `CF_JVal` to a string.
 * @remarks  The string must be retained in memory while the `CF_JDoc` persists.
 * @related  CF_JVal cf_json_set_null cf_json_set_int cf_json_set_i64 cf_json_set_u64 cf_json_set_float cf_json_set_double cf_json_set_bool cf_json_set_string cf_json_set_string_range
 */
CF_API void CF_CALL cf_json_set_string_range(CF_JVal jval, const char* begin, const char* end);

//--------------------------------------------------------------------------------------------------
// Creating arrays.

/**
 * @function cf_json_array
 * @category json
 * @brief    Creates a new json array.
 * @remarks  The value can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_array cf_json_array_from_int cf_json_array_from_i64 cf_json_array_from_u64 cf_json_array_from_float cf_json_array_from_double cf_json_array_from_bool cf_json_array_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_array(CF_JDoc doc);

/**
 * @function cf_json_array_from_int
 * @category json
 * @brief    Creates a new json array from an array of integers.
 * @remarks  The returned `CF_JVal` can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_array cf_json_array_from_int cf_json_array_from_i64 cf_json_array_from_u64 cf_json_array_from_float cf_json_array_from_double cf_json_array_from_bool cf_json_array_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_array_from_int(CF_JDoc doc, int* vals, int count);

/**
 * @function cf_json_array_from_i64
 * @category json
 * @brief    Creates a new json array from an array of integers.
 * @remarks  The returned `CF_JVal` can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_array cf_json_array_from_int cf_json_array_from_i64 cf_json_array_from_u64 cf_json_array_from_float cf_json_array_from_double cf_json_array_from_bool cf_json_array_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_array_from_i64(CF_JDoc doc, int64_t* vals, int count);

/**
 * @function cf_json_array_from_u64
 * @category json
 * @brief    Creates a new json array from an array of integers.
 * @remarks  The returned `CF_JVal` can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_array cf_json_array_from_int cf_json_array_from_i64 cf_json_array_from_u64 cf_json_array_from_float cf_json_array_from_double cf_json_array_from_bool cf_json_array_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_array_from_u64(CF_JDoc doc, uint64_t* vals, int count);

/**
 * @function cf_json_array_from_float
 * @category json
 * @brief    Creates a new json array from an array of floats.
 * @remarks  The returned `CF_JVal` can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_array cf_json_array_from_int cf_json_array_from_i64 cf_json_array_from_u64 cf_json_array_from_float cf_json_array_from_double cf_json_array_from_bool cf_json_array_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_array_from_float(CF_JDoc doc, float* vals, int count);

/**
 * @function cf_json_array_from_double
 * @category json
 * @brief    Creates a new json array from an array of 64-bit floats.
 * @remarks  The returned `CF_JVal` can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_array cf_json_array_from_int cf_json_array_from_i64 cf_json_array_from_u64 cf_json_array_from_float cf_json_array_from_double cf_json_array_from_bool cf_json_array_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_array_from_double(CF_JDoc doc, double* vals, int count);

/**
 * @function cf_json_array_from_bool
 * @category json
 * @brief    Creates a new json array from an array of bools.
 * @remarks  The returned `CF_JVal` can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_array cf_json_array_from_int cf_json_array_from_i64 cf_json_array_from_u64 cf_json_array_from_float cf_json_array_from_double cf_json_array_from_bool cf_json_array_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_array_from_bool(CF_JDoc doc, bool* vals, int count);

/**
 * @function cf_json_array_from_string
 * @category json
 * @brief    Creates a new json array from an array of strings.
 * @remarks  The returned `CF_JVal` can be attached to the document by `cf_json_array_add` or `cf_json_object_add`.
 * @related  CF_JVal cf_json_array cf_json_array_from_int cf_json_array_from_i64 cf_json_array_from_u64 cf_json_array_from_float cf_json_array_from_double cf_json_array_from_bool cf_json_array_from_string cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_array_from_string(CF_JDoc doc, const char** vals, int count);

//--------------------------------------------------------------------------------------------------
// Adding elements onto arrays.

/**
 * @function cf_json_array_add
 * @category json
 * @brief    Adds a json value to the end of a json array.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API void CF_CALL cf_json_array_add(CF_JVal arr, CF_JVal val);

/**
 * @function cf_json_array_add_null
 * @category json
 * @brief    Adds a null value to the end of a json array.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API void CF_CALL cf_json_array_add_null(CF_JDoc doc, CF_JVal arr);

/**
 * @function cf_json_array_add_int
 * @category json
 * @brief    Adds a 32-bit signed int to the end of a json array.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API void CF_CALL cf_json_array_add_int(CF_JDoc doc, CF_JVal arr, int val);

/**
 * @function cf_json_array_add_i64
 * @category json
 * @brief    Adds a 64-bit signed int to the end of a json array.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API void CF_CALL cf_json_array_add_i64(CF_JDoc doc, CF_JVal arr, int64_t val);

/**
 * @function cf_json_array_add_u64
 * @category json
 * @brief    Adds a 64-bit unsigned int to the end of a json array.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API void CF_CALL cf_json_array_add_u64(CF_JDoc doc, CF_JVal arr, uint64_t val);

/**
 * @function cf_json_array_add_float
 * @category json
 * @brief    Adds a 32-bit float to the end of a json array.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API void CF_CALL cf_json_array_add_float(CF_JDoc doc, CF_JVal arr, float val);

/**
 * @function cf_json_array_add_double
 * @category json
 * @brief    Adds a 64-bit float to the end of a json array.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API void CF_CALL cf_json_array_add_double(CF_JDoc doc, CF_JVal arr, double val);

/**
 * @function cf_json_array_add_bool
 * @category json
 * @brief    Adds a bool to the end of a json array.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API void CF_CALL cf_json_array_add_bool(CF_JDoc doc, CF_JVal arr, bool val);

/**
 * @function cf_json_array_add_string
 * @category json
 * @brief    Adds a string to the end of a json array.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API void CF_CALL cf_json_array_add_string(CF_JDoc doc, CF_JVal arr, const char* val);

/**
 * @function cf_json_array_add_string_range
 * @category json
 * @brief    Adds a string to the end of a json array.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API void CF_CALL cf_json_array_add_string_range(CF_JDoc doc, CF_JVal arr, const char* begin, const char* end);

/**
 * @function cf_json_array_add_array
 * @category json
 * @brief    Creates a new array and adds it to the end of a json array.
 * @return   Returns the newly added array.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_array_add_array(CF_JDoc doc, CF_JVal arr);

/**
 * @function cf_json_array_add_object
 * @category json
 * @brief    Adds an empty object to the end of a json array.
 * @return   Returns the newly added empty object.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_array_add_object(CF_JDoc doc, CF_JVal arr);

/**
 * @function cf_json_array_pop
 * @category json
 * @brief    Removes the last element of the array and returns it.
 * @related  CF_JVal cf_json_array_add cf_json_object_add
 */
CF_API CF_JVal CF_CALL cf_json_array_pop(CF_JVal arr);

//--------------------------------------------------------------------------------------------------
// Creating objects.

/**
 * @function cf_json_object
 * @category json
 * @brief    Creates a new empty json object.
 * @related  CF_JVal cf_json_object cf_json_object_from_strings cf_json_object_from_string_pairs
 */
CF_API CF_JVal CF_CALL cf_json_object(CF_JDoc doc);

/**
 * @function cf_json_object_from_strings
 * @category json
 * @brief    Creates a new json object from an array of key/val pairs as strings.
 * @related  CF_JVal cf_json_object cf_json_object_from_strings cf_json_object_from_string_pairs
 */
CF_API CF_JVal CF_CALL cf_json_object_from_strings(CF_JDoc doc, const char** keys, const char** vals, int count);

/**
 * @function cf_json_object_from_string_pairs
 * @category json
 * @brief    Creates a new json object from an array of key/val pairs as strings.
 * @related  CF_JVal cf_json_object cf_json_object_from_strings cf_json_object_from_string_pairs
 */
CF_API CF_JVal CF_CALL cf_json_object_from_string_pairs(CF_JDoc doc, const char** kv_pairs, int pair_count);

//--------------------------------------------------------------------------------------------------
// Adding properties onto objects.

/**
 * @function cf_json_object_add
 * @category json
 * @brief    Adds a property (key/val pair) to a json object.
 * @related  CF_JVal cf_json_object_add cf_json_object_add_null cf_json_object_add_int cf_json_object_add_float cf_json_object_add_bool cf_json_object_add_string
 */
CF_API void CF_CALL cf_json_object_add(CF_JDoc doc, CF_JVal obj, const char* key, CF_JVal val);

/**
 * @function cf_json_object_add_null
 * @category json
 * @brief    Adds a null to a json object.
 * @related  CF_JVal cf_json_object_add cf_json_object_add_null cf_json_object_add_int cf_json_object_add_float cf_json_object_add_bool cf_json_object_add_string
 */
CF_API void CF_CALL cf_json_object_add_null(CF_JDoc doc, CF_JVal obj, const char* key);

/**
 * @function cf_json_object_add_int
 * @category json
 * @brief    Adds a 32-bit signed int to a json object.
 * @related  CF_JVal cf_json_object_add cf_json_object_add_null cf_json_object_add_int cf_json_object_add_float cf_json_object_add_bool cf_json_object_add_string
 */
CF_API void CF_CALL cf_json_object_add_int(CF_JDoc doc, CF_JVal obj, const char* key, int val);

/**
 * @function cf_json_object_add_i64
 * @category json
 * @brief    Adds a 64-bit signed int to a json object.
 * @related  CF_JVal cf_json_object_add cf_json_object_add_null cf_json_object_add_int cf_json_object_add_float cf_json_object_add_bool cf_json_object_add_string
 */
CF_API void CF_CALL cf_json_object_add_i64(CF_JDoc doc, CF_JVal obj, const char* key, int64_t val);

/**
 * @function cf_json_object_add_u64
 * @category json
 * @brief    Adds a 64-bit unsigned int to a json object.
 * @related  CF_JVal cf_json_object_add cf_json_object_add_null cf_json_object_add_int cf_json_object_add_float cf_json_object_add_bool cf_json_object_add_string
 */
CF_API void CF_CALL cf_json_object_add_u64(CF_JDoc doc, CF_JVal obj, const char* key, uint64_t val);

/**
 * @function cf_json_object_add_float
 * @category json
 * @brief    Adds a 32-bit float to a json object.
 * @related  CF_JVal cf_json_object_add cf_json_object_add_null cf_json_object_add_int cf_json_object_add_float cf_json_object_add_bool cf_json_object_add_string
 */
CF_API void CF_CALL cf_json_object_add_float(CF_JDoc doc, CF_JVal obj, const char* key, float val);

/**
 * @function cf_json_object_add_double
 * @category json
 * @brief    Adds a 64-bit float to a json object.
 * @related  CF_JVal cf_json_object_add cf_json_object_add_null cf_json_object_add_int cf_json_object_add_float cf_json_object_add_bool cf_json_object_add_string
 */
CF_API void CF_CALL cf_json_object_add_double(CF_JDoc doc, CF_JVal obj, const char* key, double val);

/**
 * @function cf_json_object_add_bool
 * @category json
 * @brief    Adds a bool to a json object.
 * @related  CF_JVal cf_json_object_add cf_json_object_add_null cf_json_object_add_int cf_json_object_add_float cf_json_object_add_bool cf_json_object_add_string
 */
CF_API void CF_CALL cf_json_object_add_bool(CF_JDoc doc, CF_JVal obj, const char* key, bool val);

/**
 * @function cf_json_object_add_string
 * @category json
 * @brief    Adds a string to a json object.
 * @related  CF_JVal cf_json_object_add cf_json_object_add_null cf_json_object_add_int cf_json_object_add_float cf_json_object_add_bool cf_json_object_add_string cf_json_object_add_string_range
 */
CF_API void CF_CALL cf_json_object_add_string(CF_JDoc doc, CF_JVal obj, const char* key, const char* val);

/**
 * @function cf_json_object_add_string_range
 * @category json
 * @brief    Adds a string to a json object.
 * @related  CF_JVal cf_json_object_add cf_json_object_add_null cf_json_object_add_int cf_json_object_add_float cf_json_object_add_bool cf_json_object_add_string cf_json_object_add_string_range
 */
CF_API void CF_CALL cf_json_object_add_string_range(CF_JDoc doc, CF_JVal obj, const char* key, const char* begin, const char* end);

//--------------------------------------------------------------------------------------------------
// Object manipulation.

/**
 * @function cf_json_object_remove_key
 * @category json
 * @brief    Removes a key/val pair from a json object.
 * @related  CF_JVal cf_json_object_remove_key cf_json_object_remove_key_range cf_json_object_rename_key cf_json_object_rename_key_range
 */
CF_API void CF_CALL cf_json_object_remove_key(CF_JVal obj, const char* key);

/**
 * @function cf_json_object_remove_key_range
 * @category json
 * @brief    Removes a key/val pair from a json object.
 * @related  CF_JVal cf_json_object_remove_key cf_json_object_remove_key_range cf_json_object_rename_key cf_json_object_rename_key_range
 */
CF_API void CF_CALL cf_json_object_remove_key_range(CF_JVal obj, const char* key_begin, const char* key_end);

/**
 * @function cf_json_object_rename_key
 * @category json
 * @brief    Renames a key/val pair from a json object.
 * @related  CF_JVal cf_json_object_remove_key cf_json_object_remove_key_range cf_json_object_rename_key cf_json_object_rename_key_range
 */
CF_API void CF_CALL cf_json_object_rename_key(CF_JDoc doc, CF_JVal obj, const char* key, const char* rename);

/**
 * @function cf_json_object_rename_key_range
 * @category json
 * @brief    Renames a key/val pair from a json object.
 * @related  CF_JVal cf_json_object_remove_key cf_json_object_remove_key_range cf_json_object_rename_key cf_json_object_rename_key_range
 */
CF_API void CF_CALL cf_json_object_rename_key_range(CF_JDoc doc, CF_JVal obj, const char* key_begin, const char* key_end, const char* rename_begin, const char* rename_end);

//--------------------------------------------------------------------------------------------------
// Write as string.

/**
 * @function cf_json_to_string
 * @category json
 * @brief    Saves the json document as a string.
 * @return   Returns a dynamic string, free it with `sfree` when done.
 * @remarks  If you want to remove all unnecessary formatting/whitespace then use `cf_json_to_string_minimal`.
 * @related  CF_JDoc cf_make_json cf_make_json_from_file cf_json_get_root cf_destroy_json cf_json_get_root cf_json_to_string cf_json_to_file cf_json_to_string_minimal cf_json_to_file_minimal
 */
CF_API dyna char* CF_CALL cf_json_to_string(CF_JDoc doc);

/**
 * @function cf_json_to_string_minimal
 * @category json
 * @brief    Saves the json document as a string.
 * @return   Returns a dynamic string, free it with `sfree` when done.
 * @related  CF_JDoc cf_make_json cf_make_json_from_file cf_json_get_root cf_destroy_json cf_json_get_root cf_json_to_string cf_json_to_file
 */
CF_API dyna char* CF_CALL cf_json_to_string_minimal(CF_JDoc doc);

/**
 * @function cf_json_to_file
 * @category json
 * @param    doc           The json document to save.
 * @param    virtual_path  A virtual path to the json file. Make sure to setup your write directory with `cf_fs_set_write_directory`. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @brief    Saves the json document to a file.
 * @remarks  If you want to remove all unnecessary formatting/whitespace then use `cf_json_to_file_minimal`.
 * @related  CF_JDoc cf_make_json cf_make_json_from_file cf_json_get_root cf_destroy_json cf_json_get_root cf_json_to_string cf_json_to_file cf_json_to_string_minimal cf_json_to_file_minimal
 */
CF_API CF_Result CF_CALL cf_json_to_file(CF_JDoc doc, const char* virtual_path);

/**
 * @function cf_json_to_file_minimal
 * @category json
 * @param    doc           The json document to save.
 * @param    virtual_path  A virtual path to the json file. Make sure to setup your write directory with `cf_fs_set_write_directory`. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @brief    Saves the json document to a file.
 * @related  CF_JDoc cf_make_json cf_make_json_from_file cf_json_get_root cf_destroy_json cf_json_get_root cf_json_to_string cf_json_to_file cf_json_to_string_minimal cf_json_to_file_minimal
 */
CF_API CF_Result CF_CALL cf_json_to_file_minimal(CF_JDoc doc, const char* virtual_path);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

using JType = CF_JType;

namespace Cute
{

struct JVal;

// JSON iterator, for traversing key-value properties on objects, or arrays.
// Traversals are done in one direction, linearly.
struct JIter
{
	CF_INLINE JIter(CF_JIter i, CF_JDoc d) { this->i = i; this->d = d; }

	bool done() const;
	const char* key() const;
	JVal val() const;

	JIter& next();
	JVal next(const char* key);
	JVal remove();

	// Only use this when iterating an array.
	CF_INLINE int index() const { return (int)i.index; }

private:
	CF_JDoc d;
	CF_JIter i;
};

// Represents a single JSON value, such as an integer, object, or array.
// Create these with JDoc::alloc_*** functions.
struct JVal
{
	CF_INLINE JType type() const { return cf_json_type(v); }
	CF_INLINE bool is_null() const { return cf_json_is_null(v); }
	CF_INLINE bool is_int() const { return cf_json_is_int(v); }
	CF_INLINE bool is_float() const { return cf_json_is_float(v); }
	CF_INLINE bool is_string() const { return cf_json_is_string(v); }
	CF_INLINE bool is_array() const { return cf_json_is_array(v); }
	CF_INLINE bool is_object() const { return cf_json_is_object(v); }

	CF_INLINE JVal get(const char* key) const { return JVal(cf_json_get(v, key), d); }
	CF_INLINE JVal get(int index) const { return JVal(cf_json_array_get(v, index), d); }
	CF_INLINE JVal at(int index) const { return JVal(cf_json_array_at(v, index), d); }
	CF_INLINE int get_int() const { return cf_json_get_int(v); }
	CF_INLINE int64_t get_i64() const { return cf_json_get_i64(v); }
	CF_INLINE uint64_t get_u64() const { return cf_json_get_u64(v); }
	CF_INLINE float get_float() const { return cf_json_get_float(v); }
	CF_INLINE double get_double() const { return cf_json_get_double(v); }
	CF_INLINE bool get_bool() const { return cf_json_get_bool(v); }
	CF_INLINE const char* get_string() const { return cf_json_get_string(v); }
	CF_INLINE int get_len() const { return cf_json_get_len(v); }
	CF_INLINE JIter iter() const { return JIter(cf_json_iter(v), d); }

	CF_INLINE void set_null() { cf_json_set_null(v); }
	CF_INLINE void set(int i) { cf_json_set_int(v, i); }
	CF_INLINE JVal& operator=(int i) { cf_json_set_int(v, i); return *this; }
	CF_INLINE void set(int64_t i) { cf_json_set_i64(v, i); }
	CF_INLINE JVal& operator=(int64_t i) { cf_json_set_i64(v, i); return *this; }
	CF_INLINE void set(uint64_t i) { cf_json_set_u64(v, i); }
	CF_INLINE JVal& operator=(uint64_t i) { cf_json_set_u64(v, i); return *this; }
	CF_INLINE void set(float i) { cf_json_set_float(v, i); }
	CF_INLINE JVal& operator=(float i) { cf_json_set_float(v, i); return *this; }
	CF_INLINE void set(double i) { cf_json_set_double(v, i); }
	CF_INLINE JVal& operator=(double i) { cf_json_set_double(v, i); return *this; }
	CF_INLINE void set(bool i) { cf_json_set_bool(v, i); }
	CF_INLINE JVal& operator=(bool i) { cf_json_set_bool(v, i); return *this; }
	CF_INLINE void set(const char* i) { cf_json_set_string(v, i); }
	CF_INLINE JVal& operator=(const char* i) { cf_json_set_string(v, i); return *this; }
	CF_INLINE void set(const char* begin, const char* end) { cf_json_set_string_range(v, begin, end); }

	// Array functions.
	CF_INLINE void add_null() { cf_json_array_add_null(d, v); }
	CF_INLINE void add(int val) { cf_json_array_add_int(d, v, val); }
	CF_INLINE void add(int64_t val) { cf_json_array_add_i64(d, v, val); }
	CF_INLINE void add(uint64_t val) { cf_json_array_add_u64(d, v, val); }
	CF_INLINE void add(float val) { cf_json_array_add_float(d, v, val); }
	CF_INLINE void add(double val) { cf_json_array_add_double(d, v, val); }
	CF_INLINE void add(bool val) { cf_json_array_add_bool(d, v, val); }
	CF_INLINE void add(const char* val) { cf_json_array_add_string(d, v, val); }
	CF_INLINE void add_range(const char* begin, const char* end) { cf_json_array_add_string_range(d, v, begin, end); }
	CF_INLINE void add(JVal val) { cf_json_array_add(v, val.v); }
	CF_INLINE JVal pop() { return JVal(cf_json_array_pop(v), d); }

	// Adding properties.
	CF_INLINE void add(const char* key, JVal val) { cf_json_object_add(d, v, key, val.v); }
	CF_INLINE void add_null(const char* key) { cf_json_object_add_null(d, v, key); }
	CF_INLINE void add(const char* key, int val) { cf_json_object_add_int(d, v, key, val); }
	CF_INLINE void add(const char* key, int64_t val) { cf_json_object_add_i64(d, v, key, val); }
	CF_INLINE void add(const char* key, uint64_t val) { cf_json_object_add_u64(d, v, key, val); }
	CF_INLINE void add(const char* key, float val) { cf_json_object_add_float(d, v, key, val); }
	CF_INLINE void add(const char* key, double val) { cf_json_object_add_double(d, v, key, val); }
	CF_INLINE void add(const char* key, bool val) { cf_json_object_add_bool(d, v, key, val); }
	CF_INLINE void add(const char* key, const char* val) { cf_json_object_add_string(d, v, key, val); }
	CF_INLINE void add(const char* key, const char* begin, const char* end) { cf_json_object_add_string_range(d, v, key, begin, end); }

	// Removing properties.
	CF_INLINE void remove(const char* key) { cf_json_object_remove_key(v, key); }
	CF_INLINE void remove(const char* key_begin, const char* key_end) { cf_json_object_remove_key_range(v, key_begin, key_end); }
	CF_INLINE void rename(const char* key, const char* rename) { cf_json_object_rename_key(d, v, key, rename); }
	CF_INLINE void rename(const char* key_begin, const char* key_end, const char* rename_begin, const char* rename_end) { cf_json_object_rename_key_range(d, v, key_begin, key_end, rename_begin, rename_end); }

private:
	CF_INLINE JVal(CF_JVal v, CF_JDoc d) { this->v = v; this->d = d; }
	CF_JDoc d;
	CF_JVal v;

	friend struct JDoc;
	friend struct JIter;
};

// A JSON document, capable of loading up JSON documents from disk or memory, and also editing
// the JSON in-memory. JVal's are created relative to one document, and cannot be directly transferred
// to another JDoc. Instead, use JVal::get_*** to fetch C-values out of the document, and then create
// new JVals in another document using JDoc::alloc_*** functions.
struct JDoc
{
	// Creating/loading documents. Be sure to call `destroy` when you're done.
	CF_INLINE static JDoc make() { return JDoc(cf_make_json(NULL, 0)); }
	CF_INLINE static JDoc make(const void* data, size_t size) { return JDoc(cf_make_json(data, size)); }
	CF_INLINE static JDoc make(const char* virtual_path) { return JDoc(cf_make_json_from_file(virtual_path)); }
	CF_INLINE static void destroy(JDoc doc) { cf_destroy_json(doc.d); }
	CF_INLINE void destroy() { cf_destroy_json(d); }

	CF_INLINE void set_root(JVal v) { cf_json_set_root(d, v.v); }
	CF_INLINE JVal root() const { return JVal(cf_json_get_root(d), d); }

	// Creating JSON values, JVal.
	CF_INLINE JVal alloc_null() { return JVal(cf_json_from_null(d), d); }
	CF_INLINE JVal alloc(int v) { return JVal(cf_json_from_int(d, v), d); }
	CF_INLINE JVal alloc(int64_t v) { return JVal(cf_json_from_i64(d, v), d); }
	CF_INLINE JVal alloc(uint64_t v) { return JVal(cf_json_from_u64(d, v), d); }
	CF_INLINE JVal alloc(float v) { return JVal(cf_json_from_float(d, v), d); }
	CF_INLINE JVal alloc(double v) { return JVal(cf_json_from_double(d, v), d); }
	CF_INLINE JVal alloc(bool v) { return JVal(cf_json_from_bool(d, v), d); }
	CF_INLINE JVal alloc(const char* v) { return JVal(cf_json_from_string(d, v), d); }
	CF_INLINE JVal alloc(const char* begin, const char* end) { return JVal(cf_json_from_string_range(d, begin, end), d); }

	CF_INLINE JVal alloc_array() { return JVal(cf_json_array(d), d); }
	CF_INLINE JVal alloc_array(int* vals, int count) { return JVal(cf_json_array_from_int(d, vals, count), d); }
	CF_INLINE JVal alloc_array(int64_t* vals, int count) { return JVal(cf_json_array_from_i64(d, vals, count), d); }
	CF_INLINE JVal alloc_array(uint64_t* vals, int count) { return JVal(cf_json_array_from_u64(d, vals, count), d); }
	CF_INLINE JVal alloc_array(float* vals, int count) { return JVal(cf_json_array_from_float(d, vals, count), d); }
	CF_INLINE JVal alloc_array(double* vals, int count) { return JVal(cf_json_array_from_double(d, vals, count), d); }
	CF_INLINE JVal alloc_array(bool* vals, int count) { return JVal(cf_json_array_from_bool(d, vals, count), d); }
	CF_INLINE JVal alloc_array(const char** vals, int count) { return JVal(cf_json_array_from_string(d, vals, count), d); }

	CF_INLINE JVal alloc_object() { return JVal(cf_json_object(d), d); }
	CF_INLINE JVal alloc_object(const char** keys, const char** vals, int count) { return JVal(cf_json_object_from_strings(d, keys, vals, count), d); }
	CF_INLINE JVal alloc_object(const char** kv_pairs, int pair_count) { return JVal(cf_json_object_from_string_pairs(d, kv_pairs, pair_count), d); }

	// Writing out the document to string or disk.
	CF_INLINE void to_file(const char* virtual_path) { cf_json_to_file(d, virtual_path); }
	CF_INLINE String to_string() { return String::steal_from(cf_json_to_string(d)); }

	// Minimal versions remove extra formatting and whitespace.
	CF_INLINE String to_string_minimal() { return String::steal_from(cf_json_to_string_minimal(d)); }
	CF_INLINE void to_file_minimal(const char* virtual_path) { cf_json_to_file_minimal(d, virtual_path); }

private:
	CF_INLINE JDoc(CF_JDoc d) { this->d = d; }
	CF_JDoc d;
};

// Inline implementations placed down here, as opposed to inside the class, to avoid circular reference compile errors.
CF_INLINE bool JIter::done() const { return cf_json_iter_done(i); }
CF_INLINE const char* JIter::key() const { return cf_json_iter_key(i); }
CF_INLINE JVal JIter::val() const { return JVal(cf_json_iter_val(i), d); }

CF_INLINE JIter& JIter::next() { i = cf_json_iter_next(i); return *this; }
CF_INLINE JVal JIter::next(const char* key) { return JVal(cf_json_iter_next_by_name(&i, key), d); }
CF_INLINE JVal JIter::remove() { return JVal(cf_json_iter_remove(&i), d); }

}

#endif // CF_CPP

#endif // CF_JSON_H
