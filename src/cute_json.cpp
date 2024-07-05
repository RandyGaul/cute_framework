/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "cute_json.h"
#include "cute_file_system.h"
#include "internal/yyjson.h"

#include <stddef.h>

CF_JDoc cf_make_json(const void* data, size_t size)
{
	yyjson_mut_doc* doc = NULL;
	if (data) {
		yyjson_read_flag flags = YYJSON_READ_ALLOW_TRAILING_COMMAS | YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_INF_AND_NAN | YYJSON_READ_ALLOW_INVALID_UNICODE;
		yyjson_doc* read_only_doc = yyjson_read((char*)data, size, flags);
		doc = yyjson_doc_mut_copy(read_only_doc, NULL);
		yyjson_doc_free(read_only_doc);
	} else {
		doc = yyjson_mut_doc_new(NULL);
	}
	CF_JDoc result = { (uint64_t)doc };
	return result;
}

CF_JDoc cf_make_json_from_file(const char* virtual_path)
{
	CF_JDoc result = { 0 };
	size_t size;
	char* file = cf_fs_read_entire_file_to_memory_and_nul_terminate(virtual_path, &size);
	if (!file) return result;
	result = cf_make_json(file, CF_STRLEN(file));
	cf_free(file);
	return result;
}

void cf_destroy_json(CF_JDoc doc_handle)
{
	yyjson_mut_doc_free((yyjson_mut_doc*)doc_handle.id);
}

CF_JVal cf_json_get_root(CF_JDoc doc_handle)
{
	CF_JVal result = { (uint64_t)yyjson_mut_doc_get_root((yyjson_mut_doc*)doc_handle.id) };
	return result;
}

void cf_json_set_root(CF_JDoc doc_handle, CF_JVal val)
{
	yyjson_mut_doc_set_root((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)val.id);
}

CF_JType cf_json_type(CF_JVal val_handle)
{
	yyjson_mut_val* val = (yyjson_mut_val*)val_handle.id;
	yyjson_type type = yyjson_mut_get_type(val);
	switch (type) {
	case YYJSON_TYPE_NULL: return CF_JTYPE_NULL;
	case YYJSON_TYPE_RAW:  // fall-thru
	case YYJSON_TYPE_STR:  return CF_JTYPE_STRING;
	case YYJSON_TYPE_NUM:  if (yyjson_mut_is_int(val)) return CF_JTYPE_INT;
	                       else                        return CF_JTYPE_FLOAT;
	case YYJSON_TYPE_BOOL: return CF_JTYPE_BOOL;
	case YYJSON_TYPE_ARR:  return CF_JTYPE_ARRAY;
	case YYJSON_TYPE_OBJ:  return CF_JTYPE_OBJECT;
	default:               return CF_JTYPE_NONE;
	}
}

CF_API bool CF_CALL cf_json_is_null(CF_JVal val_handle)
{
	return yyjson_mut_is_null((yyjson_mut_val*)val_handle.id);
}

CF_API bool CF_CALL cf_json_is_int(CF_JVal val_handle)
{
	return yyjson_mut_is_int((yyjson_mut_val*)val_handle.id);
}

CF_API bool CF_CALL cf_json_is_float(CF_JVal val_handle)
{
	return yyjson_mut_is_real((yyjson_mut_val*)val_handle.id);
}

CF_API bool CF_CALL cf_json_is_bool(CF_JVal val_handle)
{
	return yyjson_mut_is_bool((yyjson_mut_val*)val_handle.id);
}

CF_API bool CF_CALL cf_json_is_string(CF_JVal val_handle)
{
	return yyjson_mut_is_str((yyjson_mut_val*)val_handle.id);
}

CF_API bool CF_CALL cf_json_is_array(CF_JVal val_handle)
{
	return yyjson_mut_is_arr((yyjson_mut_val*)val_handle.id);
}

CF_API bool CF_CALL cf_json_is_object(CF_JVal val_handle)
{
	return yyjson_mut_is_obj((yyjson_mut_val*)val_handle.id);
}

int cf_json_get_int(CF_JVal val_handle)
{
	yyjson_mut_val* val = (yyjson_mut_val*)val_handle.id;
	if (yyjson_mut_is_num(val)) {
		if (yyjson_mut_is_real(val)) {
			return (int)yyjson_mut_get_real(val);
		} else {
			return (int)yyjson_mut_get_int(val);
		}
	} else if (yyjson_mut_is_bool(val)) {
		return (int)yyjson_mut_get_bool(val);
	} else {
		return 0;
	}
}

