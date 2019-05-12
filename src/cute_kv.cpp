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

namespace cute
{

struct kv_string_t
{
	uint8_t* str = NULL;
	int len = 0;
};

enum kv_type_t
{
	CUTE_KV_TYPE_NULL   = 0,
	CUTE_KV_TYPE_INT64  = 1,
	CUTE_KV_TYPE_DOUBLE = 2,
	CUTE_KV_TYPE_STRING = 3,
	CUTE_KV_TYPE_ARRAY  = 4,
	CUTE_KV_TYPE_BLOB   = 5,
	CUTE_KV_TYPE_OBJECT = 6,
};

union kv_union_t
{
	int64_t ival;
	double dval;
	kv_string_t sval;
	kv_string_t bval;
	int object_index;
	kv_type_t array_type;
};

struct kv_val_t
{
	kv_type_t type;
	kv_union_t u;
	array<kv_union_t> aval;
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

	array<int> top_level_object_indices;
	array<kv_object_t> objects;

	int in_array = CUTE_KV_NOT_IN_ARRAY;
	array<int> in_array_stack;

	int tabs = 0;
	int temp_size = 0;
	uint8_t* temp = NULL;

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
		CUTE_RETURN_IF_FALSE(s_next(kv) == expected_character, "kv : Found unexpected token."); \
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
		uint8_t* end = (uint8_t*)CUTE_MEMCHR(kv->in, ',', kv->in_end - kv->in);
		*end_of_string = end;
		kv->in = end + 1;
	}
	if (kv->in == kv->in_end) return error_failure("kv : Unterminated string at end of file.");
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

static CUTE_INLINE error_t s_skip_to(kv_t* kv, uint8_t c)
{
	int has_quotes = s_try(kv, '"');
	if (has_quotes) {
		while (1)
		{
			// Skip over a string.
			while (kv->in < kv->in_end)
			{
				uint8_t* end = (uint8_t*)CUTE_MEMCHR(kv->in, '"', kv->in_end - kv->in);
				if (end == kv->in_end) return error_failure("kv : Unterminated string at end of file.");
				if (*(end - 1) != '\\') {
					kv->in = end + 1;
					break;
				}
			}

			uint8_t* end = (uint8_t*)CUTE_MEMCHR(kv->in, c, kv->in_end - kv->in);
			if (end == kv->in_end) return error_failure("kv : End of file encountered abruptly.");
			uint8_t* quote = (uint8_t*)CUTE_MEMCHR(kv->in, '"', kv->in_end - kv->in);
			kv->in = end + 1;
			if (end < quote) {
				break;
			}
		}
	} else {
		uint8_t* end = (uint8_t*)CUTE_MEMCHR(kv->in, c, kv->in_end - kv->in);
		if (end == kv->in_end) return error_failure("kv : End of file encountered abruptly.");
		kv->in = end + 1;
	}

	return error_success();
}

static CUTE_INLINE uint8_t s_parse_escape_code(uint8_t c)
{
	switch (c)
	{
	case '\\': return '\\';
	case '\'': return '\'';
	case '"': return '"';
	case 't': return '\t';
	case 'f': return '\f';
	case 'n': return '\n';
	case 'r': return '\r';
	case '0': return '\0';
	default: return c;
	}
}

static CUTE_INLINE error_t s_parse_int(kv_t* kv, int* out)
{
	uint8_t* end;
	int val = (int)strtoll((char*)kv->in, (char**)&end, 10);
	CUTE_RETURN_IF_FALSE(kv->in != end, "Invalid integer found during parse.");
	kv->in = end;
	*out = val;
	return error_success();
}

static CUTE_INLINE error_t s_parse_object(kv_t* kv)
{
	return error_success();
}

error_t kv_reset(kv_t* kv, const void* data, int size, int mode)
{
	kv->start = (uint8_t*)data;
	kv->in = (uint8_t*)data;
	kv->in_end = kv->in + size;
	kv->mode = mode;

	if (mode == CUTE_KV_MODE_READ) {
		while (s_peek(kv) == '{')
		{
			CUTE_RETURN_IF_ERROR(s_parse_object(kv));
		}
	}

	return error_success();
}

int kv_size_written(kv_t* kv)
{
	return (int)(kv->in - kv->start);
}

static CUTE_INLINE error_t s_write_u8(kv_t* kv, uint8_t val)
{
	uint8_t* in = kv->in;
	uint8_t* end = kv->in + 1;
	if (end >= kv->in_end) {
		error_failure("Attempted to write uint8_t beyond buffer.");
	}
	*in = val;
	kv->in = end;
	return error_success();
}

static CUTE_INLINE void s_tabs_delta(kv_t* kv, int delta)
{
	kv->tabs += delta;
}

static CUTE_INLINE error_t s_tabs(kv_t* kv)
{
	int tabs = kv->tabs;
	for (int i = 0; i < tabs; ++i)
		CUTE_RETURN_IF_ERROR(s_write_u8(kv, '\t'));
	return error_success();
}

static CUTE_INLINE error_t s_write_str_no_quotes(kv_t* kv, const char* str, int len)
{
	uint8_t* in = kv->in;
	uint8_t* end = kv->in + len;
	if (end >= kv->in_end) {
		return error_failure("Attempted to write string beyond buffer.");
	}
	// TODO: Handle escapes and utf8 somehow.
	CUTE_STRNCPY((char*)in, str, len);
	kv->in = end;
	return error_success();
}

static CUTE_INLINE error_t s_write_str(kv_t* kv, const char* str, int len)
{
	CUTE_RETURN_IF_ERROR(s_write_u8(kv, '"'));
	CUTE_RETURN_IF_ERROR(s_write_str_no_quotes(kv, str, len));
	CUTE_RETURN_IF_ERROR(s_write_u8(kv, '"'));
	return error_success();
}

static CUTE_INLINE error_t s_write_str(kv_t* kv, const char* str)
{
	CUTE_RETURN_IF_ERROR(s_write_str(kv, str, (int)CUTE_STRLEN(str)));
	return error_success();
}

error_t kv_key(kv_t* kv, const char* key)
{
	CUTE_RETURN_IF_ERROR(s_tabs(kv));
	CUTE_RETURN_IF_ERROR(s_write_str_no_quotes(kv, key, (int)CUTE_STRLEN(key)));
	CUTE_RETURN_IF_ERROR(s_write_str_no_quotes(kv, " = ", 3));
	return error_success();
}

static uint8_t* s_temp(kv_t* kv, int size)
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
			CUTE_RETURN_IF_ERROR(s_tabs(kv));
		} else {
			CUTE_RETURN_IF_ERROR(s_write_u8(kv, ' '));
		}
	}
	return error_success();
}

