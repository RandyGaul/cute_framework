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

#include <cute_kv.h>
#include <cute_c_runtime.h>
#include <cute_base64.h>
#include <cute_array.h>
#include <cute_result.h>
#include <cute_string.h>

#include <stdio.h>
#include <inttypes.h>

using namespace cute;

struct cf_kv_string_t
{
	uint8_t* str = NULL;
	size_t len = 0;
};

union cf_kv_union_t
{
	cf_kv_union_t() {}

	int64_t ival;
	double dval;
	cf_kv_string_t sval;
	cf_kv_string_t bval;
	int object_index;
};

struct cf_kv_val_t
{
	cf_kv_type_t type = CF_KV_TYPE_NULL;
	cf_kv_union_t u;
	array<cf_kv_val_t> aval;
};

struct cf_kv_field_t
{
	cf_kv_string_t key;
	cf_kv_val_t val;
};

struct cf_kv_object_t
{
	int parent_index = ~0;
	int parsing_array = 0;

	cf_kv_string_t key;
	array<cf_kv_field_t> fields;
};

#define CUTE_KV_NOT_IN_ARRAY               0
#define CUTE_KV_IN_ARRAY                   1
#define CUTE_KV_IN_ARRAY_AND_FIRST_ELEMENT 2

struct cf_kv_cache_t
{
	cf_kv_t* kv = NULL;
	int object_index = 0;
};

struct cf_kv_t
{
	cf_kv_state_t mode;
	uint8_t* in = NULL;
	uint8_t* in_end = NULL;
	uint8_t* start = NULL;
	string_t write_buffer;

	// Reading state.
	int object_skip_count = 0;
	cf_kv_val_t* matched_val = NULL;
	int matched_cache_index = ~0;
	cf_kv_val_t* matched_cache_val = NULL;
	array<cf_kv_cache_t> cache;
	array<cf_kv_object_t> objects;

	int read_mode_from_array = 0;
	array<cf_kv_val_t*> read_mode_array_stack;
	array<int> read_mode_array_index_stack;

	// Writing state.
	size_t backup_base_key_bytes = 0;
	cf_kv_t* base = NULL;
	int in_array = CUTE_KV_NOT_IN_ARRAY;
	array<int> in_array_stack;
	int tabs = 0;

	CF_Result last_err = cf_result_success();
};

static cf_kv_t* s_kv()
{
	cf_kv_t* kv = (cf_kv_t*)CUTE_ALLOC(sizeof(cf_kv_t));
	CUTE_PLACEMENT_NEW(kv) cf_kv_t;

	cf_kv_cache_t cache;
	cache.kv = kv;
	kv->cache.add(cache);

	return kv;
}

void cf_kv_destroy(cf_kv_t* kv)
{
	if (kv) {
		kv->~cf_kv_t();
		CUTE_FREE(kv);
	}
}

static CUTE_INLINE void s_push_array(cf_kv_t* kv, int in_array)
{
	kv->in_array_stack.add(kv->in_array);
	kv->in_array = in_array;
}

static CUTE_INLINE void s_pop_array(cf_kv_t* kv)
{
	kv->in_array = kv->in_array_stack.pop();
}

static CUTE_INLINE void s_push_read_mode_array(cf_kv_t* kv, cf_kv_val_t* val)
{
	kv->read_mode_array_stack.add(val);
	kv->read_mode_array_index_stack.add(0);
	kv->read_mode_from_array = val ? 1 : 0;
}

static CUTE_INLINE void s_pop_read_mode_array(cf_kv_t* kv)
{
	kv->read_mode_array_stack.pop();
	kv->read_mode_array_index_stack.pop();
	if (kv->read_mode_array_stack.count()) {
		kv->read_mode_from_array = kv->read_mode_array_stack.last() ? 1 : 0;
	} else {
		kv->read_mode_from_array = 0;
	}
}

static CUTE_INLINE int s_isspace(uint8_t c)
{
	return (c == ' ') |
		(c == '\t') |
		(c == '\n') |
		(c == '\v') |
		(c == '\f') |
		(c == '\r');
}

static CUTE_INLINE void s_skip_white(cf_kv_t* kv)
{
	while (kv->in < kv->in_end && s_isspace(*kv->in)) kv->in++;
}

static CUTE_INLINE uint8_t s_peek(cf_kv_t* kv)
{
	while (kv->in < kv->in_end && s_isspace(*kv->in)) kv->in++;
	return kv->in < kv->in_end ? *kv->in : 0;
}

static CUTE_INLINE uint8_t s_next(cf_kv_t* kv)
{
	uint8_t c;
	if (kv->in == kv->in_end) return 0;
	while (s_isspace(c = *kv->in++)) if (kv->in == kv->in_end) return 0;
	return c;
}