int64_t cf_json_get_i64(CF_JVal val_handle)
{
	yyjson_mut_val* val = (yyjson_mut_val*)val_handle.id;
	if (yyjson_mut_is_num(val)) {
		if (yyjson_mut_is_real(val)) {
			return (int64_t)yyjson_mut_get_real(val);
		} else {
			return (int64_t)yyjson_mut_get_sint(val);
		}
	} else if (yyjson_mut_is_bool(val)) {
		return (int64_t)yyjson_mut_get_bool(val);
	} else {
		return 0;
	}
}

uint64_t cf_json_get_u64(CF_JVal val_handle)
{
	yyjson_mut_val* val = (yyjson_mut_val*)val_handle.id;
	if (yyjson_mut_is_num(val)) {
		if (yyjson_mut_is_real(val)) {
			return (uint64_t)yyjson_mut_get_real(val);
		} else {
			return (uint64_t)yyjson_mut_get_uint(val);
		}
	} else if (yyjson_mut_is_bool(val)) {
		return (uint64_t)yyjson_mut_get_bool(val);
	} else {
		return 0;
	}
}

float cf_json_get_float(CF_JVal val_handle)
{
	yyjson_mut_val* val = (yyjson_mut_val*)val_handle.id;
	if (yyjson_mut_is_num(val)) {
		if (yyjson_mut_is_real(val)) {
			return (float)yyjson_mut_get_real(val);
		} else {
			return (float)yyjson_mut_get_int(val);
		}
	} else if (yyjson_mut_is_bool(val)) {
		return (float)yyjson_mut_get_bool(val);
	} else {
		return 0;
	}
}

double cf_json_get_double(CF_JVal val_handle)
{
	yyjson_mut_val* val = (yyjson_mut_val*)val_handle.id;
	if (yyjson_mut_is_num(val)) {
		if (yyjson_mut_is_real(val)) {
			return (double)yyjson_mut_get_real(val);
		} else {
			return (double)yyjson_mut_get_int(val);
		}
	} else if (yyjson_mut_is_bool(val)) {
		return (double)yyjson_mut_get_bool(val);
	} else {
		return 0;
	}
}

bool cf_json_get_bool(CF_JVal val_handle)
{
	yyjson_mut_val* val = (yyjson_mut_val*)val_handle.id;
	if (yyjson_mut_is_num(val)) {
		if (yyjson_mut_is_real(val)) {
			return (bool)yyjson_mut_get_real(val);
		} else {
			return (bool)yyjson_mut_get_int(val);
		}
	} else if (yyjson_mut_is_bool(val)) {
		return (bool)yyjson_mut_get_bool(val);
	} else {
		return 0;
	}
}

const char* cf_json_get_string(CF_JVal val_handle)
{
	return yyjson_mut_get_str((yyjson_mut_val*)val_handle.id);
}

int cf_json_get_len(CF_JVal val_handle)
{
	return (int)yyjson_mut_get_len((yyjson_mut_val*)val_handle.id);
}

CF_JVal cf_json_get(CF_JVal val_handle, const char* key)
{
	CF_JVal result = { (uint64_t)yyjson_mut_obj_get((yyjson_mut_val*)val_handle.id, key) };
	return result;
}

CF_JVal cf_json_array_at(CF_JVal val_handle, int index)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_get((yyjson_mut_val*)val_handle.id, index) };
	return result;
}

CF_JVal cf_json_array_get(CF_JVal val_handle, int index)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_get((yyjson_mut_val*)val_handle.id, index) };
	return result;
}

