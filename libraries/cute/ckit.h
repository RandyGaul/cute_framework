/*
    ckit -- A single-header kit of high-performance C essentials.

    In exactly one .c/.cpp file:
           #define CKIT_IMPLEMENTATION
           #include "ckit.h"
       In all other cases include as-expected:
           #include "ckit.h"

    QUICK START GUIDE

        1) Dynamic arrays:
            int* a = NULL;
            for (int i = 0; i < 10; ++i) {
                apush(a, i);
            }
            for (int i = 0; i < 10; ++i) {
                printf("%d\n", a[i]);
            }
            printf("len=%d cap=%d\n", acount(a), acap(a));
            afree(a);

        2) Dynamic strings:
            char* s = NULL;
            sset(s, "Hello ");
            sappend(s, "world!");
            printf("%s\n", s);
            sfree(s);

        3) Map (typed hashtable):
            CK_MAP(int) m = NULL;
            map_set(m, sintern("x"), 10);
            map_set(m, sintern("y"), 20);
            int x = map_get(m, sintern("x"));
            int y = map_get(m, sintern("y"));
            for (int i = 0; i < map_size(m); i++) {
                printf("key=%llu val=%d\n", map_keys(m)[i], map_items(m)[i]);
            }
            map_free(m);

        4) String interning:
           const char* a = sintern("hello");
           char buf[64];
           strcpy(buf, "he");
           strcat(buf, "llo");
           const char *b = sintern(buf);
           assert(a == b);

    Public domain - Do whatever you want with this code.
*/

#ifndef CKIT_H
#define CKIT_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ----------------------------------------------------------------------------------------------------
// Overrideable macros (define before including ckit.h to customize behavior):
//
//   CK_ALLOC(sz)       - Custom allocator. Default: malloc(sz)
//   CK_REALLOC(p, sz)  - Custom reallocator. Default: realloc(p, sz)
//   CK_FREE(p)         - Custom free. Default: free(p)

#ifndef CK_ALLOC
#	define CK_ALLOC(sz) malloc(sz)
#endif
#ifndef CK_REALLOC
#	define CK_REALLOC(p, sz) realloc(p, sz)
#endif
#ifndef CK_FREE
#	define CK_FREE(p) free(p)
#endif

//--------------------------------------------------------------------------------------------------
// Dynamic arrays (stretchy buffers).
//
// Declare as T* arr = NULL. Push elements with apush(). Access like a normal array.
// NULL is a valid empty array.
//
// Example:
//     int* a = NULL;
//     apush(a, 10);
//     apush(a, 20);
//     printf("%d %d\n", a[0], a[1]);  // 10 20
//     afree(a);

// Markup macros for documentation purposes. Expand to nothing.
#define CK_DYNA   // Annotates a pointer as a dynamic array.
#define CK_SDYNA  // Annotates a pointer as a dynamic string.

// asize/acount: Number of elements. Returns 0 for NULL.
#define asize(a)      ((a) ? CK_AHDR(a)->size : 0)
#define acount(a)     asize(a)

// asetlen: Set size directly (must be <= capacity). Does not allocate.
#define asetlen(a, n) (CK_AHDR(a)->size = (n))

// acap: Allocated capacity. Returns 0 for NULL.
#define acap(a)       ((a) ? CK_AHDR(a)->capacity : 0)

// afit: Ensure capacity for n elements. May reallocate.
#define afit(a, n)    ((n) <= acap(a) ? 0 : (*(void**)&(a) = ck_agrow((a), (n), sizeof(*(a)))))

// apush: Append element. May reallocate.
#define apush(a, ...) (CK_ACANARY(a), afit((a), 1 + asize(a)), (a)[CK_AHDR(a)->size++] = (__VA_ARGS__))

// apop: Remove and return last element. Array must not be empty.
#define apop(a)       ((a)[--CK_AHDR(a)->size])

// aend: Pointer one past the last element.
#define aend(a)       ((a) + asize(a))

// alast: Last element. Array must not be empty.
#define alast(a)      ((a)[asize(a) - 1])

// aclear: Set size to 0 but keep allocated memory.
#define aclear(a)     (CK_ACANARY(a), (a) ? CK_AHDR(a)->size = 0 : 0)

// adel: Remove element at index i by swapping with last (O(1), unordered).
#define adel(a, i)    ((a)[i] = (a)[--CK_AHDR(a)->size])

// astatic: Use a stack/static buffer instead of heap allocation.
//     char buffer[1024];
//     int* a = NULL;
//     astatic(a, buffer, sizeof(buffer));
#define astatic(a, buffer, buffer_size) (*(void**)&(a) = ck_astatic(buffer, buffer_size, sizeof(*(a))))

// aset: Copy array b into a.
#define aset(a, b)    (*(void**)&(a) = ck_aset((void*)(a), (void*)(b), sizeof(*(a))))

// arev: Reverse array in-place.
#define arev(a)       ((a) ? ck_arev(a, sizeof(*(a))) : (void*)0)

// ahash: Hash all bytes in the array using FNV1a.
#define ahash(a)      ((a) ? ck_hash_fnv1a(a, sizeof(*(a)) * asize(a)) : 0)

// afree: Free array memory and set pointer to NULL.
#define afree(a)      do { CK_ACANARY(a); if (a && !CK_AHDR(a)->is_static) CK_FREE(CK_AHDR(a)); (a) = NULL; } while (0)

// Check if array is a valid dynamic array.
#define avalid(a)  ((a) && CK_AHDR(a)->cookie.val == CK_ACOOKIE.val)

//--------------------------------------------------------------------------------------------------
// Dynamic strings (built on dynamic arrays).
//
// 100% compatible with normal C-strings. Free with sfree when done.
//
// To create a new string use smake or sfmake. To overwrite an existing string
// use sset or sfmt. The first argument to sset/sfmt must be an l-value (a
// variable), not a literal like NULL -- use smake/sfmake to create from scratch.
//
// Example:
//
//     char* s = smake("Hello world!");
//     printf("%s\n", s);
//     sset(s, "Goodbye!");
//     printf("%s\n", s);
//     sfree(s);

// slen: String length (not including null terminator). Returns 0 for NULL.
#define slen(s)                 ((s) ? (asize(s) ? asize(s) - 1 : asize(s)) : 0)

// scount: Internal array size (length + null terminator).
#define scount(s)               asize(s)

// scap: Allocated capacity.
#define scap(s)                 acap(s)

// sempty: True if string is NULL or has length 0.
#define sempty(s)               ((s) ? slen(s) < 1 : 1)

// sfirst/slast: First or last character. Returns '\0' for empty/NULL.
#define sfirst(s)               ((s) ? (s)[0] : '\0')
#define slast(s)                ((s) ? (s)[slen(s) - 1] : '\0')

// sclear: Clear string to empty but keep allocated memory.
#define sclear(s)               (aclear(s), apush(s, 0))

// sstatic: Create a string backed by a static buffer. Grows to heap if needed.
#define sstatic(s, buf, sz)     (astatic(s, buf, sz), apush(s, 0))

// sisdyna: True if s is a dynamic string from this API (not a literal or static buffer).
#define sisdyna(s)              (!((#s)[0] == '"') && ck_avalid(s))

// sfree: Free string memory and set pointer to NULL.
#define sfree(s)                afree(s)

// sfit: Ensure capacity for n characters.
#define sfit(s, n)              (s = ck_sfit(s, n))

// spush/spop: Append or remove a single character.
#define spush(s, ch)            do { if (!(s)) apush(s, ch); else (s)[slen(s)] = (ch); apush(s, 0); } while (0)
#define spop(s)                 (s = ck_spop(s))
#define spopn(s, n)             (s = ck_spopn(s, n))

// sset: Overwrite a with contents of b.
#define sset(a, b)              (a = ck_sset(a, b))

// sdup/smake: Create a new string (copies from source).
//     char* s = smake("hello");
#define sdup(s)                 ck_sset(NULL, s)
#define smake(s)                ck_sset(NULL, s)

// sfmake/sfmt: Printf-style string creation/formatting.
//     char* s = sfmake("x=%d", 42);  // Create new
//     sfmt(s, "x=%d", 99);           // Overwrite existing
#define sfmake(fmt, ...)        ck_sfmt(NULL, fmt, __VA_ARGS__)
#define sfmt(s, fmt, ...)       (s = ck_sfmt(s, fmt, __VA_ARGS__))
#define sfmt_append(s, fmt, ...) (s = ck_sfmt_append(s, fmt, __VA_ARGS__))
#define svfmt(s, fmt, args)     (s = ck_svfmt(s, fmt, args))
#define svfmt_append(s, fmt, args) (s = ck_svfmt_append(s, fmt, args))

// sappend/scat: Concatenate b onto a.
#define sappend(a, b)           (a = ck_sappend(a, b))
#define scat(a, b)              sappend(a, b)
#define sappend_range(a, b, e)  (a = ck_sappend_range(a, b, e))
#define scat_range(a, b, e)     sappend_range(a, b, e)

