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
#include "cute_hashtable.h"
#include "cute_array.h"

#include <inttypes.h>
#include <stdarg.h>

//--------------------------------------------------------------------------------------------------
// C API

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
#define sfmt(s, fmt, ...) (s = cf_sfmt(s, fmt, __VA_ARGS__))
#define sfmt_append(s, fmt, ...) (s = cf_sfmt_append(s, fmt, __VA_ARGS__))
#define svfmt(s, fmt, args) (s = cf_svfmt(s, fmt, args))
#define svfmt_append(s, fmt, args) (s = cf_svfmt_append(s, fmt, args))
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
#define stoupper(s) cf_stoupper(s)
#define stolower(s) cf_stolower(s)
#define shash(s) ahash(s)
#define sappend(a, b) (a = cf_sappend(a, b))
#define sappend_range(a, b, b_end) (a = cf_sappend_range(a, b, b_end))
#define strim(s) (s = cf_strim(s))
#define sltrim(s) (s = cf_sltrim(s))
#define srtrim(s) (s = cf_srtrim(s))
#define slpad(s, ch, n) (s = cf_slpad(s, ch, n))
#define srpad(s, ch, n) (s = cf_srpad(s, ch, n))
#define ssplit_once(s, ch) cf_ssplit_once(s, ch)
#define ssplit(s, ch) cf_ssplit(s, ch)
#define sfirst_index_of(s, ch) cf_sfirst_index_of(s, ch)
#define slast_index_of(s, ch) cf_slast_index_of(s, ch)
#define sfind(s, find) CUTE_STRSTR(s, find)
#define sint(s, i) sfmt(s, "%d", i)
#define suint(s, i) sfmt(s, "%" PRIu64, i)
#define sfloat(s, f) sfmt(s, "%f", f)
#define sdouble(s, f) sfmt(s, "%f", d)
#define shex(s, uint) sfmt(s, "0x%x", uint)
#define sbool(s, b) sfmt(s, "%s", b ? "true" : "false")
#define stoint(s) cf_stoint(s)
#define stouint(s) cf_stouint(s)
#define stofloat(s) cf_stofloat(s)
#define stodouble(s) cf_stodouble(s)
#define stohex(s) cf_stohex(s)
#define stobool(s) (!CUTE_STRCMP(s, "true"))
#define sreplace(s, replace_me, with_me) (s = cf_sreplace(s, replace_me, with_me))
#define serase(s, index, count) (s = cf_serase(s, index, count))
#define sstatic(s, buffer, buffer_size) astatic(s, buffer, buffer_size())

#define INTERN_COOKIE 0x75AFC82E

typedef struct cf_intern_t
{
	uint32_t cookie; // Type check.
	int len;
	struct cf_intern_t* next;
	const char* string; // For debugging convenience but allocated after this struct.
} cf_intern_t;

#define sintern(s) cf_sintern(s)
#define sivalid(s) (((cf_intern_t*)s - 1)->cookie == INTERN_COOKIE)
#define silen(s) (((cf_intern_t*)s - 1)->len)

CUTE_API char* CUTE_CALL cf_sset(char* a, const char* b);
CUTE_API char* CUTE_CALL cf_sfmt(char* s, const char* fmt, ...);
CUTE_API char* CUTE_CALL cf_sfmt_append(char* s, const char* fmt, ...);
CUTE_API char* CUTE_CALL cf_svfmt(char* s, const char* fmt, va_list args);
CUTE_API char* CUTE_CALL cf_svfmt_append(char* s, const char* fmt, va_list args);
CUTE_API bool CUTE_CALL cf_sprefix(char* s, const char* prefix);
CUTE_API bool CUTE_CALL cf_ssuffix(char* s, const char* suffix);
CUTE_API void CUTE_CALL cf_stoupper(char* s);
CUTE_API void CUTE_CALL cf_stolower(char* s);
CUTE_API char* CUTE_CALL cf_sappend(char* a, const char* b);
CUTE_API char* CUTE_CALL cf_sappend_range(char* a, const char* b, const char* b_end);
CUTE_API char* CUTE_CALL cf_strim(char* s);
CUTE_API char* CUTE_CALL cf_sltrim(char* s);
CUTE_API char* CUTE_CALL cf_srtrim(char* s);
CUTE_API char* CUTE_CALL cf_slpad(char* s, char pad, int count);
CUTE_API char* CUTE_CALL cf_srpad(char* s, char pad, int count);
CUTE_API char* CUTE_CALL cf_ssplit_once(char* s, char split_c);
CUTE_API char** CUTE_CALL cf_ssplit(char* s, char split_c);
CUTE_API int CUTE_CALL cf_sfirst_index_of(const char* s, char c);
CUTE_API int CUTE_CALL cf_slast_index_of(const char* s, char c);
CUTE_API int CUTE_CALL cf_stoint(const char* s);
CUTE_API uint64_t CUTE_CALL cf_stouint(const char* s);
CUTE_API float CUTE_CALL cf_stofloat(const char* s);
CUTE_API double CUTE_CALL cf_stodouble(const char* s);
CUTE_API uint64_t CUTE_CALL cf_stohex(const char* s);
CUTE_API char* CUTE_CALL cf_sreplace(char* s, const char* replace_me, const char* with_me);
CUTE_API char* CUTE_CALL cf_serase(char* s, int index, int count);

