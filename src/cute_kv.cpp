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

struct kv_str_t
{
	char** str;
	size_t* size;
};

struct kv_blob_t
{
	void* data;
	size_t* size;
};

struct kv_entry_t
{
	kv_type_t type;
	const char* key;
	union
	{
		void* ptr;
		kv_str_t str;
		kv_blob_t blob;
		int* array_count;
	} u;
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

	int entry_capacity;
	int entry_count;
	kv_entry_t* entries;

	int tabs;
	error_t error;
	size_t temp_size;
	uint8_t* temp;

	void* mem_ctx;
};

kv_t* kv_make(void* user_allocator_context)
{
	kv_t* kv = (kv_t*)CUTE_ALLOC(sizeof(kv_t), user_allocator_context);

	kv->mode = -1;
	kv->start = NULL;
	kv->in = NULL;
	kv->in_end = NULL;
	
	kv->offset_stack_count = 0;
	kv->offset_stack_capacity = 0;
	kv->offset_stack = NULL;

	kv->entry_capacity = 0;
	kv->entry_count = 0;
	kv->entries = NULL;

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

void kv_reset(kv_t* kv, const void* data, size_t size, int mode)
{
	kv->start = (uint8_t*)data;
	kv->in = (uint8_t*)data;
	kv->in_end = kv->in + size;
	kv->mode = mode;
}

void kv_peek_object(kv_t* kv, const char** str, size_t* len)
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

static CUTE_INLINE void s_write_str_no_quotes(kv_t* kv, const char* str, size_t len)
{
	uint8_t* in = kv->in;
	uint8_t* end = kv->in + len;
	if (end >= kv->in_end) {
		s_error(kv, "kv : Attempted to write string beyond buffer.");
		return;
	}
	CUTE_STRNCPY((char*)in, str, len);
	kv->in = end;
}

static CUTE_INLINE void s_write_str(kv_t* kv, const char* str, size_t len)
{
	s_write_u8(kv, '"');
	s_write_str_no_quotes(kv, str, len);
	s_write_u8(kv, '"');
}

static CUTE_INLINE void s_write_str(kv_t* kv, const char* str)
{
	s_write_str(kv, str, CUTE_STRLEN(str));
}

void kv_begin(kv_t* kv, const char* key, const char* type_id)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_tabs(kv, 1);
		if (key) {
			s_write_str(kv, key);
			s_write_str_no_quotes(kv, " -> ", 4);
			s_write_str(kv, type_id);
			s_write_str_no_quotes(kv, " {\n", 3);
		} else {
			s_write_str_no_quotes(kv, "-> ", 3);
			s_write_str(kv, type_id);
			s_write_str_no_quotes(kv, " {\n", 3);
		}

		CUTE_CHECK_BUFFER_GROW(kv, offset_stack_count, offset_stack_capacity, offset_stack, int, 16, mem_ctx);
		kv->offset_stack[kv->offset_stack_count++] = kv->entry_count;
	}
}

static CUTE_INLINE int s_match(kv_t* kv, const char* key)
{
	return 1;
}

static CUTE_INLINE void s_skip_until(kv_t* kv, uint8_t val)
{
}

static uint8_t* s_temp(kv_t* kv, size_t size)
{
	if (kv->temp_size < size + 1) {
		CUTE_FREE(kv->temp, kv->mem_ctx);
		kv->temp_size = size + 1;
		kv->temp = (uint8_t*)CUTE_ALLOC(size + 1, kv->mem_ctx);
	}
	return kv->temp;
}

static size_t s_to_string(kv_t* kv, uint64_t val)
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

static size_t s_to_string(kv_t* kv, int64_t val)
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

static size_t s_to_string(kv_t* kv, float val)
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

static size_t s_to_string(kv_t* kv, double val)
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
	size_t size = s_to_string(kv, val);
	s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static void s_write(kv_t* kv, int64_t val)
{
	size_t size = s_to_string(kv, val);
	s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static void s_write(kv_t* kv, float val)
{
	size_t size = s_to_string(kv, val);
	s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

static void s_write(kv_t* kv, double val)
{
	size_t size = s_to_string(kv, val);
	s_write_str_no_quotes(kv, (char*)kv->temp, size);
}

error_t kv_end(kv_t* kv)
{
	int offset = kv->offset_stack[--kv->offset_stack_count];
	int count = kv->entry_count - offset;
	kv_entry_t* entries = kv->entries + offset;
	kv->entry_count = offset;


	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_tabs(kv, -1);
		s_write_u8(kv, '}');
		s_write_u8(kv, ',');
		s_write_u8(kv, '\n');
	} else {
		for (int i = 0; i < count; ++i)
		{
			kv_entry_t* entry = entries + i;

			switch (entry->type)
			{
			case CUTE_KV_TYPE_UINT8:
				break;

			case CUTE_KV_TYPE_UINT16:
				break;

			case CUTE_KV_TYPE_UINT32:
				break;

			case CUTE_KV_TYPE_UINT64:
				break;

			case CUTE_KV_TYPE_INT8:
				break;

			case CUTE_KV_TYPE_INT16:
				break;

			case CUTE_KV_TYPE_INT32:
				break;

			case CUTE_KV_TYPE_INT64:
				break;

			case CUTE_KV_TYPE_FLOAT:
				break;

			case CUTE_KV_TYPE_DOUBLE:
				break;

			case CUTE_KV_TYPE_STRING:
				break;

			case CUTE_KV_TYPE_ARRAY:
				break;

			case CUTE_KV_TYPE_BLOB:
				break;

			default:
				s_error(kv, "kv : Encountered unknown field.");
				break;
			}
		}
	}

	return kv->error;
}

static CUTE_INLINE void s_entry_ptr(kv_t* kv, const char* key, void* val, kv_type_t type)
{
	CUTE_CHECK_BUFFER_GROW(kv, entry_count, entry_capacity, entries, kv_entry_t, 32, kv->mem_ctx);
	kv_entry_t entry;
	entry.type = type;
	entry.key = key;
	entry.u.ptr = val;
	kv->entries[kv->entry_count++] = entry;
}

static CUTE_INLINE void s_field_begin(kv_t* kv, const char* key)
{
	s_tabs(kv);
	s_write_str(kv, key);
	s_write_u8(kv, ' ');
	s_write_u8(kv, ':');
	s_write_u8(kv, ' ');
}

static CUTE_INLINE void s_field_end(kv_t* kv)
{
	s_write_u8(kv, ',');
	s_write_u8(kv, '\n');
}

void kv_field(kv_t* kv, const char* key, uint8_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write_u8(kv, *val);
		s_field_end(kv);
	} else {
		s_entry_ptr(kv, key, val, CUTE_KV_TYPE_UINT8);
	}
}

