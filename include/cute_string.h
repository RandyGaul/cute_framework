/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_STRING_H
#define CF_STRING_H

#include "cute_defines.h"
#include "cute_c_runtime.h"
#include "cute_alloc.h"
#include "cute_hashtable.h"
#include "cute_array.h"
#include "cute_math.h"

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

/**
 * @function slen
 * @category string
 * @brief    Returns the number of characters in the string, not counting the nul-terminator.
 * @param    s            The string. Can be `NULL`.
 * @related  slen scount scap sempty
 */
#define slen(s) cf_string_len(s)

/**
 * @function sempty
 * @category string
 * @brief    Returns whether or not the string is empty.
 * @param    s            The string. Can be `NULL`.
 * @remarks  Both "" and NULL count as empty.
 * @related  slen scount scap sempty
 */
#define sempty(s) cf_string_empty(s)

/**
 * @function spush
 * @category string
 * @brief    Pushes character `ch` onto the end of the string.
 * @param    s            The string. Can be `NULL`.
 * @param    ch           A character to push onto the end of the string.
 * @remarks  Does not overwite the nul-byte. If the string is empty a nul-byte is pushed afterwards. Can be NULL, will create a new string and assign `s` if so.
 * @related  spush spop sfit sfree sset
 */
#define spush(s, ch) cf_string_push(s, ch)

/**
 * @function sfree
 * @category string
 * @brief    Frees up all resources used by the string and sets it to `NULL`.
 * @param    s            The string. Can be `NULL`.
 * @related  spush spop sfit sfree sset
 */
#define sfree(s) cf_string_free(s)

/**
 * @function scount
 * @category string
 * @brief    Returns the number of characters in the string, including the nul-terminator.
 * @param    s            The string.
 * @related  slen scount scap sempty
 */
#define scount(s) cf_string_count(s)

/**
 * @function scap
 * @category string
 * @brief    Gets the capacity of the string.
 * @param    s            The string. Can be `NULL`.
 * @remarks  This is not the number of characters, but the size of the internal buffer. The capacity automatically grows as necessary, but
 *           you can use `sfit` to ensure a minimum capacity manually, as an optimization.
 * @related  slen scount scap sempty
 */
#define scap(s) cf_string_cap(s)

/**
 * @function sfirst
 * @category string
 * @brief    Returns the first character in the string.
 * @param    s            The string. Can be `NULL`.
 * @return   Returns '\0' if `s` is `NULL`.
 * @related  spush spop sfirst slast sclear
 */
#define sfirst(s) cf_string_first(s)

/**
 * @function slast
 * @category string
 * @brief    Returns the last character in the string. Not the nul-byte.
 * @param    s            The string. Can be `NULL`.
 * @return   Returns '\0' if `s` is `NULL`.
 * @related  spush spop sfirst slast sclear
 */
#define slast(s) cf_string_last(s)

/**
 * @function sclear
 * @category string
 * @brief    Sets the string size to zero.
 * @param    s            The string. Can be `NULL`.
 * @remarks  Does not free up any resources.
 * @related  spush spop sfirst slast sclear
 */
#define sclear(s) cf_string_clear(s)

/**
 * @function sfit
 * @category string
 * @brief    Ensures the capacity of the string is at least n+1 elements.
 * @param    s            The string. Can be `NULL`.
 * @param    n            The number of elements for the new internal capacity.
 * @remarks  Does not change the size/count of the string, or the len. This function is just here for optimization purposes.
 * @related  sfit scap sclear
 */
#define sfit(s, n) cf_string_fit(s, n)

/**
 * @function sfmt
 * @category string
 * @brief    Printf's into the string using the format string `fmt`.
 * @param    s            The string. Can be `NULL`.
 * @param    fmt          The format string.
 * @param    ...          The parameters for the format string.
 * @remarks  The string will be overwritten from the beginning. Will automatically adjust capacity as needed.
 * @related  sfmt sfmt_append svfmt svfmt_append sset
 */
#define sfmt(s, fmt, ...) cf_string_fmt(s, fmt, (__VA_ARGS__))

/**
 * @function sfmt_append
 * @category string
 * @brief    Printf's into the *end* of the string, using the format string `fmt`.
 * @param    s            The string. Can be `NULL`.
 * @param    fmt          The format string.
 * @param    ...          The parameters for the format string.
 * @remarks  All printed data is appended to the end of the string. Will automatically adjust it's capacity as needed.
 * @related  sfmt sfmt_append svfmt svfmt_append sset
 */
#define sfmt_append(s, fmt, ...) cf_string_fmt_append(s, fmt, (__VA_ARGS__))

/**
 * @function svfmt
 * @category string
 * @brief    Printf's into the string using the format string `fmt`.
 * @param    s            The string. Can be `NULL`.
 * @param    fmt          The format string.
 * @param    ...          The parameters for the format string.
 * @remarks  You probably are looking for `sfmt` instead. The string will be overwritten from the beginning. Will automatically adjust it's
 *           capacity as needed. args must be a `va_list`.
 * @related  sfmt sfmt_append svfmt svfmt_append sset
 */
#define svfmt(s, fmt, args) cf_string_vfmt(s, fmt, args)

/**
 * @function svfmt_append
 * @category string
 * @brief    Printf's into the string using the format string `fmt`.
 * @param    s            The string. Can be `NULL`.
 * @param    fmt          The format string.
 * @param    ...          The parameters for the format string.
 * @remarks  You probably are looking for `sfmt_append` instead. The string will be overwritten from the beginning. Will automatically adjust it's
 *           capacity as needed. args must be a `va_list`.
 * @related  sfmt sfmt_append svfmt svfmt_append sset
 */
