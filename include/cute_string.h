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

#ifndef CUTE_STRING_H
#define CUTE_STRING_H

#include "cute_defines.h"
#include "cute_c_runtime.h"
#include "cute_alloc.h"
#include <stdarg.h>

//--------------------------------------------------------------------------------------------------
// C API

typedef struct cf_ahdr_t
{
	int size;
	int capacity;
	char* data;
	uint32_t cookie;
} cf_ahdr_t;

#define ahdr(a) ((cf_ahdr_t*)((uintptr_t)a - sizeof(cf_ahdr_t)))
#define ACOOKIE 0xE6F7E359
#define ACANARY(a) do { if (a) CUTE_ASSERT(ahdr(a)->cookie == ACOOKIE); } while (0) // Detects buffer underruns.

#define alen(a) (ahdr(a)->size)
#define acount(a) alen(a)
#define asize(a) alen(a)
#define acap(a) ((a) ? ahdr(a)->capacity : 0)
#define afit(a, n) ((n) <= acap(a) ? 0 : (*(void**)&(a) = cf_agrow((a), (n), sizeof(*a))))
#define apush(a, ...) do { ACANARY(a); afit((a), 1 + ((a) ? alen(a) : 0)); (a)[alen(a)++] = (__VA_ARGS__); } while (0)
#define apop(a) (a[--alen(a)])
#define afree(a) do { ACANARY(a); CUTE_FREE(ahdr(a), NULL); a = NULL; } while (0)
#define aend(a) (a + alen(a))
#define aclear(a) do { ACANARY(a); if (a) alen(a) = 0; } while (0)
#define aset(a, b) (*(void**)&(a) = cf_aset((void*)(a), (void*)(b), sizeof(*a)))
#define ahash(a) cf_fnv1a(a, alen(a))

#define slen(s) ((int)((size_t)acount(s) - 1))
#define sempty(s) (s ? slen(s) < 1 : 1)
#define spush(s, ch) do { if (!s) apush(s, ch); else s[slen(s)] = ch; apush(s, 0); } while (0)
#define sfree(s) afree(s)
#define ssize(s) acount(s)
#define scount(s) acount(s)
#define scap(s) acap(s)
#define slast(s) (s + slen(s))
#define sclear(s) aclear(s)
#define sfit(s, n) afit(s, n)
#define sfmt(s, fmt, ...) ((char*)(*(void**)&(s) = (void*)cf_sfmt(s, fmt, __VA_ARGS__)))
#define sfmt_append(s, fmt, ...) ((char*)(*(void**)&(s) = (void*)cf_sfmt_append(s, fmt, __VA_ARGS__)))
#define sset(a, b) (a = cf_sset(a, b))
#define sdup(s) cf_sset(NULL, s)
#define smake(s) cf_sset(NULL, s)
#define scmp(a, b) CUTE_STRCMP(a, b)
#define sicmp(a, b) CUTE_STRICMP(a, b)
#define sequ(a, b) !CUTE_STRCMP(a, b)
#define siequ(a, b) !CUTE_STRICMP(a, b)
#define sprefix(s, prefix) cf_sprefix(s, prefix)
#define ssuffix(s, suffix) cf_ssuffix(s, suffix)
#define scontains(s, contains_me) (slen(s) >= CUTE_STRLEN(contains_me) && !!CUTE_STRSTR(s, contains_me))
#define supper(s) cf_supper(s)
#define slower(s) cf_slower(s)
#define shash(s) ahash(s)
#define sappend(a, b) (a = sfmt_append(a, "%s", b))
#define strim(s) (s = cf_strim(s))
#define sltrim(s) (s = cf_sltrim(s))
#define srtrim(s) (s = cf_srtrim(s))
#define slpad(s, ch, n) (s = cf_slpad(s, ch, n))
#define srpad(s, ch, n) (s = cf_srpad(s, ch, n))
#define ssplit(s, ch) cf_ssplit(s, ch)
#define sfirst_index_of(s, ch) cf_sfirst_index_of(s, ch)
#define slast_index_of(s, ch) cf_slast_index_of(s, ch)
#define sfind(s, find) CUTE_STRSTR(s, find)
#define sint(i) cf_sint(i)
#define suint(i) cf_suint(i)
#define sfloat(f) cf_sfloat(f)
#define sdouble(f) cf_sdouble(f)
#define shex(s) cf_shex(s)
#define sbool(s) cf_bool(s);
#define stoint(s) cf_stoint(s)
#define stouint(s) cf_stouint(s)
#define stofloat(s) cf_stofloat(s)
#define stodouble(s) cf_stodouble(s)
#define stohex(i) cf_stohex(i)
#define stobool(b) (!CUTE_STRCMP(s, "true"))
#define sintern(s) cf_sintern(s)

