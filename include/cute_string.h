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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// General purpose C-string API. 100% compatible with normal C-strings.
// Automatically grows on the heap as needed.
// Free it up with `sfree` when done.
// 
// Example:
// 
//     char* s = NULL;
//     sset(s, "Hello world!");
//     printf("%s", s);
//     sfree(s);

#ifndef CUTE_NO_SHORTHAND_API
/**
 * Gets the number of characters in the string, not counting the nul-terminator. Can be NULL.
 */
#define slen(s) cf_string_len(s)

/**
 * Returns whether or not the string is empty. Both "" and NULL count as empty.
 */
#define sempty(s) cf_string_empty(s)

/**
 * Pushes character `ch` onto the end of the string (does not overwite the nul-byte).
 * If the string is empty a nul-byte is pushed afterwards.
 * Can be NULL, will create a new string and assign `s` if so.
 */
#define spush(s, ch) cf_string_push(s, ch)

/**
 * Frees up all resources used by the string and sets it to NULL.
 */
#define sfree(s) cf_string_free(s)

/**
 * Gets the number of characters in the string. Must not be NULL.
 * Returns a proper l-value, so you can assign or increment it.
 * 
 * Example:
 * 
 *     char* s = NULL;
 *     spush(s, 'a');
 *     CUTE_ASSERT(ssize(s) == 1);
 *     ssize(s)--;
 *     CUTE_ASSERT(ssize(a) == 0);
 *     sfree(ssize);
 */
#define ssize(s) cf_string_size(s)

/**
 * Gets the number of characters in the string. Cannot be NULL.
 */
#define scount(s) cf_string_count(s)

/**
 * Gets the capacity of the string. Can be NULL.
 * This is not the number of characters, but the size of the internal buffer.
 * The capacity automatically grows as necessary, but you can use `sfit` to ensure
 * a minimum capacity manually, as an optimization.
 */
#define scap(s) cf_string_cap(s)

/**
 * Returns the last character in the string. Not the nul-byte. Undefined for empty strings.
 */
#define slast(s) cf_string_last(s)

/**
 * Sets the string size to zero. Does not free up any resources.
 */
#define sclear(s) cf_string_clear(s)

/**
 * Ensures the capacity of the string is at least n elements.
 * Does not change the size/count of the string, or the len.
 * Can be NULL.
 */
#define sfit(s, n) cf_string_fit(s, n)

/**
 * Printf's into the string using the format string `fmt`.
 * The string will be overwritten from the beginning.
 * Will automatically adjust capacity as needed.
 */
#define sfmt(s, fmt, ...) cf_string_fmt(s, fmt, (__VA_ARGS__))

/**
 * Printf's into the *end* of the string, using the format string `fmt`.
 * All printed data is appended to the end of the string.
 * Will automatically adjust it's capacity as needed.
 */
#define sfmt_append(s, fmt, ...) cf_string_fmt_append(s, fmt, (__VA_ARGS__))

/**
 * Printf's into the string using the format string `fmt`.
 * The string will be overwritten from the beginning.
 * Will automatically adjust it's capacity as needed.
 * args must be a `va_list`.
 * You probably are looking for `sfmt` instead.
 */
#define svfmt(s, fmt, args) cf_string_vfmt(s, fmt, args)

/**
 * Printf's into the *end* of the string, using the format string `fmt`.
 * All printed data is appended to the end of the string.
 * Will automatically adjust it's capacity as needed.
 * args must be a `va_list`.
 * You probably are looking for `sfmt` instead.
 */
#define svfmt_append(s, fmt, args) cf_string_vfmt_append(s, fmt, args)

/**
 * Copies the string b into string a.
 * Automatically resizes as necessary.
 */
#define sset(a, b) cf_string_set(a, b)

/**
 * Returns a completely new string copy.
 * You must free the copy with `sfree` when done.
 */
#define sdup(s) cf_string_dup(s)

/**
 * Returns a completely new string copy.
 * You must free the copy with `sfree` when done.
 */
#define smake(s) cf_string_make(s)

/**
 * Returns 0 if the two strings are equivalent.
 * Otherwise returns 1 if a[i] > b[i], or -1 if a[i] < b[i].
 */