#define svfmt_append(s, fmt, args) cf_string_vfmt_append(s, fmt, args)

/**
 * @function sset
 * @category string
 * @brief    Copies the string `b` into string `a`.
 * @param    a            Destination for copying. Can be `NULL`.
 * @param    b            Source for copying.
 * @related  sfmt sfmt_append svfmt svfmt_append sset sdup smake
 */
#define sset(a, b) cf_string_set(a, b)

/**
 * @function sdup
 * @category string
 * @brief    Returns a completely new string copy.
 * @param    s            The string to duplicate.
 * @remarks  You must free the copy with `sfree` when done. Does the same thing as `smake`.
 * @related  sset sdup smake
 */
#define sdup(s) cf_string_dup(s)

/**
 * @function smake
 * @category string
 * @brief    Returns a completely new string copy.
 * @param    s            The string to duplicate.
 * @param    b            Source for copying.
 * @remarks  You must free the copy with `sfree` when done. Does the same thing as `sdup`.
 * @related  sset sdup smake
 */
#define smake(s) cf_string_make(s)

/**
 * @function scmp
 * @category string
 * @brief    Compares two strings.
 * @param    a            The first string.
 * @param    b            The second string.
 * @remarks  Returns 0 if the two strings are equivalent. Otherwise returns 1 if a[i] > b[i], or -1 if a[i] < b[i].
 * @related  scmp sicmp sequ siequ
 */
#define scmp(a, b) cf_string_cmp(a, b)

/**
 * @function sicmp
 * @category string
 * @brief    Compares two strings, ignoring case.
 * @param    a            The first string.
 * @param    b            The second string.
 * @remarks  Returns 0 if the two strings are equivalent. Otherwise returns 1 if a[i] > b[i], or -1 if a[i] < b[i].
 * @related  scmp sicmp sequ siequ
 */
#define sicmp(a, b) cf_string_icmp(a, b)

/**
 * @function sequ
 * @category string
 * @brief    Returns true if the two strings are equivalent, false otherwise.
 * @param    a            The first string.
 * @param    b            The second string.
 * @related  scmp sicmp sequ siequ
 */
#define sequ(a, b) cf_string_equ(a, b)

/**
 * @function siequ
 * @category string
 * @brief    Returns true if the two strings are equivalent, ignoring case, false otherwise.
 * @param    a            The first string.
 * @param    b            The second string.
 * @related  scmp sicmp sequ siequ
 */
#define siequ(a, b) cf_string_iequ(a, b)

/**
 * @function sprefix
 * @category string
 * @brief    Check to see if the string's prefix matches.
 * @param    s            The string. Can be `NULL`.
 * @param    prefix       A string to compare against the beginning of `s`.
 * @return   Returns true if `prefix` is the prefix of `s`, false otherwise.
 * @related  sprefix ssuffix scontains sfirst_index_of slast_index_of sfind
 */
#define sprefix(s, prefix) cf_string_prefix(s, prefix)

/**
 * @function ssuffix
 * @category string
 * @brief    Check to see if the string's suffix matches.
 * @param    s            The string. Can be `NULL`.
 * @param    prefix       A string to compare against the end of `s`.
 * @return   Returns true if `suffix` is the suffix of `s`, false otherwise.
 * @related  sprefix ssuffix scontains sfirst_index_of slast_index_of sfind
 */
#define ssuffix(s, suffix) cf_string_suffix(s, suffix)

/**
 * @function scontains
 * @category string
 * @brief    Returns true if s contains a certain substring.
 * @param    s            The string. Can be `NULL`.
 * @param    contains_me  A substring to search for.
 * @related  sprefix ssuffix scontains sfirst_index_of slast_index_of sfind
 */
#define scontains(s, contains_me) cf_string_contains(s, contains_me)

/**
 * @function stoupper
 * @category string
 * @brief    Sets all characters in the string to upper case.
 * @param    s            The string. Can be `NULL`.
 * @related  stoupper stolower siequ sicmp
 */
#define stoupper(s) cf_string_toupper(s)

/**
 * @function stolower
 * @category string
 * @brief    Sets all characters in the string to lower case.
 * @param    s            The string. Can be `NULL`.
 * @related  stoupper stolower siequ sicmp
 */
#define stolower(s) cf_string_tolower(s)

/**
 * @function shash
 * @category string
 * @brief    Returns a hash of the string as `uint64_t`.
 * @param    s            The string.
 */
#define shash(s) cf_string_hash(s)

/**
 * @function sappend
 * @category string
 * @brief    Appends the string b onto the end of a.
 * @param    a            The string to modify. Can be `NULL`.
 * @param    b            Used to append onto `a`.
 * @remarks  You can technically do this with `sfmt`, but this function is optimized much faster. Does the same thing as `scat`.
 * @related  sappend scat sappend_range scat_range sfmt sfmt_append
 */
#define sappend(a, b) cf_string_append(a, b)

/**
 * @function scat
 * @category string
 * @brief    Appends the string b onto the end of a.
 * @param    a            The string to modify. Can be `NULL`.
 * @param    b            Used to append onto `a`.
 * @remarks  You can technically do this with `sfmt`, but this function is optimized much faster. Does the same thing as `sappend`.
 * @related  sappend scat sappend_range scat_range sfmt sfmt_append
 */
#define scat(a, b) cf_string_append(a, b)

