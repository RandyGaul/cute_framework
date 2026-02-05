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
#include "cute_array.h"
#include "cute_math.h"

#include <inttypes.h>
#include <stdarg.h>

// Shortform string macros (slen, sset, sfmt, sappend, sfree, etc.)
// are provided by ckit.h (included via cute_array.h).

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @function sdyna
 * @category string
 * @brief    An empty macro used in the C API to markup dynamic strings.
 * @example > Showcase of basic dynamic string features.
 *     // Create a new dynamic string.
 *     sdyna char* s = smake("Hello world!");
 *     printf("%s\n", s);  // Prints: Hello world!
 *
 *     // Overwrite it with a new value.
 *     sset(s, "Goodbye!");
 *     printf("%s\n", s);  // Prints: Goodbye!
 *
 *     // Append text to the string.
 *     sappend(s, " See you later.");
 *     printf("%s\n", s);  // Prints: Goodbye! See you later.
 *
 *     // Printf-style formatting.
 *     sfmt(s, "The answer is %d", 42);
 *     printf("%s\n", s);  // Prints: The answer is 42
 *
 *     // String comparison.
 *     if (sequ(s, "The answer is 42")) {
 *         printf("Correct!\n");
 *     }
 *
 *     // Clean up.
 *     sfree(s);
 * @remarks  This is an optional and _completely_ empty macro. It's only purpose is to provide a bit of visual indication a type is a
 *           dynamic string. One downside of the C-macro API is the opaque nature of the pointer type. Since the macros use polymorphism
 *           on typed pointers (`char*`), there's no actual string struct type visible. `sdyna` helps visually indicate a pointer is a
 *           dynamic string, not just a plain `char*`.
 *
 *           Dynamic strings are 100% compatible with normal C-strings -- pass them to `printf`, `strcmp`, etc.
 *           They automatically grow on the heap as needed. Free with `sfree` when done.
 *
 *           To create a new string use `smake` or `sfmake`. To overwrite an existing string use `sset` or `sfmt`.
 *           Note: `sset`/`sfmt` require an l-value (a variable), not a literal like `NULL` -- use `smake`/`sfmake` to create from scratch.
 * @related  sdyna smake sfmake sset sfmt sappend sfree slen sequ
 */
#define sdyna CK_SDYNA

/**
 * @function shex
 * @category string
 * @brief    Converts a uint64_t to a hex-string and assigns `s` to it.
 * @param    s            The string.
 * @param    uint         The value to convert.
 * @related  sdyna sint suint sfloat sdouble shex sbool stint stouint stofloat stodouble stohex stobool
 */
#define shex(s, uint) cf_string_hex(s, uint)

/**
 * @function sstatic
 * @category string
 * @brief    Creates a string with an initial static storage backing.
 * @param    s            The string. Can be `NULL`.
 * @param    buffer       Pointer to a static memory buffer.
 * @param    buffer_size  The size of `buffer` in bytes.
 * @remarks  Will grow onto the heap if the size becomes too large. Call `sfree` when done.
 * @related  sdyna sstatic sisdyna spush sset
 */
#define sstatic(s, buffer, buffer_size) cf_string_static(s, buffer, buffer_size)

/**
 * @function sisdyna
 * @category string
 * @brief    Checks to see if a C string is a dynamic string from Cute Framework's string API, or not.
 * @param    s            The string. Can be `NULL`.
 * @return   Returns true if `s` is a dynamically alloced string from this C string API.
 * @remarks  This can be evaluated at compile time for string literals.
 * @related  sdyna sstatic sisdyna spush sset
 */
#define sisdyna(s) cf_string_is_dynamic(s)

//--------------------------------------------------------------------------------------------------
// String Interning C API (global string table).

// Override sintern to use CF's thread-safe implementation instead of ckit's.
#undef sintern
/**
 * @function sintern
 * @category string
 * @brief    Stores unique, static copy of a string in a global string interning table.
 * @param    s            The string to insert into the global table.
 * @return   Returns a static, unique, stable, read-only copy of the string. The pointer is stable until `sinuke` is called.
 * @example > Using sintern for fast string comparison and as hash table keys.
 *     // Intern some strings.
 *     const char* apple = sintern("apple");
 *     const char* banana = sintern("banana");
 *     const char* apple2 = sintern("apple");  // Returns same pointer as first apple!
 *
 *     // Fast pointer comparison (no strcmp needed).
 *     CF_ASSERT(apple == apple2);
 *     CF_ASSERT(apple != banana);
 *
 *     // Use as hash table keys (cast pointer to uint64_t).
 *     CF_MAP(int) prices = NULL;
 *     map_set(prices, (uint64_t)apple, 100);
 *     map_set(prices, (uint64_t)banana, 50);
 *     int apple_price = map_get(prices, (uint64_t)sintern("apple"));
 *     CF_ASSERT(apple_price == 100);
 *     map_free(prices);
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

// Override sintern to use CF's thread-safe implementation instead of ckit's.
#undef sintern_range
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
#define sivalid ck_sivalid

/**
 * @function silen
 * @category string
 * @brief    Returns the length of an intern'd string.
 * @param    s            The string.
 * @remarks  This is *not* a secure method -- do not use it on any potentially dangerous strings. It's designed to be very simple and fast, nothing more.
 *           The return value is calculated in constant time, as opposed to calling `CF_STRLEN` (`strlen`).
 * @related  sintern sintern_range sivalid silen sinuke
 */
#define silen ck_silen