#define scmp(a, b) cf_string_cmp(a, b)

/**
 * Returns 0 if the two strings are equivalent, ignoring case.
 * Otherwise returns 1 if a[i] > b[i], or -1 if a[i] < b[i].
 * Ignores case.
 */
#define sicmp(a, b) cf_string_icmp(a, b)

/**
 * Returns false if the two strings are equivalent, true otherwise.
 */
#define sequ(a, b) cf_string_equ(a, b)

/**
 * Returns false if the two strings are equivalent, true otherwise.
 * Ignores case.
 */
#define siequ(a, b) cf_string_iequ(a, b)

/**
 * Returns true if `prefix` is the prefix of `s`, false otherwise.
 */
#define sprefix(s, prefix) cf_string_prefix(s, prefix)

/**
 * Returns true if `suffix` is the suffix of `s`, false otherwise.
 */
#define ssuffix(s, suffix) cf_string_suffix(s, suffix)

/**
 * Returns true if s contains the substring `contains_me`.
 */
#define scontains(s, contains_me) cf_string_contains(s, contains_me)

/**
 * Sets all characters in the string to upper case.
 */
#define stoupper(s) cf_string_toupper(s)

/**
 * Sets all characters in the string to lower case.
 */
#define stolower(s) cf_string_tolower(s)

/**
 * Returns a hash of the string as uint64_t.
 */
#define shash(s) cf_string_hash(s)

/**
 * Appends the string b onto the end of a.
 * You can technically do this with `sfmt`, but this function is optimized much faster.
 */
#define sappend(a, b) cf_string_append(a, b)

/**
 * Appends the string b onto the end of a.
 * You can technically do this with `sfmt`, but this function is optimized much faster.
 */
#define scat(a, b) cf_string_append(a, b)

/**
 * Appends a range of characters from string b onto the end of a.
 * You can technically do this with `sfmt`, but this function is optimized much faster.
 */
#define sappend_range(a, b, b_end) cf_string_append_range(a, b, b_end)

/**
 * Appends a range of characters from string b onto the end of a.
 * You can technically do this with `sfmt`, but this function is optimized much faster.
 */
#define scat_range(a, b, b_end) cf_string_append_range(a, b, b_end)

/**
 * Removes all whitespace from the beginning and end of the string.
 */
#define strim(s) cf_string_trim(s)

/**
 * Removes all whitespace from the beginning of the string.
 */
#define sltrim(s) cf_string_ltrim(s)

/**
 * Removes all whitespace from the end of the string.
 */
#define srtrim(s) cf_string_rtrim(s)

/**
 * Places n characters `ch` onto the front of the string.
 */
#define slpad(s, ch, n) cf_string_lpad(s, ch, n)

/**
 * Appends n characters `ch` onto the end of the string.
 */
#define srpad(s, ch, n) cf_string_rpad(s, ch, n)

/**
 * Splits a string about the character `ch` one time, scanning from left-to-right.
 * `s` will contain the string to the right of `ch`.
 * Returns the string to the left of `ch`.
 * If `ch` isn't found, simply returns NULL and does not modify `s`.
 * You must call `sfree` on the returned string.
 * 
 * This function is intended to be used in a loop, successively chopping off pieces of `s`.
 * A much easier, but slightly slower, version of this function is `ssplit`, which returns
 * an array of strings.
 */
#define ssplit_once(s, ch) cf_string_split_once(s, ch)

/**
 * Splits a string about the character `ch`, scanning from left-to-right.
 * `s` is not modified.
 * Returns an array of all delimited strings.
 * You must call `sfree` on the returned strings and `afree` on the returned array.
 * 
 *     char* s = NULL;
 *     sset(s, "split.here.in.a.loop");
 *     const char* splits_expected[] = {
 *         "split",
 *         "here",
 *         "in",
 *         "a",
 *         "loop",
 *     };
 *     char** array_of_splits = ssplit(s, '.');
 *     for (int i = 0; i < alen(array_of_splits); ++i) {
 *         const char* split = array_of_splits[i];
 *         CUTE_TEST_ASSERT(sequ(split, splits_expected[i]));
 *         sfree(split);
 *     }
 *     afree(array_of_splits);
 */