CUTE_API const char* CUTE_CALL cf_sintern(const char* s);
CUTE_API void CUTE_CALL cf_snuke_intern_table();

CUTE_API int CUTE_CALL cf_utf8_size(int codepoint);
CUTE_API const char* CUTE_CALL cf_utf8_next(const char* s, int* codepoint);
CUTE_API char* CUTE_CALL cf_utf8_write(char* buffer, int codepoint);

CUTE_API const char* CUTE_CALL cf_path_get_filename(const char* path);
CUTE_API const char* CUTE_CALL cf_path_get_ext(const char* path);
CUTE_API const char* CUTE_CALL cf_path_append(const char* a, const char* b);
CUTE_API const char* CUTE_CALL cf_path_pop(const char* path);
CUTE_API const char* CUTE_CALL cf_path_compact(const char* path);
CUTE_API const char* CUTE_CALL cf_path_name_of_folder_im_in(const char* path);

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

CUTE_INLINE uint64_t constexpr fnv1a(const void* data, int size)
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

struct string_t
{
	CUTE_INLINE string_t() { astatic(m_str, u.m_buffer, sizeof(u.m_buffer)); }
	CUTE_INLINE string_t(const char* s) { astatic(m_str, u.m_buffer, sizeof(u.m_buffer)); sset(m_str, s); }
	CUTE_INLINE string_t(const char* start, const char* end) { astatic(m_str, u.m_buffer, sizeof(u.m_buffer)); int length = (int)(end - start); sfit(m_str, length); CUTE_STRNCPY(m_str, start, length); ssize(m_str) = length + 1; }
	CUTE_INLINE ~string_t() { sfree(m_str); }

	CUTE_INLINE static string_t steal_from(char* c_api_string) { ACANARY(c_api_string); string_t r; r.m_str = c_api_string; return r; }
	CUTE_INLINE static string_t from(int i) { string_t r; sint(r.m_str, i); return r; }
	CUTE_INLINE static string_t from(uint64_t uint) { string_t r; suint(r.m_str, uint); return r; }
	CUTE_INLINE static string_t from(float f) { string_t r; sfloat(r.m_str, f); return r; }
	CUTE_INLINE static string_t from(double f) { string_t r; sfloat(r.m_str, f); return r; }
	CUTE_INLINE static string_t from_hex(uint64_t uint) { string_t r; shex(r.m_str, uint); return r; }
	CUTE_INLINE static string_t from(bool b) { string_t r; sbool(r.m_str, b); return r; }

	CUTE_INLINE int to_int() const { return stoint(m_str); }
	CUTE_INLINE uint64_t to_uint() const { return stouint(m_str); }
	CUTE_INLINE float to_float() const { return stofloat(m_str); }
	CUTE_INLINE double to_double() const { return stodouble(m_str); }
	CUTE_INLINE uint64_t to_hex() const { return stohex(m_str); }
	CUTE_INLINE bool to_bool() const { return stobool(m_str); }

	CUTE_INLINE const char* c_str() const { return m_str; }
	CUTE_INLINE char* c_str() { return m_str; }
	CUTE_INLINE const char* begin() const { return m_str; }
	CUTE_INLINE char* begin() { return m_str; }
	CUTE_INLINE const char* end() const { return slast(m_str) + 1; }
	CUTE_INLINE char* end() { return slast(m_str) + 1; }
	CUTE_INLINE const char* last() const { return slast(m_str); }
	CUTE_INLINE char* last() { return slast(m_str); }
	CUTE_INLINE operator const char*() const { return m_str; }
	CUTE_INLINE operator char*() const { return m_str; }