// Comparison. sequ/siequ are NULL-safe (NULL == NULL is true).
#define scmp(a, b)              strcmp(a, b)
#define sicmp(a, b)             ck_stricmp(a, b)
#define sequ(a, b)              ((a) == NULL && (b) == NULL ? 1 : ((a) == NULL || (b) == NULL ? 0 : !strcmp((a), (b))))
#define siequ(a, b)             ((a) == NULL && (b) == NULL ? 1 : ((a) == NULL || (b) == NULL ? 0 : !ck_stricmp((a), (b))))

// sprefix/ssuffix: Check if string starts/ends with given prefix/suffix.
#define sprefix(s, p)           ck_sprefix(s, p)
#define ssuffix(s, p)           ck_ssuffix(s, p)
#define scontains(s, sub)       (slen(s) >= (int)strlen(sub) && !!strstr(s, sub))

// sfirst_index_of/slast_index_of: Find character. Returns -1 if not found.
#define sfirst_index_of(s, ch)  ck_sfirst_index_of(s, ch)
#define slast_index_of(s, ch)   ck_slast_index_of(s, ch)

// sfind: Find substring. Returns pointer to match or NULL.
#define sfind(s, sub)           strstr(s, sub)

// stoupper/stolower: Convert case in-place.
#define stoupper(s)             ck_stoupper(s)
#define stolower(s)             ck_stolower(s)

// shash: FNV-1a hash of string.
#define shash(s)                ck_hash_fnv1a(s, slen(s))

// strim/sltrim/srtrim: Trim whitespace from both ends, left only, or right only.
#define strim(s)                (s = ck_strim(s))
#define sltrim(s)               (s = ck_sltrim(s))
#define srtrim(s)               (s = ck_srtrim(s))

// slpad/srpad: Pad string with n copies of ch on left or right.
#define slpad(s, ch, n)         (s = ck_slpad(s, ch, n))
#define srpad(s, ch, n)         (s = ck_srpad(s, ch, n))

// sreplace: Replace all occurrences of old with new.
#define sreplace(s, old, new)   (s = ck_sreplace(s, old, new))

// sdedup: Collapse consecutive runs of ch into single ch.
#define sdedup(s, ch)           (s = ck_sdedup(s, ch))

// serase: Remove n characters starting at idx.
#define serase(s, idx, n)       (s = ck_serase(s, idx, n))

// ssplit_once: Split at first occurrence of ch.
// Returns left part (new string). Modifies s to contain right part.
//     char* s = smake("a:b:c");
//     char* left = ssplit_once(s, ':');  // left="a", s="b:c"
#define ssplit_once(s, ch)      ck_ssplit_once(s, ch)

// ssplit: Split into array of strings. Caller must afree the array and each string.
//     char** parts = ssplit("a:b:c", ':');
//     for (int i = 0; i < acount(parts); ++i) printf("%s\n", parts[i]);
//     for (int i = 0; i < acount(parts); ++i) sfree(parts[i]);
//     afree(parts);
#define ssplit(s, ch)           ck_ssplit(s, ch)

// Parsing functions.
#define stoint(s)               ck_stoint(s)
#define stouint(s)              ck_stouint(s)
#define stofloat(s)             ck_stofloat(s)
#define stodouble(s)            ck_stodouble(s)
#define stohex(s)               ck_stohex(s)   // Accepts 0x or # prefix.
#define stobool(s)              (!strcmp(s, "true"))

// sappend_UTF8: Append a Unicode codepoint encoded as UTF-8.
#define sappend_UTF8(s, cp)     (s = ck_sappend_UTF8(s, cp))

// decode_UTF8: Decode one UTF-8 codepoint from s, store it in *codepoint.
// Returns a pointer past the consumed bytes. Invalid sequences yield 0xFFFD.
#define decode_UTF8(s, cp)      ck_decode_UTF8(s, cp)

// decode_UTF16: Decode one UTF-16 codepoint (with surrogate pair support) from s, store it in *codepoint.
// Returns a pointer past the consumed uint16_t(s). Invalid surrogates yield 0xFFFD.
#define decode_UTF16(s, cp)     ck_decode_UTF16(s, cp)

// String formatting from primitives.
#define sint(s, i)              sfmt(s, "%d", (int)(i))
#define suint(s, u)             sfmt(s, "%" PRIu64, (uint64_t)(u))
#define sfloat(s, f)            sfmt(s, "%f", (double)(f))
#define sdouble(s, f)           sfmt(s, "%f", (double)(f))
#define shex(s, u)              sfmt(s, "0x%x", (unsigned)(u))
#define sptr(s, p)              sfmt(s, "%p", (void*)(p))
#define sbool(s, b)             sfmt(s, "%s", (b) ? "true" : "false")

//--------------------------------------------------------------------------------------------------
// String path utilities.
//
// All return newly allocated strings (caller must sfree).
//
// Example:
//     char* ext = spext("file.txt");       // -> ".txt"
//     char* norm = spnorm("a/../b");       // -> "/b"
//     sfree(ext); sfree(norm);

// spfname: Get filename from path. "/usr/bin/app" -> "app"
#define spfname(s)              ck_spfname(s)

// spfname_no_ext: Get filename without extension. "/usr/bin/app.exe" -> "app"
#define spfname_no_ext(s)       ck_spfname_no_ext(s)

// spext: Get file extension including dot. "file.txt" -> ".txt"
#define spext(s)                ck_spext(s)

// spext_equ: Check if path has given extension. spext_equ("file.txt", ".txt") -> 1
#define spext_equ(s, ext)       ck_spext_equ(s, ext)

// sppop: Remove last path component. "/a/b/c" -> "/a/b"
#define sppop(s)                ck_sppop(s)

// sppopn: Remove last n path components.
#define sppopn(s, n)            ck_sppopn(s, n)

// spcompact: Shorten path to fit n characters using "...".
#define spcompact(s, n)         ck_spcompact(s, n)

// spdir_of: Get parent directory. "/a/b/c" -> "/b"
#define spdir_of(s)             ck_spdir_of(s)

// sptop_of: Get top-level directory after root.
#define sptop_of(s)             ck_sptop_of(s)

// spnorm: Normalize path. Resolves "..", converts \ to /.
#define spnorm(s)               ck_spnorm(s)

//--------------------------------------------------------------------------------------------------
// Map (typed hashtable) - Stretchy-buffer style.
//
// CK_MAP(T) resolves to T*. NULL is a valid empty map. Keys are always uint64_t.
// For string keys, use sintern() to get a unique pointer and cast to uint64_t.
// Keys and values are stored in parallel dense arrays for fast iteration.
//
// Example:
//     CK_MAP(int) scores = NULL;
//     map_set(scores, sintern("player1"), 100);
//     map_set(scores, sintern("player2"), 200);
//     int p1 = map_get(scores, sintern("player1"));  // 100
//
//     // Iteration:
//     for (int i = 0; i < map_size(scores); i++) {
//         uint64_t key = map_keys(scores)[i];
//         int val = scores[i];  // or map_items(scores)[i]
//     }
//     map_free(scores);

// CK_MAP(T): Declares a map type. Just T* under the hood.
#define CK_MAP(T) T*

// map_size: Number of {key, value} pairs. Returns 0 for NULL.
#define map_size(m)     (map_validate(m), (m) ? CK_MHDR(m)->size : 0)

// map_capacity: Allocated capacity.
#define map_capacity(m) (map_validate(m), (m) ? CK_MHDR(m)->capacity : 0)

// map_keys: Pointer to keys array (uint64_t*). Parallel to items array.
#define map_keys(m)  (map_validate(m), (m) ? ck_map_keys_ptr(CK_MHDR(m)) : NULL)

// map_items: Pointer to items array. Same as the map pointer itself.
#define map_items(m) (map_validate(m), (m))

// map_has: Check if key exists.
#define map_has(m, k) ( \
	map_validate(m), \
	(m) && ck_map_find_impl(CK_MHDR(m), (uint64_t)(k)) >= 0)

// map_del: Remove entry by key. Returns 1 if deleted, 0 if not found.
#define map_del(m, k) ( \
	map_validate(m), \
	(m) ? ck_map_del_impl(CK_MHDR(m), (uint64_t)(k)) : 0)

// map_clear: Remove all entries but keep allocated memory.
#define map_clear(m) do { \
	map_validate(m); \
	if (m) ck_map_clear_impl(CK_MHDR(m)); \
} while(0)

// map_free: Free all memory and set pointer to NULL.
#define map_free(m) do { \
	if (m) { map_validate(m); ck_map_free_impl(CK_MHDR(m)); } \
	(m) = NULL; \
} while(0)

// map_find: Alias for map_get.
#define map_find(m, k) map_get(m, k)

// map_key/map_val: Access key or value at index i.
#define map_key(m, i)  (map_keys(m)[i])
#define map_val(m, i)  (map_items(m)[i])

// map_sort: Sort entries by values. Comparator receives pointers to values.
//     int cmp(const void* a, const void* b) { return *(int*)a - *(int*)b; }
//     map_sort(m, cmp);
#define map_sort(m, cmp) do { \
	map_validate(m); \
	if (m) ck_map_sort_impl(CK_MHDR(m), cmp); \
} while(0)