static CUTE_INLINE int s_try(cf_kv_t* kv, uint8_t expect)
{
	if (kv->in == kv->in_end) return 0;
	if (s_peek(kv) == expect) {
		kv->in++;
		return 1;
	}
	return 0;
}

#define s_expect(kv, expected_character) \
	do { \
		if (s_next(kv) != expected_character) { kv->last_err = cf_result_error("Found unexpected token."); return kv->last_err; } \
	} while (0)

static CF_Result s_scan_string(cf_kv_t* kv, uint8_t** start_of_string, uint8_t** end_of_string)
{
	*start_of_string = NULL;
	*end_of_string = NULL;
	int has_quotes = s_try(kv, '"');
	*start_of_string = kv->in;
	int terminated = 0;
	if (has_quotes) {
		while (kv->in < kv->in_end) {
			uint8_t* end = (uint8_t*)CUTE_MEMCHR(kv->in, '"', kv->in_end - kv->in);
			if (*(end - 1) != '\\') {
				*end_of_string = end;
				kv->in = end + 1;
				terminated = 1;
				break;
			}
		}
	} else {
		uint8_t* end = kv->in;
		while (end < kv->in_end && !s_isspace(*end)) end++;
		*end_of_string = end;
		kv->in = end + 1;
	}
	if (!terminated && kv->in == kv->in_end) {
		kv->last_err = cf_result_error("Unterminated string at end of file.");
		return kv->last_err;
	}
	return cf_result_success();
}

static CF_Result s_scan_string(cf_kv_t* kv, cf_kv_string_t* str)
{
	uint8_t* string_start;
	uint8_t* string_end;
	CF_Result err = s_scan_string(kv, &string_start, &string_end);
	str->str = string_start;
	str->len = (int)(string_end - string_start);
	return err;
}

static CUTE_INLINE CF_Result s_parse_int(cf_kv_t* kv, int64_t* out)
{
	uint8_t* end;
	int64_t val = CUTE_STRTOLL((char*)kv->in, (char**)&end, 10);
	if (kv->in == end) {
		kv->last_err = cf_result_error("Invalid integer found during parse.");
		return kv->last_err;
	}
	kv->in = end;
	*out = val;
	return cf_result_success();
}

static CUTE_INLINE CF_Result s_parse_float(cf_kv_t* kv, double* out)
{
	uint8_t* end;
	double val = CUTE_STRTOD((char*)kv->in, (char**)&end);
	if (kv->in == end) {
		kv->last_err = cf_result_error("Invalid float found during parse.");
		return kv->last_err;
	}
	kv->in = end;
	*out = val;
	return cf_result_success();
}

static CF_Result s_parse_hex(cf_kv_t* kv, uint64_t* hex)
{
	s_expect(kv, '0');
	uint8_t c = s_next(kv);
	if (!(c != 'x' && c != 'X')) {
		kv->last_err = cf_result_error("Expected 'x' or 'X' when parsing a hex number.");
		return kv->last_err;
	}
	uint8_t* end;
	uint64_t val = (uint64_t)CUTE_STRTOLL((char*)kv->in, (char**)&end, 16);
	if (kv->in == end) {
		kv->last_err = cf_result_error("Invalid hex integer found during parse.");
		return kv->last_err;
	}
	kv->in = end;
	*hex = val;
	return cf_result_success();
}

static CUTE_INLINE CF_Result s_parse_number(cf_kv_t* kv, cf_kv_val_t* val)
{
	CF_Result err;
	if (kv->in + 1 < kv->in_end && ((kv->in[1] == 'x') | (kv->in[1] == 'X'))) {
		uint64_t hex;
		err = s_parse_hex(kv, &hex);
		if (cf_is_error(err)) return err;
		val->type = CF_KV_TYPE_INT64;
		val->u.ival = (int64_t)hex;
	} else {
		uint8_t c;
		uint8_t* s = kv->in;
		int is_float = 0;
		while (s < kv->in_end && (c = *s++) != ',') {
			if (c == '.') {
				is_float = 1;
				break;
			}
		}

		if (is_float) {
			double dval;
			err = s_parse_float(kv, &dval);
			if (cf_is_error(err)) return err;
			val->type = CF_KV_TYPE_DOUBLE;
			val->u.dval = dval;
		} else {
			int64_t ival;
			err = s_parse_int(kv, &ival);
			if (cf_is_error(err)) return err;
			val->type = CF_KV_TYPE_INT64;
			val->u.ival = ival;
		}
	}
	return cf_result_success();
}

