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

#include "cute_string.h"
#include "cute_concurrency.h"
#include "cute_handle_table.h"
#include "cute_array.h"
#include "cute_math.h"

#include <cute_string.h>
#include <cute_alloc.h>
#include <cute_strpool.h>

#include <internal/cute_app_internal.h>

#include <inttypes.h>

using namespace cute;

void* cf_agrow(const void* a, int new_size, size_t element_size)
{
	ACANARY(a);
	CUTE_ASSERT(acap(a) <= (SIZE_MAX - 1)/2);
	int new_capacity = max(2 * acap(a), max(new_size, 16));
	CUTE_ASSERT(new_size <= new_capacity);
	CUTE_ASSERT(new_capacity <= (SIZE_MAX - sizeof(cf_ahdr_t)) / element_size);
	size_t total_size = sizeof(cf_ahdr_t) + new_capacity * element_size;
	cf_ahdr_t* hdr;
	if (a) {
		hdr = (cf_ahdr_t*)CUTE_REALLOC(ahdr(a), total_size, NULL);
	} else {
		hdr = (cf_ahdr_t*)CUTE_ALLOC(total_size, NULL);
		hdr->size = 0;
		hdr->cookie = ACOOKIE;
	}
	hdr->capacity = new_capacity;
	hdr->data = (char*)(hdr + 1); // For debugging convenience.
	return (void*)(hdr + 1);
}

void* cf_aset(const void* a, const void* b, size_t element_size)
{
	ACANARY(a);
	ACANARY(b);
	if (acap(a) < asize(b)) {
		int len = asize(b);
		a = cf_agrow(a, asize(b), element_size);
	}
	CUTE_MEMCPY((void*)a, b, asize(b) * element_size);
	asize(a) = asize(b);
	return (void*)a;
}

char* cf_sset(char* a, const char* b)
{
	ACANARY(a);
	int bsize = (int)(b ? CUTE_STRLEN(b) : 0) + 1;
	if (!bsize) {
		sclear(a);
		return a;
	} else if (acap(a) < bsize) {
		a = (char*)cf_agrow(a, bsize, 1);
	}
	CUTE_MEMCPY((void*)a, b, bsize);
	ssize(a) = bsize;
	return a;
}

char* cf_sfmt(char* s, const char* fmt, ...)
{
	ACANARY(s);
	va_list args;
	va_start(args, fmt);
	int n = 1 + vsnprintf(s, scap(s), fmt, args);
	va_end(args);
	if (n > scap(s)) {
		sfit(s, n);
		va_start(args, fmt);
		n = 1 + vsnprintf(s, scap(s), fmt, args);
		va_end(args);
	}
	scount(s) = n;
	return s;
}

char* cf_sfmt_append(char* s, const char* fmt, ...)
{
	ACANARY(s);
	va_list args;
	va_start(args, fmt);
	int capacity = scap(s) - ssize(s);
	int n = 1 + vsnprintf(slast(s), capacity, fmt, args);
	va_end(args);
	if (n > capacity) {
		afit(s, n + slen(s));
		va_start(args, fmt);
		int new_capacity = scap(s) - ssize(s);
		n = 1 + vsnprintf(slast(s), new_capacity, fmt, args);
		CUTE_ASSERT(n <= new_capacity);
		va_end(args);
	}
	asize(s) += n - 1;
	return s;
}

bool cf_sprefix(char* s, const char* prefix)
{
	ACANARY(s);
	int prefix_len = (int)(prefix ? CUTE_STRLEN(prefix) : 0);
	bool a = slen(s) >= prefix_len;
	bool b = !CUTE_MEMCMP(s, prefix, prefix_len);
	return a && b;
}

bool cf_ssuffix(char* s, const char* suffix)
{
	ACANARY(s);
	int suffix_len = (int)(suffix ? CUTE_STRLEN(suffix) : 0);
	bool a = slen(s) >= suffix_len;
	bool b = !CUTE_MEMCMP(slast(s) - suffix_len, suffix, suffix_len);
	return a && b;
}

