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
#include <cute_buffer.h>
#include <cute_base64.h>
#include <cute_array.h>
#include <cute_error.h>

#include <stdio.h>
#include <inttypes.h>

#define CUTE_KV_MODE_WRITE 1
#define CUTE_KV_MODE_READ  0

namespace cute
{

struct kv_string_t
{
	uint8_t* str = NULL;
	size_t len = 0;
};

union kv_union_t
{
	kv_union_t() {}

	int64_t ival;
	double dval;
	kv_string_t sval;
	kv_string_t bval;
	int object_index;
};

struct kv_val_t
{
	kv_type_t type = KV_TYPE_NULL;
	kv_union_t u;
	array<kv_val_t> aval;
};

struct kv_field_t
{
	kv_string_t key;
	kv_val_t val;
};

struct kv_object_t
{
	int parent_index = ~0;
	int parsing_array = 0;

	kv_string_t key;
	array<kv_field_t> fields;
};

#define CUTE_KV_NOT_IN_ARRAY               0
#define CUTE_KV_IN_ARRAY                   1
#define CUTE_KV_IN_ARRAY_AND_FIRST_ELEMENT 2

struct kv_t
{
	int mode = -1;
	uint8_t* in = NULL;
	uint8_t* in_end = NULL;
	uint8_t* start = NULL;

	kv_val_t* read_mode_matched_val = NULL;
	int read_mode_object_index = 0;
	int read_mode_top_level_index = 0;
	array<int> top_level_object_indices;
	array<kv_object_t> objects;

	int read_mode_from_array = 0;
	array<kv_val_t*> read_mode_array_stack;
	array<int> read_mode_array_index_stack;

	int in_array = CUTE_KV_NOT_IN_ARRAY;
	array<int> in_array_stack;

	int tabs = 0;
	size_t temp_size = 0;
	uint8_t* temp = NULL;

	error_t err = error_success();