CUTE_API void* CUTE_CALL cf_agrow(const void* a, int new_size, size_t element_size);
CUTE_API void* CUTE_CALL cf_aset(const void* a, const void* b, size_t element_size);
CUTE_API char* CUTE_CALL cf_sset(char* a, const char* b);
CUTE_API char* CUTE_CALL cf_sfmt(char* s, const char* fmt, ...);
CUTE_API char* CUTE_CALL cf_sfmt_append(char* s, const char* fmt, ...);
CUTE_API bool CUTE_CALL cf_sprefix(char* s, const char* prefix);
CUTE_API bool CUTE_CALL cf_ssuffix(char* s, const char* suffix);
CUTE_API void CUTE_CALL cf_supper(char* s);
CUTE_API void CUTE_CALL cf_slower(char* s);
CUTE_API char* CUTE_CALL cf_strim(char* s);
CUTE_API char* CUTE_CALL cf_sltrim(char* s);
CUTE_API char* CUTE_CALL cf_srtrim(char* s);
CUTE_API char* CUTE_CALL cf_slpad(char* s, char pad, int count);
CUTE_API char* CUTE_CALL cf_srpad(char* s, char pad, int count);
CUTE_API char* CUTE_CALL cf_ssplit(char* s, char split_c);
CUTE_API int CUTE_CALL cf_sfirst_index_of(const char* s, char c);
CUTE_API int CUTE_CALL cf_slast_index_of(const char* s, char c);
CUTE_API char* CUTE_CALL cf_sint(int i);
CUTE_API char* CUTE_CALL cf_suint(uint64_t i);
CUTE_API char* CUTE_CALL cf_sfloat(float f);
CUTE_API char* CUTE_CALL cf_sdouble(double d);
CUTE_API char* CUTE_CALL cf_shex(uint64_t i);
CUTE_API int CUTE_CALL cf_stoint(const char* s);
CUTE_API uint64_t CUTE_CALL cf_stouint(const char* s);
CUTE_API float CUTE_CALL cf_stofloat(const char* s);
CUTE_API double CUTE_CALL cf_stodouble(const char* s);
CUTE_API uint64_t CUTE_CALL cf_stohex(const char* s);
CUTE_API bool CUTE_CALL cf_sbool(const char* s);

CUTE_API const char* CUTE_CALL cf_sintern(const char* s);
CUTE_API const char* CUTE_CALL cf_snuke_intern_table();

CUTE_API int CUTE_CALL cf_utf8_size(int codepoint);
CUTE_API const char* CUTE_CALL cf_utf8_next(const char* s, int* codepoint);
CUTE_API char* CUTE_CALL cf_utf8_write(char* buffer, int codepoint);

CUTE_API const char* CUTE_CALL cf_path_get_filename(const char* path);
CUTE_API const char* CUTE_CALL cf_path_get_ext(const char* path);
CUTE_API const char* CUTE_CALL cf_path_append(const char* a, const char* b);
CUTE_API const char* CUTE_CALL cf_path_pop(const char* path);
CUTE_API const char* CUTE_CALL cf_path_compact(const char* path);
CUTE_API const char* CUTE_CALL cf_path_name_of_folder_im_in(const char* path);

CUTE_INLINE uint64_t cf_fnv1a(const void* data, int size)
{
	const char* s = (const char*)data;
	uint64_t h = 14695981039346656037ULL;
	char c = 0;
	while (size--) {
		h = h ^ (uint64_t)(*s++);
		h = h * 1099511628211ULL;
	}
	return h;
}

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

#include "cute_strpool.h"

/*
	Implements a *single-threaded* string-interning system where each string on the stack
	is represented by a `uint64_t`, and internally ref-counts inside of a global string-
	interning system stored statically.
	There is no special support for string operations in a multi-threaded scenario. Simply
	make sure there is only one pool in a specific thread, if you really want to use cf_string_t
	between threads.
*/

typedef struct cf_string_t
{
	CUTE_API cf_string_t();
	CUTE_API cf_string_t(char* str);
	CUTE_API cf_string_t(const char* str);
	CUTE_API cf_string_t(const char* begin, const char* end);
	CUTE_API cf_string_t(const cf_string_t& other);
	CUTE_API cf_string_t(cf_strpool_id id);
	CUTE_API ~cf_string_t();

	CUTE_API size_t len() const;
	CUTE_API const char* c_str() const;

	CUTE_API cf_string_t& operator=(const cf_string_t& rhs);
	CUTE_API bool operator==(const cf_string_t& rhs) const;
	CUTE_API bool operator!=(const cf_string_t& rhs) const;
	CUTE_API char operator[](const int i) const;

	CUTE_API void incref();
	CUTE_API void decref();

	CUTE_API bool is_valid() const;

	cf_strpool_id id;
} cf_string_t;

namespace cute
{

CUTE_INLINE uint64_t constexpr fnv1a(void* data, int size)
{
	const char* s = (const char*)data;
	uint64_t h = 14695981039346656037ULL;
	char c = 0;
	while (size--) {
		h = h ^ (uint64_t)(*s++);
		h = h * 1099511628211ULL;
	}
	return h;
}

using string_t = cf_string_t;

}

#endif // CUTE_CPP

#endif // CUTE_STRING_H