void cf_supper(char* s)
{
	ACANARY(s);
	if (!s) return;
	for (int i = 0; i < slen(s); ++i) {
		s[i] = toupper(s[i]);
	}
}

void cf_slower(char* s)
{
	ACANARY(s);
	if (!s) return;
	for (int i = 0; i < slen(s); ++i) {
		s[i] = tolower(s[i]);
	}
}

char* cf_strim(char* s)
{
	ACANARY(s);
	char* start = s;
	char* end = slast(s) - 1;
	while (isspace(*start)) start++;
	while (isspace(*end)) end--;
	size_t len = end - start + 1;
	CUTE_MEMMOVE(s, start, len);
	s[len] = 0;
	ssize(s) = (int)(len + 1);
	return s;
}

char* cf_sltrim(char* s)
{
	ACANARY(s);
	char* start = s;
	while (isspace(*start)) start++;
	size_t len = slen(s) - (start - s);
	CUTE_MEMMOVE(s, start, len);
	s[len] = 0;
	ssize(s) = (int)(len + 1);
	return s;
}

char* cf_srtrim(char* s)
{
	ACANARY(s);
	while (isspace(*(slast(s) - 1))) scount(s)--;
	s[slen(s)] = 0;
	return s;
}

char* cf_slpad(char* s, char pad, int count)
{
	ACANARY(s);
	int cap = scap(s) - ssize(s);
	if (cap < count) {
		sfit(s, ssize(s) + cap);
	}
	CUTE_MEMMOVE(s + count, s, ssize(s));
	CUTE_MEMSET(s, pad, count);
	ssize(s) += count;
	return s;
}

char* cf_srpad(char* s, char pad, int count)
{
	ACANARY(s);
	int cap = scap(s) - ssize(s);
	if (cap < count) {
		sfit(s, ssize(s) + cap);
	}
	CUTE_MEMSET(s + slen(s), pad, count);
	ssize(s) += count;
	s[slen(s)] = 0;
	return s;
}

char* cf_ssplit(char* s, char split_c)
{
	ACANARY(s);
	char* start = s;
	char* end = slast(s) - 1;
	while (start < end) {
		if (*start == split_c) {
			break;
		}
		++start;
	}
	int len = (int)(start - s);
	if (len + 1 == slen(s)) return NULL;
	char* split = NULL;
	sfit(split, len + 1);
	ssize(split) = len + 1;
	CUTE_MEMCPY(split, s, len);
	split[len] = 0;
	int new_len = slen(s) - len - 1;
	CUTE_MEMMOVE(s, s + len + 1, new_len);
	ssize(s) = new_len + 1;
	s[new_len] = 0;
	return split;
}

int cf_sfirst_index_of(const char* s, char c)
{
	ACANARY(s);
	if (!s) return -1;
	const char* p = CUTE_STRCHR(s, c);
	if (!p) return -1;
	return ((int)(uintptr_t)(p - s));
}

int cf_slast_index_of(const char* s, char c)
{
	ACANARY(s);
	if (!s) return -1;
	const char* p = CUTE_STRRCHR(s, c);
	if (!p) return -1;
	return ((int)(uintptr_t)(p - s));
}

char* cf_sint(int i)
{
	char* result = NULL;
	sfmt(result, "%d", i);
	return result;
}

char* cf_suint(uint64_t i)
{
	char* result = NULL;
	sfmt(result, "%" PRIu64, i);
	return result;
}

char* cf_sfloat(float f)
{
	char* result = NULL;
	sfmt(result, "%f", f);
	return result;
}

char* cf_sdouble(double d)
{
	char* result = NULL;
	sfmt(result, "%f", d);
	return result;
}

char* cf_shex(uint64_t i)
{
	char* result = NULL;
	sfmt(result, "0x%x", i);
	return result;
}