	void* mem_ctx = NULL;
};

kv_t* kv_make(void* user_allocator_context)
{
	kv_t* kv = (kv_t*)CUTE_ALLOC(sizeof(kv_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(kv) kv_t;
	kv->mem_ctx = user_allocator_context;
	return kv;
}

void kv_destroy(kv_t* kv)
{
	kv->~kv_t();
	CUTE_FREE(kv->temp, kv->mem_ctx);
	CUTE_FREE(kv, kv->mem_ctx);
}

static CUTE_INLINE void s_push_array(kv_t* kv, int in_array)
{
	kv->in_array_stack.add(kv->in_array);
	kv->in_array = in_array;
}

static CUTE_INLINE void s_pop_array(kv_t* kv)
{
	kv->in_array = kv->in_array_stack.pop();
}

static CUTE_INLINE void s_push_read_mode_array(kv_t* kv, kv_val_t* val)
{
	CUTE_ASSERT(!kv->read_mode_matched_val);
	kv->read_mode_array_stack.add(val);
	kv->read_mode_array_index_stack.add(0);
	kv->read_mode_from_array = val ? 1 : 0;
}

static CUTE_INLINE void s_pop_read_mode_array(kv_t* kv)
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

static CUTE_INLINE uint8_t s_peek(kv_t* kv)
{
	while (kv->in < kv->in_end && s_isspace(*kv->in)) kv->in++;
	return kv->in < kv->in_end ? *kv->in : 0;
}

static CUTE_INLINE uint8_t s_next(kv_t* kv)
{
	uint8_t c;
	if (kv->in == kv->in_end) return 0;
	while (s_isspace(c = *kv->in++)) if (kv->in == kv->in_end) return 0;
	return c;
}

static CUTE_INLINE int s_try(kv_t* kv, uint8_t expect)
{
	if (kv->in == kv->in_end) return 0;
	if (s_peek(kv) == expect)
	{
		kv->in++;
		return 1;
	}
	return 0;
}

#define s_expect(kv, expected_character) \
	do { \
		if (s_next(kv) != expected_character) { kv->err = error_failure("Found unexpected token."); return kv->err; } \
	} while (0)

static error_t s_scan_string(kv_t* kv, uint8_t** start_of_string, uint8_t** end_of_string)
{
	*start_of_string = NULL;
	*end_of_string = NULL;
	int has_quotes = s_try(kv, '"');
	*start_of_string = kv->in;
	if (has_quotes) {
		while (kv->in < kv->in_end)
		{
			uint8_t* end = (uint8_t*)CUTE_MEMCHR(kv->in, '"', kv->in_end - kv->in);
			if (*(end - 1) != '\\') {
				*end_of_string = end;
				kv->in = end + 1;
				break;
			}
		}
	} else {
		uint8_t* end = kv->in;
		while (end < kv->in_end && !s_isspace(*end)) end++;
		*end_of_string = end;
		kv->in = end + 1;
	}
	if (kv->in == kv->in_end) {
		kv->err = error_failure("Unterminated string at end of file.");
		return kv->err;
	}
	return error_success();
}

static error_t s_scan_string(kv_t* kv, kv_string_t* str)
{
	uint8_t* string_start;
	uint8_t* string_end;
	error_t err = s_scan_string(kv, &string_start, &string_end);
	str->str = string_start;
	str->len = (int)(string_end - string_start);
	return err;
}

static CUTE_INLINE error_t s_parse_int(kv_t* kv, int64_t* out)
{
	uint8_t* end;
	int64_t val = CUTE_STRTOLL((char*)kv->in, (char**)&end, 10);
	if (kv->in == end) {
		kv->err = error_failure("Invalid integer found during parse.");
		return kv->err;
	}
	kv->in = end;
	*out = val;
	return error_success();
}

static CUTE_INLINE error_t s_parse_float(kv_t* kv, double* out)
{
	uint8_t* end;
	double val = CUTE_STRTOD((char*)kv->in, (char**)&end);
	if (kv->in == end) {
		kv->err = error_failure("Invalid float found during parse.");
		return kv->err;
	}
	kv->in = end;
	*out = val;
	return error_success();
}

static error_t s_parse_hex(kv_t* kv, uint64_t* hex)
{
	s_expect(kv, '0');
	uint8_t c = s_next(kv);
	if (!(c != 'x' && c != 'X')) {
		kv->err = error_failure("Expected 'x' or 'X' when parsing a hex number.");
		return kv->err;
	}
	uint8_t* end;
	uint64_t val = (uint64_t)CUTE_STRTOLL((char*)kv->in, (char**)&end, 16);
	if (kv->in == end) {
		kv->err = error_failure("Invalid hex integer found during parse.");
		return kv->err;
	}
	kv->in = end;
	*hex = val;
	return error_success();
}

static CUTE_INLINE error_t s_parse_number(kv_t* kv, kv_val_t* val)
{
	error_t err;
	if (kv->in + 1 < kv->in_end && ((kv->in[1] == 'x') | (kv->in[1] == 'X'))) {
		uint64_t hex;
		err = s_parse_hex(kv, &hex);
		if (err.is_error()) return err;
		val->type = KV_TYPE_INT64;
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
			err = s_parse_float(kv, &dval);
			if (err.is_error()) return err;
			val->type = KV_TYPE_DOUBLE;
			val->u.dval = dval;
		} else {
			int64_t ival;
			err = s_parse_int(kv, &ival);
			if (err.is_error()) return err;
			val->type = KV_TYPE_INT64;
			val->u.ival = ival;
		}
	}
	return error_success();
}

error_t s_parse_value(kv_t* kv, kv_val_t* val);

static error_t s_parse_array(kv_t* kv, array<kv_val_t>* array_val)
{
	error_t err;
	int64_t count;
	s_expect(kv, '[');
	err = s_parse_int(kv, &count);
	if (err.is_error()) return err;
	s_expect(kv, ']');
	s_expect(kv, '{');
	array_val->ensure_capacity((int)count);
	for (int i = 0; i < (int)count; ++i)
	{
		kv_val_t* val = &array_val->add();
		CUTE_PLACEMENT_NEW(val) kv_val_t;
		err = s_parse_value(kv, val);
		if (err.is_error()) return err;
	}
	s_expect(kv, '}');
	return error_success();
}

error_t s_parse_object(kv_t* kv, int* index);

static error_t s_parse_value(kv_t* kv, kv_val_t* val)
{
	error_t err;
	uint8_t c = s_peek(kv);

	if (c == '"') {
		kv_string_t string;
		err = s_scan_string(kv, &string);
		if (err.is_error()) return err;
		val->type = KV_TYPE_STRING;
		val->u.sval = string;
	} else if ((c >= '0' && c <= '9') | (c == '-')) {
		err = s_parse_number(kv, val);
		if (err.is_error()) return err;
	} else if (c == '[') {
		err = s_parse_array(kv, &val->aval);
		if (err.is_error()) return err;
		val->type = KV_TYPE_ARRAY;
	} else if (c == '{') {
		int index;
		err = s_parse_object(kv, &index);
		if (err.is_error()) return err;
		val->type = KV_TYPE_OBJECT;
		val->u.object_index = index;
	} else {
		kv->err = error_failure("Unexpected character when parsing a value.");
		return kv->err;
	}

	s_try(kv, ',');

	return error_success();
}

static error_t s_parse_object(kv_t* kv, int* index)
{
	kv_object_t* object = &kv->objects.add();
	CUTE_PLACEMENT_NEW(object) kv_object_t;
	*index = kv->objects.count() - 1;
	int parent_index = *index;

	s_expect(kv, '{');

	while (!s_try(kv, '}'))
	{
		kv_field_t* field = &object->fields.add();
		CUTE_PLACEMENT_NEW(field) kv_field_t;

		error_t err = s_scan_string(kv, &field->key);
		if (err.is_error()) return err;
		s_expect(kv, '=');
		err = s_parse_value(kv, &field->val);
		if (err.is_error()) return err;

		// If objects were parsed, assign proper parent indices.
		if (field->val.type == KV_TYPE_OBJECT) {
			kv->objects[field->val.u.object_index].parent_index = parent_index;
		} else if (field->val.type == KV_TYPE_ARRAY) {
			int count = field->val.aval.count();
			if (count && field->val.aval[0].type == KV_TYPE_OBJECT) {
				array<kv_val_t>& object_val_array = field->val.aval;
				for (int i = 0; i < count; ++i)
				{
					int object_index = object_val_array[i].u.object_index;
					kv->objects[object_index].parent_index = parent_index;
				}
			}
		}
	}

	s_try(kv, ',');

	return error_success();
}

static void s_reset(kv_t* kv, const void* ptr, size_t size, int mode)
{
	kv->start = (uint8_t*)ptr;
	kv->in = (uint8_t*)ptr;
	kv->in_end = kv->in + size;
	kv->mode = mode;

	kv->top_level_object_indices.clear();
	kv->objects.clear();
	kv->read_mode_array_stack.clear();
	kv->read_mode_array_index_stack.clear();
	kv->in_array_stack.clear();

	kv->err = error_success();
}

error_t kv_parse(kv_t* kv, const void* data, size_t size)
{
	s_reset(kv, data, size, CUTE_KV_MODE_READ);

	while (s_peek(kv) == '{')
	{
		int index;
		error_t err = s_parse_object(kv, &index);
		if (err.is_error()) return err;
		kv->top_level_object_indices.add(index);
	}

	uint8_t c;
	while (kv->in != kv->in_end && s_isspace(c = *kv->in)) kv->in++;

	if (kv->in != kv->in_end) {
		kv->err = error_failure("Unable to parse entire input `data`.");
		return kv->err;
	}

	kv->start = NULL;
	kv->in = NULL;
	kv->in_end = NULL;

	return error_success();
}

void kv_set_write_buffer(kv_t* kv, void* buffer, size_t size)
{
	s_reset(kv, buffer, size, CUTE_KV_MODE_WRITE);
}

void kv_reset_read_state(kv_t* kv)
{
	CUTE_ASSERT(kv->mode == CUTE_KV_MODE_READ);
	kv->read_mode_matched_val = NULL;
	kv->read_mode_object_index = 0;
	kv->read_mode_top_level_index = 0;
	kv->read_mode_from_array = 0;
	kv->read_mode_array_stack.clear();
	kv->read_mode_array_index_stack.clear();
}

int kv_size_written(kv_t* kv)
{
	return (int)(kv->in - kv->start);
}

error_t kv_error_state(kv_t* kv)
{
	return kv->err;
}

static CUTE_INLINE error_t s_write_u8(kv_t* kv, uint8_t val)
{
	uint8_t* in = kv->in;
	uint8_t* end = kv->in + 1;
	if (end >= kv->in_end) {
		kv->err = error_failure("Attempted to write uint8_t beyond buffer.");
		return kv->err;
	}
	*in = val;
	kv->in = end;
	return error_success();
}

static CUTE_INLINE void s_try_consume_one_tab(kv_t* kv)
{
	if (kv->in - 1 >= kv->start && kv->in[-1] == '\t') {
		kv->in--;
	}
}

static CUTE_INLINE void s_try_consume_whitespace(kv_t* kv)
{
	while (kv->in - 1 >= kv->start && s_isspace(kv->in[-1])) {
		kv->in--;
	}
}

static CUTE_INLINE void s_tabs_delta(kv_t* kv, int delta)
{
	kv->tabs += delta;
}

static CUTE_INLINE error_t s_tabs(kv_t* kv)
{
	int tabs = kv->tabs;
	for (int i = 0; i < tabs; ++i)
	{
		error_t err = s_write_u8(kv, '\t');
		if (err.is_error()) return err;
	}
	return error_success();
}

static CUTE_INLINE error_t s_write_str_no_quotes(kv_t* kv, const char* str, size_t len)
{
	uint8_t* in = kv->in;
	uint8_t* end = kv->in + len;
	if (end >= kv->in_end) {
		kv->err = error_failure("Attempted to write string beyond buffer.");
		return kv->err;
	}
	CUTE_STRNCPY((char*)in, str, len);
	kv->in = end;
	return error_success();
}

static CUTE_INLINE error_t s_write_str(kv_t* kv, const char* str, size_t len)
{
	error_t err = s_write_u8(kv, '"');
	if (err.is_error()) return err;
	err = s_write_str_no_quotes(kv, str, len);
	if (err.is_error()) return err;
	err = s_write_u8(kv, '"');
	if (err.is_error()) return err;
	return error_success();
}

static CUTE_INLINE error_t s_write_str(kv_t* kv, const char* str)
{
	error_t err = s_write_str(kv, str, (int)CUTE_STRLEN(str));
	if (err.is_error()) return err;
	return error_success();
}

static CUTE_INLINE kv_field_t* s_find_field(kv_object_t* object, const char* key)
{
	size_t len = CUTE_STRLEN(key);
	int count = object->fields.count();
	for (int i = 0; i < count; ++i)
	{
		kv_field_t* field = object->fields + i;
		kv_string_t string = field->key;
		if (len != string.len) continue;
		if (CUTE_STRNCMP(key, (const char*)string.str, string.len)) continue;
		return field;
	}
	return NULL;
}

error_t kv_key(kv_t* kv, const char* key, kv_type_t* type)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_write_str_no_quotes(kv, key, (int)CUTE_STRLEN(key));
		if (err.is_error()) return err;
		err = s_write_str_no_quotes(kv, " = ", 3);
		if (err.is_error()) return err;
		CUTE_UNUSED(type);
	} else {
		if (kv->read_mode_from_array) {
			return error_failure("Can not lookup key while reading from array.");
		}

		kv_object_t* object = kv->objects + kv->read_mode_object_index;
		kv_field_t* field = s_find_field(object, key);
		if (field) {
			CUTE_ASSERT(field->val.type != KV_TYPE_NULL);
			kv->read_mode_matched_val = &field->val;
			if (type) *type = field->val.type;
		} else {
			return error_failure("Unable to find field to match `key`.");
		}
	}
	return error_success();
}

