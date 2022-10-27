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
#include <cute_error.h>

#include <stdio.h>
#include <inttypes.h>

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
	cf_array<cf_kv_val_t> aval;
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
	cf_array<cf_kv_field_t> fields;
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
	cf_kv_state_t mode = CF_KV_STATE_UNITIALIZED;
	uint8_t* in = NULL;
	uint8_t* in_end = NULL;
	uint8_t* start = NULL;
	cf_array<uint8_t> write_buffer;

	// Reading state.
	int object_skip_count = 0;
	cf_kv_val_t* matched_val = NULL;
	int matched_cache_index = ~0;
	cf_kv_val_t* matched_cache_val = NULL;
	cf_array<cf_kv_cache_t> cache;
	cf_array<cf_kv_object_t> objects;

	int read_mode_from_array = 0;
	cf_array<cf_kv_val_t*> read_mode_array_stack;
	cf_array<int> read_mode_array_index_stack;

	// Writing state.
	size_t backup_base_key_bytes = 0;
	cf_kv_t* base = NULL;
	int in_array = CUTE_KV_NOT_IN_ARRAY;
	cf_array<int> in_array_stack;
	int tabs = 0;
	size_t temp_size = 0;
	uint8_t* temp = NULL;

	cf_error_t err = cf_error_success();

	void* mem_ctx = NULL;
};