char* cf_sbool(bool b)
{
	char* result = NULL;
	sfmt(result, "%s", b ? "true" : "false");
	return result;
}

static uint64_t s_stoint(const char* s)
{
	char* end;
	uint64_t result = CUTE_STRTOLL(s, &end, 10);
	CUTE_ASSERT(end == s + CUTE_STRLEN(s));
	return result;
}

int cf_stoint(const char* s)
{
	return (int)s_stoint(s);
}

uint64_t cf_stouint(const char* s)
{
	return s_stoint(s);
}

static double s_stod(const char* s)
{
	char* end;
	double result = CUTE_STRTOD(s, &end);
	CUTE_ASSERT(end == s + CUTE_STRLEN(s));
	return result;
}

float cf_stofloat(const char* s)
{
	return (float)s_stod(s);
}

double cf_stodouble(const char* s)
{
	return s_stod(s);
}

uint64_t cf_stohex(const char* s)
{
	if (!CUTE_STRNCMP(s, "0x", 2)) s += 2;
	char* end;
	uint64_t result = CUTE_STRTOLL(s, &end, 16);
	CUTE_ASSERT(end == s + CUTE_STRLEN(s));
	return result;
}

char* cf_sreplace(char* s, const char* replace_me, const char* with_me)
{
	ACANARY(s);
	size_t replace_len = CUTE_STRLEN(replace_me);
	size_t with_len = CUTE_STRLEN(with_me);
	char* start = s;
	char* find;
	while ((find = sfind(s, replace_me))) {
		if (replace_len > with_len) {
			int remaining = ssize(s) - (int)(s - start) - (int)with_len;
			CUTE_MEMCPY(find, with_me, with_len);
			CUTE_MEMMOVE(find + with_len, find + replace_len, remaining);
			ssize(s) -= (int)(replace_len - with_len);
		} else {
			int remaining = ssize(s) - (int)(s - start) - (int)replace_len;
			int diff = (int)(with_len - replace_len);
			sfit(s, ssize(s) + diff);
			CUTE_MEMMOVE(find + with_len, find + replace_len, remaining);
			CUTE_MEMCPY(find, with_me, with_len);
			ssize(s) += diff;
		}
	}
	return s;
}

char* cf_serase(char* s, int index, int count)
{
	ACANARY(s);
	if (index < 0) {
		count += index;
		index = 0;
		if (count <= 0) return s;
	}
	if (index >= slen(s)) return s;
	if (index + count > slen(s)) {
		ssize(s) = index;
		s[slen(s)] = 0;
		return s;
	} else {
		int remaining = ssize(s) - count;
		CUTE_MEMMOVE(s + index, s + index + count, remaining);
		ssize(s) -= count;
	}
	return s;
}

struct arena_t
{
	int alignment;
	int block_size;
	char* ptr;
	char* end;
	char** blocks;
};

void* aligned_alloc(size_t size, size_t alignment)
{
	CUTE_ASSERT(alignment <= 256);
	void* p = CUTE_ALLOC(size + alignment, NULL);
	if (!p) return NULL;
	size_t offset = (size_t)p & (alignment - 1);
	p = CUTE_ALIGN_DOWN_PTR((char*)p + 1, alignment);
	CUTE_ASSERT(!(((size_t)p) & (alignment - 1)));
	*((char*)p - 1) = (char)(alignment - offset);
	return p;
}

void aligned_free(void* p)
{
	if (!p) return;
	size_t offset = (size_t)*((uint8_t*)p - 1);
	CUTE_FREE((char*)p - (offset & 0xFF), NULL);
}

void* arena_alloc(arena_t* arena, size_t size)
{
	if (size > (size_t)(arena->end - arena->ptr)) {
		arena->ptr = (char*)aligned_alloc(arena->block_size, arena->alignment);
		arena->end = arena->ptr + arena->block_size;
		apush(arena->blocks, arena->ptr);
	}
	void* result = arena->ptr;
	arena->ptr = (char*)CUTE_ALIGN_DOWN_PTR(arena->ptr + size, arena->alignment);
	CUTE_ASSERT(!(((size_t)(arena->ptr)) & (arena->alignment - 1)));
	CUTE_ASSERT(arena->ptr <= arena->end);
	return result;
}