static uint8_t* s_temp(kv_t* kv, size_t size)
{
	if (kv->temp_size < size + 1) {
		CUTE_FREE(kv->temp, kv->mem_ctx);
		kv->temp_size = size + 1;
		kv->temp = (uint8_t*)CUTE_ALLOC(size + 1, kv->mem_ctx);
		if (size) CUTE_ASSERT(kv->temp);
	}
	return kv->temp;
}

static int s_to_string(kv_t* kv, uint64_t val)
{
	const char* fmt = "%" PRIu64;
	uint8_t* temp = s_temp(kv, 256);

	#ifdef _WIN32
		int size = _scprintf(fmt, val) + 1;
	#else
		int size = snprintf(temp, 0, fmt, val) + 1;
	#endif

	temp = s_temp(kv, size);
	snprintf((char*)temp, size, fmt, val);
	return size - 1;
}

static int s_to_string(kv_t* kv, int64_t val)
{
	const char* fmt = "%" PRIi64;
	uint8_t* temp = s_temp(kv, 256);

	#ifdef _WIN32
		int size = _scprintf(fmt, val) + 1;
	#else
		int size = snprintf(temp, 0, fmt, val) + 1;
	#endif

	temp = s_temp(kv, size);
	snprintf((char*)temp, size, fmt, val);
	return size - 1;
}