// map_ssort: Sort entries by keys (treating keys as interned string pointers).
//     map_ssort(m, 0);  // case-sensitive
//     map_ssort(m, 1);  // case-insensitive
#define map_ssort(m, ignore_case) do { \
	map_validate(m); \
	if (m) ck_map_ssort_impl(CK_MHDR(m), ignore_case); \
} while(0)

// map_swap: Swap entries at indices i and j.
#define map_swap(m, i, j) do { \
	map_validate(m); \
	if (m) ck_map_swap_impl(CK_MHDR(m), i, j); \
} while(0)

//--------------------------------------------------------------------------------------------------
// C/C++ specific map macros (type inference differs).
//
// map_get: Get value by key. Returns zero-initialized value if not found.
// map_get_ptr: Get pointer to value. Returns NULL if not found.
// map_set: Set value for key. Creates entry if not exists.
// map_add: Alias for map_set with uint64_t value (backwards compat).

#ifdef __cplusplus
#	include <type_traits>
#	define map_get(m, k) (map_validate(m), (m) && ck_map_find_impl(CK_MHDR(m), (uint64_t)(k)) >= 0 ? (m)[ck_map_find_impl(CK_MHDR(m), (uint64_t)(k))] : std::remove_pointer_t<std::remove_reference_t<decltype(m)>>{})
#	define map_get_ptr(m, k) (map_validate(m), (m) ? (std::remove_reference_t<decltype(m)>)ck_map_get_ptr_impl(CK_MHDR(m), (uint64_t)(k)) : nullptr)
#	define map_set(m, k, v) do { std::remove_pointer_t<decltype(m)> ck_v_ = (v); ck_map_set_stretchy((void**)&(m), (uint64_t)(k), &ck_v_, sizeof(ck_v_)); map_validate(m); } while(0)
#	define map_add(m, k, v) do { uint64_t ck_v_ = (uint64_t)(v); ck_map_set_stretchy((void**)&(m), (uint64_t)(k), &ck_v_, sizeof(ck_v_)); map_validate(m); } while(0)
#else

// map_get: Get value by key. Returns zero-initialized value if not found.
//     int x = map_get(m, sintern("x"));
#define map_get(m, k) ( \
	map_validate(m), \
	(m) && ck_map_find_impl(CK_MHDR(m), (uint64_t)(k)) >= 0 \
		? (m)[ck_map_find_impl(CK_MHDR(m), (uint64_t)(k))] \
		: (typeof(*(m))){ 0 })

// map_get_ptr: Get pointer to value. Returns NULL if not found.
//     int* px = map_get_ptr(m, sintern("x"));
//     if (px) *px = 999;
#define map_get_ptr(m, k) ( \
	map_validate(m), \
	(m) ? (typeof(m))ck_map_get_ptr_impl(CK_MHDR(m), (uint64_t)(k)) : NULL)

// map_set: Set value for key. Creates entry if not exists. May reallocate.
//     map_set(m, sintern("x"), 42);
#define map_set(m, k, v) do { \
	typeof(*(m)) ck_v_ = (v); \
	ck_map_set_stretchy((void**)&(m), (uint64_t)(k), &ck_v_, sizeof(ck_v_)); \
	map_validate(m); \
} while(0)

// map_add: Alias for map_set with uint64_t value (backwards compat).
#define map_add(m, k, v) do { \
	uint64_t ck_v_ = (uint64_t)(v); \
	ck_map_set_stretchy((void**)&(m), (uint64_t)(k), &ck_v_, sizeof(ck_v_)); \
	map_validate(m); \
} while(0)

#endif

//--------------------------------------------------------------------------------------------------
// String interning.
//
// Returns a unique, stable pointer for each unique string.
// Interned strings can be compared by pointer (==) instead of strcmp.
// Great as map keys: cast the pointer to uint64_t.
// Also reduces memory by deduplicating identical strings.
//
// Example:
//     const char* a = sintern("hello");
//     char buf[64]; strcpy(buf, "hel"); strcat(buf, "lo");
//     const char* b = sintern(buf);
//     assert(a == b);  // Same pointer!
//
//     // Use as map key:
//     CK_MAP(int) m = NULL;
//     map_set(m, sintern("x"), 10);
//     int x = map_get(m, sintern("x"));  // 10

// sintern: Return interned string. Same contents always returns same pointer.
#define sintern(s) ck_sintern(s)

// sintern_range: Intern a substring [start, end).
#define sintern_range(start, end) ck_sintern_range(start, end)

// sivalid: True if s is an interned string (from sintern).
#define sivalid(s) (((CK_UniqueString*)(s) - 1)->cookie.val == CK_INTERN_COOKIE.val)

// silen: Length of an interned string (constant-time).
#define silen(s)   (((CK_UniqueString*)(s) - 1)->len)

// sinuke: Free all interned strings. All previous pointers become invalid.
#define sinuke()   sintern_nuke()

//--------------------------------------------------------------------------------------------------
// Private implementation details.

// Cookie union for debugger-friendly viewing of 4-char magic values.
typedef union CK_Cookie
{
	uint32_t val;
	char c[4];
} CK_Cookie;

// Helper to create cookies (works in both C and C++).
static inline CK_Cookie ck_cookie(char a, char b, char c, char d)
{
	CK_Cookie ck;
	ck.c[0] = a;
	ck.c[1] = b;
	ck.c[2] = c;
	ck.c[3] = d;
	return ck;
}

// Portable case-insensitive string compare.
#ifdef _WIN32
#	define ck_stricmp _stricmp
#else
#	define ck_stricmp strcasecmp
#endif

// Intern structure for validation and length access.
#define CK_INTERN_COOKIE ck_cookie('I','N','T','R')
typedef struct CK_UniqueString
{
	CK_Cookie cookie;
	int len;
	struct CK_UniqueString* next;
	char* str;
} CK_UniqueString;

// Hidden array header behind the user pointer.
typedef struct CK_ArrayHeader
{
	int size;
	int capacity;
	int is_static;
	char* data;
	CK_Cookie cookie;
} CK_ArrayHeader;

#define CK_AHDR(a)    ((CK_ArrayHeader*)(a) - 1)
#define CK_ACOOKIE    ck_cookie('A','R','R','Y')
#define CK_ACANARY(a) ((a) ? assert(CK_AHDR(a)->cookie.val == CK_ACOOKIE.val) : (void)0)

#ifndef CK_API
#define CK_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

CK_API void sintern_nuke();

CK_API void* ck_agrow(const void* a, int new_size, size_t element_size);
CK_API void* ck_astatic(const void* a, int buffer_size, size_t element_size);
CK_API void* ck_aset(const void* a, const void* b, size_t element_size);
CK_API void* ck_arev(const void* a, size_t element_size);

typedef struct CK_MapSlot
{
	uint64_t h;
	int item_index;
	int base_count;
} CK_MapSlot;

// Safety cookie for map validation.
#define CK_MAP_COOKIE ck_cookie('M','A','P','!')

// Compute pointers to arrays within the single allocation.
// items: right after header (header is 24 bytes, 8-byte aligned)
#define ck_map_items_ptr(hdr) ((void*)((hdr) + 1))

// keys: after items (aligned to 8)
#define ck_map_keys_offset(cap, vsz) (sizeof(CK_MapHeader) + CK_ALIGN8((size_t)(cap) * (vsz)))
#define ck_map_keys_ptr(hdr) ((uint64_t*)((char*)(hdr) + ck_map_keys_offset((hdr)->capacity, (hdr)->val_size)))

// islot: after keys (already 8-byte aligned since keys are uint64_t)
#define ck_map_islot_ptr(hdr) ((int*)(ck_map_keys_ptr(hdr) + (hdr)->capacity))

// slots: after islot (aligned to 8 for CK_MapSlot)
#define ck_map_slots_offset(hdr) CK_ALIGN8((size_t)((char*)(ck_map_islot_ptr(hdr) + (hdr)->capacity) - (char*)(hdr)))
#define ck_map_slots_ptr(hdr) ((CK_MapSlot*)((char*)(hdr) + ck_map_slots_offset(hdr)))

// Internal: Get header pointer from map pointer.
#define CK_MHDR(m) ((m) ? (CK_MapHeader*)((char*)(m) - sizeof(CK_MapHeader)) : NULL)

// Internal: Validate map cookie (catches use-after-free, etc).
#define map_validate(m) ((void)(!(m) || (assert(CK_MHDR(m)->cookie.val == CK_MAP_COOKIE.val), 1)))

// Map header stored just before the values array.
// All arrays (items, keys, islot, slots) are in a single allocation following the header.
// Layout: [Header][items...][keys...][islot...][slots...]
typedef struct CK_MapHeader
{
	CK_Cookie    cookie;         // Safety cookie for validation
	int          val_size;       // Size of each value in bytes
	int          size;           // Number of items
	int          capacity;       // Capacity for items/keys/islot
	int          slot_count;     // Number of used hash slots
	int          slot_capacity;  // Capacity for hash slots (power of 2)
} CK_MapHeader;

// Alignment helper.
#define CK_ALIGN8(x) (((x) + 7) & ~(size_t)7)