/**
 * @function sappend_range
 * @category string
 * @brief    Appends a range of characters from string b onto the end of a.
 * @param    a            The string to modify. Can be `NULL`.
 * @param    b            Used to append onto `a`.
 * @remarks  You can technically do this with `sfmt`, but this function is optimized much faster. Does the same thing as `scat_range`.
 * @related  sappend scat sappend_range scat_range sfmt sfmt_append
 */
#define sappend_range(a, b, b_end) cf_string_append_range(a, b, b_end)

/**
 * @function scat_range
 * @category string
 * @brief    Appends a range of characters from string b onto the end of a.
 * @param    a            The string to modify. Can be `NULL`.
 * @param    b            Used to append onto `a`.
 * @remarks  You can technically do this with `sfmt`, but this function is optimized much faster. Does the same thing as `sappend_range`.
 * @related  sappend scat sappend_range scat_range sfmt sfmt_append
 */
#define scat_range(a, b, b_end) cf_string_append_range(a, b, b_end)

/**
 * @function strim
 * @category string
 * @brief    Removes all whitespace from the beginning and end of the string.
 * @param    s            The string.
 * @related  strim sltrim srtrim slpad srpad sdedup sreplace serase
 */
#define strim(s) cf_string_trim(s)

/**
 * @function sltrim
 * @category string
 * @brief    Removes all whitespace from the beginning of the string.
 * @param    s            The string.
 * @related  strim sltrim srtrim slpad srpad sdedup sreplace serase
 */
#define sltrim(s) cf_string_ltrim(s)

/**
 * @function srtrim
 * @category string
 * @brief    Removes all whitespace from the end of the string.
 * @param    s            The string.
 * @related  strim sltrim srtrim slpad srpad sdedup sreplace serase
 */
#define srtrim(s) cf_string_rtrim(s)

/**
 * @function slpad
 * @category string
 * @brief    Places n characters `ch` onto the front of the string.
 * @param    s            The string. Can be `NULL`.
 * @param    ch           A character to insert.
 * @param    n            Number of times to insert `ch`.
 * @related  strim sltrim srtrim slpad srpad sdedup sreplace serase
 */
#define slpad(s, ch, n) cf_string_lpad(s, ch, n)

/**
 * @function srpad
 * @category string
 * @brief    Appends n characters `ch` onto the end of the string.
 * @param    s            The string. Can be `NULL`.
 * @param    ch           A character to insert.
 * @param    n            Number of times to insert `ch`.
 * @related  strim sltrim srtrim slpad srpad sdedup sreplace serase
 */
#define srpad(s, ch, n) cf_string_rpad(s, ch, n)

/**
 * @function ssplit_once
 * @category string
 * @brief    Splits a string about the character `ch` one time, scanning from left-to-right.
 * @param    s            The string. Must be a dynamically allocated string from this string API. Does *not* work on string literals.
 * @param    ch           A character to split about.
 * @return   Returns the string to the left of `ch`.
 * @remarks  `s` will contain the string to the right of `ch`.
 *           Returns the string to the left of `ch`.
 *           If `ch` isn't found, simply returns `NULL` and does not modify `s`.
 *           You must call `sfree` on the returned string.
 *           
 *           This function is intended to be used in a loop, successively chopping off pieces of `s`.
 *           A much easier, but slightly slower, version of this function is `ssplit`, which returns
 *           an array of strings.
 * @related  ssplit_once ssplit
 */
#define ssplit_once(s, ch) cf_string_split_once(s, ch)

/**
 * @function ssplit
 * @category string
 * @brief    Splits a string about the character `ch`, scanning from left-to-right.
 * @param    s            The string.
 * @param    ch           A character to split about.
 * @return   Returns a dynamic array of all delimited strings (see `dyna`).
 * @example > Splitting a string about '.'.
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
 *         CF_TEST_ASSERT(sequ(split, splits_expected[i]));
 *         sfree(split);
 *     }
 *     afree(array_of_splits);
 * @remarks  `s` is not modified. You must call `sfree` on the returned strings and `afree` on the returned array.
 * @related  ssplit_once ssplit
 */
#define ssplit(s, ch) cf_string_split(s, ch)

/**
 * @function sfirst_index_of
 * @category string
 * @brief    Scanning from left-to-right, returns the first index of `ch` found.
 * @param    s            The string. Can be `NULL`.
 * @param    ch           A character to search for.
 * @return   Returns -1 if none are found.
 * @related  sfirst_index_of slast_index_of sfind
 */
#define sfirst_index_of(s, ch) cf_string_first_index_of(s, ch)

/**
 * @function slast_index_of
 * @category string
 * @brief    Scanning from right-to-left, returns the first index of `ch` found.
 * @param    s            The string. Can be `NULL`.
 * @param    ch           A character to search for.
 * @return   Returns -1 if none are found.
 * @related  sfirst_index_of slast_index_of sfind
 */
#define slast_index_of(s, ch) cf_string_last_index_of(s, ch)

/**
 * @function sfind
 * @category string
 * @brief    Scanning from left-to-right, returns a pointer to the substring `find`.
 * @param    s            The string.
 * @param    find         A substring to search for.
 * @return   Returns `NULL` if not found.
 * @related  sfirst_index_of slast_index_of sfind
 */
#define sfind(s, find) cf_string_find(s, find)

