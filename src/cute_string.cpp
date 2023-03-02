/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#include <internal/cute_app_internal.h>

using namespace Cute;

char* cf_sset(char* a, const char* b)
{
	CF_ACANARY(a);
	if (!b) return NULL;
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
	CF_ACANARY(s);
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
	ssize(s) = n;
	return s;
}

char* cf_sfmt_append(char* s, const char* fmt, ...)
{
	CF_ACANARY(s);
	va_list args;
	va_start(args, fmt);
	int capacity = scap(s) - scount(s);
	int n = 1 + vsnprintf(s + slen(s), capacity, fmt, args);
	va_end(args);
	if (n > capacity) {
		afit(s, n + slen(s));
		va_start(args, fmt);
		int new_capacity = scap(s) - scount(s);
		n = 1 + vsnprintf(s + slen(s), new_capacity, fmt, args);
		CUTE_ASSERT(n <= new_capacity);
		va_end(args);
	}
	alen(s) += n - 1;
	return s;
}

char* cf_svfmt(char* s, const char* fmt, va_list args)
{
	CF_ACANARY(s);
	va_list copy_args;
	va_copy(copy_args, args);
	int n = 1 + vsnprintf(s, scap(s), fmt, args);
	if (n > scap(s)) {
		sfit(s, n);
		n = 1 + vsnprintf(s, scap(s), fmt, copy_args);
		va_end(copy_args);
	}
	ssize(s) = n;
	return s;
}

char* cf_svfmt_append(char* s, const char* fmt, va_list args)
{
	CF_ACANARY(s);
	va_list copy_args;
	va_copy(copy_args, args);
	int capacity = scap(s) - scount(s);
	int n = 1 + vsnprintf(s + slen(s), capacity, fmt, copy_args);
	va_end(copy_args);
	if (n > capacity) {
		afit(s, n + slen(s));
		int new_capacity = scap(s) - scount(s);
		n = 1 + vsnprintf(s + slen(s), new_capacity, fmt, args);
		CUTE_ASSERT(n <= new_capacity);
	}
	alen(s) += n - 1;
	return s;
}

bool cf_sprefix(char* s, const char* prefix)
{
	CF_ACANARY(s);
	int prefix_len = (int)(prefix ? CUTE_STRLEN(prefix) : 0);
	bool a = slen(s) >= prefix_len;
	bool b = !CUTE_MEMCMP(s, prefix, prefix_len);
	return a && b;
}

bool cf_ssuffix(char* s, const char* suffix)
{
	CF_ACANARY(s);
	int suffix_len = (int)(suffix ? CUTE_STRLEN(suffix) : 0);
	bool a = slen(s) >= suffix_len;
	bool b = !CUTE_MEMCMP(s + slen(s) - suffix_len, suffix, suffix_len);
	return a && b;
}

void cf_stoupper(char* s)
{
	CF_ACANARY(s);
	if (!s) return;
	for (int i = 0; i < slen(s); ++i) {
		s[i] = toupper(s[i]);
	}
}

void cf_stolower(char* s)
{
	CF_ACANARY(s);
	if (!s) return;
	for (int i = 0; i < slen(s); ++i) {
		s[i] = tolower(s[i]);
	}
}

char* cf_sappend(char* a, const char* b)
{
	CF_ACANARY(a);
	int blen = (int)CUTE_STRLEN(b);
	if (blen <= 0) return a;
	sfit(a, slen(a) + blen + 1);
	CUTE_MEMCPY(a + slen(a), b, blen);
	ssize(a) += blen;
	a[slen(a)] = 0;
	return a;
}

char* cf_sappend_range(char* a, const char* b, const char* b_end)
{
	CF_ACANARY(a);
	int blen = (int)(b_end - b);
	if (blen <= 0) return a;
	sfit(a, slen(a) + blen + 1);
	CUTE_MEMCPY(a + slen(a), b, blen);
	ssize(a) += blen;
	a[slen(a)] = 0;
	return a;
}

char* cf_strim(char* s)
{
	CF_ACANARY(s);
	if (slen(s) == 0) return s;
	char* start = s;
	char* end = s + slen(s) - 1;
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
	CF_ACANARY(s);
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
	CF_ACANARY(s);
	while (isspace(*(s + slen(s) - 1))) ssize(s)--;
	s[slen(s)] = 0;
	return s;
}

char* cf_slpad(char* s, char pad, int count)
{
	CF_ACANARY(s);
	int cap = scap(s) - scount(s);
	if (cap < count) {
		sfit(s, scount(s) + cap);
	}
	CUTE_MEMMOVE(s + count, s, scount(s));
	CUTE_MEMSET(s, pad, count);
	ssize(s) += count;
	return s;
}

char* cf_srpad(char* s, char pad, int count)
{
	CF_ACANARY(s);
	int cap = scap(s) - scount(s);
	if (cap < count) {
		sfit(s, scount(s) + cap);
	}
	CUTE_MEMSET(s + slen(s), pad, count);
	ssize(s) += count;
	s[slen(s)] = 0;
	return s;
}