#define ssplit(s, ch) cf_string_split(s, ch)

/**
 * Scanning from left-to-right, returns the first index of `ch` found.
 * Returns -1 if none are found.
 */
#define sfirst_index_of(s, ch) cf_string_first_index_of(s, ch)

/**
 * Scanning from right-to-left, returns the first index of `ch` found.
 * Returns -1 if none are found.
 */
#define slast_index_of(s, ch) cf_string_last_index_of(s, ch)

/**
 * Scanning from left-to-right, returns a pointer to the substring `find`.
 * Returns NULL if not found.
 */
#define sfind(s, find) cf_string_find(s, find)

/**
 * Converts an int64_t to a string and assigns `s` to it.
 */
#define sint(s, i) cf_string_int(s, i)

/**
 * Converts a uint64_t to a string and assigns `s` to it.
 */
#define suint(s, uint) cf_string_uint(s, uint)

/**
 * Converts a float to a string and assigns `s` to it.
 */
#define sfloat(s, f) cf_string_float(s, f)

/**
 * Converts a double to a string and assigns `s` to it.
 */
#define sdouble(s, f) cf_string_double(s, f)

/**
 * Converts a uint64_t to a hex-string and assigns `s` to it.
 */
#define shex(s, uint) cf_string_hex(s, uint)

/**
 * Converts a string to a bool and returns it.
 */
#define sbool(s, b) cf_string_bool(s, b)

/**
 * Converts a string to an int64_t and returns it.
 */
#define stoint(s) cf_string_toint(s)

/**
 * Converts a string to an uint64_t and returns it.
 */
#define stouint(s) cf_string_touint(s)

/**
 * Converts a string to a float and returns it.
 */
#define stofloat(s) cf_string_tofloat(s)

/**
 * Converts a string to a double and returns it.
 */
#define stodouble(s) cf_string_todouble(s)

/**
 * Converts a hex-string to a uint64_t and returns it.
 * Supports srings that start with "0x", "#", or no prefix.
 */
#define stohex(s) cf_string_tohex(s)

/**
 * Converts a string to a bool and returns it.
 */
#define stobool(s) cf_string_tobool(s)

/**
 * Replaces all substrings `replace_me` with the substring `with_me`.
 */
#define sreplace(s, replace_me, with_me) cf_string_replace(s, replace_me, with_me)

/**
 * Deletes a number of characters from the string.
 */
#define serase(s, index, count) cf_string_erase(s, index, count)

/**
 * Removes a character from the end of the string.
 */
#define spop(s) (s = cf_string_pop(s))

/**
 * Removes n characters from the back of a string.
 */
#define spopn(s, n) (s = cf_string_pop_n(s, n))

/**
 * Creates a string with an initial static storage backing. Will grow onto the heap
 * if the size becomes too large.
 * 
 * s           - Your string, can be NULL. Should be char*.
 * buffer      - Pointer to a static memory buffer.
 * buffer_size - The size of `buffer` in bytes.
 */
#define sstatic(s, buffer, buffer_size) cf_string_static(s, buffer, buffer_size)

/**
 * Returns true if `s` is a dynamically alloced string from this C string API.
 * This can be evaluated at compile time for string literals.
 */
#define sisdyna(s) cf_string_is_dynamic(s)

//--------------------------------------------------------------------------------------------------
// UTF8 and UTF16.

/**
 * Encodes a UTF32 codepoint (as `uint32_t`) as UTF8. The UTF8 bytes are appended onto the string.
 * 
 * Each UTF32 codepoint represents a single character. Each character can be encoded from 1 to 4
 * bytes. Therefor, this function will push 1 to 4 bytes onto the string.
 * 
 * If an invalid codepoint is found the "replacement character" 0xFFFD will be appended instead, which
 * looks like question mark inside of a dark diamond.
 */
#define sappend_UTF8(s, codepoint) cf_string_append_UTF8(s, codepoint)