/**
 * @function sint
 * @category string
 * @brief    Converts an int64_t to a string and assigns `s` to it.
 * @param    s            The string.
 * @param    i            The value to convert.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define sint(s, i) cf_string_int(s, i)

/**
 * @function suint
 * @category string
 * @brief    Converts a uint64_t to a string and assigns `s` to it.
 * @param    s            The string.
 * @param    uint         The value to convert.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define suint(s, uint) cf_string_uint(s, uint)

/**
 * @function sfloat
 * @category string
 * @brief    Converts a float to a string and assigns `s` to it.
 * @param    s            The string.
 * @param    f            The value to convert.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define sfloat(s, f) cf_string_float(s, f)

/**
 * @function sdouble
 * @category string
 * @brief    Converts a double to a string and assigns `s` to it.
 * @param    s            The string.
 * @param    f            The value to convert.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define sdouble(s, f) cf_string_double(s, f)

/**
 * @function shex
 * @category string
 * @brief    Converts a uint64_t to a hex-string and assigns `s` to it.
 * @param    s            The string.
 * @param    uint         The value to convert.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define shex(s, uint) cf_string_hex(s, uint)

/**
 * @function sbool
 * @category string
 * @brief    Converts a bool to a string and assigns `s` to it.
 * @param    s            The string.
 * @param    uint         The value to convert.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define sbool(s, b) cf_string_bool(s, b)

/**
 * @function stoint
 * @category string
 * @brief    Converts a string to an int64_t and returns it.
 * @param    s            The string.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define stoint(s) cf_string_toint(s)

/**
 * @function stouint
 * @category string
 * @brief    Converts a string to an uint64_t and returns it.
 * @param    s            The string.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define stouint(s) cf_string_touint(s)

/**
 * @function stofloat
 * @category string
 * @brief    Converts a string to a float and returns it.
 * @param    s            The string.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define stofloat(s) cf_string_tofloat(s)

/**
 * @function stodouble
 * @category string
 * @brief    Converts a string to a double and returns it.
 * @param    s            The string.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define stodouble(s) cf_string_todouble(s)

/**
 * @function stohex
 * @category string
 * @brief    Converts a hex-string to a uint64_t and returns it.
 * @param    s            The string.
 * @remarks  Supports srings that start with "0x", "#", or no prefix.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define stohex(s) cf_string_tohex(s)

/**
 * @function stobool
 * @category string
 * @brief    Converts a string to a bool and returns it.
 * @param    s            The string.
 * @remarks  Supports srings that start with "0x", "#", or no prefix.
 * @related  sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define stobool(s) cf_string_tobool(s)

/**
 * @function sreplace
 * @category string
 * @brief    Replaces all substrings `replace_me` with the substring `with_me`.
 * @param    s            The string. Can be `NULL`.
 * @param    replace_me   Substring to replace.
 * @param    with_me      The replacement string.
 * @remarks  Supports srings that start with "0x", "#", or no prefix.
 * @related  strim sltrim srtrim slpad srpad sdedup sreplace serase
 */
#define sreplace(s, replace_me, with_me) cf_string_replace(s, replace_me, with_me)

/**
 * @function sdedup
 * @category string
 * @brief    Removes all consecutive occurances of `ch` from the string.
 * @param    s            The string. Can be `NULL`.
 * @param    ch           A character.
 * @related  strim sltrim srtrim slpad srpad sdedup sreplace serase
 */
#define sdedup(s, ch) cf_string_dedup(s, ch)

/**
 * @function serase
 * @category string
 * @brief    Deletes a number of characters from the string.
 * @param    s            The string. Can be `NULL`.
 * @param    index        Index in the string to start deleting from.
 * @param    count        Number of character to delete.
 * @related  strim sltrim srtrim slpad srpad sdedup sreplace serase
 */
#define serase(s, index, count) cf_string_erase(s, index, count)

/**
 * @function spop
 * @category string
 * @brief    Removes a character from the end of the string.
 * @param    s            The string. Can be `NULL`.
 * @related  spop spopn serase slast
 */
#define spop(s) (s = cf_string_pop(s))

/**
 * @function spopn
 * @category string
 * @brief    Removes n characters from the back of a string.
 * @param    s            The string. Can be `NULL`.
 * @param    n            Number of characters to pop.
 * @related  spop spopn serase slast
 */
#define spopn(s, n) (s = cf_string_pop_n(s, n))

/**
 * @function sstatic
 * @category string
 * @brief    Creates a string with an initial static storage backing.
 * @param    s            The string. Can be `NULL`.
 * @param    buffer       Pointer to a static memory buffer.
 * @param    buffer_size  The size of `buffer` in bytes.
 * @remarks  Will grow onto the heap if the size becomes too large. Call `sfree` when done.
 * @related  sstatic sisdyna spush sset
 */
#define sstatic(s, buffer, buffer_size) cf_string_static(s, buffer, buffer_size)

/**
 * @function sisdyna
 * @category string
 * @brief    Checks to see if a C string is a dynamic string from Cute Framework's string API, or not.
 * @param    s            The string. Can be `NULL`.
 * @return   Returns true if `s` is a dynamically alloced string from this C string API.
 * @remarks  This can be evaluated at compile time for string literals.
 * @related  sstatic sisdyna spush sset
 */
#define sisdyna(s) cf_string_is_dynamic(s)

//--------------------------------------------------------------------------------------------------
// UTF8 and UTF16.