char* cf_ssplit_once(char* s, char split_c)
{
	CF_ACANARY(s);
	char* start = s;
	char* end = s + slen(s) - 1;
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

char** cf_ssplit(const char* s, char split_c)
{
	CF_ACANARY(s);
	char* copy = NULL;
	char** result = NULL;
	char* split = NULL;
	sset(copy, s);
	while ((split = ssplit_once(copy, split_c))) {
		apush(result, split);
	}
	apush(result, copy);
	return result;
}

int cf_sfirst_index_of(const char* s, char c)
{
	if (!s) return -1;
	const char* p = CUTE_STRCHR(s, c);
	if (!p) return -1;
	return ((int)(uintptr_t)(p - s));
}

int cf_slast_index_of(const char* s, char c)
{
	if (!s) return -1;
	const char* p = CUTE_STRRCHR(s, c);
	if (!p) return -1;
	return ((int)(uintptr_t)(p - s));
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
	if (!CUTE_STRNCMP(s, "#", 1)) s += 1;
	if (!CUTE_STRNCMP(s, "0x", 2)) s += 2;
	char* end;
	uint64_t result = CUTE_STRTOLL(s, &end, 16);
	CUTE_ASSERT(end == s + CUTE_STRLEN(s));
	return result;
}

char* cf_sreplace(char* s, const char* replace_me, const char* with_me)
{
	CF_ACANARY(s);
	size_t replace_len = CUTE_STRLEN(replace_me);
	size_t with_len = CUTE_STRLEN(with_me);
	char* find;
	char* search = s;
	while ((find = sfind(search, replace_me))) {
		int find_offset = (int)(find - s);
		if (replace_len > with_len) {
			int remaining = scount(s) - find_offset - (int)with_len;
			int diff = (int)(replace_len - with_len);
			CUTE_MEMCPY(find, with_me, with_len);
			CUTE_MEMMOVE(find + with_len, find + replace_len, remaining);
			ssize(s) -= diff;
		} else {
			int remaining = scount(s) - find_offset - (int)replace_len;
			int diff = (int)(with_len - replace_len);
			sfit(s, scount(s) + diff);
			find = s + find_offset;
			CUTE_MEMMOVE(find + with_len, find + replace_len, remaining);
			CUTE_MEMCPY(find, with_me, with_len);
			ssize(s) += diff;
		}
		search = find + with_len;
	}
	return s;
}

char* cf_sdedup(char* s, int ch)
{
	CF_ACANARY(s);
	int len = (int)CUTE_STRLEN(s);
	int i = 0, j = 1;
	bool dup = false;
	while (j < len) {
		if (s[i] == ch && s[j] == ch) {
			dup = true;
			++j;
		} else {
			++i;
			if (dup) {
				s[i] = s[j];
			}
			++j;
		}
	}
	s[i + 1] = 0;
	ssize(s) = i + 1;
	return s;
}

char* cf_serase(char* s, int index, int count)
{
	CF_ACANARY(s);
	if (index < 0) {
		count += index;
		index = 0;
		if (count <= 0) return s;
	}
	if (index >= slen(s)) return s;
	if (index + count >= slen(s)) {
		ssize(s) = index + 1;
		s[index] = 0;
		return s;
	} else {
		int remaining = scount(s) - (count + index) + 1;
		CUTE_MEMMOVE(s + index, s + count + index, remaining);
		ssize(s) -= count;
	}
	return s;
}

char* cf_spop(char* s)
{
	CF_ACANARY(s);
	if (s && slen(s)) s[--ssize(s) - 1] = 0;
	return s;
}

char* cf_spopn(char* s, int n)
{
	CF_ACANARY(s);
	if (!s || n < 0) return s;
	while (ssize(s) > 1 && n--) {
		ssize(s)--;
	}
	s[slen(s)] = 0;
	return s;
}

using intern_t = cf_intern_t;

struct intern_table_t
{
	htbl intern_t** interns;
	Arena arena;
	ReadWriteLock lock;

	CUTE_INLINE void read_lock() { cf_read_lock(&lock); }
	CUTE_INLINE void read_unlock() { cf_read_unlock(&lock); }
	CUTE_INLINE void write_lock() { cf_write_lock(&lock); }
	CUTE_INLINE void write_unlock() { cf_write_unlock(&lock); }
};

static intern_table_t* g_intern_table;

static intern_table_t* s_inst()
{
	// Locklessly get/instantiate a global instance.
	intern_table_t* inst = (intern_table_t*)cf_atomic_ptr_get((void**)&g_intern_table);
	if (!inst) {
		// Create a new instance of the table.
		inst = (intern_table_t*)CUTE_ALLOC(sizeof(intern_table_t));
		CUTE_MEMSET(inst, 0, sizeof(*inst));
		inst->interns = NULL;
		inst->arena.alignment = 8;
		inst->arena.block_size = CUTE_MB;
		inst->lock = cf_make_rw_lock();

		// Try and set the global pointer. If this fails it means another thread
		// has raced us and completed first, so then just destroy ours and use theirs.
		Result result = cf_atomic_ptr_cas((void**)&g_intern_table, NULL, inst);
		if (is_error(result)) {
			cf_destroy_rw_lock(&inst->lock);
			CUTE_FREE(inst);
			inst = (intern_table_t*)cf_atomic_ptr_get((void**)&g_intern_table);
			CUTE_ASSERT(inst);
		}
	}
	return inst;
}

const char* cf_sintern(const char* s)
{
	return cf_sintern_range(s, s + CUTE_STRLEN(s));
}

const char* cf_sintern_range(const char* start, const char* end)
{
	intern_table_t* table = s_inst();
	intern_t* intern = NULL;
	int len = (int)(end - start);
	uint64_t hash = fnv1a(start, len);

	// Fast-path, fetch already intern'd strings and return them as-is.
	// Uses a mere read-lock, which is just a couple atomic read operations
	// and supports *many* simultaneous lockless readers.
	if (table->interns) {
		table->read_lock();
		intern = hget(table->interns, hash);
		table->read_unlock();
		if (intern) {
			// This loop is highly unlikely to ever actually run more than one iteration.
			intern_t* i = intern;
			while (i) {
				if (len == i->len && !CUTE_STRNCMP(i->string, start, len)) {
					return i->string;
				}
				i = i->next;
			}
		}
	}

	// String is not yet interned, create a new allocation for it.
	// We write-lock the data structure, this will wait for all readers to flush.
	CUTE_ASSERT(!intern || intern->cookie == CF_INTERN_COOKIE);
	intern_t* list = intern;
	table->write_lock();
	intern = (intern_t*)arena_alloc(&table->arena, sizeof(intern_t) + len + 1);
	hset(table->interns, hash, intern);
	intern->cookie = CF_INTERN_COOKIE;
	intern->len = len;
	intern->string = (char*)(intern + 1);
	CUTE_MEMCPY((char*)intern->string, start, len);
	((char*)intern->string)[len] = 0;
	intern->next = list;
	table->write_unlock();

	// Return a copy of the string as a stable pointer.
	return intern->string;
}

void cf_sinuke_intern_table()
{
	intern_table_t* table = s_inst();
	table->write_lock();
	cf_atomic_ptr_set((void**)&g_intern_table, NULL);
	table->write_unlock();
	arena_reset(&table->arena);
	hfree(table->interns);
	cf_destroy_rw_lock(&table->lock);
	CUTE_FREE(table);
}

// All invalid characters are encoded as the "replacement character" 0xFFFD for both
// UTF8 and UTF16 functions.

char* cf_string_append_UTF8_impl(char *s, int codepoint)
{
	CF_ACANARY(s);
	if (codepoint > 0x10FFFF) codepoint = 0xFFFD;
#define CUTE_EMIT(X, Y, Z) spush(s, X | ((codepoint >> Y) & Z))
	     if (codepoint <    0x80) { CUTE_EMIT(0x00,0,0x7F); }
	else if (codepoint <   0x800) { CUTE_EMIT(0xC0,6,0x1F); CUTE_EMIT(0x80, 0,  0x3F); }
	else if (codepoint < 0x10000) { CUTE_EMIT(0xE0,12,0xF); CUTE_EMIT(0x80, 6,  0x3F); CUTE_EMIT(0x80, 0, 0x3F); }
	else                          { CUTE_EMIT(0xF0,18,0x7); CUTE_EMIT(0x80, 12, 0x3F); CUTE_EMIT(0x80, 6, 0x3F); CUTE_EMIT(0x80, 0, 0x3F); }
	return s;
}

const char* cf_decode_UTF8(const char* s, int* codepoint)
{
	unsigned char c = *s++;
	int extra = 0;
	int min = 0;
	*codepoint = 0;
	     if (c >= 0xF0) { *codepoint = c & 0x07; extra = 3; min = 0x10000; }
	else if (c >= 0xE0) { *codepoint = c & 0x0F; extra = 2; min = 0x800; }
	else if (c >= 0xC0) { *codepoint = c & 0x1F; extra = 1; min = 0x80; }
	else if (c >= 0x80) { *codepoint = 0xFFFD; }
	else *codepoint = c;
	while (extra--) {
		c = *s++;
		if ((c & 0xC0) != 0x80) { *codepoint = 0xFFFD; break; }
		*codepoint = ((*codepoint) << 6) | (c & 0x3F);
	}
	if (*codepoint < min) *codepoint = 0xFFFD;
	return s;
}

const uint16_t* cf_decode_UTF16(const uint16_t* s, int* codepoint)
{
	int W1 = *s++;
	if (W1 < 0xD800 || W1 > 0xDFFF) {
		*codepoint = W1;
	} else if (W1 > 0xD800 && W1 < 0xDBFF) {
		int W2 = *s++;
		if (W2 > 0xDC00 && W2 < 0xDFFF) *codepoint = 0x10000 + (((W1 & 0x03FF) << 10) | W2 & 0x03FF);
		else *codepoint = 0xFFFD;
	} else *codepoint = 0xFFFD;
	return s;
}