static int s_to_string(kv_t* kv, float val)
{
	const char* fmt = "%f";
	uint8_t* temp = s_temp(kv, 256);

	#ifdef _WIN32
		int size = _scprintf(fmt, val) + 1;
	#else
		int size = snprintf(temp, 0, fmt, val) + 1;
	#endif

	temp = s_temp(kv, size);
	snprintf((char*)temp, size, fmt, val);
	return size - 1;
}

static int s_to_string(kv_t* kv, double val)
{
	const char* fmt = "%f";
	uint8_t* temp = s_temp(kv, 256);

	#ifdef _WIN32
		int size = _scprintf(fmt, val) + 1;
	#else
		int size = snprintf(temp, 0, fmt, val) + 1;
	#endif

	temp = s_temp(kv, size);
	snprintf((char*)temp, size, fmt, val);
	return size - 1;
}

static error_t s_write(kv_t* kv, uint64_t val)
{
	int size = s_to_string(kv, val);
	return s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static error_t s_write(kv_t* kv, int64_t val)
{
	int size = s_to_string(kv, val);
	return s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static error_t s_write(kv_t* kv, float val)
{
	int size = s_to_string(kv, val);
	return s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static error_t s_write(kv_t* kv, double val)
{
	int size = s_to_string(kv, val);
	return s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static CUTE_INLINE error_t s_begin_val(kv_t* kv)
{
	if (kv->in_array) {
		if (kv->in_array == CUTE_KV_IN_ARRAY_AND_FIRST_ELEMENT) {
			kv->in_array = CUTE_KV_IN_ARRAY;
		} else {
			error_t err = s_write_u8(kv, ' ');
			if (err.is_error()) return err;
		}
	}
	return error_success();
}

static CUTE_INLINE error_t s_end_val(kv_t* kv)
{
	error_t err = s_write_u8(kv, ',');
	if (err.is_error()) return err;
	if (!kv->in_array) {
		err = s_write_u8(kv, '\n');
		if (err.is_error()) return err;
		err = s_tabs(kv);
		if (err.is_error()) return err;
	}
	return error_success();
}

static CUTE_INLINE kv_val_t* s_pop_val(kv_t* kv, kv_type_t type)
{
	if (kv->read_mode_from_array) {
		CUTE_ASSERT(!kv->read_mode_matched_val);
		kv_val_t* array_val = kv->read_mode_array_stack.last();
		int& index = kv->read_mode_array_index_stack.last();
		if (index == array_val->aval.count()) {
			return NULL;
		}
		kv_val_t* val = array_val->aval + index++;
		return val;
	} else {
		kv_val_t* val = kv->read_mode_matched_val;
		if (!val) return NULL;
		if (val->type != type) return NULL;
		kv->read_mode_matched_val = NULL;
		return val;
	}
}

error_t kv_val(kv_t* kv, uint8_t* val)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_begin_val(kv);
		if (err.is_error()) return err;
		err = s_write_u8(kv, *val);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_INT64);
		if (!matched_val) return error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		*val = (uint8_t)matched_val->u.ival;
	}
	return error_success();
}

error_t kv_val(kv_t* kv, uint16_t* val)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_begin_val(kv);
		if (err.is_error()) return err;
		err = s_write(kv, (uint64_t)*(uint16_t*)val);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_INT64);
		if (!matched_val) return error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		*val = (uint16_t)matched_val->u.ival;
	}
	return error_success();
}