cf_kv_t* cf_kv_make(void* user_allocator_context)
{
	cf_kv_t* kv = (cf_kv_t*)CUTE_ALLOC(sizeof(cf_kv_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(kv) cf_kv_t;
	kv->mem_ctx = user_allocator_context;

	cf_kv_cache_t cache;
	cache.kv = kv;
	kv->cache.add(cache);

	return kv;
}

void cf_kv_destroy(cf_kv_t* kv)
{
	kv->~cf_kv_t();
	CUTE_FREE(kv->temp, kv->mem_ctx);
	CUTE_FREE(kv, kv->mem_ctx);
}

static CUTE_INLINE void cf_s_push_array(cf_kv_t* kv, int in_array)
{
	kv->in_array_stack.add(kv->in_array);
	kv->in_array = in_array;
}

static CUTE_INLINE void cf_s_pop_array(cf_kv_t* kv)
{
	kv->in_array = kv->in_array_stack.pop();
}

static CUTE_INLINE void cf_s_push_read_mode_array(cf_kv_t* kv, cf_kv_val_t* val)
{
	kv->read_mode_array_stack.add(val);
	kv->read_mode_array_index_stack.add(0);
	kv->read_mode_from_array = val ? 1 : 0;
}

static CUTE_INLINE void cf_s_pop_read_mode_array(cf_kv_t* kv)
{
	kv->read_mode_array_stack.pop();
	kv->read_mode_array_index_stack.pop();
	if (kv->read_mode_array_stack.count()) {
		kv->read_mode_from_array = kv->read_mode_array_stack.last() ? 1 : 0;
	} else {
		kv->read_mode_from_array = 0;
	}
}

static CUTE_INLINE int cf_s_isspace(uint8_t c)
{
	return (c == ' ') |
		(c == '\t') |
		(c == '\n') |
		(c == '\v') |
		(c == '\f') |
		(c == '\r');
}

static CUTE_INLINE void cf_s_skip_white(cf_kv_t* kv)
{
	while (kv->in < kv->in_end && cf_s_isspace(*kv->in)) kv->in++;
}

static CUTE_INLINE uint8_t cf_s_peek(cf_kv_t* kv)
{
	while (kv->in < kv->in_end && cf_s_isspace(*kv->in)) kv->in++;
	return kv->in < kv->in_end ? *kv->in : 0;
}

static CUTE_INLINE uint8_t cf_s_next(cf_kv_t* kv)
{
	uint8_t c;
	if (kv->in == kv->in_end) return 0;
	while (cf_s_isspace(c = *kv->in++)) if (kv->in == kv->in_end) return 0;
	return c;
}

static CUTE_INLINE int cf_s_try(cf_kv_t* kv, uint8_t expect)
{
	if (kv->in == kv->in_end) return 0;
	if (cf_s_peek(kv) == expect)
	{
		kv->in++;
		return 1;
	}
	return 0;
}

#define cf_s_expect(kv, expected_character) \
	do { \
		if (cf_s_next(kv) != expected_character) { kv->err = cf_error_failure("Found unexpected token."); return kv->err; } \
	} while (0)

static cf_error_t cf_s_scan_string(cf_kv_t* kv, uint8_t** start_of_string, uint8_t** end_of_string)
{
	*start_of_string = NULL;
	*end_of_string = NULL;
	int has_quotes = cf_s_try(kv, '"');
	*start_of_string = kv->in;
	int terminated = 0;
	if (has_quotes) {
		while (kv->in < kv->in_end)
		{
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
		while (end < kv->in_end && !cf_s_isspace(*end)) end++;
		*end_of_string = end;
		kv->in = end + 1;
	}
	if (!terminated && kv->in == kv->in_end) {
		kv->err = cf_error_failure("Unterminated string at end of file.");
		return kv->err;
	}
	return cf_error_success();
}

static cf_error_t cf_s_scan_string(cf_kv_t* kv, cf_kv_string_t* str)
{
	uint8_t* string_start;
	uint8_t* string_end;
	cf_error_t err = cf_s_scan_string(kv, &string_start, &string_end);
	str->str = string_start;
	str->len = (int)(string_end - string_start);
	return err;
}

static CUTE_INLINE cf_error_t cf_s_parse_int(cf_kv_t* kv, int64_t* out)
{
	uint8_t* end;
	int64_t val = CUTE_STRTOLL((char*)kv->in, (char**)&end, 10);
	if (kv->in == end) {
		kv->err = cf_error_failure("Invalid integer found during parse.");
		return kv->err;
	}
	kv->in = end;
	*out = val;
	return cf_error_success();
}

static CUTE_INLINE cf_error_t cf_s_parse_float(cf_kv_t* kv, double* out)
{
	uint8_t* end;
	double val = CUTE_STRTOD((char*)kv->in, (char**)&end);
	if (kv->in == end) {
		kv->err = cf_error_failure("Invalid float found during parse.");
		return kv->err;
	}
	kv->in = end;
	*out = val;
	return cf_error_success();
}

static cf_error_t cf_s_parse_hex(cf_kv_t* kv, uint64_t* hex)
{
	cf_s_expect(kv, '0');
	uint8_t c = cf_s_next(kv);
	if (!(c != 'x' && c != 'X')) {
		kv->err = cf_error_failure("Expected 'x' or 'X' when parsing a hex number.");
		return kv->err;
	}
	uint8_t* end;
	uint64_t val = (uint64_t)CUTE_STRTOLL((char*)kv->in, (char**)&end, 16);
	if (kv->in == end) {
		kv->err = cf_error_failure("Invalid hex integer found during parse.");
		return kv->err;
	}
	kv->in = end;
	*hex = val;
	return cf_error_success();
}

static CUTE_INLINE cf_error_t cf_s_parse_number(cf_kv_t* kv, cf_kv_val_t* val)
{
	cf_error_t err;
	if (kv->in + 1 < kv->in_end && ((kv->in[1] == 'x') | (kv->in[1] == 'X'))) {
		uint64_t hex;
		err = cf_s_parse_hex(kv, &hex);
		if (cf_is_error(err)) return err;
		val->type = CF_KV_TYPE_INT64;
		val->u.ival = (int64_t)hex;
	} else {
		uint8_t c;
		uint8_t* s = kv->in;
		int is_float = 0;
		while (s < kv->in_end && (c = *s++) != ',')
		{
			if (c == '.')
			{
				is_float = 1;
				break;
			}
		}

		if (is_float) {
			double dval;
			err = cf_s_parse_float(kv, &dval);
			if (cf_is_error(err)) return err;
			val->type = CF_KV_TYPE_DOUBLE;
			val->u.dval = dval;
		} else {
			int64_t ival;
			err = cf_s_parse_int(kv, &ival);
			if (cf_is_error(err)) return err;
			val->type = CF_KV_TYPE_INT64;
			val->u.ival = ival;
		}
	}
	return cf_error_success();
}

static cf_error_t cf_s_parse_value(cf_kv_t* kv, cf_kv_val_t* val);

static cf_error_t cf_s_parse_array(cf_kv_t* kv, cf_array<cf_kv_val_t>* array_val)
{
	cf_error_t err;
	int64_t count;
	cf_s_expect(kv, '[');
	err = cf_s_parse_int(kv, &count);
	if (cf_is_error(err)) return err;
	cf_s_expect(kv, ']');
	cf_s_expect(kv, '{');
	array_val->ensure_capacity((int)count);
	for (int i = 0; i < (int)count; ++i)
	{
		cf_kv_val_t* val = &array_val->add();
		CUTE_PLACEMENT_NEW(val) cf_kv_val_t;
		err = cf_s_parse_value(kv, val);
		if (cf_is_error(err)) {
			return cf_error_failure("Unexecpted value when parsing an array. Make sure the elements are well-formed, and the length is correct.");
		}
	}
	cf_s_expect(kv, '}');
	return cf_error_success();
}

static cf_error_t cf_s_parse_object(cf_kv_t* kv, int* index, bool is_top_level = false);

static cf_error_t cf_s_parse_value(cf_kv_t* kv, cf_kv_val_t* val)
{
	cf_error_t err;
	uint8_t c = cf_s_peek(kv);

	if (c == '"') {
		cf_kv_string_t string;
		err = cf_s_scan_string(kv, &string);
		if (cf_is_error(err)) return err;
		val->type = CF_KV_TYPE_STRING;
		val->u.sval = string;
	} else if ((c >= '0' && c <= '9') | (c == '-')) {
		err = cf_s_parse_number(kv, val);
		if (cf_is_error(err)) return err;
	} else if (c == '[') {
		err = cf_s_parse_array(kv, &val->aval);
		if (cf_is_error(err)) return err;
		val->type = CF_KV_TYPE_ARRAY;
	} else if (c == '{') {
		int index;
		err = cf_s_parse_object(kv, &index);
		if (cf_is_error(err)) return err;
		val->type = CF_KV_TYPE_OBJECT;
		val->u.object_index = index;
	} else {
		kv->err = cf_error_failure("Unexpected character when parsing a value.");
		return kv->err;
	}

	cf_s_try(kv, ',');

	return cf_error_success();
}

static cf_error_t cf_s_parse_object(cf_kv_t* kv, int* index, bool is_top_level)
{
	cf_kv_object_t* object = &kv->objects.add();
	CUTE_PLACEMENT_NEW(object) cf_kv_object_t;
	*index = kv->objects.count() - 1;
	int parent_index = *index;

	if (!is_top_level) {
		cf_s_expect(kv, '{');
	}

	while (1)
	{
		if (is_top_level) {
			if (kv->in >= kv->in_end || kv->in[0] == 0) {
				CUTE_ASSERT(kv->in == kv->in_end || kv->in[0] == 0);
				break;
			}
		} else {
			if (cf_s_try(kv, '}')) {
				break;
			}
		}

		cf_kv_field_t* field = &object->fields.add();
		CUTE_PLACEMENT_NEW(field) cf_kv_field_t;

		cf_error_t err = cf_s_scan_string(kv, &field->key);
		if (cf_is_error(err)) return err;
		cf_s_expect(kv, '=');
		err = cf_s_parse_value(kv, &field->val);
		if (cf_is_error(err)) return err;

		// If objects were parsed, assign proper parent indices.
		if (field->val.type == CF_KV_TYPE_OBJECT) {
			kv->objects[field->val.u.object_index].parent_index = parent_index;
		} else if (field->val.type == CF_KV_TYPE_ARRAY) {
			int count = field->val.aval.count();
			if (count && field->val.aval[0].type == CF_KV_TYPE_OBJECT) {
				cf_array<cf_kv_val_t>& object_val_array = field->val.aval;
				for (int i = 0; i < count; ++i)
				{
					int object_index = object_val_array[i].u.object_index;
					kv->objects[object_index].parent_index = parent_index;
				}
			}
		}

		cf_s_skip_white(kv);
	}

	cf_s_try(kv, ',');

	return cf_error_success();
}

static void cf_s_reset(cf_kv_t* kv, const void* ptr, size_t size, cf_kv_state_t mode)
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

	kv->err = cf_error_success();
}

cf_kv_state_t cf_kv_get_state(cf_kv_t* kv)
{
	return kv->mode;
}

cf_error_t cf_kv_parse(cf_kv_t* kv, const void* data, size_t size)
{
	cf_s_reset(kv, data, size, CF_KV_STATE_READ);

	bool is_top_level = true;
	int index;
	cf_error_t err = cf_s_parse_object(kv, &index, is_top_level);
	if (cf_is_error(err)) return err;
	CUTE_ASSERT(index == 0);

	uint8_t c;
	while (kv->in != kv->in_end && cf_s_isspace(c = *kv->in)) kv->in++;

	if (kv->in != kv->in_end && kv->in[0] != 0) {
		kv->err = cf_error_failure("Unable to parse entire input `data`.");
		return kv->err;
	}

	kv->start = NULL;
	kv->in = NULL;
	kv->in_end = NULL;

	return cf_error_success();
}

void CUTE_CALL cf_kv_write_mode(cf_kv_t* kv)
{
	cf_s_reset(kv, NULL, 0, CF_KV_STATE_WRITE);
}

void* cf_kv_get_buffer(cf_kv_t* kv)
{
	return (void*)kv->write_buffer.data();
}

void cf_kv_nul_terminate(cf_kv_t* kv)
{
	kv->write_buffer.add(0);
}

static void cf_s_build_cache(cf_kv_t* kv)
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
	cf_s_build_cache(kv);
}

void cf_kv_reset_read_state(cf_kv_t* kv)
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

size_t cf_kv_size_written(cf_kv_t* kv)
{
	return kv->write_buffer.size();
}

cf_error_t cf_kv_error_state(cf_kv_t* kv)
{
	if (!kv) return cf_error_success();
	return kv->err;
}

static CUTE_INLINE void cf_s_write_u8(cf_kv_t* kv, uint8_t val)
{
	kv->write_buffer.add(val);
}

static CUTE_INLINE void cf_s_try_consume_one_tab(cf_kv_t* kv)
{
	CUTE_ASSERT(kv->mode == CF_KV_STATE_WRITE);
	if (kv->write_buffer.size() && kv->write_buffer.last() == '\t') kv->write_buffer.pop();
}

static CUTE_INLINE void cf_s_try_consume_whitespace(cf_kv_t* kv)
{
	CUTE_ASSERT(kv->mode == CF_KV_STATE_WRITE);
	while (kv->write_buffer.size() && cf_s_isspace(kv->write_buffer.last())) {
		kv->write_buffer.pop();
	}
}

static CUTE_INLINE void cf_s_tabs_delta(cf_kv_t* kv, int delta)
{
	kv->tabs += delta;
}

static CUTE_INLINE void cf_s_tabs(cf_kv_t* kv)
{
	int tabs = kv->tabs;
	for (int i = 0; i < tabs; ++i) {
		cf_s_write_u8(kv, '\t');
	}
}

static CUTE_INLINE void cf_s_write_str_no_quotes(cf_kv_t* kv, const char* str, size_t len)
{
	int old_count = kv->write_buffer.count();
	kv->write_buffer.ensure_count((int)(old_count + len));
	CUTE_STRNCPY((char*)kv->write_buffer.data() + old_count, str, len);
}

static CUTE_INLINE void cf_s_write_str(cf_kv_t* kv, const char* str, size_t len)
{
	cf_s_write_u8(kv, '"');
	cf_s_write_str_no_quotes(kv, str, len);
	cf_s_write_u8(kv, '"');
}

static CUTE_INLINE void cf_s_write_str(cf_kv_t* kv, const char* str)
{
	cf_s_write_str(kv, str, (int)CUTE_STRLEN(str));
}

static CUTE_INLINE cf_kv_field_t* cf_s_find_field(cf_kv_object_t* object, const char* key)
{
	size_t len = CUTE_STRLEN(key);
	int count = object->fields.count();
	for (int i = 0; i < count; ++i)
	{
		cf_kv_field_t* field = object->fields + i;
		cf_kv_string_t string = field->key;
		if (len != string.len) continue;
		if (CUTE_STRNCMP(key, (const char*)string.str, string.len)) continue;
		return field;
	}
	return NULL;
}

static void cf_s_match_key(cf_kv_t* kv, const char* key)
{
	kv->matched_val = NULL;
	kv->matched_cache_val = NULL;
	kv->matched_cache_index = 0;

	for (int i = 0; i < kv->cache.count(); ++i) {
		cf_kv_cache_t cache = kv->cache[i];
		cf_kv_t* base = cache.kv;
		if (!base->objects.count()) continue;
		cf_kv_object_t* object = base->objects + cache.object_index;
		cf_kv_field_t* field = cf_s_find_field(object, key);
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

static void cf_s_write_key(cf_kv_t* kv, const char* key, cf_kv_type_t* type)
{
	CUTE_UNUSED(type);
	cf_s_write_str_no_quotes(kv, key, (int)CUTE_STRLEN(key));
	cf_s_write_str_no_quotes(kv, " = ", 3);
}

cf_error_t cf_kv_key(cf_kv_t* kv, const char* key, cf_kv_type_t* type)
{
	if (kv->mode == CF_KV_STATE_UNITIALIZED) return cf_error_failure("Read or write mode have not been set.");
	if (cf_is_error(kv->err)) return kv->err;
	cf_s_match_key(kv, key);
	if (kv->mode == CF_KV_STATE_WRITE) {
		size_t bytes_written = cf_kv_size_written(kv);
		cf_s_write_key(kv, key, type);
		kv->backup_base_key_bytes = cf_kv_size_written(kv) - bytes_written;
		return cf_error_success();
	} else {
		if (kv->read_mode_from_array) {
			return cf_error_failure("Can not lookup key while reading from array.");
		}

		cf_kv_val_t* match = NULL;
		if (kv->matched_val) {
			match = kv->matched_val;
		} else if (kv->matched_cache_val) {
			match = kv->matched_cache_val;
		} else {
			return cf_error_failure("Unable to find field to match `key`.");
		}

		CUTE_ASSERT(match->type != CF_KV_TYPE_NULL);
		if (type) *type = match->type;
	}
	return cf_error_success();
}

static uint8_t* cf_s_temp(cf_kv_t* kv, size_t size)
{
	if (kv->temp_size < size + 1) {
		CUTE_FREE(kv->temp, kv->mem_ctx);
		kv->temp_size = size + 1;
		kv->temp = (uint8_t*)CUTE_ALLOC(size + 1, kv->mem_ctx);
		if (size) CUTE_ASSERT(kv->temp);
	}
	return kv->temp;
}

static int cf_s_to_string(cf_kv_t* kv, uint64_t val)
{
	const char* fmt = "%" PRIu64;
	uint8_t* temp = cf_s_temp(kv, 256);

	#ifdef _WIN32
		int size = _scprintf(fmt, val) + 1;
	#else
		int size = snprintf((char*)temp, 0, fmt, val) + 1;
	#endif

	temp = cf_s_temp(kv, size);
	snprintf((char*)temp, size, fmt, val);
	return size - 1;
}

static int cf_s_to_string(cf_kv_t* kv, int64_t val)
{
	const char* fmt = "%" PRIi64;
	uint8_t* temp = cf_s_temp(kv, 256);

	#ifdef _WIN32
		int size = _scprintf(fmt, val) + 1;
	#else
		int size = snprintf((char*)temp, 0, fmt, val) + 1;
	#endif

	temp = cf_s_temp(kv, size);
	snprintf((char*)temp, size, fmt, val);
	return size - 1;
}

static int cf_s_to_string(cf_kv_t* kv, float val)
{
	const char* fmt = "%f";
	uint8_t* temp = cf_s_temp(kv, 256);

	#ifdef _WIN32
		int size = _scprintf(fmt, val) + 1;
	#else
		int size = snprintf((char*)temp, 0, fmt, val) + 1;
	#endif

	temp = cf_s_temp(kv, size);
	snprintf((char*)temp, size, fmt, val);
	return size - 1;
}

static int cf_s_to_string(cf_kv_t* kv, double val)
{
	const char* fmt = "%f";
	uint8_t* temp = cf_s_temp(kv, 256);

	#ifdef _WIN32
		int size = _scprintf(fmt, val) + 1;
	#else
		int size = snprintf((char*)temp, 0, fmt, val) + 1;
	#endif

	temp = cf_s_temp(kv, size);
	snprintf((char*)temp, size, fmt, val);
	return size - 1;
}

static void cf_s_write(cf_kv_t* kv, uint64_t val)
{
	int size = cf_s_to_string(kv, val);
	cf_s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static void cf_s_write(cf_kv_t* kv, int64_t val)
{
	int size = cf_s_to_string(kv, val);
	cf_s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static void cf_s_write(cf_kv_t* kv, float val)
{
	int size = cf_s_to_string(kv, val);
	cf_s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static void cf_s_write(cf_kv_t* kv, double val)
{
	int size = cf_s_to_string(kv, val);
	cf_s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static CUTE_INLINE void cf_s_begin_val(cf_kv_t* kv)
{
	if (kv->in_array) {
		if (kv->in_array == CUTE_KV_IN_ARRAY_AND_FIRST_ELEMENT) {
			kv->in_array = CUTE_KV_IN_ARRAY;
		} else {
			cf_s_write_u8(kv, ' ');
		}
	}
}

static CUTE_INLINE void cf_s_end_val(cf_kv_t* kv)
{
	cf_s_write_u8(kv, ',');
	if (!kv->in_array) {
		cf_s_write_u8(kv, '\n');
		cf_s_tabs(kv);
	}
}

static CUTE_INLINE cf_kv_val_t* cf_s_pop_val(cf_kv_t* kv, cf_kv_type_t type, bool pop_val = true)
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

static cf_kv_val_t* cf_s_pop_base_val(cf_kv_t* kv, cf_kv_type_t type, bool pop_val = true)
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

static void cf_s_backup_base_key(cf_kv_t* kv)
{
	if (kv->backup_base_key_bytes) {
		while (kv->backup_base_key_bytes--) {
			kv->write_buffer.pop();
		}
	}
}

template <typename T>
static inline bool cf_s_does_matched_base_equal_int64(cf_kv_t* kv, T* val)
{
	cf_kv_val_t* match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_INT64);
	if (match_base) {
		if ((T)match_base->u.ival == *val) {
			cf_s_backup_base_key(kv);
			return true;
		}
	} else {
		match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_DOUBLE);
		if (match_base && match_base->u.dval == (double)*val) {
			cf_s_backup_base_key(kv);
			return true;
		}
	}
	return false;
}

template <typename T>
cf_error_t cf_s_find_match_int64(cf_kv_t* kv, T* val)
{
	cf_kv_val_t* match = cf_s_pop_val(kv, CF_KV_TYPE_INT64);
	cf_kv_val_t* match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_INT64);
	if (!match) match = match_base;
	if (!match) {
		match = cf_s_pop_val(kv, CF_KV_TYPE_DOUBLE);
		match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_DOUBLE);
		if (!match) match = match_base;
		if (!match) return cf_error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		*val = (T)match->u.dval;
	} else {
		*val = (T)match->u.ival;
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_uint8(cf_kv_t* kv, uint8_t* val)
{
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (cf_s_does_matched_base_equal_int64(kv, val)) {
			return cf_error_success();
		}
		cf_s_begin_val(kv);
		cf_s_write_u8(kv, *val);
		cf_s_end_val(kv);
	} else {
		return cf_s_find_match_int64(kv, val);
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_uint16(cf_kv_t* kv, uint16_t* val)
{
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (cf_s_does_matched_base_equal_int64(kv, val)) {
			return cf_error_success();
		}
		cf_s_begin_val(kv);
		cf_s_write(kv, (uint64_t)*(uint16_t*)val);
		cf_s_end_val(kv);
	} else {
		return cf_s_find_match_int64(kv, val);
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_uint32(cf_kv_t* kv, uint32_t* val)
{
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (cf_s_does_matched_base_equal_int64(kv, val)) {
			return cf_error_success();
		}
		cf_s_begin_val(kv);
		cf_s_write(kv, (uint64_t)*(uint32_t*)val);
		cf_s_end_val(kv);
	} else {
		return cf_s_find_match_int64(kv, val);
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_uint64(cf_kv_t* kv, uint64_t* val)
{
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (cf_s_does_matched_base_equal_int64(kv, val)) {
			return cf_error_success();
		}
		cf_s_begin_val(kv);
		cf_s_write(kv, *(uint64_t*)val);
		cf_s_end_val(kv);
	} else {
		return cf_s_find_match_int64(kv, val);
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_int8(cf_kv_t* kv, int8_t* val)
{
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (cf_s_does_matched_base_equal_int64(kv, val)) {
			return cf_error_success();
		}
		cf_s_begin_val(kv);
		cf_s_write(kv, (int64_t)*(int8_t*)val);
		cf_s_end_val(kv);
	} else {
		return cf_s_find_match_int64(kv, val);
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_int16(cf_kv_t* kv, int16_t* val)
{
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (cf_s_does_matched_base_equal_int64(kv, val)) {
			return cf_error_success();
		}
		cf_s_begin_val(kv);
		cf_s_write(kv, (int64_t)*(int16_t*)val);
		cf_s_end_val(kv);
	} else {
		return cf_s_find_match_int64(kv, val);
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_int32(cf_kv_t* kv, int32_t* val)
{
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (cf_s_does_matched_base_equal_int64(kv, val)) {
			return cf_error_success();
		}
		cf_s_begin_val(kv);
		cf_s_write(kv, (int64_t)*(int32_t*)val);
		cf_s_end_val(kv);
	} else {
		return cf_s_find_match_int64(kv, val);
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_int64(cf_kv_t* kv, int64_t* val)
{
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (cf_s_does_matched_base_equal_int64(kv, val)) {
			return cf_error_success();
		}
		cf_s_begin_val(kv);
		cf_s_write(kv, *(int64_t*)val);
		cf_s_end_val(kv);
	} else {
		return cf_s_find_match_int64(kv, val);
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_float(cf_kv_t* kv, float* val)
{
	if (cf_is_error(kv->err)) return kv->err;
	cf_kv_val_t* match = cf_s_pop_val(kv, CF_KV_TYPE_DOUBLE);
	cf_kv_val_t* match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_DOUBLE);
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (match_base) {
			if ((float)match_base->u.dval == *val) {
				cf_s_backup_base_key(kv);
				return cf_error_success();
			}
		}
		cf_s_begin_val(kv);
		cf_s_write(kv, *val);
		cf_s_end_val(kv);
	} else {
		if (!match) match = match_base;
		if (!match) {
			match = cf_s_pop_val(kv, CF_KV_TYPE_INT64);
			match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_INT64);
			if (!match) match = match_base;
			if (!match) return cf_error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
			else *val = (float)match->u.ival;
		} else {
			*val = (float)match->u.dval;
		}
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_double(cf_kv_t* kv, double* val)
{
	if (cf_is_error(kv->err)) return kv->err;
	cf_kv_val_t* match = cf_s_pop_val(kv, CF_KV_TYPE_DOUBLE);
	cf_kv_val_t* match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_DOUBLE);
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (match_base) {
			if (match_base->u.dval == *val) {
				cf_s_backup_base_key(kv);
				return cf_error_success();
			}
		}
		cf_s_begin_val(kv);
		cf_s_write(kv, *val);
		cf_s_end_val(kv);
	} else {
		if (!match) match = match_base;
		if (!match) {
			match = cf_s_pop_val(kv, CF_KV_TYPE_INT64);
			match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_INT64);
			if (!match) match = match_base;
			if (!match) return cf_error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
			else *val = (double)match->u.ival;
		} else {
			*val = match->u.dval;
		}
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_bool(cf_kv_t* kv, bool* val)
{
	if (kv->mode == CF_KV_STATE_READ) {
		const char* string;
		size_t sz;
		cf_error_t err = cf_kv_val_string(kv, &string, &sz);
		if (!cf_is_error(err)) {
			if (sz == 4 && !CUTE_STRNCMP("true", string, sz)) *val = true;
			else *val = false;
		}
		return err;
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

cf_error_t cf_kv_val_string(cf_kv_t* kv, const char** str, size_t* size)
{
	if (kv->mode == CF_KV_STATE_READ) {
		*str = NULL;
		*size = 0;
	}
	if (cf_is_error(kv->err)) return kv->err;
	cf_kv_val_t* match = cf_s_pop_val(kv, CF_KV_TYPE_STRING);
	cf_kv_val_t* match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_STRING);
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (match_base) {
			if (!CUTE_STRNCMP((const char*)match_base->u.sval.str, *str, match_base->u.sval.len)) {
				cf_s_backup_base_key(kv);
				return cf_error_success();
			}
		}
		cf_s_begin_val(kv);
		cf_s_write_str(kv, *str, *size);
		cf_s_end_val(kv);
	} else {
		if (!match) match = match_base;
		if (!match) return cf_error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		*str = (const char*)match->u.sval.str;
		*size = (int)match->u.sval.len;
	}
	return cf_error_success();
}

cf_error_t cf_kv_val_blob(cf_kv_t* kv, void* data, size_t data_capacity, size_t* data_len)
{
	if (kv->mode == CF_KV_STATE_READ) *data_len = 0;
	if (cf_is_error(kv->err)) return kv->err;
	cf_kv_val_t* match = cf_s_pop_val(kv, CF_KV_TYPE_STRING);
	cf_kv_val_t* match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_STRING);
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (match_base) {
			if (!CUTE_MEMCMP(match_base->u.sval.str, data, match_base->u.sval.len)) {
				cf_s_backup_base_key(kv);
				return cf_error_success();
			}
		}
		size_t buffer_size = CUTE_BASE64_ENCODED_SIZE(*data_len);
		uint8_t* buffer = cf_s_temp(kv, buffer_size);
		cf_base64_encode(buffer, buffer_size, data, *data_len);
		cf_s_write_str(kv, (const char*)buffer, buffer_size);
		cf_s_end_val(kv);
	} else {
		if (!match) match = match_base;
		if (!match) return cf_error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		size_t buffer_size = CUTE_BASE64_DECODED_SIZE(match->u.sval.len);
		if (!(buffer_size <= data_capacity)) {
			kv->err = cf_error_failure("Decoded base 64 string is too large to store in `data`.");
			return kv->err;
		}
		cf_error_t err = cf_base64_decode(data, buffer_size, match->u.sval.str, match->u.sval.len);
		if (cf_is_error(err)) return err;
		*data_len = buffer_size;
	}
	return cf_error_success();
}

cf_error_t cf_kv_object_begin(cf_kv_t* kv, const char* key)
{
	if (key) {
		cf_error_t err = cf_kv_key(kv, key, NULL);
		if (cf_is_error(err)) return err;
	}
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (!key && kv->in_array == CUTE_KV_NOT_IN_ARRAY) return cf_error_failure("`key` must be supplied if not in an array.");
		cf_s_write_str_no_quotes(kv, "{\n", 2);
		cf_s_tabs_delta(kv, 1);
		cf_s_tabs(kv);
		cf_s_push_array(kv, CUTE_KV_NOT_IN_ARRAY);
	} else {
		cf_kv_val_t* match = cf_s_pop_val(kv, CF_KV_TYPE_OBJECT);
		int match_base_index = kv->matched_cache_index;
		cf_kv_val_t* match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_OBJECT);
		if (match_base) {
			kv->cache[match_base_index].object_index = match_base->u.object_index;
		}
		if (match) {
			kv->cache[0].object_index = match->u.object_index;
			cf_s_push_read_mode_array(kv, NULL);
		} else if (match_base) {
			kv->object_skip_count++;
		} else {
			kv->err = cf_error_failure("Unable to get object, no matching `kv_key` call.");
			return kv->err;
		}
	}
	return cf_error_success();
}

cf_error_t cf_kv_object_end(cf_kv_t* kv)
{
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		cf_s_tabs_delta(kv, -1);
		cf_s_try_consume_one_tab(kv);
		cf_s_write_str_no_quotes(kv, "},\n", 3);
		cf_s_tabs(kv);
		cf_s_pop_array(kv);
	} else {
		for (int i = 1; i < kv->cache.count(); ++i) {
			cf_kv_cache_t cache = kv->cache[i];
			cache.object_index = cache.kv->objects[cache.object_index].parent_index;
			if (cache.object_index == ~0) {
				kv->err = cf_error_failure("Tried to end kv object, but none was currently set.");
				return kv->err;
			}
			kv->cache[i] = cache;
		}
		if (kv->object_skip_count) {
			--kv->object_skip_count;
		} else {
			cf_s_pop_read_mode_array(kv);
			kv->cache[0].object_index = kv->objects[kv->cache[0].object_index].parent_index;
		}
	}
	return cf_error_success();
}

cf_error_t cf_kv_array_begin(cf_kv_t* kv, int* count, const char* key)
{
	if (key) {
		cf_error_t err = cf_kv_key(kv, key, NULL);
		if (cf_is_error(err)) return err;
	}
	if (kv->mode == CF_KV_STATE_READ) *count = 0;
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		if (!key && kv->in_array == CUTE_KV_NOT_IN_ARRAY) return cf_error_failure("`key` must be supplied if not in an array.");
		cf_s_tabs_delta(kv, 1);
		cf_s_write_u8(kv, '[');
		cf_s_write(kv, (int64_t)*count);
		cf_s_write_str_no_quotes(kv, "] {\n", 4);
		cf_s_tabs(kv);
		cf_s_push_array(kv, CUTE_KV_IN_ARRAY_AND_FIRST_ELEMENT);
	} else {
		cf_kv_val_t* match = cf_s_pop_val(kv, CF_KV_TYPE_ARRAY);
		cf_kv_val_t* match_base = cf_s_pop_base_val(kv, CF_KV_TYPE_ARRAY);
		if (!match) match = match_base;
		if (!match) return cf_error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		cf_s_push_read_mode_array(kv, match);
		*count = match->aval.count();
	}
	return cf_error_success();
}

cf_error_t cf_kv_array_end(cf_kv_t* kv)
{
	if (cf_is_error(kv->err)) return kv->err;
	if (kv->mode == CF_KV_STATE_WRITE) {
		cf_s_tabs_delta(kv, -1);
		cf_s_try_consume_whitespace(kv);
		if (kv->in_array) {
			cf_s_write_u8(kv, '\n');
		}
		cf_s_tabs(kv);
		cf_s_write_str_no_quotes(kv, "},\n", 3);
		cf_s_tabs(kv);
		cf_s_pop_array(kv);
	} else {
		cf_s_pop_read_mode_array(kv);
	}
	return cf_error_success();
}

void cf_kv_print(cf_kv_t* kv)
{
	printf("\n\n%.*s", (int)(kv->in - kv->start), kv->start);
}