/**
 * @function sinuke
 * @category string
 * @brief    Frees up all resources used by the global string table built by `sintern`.
 * @remarks  All strings previously returned by `sintern` are now invalid.
 * @related  sintern sintern_range sivalid silen sinuke
 */
#define sinuke() cf_sinuke_intern_table()

//--------------------------------------------------------------------------------------------------
// UTF8 and UTF16.

/**
 * @function sappend_UTF8
 * @category string
 * @brief    Appends a UTF32 codepoint (as `uint32_t`) encoded as UTF8 onto the string.
 * @param    s            The string. Can be `NULL`.
 * @param    codepoint    An `int` codepoint in UTF32 form.
 * @example > Appending UTF32 codepoints as UTF8 in a loop.
 *     sdyna char* s = NULL;
 *     while (has_codepoint()) {
 *         sappend_UTF8(s, get_codepoint());
 *     }
 *     sfree(s);
 * @remarks  The UTF8 bytes are appended onto the string.
 *
 *           Each UTF32 codepoint represents a single character. Each character can be encoded from 1 to 4
 *           bytes. Therefore, this function will push 1 to 4 bytes onto the string.
 *
 *           If an invalid codepoint is found the "replacement character" 0xFFFD will be appended instead, which
 *           looks like a question mark inside a dark diamond.
 * @related  sappend_UTF8 cf_decode_UTF8 cf_decode_UTF16
 */
// #define sappend_UTF8(s, cp)

/**
 * @function cf_decode_UTF8
 * @category string
 * @brief    Decodes a single UTF8 character from the string as a UTF32 codepoint.
 * @param    s            The string.
 * @param    codepoint    An `int` pointer to receive the decoded codepoint.
 * @return   Returns pointer advanced past the decoded character (1 to 4 bytes).
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
// const char* cf_decode_UTF8(const char* s, int* codepoint);

/**
 * @function cf_decode_UTF16
 * @category string
 * @brief    Decodes a single UTF16 character from the string as a UTF32 codepoint.
 * @param    s            The string.
 * @param    codepoint    An `int` pointer to receive the decoded codepoint.
 * @return   Returns pointer advanced past the decoded character (1 to 2 uint16_t's).
 * @example > Decoding a UTF16 string one codepoint at a time.
 *     int cp;
 *     const uint16_t* tmp = my_string;
 *     while (*tmp) {
 *         tmp = cf_decode_UTF16(tmp, &cp);
 *         DoSomethingWithCodepoint(cp);
 *     }
 * @example > Converting a UTF16 string to UTF8.
 *     char* utf16_to_utf8(const uint16_t* text)
 *     {
 *         int cp;
 *         sdyna char* s = NULL;
 *         while (*text) {
 *             text = cf_decode_UTF16(text, &cp);
 *             sappend_UTF8(s, cp);
 *         }
 *         return s;
 *     }
 * @remarks  You can use this function in a loop to decode one codepoint at a time, where each codepoint
 *           represents a single UTF16 character. You can convert a UTF16 string to UTF8 by calling `sappend_UTF8`
 *           on another string instance inside a decode loop.
 * @related  sappend_UTF8 cf_decode_UTF8 cf_decode_UTF16
 */
CF_API const uint16_t* CF_CALL cf_decode_UTF16(const uint16_t* s, int* codepoint);

//--------------------------------------------------------------------------------------------------
// Longform C API.
// These map cf_string_* names to the shortform ckit macros.

/**
 * @function cf_string_len
 * @category string
 * @brief    Returns the length of the string (not counting the nul-terminator), or 0 if NULL.
 * @param    s            The string.
 * @return   Returns the length of `s`.
 * @remarks  Shortform: `slen(s)`.
 * @related  cf_string_size cf_string_count cf_string_empty cf_string_free
 */
#define cf_string_len(s) slen(s)

/**
 * @function cf_string_empty
 * @category string
 * @brief    Returns true if the string is NULL or has zero length.
 * @param    s            The string. Can be `NULL`.
 * @return   Returns true if the string is empty.
 * @remarks  Shortform: `sempty(s)`.
 * @related  cf_string_len cf_string_free
 */
#define cf_string_empty(s) sempty(s)

/**
 * @function cf_string_push
 * @category string
 * @brief    Appends a single character to the end of the string.
 * @param    s            The string. Can be `NULL`.
 * @param    ch           The character to append.
 * @remarks  Shortform: `spush(s, ch)`.
 * @related  cf_string_pop cf_string_append cf_string_set
 */
#define cf_string_push(s, ch) spush(s, ch)

/**
 * @function cf_string_free
 * @category string
 * @brief    Frees the string and sets the pointer to NULL.
 * @param    s            The string. Modified in-place. Safe to call on NULL.
 * @remarks  Shortform: `sfree(s)`.
 * @related  cf_string_set cf_string_clear cf_string_make
 */
#define cf_string_free(s) sfree(s)

/**
 * @function cf_string_size
 * @category string
 * @brief    Returns the byte count including the nul-terminator, or 0 if NULL.
 * @param    s            The string. Can be `NULL`.
 * @return   Returns the byte count of `s` including the nul-terminator.
 * @remarks  Shortform: `scount(s)`.
 * @related  cf_string_len cf_string_count cf_string_cap
 */
#define cf_string_size(s) scount(s)

/**
 * @function cf_string_count
 * @category string
 * @brief    Returns the byte count including the nul-terminator, or 0 if NULL. Same as `cf_string_size`.
 * @param    s            The string. Can be `NULL`.
 * @return   Returns the byte count of `s` including the nul-terminator.
 * @remarks  Shortform: `scount(s)`.
 * @related  cf_string_len cf_string_size cf_string_cap
 */