error_t kv_val(kv_t* kv, uint32_t* val)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_begin_val(kv);
		if (err.is_error()) return err;
		err = s_write(kv, (uint64_t)*(uint32_t*)val);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_INT64);
		if (!matched_val) return error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		*val = (uint32_t)matched_val->u.ival;
	}
	return error_success();
}

error_t kv_val(kv_t* kv, uint64_t* val)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_begin_val(kv);
		if (err.is_error()) return err;
		err = s_write(kv, *(uint64_t*)val);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_INT64);
		if (!matched_val) return error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		*val = (uint64_t)matched_val->u.ival;
	}
	return error_success();
}

error_t kv_val(kv_t* kv, int8_t* val)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_begin_val(kv);
		if (err.is_error()) return err;
		err = s_write(kv, (int64_t)*(int8_t*)val);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_INT64);
		if (!matched_val) return error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		*val = (int8_t)matched_val->u.ival;
	}
	return error_success();
}

error_t kv_val(kv_t* kv, int16_t* val)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_begin_val(kv);
		if (err.is_error()) return err;
		err = s_write(kv, (int64_t)*(int16_t*)val);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_INT64);
		if (!matched_val) return error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		*val = (int16_t)matched_val->u.ival;
	}
	return error_success();
}

error_t kv_val(kv_t* kv, int32_t* val)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_begin_val(kv);
		if (err.is_error()) return err;
		err = s_write(kv, (int64_t)*(int32_t*)val);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_INT64);
		if (!matched_val) return error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		*val = (int32_t)matched_val->u.ival;
	}
	return error_success();
}