/**
 * @function sappend_UTF8
 * @category string
 * @brief    Appends a UTF32 codepoint (as `uint32_t`) encoded as UTF8 onto the string.
 * @param    s            The string. Can be `NULL`.
 * @param    codepoint    An `int` codepoint in UTF32 form.
 * @example > Example of suggested way to use this function within a loop.
 *     char* s = NULL;
 *     while (has_codepoint()) {
 *         sappend_UTF8(s, get_codepoint());
 *     }
 *     sfree(s);
 * @remarks  The UTF8 bytes are appended onto the string.
 *           
 *           Each UTF32 codepoint represents a single character. Each character can be encoded from 1 to 4
 *           bytes. Therefor, this function will push 1 to 4 bytes onto the string.
 *           
 *           If an invalid codepoint is found the "replacement character" 0xFFFD will be appended instead, which
 *           looks like question mark inside of a dark diamond.
 * @related  sappend_UTF8 cf_decode_UTF8 cf_decode_UTF16
 */
#define sappend_UTF8(s, codepoint) cf_string_append_UTF8(s, codepoint)

/**
 * @function cf_decode_UTF8
 * @category string
 * @brief    Decodes a single UTF8 character from the string as a UTF32 codepoint.
 * @param    s            The string. Can be `NULL`.
 * @param    codepoint    An `int` codepoint in UTF32 form.
 * @return   The return value is not a new string, but just s + bytes, where bytes is anywhere from 1 to 4.
 * @example > Decoding a UTF8 string one codepoint at a time.
 *     int cp;
 *     const char* tmp = my_string;
 *     while (*tmp) {
 *         tmp = cf_decode_UTF8(tmp, &cp);
 *         DoSomethingWithCodepoint(cp);
 *     }
 * @remarks  You can use this function in a loop to decode one codepoint at a time, where each codepoint
 *           represents a single UTF8 character. If the decoded codepoint is invalid then the "replacement character"
 *           0xFFFD will be recorded instead.
 * @related  sappend_UTF8 cf_decode_UTF8 cf_decode_UTF16
 */
CF_API const char* CF_CALL cf_decode_UTF8(const char* s, int* codepoint);

/**
 * @function cf_decode_UTF16
 * @category string
 * @brief    Decodes a single UTF16 character from the string as a UTF32 codepoint.
 * @param    s            The string. Can be `NULL`.
 * @param    codepoint    An `int` codepoint in UTF32 form.
 * @return   The return value is not a new string, but just s + count, where count is anywhere from 1 to 2.
 * @remarks  You can use this function in a loop to decode one codepoint at a time, where each codepoint
 *           represents a single UTF8 character.
 *           
 *           ```cpp
 *           int cp;
 *           const uint16_t* tmp = my_string;
 *           while (tmp) {
 *               tmp = cf_decode_UTF16(tmp, &cp);
 *               DoSomethingWithCodepoint(cp);
 *           }
 *           ```
 *           
 *           You can convert a UTF16 string to UTF8 by calling `sappend_UTF8` on another string
 *           instance inside the above example loop. Here's an example function to return a new string
 *           instance in UTF8 form given a UTF16 string.
 *           
 *           ```cpp
 *           char* utf8(uint16_t* text)
 *           {
 *           int cp;
 *           char* s = NULL;
 *           while (*text) {
 *               text = cf_decode_UTF16(text, &cp);
 *               s = sappend_UTF8(s, cp);
 *           }
 *           return s;
 *           }
 *           ```
 * @related  sappend_UTF8 cf_decode_UTF8 cf_decode_UTF16
 */
CF_API const uint16_t* CF_CALL cf_decode_UTF16(const uint16_t* s, int* codepoint);

//--------------------------------------------------------------------------------------------------
// String Intering C API (global string table).
// ^      ^

/**
 * @function sintern
 * @category string
 * @brief    Stores unique, static copy of a string in a global string interning table.
 * @param    s            The string to insert into the global table.
 * @return   Returns a static, unique, stable, read-only copy of the string. The pointer is stable until `sinuke` is called.
 * @remarks  Only one copy of each unique string is stored. The purpose is primarily a memory optimization to reduce duplicate strings.
 *           You *can not* modify this string in any way. It is 100% immutable. Some major benefits come from placing strings into this
 *           table.
 *           
 *           - You can hash returned pointers directly into hash tables (instead of hashing the entire string).
 *           - You can simply compare pointers for equality, as opposed to comparing the string contents, as long as both strings came from this function.
 *           - You may optionally call `sinuke` to free all resources used by the global string table.
 *           - This function is very fast if the string was already stored previously.
 * @related  sintern sintern_range sivalid silen sinuke
 */
#define sintern(s) cf_sintern(s)

/**
 * @function sintern_range
 * @category string
 * @brief    Stores unique, static copy of a string in a global string interning table.
 * @param    start         A pointer to the start of the string to insert into the global table.
 * @param    end           A pointer to the end of the string to insert into the global table. Should point just before the nul-byte (if there is a nul-byte).
 * @return   Returns a static, unique, stable, read-only copy of the string. The pointer is stable until `sinuke` is called.
 * @remarks  Only one copy of each unique string is stored. The purpose is primarily a memory optimization to reduce duplicate strings.
 *           You *can not* modify this string in any way. It is 100% immutable. Some major benefits come from placing strings into this
 *           table.
 *           
 *           - You can hash returned pointers directly into hash tables (instead of hashing the entire string).
 *           - You can simply compare pointers for equality, as opposed to comparing the string contents, as long as both strings came from this function.
 *           - You may optionally call `sinuke` to free all resources used by the global string table.
 *           - This function is very fast if the string was already stored previously.
 * @related  sintern sintern_range sivalid silen sinuke
 */
#define sintern_range(start, end) cf_sintern_range(start, end)