static CUTE_INLINE error_t s_end_val(kv_t* kv)
{
	CUTE_RETURN_IF_ERROR(s_write_u8(kv, ','));
	if (!kv->in_array) {
		CUTE_RETURN_IF_ERROR(s_write_u8(kv, '\n'));
	}
	return error_success();
}

error_t kv_val(kv_t* kv, uint8_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		CUTE_RETURN_IF_ERROR(s_begin_val(kv));
		CUTE_RETURN_IF_ERROR(s_write_u8(kv, *val));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_val(kv_t* kv, uint16_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		
		CUTE_RETURN_IF_ERROR(s_begin_val(kv));
		CUTE_RETURN_IF_ERROR(s_write(kv, (uint64_t)*(uint16_t*)val));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_val(kv_t* kv, uint32_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		
		CUTE_RETURN_IF_ERROR(s_begin_val(kv));
		CUTE_RETURN_IF_ERROR(s_write(kv, (uint64_t)*(uint32_t*)val));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_val(kv_t* kv, uint64_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		
		CUTE_RETURN_IF_ERROR(s_begin_val(kv));
		CUTE_RETURN_IF_ERROR(s_write(kv, *(uint64_t*)val));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_val(kv_t* kv, int8_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		
		CUTE_RETURN_IF_ERROR(s_begin_val(kv));
		CUTE_RETURN_IF_ERROR(s_write(kv, (int64_t)*(int8_t*)val));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_val(kv_t* kv, int16_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		
		CUTE_RETURN_IF_ERROR(s_begin_val(kv));
		CUTE_RETURN_IF_ERROR(s_write(kv, (int64_t)*(int16_t*)val));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_val(kv_t* kv, int32_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		
		CUTE_RETURN_IF_ERROR(s_begin_val(kv));
		CUTE_RETURN_IF_ERROR(s_write(kv, (int64_t)*(int32_t*)val));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_val(kv_t* kv, int64_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		
		CUTE_RETURN_IF_ERROR(s_begin_val(kv));
		CUTE_RETURN_IF_ERROR(s_write(kv, *(int64_t*)val));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_val(kv_t* kv, float* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		
		CUTE_RETURN_IF_ERROR(s_begin_val(kv));
		CUTE_RETURN_IF_ERROR(s_write(kv, *val));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_val(kv_t* kv, double* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		
		CUTE_RETURN_IF_ERROR(s_begin_val(kv));
		CUTE_RETURN_IF_ERROR(s_write(kv, *val));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_val_string(kv_t* kv, char** str, int* size)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		CUTE_RETURN_IF_ERROR(s_begin_val(kv));
		CUTE_RETURN_IF_ERROR(s_write_str(kv, *str, *size));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_val_blob(kv_t* kv, void* data, int* size)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		int buffer_size = CUTE_BASE64_ENCODED_SIZE(*size);
		uint8_t* buffer = s_temp(kv, buffer_size);
		CUTE_RETURN_IF_ERROR(base64_encode(buffer, buffer_size, data, *size));
		CUTE_RETURN_IF_ERROR(s_write_str(kv, (const char*)buffer, buffer_size));
		CUTE_RETURN_IF_ERROR(s_end_val(kv));
	} else {
	}
	return error_success();
}

error_t kv_object_begin(kv_t* kv)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		if (kv->in_array) {
			if (kv->in_array == CUTE_KV_IN_ARRAY_AND_FIRST_ELEMENT) {
				kv->in_array = CUTE_KV_IN_ARRAY;
				CUTE_RETURN_IF_ERROR(s_tabs(kv));
			}
		}
		CUTE_RETURN_IF_ERROR(s_write_str_no_quotes(kv, "{\n", 2));
		s_tabs_delta(kv, 1);
		s_push_array(kv, CUTE_KV_NOT_IN_ARRAY);
	} else {
	}
	return error_success();
}

error_t kv_object_end(kv_t* kv)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_tabs_delta(kv, -1);
		CUTE_RETURN_IF_ERROR(s_tabs(kv));
		if (kv->in_array_stack[kv->in_array_stack.count() - 1]) {
			CUTE_RETURN_IF_ERROR(s_write_str_no_quotes(kv, "}, ", 3));
		} else {
			CUTE_RETURN_IF_ERROR(s_write_str_no_quotes(kv, "},\n", 3));
		}
		s_pop_array(kv);
	} else {
	}
	return error_success();
}

error_t kv_array_begin(kv_t* kv, int* count)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		if (kv->in_array) {
			CUTE_RETURN_IF_ERROR(s_tabs(kv));
		}
		s_tabs_delta(kv, 1);
		CUTE_RETURN_IF_ERROR(s_write_u8(kv, '['));
		CUTE_RETURN_IF_ERROR(s_write(kv, (int64_t)*count));
		CUTE_RETURN_IF_ERROR(s_write_str_no_quotes(kv, "] {\n", 4));
		s_push_array(kv, CUTE_KV_IN_ARRAY_AND_FIRST_ELEMENT);
	} else {
	}
	return error_success();
}

error_t kv_array_end(kv_t* kv)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_pop_array(kv);
		s_tabs_delta(kv, -1);
		CUTE_RETURN_IF_ERROR(s_write_u8(kv, '\n'));
		CUTE_RETURN_IF_ERROR(s_tabs(kv));
		CUTE_RETURN_IF_ERROR(s_write_str_no_quotes(kv, "},\n", 3));
	} else {
	}
	return error_success();
}

void kv_print(kv_t* kv)
{
	printf("\n\n%.*s", (int)(kv->in - kv->start), kv->start);
}

}