/**
 * Decodes a single UTF8 character from the string as a UTF32 codepoint.
 * 
 * The return value is not a new string, but just s + bytes, where bytes is anywhere from 1 to 4.
 * You can use this function in a loop to decode one codepoint at a time, where each codepoint
 * represents a single UTF8 character.
 * 
 *     int cp;
 *     const char* tmp = my_string;
 *     while (*tmp) {
 *         tmp = cf_decode_UTF8(tmp, &cp);
 *         DoSomethingWithCodepoint(cp);
 *     }
 */
CUTE_API const char* CUTE_CALL cf_decode_UTF8(const char* s, uint32_t* codepoint);

/**
 * Decodes a single UTF16 character from the string as a UTF32 codepoint.
 * 
 * The return value is not a new string, but just s + count, where count is anywhere from 1 to 2.
 * You can use this function in a loop to decode one codepoint at a time, where each codepoint
 * represents a single UTF8 character.
 * 
 *     int cp;
 *     const uint16_t* tmp = my_string;
 *     while (tmp) {
 *         tmp = cf_decode_UTF16(tmp, &cp);
 *         DoSomethingWithCodepoint(cp);
 *     }
 * 
 * You can convert a UTF16 string to UTF8 by calling `sappend_UTF8` on another string
 * instance inside the above example loop. Here's an example function to return a new string
 * instance in UTF8 form given a UTF16 string.
 * 
 * char* utf8(uint16_t* text)
 * {
 *     int cp;
 *     char* s = NULL;
 *     while (*text) {
 *         text = cf_decode_UTF16(text, &cp);
 *         s = sappend_UTF8(s, cp);
 *     }
 *     return s;
 * }
 */
CUTE_API const uint16_t* CUTE_CALL cf_decode_UTF16(const uint16_t* s, uint32_t* codepoint);

//--------------------------------------------------------------------------------------------------
// String Intering C API (global string table).

/**
 * Global string table.
 * Only one copy of each unique string is stored inside.
 * Use this function to get a stable pointer and unique to a string.
 * Primarily used as a memory optimization to reduce duplicate strings.
 * You *can not* modify this string in any way. It is 100% immutable.
 * You can hash returned pointers directly into hash tables (instead of hashing the entire string).
 * You can also simply compare pointers for equality, as opposed to comparing the string contents.
 * You may optionally call `sinuke` to free all resources used by the global string table.
 */
#define sintern(s) cf_sintern(s)

/**
 * Global string table.
 * Only one copy of each unique string is stored inside.
 * Use this function to get a stable pointer and unique to a string.
 * Primarily used as a memory optimization to reduce duplicate strings.
 * You *can not* modify this string in any way. It is 100% immutable.
 * You can hash returned pointers directly into hash tables (instead of hashing the entire string).
 * You can also simply compare pointers for equality, as opposed to comparing the string contents.
 * You may optionally call `sinuke` to free all resources used by the global string table.
 */
#define sintern_range(start, end) cf_sintern_range(start, end)

/**
 * Returns true if this string is a valid intern'd string (it was returned to you by `sintern`).
 * Returns false for all other strings.
 */
#define sivalid(s) (((cf_intern_t*)s - 1)->cookie == CF_INTERN_COOKIE)

/**
 * Returns the length of an intern'd string.
 */
#define silen(s) (((cf_intern_t*)s - 1)->len)

/**
 * Frees up all resources used by the global string table built by `sintern`.
 * All strings previously returned by `sintern` are now invalid.
 */
#define sinuke() cf_sinuke()
#endif // CUTE_NO_SHORTHAND_API

//--------------------------------------------------------------------------------------------------
// Longform C API.