#define cf_string_count(s) scount(s)

/**
 * @function cf_string_cap
 * @category string
 * @brief    Returns the capacity of the string's backing buffer, or 0 if NULL.
 * @param    s            The string. Can be `NULL`.
 * @return   Returns the number of bytes allocated.
 * @remarks  Shortform: `scap(s)`.
 * @related  cf_string_len cf_string_fit
 */
#define cf_string_cap(s) scap(s)

/**
 * @function cf_string_first
 * @category string
 * @brief    Returns the first character of the string, or `'\0'` if NULL/empty.
 * @param    s            The string. Can be `NULL`.
 * @return   Returns the first character.
 * @remarks  Shortform: `sfirst(s)`.
 * @related  cf_string_last cf_string_len
 */
#define cf_string_first(s) sfirst(s)

/**
 * @function cf_string_last
 * @category string
 * @brief    Returns the last character of the string, or `'\0'` if NULL/empty.
 * @param    s            The string. Can be `NULL`.
 * @return   Returns the last character.
 * @remarks  Shortform: `slast(s)`.
 * @related  cf_string_first cf_string_len
 */
#define cf_string_last(s) slast(s)

/**
 * @function cf_string_clear
 * @category string
 * @brief    Sets the string to empty without freeing memory.
 * @param    s            The string.
 * @remarks  Shortform: `sclear(s)`.
 * @related  cf_string_free cf_string_len
 */
#define cf_string_clear(s) sclear(s)

/**
 * @function cf_string_fit
 * @category string
 * @brief    Ensures the string can hold at least `n` bytes, growing if necessary.
 * @param    s            The string. Modified in-place.
 * @param    n            The minimum number of bytes to reserve.
 * @remarks  Shortform: `sfit(s, n)`.
 * @related  cf_string_cap cf_string_len
 */
#define cf_string_fit(s, n) sfit(s, n)

/**
 * @function cf_string_fmt
 * @category string
 * @brief    Sets the string to a printf-style formatted result.
 * @param    s            The string. Modified in-place.
 * @param    fmt          A printf-style format string.
 * @param    ...          Format arguments.
 * @remarks  Shortform: `sfmt(s, fmt, ...)`.
 * @related  cf_string_fmt_append cf_string_vfmt cf_string_set
 */
#define cf_string_fmt(s, fmt, ...) sfmt(s, fmt, __VA_ARGS__)

/**
 * @function cf_string_fmt_append
 * @category string
 * @brief    Appends a printf-style formatted result to the string.
 * @param    s            The string. Modified in-place.
 * @param    fmt          A printf-style format string.
 * @param    ...          Format arguments.
 * @remarks  Shortform: `sfmt_append(s, fmt, ...)`.
 * @related  cf_string_fmt cf_string_vfmt_append cf_string_append
 */
#define cf_string_fmt_append(s, fmt, ...) sfmt_append(s, fmt, __VA_ARGS__)

/**
 * @function cf_string_vfmt
 * @category string
 * @brief    Sets the string to a vprintf-style formatted result.
 * @param    s            The string. Modified in-place.
 * @param    fmt          A printf-style format string.
 * @param    args         A `va_list` of format arguments.
 * @remarks  Shortform: `svfmt(s, fmt, args)`.
 * @related  cf_string_fmt cf_string_vfmt_append
 */
#define cf_string_vfmt(s, fmt, args) svfmt(s, fmt, args)

/**
 * @function cf_string_vfmt_append
 * @category string
 * @brief    Appends a vprintf-style formatted result to the string.
 * @param    s            The string. Modified in-place.
 * @param    fmt          A printf-style format string.
 * @param    args         A `va_list` of format arguments.
 * @remarks  Shortform: `svfmt_append(s, fmt, args)`.
 * @related  cf_string_vfmt cf_string_fmt_append
 */
#define cf_string_vfmt_append(s, fmt, args) svfmt_append(s, fmt, args)

/**
 * @function cf_string_set
 * @category string
 * @brief    Sets the string to a copy of another string.
 * @param    a            The destination string. Must be an l-value (a variable, not NULL). Modified in-place.
 * @param    b            The source C string.
 * @remarks  Shortform: `sset(a, b)`. To create a new string from scratch use `smake` or `sfmake` instead.
 * @related  cf_string_dup cf_string_make cf_string_fmt_make cf_string_fmt cf_string_free
 */
#define cf_string_set(a, b) sset(a, b)

/**
 * @function cf_string_dup
 * @category string
 * @brief    Allocates and returns a new dynamic string copy. Free with `sfree`.
 * @param    s            The source C string.
 * @return   Returns a new dynamic string.
 * @remarks  Shortform: `sdup(s)`.
 * @related  cf_string_make cf_string_set cf_string_free
 */
#define cf_string_dup(s) sdup(s)

/**
 * @function cf_string_make
 * @category string
 * @brief    Allocates and returns a new dynamic string copy. Same as `cf_string_dup`. Free with `sfree`.
 * @param    s            The source C string.
 * @return   Returns a new dynamic string.
 * @remarks  Shortform: `smake(s)`.
 * @related  cf_string_dup cf_string_set cf_string_free
 */
#define cf_string_make(s) smake(s)

/**
 * @function cf_string_fmt_make
 * @category string
 * @brief    Allocates and returns a new formatted dynamic string. Free with `sfree`.
 * @param    fmt          The printf-style format string.
 * @return   Returns a new dynamic string.
 * @remarks  Shortform: `sfmake(fmt, ...)`. Use this instead of `sfmt` when creating a string from scratch.
 * @related  cf_string_make cf_string_fmt cf_string_set cf_string_free
 */