static CF_Result s_parse_value(cf_kv_t* kv, cf_kv_val_t* val);

static CF_Result s_parse_array(cf_kv_t* kv, array<cf_kv_val_t>* array_val)
{
	CF_Result err;
	int64_t count;
	s_expect(kv, '[');
	err = s_parse_int(kv, &count);
	if (cf_is_error(err)) return err;
	s_expect(kv, ']');
	s_expect(kv, '{');
	array_val->ensure_capacity((int)count);
	for (int i = 0; i < (int)count; ++i) {
		cf_kv_val_t* val = &array_val->add();
		CUTE_PLACEMENT_NEW(val) cf_kv_val_t;
		err = s_parse_value(kv, val);
		if (cf_is_error(err)) {
			return cf_result_error("Unexecpted value when parsing an array. Make sure the elements are well-formed, and the length is correct.");
		}
	}
	s_expect(kv, '}');
	return cf_result_success();
}

static CF_Result s_parse_object(cf_kv_t* kv, int* index, bool is_top_level = false);

static CF_Result s_parse_value(cf_kv_t* kv, cf_kv_val_t* val)
{
	CF_Result err;
	uint8_t c = s_peek(kv);

	if (c == '"') {
		cf_kv_string_t string;
		err = s_scan_string(kv, &string);
		if (cf_is_error(err)) return err;
		val->type = CF_KV_TYPE_STRING;
		val->u.sval = string;
	} else if ((c >= '0' && c <= '9') | (c == '-')) {
		err = s_parse_number(kv, val);
		if (cf_is_error(err)) return err;
	} else if (c == '[') {
		err = s_parse_array(kv, &val->aval);
		if (cf_is_error(err)) return err;
		val->type = CF_KV_TYPE_ARRAY;
	} else if (c == '{') {
		int index;
		err = s_parse_object(kv, &index);
		if (cf_is_error(err)) return err;
		val->type = CF_KV_TYPE_OBJECT;
		val->u.object_index = index;
	} else {
		kv->last_err = cf_result_error("Unexpected character when parsing a value.");
		return kv->last_err;
	}

	s_try(kv, ',');

	return cf_result_success();
}

static CF_Result s_parse_object(cf_kv_t* kv, int* index, bool is_top_level)
{
	cf_kv_object_t* object = &kv->objects.add();
	CUTE_PLACEMENT_NEW(object) cf_kv_object_t;
	*index = kv->objects.count() - 1;
	int parent_index = *index;

	if (!is_top_level) {
		s_expect(kv, '{');
	}

	while (1) {
		if (is_top_level) {
			if (kv->in >= kv->in_end || kv->in[0] == 0) {
				CUTE_ASSERT(kv->in == kv->in_end || kv->in[0] == 0);
				break;
			}
		} else {
			if (s_try(kv, '}')) {
				break;
			}
		}

		cf_kv_field_t* field = &object->fields.add();
		CUTE_PLACEMENT_NEW(field) cf_kv_field_t;

		CF_Result err = s_scan_string(kv, &field->key);
		if (cf_is_error(err)) return err;
		s_expect(kv, '=');
		err = s_parse_value(kv, &field->val);
		if (cf_is_error(err)) return err;

		// If objects were parsed, assign proper parent indices.
		if (field->val.type == CF_KV_TYPE_OBJECT) {
			kv->objects[field->val.u.object_index].parent_index = parent_index;
		} else if (field->val.type == CF_KV_TYPE_ARRAY) {
			int count = field->val.aval.count();
			if (count && field->val.aval[0].type == CF_KV_TYPE_OBJECT) {
				array<cf_kv_val_t>& object_val_array = field->val.aval;
				for (int i = 0; i < count; ++i) {
					int object_index = object_val_array[i].u.object_index;
					kv->objects[object_index].parent_index = parent_index;
				}
			}
		}

		s_skip_white(kv);
	}

	s_try(kv, ',');

	return cf_result_success();
}

static void s_set_mode(cf_kv_t* kv, const void* ptr, size_t size, cf_kv_state_t mode)
{
	kv->start = (uint8_t*)ptr;
	kv->in = (uint8_t*)ptr;
	kv->in_end = kv->in + size;
	kv->mode = mode;
	kv->write_buffer.clear();

	kv->backup_base_key_bytes = 0;
	kv->base = NULL;

	kv->objects.clear();
	kv->read_mode_array_stack.clear();
	kv->read_mode_array_index_stack.clear();
	kv->in_array_stack.clear();

	kv->last_err = cf_result_success();
}

