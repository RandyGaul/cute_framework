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

#include <stdio.h>
#include <inttypes.h>

enum kv_type_t
{
	CUTE_KV_TYPE_NULL   = -1,
	CUTE_KV_TYPE_UINT8  = 0,
	CUTE_KV_TYPE_UINT16 = 1,
	CUTE_KV_TYPE_UINT32 = 2,
	CUTE_KV_TYPE_UINT64 = 3,
	CUTE_KV_TYPE_INT8   = 4,
	CUTE_KV_TYPE_INT16  = 5,
	CUTE_KV_TYPE_INT32  = 6,
	CUTE_KV_TYPE_INT64  = 7,
	CUTE_KV_TYPE_FLOAT  = 8,
	CUTE_KV_TYPE_DOUBLE = 9,
	CUTE_KV_TYPE_STRING = 10,
	CUTE_KV_TYPE_ARRAY  = 11,
	CUTE_KV_TYPE_BLOB   = 12,
	CUTE_KV_TYPE_OBJECT = 13,
};

namespace cute
{

struct kv_string_t
{
	uint8_t* str = NULL;
	int len = 0;
};

struct kv_object_t
{
	int parent_index = ~0;
	kv_string_t key;

	array<kv_string_t> keys;
	array<kv_type_t> types;
	array<kv_string_t> values;
};

struct kv_t
{
	int mode;
	uint8_t* start;
	uint8_t* in;
	uint8_t* in_end;

	int offset_stack_count;
	int offset_stack_capacity;
	int* offset_stack;

	array <int> top_level_object_indices;
	array<kv_object_t> objects;

	int is_array;
	int is_array_stack_capacity;
	int is_array_stack_count;
	int* is_array_stack;

	int tabs;
	error_t error;
	int temp_size;
	uint8_t* temp;