#define cf_string_len(s) ((int)((size_t)cf_array_count(s) - 1))
#define cf_string_empty(s) (s ? cf_string_len(s) < 1 : 1)
#define cf_string_push(s, ch) do { if (!s) cf_array_push(s, ch); else s[cf_string_len(s)] = ch; cf_array_push(s, 0); } while (0)
#define cf_string_free(s) cf_array_free(s)
#define cf_string_size(s) cf_array_len(s)
#define cf_string_count(s) cf_array_count(s)
#define cf_string_cap(s) cf_array_capacity(s)
#define cf_string_last(s) (s[cf_string_len(s) - 1])
#define cf_string_clear(s) (cf_array_clear(s), cf_array_push(s, 0))
#define cf_string_fit(s, n) cf_array_fit(s, n)
#define cf_string_fmt(s, fmt, ...) (s = cf_sfmt(s, fmt, __VA_ARGS__))
#define cf_string_fmt_append(s, fmt, ...) (s = cf_sfmt_append(s, fmt, __VA_ARGS__))
#define cf_string_vfmt(s, fmt, args) (s = cf_svfmt(s, fmt, args))
#define cf_string_vfmt_append(s, fmt, args) (s = cf_svfmt_append(s, fmt, args))
#define cf_string_set(a, b) (a = cf_sset(a, b))
#define cf_string_dup(s) cf_sset(NULL, s)
#define cf_string_make(s) cf_sset(NULL, s)
#define cf_string_cmp(a, b) CUTE_STRCMP(a, b)
#define cf_string_icmp(a, b) CUTE_STRICMP(a, b)
#define cf_string_equ(a, b) ((!(a) && !(b)) || !CUTE_STRCMP(a, b))
#define cf_string_iequ(a, b) ((!(a) && !(b)) || !CUTE_STRICMP(a, b))
#define cf_string_prefix(s, prefix) cf_sprefix(s, prefix)
#define cf_string_suffix(s, suffix) cf_ssuffix(s, suffix)
#define cf_string_contains(s, contains_me) (cf_string_len(s) >= CUTE_STRLEN(contains_me) && !!CUTE_STRSTR(s, contains_me))
#define cf_string_toupper(s) cf_stoupper(s)
#define cf_string_tolower(s) cf_stolower(s)
#define cf_string_hash(s) ahash(s)
#define cf_string_append(a, b) (a = cf_sappend(a, b))
#define cf_string_append_range(a, b, b_end) (a = cf_sappend_range(a, b, b_end))
#define cf_string_trim(s) (s = cf_strim(s))
#define cf_string_ltrim(s) (s = cf_sltrim(s))
#define cf_string_rtrim(s) (s = cf_srtrim(s))
#define cf_string_lpad(s, ch, n) (s = cf_slpad(s, ch, n))
#define cf_string_rpad(s, ch, n) (s = cf_srpad(s, ch, n))
#define cf_string_split_once(s, ch) cf_ssplit_once(s, ch)
#define cf_string_split(s, ch) cf_ssplit(s, ch)
#define cf_string_first_index_of(s, ch) cf_sfirst_index_of(s, ch)
#define cf_string_last_index_of(s, ch) cf_slast_index_of(s, ch)
#define cf_string_find(s, find) CUTE_STRSTR(s, find)
#define cf_string_int(s, i) cf_string_fmt(s, "%d", i)
#define cf_string_uint(s, uint) cf_string_fmt(s, "%" PRIu64, uint)
#define cf_string_float(s, f) cf_string_fmt(s, "%f", f)
#define cf_string_double(s, f) cf_string_fmt(s, "%f", d)
#define cf_string_hex(s, uint) cf_string_fmt(s, "0x%x", uint)
#define cf_string_bool(s, b) cf_string_fmt(s, "%s", b ? "true" : "false")
#define cf_string_toint(s) cf_stoint(s)
#define cf_string_touint(s) cf_stouint(s)
#define cf_string_tofloat(s) cf_stofloat(s)
#define cf_string_todouble(s) cf_stodouble(s)
#define cf_string_tohex(s) cf_stohex(s)
#define cf_string_tobool(s) (!CUTE_STRCMP(s, "true"))
#define cf_string_replace(s, replace_me, with_me) (s = cf_sreplace(s, replace_me, with_me))
#define cf_string_erase(s, index, count) (s = cf_serase(s, index, count))
#define cf_string_pop(s) (s = cf_spop(s))
#define cf_string_pop_n(s, n) (s = cf_spopn(s, n))
#define cf_string_static(s, buffer, buffer_size) (cf_array_static(s, buffer, buffer_size), cf_array_push(s, 0))
#define cf_string_is_dynamic(s) (s && !((#s)[0] == '"') && CF_AHDR(s)->cookie == CF_ACOOKIE)
#define cf_sinuke() cf_sinuke_intern_table()
#define cf_string_append_UTF8(s, codepoint) (s = cf_string_append_UTF8_impl(s, codepoint))