/**
 * @function sivalid
 * @category string
 * @brief    Returns true if the string is a static, stable, unique pointer from `sintern`.
 * @param    s            The string.
 * @remarks  This is *not* a secure method -- do not use it on any potentially dangerous strings. It's designed to be very simple and fast, nothing more.
 * @related  sintern sintern_range sivalid silen sinuke
 */
#define sivalid(s) (((cf_intern_t*)s - 1)->cookie == CF_INTERN_COOKIE)

/**
 * @function silen
 * @category string
 * @brief    Returns the length of an intern'd string.
 * @param    s            The string.
 * @remarks  This is *not* a secure method -- do not use it on any potentially dangerous strings. It's designed to be very simple and fast, nothing more.
 *           The return value is calculated in constant time, as opposed to calling `CF_STRLEN` (`strlen`).
 * @related  sintern sintern_range sivalid silen sinuke
 */
#define silen(s) (((cf_intern_t*)s - 1)->len)

/**
 * @function sinuke
 * @category string
 * @brief    Frees up all resources used by the global string table built by `sintern`.
 * @remarks  All strings previously returned by `sintern` are now invalid.
 * @related  sintern sintern_range sivalid silen sinuke
 */
#define sinuke() cf_sinuke()

//--------------------------------------------------------------------------------------------------
// Longform C API.

#define cf_string_len(s) (s ? (int)((size_t)cf_array_count(s) - 1) : 0)
#define cf_string_empty(s) (s ? cf_string_len(s) < 1 : 1)
#define cf_string_push(s, ch) do { if (!s) cf_array_push(s, ch); else s[cf_string_len(s)] = ch; cf_array_push(s, 0); } while (0)
#define cf_string_free(s) cf_array_free(s)
#define cf_string_size(s) cf_array_len(s)
#define cf_string_count(s) cf_array_count(s)
#define cf_string_cap(s) cf_array_capacity(s)
#define cf_string_first(s) (s ? s[0] : '\0')
#define cf_string_last(s) (s ? s[cf_string_len(s) - 1] : '\0')
#define cf_string_clear(s) (cf_array_clear(s), cf_array_push(s, 0))
#define cf_string_fit(s, n) (s = cf_sfit(s, n))
#define cf_string_fmt(s, fmt, ...) (s = cf_sfmt(s, fmt, __VA_ARGS__))
#define cf_string_fmt_append(s, fmt, ...) (s = cf_sfmt_append(s, fmt, __VA_ARGS__))
#define cf_string_vfmt(s, fmt, args) (s = cf_svfmt(s, fmt, args))
#define cf_string_vfmt_append(s, fmt, args) (s = cf_svfmt_append(s, fmt, args))
#define cf_string_set(a, b) (a = cf_sset(a, b))
#define cf_string_set(a, b) (a = cf_sset(a, b))
#define cf_string_dup(s) cf_sset(NULL, s)
#define cf_string_make(s) cf_sset(NULL, s)
#define cf_string_cmp(a, b) CF_STRCMP(a, b)
#define cf_string_icmp(a, b) CF_STRICMP(a, b)
#define cf_string_equ(a, b) ((!(a) && !(b)) || !CF_STRCMP(a, b))
#define cf_string_iequ(a, b) ((!(a) && !(b)) || !CF_STRICMP(a, b))
#define cf_string_prefix(s, prefix) cf_sprefix(s, prefix)
#define cf_string_suffix(s, suffix) cf_ssuffix(s, suffix)
#define cf_string_contains(s, contains_me) (cf_string_len(s) >= CF_STRLEN(contains_me) && !!CF_STRSTR(s, contains_me))
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
#define cf_string_find(s, find) CF_STRSTR(s, find)
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
#define cf_string_tobool(s) (!CF_STRCMP(s, "true"))
#define cf_string_replace(s, replace_me, with_me) (s = cf_sreplace(s, replace_me, with_me))
#define cf_string_dedup(s, ch) (s = cf_sdedup(s, ch))
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

CF_API char* CF_CALL cf_sfit(char* a, int n);
CF_API char* CF_CALL cf_sset(char* a, const char* b);
CF_API char* CF_CALL cf_sfmt(char* s, const char* fmt, ...);
CF_API char* CF_CALL cf_sfmt_append(char* s, const char* fmt, ...);
CF_API char* CF_CALL cf_svfmt(char* s, const char* fmt, va_list args);
CF_API char* CF_CALL cf_svfmt_append(char* s, const char* fmt, va_list args);
CF_API bool CF_CALL cf_sprefix(char* s, const char* prefix);
CF_API bool CF_CALL cf_ssuffix(char* s, const char* suffix);
CF_API void CF_CALL cf_stoupper(char* s);
CF_API void CF_CALL cf_stolower(char* s);
CF_API char* CF_CALL cf_sappend(char* a, const char* b);
CF_API char* CF_CALL cf_sappend_range(char* a, const char* b, const char* b_end);
CF_API char* CF_CALL cf_strim(char* s);
CF_API char* CF_CALL cf_sltrim(char* s);
CF_API char* CF_CALL cf_srtrim(char* s);
CF_API char* CF_CALL cf_slpad(char* s, char pad, int count);
CF_API char* CF_CALL cf_srpad(char* s, char pad, int count);
CF_API char* CF_CALL cf_ssplit_once(char* s, char split_c);
CF_API char** CF_CALL cf_ssplit(const char* s, char split_c);
CF_API int CF_CALL cf_sfirst_index_of(const char* s, char c);
CF_API int CF_CALL cf_slast_index_of(const char* s, char c);
CF_API int CF_CALL cf_stoint(const char* s);
CF_API uint64_t CF_CALL cf_stouint(const char* s);
CF_API float CF_CALL cf_stofloat(const char* s);
CF_API double CF_CALL cf_stodouble(const char* s);
CF_API uint64_t CF_CALL cf_stohex(const char* s);
CF_API char* CF_CALL cf_sreplace(char* s, const char* replace_me, const char* with_me);
CF_API char* CF_CALL cf_sdedup(char* s, int ch);
CF_API char* CF_CALL cf_serase(char* s, int index, int count);
CF_API char* CF_CALL cf_spop(char* s);
CF_API char* CF_CALL cf_spopn(char* s, int n);
CF_API char* CF_CALL cf_string_append_UTF8_impl(char *s, int codepoint);