	void* mem_ctx;
};

kv_t* kv_make(void* user_allocator_context)
{
	kv_t* kv = (kv_t*)CUTE_ALLOC(sizeof(kv_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(kv) kv_t;

	kv->mode = -1;
	kv->start = NULL;
	kv->in = NULL;
	kv->in_end = NULL;
	
	kv->offset_stack_count = 0;
	kv->offset_stack_capacity = 0;
	kv->offset_stack = NULL;

	kv->is_array = 0;
	kv->is_array_stack_capacity = 0;
	kv->is_array_stack_count = 0;
	kv->is_array_stack = NULL;

	kv->tabs = 0;
	kv->error = error_success();
	kv->temp_size = 0;
	kv->temp = NULL;

	kv->mem_ctx = user_allocator_context;

	return kv;
}

void kv_destroy(kv_t* kv)
{
	CUTE_FREE(kv, kv->mem_ctx);
}

static CUTE_INLINE void s_push_array(kv_t* kv, int is_array)
{
	CUTE_CHECK_BUFFER_GROW(kv, is_array_stack_count, is_array_stack_capacity, is_array_stack, int, 8, kv->mem_ctx);
	CUTE_ASSERT(kv->is_array_stack);
	kv->is_array_stack[kv->is_array_stack_count++] = kv->is_array;
	kv->is_array = is_array;
}

static CUTE_INLINE void s_pop_array(kv_t* kv)
{
	CUTE_ASSERT(kv->is_array_stack_count > 0);
	kv->is_array = kv->is_array_stack[--kv->is_array_stack_count];
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

static CUTE_INLINE uint8_t s_peak(kv_t* kv)
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
	if (s_peak(kv) == expect)
	{
		kv->in++;
		return 1;
	}
	return 0;
}

#define CUTE_KV_CHECK_CONDITION(condition, failure_details) do { if (!(condition)) { return error_failure(failure_details); } } while (0)
#define CUTE_KV_CHECK(x) do { error_t err = (x); if (err.is_error()) return err; } while (0)

#define s_expect(kv, expected_character) \
	do { \
		CUTE_KV_CHECK_CONDITION(s_next(kv) != expected_character, "kv : Found unexpected token."); \
	} while (0)

static error_t s_scan_string(kv_t* kv, uint8_t** end_of_string)
{
	*end_of_string = NULL;
	s_expect(kv, '"');
	while (kv->in < kv->in_end)
	{
		uint8_t* end = (uint8_t*)CUTE_MEMCHR(kv->in, '"', kv->in_end - kv->in);
		if (*(end - 1) != '\\') {
			*end_of_string = end;
			break;
		}
	}
	if (kv->in == kv->in_end) return error_failure("kv : Unterminated string at end of file.");
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

error_t kv_reset(kv_t* kv, const void* data, int size, int mode)
{
	kv->start = (uint8_t*)data;
	kv->in = (uint8_t*)data;
	kv->in_end = kv->in + size;
	kv->mode = mode;

	if (mode == CUTE_KV_MODE_READ) {
		/*
			Start parsing at top level.
			The top level should have special rules.
			*Only objects* at the top level, without keys.

			User can peek at their types. So we can simply store them in an array.
		*/

		kv->top_level_object_indices.add(0);

		// Read in objects in a loop. Add them to top level indices array.
		// Parse out the members iteratively.

		// WORKING HERE
		// Scanning for top level object.
		// Will probably need typesafe dictionary soon (like array).

		uint8_t* end;
		CUTE_KV_CHECK(s_scan_string(kv, &end));
	}

	return error_success();
}

void kv_peek_object(kv_t* kv, const char** str, int* len)
{
}

static CUTE_INLINE int s_is_error(kv_t* kv)
{
	return kv->error.is_error();
}

static CUTE_INLINE void s_error(kv_t* kv, const char* details)
{
	if (!kv->error.is_error()) {
		kv->error = error_failure(details);
	}
}

static CUTE_INLINE void s_write_u8(kv_t* kv, uint8_t val)
{
	uint8_t* in = kv->in;
	uint8_t* end = kv->in + 1;
	if (end >= kv->in_end) {
		s_error(kv, "kv : Attempted to write uint8_t beyond buffer.");
		return;
	}
	*in = val;
	kv->in = end;
}

static CUTE_INLINE void s_tabs(kv_t* kv, int delta = 0)
{
	if (delta < 0) kv->tabs += delta;
	int tabs = kv->tabs;
	for (int i = 0; i < tabs; ++i)
		s_write_u8(kv, '\t');
	if (delta > 0) kv->tabs += delta;
}

static CUTE_INLINE void s_write_str_no_quotes(kv_t* kv, const char* str, int len)
{
	uint8_t* in = kv->in;
	uint8_t* end = kv->in + len;
	if (end >= kv->in_end) {
		s_error(kv, "kv : Attempted to write string beyond buffer.");
		return;
	}
	// TODO: Handle escapes and utf8 somehow.
	CUTE_STRNCPY((char*)in, str, len);
	kv->in = end;
}

static CUTE_INLINE void s_write_str(kv_t* kv, const char* str, int len)
{
	s_write_u8(kv, '"');
	s_write_str_no_quotes(kv, str, len);
	s_write_u8(kv, '"');
}

static CUTE_INLINE void s_write_str(kv_t* kv, const char* str)
{
	s_write_str(kv, str, (int)CUTE_STRLEN(str));
}

void kv_object_begin(kv_t* kv, const char* key, const char* type_id)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		if (!kv->is_array) {
			s_tabs(kv, 1);
			if (key) {
				s_write_str(kv, key);
				s_write_str_no_quotes(kv, " -> ", 4);
			} else {
				s_write_str_no_quotes(kv, "-> ", 3);
			}
			s_write_str(kv, type_id);
			s_write_str_no_quotes(kv, " {\n", 3);
		} else {
			s_write_str_no_quotes(kv, "{\n", 2);
			kv->tabs += 1;
		}
		s_push_array(kv, 0);
	} else {
	}
}

static CUTE_INLINE int s_match(kv_t* kv, const char* key)
{
	return 1;
}

static CUTE_INLINE void s_skip_until(kv_t* kv, uint8_t val)
{
}

static uint8_t* s_temp(kv_t* kv, int size)
{
	if (kv->temp_size < size + 1) {
		CUTE_FREE(kv->temp, kv->mem_ctx);
		kv->temp_size = size + 1;
		kv->temp = (uint8_t*)CUTE_ALLOC(size + 1, kv->mem_ctx);
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

static void s_write(kv_t* kv, uint64_t val)
{
	int size = s_to_string(kv, val);
	s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static void s_write(kv_t* kv, int64_t val)
{
	int size = s_to_string(kv, val);
	s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static void s_write(kv_t* kv, float val)
{
	int size = s_to_string(kv, val);
	s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static void s_write(kv_t* kv, double val)
{
	int size = s_to_string(kv, val);
	s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

error_t kv_object_end(kv_t* kv)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_tabs(kv, -1);
		s_write_u8(kv, '}');
		s_write_u8(kv, ',');
		s_write_u8(kv, '\n');
		s_tabs(kv);
		s_pop_array(kv);
	} else {
	}

	return kv->error;
}

static CUTE_INLINE void s_field_begin(kv_t* kv, const char* key)
{
	if (!kv->is_array) {
		s_tabs(kv);
		s_write_str(kv, key);
		s_write_u8(kv, ' ');
		s_write_u8(kv, ':');
		s_write_u8(kv, ' ');
	}
}

static CUTE_INLINE void s_field_end(kv_t* kv)
{
	if (!kv->is_array) {
		s_write_u8(kv, ',');
		s_write_u8(kv, '\n');
	} else {
		s_write_u8(kv, ',');
		s_write_u8(kv, ' ');
	}
}

void kv_field(kv_t* kv, const char* key, uint8_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write_u8(kv, *val);
		s_field_end(kv);
	} else {
	}
}

void kv_field(kv_t* kv, const char* key, uint16_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, (uint64_t)*(uint16_t*)val);
		s_field_end(kv);
	} else {
	}
}

void kv_field(kv_t* kv, const char* key, uint32_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, (uint64_t)*(uint32_t*)val);
		s_field_end(kv);
	} else {
	}
}

void kv_field(kv_t* kv, const char* key, uint64_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, *(uint64_t*)val);
		s_field_end(kv);
	} else {
	}
}