#define cf_string_fmt_make(fmt, ...) sfmake(fmt, __VA_ARGS__)

/**
 * @function cf_string_cmp
 * @category string
 * @brief    Compares two strings lexicographically (case-sensitive).
 * @param    a            First string.
 * @param    b            Second string.
 * @return   Returns 0 if equal, negative if `a < b`, positive if `a > b`.
 * @remarks  Shortform: `scmp(a, b)`.
 * @related  cf_string_icmp cf_string_equ cf_string_iequ
 */
#define cf_string_cmp(a, b) scmp(a, b)

/**
 * @function cf_string_icmp
 * @category string
 * @brief    Compares two strings lexicographically (case-insensitive).
 * @param    a            First string.
 * @param    b            Second string.
 * @return   Returns 0 if equal, negative if `a < b`, positive if `a > b`.
 * @remarks  Shortform: `sicmp(a, b)`.
 * @related  cf_string_cmp cf_string_equ cf_string_iequ
 */
#define cf_string_icmp(a, b) sicmp(a, b)

/**
 * @function cf_string_equ
 * @category string
 * @brief    Returns true if two strings are equal (case-sensitive). NULL-safe.
 * @param    a            First string. Can be `NULL`.
 * @param    b            Second string. Can be `NULL`.
 * @return   Returns true if the strings are equal.
 * @remarks  Shortform: `sequ(a, b)`.
 * @related  cf_string_iequ cf_string_cmp cf_string_icmp
 */
#define cf_string_equ(a, b) sequ(a, b)

/**
 * @function cf_string_iequ
 * @category string
 * @brief    Returns true if two strings are equal (case-insensitive). NULL-safe.
 * @param    a            First string. Can be `NULL`.
 * @param    b            Second string. Can be `NULL`.
 * @return   Returns true if the strings are equal (ignoring case).
 * @remarks  Shortform: `siequ(a, b)`.
 * @related  cf_string_equ cf_string_cmp cf_string_icmp
 */
#define cf_string_iequ(a, b) siequ(a, b)

/**
 * @function cf_string_prefix
 * @category string
 * @brief    Returns true if the string starts with the given prefix.
 * @param    s            The string.
 * @param    prefix       The prefix to test.
 * @return   Returns true if `s` starts with `prefix`.
 * @remarks  Shortform: `sprefix(s, prefix)`.
 * @related  cf_string_suffix cf_string_contains cf_string_find
 */
#define cf_string_prefix(s, prefix) sprefix(s, prefix)

/**
 * @function cf_string_suffix
 * @category string
 * @brief    Returns true if the string ends with the given suffix.
 * @param    s            The string.
 * @param    suffix       The suffix to test.
 * @return   Returns true if `s` ends with `suffix`.
 * @remarks  Shortform: `ssuffix(s, suffix)`.
 * @related  cf_string_prefix cf_string_contains cf_string_find
 */
#define cf_string_suffix(s, suffix) ssuffix(s, suffix)

/**
 * @function cf_string_contains
 * @category string
 * @brief    Returns true if the string contains the given substring.
 * @param    s            The string.
 * @param    contains_me  The substring to search for.
 * @return   Returns true if `contains_me` is found within `s`.
 * @remarks  Shortform: `scontains(s, contains_me)`.
 * @related  cf_string_find cf_string_prefix cf_string_suffix
 */
#define cf_string_contains(s, contains_me) scontains(s, contains_me)

/**
 * @function cf_string_toupper
 * @category string
 * @brief    Converts the string to uppercase in-place.
 * @param    s            The string.
 * @remarks  Shortform: `stoupper(s)`.
 * @related  cf_string_tolower
 */
#define cf_string_toupper(s) stoupper(s)

/**
 * @function cf_string_tolower
 * @category string
 * @brief    Converts the string to lowercase in-place.
 * @param    s            The string.
 * @remarks  Shortform: `stolower(s)`.
 * @related  cf_string_toupper
 */
#define cf_string_tolower(s) stolower(s)

/**
 * @function cf_string_hash
 * @category string
 * @brief    Returns a FNV1a hash of the string.
 * @param    s            The string.
 * @return   Returns a `uint64_t` hash value.
 * @remarks  Shortform: `shash(s)`.
 * @related  cf_string_equ cf_string_cmp
 */
#define cf_string_hash(s) shash(s)

/**
 * @function cf_string_append
 * @category string
 * @brief    Appends string `b` onto the end of string `a`.
 * @param    a            The destination string. Modified in-place.
 * @param    b            The source C string to append.
 * @remarks  Shortform: `sappend(a, b)`.
 * @related  cf_string_append_range cf_string_fmt_append cf_string_set
 */
#define cf_string_append(a, b) sappend(a, b)

/**
 * @function cf_string_append_range
 * @category string
 * @brief    Appends a range of characters `[b, b_end)` onto string `a`.
 * @param    a            The destination string. Modified in-place.
 * @param    b            Pointer to the start of the range.
 * @param    b_end        Pointer one past the end of the range.
 * @remarks  Shortform: `sappend_range(a, b, b_end)`.
 * @related  cf_string_append cf_string_set
 */
#define cf_string_append_range(a, b, b_end) sappend_range(a, b, b_end)

/**
 * @function cf_string_trim
 * @category string
 * @brief    Trims leading and trailing whitespace from the string in-place.
 * @param    s            The string. Modified in-place.
 * @remarks  Shortform: `strim(s)`.
 * @related  cf_string_ltrim cf_string_rtrim
 */