void cf_read_reset(cf_kv_t* kv)
{
	CUTE_ASSERT(kv->mode == CF_KV_STATE_READ);
	kv->read_mode_from_array = 0;
	kv->read_mode_array_stack.clear();
	kv->read_mode_array_index_stack.clear();
	kv->object_skip_count = 0;
	kv->matched_val = NULL;
	kv->matched_cache_index = ~0;
	kv->matched_cache_val = NULL;
}

cf_kv_state_t cf_kv_state(cf_kv_t* kv)
{
	return kv->mode;
}

cf_kv_t* cf_kv_read(const void* data, size_t size, result_t* result_out)
{
	cf_kv_t* kv = s_kv();
	s_set_mode(kv, data, size, CF_KV_STATE_READ);

	bool is_top_level = true;
	int index;
	CF_Result err = s_parse_object(kv, &index, is_top_level);
	if (cf_is_error(err)) {
		if (result_out) *result_out = err;
		cf_kv_destroy(kv);
		return NULL;
	}
	CUTE_ASSERT(index == 0);

	uint8_t c;
	while (kv->in != kv->in_end && s_isspace(c = *kv->in)) kv->in++;

	if (kv->in != kv->in_end && kv->in[0] != 0) {
		if (result_out) *result_out = cf_result_error("Unable to parse entire input `data`.");
		cf_kv_destroy(kv);
		return NULL;
	}

	kv->start = NULL;
	kv->in = NULL;
	kv->in_end = NULL;
	if (result_out) *result_out = cf_result_success();

	return kv;
}

cf_kv_t* CUTE_CALL cf_kv_write()
{
	cf_kv_t* kv = s_kv();
	s_set_mode(kv, NULL, 0, CF_KV_STATE_WRITE);
	return kv;
}

const char* cf_kv_buffer(cf_kv_t* kv)
{
	return kv->write_buffer;
}

size_t cf_kv_buffer_size(cf_kv_t* kv)
{
	return kv->write_buffer.len();
}

static void s_build_cache(cf_kv_t* kv)
{
	cf_kv_t* base = kv->base;
	while (base) {
		CUTE_ASSERT(base->mode == CF_KV_STATE_READ);
		cf_kv_cache_t cache;
		cache.kv = base;
		kv->cache.add(cache);
		base = base->base;
	}
}

void cf_kv_set_base(cf_kv_t* kv, cf_kv_t* base)
{
	CUTE_ASSERT(base->mode == CF_KV_STATE_READ);
	kv->base = base;
	s_build_cache(kv);
}

CF_Result cf_kv_last_error(cf_kv_t* kv)
{
	if (!kv) return cf_result_success();
	return kv->last_err;
}

static CUTE_INLINE void s_write_u8(cf_kv_t* kv, uint8_t val)
{
	kv->write_buffer.add(val);
}

static CUTE_INLINE void s_try_consume_one_tab(cf_kv_t* kv)
{
	CUTE_ASSERT(kv->mode == CF_KV_STATE_WRITE);
	char last = kv->write_buffer.last();
	if (last == '\t') {
		kv->write_buffer.pop();
	}
}

static CUTE_INLINE void s_try_consume_whitespace(cf_kv_t* kv)
{
	CUTE_ASSERT(kv->mode == CF_KV_STATE_WRITE);
	while (kv->write_buffer.size() && s_isspace(kv->write_buffer.last())) {
		kv->write_buffer.pop();
	}
}

static CUTE_INLINE void s_tabs_delta(cf_kv_t* kv, int delta)
{
	kv->tabs += delta;
}

static CUTE_INLINE void s_tabs(cf_kv_t* kv)
{
	int tabs = kv->tabs;
	for (int i = 0; i < tabs; ++i) {
		s_write_u8(kv, '\t');
	}
}

static CUTE_INLINE void s_write_str_no_quotes(cf_kv_t* kv, const char* str, size_t len)
{
	kv->write_buffer.fit((int)(kv->write_buffer.count() + len));
	kv->write_buffer.append(str, str + len);
}

static CUTE_INLINE void s_write_str(cf_kv_t* kv, const char* str, size_t len)
{
	s_write_u8(kv, '"');
	s_write_str_no_quotes(kv, str, len);
	s_write_u8(kv, '"');
}

static CUTE_INLINE void s_write_str(cf_kv_t* kv, const char* str)
{
	s_write_str(kv, str, (int)CUTE_STRLEN(str));
}

static CUTE_INLINE cf_kv_field_t* s_find_field(cf_kv_object_t* object, const char* key)
{
	size_t len = CUTE_STRLEN(key);
	int count = object->fields.count();
	for (int i = 0; i < count; ++i) {
		cf_kv_field_t* field = object->fields + i;
		cf_kv_string_t string = field->key;
		if (len != string.len) continue;
		if (CUTE_STRNCMP(key, (const char*)string.str, string.len)) continue;
		return field;
	}
	return NULL;
}