void kv_field(kv_t* kv, const char* key, uint16_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, (uint64_t)*(uint16_t*)val);
		s_field_end(kv);
	} else {
		s_entry_ptr(kv, key, val, CUTE_KV_TYPE_UINT16);
	}
}

void kv_field(kv_t* kv, const char* key, uint32_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, (uint64_t)*(uint32_t*)val);
		s_field_end(kv);
	} else {
		s_entry_ptr(kv, key, val, CUTE_KV_TYPE_UINT32);
	}
}

void kv_field(kv_t* kv, const char* key, uint64_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, *(uint64_t*)val);
		s_field_end(kv);
	} else {
		s_entry_ptr(kv, key, val, CUTE_KV_TYPE_UINT64);
	}
}

void kv_field(kv_t* kv, const char* key, int8_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, (int64_t)*(int8_t*)val);
		s_field_end(kv);
	} else {
		s_entry_ptr(kv, key, val, CUTE_KV_TYPE_INT8);
	}
}

void kv_field(kv_t* kv, const char* key, int16_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, (int64_t)*(int16_t*)val);
		s_field_end(kv);
	} else {
		s_entry_ptr(kv, key, val, CUTE_KV_TYPE_INT16);
	}
}

void kv_field(kv_t* kv, const char* key, int32_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, (int64_t)*(int32_t*)val);
		s_field_end(kv);
	} else {
		s_entry_ptr(kv, key, val, CUTE_KV_TYPE_INT32);
	}
}

void kv_field(kv_t* kv, const char* key, int64_t* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, *(int64_t*)val);
		s_field_end(kv);
	} else {
		s_entry_ptr(kv, key, val, CUTE_KV_TYPE_INT64);
	}
}

void kv_field(kv_t* kv, const char* key, float* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, *val);
		s_field_end(kv);
	} else {
		s_entry_ptr(kv, key, val, CUTE_KV_TYPE_FLOAT);
	}
}

void kv_field(kv_t* kv, const char* key, double* val)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write(kv, *val);
		s_field_end(kv);
	} else {
		s_entry_ptr(kv, key, val, CUTE_KV_TYPE_DOUBLE);
	}
}

void kv_field_str(kv_t* kv, const char* key, char** str, size_t* size)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
		s_field_begin(kv, key);
		s_write_str(kv, *str, *size);
		s_field_end(kv);
	} else {
		CUTE_CHECK_BUFFER_GROW(kv, entry_count, entry_capacity, entries, kv_entry_t, 32, kv->mem_ctx);
		kv_entry_t entry;
		entry.type = CUTE_KV_TYPE_STRING;
		entry.key = key;
		entry.u.str.str = str;
		entry.u.str.size = size;
		kv->entries[kv->entry_count++] = entry;
	}
}

void kv_field_blob(kv_t* kv, const char* key, void* data, size_t* size)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
	} else {
		CUTE_CHECK_BUFFER_GROW(kv, entry_count, entry_capacity, entries, kv_entry_t, 32, kv->mem_ctx);
		kv_entry_t entry;
		entry.type = CUTE_KV_TYPE_BLOB;
		entry.key = key;
		entry.u.blob.data = data;
		entry.u.blob.size = size;
		kv->entries[kv->entry_count++] = entry;
	}
}

void kv_field_array(kv_t* kv, const char* key, int* count)
{
	if (kv->mode == CUTE_KV_MODE_WRITE) {
	} else {
		CUTE_CHECK_BUFFER_GROW(kv, entry_count, entry_capacity, entries, kv_entry_t, 32, kv->mem_ctx);
		kv_entry_t entry;
		entry.type = CUTE_KV_TYPE_BLOB;
		entry.key = key;
		entry.u.array_count = count;
		kv->entries[kv->entry_count++] = entry;
	}
}

void kv_print(kv_t* kv)
{
	printf("\n\n%.*s", (int)(kv->in - kv->start), kv->start);
}

}