#define cf_string_trim(s) strim(s)

/**
 * @function cf_string_ltrim
 * @category string
 * @brief    Trims leading whitespace from the string in-place.
 * @param    s            The string. Modified in-place.
 * @remarks  Shortform: `sltrim(s)`.
 * @related  cf_string_trim cf_string_rtrim
 */
#define cf_string_ltrim(s) sltrim(s)

/**
 * @function cf_string_rtrim
 * @category string
 * @brief    Trims trailing whitespace from the string in-place.
 * @param    s            The string. Modified in-place.
 * @remarks  Shortform: `srtrim(s)`.
 * @related  cf_string_trim cf_string_ltrim
 */
#define cf_string_rtrim(s) srtrim(s)

/**
 * @function cf_string_lpad
 * @category string
 * @brief    Left-pads the string to at least `n` characters using `ch`.
 * @param    s            The string. Modified in-place.
 * @param    ch           The padding character.
 * @param    n            The minimum total length after padding.
 * @remarks  Shortform: `slpad(s, ch, n)`.
 * @related  cf_string_rpad cf_string_trim
 */
#define cf_string_lpad(s, ch, n) slpad(s, ch, n)

/**
 * @function cf_string_rpad
 * @category string
 * @brief    Right-pads the string to at least `n` characters using `ch`.
 * @param    s            The string. Modified in-place.
 * @param    ch           The padding character.
 * @param    n            The minimum total length after padding.
 * @remarks  Shortform: `srpad(s, ch, n)`.
 * @related  cf_string_lpad cf_string_trim
 */
#define cf_string_rpad(s, ch, n) srpad(s, ch, n)

/**
 * @function cf_string_split_once
 * @category string
 * @brief    Splits the string at the first occurrence of `ch`, returning the left half.
 * @param    s            The string. Must be a dynamically allocated string from this string API. Does *not* work on string literals.
 * @param    ch           The delimiter character.
 * @return   Returns the string to the left of `ch`. Returns `NULL` if `ch` isn't found (does not modify `s`).
 * @example > Splitting a path into components using a loop.
 *     sdyna char* path = smake("/home/user/documents/file.txt");
 *     sdyna char* part;
 *     while ((part = ssplit_once(path, '/'))) {
 *         printf("Part: %s\n", part);
 *         sfree(part);
 *     }
 *     printf("Remaining: %s\n", path);
 *     sfree(path);
 * @remarks  Shortform: `ssplit_once(s, ch)`. After the call, `s` contains the string to the right of `ch`.
 *           You must call `sfree` on the returned string.
 *
 *           This function is intended to be used in a loop, successively chopping off pieces of `s`.
 *           A much easier, but slightly slower, version of this function is `ssplit`, which returns
 *           an array of strings all at once.
 * @related  cf_string_split cf_string_find
 */
#define cf_string_split_once(s, ch) ssplit_once(s, ch)

/**
 * @function cf_string_split
 * @category string
 * @brief    Splits the string at every occurrence of `ch`, returning a dynamic array of strings.
 * @param    s            The source C string (not modified).
 * @param    ch           The delimiter character.
 * @return   Returns a `dyna char**` array of newly allocated strings. Free each string with `sfree`, then free the array with `afree`.
 * @example > Splitting a string about '.'.
 *     sdyna char* s = NULL;
 *     sset(s, "split.here.in.a.loop");
 *     const char* splits_expected[] = {
 *         "split",
 *         "here",
 *         "in",
 *         "a",
 *         "loop",
 *     };
 *     dyna char** array_of_splits = ssplit(s, '.');
 *     for (int i = 0; i < alen(array_of_splits); ++i) {
 *         const char* split = array_of_splits[i];
 *         CF_ASSERT(sequ(split, splits_expected[i]));
 *         sfree(split);
 *     }
 *     afree(array_of_splits);
 *     sfree(s);
 * @remarks  Shortform: `ssplit(s, ch)`. The original string `s` is not modified. You must call `sfree` on each
 *           returned string and `afree` on the returned array.
 * @related  cf_string_split_once cf_string_find
 */
#define cf_string_split(s, ch) ssplit(s, ch)

/**
 * @function cf_string_first_index_of
 * @category string
 * @brief    Returns the index of the first occurrence of `ch` in the string.
 * @param    s            The string.
 * @param    ch           The character to search for.
 * @return   Returns the index, or -1 if not found.
 * @remarks  Shortform: `sfirst_index_of(s, ch)`.
 * @related  cf_string_last_index_of cf_string_find cf_string_contains
 */
#define cf_string_first_index_of(s, ch) sfirst_index_of(s, ch)

/**
 * @function cf_string_last_index_of
 * @category string
 * @brief    Returns the index of the last occurrence of `ch` in the string.
 * @param    s            The string.
 * @param    ch           The character to search for.
 * @return   Returns the index, or -1 if not found.
 * @remarks  Shortform: `slast_index_of(s, ch)`.
 * @related  cf_string_first_index_of cf_string_find cf_string_contains
 */
#define cf_string_last_index_of(s, ch) slast_index_of(s, ch)

/**
 * @function cf_string_find
 * @category string
 * @brief    Finds the first occurrence of a substring within the string.
 * @param    s            The string to search in.
 * @param    find         The substring to search for.
 * @return   Returns a pointer to the first match, or NULL if not found.
 * @remarks  Shortform: `sfind(s, find)`.
 * @related  cf_string_contains cf_string_first_index_of cf_string_replace
 */