CF_API const char* CF_CALL cf_sintern(const char* s);
CF_API const char* CF_CALL cf_sintern_range(const char* start, const char* end);
CF_API void CF_CALL cf_sinuke_intern_table();

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

CF_INLINE uint64_t constexpr fnv1a(const void* data, int size)
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
 * Each string is stored in its own buffer internally.
 * 8 byte stack size.
 */
struct String
{
	CF_INLINE String() { }
	CF_INLINE String(const char* s) { sset(m_str, s); }
	CF_INLINE String(const char* start, const char* end) { int length = (int)(end - start); sfit(m_str, length); CF_STRNCPY(m_str, start, length); CF_AHDR(m_str)->size = length + 1; }
	CF_INLINE String(const String& s) { sset(m_str, s); }
	CF_INLINE String(String&& s) {  m_str = s.m_str; s.m_str = NULL; }
	CF_INLINE String(int i) { sint(m_str, i); }
	CF_INLINE String(uint32_t i) { suint(m_str, i); }
	CF_INLINE String(int64_t uint) { sint(m_str, uint); }
	CF_INLINE String(uint64_t uint) { suint(m_str, uint); }
	CF_INLINE String(float f) { sfloat(m_str, f); }
	CF_INLINE String(double f) { sfloat(m_str, f); }
	CF_INLINE String(bool b) { sbool(m_str, b); }
	CF_INLINE ~String() { sfree(m_str); m_str = NULL; }

	CF_INLINE static String steal_from(char* cute_c_api_string) { CF_ACANARY(cute_c_api_string); String r; r.m_str = cute_c_api_string; return r; }
	CF_INLINE char* steal() { char* result = m_str; m_str = NULL; return result; }
	CF_INLINE static String from_hex(uint64_t uint) { String r; shex(r.m_str, uint); return r; }

	CF_INLINE int to_int() const { return stoint(m_str); }
	CF_INLINE uint64_t to_uint() const { return stouint(m_str); }
	CF_INLINE float to_float() const { return stofloat(m_str); }
	CF_INLINE double to_double() const { return stodouble(m_str); }
	CF_INLINE uint64_t to_hex() const { return stohex(m_str); }
	CF_INLINE bool to_bool() const { return stobool(m_str); }

	CF_INLINE const char* c_str() const { return m_str; }
	CF_INLINE char* c_str() { return m_str; }
	CF_INLINE const char* begin() const { return m_str; }
	CF_INLINE char* begin() { return m_str; }
	CF_INLINE const char* end() const { return m_str + scount(m_str); }
	CF_INLINE char* end() { return m_str + scount(m_str); }
	CF_INLINE char last() const { return slast(m_str); }
	CF_INLINE char first() const { return sfirst(m_str); }
	CF_INLINE operator const char*() const { return m_str; }
	CF_INLINE operator char*() const { return m_str; }

	CF_INLINE char& operator[](int index) { s_chki(index); return m_str[index]; }
	CF_INLINE const char& operator[](int index) const { s_chki(index); return m_str[index]; }

	CF_INLINE int len() const { return slen(m_str); }
	CF_INLINE int capacity() const { return scap(m_str); }
	CF_INLINE int size() const { return scount(m_str); }
	CF_INLINE int count() const { return scount(m_str); }
	CF_INLINE void ensure_capacity(int capacity) { sfit(m_str, capacity); }
	CF_INLINE void fit(int capacity) { sfit(m_str, capacity); }
	CF_INLINE void set_len(int len) { sfit(m_str, len + 1); cf_array_len(m_str) = len + 1; m_str[len] = 0; }
	CF_INLINE bool empty() const { return sempty(m_str); }