// Make sure memory layout is identical.
static_assert(sizeof(CF_JIter) == sizeof(yyjson_mut_arr_iter));
static_assert(offsetof(CF_JIter, index) == offsetof(yyjson_mut_arr_iter, idx));
static_assert(offsetof(CF_JIter, count) == offsetof(yyjson_mut_arr_iter, max));
static_assert(offsetof(CF_JIter, val) == offsetof(yyjson_mut_arr_iter, cur));
static_assert(offsetof(CF_JIter, prev) == offsetof(yyjson_mut_arr_iter, pre));
static_assert(offsetof(CF_JIter, parent) == offsetof(yyjson_mut_arr_iter, arr));
static_assert(sizeof(CF_JIter) == sizeof(yyjson_mut_obj_iter));
static_assert(offsetof(CF_JIter, index) == offsetof(yyjson_mut_obj_iter, idx));
static_assert(offsetof(CF_JIter, count) == offsetof(yyjson_mut_obj_iter, max));
static_assert(offsetof(CF_JIter, val) == offsetof(yyjson_mut_obj_iter, cur));
static_assert(offsetof(CF_JIter, prev) == offsetof(yyjson_mut_obj_iter, pre));
static_assert(offsetof(CF_JIter, parent) == offsetof(yyjson_mut_obj_iter, obj));

CF_JIter cf_json_iter(CF_JVal val_handle)
{
	yyjson_mut_val* val = (yyjson_mut_val*)val_handle.id;
	CF_JIter iter = { 0 };
	if (yyjson_mut_is_arr(val)) {
		yyjson_mut_arr_iter* i = (yyjson_mut_arr_iter*)&iter;
		i->max = unsafe_yyjson_get_len(val);
		i->pre = i->max ? (yyjson_mut_val *)val->uni.ptr : NULL;
		i->cur = i->pre ? i->pre->next : NULL;
		i->arr = val;
	} else if (yyjson_mut_is_obj(val)) {
		yyjson_mut_obj_iter* i = (yyjson_mut_obj_iter*)&iter;
		i->max = unsafe_yyjson_get_len(val);
		i->pre = i->max ? (yyjson_mut_val *)val->uni.ptr : NULL;
		i->cur = i->pre ? i->pre->next->next : NULL;
		i->obj = val;
	}
	return iter;
}

CF_JIter cf_json_iter_next(CF_JIter iter)
{
	yyjson_mut_val* parent = (yyjson_mut_val*)iter.parent.id;
	if (yyjson_mut_is_arr(parent)) {
		yyjson_mut_arr_iter_next((yyjson_mut_arr_iter*)&iter);
	} else if (yyjson_mut_is_obj(parent)) {
		yyjson_mut_obj_iter_next((yyjson_mut_obj_iter*)&iter);
	}
	return iter;
}

CF_JVal cf_json_iter_next_by_name(CF_JIter* iter, const char* key)
{
	yyjson_mut_val* parent = (yyjson_mut_val*)iter->parent.id;
	CF_JVal result = { 0 };
	if (yyjson_mut_is_obj(parent)) {
		result.id = (uint64_t)yyjson_mut_obj_iter_get((yyjson_mut_obj_iter*)iter, key);
	}
	return result;
}

CF_JVal cf_json_iter_remove(CF_JIter* iter)
{
	CF_JVal result = { 0 };
	if (yyjson_mut_is_arr((yyjson_mut_val*)iter->parent.id)) {
		result = { (uint64_t)yyjson_mut_arr_iter_remove((yyjson_mut_arr_iter*)iter) };
	} else if (yyjson_mut_is_obj((yyjson_mut_val*)iter->parent.id)) {
		result = { (uint64_t)yyjson_mut_obj_iter_remove((yyjson_mut_obj_iter*)iter) };
	}
	return result;
}

CF_JVal cf_json_iter_val(CF_JIter iter)
{
	yyjson_mut_val* parent = (yyjson_mut_val*)iter.parent.id;
	CF_JVal val = { 0 };
	if (yyjson_mut_is_arr(parent)) {
		val.id = (uint64_t)((yyjson_mut_arr_iter*)&iter)->cur;
	} else if (yyjson_mut_is_obj(parent)) {
		// Objects are stored as k,v pairs in a list; the key is cur, the val is cur->next.
		val.id = (uint64_t)((yyjson_mut_obj_iter*)&iter)->cur->next;
	}
	return val;
}

const char* cf_json_iter_key(CF_JIter iter)
{
	yyjson_mut_val* parent = (yyjson_mut_val*)iter.parent.id;
	if (yyjson_mut_is_obj(parent)) {
		return yyjson_mut_get_str(((yyjson_mut_arr_iter*)&iter)->cur);
	}
	return NULL;
}