void kv_field(kv_t* kv, const char* key, int8_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, (int64_t)*(int8_t*)val);
		s_field_end(kv);
	} else {
	}
}

void kv_field(kv_t* kv, const char* key, int16_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, (int64_t)*(int16_t*)val);
		s_field_end(kv);
	} else {
	}
}

void kv_field(kv_t* kv, const char* key, int32_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, (int64_t)*(int32_t*)val);
		s_field_end(kv);
	} else {
	}
}

void kv_field(kv_t* kv, const char* key, int64_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, *(int64_t*)val);
		s_field_end(kv);
	} else {
	}
}

void kv_field(kv_t* kv, const char* key, float* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, *val);
		s_field_end(kv);
	} else {
	}
}

void kv_field(kv_t* kv, const char* key, double* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, *val);
		s_field_end(kv);
	} else {
	}
}

void kv_field_str(kv_t* kv, const char* key, char** str, int* size)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write_str(kv, *str, *size);
		s_field_end(kv);
	} else {
	}
}

void kv_field_blob(kv_t* kv, const char* key, void* data, int* size)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		int buffer_size = CUTE_BASE64_ENCODED_SIZE(*size);
		uint8_t* buffer = s_temp(kv, buffer_size);
		base64_encode(buffer, buffer_size, data, *size);
		s_write_str(kv, (const char*)buffer, buffer_size);
		s_field_end(kv);
	} else {
	}
}

void kv_field_array_begin(kv_t* kv, const char* key, int* count, const char* type_id)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		if (type_id) {
			s_tabs(kv);
			s_write_str(kv, key);
			s_write_str_no_quotes(kv, " -> ", 4);
			s_write_str(kv, type_id);
			s_write_u8(kv, ' ');
		} else {
			s_field_begin(kv, key);
		}
		s_write_str_no_quotes(kv, "[\n", 2);
		kv->tabs += 1;
		s_tabs(kv);
		s_push_array(kv, 1);
	} else {
	}
}

void kv_field_array_end(kv_t* kv)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_write_u8(kv, '\n');
		s_tabs(kv, -1);
		s_write_str_no_quotes(kv, "],\n", 3);
		s_pop_array(kv);
	} else {
	}
}

void kv_print(kv_t* kv)
{
	printf("\n\n%.*s", (int)(kv->in - kv->start), kv->start);
}

}