#define cf_string_find(s, find) sfind(s, find)

/**
 * @function cf_string_int
 * @category string
 * @brief    Sets the string to the decimal representation of an integer.
 * @param    s            The string. Modified in-place.
 * @param    i            The integer value.
 * @remarks  Shortform: `sint(s, i)`.
 * @related  cf_string_uint cf_string_float cf_string_double cf_string_hex cf_string_bool cf_string_toint
 */
#define cf_string_int(s, i) sint(s, i)

/**
 * @function cf_string_uint
 * @category string
 * @brief    Sets the string to the decimal representation of an unsigned integer.
 * @param    s            The string. Modified in-place.
 * @param    uint         The unsigned integer value.
 * @remarks  Shortform: `suint(s, uint)`.
 * @related  cf_string_int cf_string_float cf_string_double cf_string_hex cf_string_bool cf_string_touint
 */
#define cf_string_uint(s, uint) suint(s, uint)

/**
 * @function cf_string_float
 * @category string
 * @brief    Sets the string to the decimal representation of a float.
 * @param    s            The string. Modified in-place.
 * @param    f            The float value.
 * @remarks  Shortform: `sfloat(s, f)`.
 * @related  cf_string_double cf_string_int cf_string_uint cf_string_tofloat
 */
#define cf_string_float(s, f) sfloat(s, f)

/**
 * @function cf_string_double
 * @category string
 * @brief    Sets the string to the decimal representation of a double.
 * @param    s            The string. Modified in-place.
 * @param    f            The double value.
 * @remarks  Shortform: `sdouble(s, f)`.
 * @related  cf_string_float cf_string_int cf_string_uint cf_string_todouble
 */
#define cf_string_double(s, f) sdouble(s, f)

/**
 * @function cf_string_hex
 * @category string
 * @brief    Sets the string to the hex representation of an unsigned integer.
 * @param    s            The string. Modified in-place.
 * @param    uint         The unsigned integer value.
 * @remarks  Shortform: `shex(s, uint)`.
 * @related  cf_string_int cf_string_uint cf_string_tohex
 */
#define cf_string_hex(s, uint) sfmt(s, "0x%x", uint)

/**
 * @function cf_string_bool
 * @category string
 * @brief    Sets the string to `"true"` or `"false"`.
 * @param    s            The string. Modified in-place.
 * @param    b            The boolean value.
 * @remarks  Shortform: `sbool(s, b)`.
 * @related  cf_string_int cf_string_tobool
 */
#define cf_string_bool(s, b) sbool(s, b)

/**
 * @function cf_string_toint
 * @category string
 * @brief    Parses the string as a decimal integer.
 * @param    s            The string.
 * @return   Returns the parsed `int` value.
 * @remarks  Shortform: `stoint(s)`.
 * @related  cf_string_int cf_string_touint cf_string_tofloat cf_string_todouble cf_string_tohex cf_string_tobool
 */
#define cf_string_toint(s) stoint(s)

/**
 * @function cf_string_touint
 * @category string
 * @brief    Parses the string as an unsigned integer.
 * @param    s            The string.
 * @return   Returns the parsed `uint64_t` value.
 * @remarks  Shortform: `stouint(s)`.
 * @related  cf_string_uint cf_string_toint cf_string_tofloat cf_string_todouble cf_string_tohex cf_string_tobool
 */
#define cf_string_touint(s) stouint(s)

/**
 * @function cf_string_tofloat
 * @category string
 * @brief    Parses the string as a float.
 * @param    s            The string.
 * @return   Returns the parsed `float` value.
 * @remarks  Shortform: `stofloat(s)`.
 * @related  cf_string_float cf_string_toint cf_string_todouble
 */
#define cf_string_tofloat(s) stofloat(s)

/**
 * @function cf_string_todouble
 * @category string
 * @brief    Parses the string as a double.
 * @param    s            The string.
 * @return   Returns the parsed `double` value.
 * @remarks  Shortform: `stodouble(s)`.
 * @related  cf_string_double cf_string_toint cf_string_tofloat
 */
#define cf_string_todouble(s) stodouble(s)

/**
 * @function cf_string_tohex
 * @category string
 * @brief    Parses the string as a hexadecimal unsigned integer.
 * @param    s            The string.
 * @return   Returns the parsed `uint64_t` value.
 * @remarks  Shortform: `stohex(s)`.
 * @related  cf_string_hex cf_string_toint cf_string_touint
 */
#define cf_string_tohex(s) stohex(s)

/**
 * @function cf_string_tobool
 * @category string
 * @brief    Parses the string as a boolean. Returns true if the string is `"true"`.
 * @param    s            The string.
 * @return   Returns the parsed `bool` value.
 * @remarks  Shortform: `stobool(s)`.
 * @related  cf_string_bool cf_string_toint
 */
#define cf_string_tobool(s) stobool(s)

/**
 * @function cf_string_replace
 * @category string
 * @brief    Replaces all occurrences of `replace_me` with `with_me` in the string.
 * @param    s            The string. Modified in-place.
 * @param    replace_me   The substring to find.
 * @param    with_me      The replacement substring.
 * @remarks  Shortform: `sreplace(s, replace_me, with_me)`.
 * @related  cf_string_find cf_string_erase cf_string_dedup
 */
#define cf_string_replace(s, replace_me, with_me) sreplace(s, replace_me, with_me)

/**
 * @function cf_string_dedup
 * @category string
 * @brief    Collapses consecutive runs of `ch` down to a single occurrence.
 * @param    s            The string. Modified in-place.
 * @param    ch           The character to deduplicate.
 * @remarks  Shortform: `sdedup(s, ch)`.
 * @related  cf_string_replace cf_string_trim
 */