// Map implementation functions.
CK_API void  ck_map_set_stretchy(void** m_ptr, uint64_t key, const void* val, int val_size);
CK_API CK_MapHeader* ck_map_ensure_capacity(void** m_ptr, int want_items, int val_size);
CK_API int   ck_map_find_impl(CK_MapHeader* hdr, uint64_t key);
CK_API void* ck_map_get_ptr_impl(CK_MapHeader* hdr, uint64_t key);
CK_API int   ck_map_del_impl(CK_MapHeader* hdr, uint64_t key);
CK_API void  ck_map_clear_impl(CK_MapHeader* hdr);
CK_API void  ck_map_free_impl(CK_MapHeader* hdr);
CK_API void  ck_map_swap_impl(CK_MapHeader* hdr, int i, int j);
CK_API void  ck_map_sort_impl(CK_MapHeader* hdr, int (*cmp)(const void* a, const void* b));
CK_API void  ck_map_ssort_impl(CK_MapHeader* hdr, int ignore_case);

// String implementation functions.
CK_API char* ck_sfit(char* a, int n);
CK_API char* ck_sset(char* a, const char* b);
CK_API char* ck_sfmt(char* s, const char* fmt, ...);
CK_API char* ck_sfmt_append(char* s, const char* fmt, ...);
CK_API char* ck_svfmt(char* s, const char* fmt, va_list args);
CK_API char* ck_svfmt_append(char* s, const char* fmt, va_list args);
CK_API int   ck_sprefix(const char* s, const char* prefix);
CK_API int   ck_ssuffix(const char* s, const char* suffix);
CK_API void  ck_stoupper(char* s);
CK_API void  ck_stolower(char* s);
CK_API char* ck_sappend(char* a, const char* b);
CK_API char* ck_sappend_range(char* a, const char* b, const char* b_end);
CK_API char* ck_strim(char* s);
CK_API char* ck_sltrim(char* s);
CK_API char* ck_srtrim(char* s);
CK_API char* ck_slpad(char* s, char pad, int count);
CK_API char* ck_srpad(char* s, char pad, int count);
CK_API char* ck_ssplit_once(char* s, char split_c);
CK_API char** ck_ssplit(const char* s, char split_c);
CK_API int   ck_sfirst_index_of(const char* s, char c);
CK_API int   ck_slast_index_of(const char* s, char c);
CK_API int   ck_stoint(const char* s);
CK_API uint64_t ck_stouint(const char* s);
CK_API float ck_stofloat(const char* s);
CK_API double ck_stodouble(const char* s);
CK_API uint64_t ck_stohex(const char* s);
CK_API char* ck_sreplace(char* s, const char* replace_me, const char* with_me);
CK_API char* ck_sdedup(char* s, int ch);
CK_API char* ck_serase(char* s, int index, int count);
CK_API char* ck_spop(char* s);
CK_API char* ck_spopn(char* s, int n);
CK_API char* ck_sappend_UTF8(char* s, int codepoint);
CK_API const char* ck_decode_UTF8(const char* s, int* codepoint);

// Path implementation functions.
CK_API char* ck_spfname(const char* path);
CK_API char* ck_spfname_no_ext(const char* path);
CK_API char* ck_spext(const char* path);
CK_API int   ck_spext_equ(const char* path, const char* ext);
CK_API char* ck_sppop(const char* path);
CK_API char* ck_sppopn(const char* path, int n);
CK_API char* ck_spcompact(const char* path, int n);
CK_API char* ck_spdir_of(const char* path);
CK_API char* ck_sptop_of(const char* path);
CK_API char* ck_spnorm(const char* path);

// UTF16 decode.
#include <stdint.h>
CK_API const uint16_t* ck_decode_UTF16(const uint16_t* s, int* codepoint);

// Intern implementation.
CK_API const char* ck_sintern_range(const char* start, const char* end);
CK_API uint64_t ck_hash_fnv1a(const void* ptr, size_t sz);