//--------------------------------------------------------------------------------------------------
// Hidden API - Not intended for direct use.

#define CF_INTERN_COOKIE 0x75AFC82E // Used for sanity/type checking.

typedef struct cf_intern_t
{
	uint32_t cookie; // Type check.
	int len;
	struct cf_intern_t* next;
	const char* string; // For debugging convenience but allocated after this struct.
} cf_intern_t;

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
CUTE_API char** CUTE_CALL cf_ssplit(const char* s, char split_c);
CUTE_API int CUTE_CALL cf_sfirst_index_of(const char* s, char c);
CUTE_API int CUTE_CALL cf_slast_index_of(const char* s, char c);
CUTE_API int CUTE_CALL cf_stoint(const char* s);
CUTE_API uint64_t CUTE_CALL cf_stouint(const char* s);
CUTE_API float CUTE_CALL cf_stofloat(const char* s);
CUTE_API double CUTE_CALL cf_stodouble(const char* s);
CUTE_API uint64_t CUTE_CALL cf_stohex(const char* s);
CUTE_API char* CUTE_CALL cf_sreplace(char* s, const char* replace_me, const char* with_me);
CUTE_API char* CUTE_CALL cf_serase(char* s, int index, int count);
CUTE_API char* CUTE_CALL cf_spop(char* s);
CUTE_API char* CUTE_CALL cf_spopn(char* s, int n);
CUTE_API char* CUTE_CALL cf_string_append_UTF8_impl(char *s, uint32_t codepoint);

CUTE_API const char* CUTE_CALL cf_sintern(const char* s);
CUTE_API const char* CUTE_CALL cf_sintern_range(const char* start, const char* end);
CUTE_API void CUTE_CALL cf_sinuke_intern_table();

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
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

/**
 * General purpose string class.
 * 
 * Each string is stored in its own buffer internally.
 * The buffer starts out statically allocated, and grows onto the heap as necessary.
 */
struct String
{
	CUTE_INLINE String() { s_static(); }
	CUTE_INLINE String(const char* s) { s_static(); sset(m_str, s); }
	CUTE_INLINE String(const char* start, const char* end) { s_static(); int length = (int)(end - start); sfit(m_str, length); CUTE_STRNCPY(m_str, start, length); CF_AHDR(m_str)->size = length + 1; }
	CUTE_INLINE String(const String& s) { s_static(); sset(m_str, s); }
	CUTE_INLINE String(String&& s) { if (CF_AHDR(s.m_str)->is_static) { s_static(); sset(m_str, s); } else { m_str = s.m_str; } s.m_str = NULL; }
	CUTE_INLINE ~String() { sfree(m_str); }

	CUTE_INLINE static String steal_from(char* cute_c_api_string) { CF_ACANARY(cute_c_api_string); String r; r.m_str = cute_c_api_string; return r; }
	CUTE_INLINE static String from(int i) { String r; sint(r.m_str, i); return r; }
	CUTE_INLINE static String from(uint64_t uint) { String r; suint(r.m_str, uint); return r; }
	CUTE_INLINE static String from(float f) { String r; sfloat(r.m_str, f); return r; }
	CUTE_INLINE static String from(double f) { String r; sfloat(r.m_str, f); return r; }
	CUTE_INLINE static String from_hex(uint64_t uint) { String r; shex(r.m_str, uint); return r; }
	CUTE_INLINE static String from(bool b) { String r; sbool(r.m_str, b); return r; }

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
	CUTE_INLINE const char* end() const { return m_str + ssize(m_str); }
	CUTE_INLINE char* end() { return m_str + ssize(m_str); }
	CUTE_INLINE char last() const { return slast(m_str); }
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
	CUTE_INLINE void set_len(int len) { sfit(m_str, len + 1); ssize(m_str) = len + 1; }
	CUTE_INLINE bool empty() const { return sempty(m_str); }