#define cf_string_dedup(s, ch) sdedup(s, ch)

/**
 * @function cf_string_erase
 * @category string
 * @brief    Erases `count` characters starting at `index` from the string.
 * @param    s            The string. Modified in-place.
 * @param    index        The starting index.
 * @param    count        The number of characters to erase.
 * @remarks  Shortform: `serase(s, index, count)`.
 * @related  cf_string_replace cf_string_pop cf_string_clear
 */
#define cf_string_erase(s, index, count) serase(s, index, count)

/**
 * @function cf_string_pop
 * @category string
 * @brief    Removes the last character from the string.
 * @param    s            The string. Must be non-empty.
 * @remarks  Shortform: `spop(s)`.
 * @related  cf_string_pop_n cf_string_push cf_string_erase
 */
#define cf_string_pop(s) spop(s)

/**
 * @function cf_string_pop_n
 * @category string
 * @brief    Removes the last `n` characters from the string.
 * @param    s            The string.
 * @param    n            The number of characters to remove.
 * @remarks  Shortform: `spopn(s, n)`.
 * @related  cf_string_pop cf_string_erase
 */
#define cf_string_pop_n(s, n) spopn(s, n)

/**
 * @function cf_string_static
 * @category string
 * @brief    Creates a string with an initial static storage backing.
 * @param    s            The string. Can be `NULL`.
 * @param    buffer       Pointer to a static memory buffer.
 * @param    buffer_size  The size of `buffer` in bytes.
 * @remarks  Shortform: `sstatic(s, buffer, buffer_size)`. Will grow onto the heap if the size becomes too large. Call `sfree` when done.
 * @related  cf_string_is_dynamic cf_string_free cf_string_set
 */
#define cf_string_static(s, buffer, buffer_size) (astatic(s, buffer, buffer_size), apush(s, 0))

/**
 * @function cf_string_is_dynamic
 * @category string
 * @brief    Checks if a C string is a dynamic string from this API.
 * @param    s            The string. Can be `NULL`.
 * @return   Returns true if `s` is a dynamically allocated string.
 * @remarks  Shortform: `sisdyna(s)`.
 * @related  cf_string_static cf_string_free
 */
#define cf_string_is_dynamic(s) (!((#s)[0] == '"') && ck_avalid(s))

/**
 * @function cf_string_append_UTF8
 * @category string
 * @brief    Appends a UTF32 codepoint encoded as UTF8 onto the string.
 * @param    s            The string. Can be `NULL`.
 * @param    codepoint    An `int` codepoint in UTF32 form.
 * @remarks  Shortform: `sappend_UTF8(s, codepoint)`.
 * @related  cf_decode_UTF8 cf_decode_UTF16
 */
#define cf_string_append_UTF8(s, codepoint) sappend_UTF8(s, codepoint)

//--------------------------------------------------------------------------------------------------
// Longform Path C API.
// These map cf_path_* names to the shortform ckit path macros.

/**
 * @function cf_path_get_filename
 * @category path
 * @brief    Returns the filename portion of a path. Returns a new string.
 * @param    path         The path string.
 * @return   Returns a new dynamic string. Free with `sfree`.
 * @example  > Example fetching a filename from a path.
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 *     // Prints: big_gem.txt
 * @remarks  Shortform: `spfname(path)`.
 * @related  cf_path_get_filename_no_ext cf_path_get_ext cf_path_directory_of cf_path_normalize
 */
#define cf_path_get_filename(path) spfname(path)

/**
 * @function cf_path_get_filename_no_ext
 * @category path
 * @brief    Returns the filename portion of a path without the file extension. Returns a new string.
 * @param    path         The path string.
 * @return   Returns a new dynamic string. Free with `sfree`.
 * @remarks  Shortform: `spfname_no_ext(path)`.
 * @related  cf_path_get_filename cf_path_get_ext cf_path_ext_equ
 */
#define cf_path_get_filename_no_ext(path) spfname_no_ext(path)

/**
 * @function cf_path_get_ext
 * @category path
 * @brief    Returns the extension of the file for the given path (including the dot). Returns a new string.
 * @param    path         The path string.
 * @return   Returns a new dynamic string. Free with `sfree`.
 * @remarks  Shortform: `spext(path)`.
 * @related  cf_path_ext_equ cf_path_get_filename cf_path_get_filename_no_ext
 */
#define cf_path_get_ext(path) spext(path)

/**
 * @function cf_path_ext_equ
 * @category path
 * @brief    Returns true if the file's extension matches, false otherwise.
 * @param    path         The path string.
 * @param    ext          The file extension to compare (e.g. `".png"`).
 * @return   Returns true if the extensions match.
 * @remarks  Shortform: `spext_equ(path, ext)`.
 * @related  cf_path_get_ext cf_path_get_filename
 */
#define cf_path_ext_equ(path, ext) spext_equ(path, ext)

/**
 * @function cf_path_pop
 * @category path
 * @brief    Removes the rightmost file or directory from the path. Returns a new string.
 * @param    path         The path string.
 * @return   Returns a new dynamic string. Free with `sfree`.
 * @remarks  Shortform: `sppop(path)`.
 * @related  cf_path_pop_n cf_path_directory_of cf_path_top_directory cf_path_normalize
 */
#define cf_path_pop(path) sppop(path)