static void s_match_key(cf_kv_t* kv, const char* key)
{
	kv->matched_val = NULL;
	kv->matched_cache_val = NULL;
	kv->matched_cache_index = 0;

	for (int i = 0; i < kv->cache.count(); ++i) {
		cf_kv_cache_t cache = kv->cache[i];
		cf_kv_t* base = cache.kv;
		if (!base->objects.count()) continue;
		cf_kv_object_t* object = base->objects + cache.object_index;
		cf_kv_field_t* field = s_find_field(object, key);
		if (field) {
			CUTE_ASSERT(field->val.type != CF_KV_TYPE_NULL);
			bool is_base = i != 0;
			if (!is_base) {
				kv->matched_val = &field->val;
			} else {
				kv->matched_cache_val = &field->val;
				kv->matched_cache_index = i;
				return;
			}
		}
	}
}

static void s_write_key(cf_kv_t* kv, const char* key, cf_kv_type_t* type)
{
	CUTE_UNUSED(type);
	s_write_str_no_quotes(kv, key, (int)CUTE_STRLEN(key));
	s_write_str_no_quotes(kv, " = ", 3);
}

bool cf_kv_key(cf_kv_t* kv, const char* key, cf_kv_type_t* type)
{
	s_match_key(kv, key);
	if (kv->mode == CF_KV_STATE_WRITE) {
		size_t bytes_written = cf_kv_buffer_size(kv);
		s_write_key(kv, key, type);
		kv->backup_base_key_bytes = cf_kv_buffer_size(kv) - bytes_written;
		return true;
	} else {
		if (kv->read_mode_from_array) {
			kv->last_err = cf_result_error("Can not lookup key while reading from array.");
			return false;
		}

		cf_kv_val_t* match = NULL;
		if (kv->matched_val) {
			match = kv->matched_val;
		} else if (kv->matched_cache_val) {
			match = kv->matched_cache_val;
		} else {
			kv->last_err = cf_result_error("Unable to find field to match `key`.");
			return false;
		}

		CUTE_ASSERT(match->type != CF_KV_TYPE_NULL);
		if (type) *type = match->type;
	}
	return true;
}

static void s_write(cf_kv_t* kv, uint64_t val)
{
	kv->write_buffer.fmt_append("%" PRIu64, val);
}

static void s_write(cf_kv_t* kv, int64_t val)
{
	kv->write_buffer.fmt_append("%" PRIi64, val);
}

static void s_write(cf_kv_t* kv, float val)
{
	kv->write_buffer.fmt_append("%f", val);
}

static void s_write(cf_kv_t* kv, double val)
{
	kv->write_buffer.fmt_append("%f", val);
}

static CUTE_INLINE void s_begin_val(cf_kv_t* kv)
{
	if (kv->in_array) {
		if (kv->in_array == CUTE_KV_IN_ARRAY_AND_FIRST_ELEMENT) {
			kv->in_array = CUTE_KV_IN_ARRAY;
		} else {
			s_write_u8(kv, ' ');
		}
	}
}

static CUTE_INLINE void s_end_val(cf_kv_t* kv)
{
	s_write_u8(kv, ',');
	if (!kv->in_array) {
		s_write_u8(kv, '\n');
		s_tabs(kv);
	}
}

static CUTE_INLINE cf_kv_val_t* s_pop_val(cf_kv_t* kv, cf_kv_type_t type, bool pop_val = true)
{
	if (kv->read_mode_from_array) {
		cf_kv_val_t* array_val = kv->read_mode_array_stack.last();
		int& index = kv->read_mode_array_index_stack.last();
		if (index == array_val->aval.count()) {
			return NULL;
		}
		cf_kv_val_t* val = array_val->aval + index;
		if (pop_val) ++index;
		return val;
	} else {
		cf_kv_val_t* match = NULL;
		if (kv->matched_val) {
			match = kv->matched_val;
		} else {
			return NULL;
		}

		cf_kv_val_t* val = match;
		if (!val) return NULL;
		if (val->type != type) return NULL;
		if (pop_val) {
			kv->matched_val = NULL;
		}
		return val;
	}
}

static cf_kv_val_t* s_pop_base_val(cf_kv_t* kv, cf_kv_type_t type, bool pop_val = true)
{
	cf_kv_val_t* match = NULL;
	if (kv->matched_cache_val) {
		match = kv->matched_cache_val;
	} else {
		return NULL;
	}

	cf_kv_val_t* val = match;
	if (!val) return NULL;
	if (val->type != type) return NULL;
	if (pop_val) {
		kv->matched_cache_val = NULL;
		kv->matched_cache_index = ~0;
	}
	return val;
}