static inline const char* ck_sintern(const char* s)
{
	return ck_sintern_range(s, s + strlen(s));
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CKIT_H

//--------------------------------------------------------------------------------------------------

#ifdef CKIT_IMPLEMENTATION

#ifndef CKIT_IMPLEMENTATION_GUARD
#define CKIT_IMPLEMENTATION_GUARD

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------------------------
// Array.

void* ck_agrow(const void* a, int new_size, size_t element_size)
{
	CK_ACANARY(a);
	assert(acap(a) <= (SIZE_MAX - 1) / 2);
	int new_capacity = 2 * acap(a);
	int min_cap = new_size > 16 ? new_size : 16;
	if (new_capacity < min_cap) new_capacity = min_cap;
	assert(new_size <= new_capacity);
	assert((size_t)new_capacity <= (SIZE_MAX - sizeof(CK_ArrayHeader)) / element_size);
	size_t total_size = sizeof(CK_ArrayHeader) + (size_t)new_capacity * element_size;
	CK_ArrayHeader* hdr;
	if (a) {
		if (!CK_AHDR(a)->is_static) {
			hdr = (CK_ArrayHeader*)CK_REALLOC(CK_AHDR(a), total_size);
		} else {
			hdr = (CK_ArrayHeader*)CK_ALLOC(total_size);
			memcpy(hdr + 1, a, (size_t)asize(a) * element_size);
			hdr->size = asize(a);
			hdr->cookie = CK_ACOOKIE;
		}
	} else {
		hdr = (CK_ArrayHeader*)CK_ALLOC(total_size);
		hdr->size = 0;
		hdr->cookie = CK_ACOOKIE;
	}
	hdr->capacity = new_capacity;
	hdr->is_static = 0;
	hdr->data = (char*)(hdr + 1);
	return (void*)(hdr + 1);
}

void* ck_astatic(const void* a, int buffer_size, size_t element_size)
{
	CK_ArrayHeader* hdr = (CK_ArrayHeader*)a;
	hdr->size = 0;
	hdr->cookie = CK_ACOOKIE;
	if (sizeof(CK_ArrayHeader) <= element_size) {
		hdr->capacity = buffer_size / (int)element_size - 1;
	} else {
		int elements_taken = (int)sizeof(CK_ArrayHeader) / (int)element_size + ((int)sizeof(CK_ArrayHeader) % (int)element_size > 0);
		hdr->capacity = buffer_size / (int)element_size - elements_taken;
	}
	hdr->data = (char*)(hdr + 1);
	hdr->is_static = 1;
	return (void*)(hdr + 1);
}

void* ck_aset(const void* a, const void* b, size_t element_size)
{
	CK_ACANARY(a);
	if (!b) {
		aclear(a);
		return (void*)a;
	}
	CK_ACANARY(b);
	if (acap(a) < asize(b)) {
		a = ck_agrow(a, asize(b), element_size);
	}
	memcpy((void*)a, b, (size_t)asize(b) * element_size);
	if (a) asetlen(a, asize(b));
	else assert(!asize(b));
	return (void*)a;
}

void* ck_arev(const void* a_ptr, size_t element_size)
{
	CK_ACANARY(a_ptr);
	char* a = (char*)a_ptr;
	int n = acount(a_ptr);
	if (n <= 1) return (void*)a_ptr;
	char* b = a + element_size * (n - 1);
	char tmp[256];
	char* t = element_size <= sizeof(tmp) ? tmp : (char*)CK_ALLOC(element_size);
	while (a < b) {
		memcpy(t, a, element_size);
		memcpy(a, b, element_size);
		memcpy(b, t, element_size);
		a += element_size;
		b -= element_size;
	}
	if (t != tmp) CK_FREE(t);
	return (void*)a_ptr;
}

//--------------------------------------------------------------------------------------------------
// Strings.

char* ck_sfit(char* a, int n)
{
	afit(a, n + 1);
	if (scount(a) == 0) apush(a, 0);
	return a;
}

char* ck_sset(char* a, const char* b)
{
	CK_ACANARY(a);
	if (!b) return NULL;
	int bsize = (int)strlen(b) + 1;
	if (acap(a) < bsize) {
		a = (char*)ck_agrow(a, bsize, 1);
	}
	memcpy(a, b, (size_t)bsize);
	asetlen(a, bsize);
	return a;
}

char* ck_sfmt(char* s, const char* fmt, ...)
{
	CK_ACANARY(s);
	va_list args;
	va_start(args, fmt);
	int n = 1 + vsnprintf(s, (size_t)scap(s), fmt, args);
	va_end(args);
	if (n > scap(s)) {
		sfit(s, n);
		va_start(args, fmt);
		n = 1 + vsnprintf(s, (size_t)scap(s), fmt, args);
		va_end(args);
	}
	asetlen(s, n);
	return s;
}

char* ck_sfmt_append(char* s, const char* fmt, ...)
{
	CK_ACANARY(s);
	va_list args;
	va_start(args, fmt);
	int nul = !!s;
	int capacity = scap(s) - scount(s);
	int n = 1 + vsnprintf(s + slen(s), (size_t)(capacity > 0 ? capacity : 0), fmt, args);
	va_end(args);
	if (n > capacity) {
		afit(s, n + scount(s));
		va_start(args, fmt);
		int new_capacity = scap(s) - scount(s);
		n = 1 + vsnprintf(s + slen(s), (size_t)new_capacity, fmt, args);
		assert(n <= new_capacity);
		va_end(args);
	}
	asetlen(s, asize(s) + n - nul);
	return s;
}

char* ck_svfmt(char* s, const char* fmt, va_list args)
{
	CK_ACANARY(s);
	va_list copy_args;
	va_copy(copy_args, args);
	int n = 1 + vsnprintf(s, (size_t)scap(s), fmt, args);
	if (n > scap(s)) {
		sfit(s, n);
		n = 1 + vsnprintf(s, (size_t)scap(s), fmt, copy_args);
	}
	va_end(copy_args);
	asetlen(s, n);
	return s;
}

char* ck_svfmt_append(char* s, const char* fmt, va_list args)
{
	CK_ACANARY(s);
	va_list copy_args;
	va_copy(copy_args, args);
	int nul = !!s;
	int capacity = scap(s) - scount(s);
	int n = 1 + vsnprintf(s + slen(s), (size_t)(capacity > 0 ? capacity : 0), fmt, copy_args);
	va_end(copy_args);
	if (n > capacity) {
		afit(s, n + scount(s));
		int new_capacity = scap(s) - scount(s);
		n = 1 + vsnprintf(s + slen(s), (size_t)new_capacity, fmt, args);
		assert(n <= new_capacity);
	}
	asetlen(s, asize(s) + n - nul);
	return s;
}

int ck_sprefix(const char* s, const char* prefix)
{
	if (!s) return 0;
	int prefix_len = (int)strlen(prefix);
	return slen(s) >= prefix_len && !memcmp(s, prefix, (size_t)prefix_len);
}

int ck_ssuffix(const char* s, const char* suffix)
{
	if (!s) return 0;
	int suffix_len = (int)strlen(suffix);
	return slen(s) >= suffix_len && !memcmp(s + slen(s) - suffix_len, suffix, (size_t)suffix_len);
}

void ck_stoupper(char* s)
{
	if (!s) return;
	for (int i = 0; i < slen(s); ++i) s[i] = (char)toupper((unsigned char)s[i]);
}

void ck_stolower(char* s)
{
	if (!s) return;
	for (int i = 0; i < slen(s); ++i) s[i] = (char)tolower((unsigned char)s[i]);
}

char* ck_sappend(char* a, const char* b)
{
	CK_ACANARY(a);
	int blen = (int)strlen(b);
	if (blen <= 0) return a;
	sfit(a, slen(a) + blen + 1);
	memcpy(a + slen(a), b, (size_t)blen);
	asetlen(a, asize(a) + blen);
	a[slen(a)] = 0;
	return a;
}

char* ck_sappend_range(char* a, const char* b, const char* b_end)
{
	CK_ACANARY(a);
	int blen = (int)(b_end - b);
	if (blen <= 0) return a;
	sfit(a, slen(a) + blen + 1);
	memcpy(a + slen(a), b, (size_t)blen);
	asetlen(a, asize(a) + blen);
	a[slen(a)] = 0;
	return a;
}

char* ck_strim(char* s)
{
	CK_ACANARY(s);
	int len = slen(s);
	if (len <= 0) return s;
	char* start = s;
	char* end = s + len - 1;
	while (start <= end && isspace((unsigned char)*start)) start++;
	while (end >= start && isspace((unsigned char)*end)) end--;
	int new_len = (int)(end >= start ? (end - start + 1) : 0);
	if (new_len > 0) memmove(s, start, (size_t)new_len);
	s[new_len] = 0;
	asetlen(s, new_len + 1);
	return s;
}

char* ck_sltrim(char* s)
{
	CK_ACANARY(s);
	int len = slen(s);
	if (len <= 0) return s;
	char* start = s;
	char* end = s + len - 1;
	while (start <= end && isspace((unsigned char)*start)) start++;
	int new_len = (int)(end >= start ? (end - start + 1) : 0);
	if (new_len > 0) memmove(s, start, (size_t)new_len);
	s[new_len] = 0;
	asetlen(s, new_len + 1);
	return s;
}

char* ck_srtrim(char* s)
{
	CK_ACANARY(s);
	int len = slen(s);
	if (len <= 0) return s;
	char* end = s + len - 1;
	while (end >= s && isspace((unsigned char)*end)) --end;
	int new_len = (int)(end - s + 1);
	if (new_len < 0) new_len = 0;
	s[new_len] = 0;
	asetlen(s, new_len + 1);
	return s;
}

char* ck_slpad(char* s, char pad, int count)
{
	CK_ACANARY(s);
	sfit(s, scount(s) + count);
	memmove(s + count, s, (size_t)scount(s));
	memset(s, pad, (size_t)count);
	asetlen(s, asize(s) + count);
	return s;
}

char* ck_srpad(char* s, char pad, int count)
{
	CK_ACANARY(s);
	sfit(s, scount(s) + count);
	memset(s + slen(s), pad, (size_t)count);
	asetlen(s, asize(s) + count);
	s[slen(s)] = 0;
	return s;
}

char* ck_ssplit_once(char* s, char split_c)
{
	CK_ACANARY(s);
	char* start = s;
	char* end = s + slen(s);
	while (start < end) {
		if (*start == split_c) break;
		++start;
	}
	if (start == end) return NULL; // Delimiter not found.
	int len = (int)(start - s);
	char* split = NULL;
	sfit(split, len + 1);
	asetlen(split, len + 1);
	memcpy(split, s, (size_t)len);
	split[len] = 0;
	int new_len = slen(s) - len - 1;
	memmove(s, s + len + 1, (size_t)new_len);
	asetlen(s, new_len + 1);
	s[new_len] = 0;
	return split;
}

char** ck_ssplit(const char* s, char split_c)
{
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

int ck_sfirst_index_of(const char* s, char c)
{
	if (!s) return -1;
	const char* p = strchr(s, c);
	if (!p) return -1;
	return (int)(p - s);
}

int ck_slast_index_of(const char* s, char c)
{
	if (!s) return -1;
	const char* p = strrchr(s, c);
	if (!p) return -1;
	return (int)(p - s);
}

int ck_stoint(const char* s)
{
	char* end;
	long long result = strtoll(s, &end, 10);
	return (int)result;
}

uint64_t ck_stouint(const char* s)
{
	char* end;
	uint64_t result = (uint64_t)strtoll(s, &end, 10);
	return result;
}

float ck_stofloat(const char* s)
{
	char* end;
	double result = strtod(s, &end);
	return (float)result;
}

double ck_stodouble(const char* s)
{
	char* end;
	double result = strtod(s, &end);
	return result;
}

uint64_t ck_stohex(const char* s)
{
	if (!strncmp(s, "#", 1)) s += 1;
	if (!strncmp(s, "0x", 2)) s += 2;
	int len = (int)strlen(s);
	if (len != 6 && len != 8) return 0;
	char* end;
	uint64_t result = (uint64_t)strtoll(s, &end, 16);
	return len == 6 ? ((result << 8) | 0xFF) : result;
}

char* ck_sreplace(char* s, const char* replace_me, const char* with_me)
{
	CK_ACANARY(s);
	if (!s) return NULL;
	size_t replace_len = strlen(replace_me);
	if (replace_len == 0) return s; // Empty pattern: nothing to replace.
	size_t with_len = strlen(with_me);
	char* find;
	char* search = s;
	while ((find = strstr(search, replace_me))) {
		int find_offset = (int)(find - s);
		if (replace_len > with_len) {
			int remaining = scount(s) - find_offset - (int)replace_len;
			int diff = (int)(replace_len - with_len);
			memcpy(find, with_me, with_len);
			memmove(find + with_len, find + replace_len, (size_t)remaining);
			asetlen(s, asize(s) - diff);
		} else {
			int remaining = scount(s) - find_offset - (int)replace_len;
			int diff = (int)(with_len - replace_len);
			sfit(s, scount(s) + diff);
			find = s + find_offset;
			memmove(find + with_len, find + replace_len, (size_t)remaining);
			memcpy(find, with_me, with_len);
			asetlen(s, asize(s) + diff);
		}
		search = find + with_len;
	}
	return s;
}

char* ck_sdedup(char* s, int ch)
{
	CK_ACANARY(s);
	if (!s) return NULL;
	int len = (int)strlen(s);
	if (len <= 1) return s; // Empty or single char: nothing to dedup.
	int i = 0, j = 1;
	int dup = 0;
	while (j < len) {
		if (s[i] == ch && s[j] == ch) {
			dup = 1;
			++j;
		} else {
			++i;
			if (dup) s[i] = s[j];
			++j;
		}
	}
	s[i + 1] = 0;
	asetlen(s, i + 2);
	return s;
}

char* ck_serase(char* s, int index, int count)
{
	CK_ACANARY(s);
	if (!s) return NULL;
	if (index < 0) {
		count += index;
		index = 0;
		if (count <= 0) return s;
	}
	if (index >= slen(s)) return s;
	if (index + count >= slen(s)) {
		asetlen(s, index + 1);
		s[index] = 0;
		return s;
	} else {
		int remaining = scount(s) - (count + index);
		memmove(s + index, s + count + index, (size_t)remaining);
		asetlen(s, asize(s) - count);
	}
	return s;
}

char* ck_spop(char* s)
{
	CK_ACANARY(s);
	if (s && slen(s)) { asetlen(s, asize(s) - 1); s[slen(s)] = 0; }
	return s;
}

char* ck_spopn(char* s, int n)
{
	CK_ACANARY(s);
	if (!s || n < 0) return s;
	while (scount(s) > 1 && n--) asetlen(s, asize(s) - 1);
	s[slen(s)] = 0;
	return s;
}

char* ck_sappend_UTF8(char* s, int codepoint)
{
	CK_ACANARY(s);
	if (codepoint > 0x10FFFF) codepoint = 0xFFFD;
#define CK_EMIT(X, Y, Z) spush(s, (char)(X | ((codepoint >> Y) & Z)))
	     if (codepoint <    0x80) { CK_EMIT(0x00,0,0x7F); }
	else if (codepoint <   0x800) { CK_EMIT(0xC0,6,0x1F); CK_EMIT(0x80, 0,  0x3F); }
	else if (codepoint < 0x10000) { CK_EMIT(0xE0,12,0xF); CK_EMIT(0x80, 6,  0x3F); CK_EMIT(0x80, 0, 0x3F); }
	else                          { CK_EMIT(0xF0,18,0x7); CK_EMIT(0x80, 12, 0x3F); CK_EMIT(0x80, 6, 0x3F); CK_EMIT(0x80, 0, 0x3F); }
#undef CK_EMIT
	return s;
}

const char* ck_decode_UTF8(const char* s, int* codepoint)
{
	unsigned char c = (unsigned char)*s++;
	int extra = 0, min = 0;
	*codepoint = 0;
	     if (c >= 0xF0) { *codepoint = c & 0x07; extra = 3; min = 0x10000; }
	else if (c >= 0xE0) { *codepoint = c & 0x0F; extra = 2; min = 0x800; }
	else if (c >= 0xC0) { *codepoint = c & 0x1F; extra = 1; min = 0x80; }
	else if (c >= 0x80) { *codepoint = 0xFFFD; }
	else *codepoint = c;
	while (extra--) {
		c = (unsigned char)*s++;
		if ((c & 0xC0) != 0x80) { *codepoint = 0xFFFD; }
		if (*codepoint != 0xFFFD) { *codepoint = ((*codepoint) << 6) | (c & 0x3F); }
	}
	if (*codepoint < min) *codepoint = 0xFFFD;
	return s;
}

//--------------------------------------------------------------------------------------------------
// String path utilities.

char* ck_spfname(const char* path)
{
	if (!path || path[0] == '\0') return NULL;
	int at = slast_index_of(path, '/');
	const char* f = path + at + 1;
	if (f[0] != '\0') return smake(f);
	return NULL;
}

char* ck_spfname_no_ext(const char* path)
{
	char* s = ck_spfname(path);
	if (!s) return NULL;
	int at = slast_index_of(s, '.');
	if (at == 0) { sfree(s); return NULL; }
	if (at == -1) return s;
	serase(s, at, slen(s) - at);
	return s;
}

char* ck_spext(const char* path)
{
	int at = slast_index_of(path, '.');
	if (at == -1 || path[at + 1] == 0 || path[at + 1] == '/') return NULL;
	return smake(path + at);
}

int ck_spext_equ(const char* path, const char* ext)
{
	int at = slast_index_of(path, '.');
	if (at == -1 || path[at + 1] == 0 || path[at + 1] == '/') return 0;
	return sequ(path + at, ext);
}

char* ck_sppop(const char* path)
{
	char* s = sdup(path);
	if (slast(s) == '/') spop(s);
	int at = slast_index_of(s, '/');
	if (at == -1 || at == 0) return (sset(s, "/"), s);
	serase(s, at, slen(s) - at);
	return s;
}

char* ck_sppopn(const char* path, int n)
{
	char* s = sdup(path);
	while (n--) {
		if (slast(s) == '/') spop(s);
		int at = slast_index_of(s, '/');
		if (at == -1 || at == 0) { sset(s, "/"); continue; }
		serase(s, at, slen(s) - at);
	}
	return s;
}

char* ck_spcompact(const char* path, int n)
{
	int len = (int)strlen(path);
	if (n <= 6) return NULL;
	if (len < n) return sdup(path);
	int at = slast_index_of(path, '/');
	if (at == -1 || at == 0) {
		char* s = sdup(path);
		serase(s, n, slen(s) - n);
		serase(s, n - 3, 3);
		sappend(s, "...");
		return s;
	}
	int remaining = len - at - 1;
	if (remaining >= n - 3) {
		char* s = smake("...");
		sappend_range(s, path, path + at - 6);
		sappend(s, "...");
		return s;
	} else {
		char* s = sdup(path);
		int len_s = slen(s);
		int to_erase = len_s - (remaining - 3);
		serase(s, remaining - 3, to_erase);
		sappend(s, "...");
		sappend(s, path + at);
		return s;
	}
}

char* ck_spdir_of(const char* path)
{
	if (!*path || (*path == '.' && (int)strlen(path) < 3)) return NULL;
	if (sequ(path, "../")) return NULL;
	if (sequ(path, "/")) return NULL;
	int at = slast_index_of(path, '/');
	if (at == -1) return NULL;
	if (at == 0) return smake("/");
	char* s = smake(path);
	serase(s, at, slen(s) - at);
	at = slast_index_of(s, '/');
	if (at == -1) {
		if (slen(s) == 2) {
			return s;
		} else {
			s[0] = '/';
			return s;
		}
	}
	serase(s, 0, at);
	return s;
}

char* ck_sptop_of(const char* path)
{
	int at = sfirst_index_of(path, '/');
	if (at == -1) return NULL;
	int next = sfirst_index_of(path + at + 1, '/');
	if (next == -1) return smake("/");
	char* s = sdup(path + at);
	serase(s, next + 1, slen(s) - (next + 1));
	return s;
}

char* ck_spnorm(const char* path)
{
	char* result = NULL;
	int len = (int)strlen(path);
	if (*path != '\\' && *path != '/') {
		int windows_drive = len >= 2 && path[1] == ':';
		if (!windows_drive) {
			spush(result, '/');
		}
	}
	int prev_was_dot = 0;
	int prev_was_dotdot = 0;
	for (int i = 0; i < len; ++i) {
		char c = path[i];
		if (c == '\\' || c == '/') {
			if (!prev_was_dot) {
				spush(result, '/');
			} else if (prev_was_dotdot) {
				char* tmp = ck_sppop(result);
				sfree(result);
				result = tmp;
				spush(result, '/');
			}
			prev_was_dot = 0;
			prev_was_dotdot = 0;
		} else if (c == '.') {
			if (prev_was_dot) prev_was_dotdot = 1;
			prev_was_dot = 1;
		} else {
			if (prev_was_dot) spush(result, '.');
			spush(result, c);
			prev_was_dot = 0;
			prev_was_dotdot = 0;
		}
	}
	sreplace(result, "//", "/");
	if (slen(result) > 1 && slast(result) == '/') spop(result);
	return result;
}

//--------------------------------------------------------------------------------------------------
// Map implementation (originally by Mattias Gustavsson) - Stretchy buffer style.

uint64_t ck_map_hash(uint64_t x)
{
	x += 0x9e3779b97f4a7c15ull;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
	x ^= (x >> 31);
	return x ? x : 1ull;
}

void ck_map_zero_slots(CK_MapHeader* hdr)
{
	CK_MapSlot* slots = ck_map_slots_ptr(hdr);
	for (int i = 0; i < hdr->slot_capacity; ++i) {
		slots[i].h = 0;
		slots[i].item_index = -1;
		slots[i].base_count = 0;
	}
	hdr->slot_count = 0;
}

// Total allocation size for map with given capacities.
size_t ck_map_alloc_size(int capacity, int val_size, int slot_capacity)
{
	size_t items_end = sizeof(CK_MapHeader) + CK_ALIGN8((size_t)capacity * val_size);
	size_t keys_end = items_end + (size_t)capacity * sizeof(uint64_t);
	size_t islot_end = keys_end + (size_t)capacity * sizeof(int);
	size_t slots_start = CK_ALIGN8(islot_end);
	return slots_start + (size_t)slot_capacity * sizeof(CK_MapSlot);
}

int ck_map_find_insertion_slot(CK_MapHeader* hdr, uint64_t h)
{
	CK_MapSlot* slots = ck_map_slots_ptr(hdr);
	int mask = hdr->slot_capacity - 1;
	int base = (int)(h & (uint64_t)mask);
	int slot = base, first_free = base;
	int remaining = slots[base].base_count;

	while (remaining > 0) {
		if (slots[slot].item_index < 0 && slots[first_free].item_index >= 0)
			first_free = slot;
		uint64_t sh = slots[slot].h;
		if (sh) {
			if (((int)(sh & (uint64_t)mask)) == base) --remaining;
		}
		slot = (slot + 1) & mask;
	}

	slot = first_free;
	while (slots[slot].item_index >= 0) {
		slot = (slot + 1) & mask;
	}
	return slot;
}

int ck_map_find_slot(const CK_MapHeader* hdr, uint64_t key, uint64_t h)
{
	if (hdr->slot_capacity == 0) return -1;
	CK_MapSlot* slots = ck_map_slots_ptr((CK_MapHeader*)hdr);
	uint64_t* keys = ck_map_keys_ptr((CK_MapHeader*)hdr);
	int mask = hdr->slot_capacity - 1;
	int base = (int)(h & (uint64_t)mask);
	int slot = base;
	int remaining = slots[base].base_count;
	while (remaining > 0) {
		int item = slots[slot].item_index;
		if (item >= 0) {
			uint64_t sh = slots[slot].h;
			if (((int)(sh & (uint64_t)mask)) == base) {
				--remaining;
				if (sh == h && keys[item] == key) return slot;
			}
		}
		slot = (slot + 1) & mask;
	}
	return -1;
}

// Rebuild hash table after reallocation. Assumes slots are already zeroed.
void ck_map_rebuild_slots(CK_MapHeader* hdr)
{
	if (hdr->slot_capacity == 0) return;
	CK_MapSlot* slots = ck_map_slots_ptr(hdr);
	uint64_t* keys = ck_map_keys_ptr(hdr);
	int* islot = ck_map_islot_ptr(hdr);
	for (int idx = 0; idx < hdr->size; ++idx) {
		uint64_t h = ck_map_hash(keys[idx]);
		int slot = ck_map_find_insertion_slot(hdr, h);
		int base = (int)(h & (uint64_t)(hdr->slot_capacity - 1));
		slots[slot].h = h;
		slots[slot].item_index = idx;
		++slots[base].base_count;
		islot[idx] = slot;
		++hdr->slot_count;
	}
}

// Ensure we have capacity for 'want' items. May reallocate and update m_ptr.
// Returns the possibly-updated header pointer.
// Single allocation: [Header][items][keys][islot][slots]
CK_MapHeader* ck_map_ensure_capacity(void** m_ptr, int want_items, int val_size)
{
	CK_MapHeader* hdr = *m_ptr ? (CK_MapHeader*)((char*)*m_ptr - sizeof(CK_MapHeader)) : NULL;
	int old_item_cap = hdr ? hdr->capacity : 0;
	int old_slot_cap = hdr ? hdr->slot_capacity : 0;
	int old_size = hdr ? hdr->size : 0;

	// Compute new item capacity.
	int new_item_cap = old_item_cap;
	if (want_items > new_item_cap) {
		new_item_cap = old_item_cap ? old_item_cap * 2 : 16;
		while (new_item_cap < want_items) new_item_cap *= 2;
	}

	// Compute new slot capacity. Slots should be at least 2x items for good hash distribution.
	int new_slot_cap = old_slot_cap;
	int min_slot_cap = new_item_cap * 2;
	if (min_slot_cap < 16) min_slot_cap = 16;
	if (new_slot_cap < min_slot_cap) {
		new_slot_cap = 16;
		while (new_slot_cap < min_slot_cap) new_slot_cap *= 2;
	}

	// Also check load factor on existing slots.
	if (hdr && hdr->slot_capacity > 0) {
		int thresh = hdr->slot_capacity - (hdr->slot_capacity >> 2); // 75%
		if (hdr->slot_count >= thresh) {
			int grown = hdr->slot_capacity * 2;
			if (grown > new_slot_cap) new_slot_cap = grown;
		}
	}

	// If no change needed, return current.
	if (new_item_cap == old_item_cap && new_slot_cap == old_slot_cap) {
		return hdr;
	}

	// Allocate new block.
	size_t new_size = ck_map_alloc_size(new_item_cap, val_size, new_slot_cap);
	CK_MapHeader* new_hdr = (CK_MapHeader*)CK_ALLOC(new_size);
	memset(new_hdr, 0, new_size);

	// Initialize header.
	new_hdr->cookie = CK_MAP_COOKIE;
	new_hdr->val_size = val_size;
	new_hdr->size = old_size;
	new_hdr->capacity = new_item_cap;
	new_hdr->slot_count = 0;
	new_hdr->slot_capacity = new_slot_cap;

	// Copy existing data if any.
	if (hdr && old_size > 0) {
		memcpy(ck_map_items_ptr(new_hdr), ck_map_items_ptr(hdr), (size_t)old_size * val_size);
		memcpy(ck_map_keys_ptr(new_hdr), ck_map_keys_ptr(hdr), (size_t)old_size * sizeof(uint64_t));
	}

	// Initialize islot to -1.
	int* new_islot = ck_map_islot_ptr(new_hdr);
	for (int i = 0; i < new_item_cap; i++) new_islot[i] = -1;

	// Initialize slots item_index to -1 (0 would appear occupied).
	CK_MapSlot* new_slots = ck_map_slots_ptr(new_hdr);
	for (int i = 0; i < new_slot_cap; i++) new_slots[i].item_index = -1;

	// Rebuild hash table.
	ck_map_rebuild_slots(new_hdr);

	// Free old allocation.
	if (hdr) CK_FREE(hdr);

	// Update user pointer.
	*m_ptr = ck_map_items_ptr(new_hdr);
	return new_hdr;
}

int ck_map_find_impl(CK_MapHeader* hdr, uint64_t key)
{
	if (!hdr || hdr->size == 0 || hdr->slot_capacity == 0) return -1;
	uint64_t h = ck_map_hash(key);
	int s = ck_map_find_slot(hdr, key, h);
	if (s < 0) return -1;
	CK_MapSlot* slots = ck_map_slots_ptr(hdr);
	return slots[s].item_index;
}

void ck_map_set_stretchy(void** m_ptr, uint64_t key, const void* val, int val_size)
{
	CK_MapHeader* hdr = *m_ptr ? (CK_MapHeader*)((char*)*m_ptr - sizeof(CK_MapHeader)) : NULL;
	uint64_t h = ck_map_hash(key);

	// Update if entry already exists.
	if (hdr && hdr->slot_capacity) {
		int s = ck_map_find_slot(hdr, key, h);
		if (s >= 0) {
			CK_MapSlot* slots = ck_map_slots_ptr(hdr);
			int idx = slots[s].item_index;
			char* items = (char*)ck_map_items_ptr(hdr);
			memcpy(items + idx * hdr->val_size, val, (size_t)hdr->val_size);
			return;
		}
	}

	// Ensure capacity (may reallocate and rebuild hash table).
	int want = (hdr ? hdr->size : 0) + 1;
	hdr = ck_map_ensure_capacity(m_ptr, want, val_size);
	assert(hdr->val_size == val_size);

	// Get array pointers (may have changed after realloc).
	char* items = (char*)ck_map_items_ptr(hdr);
	uint64_t* keys = ck_map_keys_ptr(hdr);
	int* islot = ck_map_islot_ptr(hdr);
	CK_MapSlot* slots = ck_map_slots_ptr(hdr);

	// Append to dense set.
	int idx = hdr->size++;
	keys[idx] = key;
	memcpy(items + idx * hdr->val_size, val, (size_t)hdr->val_size);

	// Insert into hash slots.
	int slot = ck_map_find_insertion_slot(hdr, h);
	int base = (int)(h & (uint64_t)(hdr->slot_capacity - 1));
	slots[slot].h = h;
	slots[slot].item_index = idx;
	++slots[base].base_count;
	islot[idx] = slot;
	++hdr->slot_count;
}

void* ck_map_get_ptr_impl(CK_MapHeader* hdr, uint64_t key)
{
	if (!hdr || hdr->size == 0 || hdr->slot_capacity == 0) return NULL;
	uint64_t h = ck_map_hash(key);
	int s = ck_map_find_slot(hdr, key, h);
	if (s < 0) return NULL;
	CK_MapSlot* slots = ck_map_slots_ptr(hdr);
	return (char*)ck_map_items_ptr(hdr) + slots[s].item_index * hdr->val_size;
}

int ck_map_del_impl(CK_MapHeader* hdr, uint64_t key)
{
	if (!hdr || hdr->size == 0 || hdr->slot_capacity == 0) return 0;
	uint64_t h = ck_map_hash(key);
	int s = ck_map_find_slot(hdr, key, h);
	if (s < 0) return 0;

	CK_MapSlot* slots = ck_map_slots_ptr(hdr);
	uint64_t* keys = ck_map_keys_ptr(hdr);
	int* islot = ck_map_islot_ptr(hdr);
	char* items = (char*)ck_map_items_ptr(hdr);

	int mask = hdr->slot_capacity - 1;
	int base = (int)(h & (uint64_t)mask);
	int idx = slots[s].item_index;
	int last = hdr->size - 1;

	--slots[base].base_count;
	slots[s].item_index = -1;
	--hdr->slot_count;

	if (idx != last) {
		keys[idx] = keys[last];
		memcpy(items + idx * hdr->val_size, items + last * hdr->val_size, (size_t)hdr->val_size);
		int ms = islot[last];
		slots[ms].item_index = idx;
		islot[idx] = ms;
	}
	islot[last] = -1;
	--hdr->size;
	return 1;
}

void ck_map_swap_impl(CK_MapHeader* hdr, int i, int j)
{
	if (i == j) return;
	assert(i >= 0 && i < hdr->size);
	assert(j >= 0 && j < hdr->size);

	uint64_t* keys = ck_map_keys_ptr(hdr);
	int* islot = ck_map_islot_ptr(hdr);
	CK_MapSlot* slots = ck_map_slots_ptr(hdr);
	char* items = (char*)ck_map_items_ptr(hdr);

	// Swap keys.
	uint64_t tk = keys[i];
	keys[i] = keys[j];
	keys[j] = tk;

	// Swap values.
	char tmp[256];
	char* t = hdr->val_size <= (int)sizeof(tmp) ? tmp : (char*)CK_ALLOC((size_t)hdr->val_size);
	memcpy(t, items + i * hdr->val_size, (size_t)hdr->val_size);
	memcpy(items + i * hdr->val_size, items + j * hdr->val_size, (size_t)hdr->val_size);
	memcpy(items + j * hdr->val_size, t, (size_t)hdr->val_size);
	if (t != tmp) CK_FREE(t);

	// Swap islot backpointers.
	int si = islot[i];
	int sj = islot[j];
	if (si >= 0) slots[si].item_index = j;
	if (sj >= 0) slots[sj].item_index = i;
	islot[i] = sj;
	islot[j] = si;
}

void ck_map_sort_range(CK_MapHeader* hdr, int offset, int count, int (*cmp)(const void* a, const void* b))
{
	if (count <= 1) return;
	char* items = (char*)ck_map_items_ptr(hdr);
	char* pivot = items + (offset + count - 1) * hdr->val_size;
	int lo = 0;
	for (int hi = 0; hi < count - 1; ++hi) {
		char* hi_val = items + (offset + hi) * hdr->val_size;
		if (cmp(hi_val, pivot) < 0) {
			ck_map_swap_impl(hdr, offset + lo, offset + hi);
			++lo;
		}
	}
	ck_map_swap_impl(hdr, offset + (count - 1), offset + lo);
	ck_map_sort_range(hdr, offset, lo, cmp);
	ck_map_sort_range(hdr, offset + lo + 1, count - 1 - lo, cmp);
}

void ck_map_sort_impl(CK_MapHeader* hdr, int (*cmp)(const void* a, const void* b))
{
	ck_map_sort_range(hdr, 0, hdr->size, cmp);
}

void ck_map_ssort_range(CK_MapHeader* hdr, int offset, int count, int ignore_case)
{
	if (count <= 1) return;
	uint64_t* keys = ck_map_keys_ptr(hdr);

	#define CK_SSORT_KEY(i) (keys[(offset) + (i)])

	uint64_t pivot = CK_SSORT_KEY(count - 1);
	int lo = 0;
	for (int hi = 0; hi < count - 1; ++hi) {
		const char* hi_key = (const char*)CK_SSORT_KEY(hi);
		const char* pivot_key = (const char*)pivot;
		int cmp = ignore_case ? ck_stricmp(hi_key, pivot_key) : strcmp(hi_key, pivot_key);
		if (cmp < 0) {
			ck_map_swap_impl(hdr, offset + lo, offset + hi);
			++lo;
		}
	}
	ck_map_swap_impl(hdr, offset + (count - 1), offset + lo);
	ck_map_ssort_range(hdr, offset, lo, ignore_case);
	ck_map_ssort_range(hdr, offset + lo + 1, count - 1 - lo, ignore_case);

	#undef CK_SSORT_KEY
}

void ck_map_ssort_impl(CK_MapHeader* hdr, int ignore_case)
{
	ck_map_ssort_range(hdr, 0, hdr->size, ignore_case);
}

void ck_map_free_impl(CK_MapHeader* hdr)
{
	CK_FREE(hdr);  // Single allocation.
}

void ck_map_clear_impl(CK_MapHeader* hdr)
{
	hdr->size = 0;
	if (hdr->slot_capacity) ck_map_zero_slots(hdr);
}

//--------------------------------------------------------------------------------------------------
// String interning (originally from Per Vognsen).

uint64_t ck_hash_fnv1a(const void* ptr, size_t sz)
{
	uint64_t x = 0xcbf29ce484222325ull;
	const char* buf = (const char*)ptr;
	for (size_t i = 0; i < sz; i++) {
		x ^= (uint64_t)(unsigned char)buf[i];
		x *= 0x100000001b3ull;
		x ^= x >> 32;
	}
	return x;
}

// C11 atomics for C, std::atomic for C++
// Must close extern "C" before including <atomic> since it contains C++ templates.
#ifdef __cplusplus
} // extern "C"
#include <atomic>
extern "C" {
#define CK_ATOMIC(T) std::atomic<T>
#define ck_atomic_load(p) (p)->load()
#define ck_atomic_store(p, v) (p)->store(v)
#define ck_atomic_compare_exchange_strong(p, expected, desired) (p)->compare_exchange_strong(*(expected), (desired))
#define ck_atomic_compare_exchange_weak(p, expected, desired) (p)->compare_exchange_weak(*(expected), (desired))
#else
#include <stdatomic.h>
#define CK_ATOMIC(T) _Atomic(T)
#define ck_atomic_load(p) atomic_load(p)
#define ck_atomic_store(p, v) atomic_store(p, v)
#define ck_atomic_compare_exchange_strong(p, expected, desired) atomic_compare_exchange_strong(p, expected, desired)
#define ck_atomic_compare_exchange_weak(p, expected, desired) atomic_compare_exchange_weak(p, expected, desired)
#endif

typedef struct CK_InternTable
{
	CK_MAP(CK_UniqueString*) interns;
	CK_ATOMIC(int) lock;
} CK_InternTable;

static CK_ATOMIC(CK_InternTable*) g_intern_table;

static CK_InternTable* ck_sintern_get_table()
{
	CK_InternTable* table = ck_atomic_load(&g_intern_table);
	if (!table) {
		CK_InternTable* new_table = (CK_InternTable*)CK_ALLOC(sizeof(CK_InternTable));
		memset(new_table, 0, sizeof(CK_InternTable));
		CK_InternTable* expected = NULL;
		if (ck_atomic_compare_exchange_strong(&g_intern_table, &expected, new_table)) {
			table = new_table;
		} else {
			CK_FREE(new_table);
			table = expected;
		}
	}
	return table;
}

static void ck_sintern_lock(CK_InternTable* table)
{
	int expected = 0;
	while (!ck_atomic_compare_exchange_weak(&table->lock, &expected, 1)) {
		expected = 0;
	}
}

static void ck_sintern_unlock(CK_InternTable* table)
{
	ck_atomic_store(&table->lock, 0);
}

const char* ck_sintern_range(const char* start, const char* end)
{
	CK_InternTable* table = ck_sintern_get_table();
	size_t len = (size_t)(end - start);
	uint64_t key = ck_hash_fnv1a((void*)start, len);

	ck_sintern_lock(table);

	CK_UniqueString* head = map_get(table->interns, key);
	for (CK_UniqueString* it = head; it; it = it->next) {
		if ((size_t)it->len == len && memcmp(it->str, start, len) == 0) {
			ck_sintern_unlock(table);
			return it->str;
		}
	}

	size_t bytes = sizeof(CK_UniqueString) + len + 1;
	CK_UniqueString* node = (CK_UniqueString*)CK_ALLOC(bytes);
	node->cookie = CK_INTERN_COOKIE;
	node->len = (int)len;
	node->next = head;
	node->str = (char*)(node + 1);
	memcpy(node->str, start, len);
	node->str[len] = '\0';
	map_set(table->interns, key, node);

	ck_sintern_unlock(table);
	return node->str;
}

void sintern_nuke()
{
	CK_InternTable* table = ck_atomic_load(&g_intern_table);
	if (!table) return;

	ck_sintern_lock(table);
	ck_atomic_store(&g_intern_table, (CK_InternTable*)NULL);

	for (int i = 0; i < map_size(table->interns); ++i) {
		CK_UniqueString* it = map_items(table->interns)[i];
		while (it) {
			CK_UniqueString* next = it->next;
			CK_FREE(it);
			it = next;
		}
	}
	map_free(table->interns);
	ck_sintern_unlock(table);
	CK_FREE(table);
}

const uint16_t* ck_decode_UTF16(const uint16_t* s, int* codepoint)
{
	int W1 = *s++;
	if (W1 < 0xD800 || W1 > 0xDFFF) {
		*codepoint = W1;
	} else if (W1 > 0xD800 && W1 < 0xDBFF) {
		int W2 = *s++;
		if (W2 > 0xDC00 && W2 < 0xDFFF) *codepoint = 0x10000 + (((W1 & 0x03FF) << 10) | (W2 & 0x03FF));
		else *codepoint = 0xFFFD;
	} else *codepoint = 0xFFFD;
	return s;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CKIT_IMPLEMENTATION_GUARD
#endif // CKIT_IMPLEMENTATION