error_t kv_val(kv_t* kv, int64_t* val)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_begin_val(kv);
		if (err.is_error()) return err;
		err = s_write(kv, *(int64_t*)val);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_INT64);
		if (!matched_val) error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		*val = matched_val->u.ival;
	}
	return error_success();
}

error_t kv_val(kv_t* kv, float* val)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_begin_val(kv);
		if (err.is_error()) return err;
		err = s_write(kv, *val);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_DOUBLE);
		if (!matched_val) {
			matched_val = s_pop_val(kv, KV_TYPE_INT64);
			if (matched_val) {
				*val = (float)matched_val->u.ival;
			} else {
				return error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
			}
		} else {
			*val = (float)matched_val->u.dval;
		}
	}
	return error_success();
}

error_t kv_val(kv_t* kv, double* val)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_begin_val(kv);
		if (err.is_error()) return err;
		err = s_write(kv, *val);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_DOUBLE);
		if (!matched_val) {
			matched_val = s_pop_val(kv, KV_TYPE_INT64);
			if (matched_val) {
				*val = (double)matched_val->u.ival;
			} else {
				return error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
			}
		} else {
			*val = matched_val->u.dval;
		}
	}
	return error_success();
}

error_t kv_val_string(kv_t* kv, const char** str, size_t* size)
{
	if (kv->mode == CUTE_KV_MODE_READ) {
		*str = NULL;
		*size = 0;
	}
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_begin_val(kv);
		if (err.is_error()) return err;
		err = s_write_str(kv, *str, *size);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_STRING);
		if (!matched_val) return error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
		*str = (const char*)matched_val->u.sval.str;
		*size = (int)matched_val->u.sval.len;
	}
	return error_success();
}

error_t kv_val_blob(kv_t* kv, void* data, size_t* size, size_t capacity)
{
	if (kv->mode == CUTE_KV_MODE_READ) *size = 0;
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		size_t buffer_size = CUTE_BASE64_ENCODED_SIZE(*size);
		if (!(buffer_size <= capacity)) {
			kv->err = error_failure("`capacity` is too small to hold base 64 encoded `data`.");
			return kv->err;
		}
		uint8_t* buffer = s_temp(kv, buffer_size);
		error_t err = base64_encode(buffer, buffer_size, data, *size);
		if (err.is_error()) return err;
		err = s_write_str(kv, (const char*)buffer, buffer_size);
		if (err.is_error()) return err;
		err = s_end_val(kv);
		if (err.is_error()) return err;
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_STRING);
		if (!matched_val) {
			kv->err = error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
			return kv->err;
		}
		size_t buffer_size = CUTE_BASE64_DECODED_SIZE(matched_val->u.sval.len);
		if (!(buffer_size <= capacity)) {
			kv->err = error_failure("Decoded base 64 string is too large to store in `data`.");
			return kv->err;
		}
		error_t err = base64_decode(data, buffer_size, matched_val->u.sval.str, matched_val->u.sval.len);
		if (err.is_error()) return err;
		*size = buffer_size;
	}
	return error_success();
}