static void s_backup_base_key(cf_kv_t* kv)
{
	if (kv->backup_base_key_bytes) {
		while (kv->backup_base_key_bytes--) {
			kv->write_buffer.pop();
		}
	}
}

template <typename T>
static inline bool s_does_matched_base_equal_int64(cf_kv_t* kv, T* val)
{
	cf_kv_val_t* match_base = s_pop_base_val(kv, CF_KV_TYPE_INT64);
	if (match_base) {
		if ((T)match_base->u.ival == *val) {
			s_backup_base_key(kv);
			return true;
		}
	} else {
		match_base = s_pop_base_val(kv, CF_KV_TYPE_DOUBLE);
		if (match_base && match_base->u.dval == (double)*val) {
			s_backup_base_key(kv);
			return true;
		}
	}
	return false;
}

template <typename T>
bool s_find_match_int64(cf_kv_t* kv, T* val)
{
	cf_kv_val_t* match = s_pop_val(kv, CF_KV_TYPE_INT64);
	cf_kv_val_t* match_base = s_pop_base_val(kv, CF_KV_TYPE_INT64);
	if (!match) match = match_base;
	if (!match) {
		match = s_pop_val(kv, CF_KV_TYPE_DOUBLE);
		match_base = s_pop_base_val(kv, CF_KV_TYPE_DOUBLE);
		if (!match) match = match_base;
		if (!match) {
			kv->last_err = cf_result_error("Unable to get `val` (out of bounds array index, or no matching `kv_key` call.");
			return false;
		}
		*val = (T)match->u.dval;
	} else {
		*val = (T)match->u.ival;
	}
	return true;
}

bool cf_kv_val_uint8(cf_kv_t* kv, uint8_t* val)
{
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (s_does_matched_base_equal_int64(kv, val)) {
			return true;
		}
		s_begin_val(kv);
		s_write_u8(kv, *val);
		s_end_val(kv);
	} else {
		return s_find_match_int64(kv, val);
	}
	return true;
}

bool cf_kv_val_uint16(cf_kv_t* kv, uint16_t* val)
{
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (s_does_matched_base_equal_int64(kv, val)) {
			return true;
		}
		s_begin_val(kv);
		s_write(kv, (uint64_t)*(uint16_t*)val);
		s_end_val(kv);
	} else {
		return s_find_match_int64(kv, val);
	}
	return true;
}

bool cf_kv_val_uint32(cf_kv_t* kv, uint32_t* val)
{
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (s_does_matched_base_equal_int64(kv, val)) {
			return true;
		}
		s_begin_val(kv);
		s_write(kv, (uint64_t)*(uint32_t*)val);
		s_end_val(kv);
	} else {
		return s_find_match_int64(kv, val);
	}
	return true;
}

bool cf_kv_val_uint64(cf_kv_t* kv, uint64_t* val)
{
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (s_does_matched_base_equal_int64(kv, val)) {
			return true;
		}
		s_begin_val(kv);
		s_write(kv, *(uint64_t*)val);
		s_end_val(kv);
	} else {
		return s_find_match_int64(kv, val);
	}
	return true;
}

bool cf_kv_val_int8(cf_kv_t* kv, int8_t* val)
{
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (s_does_matched_base_equal_int64(kv, val)) {
			return true;
		}
		s_begin_val(kv);
		s_write(kv, (int64_t)*(int8_t*)val);
		s_end_val(kv);
	} else {
		return s_find_match_int64(kv, val);
	}
	return true;
}

bool cf_kv_val_int16(cf_kv_t* kv, int16_t* val)
{
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (s_does_matched_base_equal_int64(kv, val)) {
			return true;
		}
		s_begin_val(kv);
		s_write(kv, (int64_t)*(int16_t*)val);
		s_end_val(kv);
	} else {
		return s_find_match_int64(kv, val);
	}
	return true;
}

bool cf_kv_val_int32(cf_kv_t* kv, int32_t* val)
{
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (s_does_matched_base_equal_int64(kv, val)) {
			return true;
		}
		s_begin_val(kv);
		s_write(kv, (int64_t)*(int32_t*)val);
		s_end_val(kv);
	} else {
		return s_find_match_int64(kv, val);
	}
	return true;
}

bool cf_kv_val_int64(cf_kv_t* kv, int64_t* val)
{
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (s_does_matched_base_equal_int64(kv, val)) {
			return true;
		}
		s_begin_val(kv);
		s_write(kv, *(int64_t*)val);
		s_end_val(kv);
	} else {
		return s_find_match_int64(kv, val);
	}
	return true;
}