	CUTE_INLINE char& operator[](int index) { s_chki(index); return m_str[index]; }
	CUTE_INLINE const char& operator[](int index) const { s_chki(index); return m_str[index]; }

	CUTE_INLINE int len() const { return slen(m_str); }
	CUTE_INLINE int capacity() const { return scap(m_str); }
	CUTE_INLINE int size() const { return ssize(m_str); }
	CUTE_INLINE int count() const { return ssize(m_str); }
	CUTE_INLINE void ensure_capacity(int capacity) { sfit(m_str, capacity); }
	CUTE_INLINE void fit(int capacity) { sfit(m_str, capacity); }
	CUTE_INLINE bool empty() const { return sempty(m_str); }

	CUTE_INLINE string_t& add(char ch) { spush(m_str, ch); return *this; }
	CUTE_INLINE string_t& append(const char* s) { sappend(m_str, s); return *this; }
	CUTE_INLINE string_t& append(const char* start, const char* end) { sappend_range(m_str, start, end); return *this; }
	CUTE_INLINE string_t& fmt(const char* fmt, ...) { va_list args; va_start(args, fmt); svfmt(m_str, fmt, args); va_end(args); return *this; }
	CUTE_INLINE string_t& fmt_append(const char* fmt, ...) { va_list args; va_start(args, fmt); svfmt_append(m_str, fmt, args); va_end(args); return *this; }
	CUTE_INLINE string_t& trim() { strim(m_str); return *this; }
	CUTE_INLINE string_t& ltrim() { sltrim(m_str); return *this; }
	CUTE_INLINE string_t& rtrim() { srtrim(m_str); return *this; }
	CUTE_INLINE string_t& lpad(char pad, int count) { slpad(m_str, pad, count); return *this; }
	CUTE_INLINE string_t& rpad(char pad, int count) { srpad(m_str, pad, count); return *this; }
	CUTE_INLINE string_t& set(const char* s) { sset(m_str, s); return *this; }
	CUTE_INLINE string_t& operator=(const char* s) { set(s); return *this; }
	//CUTE_INLINE array<string_t> split(char split_c) { array<string_t> r; char** s = ssplit(str, split_c); for (int i=0;i<alen(s);++i) r.add(move(steal_from(s[i]))); return r; }
	// pop
	CUTE_INLINE int first_index_of(char ch) const { sfirst_index_of(m_str, ch); }
	CUTE_INLINE int last_index_of(char ch) const { slast_index_of(m_str, ch); }
	CUTE_INLINE string_t find(const char* find_me) const { return string_t(sfind(m_str, find_me)); }
	CUTE_INLINE string_t& replace(const char* replace_me, const char* with_me) { sreplace(m_str, replace_me, with_me); return *this; }
	CUTE_INLINE string_t& erase(int index, int count) { serase(m_str, index, count); return *this; }
	CUTE_INLINE string_t dup() const { return string_t(sdup(m_str)); }
	CUTE_INLINE void clear() { sclear(m_str); }

	CUTE_INLINE bool starts_with(const char* s) const { return sprefix(m_str, s); }
	CUTE_INLINE bool ends_with(const char* s) const { return ssuffix(m_str, s); }
	CUTE_INLINE bool prefix(const char* s) const { return sprefix(m_str, s); }
	CUTE_INLINE bool suffix(const char* s) const { return ssuffix(m_str, s); }
	CUTE_INLINE bool operator==(const char* s) { return !CUTE_STRCMP(m_str, s); }
	CUTE_INLINE bool operator!=(const char* s) { return CUTE_STRCMP(m_str, s); }
	CUTE_INLINE bool compare(const char* s, bool no_case = false) { return no_case ? sequ(m_str, s) : siequ(m_str, s); }
	CUTE_INLINE bool cmp(const char* s, bool no_case = false) { compare(s, no_case); }
	CUTE_INLINE bool contains(const char* contains_me) { return scontains(m_str, contains_me); }
	CUTE_INLINE void to_upper() { stoupper(m_str); }
	CUTE_INLINE void to_lower() { stolower(m_str); }
	CUTE_INLINE uint64_t hash() const { return shash(m_str); }

private:
	char* m_str = u.m_buffer;
	union { char m_buffer[64]; void* align; } u;

	CUTE_INLINE void s_chki(int i) const { CUTE_ASSERT(i >= 0 && i < ssize(m_str)); }
};

}

#endif // CUTE_CPP

#endif // CUTE_STRING_H