void arena_destroy(arena_t* arena)
{
	if (arena->blocks) {
		for (int i = 0; i < alen(arena->blocks); ++i) {
			aligned_free(arena->blocks[i]);
		}
		afree(arena->blocks);
	}
}

using intern_t = cf_intern_t;

struct intern_table_t
{
	intern_t** interns;
	arena_t arena;
	rw_lock_t lock;

	CUTE_INLINE void read_lock() { cf_read_lock(&lock); }
	CUTE_INLINE void read_unlock() { cf_read_unlock(&lock); }
	CUTE_INLINE void write_lock() { cf_write_lock(&lock); }
	CUTE_INLINE void write_unlock() { cf_write_unlock(&lock); }
};

static intern_table_t* g_intern_table;

static intern_table_t* s_inst()
{
	intern_table_t* inst = (intern_table_t*)cf_atomic_ptr_get((void**)&g_intern_table);
	if (!inst) {
		inst = (intern_table_t*)CUTE_ALLOC(sizeof(intern_table_t), NULL);
		CUTE_MEMSET(inst, 0, sizeof(*inst));
		inst->interns = NULL;
		inst->arena.alignment = 8;
		inst->arena.block_size = CUTE_MB;
		inst->lock = cf_make_rw_lock();
		result_t result = cf_atomic_ptr_cas((void**)&g_intern_table, NULL, inst);
		if (is_error(result)) {
			cf_destroy_rw_lock(&inst->lock);
			CUTE_FREE(inst, NULL);
			inst = (intern_table_t*)cf_atomic_ptr_get((void**)&g_intern_table);
			CUTE_ASSERT(inst);
		}
	}
	return inst;
}

const char* cf_sintern(const char* s)
{
	intern_table_t* table = s_inst();
	intern_t* intern = NULL;
	int len = (int)CUTE_STRLEN(s);
	uint64_t hash = fnv1a(s, len);

	// Fast-path, fetch already intern'd string and return it.
	if (table->interns) {
		table->read_lock();
		intern = hget(table->interns, hash);
		table->read_unlock();
		if (intern) {
			// This loop is highly unlikely to ever actually run more than one iteration.
			intern_t* i = intern;
			while (i) {
				if (len == i->len && !CUTE_STRCMP(i->string, s)) {
					return i->string;
				}
				i = i->next;
			}
		}
	}

	// String is not yet interned, create a new allocation for it.
	intern_t* list = intern;
	table->write_lock();
	intern = (intern_t*)arena_alloc(&table->arena, sizeof(intern_t) + len + 1);
	hset(table->interns, hash, intern);
	intern->cookie = INTERN_COOKIE;
	intern->len = len;
	intern->string = (char*)(intern + 1);
	CUTE_STRCPY((char*)intern->string, s);
	intern->next = list;
	table->write_unlock();

	// Return a copy of the string as a stable pointer.
	return intern->string;
}

void cf_snuke_intern_table()
{
	intern_table_t* table = s_inst();
	table->write_lock();
	cf_atomic_ptr_set((void**)&g_intern_table, NULL);
	table->write_unlock();
	arena_destroy(&table->arena);
	hfree(table->interns);
	cf_destroy_rw_lock(&table->lock);
	CUTE_FREE(table, NULL);
}

int cf_utf8_size(int codepoint)
{
	return 0;
}

const char* cf_utf8_next(const char* s, int* codepoint)
{
	return NULL;
}

char* cf_utf8_write(char* buffer, int codepoint)
{
	return NULL;
}

const char* cf_path_get_filename(const char* path)
{
	return NULL;
}

const char* cf_path_get_ext(const char* path)
{
	return NULL;
}

const char* cf_path_append(const char* a, const char* b)
{
	return NULL;
}