CF_JVal cf_json_from_null(CF_JDoc doc_handle)
{
	CF_JVal result = { (uint64_t)yyjson_mut_null((yyjson_mut_doc*)doc_handle.id) };
	return result;
}

CF_JVal cf_json_from_int(CF_JDoc doc_handle, int val)
{
	CF_JVal result = { (uint64_t)yyjson_mut_int((yyjson_mut_doc*)doc_handle.id, (int64_t)val) };
	return result;
}

CF_JVal cf_json_from_i64(CF_JDoc doc_handle, int64_t val)
{
	CF_JVal result = { (uint64_t)yyjson_mut_sint((yyjson_mut_doc*)doc_handle.id, val) };
	return result;
}

CF_JVal cf_json_from_u64(CF_JDoc doc_handle, uint64_t val)
{
	CF_JVal result = { (uint64_t)yyjson_mut_uint((yyjson_mut_doc*)doc_handle.id, val) };
	return result;
}

CF_JVal cf_json_from_float(CF_JDoc doc_handle, float val)
{
	CF_JVal result = { (uint64_t)yyjson_mut_real((yyjson_mut_doc*)doc_handle.id, (double)val) };
	return result;
}

CF_JVal cf_json_from_double(CF_JDoc doc_handle, double val)
{
	CF_JVal result = { (uint64_t)yyjson_mut_real((yyjson_mut_doc*)doc_handle.id, val) };
	return result;
}

CF_JVal cf_json_from_bool(CF_JDoc doc_handle, bool val)
{
	yyjson_mut_doc* doc = (yyjson_mut_doc*)doc_handle.id;
	CF_JVal result = { (uint64_t)(val ? yyjson_mut_true(doc) : yyjson_mut_false(doc)) };
	return result;
}

CF_JVal cf_json_from_string(CF_JDoc doc_handle, const char* val)
{
	CF_JVal result = { (uint64_t)yyjson_mut_str((yyjson_mut_doc*)doc_handle.id, val) };
	return result;
}

CF_JVal cf_json_from_string_range(CF_JDoc doc_handle, const char* begin, const char* end)
{
	CF_JVal result = { (uint64_t)yyjson_mut_strn((yyjson_mut_doc*)doc_handle.id, begin, end - begin) };
	return result;
}

void cf_json_set_null(CF_JVal jval)
{
	yyjson_mut_set_null((yyjson_mut_val*)jval.id);
}

void cf_json_set_int(CF_JVal jval, int val)
{
	yyjson_mut_set_int((yyjson_mut_val*)jval.id, (int64_t)val);
}

void cf_json_set_i64(CF_JVal jval, int64_t val)
{
	yyjson_mut_set_sint((yyjson_mut_val*)jval.id, val);
}

void cf_json_set_u64(CF_JVal jval, uint64_t val)
{
	yyjson_mut_set_uint((yyjson_mut_val*)jval.id, val);
}

void cf_json_set_float(CF_JVal jval, float val)
{
	yyjson_mut_set_real((yyjson_mut_val*)jval.id, (double)val);
}

void cf_json_set_double(CF_JVal jval, double val)
{
	yyjson_mut_set_real((yyjson_mut_val*)jval.id, val);
}

void  cf_json_set_bool(CF_JVal jval, bool val)
{
	yyjson_mut_set_bool((yyjson_mut_val*)jval.id, val);
}

void cf_json_set_string(CF_JVal jval, const char* val)
{
	yyjson_mut_set_str((yyjson_mut_val*)jval.id, val);
}

void cf_json_set_string_range(CF_JVal jval, const char* begin, const char* end)
{
	yyjson_mut_set_strn((yyjson_mut_val*)jval.id, begin, end - begin);
}

CF_JVal cf_json_array(CF_JDoc doc_handle)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr((yyjson_mut_doc*)doc_handle.id) };
	return result;
}

CF_JVal cf_json_array_from_int(CF_JDoc doc_handle, int* vals, int count)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_with_sint32((yyjson_mut_doc*)doc_handle.id, vals, count) };
	return result;
}

CF_JVal cf_json_array_from_i64(CF_JDoc doc_handle, int64_t* vals, int count)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_with_sint((yyjson_mut_doc*)doc_handle.id, vals, count) };
	return result;
}

CF_JVal cf_json_array_from_u64(CF_JDoc doc_handle, uint64_t* vals, int count)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_with_uint((yyjson_mut_doc*)doc_handle.id, vals, count) };
	return result;
}