bool cf_kv_val_float(cf_kv_t* kv, float* val)
{
	cf_kv_val_t* match = s_pop_val(kv, CF_KV_TYPE_DOUBLE);
	cf_kv_val_t* match_base = s_pop_base_val(kv, CF_KV_TYPE_DOUBLE);
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (match_base) {
			if ((float)match_base->u.dval == *val) {
				s_backup_base_key(kv);
				return true;
			}
		}
		s_begin_val(kv);
		s_write(kv, *val);
		s_end_val(kv);
	} else {
		if (!match) match = match_base;
		if (!match) {
			match = s_pop_val(kv, CF_KV_TYPE_INT64);
			match_base = s_pop_base_val(kv, CF_KV_TYPE_INT64);
			if (!match) match = match_base;
			if (!match) {
				kv->last_err = cf_result_error("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
				return false;
			}
			else *val = (float)match->u.ival;
		} else {
			*val = (float)match->u.dval;
		}
	}
	return true;
}

bool cf_kv_val_double(cf_kv_t* kv, double* val)
{
	cf_kv_val_t* match = s_pop_val(kv, CF_KV_TYPE_DOUBLE);
	cf_kv_val_t* match_base = s_pop_base_val(kv, CF_KV_TYPE_DOUBLE);
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (match_base) {
			if (match_base->u.dval == *val) {
				s_backup_base_key(kv);
				return true;
			}
		}
		s_begin_val(kv);
		s_write(kv, *val);
		s_end_val(kv);
	} else {
		if (!match) match = match_base;
		if (!match) {
			match = s_pop_val(kv, CF_KV_TYPE_INT64);
			match_base = s_pop_base_val(kv, CF_KV_TYPE_INT64);
			if (!match) match = match_base;
			if (!match) {
				kv->last_err = cf_result_error("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
				return false;
			}
			else *val = (double)match->u.ival;
		} else {
			*val = match->u.dval;
		}
	}
	return true;
}

bool cf_kv_val_bool(cf_kv_t* kv, bool* val)
{
	if (kv->mode == CF_KV_STATE_READ) {
		const char* string;
		size_t sz;
		if (cf_kv_val_string(kv, &string, &sz)) {
			if (sz == 4 && !CUTE_STRNCMP("true", string, sz)) *val = true;
			else *val = false;
		}
		return false;
	} else {
		if (*val) {
			const char* string = "true";
			size_t sz = 4;
			return cf_kv_val_string(kv, &string, &sz);
		} else {
			const char* string = "false";
			size_t sz = 5;
			return cf_kv_val_string(kv, &string, &sz);
		}
	}
}

bool cf_kv_val_string(cf_kv_t* kv, const char** str, size_t* size)
{
	if (kv->mode == CF_KV_STATE_READ) {
		*str = NULL;
		*size = 0;
	}
	cf_kv_val_t* match = s_pop_val(kv, CF_KV_TYPE_STRING);
	cf_kv_val_t* match_base = s_pop_base_val(kv, CF_KV_TYPE_STRING);
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (match_base) {
			if (!CUTE_STRNCMP((const char*)match_base->u.sval.str, *str, match_base->u.sval.len)) {
				s_backup_base_key(kv);
				return true;
			}
		}
		s_begin_val(kv);
		s_write_str(kv, *str, *size);
		s_end_val(kv);
	} else {
		if (!match) match = match_base;
		if (!match) {
			kv->last_err = cf_result_error("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
			return false;
		}
		*str = (const char*)match->u.sval.str;
		*size = (int)match->u.sval.len;
	}
	return true;
}

bool cf_kv_val_blob(cf_kv_t* kv, void* data, size_t data_capacity, size_t* data_len)
{
	if (kv->mode == CF_KV_STATE_READ) *data_len = 0;
	cf_kv_val_t* match = s_pop_val(kv, CF_KV_TYPE_STRING);
	cf_kv_val_t* match_base = s_pop_base_val(kv, CF_KV_TYPE_STRING);
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (match_base) {
			if (!CUTE_MEMCMP(match_base->u.sval.str, data, match_base->u.sval.len)) {
				s_backup_base_key(kv);
				return true;
			}
		}
		size_t buffer_size = CUTE_BASE64_ENCODED_SIZE(*data_len);
		s_write_u8(kv, '"');
		int old_len = kv->write_buffer.len();
		kv->write_buffer.set_len((int)(old_len + buffer_size));
		uint8_t* buffer = (uint8_t*)kv->write_buffer.c_str() + old_len;
		cf_base64_encode(buffer, buffer_size, data, *data_len);
		s_write_u8(kv, '"');
		s_end_val(kv);
	} else {
		if (!match) match = match_base;
		if (!match) {
			kv->last_err = cf_result_error("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
			return false;
		}
		size_t buffer_size = CUTE_BASE64_DECODED_SIZE(match->u.sval.len);
		if (!(buffer_size <= data_capacity)) {
			kv->last_err = cf_result_error("Decoded base 64 string is too large to store in `data`.");
			return false;
		}
		CF_Result err = cf_base64_decode(data, buffer_size, match->u.sval.str, match->u.sval.len);
		if (cf_is_error(err)) {
			kv->last_err = err;
			return false;
		}
		*data_len = buffer_size;
	}
	return true;
}