/**
 * @function cf_path_pop_n
 * @category path
 * @brief    Removes the rightmost `n` files or directories from the path. Returns a new string.
 * @param    path         The path string.
 * @param    n            The number of path components to pop.
 * @return   Returns a new dynamic string. Free with `sfree`.
 * @remarks  Shortform: `sppopn(path, n)`.
 * @related  cf_path_pop cf_path_directory_of
 */
#define cf_path_pop_n(path, n) sppopn(path, n)

/**
 * @function cf_path_compact
 * @category path
 * @brief    Squishes the path to be less than or equal to `n` characters in length. Returns a new string.
 * @param    path         The path string.
 * @param    n            Max character count (including nul-byte).
 * @return   Returns a new dynamic string. Free with `sfree`.
 * @remarks  Shortform: `spcompact(path, n)`. Inserts ellipses "..." as necessary. Useful for displaying paths in small UI boxes.
 * @related  cf_path_pop cf_path_pop_n
 */
#define cf_path_compact(path, n) spcompact(path, n)

/**
 * @function cf_path_directory_of
 * @category path
 * @brief    Returns the directory of a given file or directory. Returns a new string.
 * @param    path         The path string.
 * @return   Returns a new dynamic string. Free with `sfree`.
 * @remarks  Shortform: `spdir_of(path)`.
 * @related  cf_path_get_filename cf_path_top_directory cf_path_pop cf_path_normalize
 */
#define cf_path_directory_of(path) spdir_of(path)

/**
 * @function cf_path_top_directory
 * @category path
 * @brief    Returns the top-level directory of a given file or directory. Returns a new string.
 * @param    path         The path string.
 * @return   Returns a new dynamic string. Free with `sfree`.
 * @remarks  Shortform: `sptop_of(path)`.
 * @related  cf_path_directory_of cf_path_get_filename cf_path_pop
 */
#define cf_path_top_directory(path) sptop_of(path)

/**
 * @function cf_path_normalize
 * @category path
 * @brief    Normalizes a path as a new string.
 * @param    path         The path string.
 * @return   Returns a new dynamic string. Free with `sfree`.
 * @remarks  Shortform: `spnorm(path)`. All '\\' are replaced with '/'. Duplicate '////' become single '/'. Trailing '/' removed.
 *           Dot folders resolved. The first character is always '/', unless a windows drive (e.g. "C:/Users/Randy/Documents").
 * @related  cf_path_get_filename cf_path_directory_of cf_path_pop
 */
#define cf_path_normalize(path) spnorm(path)

//--------------------------------------------------------------------------------------------------
// Hidden API - Not intended for direct use.

// Aliases for code that uses the cf_ prefixed function names directly.
#define cf_sfit             ck_sfit
#define cf_sset             ck_sset
#define cf_sfmt             ck_sfmt
#define cf_sfmt_append      ck_sfmt_append
#define cf_svfmt            ck_svfmt
#define cf_svfmt_append     ck_svfmt_append
#define cf_sprefix          ck_sprefix
#define cf_ssuffix          ck_ssuffix
#define cf_stoupper         ck_stoupper
#define cf_stolower         ck_stolower
#define cf_sappend          ck_sappend
#define cf_sappend_range    ck_sappend_range
#define cf_strim            ck_strim
#define cf_sltrim           ck_sltrim
#define cf_srtrim           ck_srtrim
#define cf_slpad            ck_slpad
#define cf_srpad            ck_srpad
#define cf_ssplit_once      ck_ssplit_once
#define cf_ssplit           ck_ssplit
#define cf_sfirst_index_of  ck_sfirst_index_of
#define cf_slast_index_of   ck_slast_index_of
#define cf_stoint           ck_stoint
#define cf_stouint          ck_stouint
#define cf_stofloat         ck_stofloat
#define cf_stodouble        ck_stodouble
#define cf_stohex           ck_stohex
#define cf_sreplace         ck_sreplace
#define cf_sdedup           ck_sdedup
#define cf_serase           ck_serase
#define cf_spop             ck_spop
#define cf_spopn            ck_spopn
#define cf_string_append_UTF8_impl ck_sappend_UTF8

// Use ckit's intern structure for compatibility.
#define CF_INTERN_COOKIE CK_INTERN_COOKIE
typedef CK_UniqueString CF_Intern;

CF_API const char* CF_CALL cf_sintern(const char* s);
CF_API const char* CF_CALL cf_sintern_range(const char* start, const char* end);
CF_API void CF_CALL cf_sinuke_intern_table(void);

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
	CF_INLINE void set_len(int len) { sfit(m_str, len + 1); asetlen(m_str, len + 1); m_str[len] = 0; }
	CF_INLINE bool empty() const { return sempty(m_str); }

	CF_INLINE String& add(char ch) { spush(m_str, ch); return *this; }
	CF_INLINE String& append(const char* s) { sappend(m_str, s); return *this; }
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
	CF_INLINE Array<String> split(char split_c) { Array<String> r; char** s = ssplit(m_str, split_c); for (int i=0;i<asize(s);++i) r.add(cf_move(steal_from(s[i]))); afree(s); return r; }
	static CF_INLINE Array<String> split(const char* split_me, char split_c) { Array<String> r; char** s = ssplit(split_me, split_c); for (int i=0;i<asize(s);++i) r.add(cf_move(steal_from(s[i]))); afree(s); return r; }
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
	sdyna char* m_str = NULL;
	CF_INLINE void s_chki(int i) const { CF_ASSERT(i >= 0 && i < scount(m_str)); }
};

CF_INLINE char* operator+(const String& a, int i) { return (char*)a.c_str() + i; }
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