CF_JVal cf_json_array_from_float(CF_JDoc doc_handle, float* vals, int count)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_with_float((yyjson_mut_doc*)doc_handle.id, vals, count) };
	return result;
}

CF_JVal cf_json_array_from_double(CF_JDoc doc_handle, double* vals, int count)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_with_double((yyjson_mut_doc*)doc_handle.id, vals, count) };
	return result;
}

CF_JVal cf_json_array_from_bool(CF_JDoc doc_handle, bool* vals, int count)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_with_bool((yyjson_mut_doc*)doc_handle.id, vals, count) };
	return result;
}

CF_JVal cf_json_array_from_string(CF_JDoc doc_handle, const char** vals, int count)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_with_str((yyjson_mut_doc*)doc_handle.id, vals, count) };
	return result;
}

void cf_json_array_add(CF_JVal arr, CF_JVal val)
{
	yyjson_mut_arr_add_val((yyjson_mut_val*)arr.id, (yyjson_mut_val*)val.id);
}

void cf_json_array_add_null(CF_JDoc doc_handle, CF_JVal arr_handle)
{
	yyjson_mut_arr_add_null((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id);
}

void cf_json_array_add_int(CF_JDoc doc_handle, CF_JVal arr_handle, int val)
{
	yyjson_mut_arr_add_int((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id, (int64_t)val);
}

void cf_json_array_add_i64(CF_JDoc doc_handle, CF_JVal arr_handle, int64_t val)
{
	yyjson_mut_arr_add_sint((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id, val);
}

void cf_json_array_add_u64(CF_JDoc doc_handle, CF_JVal arr_handle, uint64_t val)
{
	yyjson_mut_arr_add_uint((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id, val);
}

void cf_json_array_add_float(CF_JDoc doc_handle, CF_JVal arr_handle, float val)
{
	yyjson_mut_arr_add_real((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id, (double)val);
}

void cf_json_array_add_double(CF_JDoc doc_handle, CF_JVal arr_handle, double val)
{
	yyjson_mut_arr_add_real((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id, val);
}

void cf_json_array_add_bool(CF_JDoc doc_handle, CF_JVal arr_handle, bool val)
{
	if (val) {
		yyjson_mut_arr_add_true((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id);
	} else {
		yyjson_mut_arr_add_false((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id);
	}
}

void cf_json_array_add_string(CF_JDoc doc_handle, CF_JVal arr_handle, const char* val)
{
	yyjson_mut_arr_add_str((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id, val);
}

void cf_json_array_add_string_range(CF_JDoc doc_handle, CF_JVal arr_handle, const char* begin, const char* end)
{
	yyjson_mut_arr_add_strn((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id, begin, end - begin);
}

CF_JVal cf_json_array_add_array(CF_JDoc doc_handle, CF_JVal arr_handle)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_add_arr((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id) };
	return result;
}

CF_JVal cf_json_array_add_object(CF_JDoc doc_handle, CF_JVal arr_handle)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_add_obj((yyjson_mut_doc*)doc_handle.id, (yyjson_mut_val*)arr_handle.id) };
	return result;
}

CF_JVal cf_json_array_pop(CF_JVal arr)
{
	CF_JVal result = { (uint64_t)yyjson_mut_arr_remove_last((yyjson_mut_val*)arr.id) };
	return result;
}

CF_JVal cf_json_object(CF_JDoc doc_handle)
{
	CF_JVal result = { (uint64_t)yyjson_mut_obj((yyjson_mut_doc*)doc_handle.id) };
	return result;
}

CF_JVal cf_json_object_from_strings(CF_JDoc doc_handle, const char** keys, const char** vals, int count)
{
	CF_JVal result = { (uint64_t)yyjson_mut_obj_with_str((yyjson_mut_doc*)doc_handle.id, keys, vals, count) };
	return result;
}

CF_JVal cf_json_object_from_string_pairs(CF_JDoc doc_handle, const char** kv_pairs, int pair_count)
{
	CF_JVal result = { (uint64_t)yyjson_mut_obj_with_kv((yyjson_mut_doc*)doc_handle.id, kv_pairs, pair_count) };
	return result;
}

void cf_json_object_add(CF_JDoc doc, CF_JVal obj, const char* key, CF_JVal val)
{
	CF_JVal k = cf_json_from_string(doc, key);
	yyjson_mut_obj_add((yyjson_mut_val*)obj.id, (yyjson_mut_val*)k.id, (yyjson_mut_val*)val.id);
}

void cf_json_object_add_null(CF_JDoc doc, CF_JVal obj, const char* key)
{
	yyjson_mut_obj_add_null((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key);
}

void cf_json_object_add_int(CF_JDoc doc, CF_JVal obj, const char* key, int val)
{
	yyjson_mut_obj_add_int((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key, (int64_t)val);
}

void cf_json_object_add_i64(CF_JDoc doc, CF_JVal obj, const char* key, int64_t val)
{
	yyjson_mut_obj_add_sint((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key, val);
}

void cf_json_object_add_u64(CF_JDoc doc, CF_JVal obj, const char* key, uint64_t val)
{
	yyjson_mut_obj_add_uint((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key, val);
}

void cf_json_object_add_float(CF_JDoc doc, CF_JVal obj, const char* key, float val)
{
	yyjson_mut_obj_add_real((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key, (double)val);
}

void cf_json_object_add_double(CF_JDoc doc, CF_JVal obj, const char* key, double val)
{
	yyjson_mut_obj_add_real((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key, val);
}

void cf_json_object_add_bool(CF_JDoc doc, CF_JVal obj, const char* key, bool val)
{
	if (val) {
		yyjson_mut_obj_add_true((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key);
	} else {
		yyjson_mut_obj_add_false((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key);
	}
}

void cf_json_object_add_string(CF_JDoc doc, CF_JVal obj, const char* key, const char* val)
{
	yyjson_mut_obj_add_str((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key, val);
}

void cf_json_object_add_string_range(CF_JDoc doc, CF_JVal obj, const char* key, const char* begin, const char* end)
{
	yyjson_mut_obj_add_strn((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key, begin, end - begin);
}

void cf_json_object_remove_key(CF_JVal obj, const char* key)
{
	yyjson_mut_obj_remove_key((yyjson_mut_val*)obj.id, key);
}

void cf_json_object_remove_key_range(CF_JVal obj, const char* key_begin, const char* key_end)
{
	yyjson_mut_obj_remove_keyn((yyjson_mut_val*)obj.id, key_begin, key_end - key_begin);
}

void cf_json_object_rename_key(CF_JDoc doc, CF_JVal obj, const char* key, const char* rename)
{
	yyjson_mut_obj_rename_key((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key, rename);
}

void cf_json_object_rename_key_range(CF_JDoc doc, CF_JVal obj, const char* key_begin, const char* key_end, const char* rename_begin, const char* rename_end)
{
	yyjson_mut_obj_rename_keyn((yyjson_mut_doc*)doc.id, (yyjson_mut_val*)obj.id, key_begin, key_end - key_begin, rename_begin, rename_end - rename_begin);
}

dyna char* cf_json_to_string(CF_JDoc doc)
{
	yyjson_write_flag flags = YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_ALLOW_INF_AND_NAN | YYJSON_WRITE_ALLOW_INVALID_UNICODE;
	char* string = yyjson_mut_write((yyjson_mut_doc*)doc.id, flags, NULL);
	char* result = NULL;
	sset(result, string);
	free(string);
	sreplace(result, "  ", "\t");
	return result;
}

dyna char* cf_json_to_string_minimal(CF_JDoc doc)
{
	yyjson_write_flag flags = YYJSON_WRITE_ALLOW_INF_AND_NAN | YYJSON_WRITE_ALLOW_INVALID_UNICODE;
	char* string = yyjson_mut_write((yyjson_mut_doc*)doc.id, flags, NULL);
	char* result = NULL;
	sset(result, string);
	free(string);
	return result;
}

CF_Result cf_json_to_file(CF_JDoc doc, const char* virtual_path)
{
	char* s = cf_json_to_string(doc);
	CF_Result result = cf_fs_write_string_to_file(virtual_path, s);
	sfree(s);
	return result;
}

CF_Result cf_json_to_file_minimal(CF_JDoc doc, const char* virtual_path)
{
	char* s = cf_json_to_string_minimal(doc);
	CF_Result result = cf_fs_write_string_to_file(virtual_path, s);
	sfree(s);
	return result;
}