	CF_INLINE String& add(char ch) { spush(m_str, ch); return *this; }
	CF_INLINE String& append(const char* s) { sappend(m_str, s); return *this; }
	CF_INLINE String& operator+(const char* s) { sappend(m_str, s); return *this; }
	CF_INLINE String& operator+(int i) { sfmt_append(m_str, "%d", i); return *this; }
	CF_INLINE String& operator+(uint64_t uint) { sfmt_append(m_str, "%" PRIu64, uint); return *this; }
	CF_INLINE String& operator+(float f) { sfmt_append(m_str, "%f", f); return *this; }
	CF_INLINE String& operator+(double f) { sfmt_append(m_str, "%f", f); return *this; }
	CF_INLINE String& operator+(bool b) { sfmt_append(m_str, "%s", b ? "true" : "false"); return *this; }
	CF_INLINE String& operator+(v2 v) { sfmt_append(m_str, "{ %f, %f }", v.x, v.y); return *this; }
	CF_INLINE String& append(const char* start, const char* end) { sappend_range(m_str, start, end); return *this; }
	CF_INLINE String& append(int codepoint) { sappend_UTF8(m_str, codepoint); return *this; }
	static CF_INLINE String fmt(const char* fmt, ...) { String result; va_list args; va_start(args, fmt); svfmt(result.m_str, fmt, args); va_end(args); return result; }
	CF_INLINE String& fmt_append(const char* fmt, ...) { va_list args; va_start(args, fmt); svfmt_append(m_str, fmt, args); va_end(args); return *this; }
	CF_INLINE String& trim() { strim(m_str); return *this; }
	CF_INLINE String& ltrim() { sltrim(m_str); return *this; }
	CF_INLINE String& rtrim() { srtrim(m_str); return *this; }
	CF_INLINE String& lpad(char pad, int count) { slpad(m_str, pad, count); return *this; }
	CF_INLINE String& rpad(char pad, int count) { srpad(m_str, pad, count); return *this; }
	CF_INLINE String& dedup(char ch) { sdedup(m_str, ch); return *this; }
	CF_INLINE String& set(const char* s) { sset(m_str, s); return *this; }
	CF_INLINE String& operator=(const char* s) { sset(m_str, s); return *this; }
	CF_INLINE String& operator=(const String& s) { sset(m_str, s); return *this; }
	CF_INLINE String& operator=(String&& s) { m_str = s.m_str; s.m_str = NULL; return *this; }
	CF_INLINE Array<String> split(char split_c) { Array<String> r; char** s = ssplit(m_str, split_c); for (int i=0;i<alen(s);++i) r.add(cf_move(steal_from(s[i]))); afree(s); return r; }
	static CF_INLINE Array<String> split(const char* split_me, char split_c) { Array<String> r; char** s = ssplit(split_me, split_c); for (int i=0;i<alen(s);++i) r.add(cf_move(steal_from(s[i]))); afree(s); return r; }
	CF_INLINE char pop() { char result = slast(m_str); spop(m_str); return result; }
	CF_INLINE char pop(int n) { char result = slast(m_str); spopn(m_str, n); return result; }
	CF_INLINE char popn(int n) { char result = slast(m_str); spopn(m_str, n); return result; }
	CF_INLINE int first_index_of(char ch) const { return sfirst_index_of(m_str, ch); }
	CF_INLINE int last_index_of(char ch) const { return slast_index_of(m_str, ch); }
	CF_INLINE int first_index_of(char ch, int offset) const { return sfirst_index_of(m_str + offset, ch); }
	CF_INLINE int last_index_of(char ch, int offset) const { return slast_index_of(m_str + offset, ch); }
	CF_INLINE int find(const char* find_me) const { const char* ptr = sfind(m_str, find_me); return (int)(ptr ? ptr - m_str : -1); }
	CF_INLINE String& replace(const char* replace_me, const char* with_me) { sreplace(m_str, replace_me, with_me); return *this; }
	CF_INLINE String& erase(int index, int count) { serase(m_str, index, count); return *this; }
	CF_INLINE String dup() const { return steal_from(sdup(m_str)); }
	CF_INLINE void clear() { sclear(m_str); }
	
	CF_INLINE bool starts_with(const char* s) const { return sprefix(m_str, s); }
	CF_INLINE bool begins_with(const char* s) const { return sprefix(m_str, s); }
	CF_INLINE bool ends_with(const char* s) const { return ssuffix(m_str, s); }
	CF_INLINE bool prefix(const char* s) const { return sprefix(m_str, s); }
	CF_INLINE bool suffix(const char* s) const { return ssuffix(m_str, s); }
	CF_INLINE bool operator==(const char* s) { return !CF_STRCMP(m_str, s); }
	CF_INLINE bool operator!=(const char* s) { return CF_STRCMP(m_str, s); }
	CF_INLINE bool compare(const char* s, bool no_case = false) { return no_case ? sequ(m_str, s) : siequ(m_str, s); }
	CF_INLINE bool cmp(const char* s, bool no_case = false) { return compare(s, no_case); }
	CF_INLINE bool contains(const char* contains_me) { return scontains(m_str, contains_me); }
	CF_INLINE String& to_upper() { stoupper(m_str); return *this; }
	CF_INLINE String& to_lower() { stolower(m_str); return *this; }
	CF_INLINE uint64_t hash() const { return shash(m_str); }

private:
	char* m_str = NULL;
	CF_INLINE void s_chki(int i) const { CF_ASSERT(i >= 0 && i < scount(m_str)); }
};

CF_INLINE String operator+(const String& a, const String& b) { String result = a; result.append(b); return result; }
CF_INLINE String to_string(const char* s) { return String(s); }
CF_INLINE String to_string(int i) { return String(i); }
CF_INLINE String to_string(uint64_t uint) { return String(uint); }
CF_INLINE String to_string(float f) { return String(f); }
CF_INLINE String to_string(double f) { return String(f); }
CF_INLINE String to_string(bool b) { return String(b); }

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
	CF_INLINE UTF8() { }
	CF_INLINE UTF8(const char* text) { this->text = text; }

	CF_INLINE bool next() { if (*text) { text = cf_decode_UTF8(text, &codepoint); return true; } else return false; }

	int codepoint;
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
	CF_INLINE UTF16() { }
	CF_INLINE UTF16(const uint16_t* text) { this->text = text; }

	CF_INLINE bool next() { if (*text) { text = cf_decode_UTF16(text, &codepoint); return true; } else return false; }

	int codepoint;
	const uint16_t* text = NULL;
};

}

#endif // CF_CPP

#endif // CF_STRING_H