bool cf_kv_object_begin(cf_kv_t* kv, const char* key)
{
	if (key) {
		if (!cf_kv_key(kv, key, NULL)) {
			return false;
		}
	}
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (!key && kv->in_array == CUTE_KV_NOT_IN_ARRAY) {
			kv->last_err = cf_result_error("`key` must be supplied if not in an array.");
			return false;
		}
		s_write_str_no_quotes(kv, "{\n", 2);
		s_tabs_delta(kv, 1);
		s_tabs(kv);
		s_push_array(kv, CUTE_KV_NOT_IN_ARRAY);
	} else {
		cf_kv_val_t* match = s_pop_val(kv, CF_KV_TYPE_OBJECT);
		int match_base_index = kv->matched_cache_index;
		cf_kv_val_t* match_base = s_pop_base_val(kv, CF_KV_TYPE_OBJECT);
		if (match_base) {
			kv->cache[match_base_index].object_index = match_base->u.object_index;
		}
		if (match) {
			kv->cache[0].object_index = match->u.object_index;
			s_push_read_mode_array(kv, NULL);
		} else if (match_base) {
			kv->object_skip_count++;
		} else {
			kv->last_err = cf_result_error("Unable to get object, no matching `kv_key` call.");
			return false;
		}
	}
	return true;
}

bool cf_kv_object_end(cf_kv_t* kv)
{
	if (kv->mode == CF_KV_STATE_WRITE) {
		s_tabs_delta(kv, -1);
		s_try_consume_one_tab(kv);
		s_write_str_no_quotes(kv, "},\n", 3);
		s_tabs(kv);
		s_pop_array(kv);
	} else {
		for (int i = 1; i < kv->cache.count(); ++i) {
			cf_kv_cache_t cache = kv->cache[i];
			cache.object_index = cache.kv->objects[cache.object_index].parent_index;
			if (cache.object_index == ~0) {
				kv->last_err = cf_result_error("Tried to end kv object, but none was currently set.");
				return false;
			}
			kv->cache[i] = cache;
		}
		if (kv->object_skip_count) {
			--kv->object_skip_count;
		} else {
			s_pop_read_mode_array(kv);
			kv->cache[0].object_index = kv->objects[kv->cache[0].object_index].parent_index;
		}
	}
	return true;
}

bool cf_kv_array_begin(cf_kv_t* kv, int* count, const char* key)
{
	if (key) {
		if (!cf_kv_key(kv, key, NULL)) {
			return false;
		}
	}
	if (kv->mode == CF_KV_STATE_READ) *count = 0;
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (!key && kv->in_array == CUTE_KV_NOT_IN_ARRAY) {
			kv->last_err = cf_result_error("`key` must be supplied if not in an array.");
			return false;
		}
		s_tabs_delta(kv, 1);
		s_write_u8(kv, '[');
		s_write(kv, (int64_t)*count);
		s_write_str_no_quotes(kv, "] {\n", 4);
		s_tabs(kv);
		s_push_array(kv, CUTE_KV_IN_ARRAY_AND_FIRST_ELEMENT);
	} else {
		cf_kv_val_t* match = s_pop_val(kv, CF_KV_TYPE_ARRAY);
		cf_kv_val_t* match_base = s_pop_base_val(kv, CF_KV_TYPE_ARRAY);
		if (!match) match = match_base;
		if (!match) {
			kv->last_err = cf_result_error("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
			return false;
		}
		s_push_read_mode_array(kv, match);
		*count = match->aval.count();
	}
	return true;
}

bool cf_kv_array_end(cf_kv_t* kv)
{
	if (kv->mode == CF_KV_STATE_WRITE) {
		s_tabs_delta(kv, -1);
		s_try_consume_whitespace(kv);
		if (kv->in_array) {
			s_write_u8(kv, '\n');
		}
		s_tabs(kv);
		s_write_str_no_quotes(kv, "},\n", 3);
		s_tabs(kv);
		s_pop_array(kv);
	} else {
		s_pop_read_mode_array(kv);
	}
	return true;
}