error_t kv_object_begin(kv_t* kv)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		error_t err = s_write_str_no_quotes(kv, "{\n", 2);
		if (err.is_error()) return err;
		s_tabs_delta(kv, 1);
		err = s_tabs(kv);
		if (err.is_error()) return err;
		s_push_array(kv, CUTE_KV_NOT_IN_ARRAY);
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_OBJECT);
		if (matched_val) {
			kv->read_mode_object_index = matched_val->u.object_index;
		} else {
			bool has_top_index = kv->read_mode_top_level_index != ~0;
			bool no_more_top_level_indices = kv->read_mode_top_level_index >= kv->top_level_object_indices.count();
			if ((!has_top_index) | no_more_top_level_indices) {
				kv->err = error_failure("Attempted to read object beyond end of input.");
				return kv->err;
			}
			kv->read_mode_object_index = kv->top_level_object_indices[kv->read_mode_top_level_index];
		}
		s_push_read_mode_array(kv, NULL);
	}
	return error_success();
}

error_t kv_object_end(kv_t* kv)
{
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_tabs_delta(kv, -1);
		s_try_consume_one_tab(kv);
		error_t err = s_write_str_no_quotes(kv, "},\n", 3);
		if (err.is_error()) return err;
		err = s_tabs(kv);
		if (err.is_error()) return err;
		s_pop_array(kv);
	} else {
		kv_object_t* object = kv->objects + kv->read_mode_object_index;
		if (object->parent_index == ~0) {
			if (kv->read_mode_top_level_index == ~0) {
				kv->err = error_failure("`kv_object_end` called an extra time.");
				return kv->err;
			}
			else if (kv->read_mode_top_level_index == kv->top_level_object_indices.count() - 1) {
				kv->read_mode_top_level_index = ~0;
			} else {
				kv->read_mode_object_index = kv->top_level_object_indices[++kv->read_mode_top_level_index];
			}
		} else {
			kv->read_mode_object_index = object->parent_index;
		}
		s_pop_read_mode_array(kv);
	}
	return error_success();
}

error_t kv_array_begin(kv_t* kv, int* count)
{
	if (kv->mode == CUTE_KV_MODE_READ) *count = 0;
	if (kv->err.is_error()) return kv->err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_tabs_delta(kv, 1);
		error_t err = s_write_u8(kv, '[');
		if (err.is_error()) return err;
		err = s_write(kv, (int64_t)*count);
		if (err.is_error()) return err;
		err = s_write_str_no_quotes(kv, "] {\n", 4);
		if (err.is_error()) return err;
		err = s_tabs(kv);
		if (err.is_error()) return err;
		s_push_array(kv, CUTE_KV_IN_ARRAY_AND_FIRST_ELEMENT);
	} else {
		kv_val_t* matched_val = s_pop_val(kv, KV_TYPE_ARRAY);
		if (!matched_val) {
			kv->err = error_failure("Unable to get `val` (out of bounds array index, or no matching `kv_key` call).");
			return kv->err;
		}
		s_push_read_mode_array(kv, matched_val);
		*count = matched_val->aval.count();
	}
	return error_success();
}

error_t kv_array_end(kv_t* kv)
{
	if (kv->err.is_error()) return kv->err;
	error_t err;
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_tabs_delta(kv, -1);
		s_try_consume_whitespace(kv);
		if (kv->in_array) {
			err = s_write_u8(kv, '\n');
			if (err.is_error()) return err;
		}
		err = s_tabs(kv);
		if (err.is_error()) return err;
		err = s_write_str_no_quotes(kv, "},\n", 3);
		if (err.is_error()) return err;
		err = s_tabs(kv);
		if (err.is_error()) return err;
		s_pop_array(kv);
	} else {
		s_pop_read_mode_array(kv);
	}
	return error_success();
}

void kv_print(kv_t* kv)
{
	printf("\n\n%.*s", (int)(kv->in - kv->start), kv->start);
}

}