const char* cf_path_pop(const char* path)
{
	return NULL;
}

const char* cf_path_compact(const char* path)
{
	return NULL;
}

const char* cf_path_name_of_folder_im_in(const char* path)
{
	return NULL;
}

//--------------------------------------------------------------------------------------------------
// Global `string` pool instance.

static void* cf_s_mem_ctx;
static bool cf_s_pool_init;
static cf_strpool_t* cf_s_pool_instance;

static cf_strpool_t* cf_s_pool(int nuke = 0)
{
	cf_strpool_t* instance = cf_s_pool_instance;
	if (nuke) {
		if (cf_s_pool_init) {
			cf_destroy_strpool(instance);
			cf_s_pool_instance = NULL;
		}
		cf_s_pool_init = 0;
		return NULL;
	} else {
		if (!cf_s_pool_init) {
			cf_s_pool_instance = instance = cf_make_strpool(NULL);
			cf_s_pool_init = 1;
		}
		return instance;
	}
}

static void cf_s_incref(cf_strpool_id id)
{
	if (id.val) {
		cf_strpool_incref(cf_s_pool(), id);
	}
}

static void cf_s_decref(cf_strpool_id id)
{
	if (id.val) {
		cf_strpool_t* pool = cf_s_pool();
		if (cf_strpool_decref(pool, id) <= 0) {
			cf_strpool_discard(pool, id);
		}
	}
}

//--------------------------------------------------------------------------------------------------

cf_string_t::cf_string_t()
	: id({ 0 })
{
}

cf_string_t::cf_string_t(char* str)
{
	int len = (int)CUTE_STRLEN(str);
	if (len) {
		id = cf_strpool_inject_len(cf_s_pool(), str, len);
		cf_s_incref(id);
	} else id.val = 0;
}

cf_string_t::cf_string_t(const char* str)
{
	int len = str ? (int)CUTE_STRLEN(str) : 0;
	if (len) {
		id = cf_strpool_inject_len(cf_s_pool(), str, len);
		cf_s_incref(id);
	} else id.val = 0;
}

cf_string_t::cf_string_t(const char* begin, const char* end)
{
	int len = begin ? (end ? (int)(end - begin) : (int)CUTE_STRLEN(begin)) : 0;
	if (len) {
		id = cf_strpool_inject_len(cf_s_pool(), begin, len);
		cf_s_incref(id);
	} else id.val = 0;
}

cf_string_t::cf_string_t(const cf_string_t& other)
	: id(other.id)
{
	cf_s_incref(id);
}

cf_string_t::cf_string_t(cf_strpool_id id)
	: id(id)
{
}

cf_string_t::~cf_string_t()
{
	cf_s_decref(id);
}

size_t cf_string_t::len() const
{
	return cf_strpool_length(cf_s_pool(), id);
}

const char* cf_string_t::c_str() const
{
	return cf_strpool_cstr(cf_s_pool(), id);
}

cf_string_t& cf_string_t::operator=(const cf_string_t& rhs)
{
	cf_s_incref(rhs.id);
	cf_s_decref(id);
	id = rhs.id;
	return *this;
}

bool cf_string_t::operator==(const cf_string_t& rhs) const
{
	return id.val == rhs.id.val;
}

bool cf_string_t::operator!=(const cf_string_t& rhs) const
{
	return id.val != rhs.id.val;
}

char cf_string_t::operator[](const int i) const
{
	CUTE_ASSERT(i >= 0 && i < len());
	return c_str()[i];
}

void cf_string_t::incref()
{
	cf_s_incref(id);
}

void cf_string_t::decref()
{
	cf_s_decref(id);
}

bool cf_string_t::is_valid() const
{
	return cf_strpool_isvalid(cf_s_pool(), id);
}

void cf_string_defrag_static_pool()
{
	cf_strpool_defrag(cf_s_pool());
}

void cf_string_nuke_static_pool()
{
	cf_s_pool(1);
}