	CUTE_INLINE String& add(char ch) { spush(m_str, ch); return *this; }
	CUTE_INLINE String& append(const char* s) { sappend(m_str, s); return *this; }
	CUTE_INLINE String& append(const char* start, const char* end) { sappend_range(m_str, start, end); return *this; }
	CUTE_INLINE String& append(uint32_t codepoint) { sappend_UTF8(m_str, codepoint); return *this; }
	CUTE_INLINE String& fmt(const char* fmt, ...) { va_list args; va_start(args, fmt); svfmt(m_str, fmt, args); va_end(args); return *this; }
	CUTE_INLINE String& fmt_append(const char* fmt, ...) { va_list args; va_start(args, fmt); svfmt_append(m_str, fmt, args); va_end(args); return *this; }
	CUTE_INLINE String& trim() { strim(m_str); return *this; }
	CUTE_INLINE String& ltrim() { sltrim(m_str); return *this; }
	CUTE_INLINE String& rtrim() { srtrim(m_str); return *this; }
	CUTE_INLINE String& lpad(char pad, int count) { slpad(m_str, pad, count); return *this; }
	CUTE_INLINE String& rpad(char pad, int count) { srpad(m_str, pad, count); return *this; }
	CUTE_INLINE String& set(const char* s) { sset(m_str, s); return *this; }
	CUTE_INLINE String& operator=(const char* s) { sset(m_str, s); return *this; }
	CUTE_INLINE String& operator=(const String& s) { sset(m_str, s); return *this; }
	CUTE_INLINE String& operator=(String&& s) { sset(m_str, s); return *this; }
	CUTE_INLINE Array<String> split(char split_c) { Array<String> r; char** s = ssplit(m_str, split_c); for (int i=0;i<alen(s);++i) r.add(move(steal_from(s[i]))); return r; }
	CUTE_INLINE char pop() { return apop(m_str); }
	CUTE_INLINE int first_index_of(char ch) const { return sfirst_index_of(m_str, ch); }
	CUTE_INLINE int last_index_of(char ch) const { return slast_index_of(m_str, ch); }
	CUTE_INLINE String find(const char* find_me) const { return String(sfind(m_str, find_me)); }
	CUTE_INLINE String& replace(const char* replace_me, const char* with_me) { sreplace(m_str, replace_me, with_me); return *this; }
	CUTE_INLINE String& erase(int index, int count) { serase(m_str, index, count); return *this; }
	CUTE_INLINE String dup() const { return steal_from(sdup(m_str)); }
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

	CUTE_INLINE void s_static() { sstatic(m_str, u.m_buffer, sizeof(u.m_buffer)); CUTE_ASSERT(slen(m_str) == 0); }
	CUTE_INLINE void s_chki(int i) const { CUTE_ASSERT(i >= 0 && i < ssize(m_str)); }
};

/**
 * UTF8 decoder. Load it up with a string and read `.codepoint`. Call `next` to fetch the
 * next codepoint.
 * 
 * Example:
 * 
 *     UTF8 utf8 = UTF8(my_string_in_utf8_format);
 *     while (utf8.next()) {
 *         DoSomethingWithCodepoint(utf8.codepoint);
 *     }
 */
struct UTF8
{
	CUTE_INLINE UTF8() { }
	CUTE_INLINE UTF8(const char* text) { this->text = text; }

	CUTE_INLINE bool next() { if (*text) { text = cf_decode_UTF8(text, &codepoint); return true; } else return false; }

	uint32_t codepoint;
	const char* text = NULL;
};

/**
 * UTF16 decoder. Load it up with a string and read `.codepoint`. Call `next` to fetch the
 * next codepoint.
 * 
 * Example:
 * 
 *     UTF16 utf16 = UTF16(my_string_in_utf16_format);
 *     while (utf16.next()) {
 *         DoSomethingWithCodepoint(utf16.codepoint);
 *     }
 */
struct UTF16
{
	CUTE_INLINE UTF16() { }
	CUTE_INLINE UTF16(const uint16_t* text) { this->text = text; }

	CUTE_INLINE bool next() { if (*text) { text = cf_decode_UTF16(text, &codepoint); return true; } else return false; }

	uint32_t codepoint;
	const uint16_t* text = NULL;
};

}

#endif // CUTE_CPP

#endif // CUTE_STRING_H
