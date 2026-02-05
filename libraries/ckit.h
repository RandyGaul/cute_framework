/*
    ckit -- A tiny, single-header kit of high-performance C essentials.

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
            CK_MAP(int) m = {0};
            map_init(m, int);
            map_set(m, sintern("x"), int, 10);
            map_set(m, sintern("y"), int, 20);
            int x = map_get(m, sintern("x"), int);
            int y = map_get(m, sintern("y"), int);
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>

// Configurable allocator. Define before including ckit.h to override.
#ifndef CK_ALLOC
#define CK_ALLOC(sz) malloc(sz)
#endif
#ifndef CK_REALLOC
#define CK_REALLOC(p, sz) realloc(p, sz)
#endif
#ifndef CK_FREE
#define CK_FREE(p) free(p)
#endif

// Portable case-insensitive string compare.
#ifdef _WIN32
#define ck_stricmp _stricmp
#else
#define ck_stricmp strcasecmp
#endif

//--------------------------------------------------------------------------------------------------
// Dynamic arrays.
//
// Example:
//
//    int* a = NULL;
//    for (int i = 0; i < 10; ++i) {
//        apush(a, i);
//    }
//    for (int i = 0; i < 10; ++i) {
//        printf("%d\n", a[i]);
//    }
//    printf("len=%d cap=%d\n", acount(a), acap(a));
//    afree(a);

#define asize(a)      ((a) ? CK_AHDR(a)->size : 0)
#define acount(a)     asize(a)
#define alen(a)       (CK_AHDR(a)->size)
#define acap(a)       ((a) ? CK_AHDR(a)->capacity : 0)
#define afit(a, n)    ((n) <= acap(a) ? 0 : (*(void**)&(a) = ck_agrow((a), (n), sizeof(*(a)))))
#define apush(a, ...) (CK_ACANARY(a), afit((a), 1 + ((a) ? asize(a) : 0)), (a)[CK_AHDR(a)->size++] = (__VA_ARGS__))
#define apop(a)       ((a)[--CK_AHDR(a)->size])
#define aend(a)       ((a) + asize(a))
#define alast(a)      ((a)[asize(a) - 1])
#define aclear(a)     (CK_ACANARY(a), (a) ? CK_AHDR(a)->size = 0 : 0)
#define adel(a, i)    ((a)[i] = (a)[--CK_AHDR(a)->size])
#define astatic(a, buffer, buffer_size) (*(void**)&(a) = ck_astatic(buffer, buffer_size, sizeof(*(a))))
#define aset(a, b)    (*(void**)&(a) = ck_aset((void*)(a), (void*)(b), sizeof(*(a))))
#define arev(a)       ((a) ? ck_arev(a, sizeof(*(a))) : (void*)0)
#define afree(a)      do { CK_ACANARY(a); if (a && !CK_AHDR(a)->is_static) CK_FREE(CK_AHDR(a)); (a) = NULL; } while (0)

//--------------------------------------------------------------------------------------------------
// Dynamic strings (built on dynamic arrays).
//
// 100% compatible with normal C-strings. Free with sfree when done.
//
// Example:
//
//     char* s = NULL;
//     sset(s, "Hello world!");
//     printf("%s\n", s);
//     sfree(s);

#define slen(s)                 ((s) ? (asize(s) ? asize(s) - 1 : asize(s)) : 0)
#define scount(s)               asize(s)
#define scap(s)                 acap(s)
#define sempty(s)               ((s) ? slen(s) < 1 : 1)
#define sfirst(s)               ((s) ? (s)[0] : '\0')
#define slast(s)                ((s) ? (s)[slen(s) - 1] : '\0')
#define sclear(s)               (aclear(s), apush(s, 0))
#define sfree(s)                afree(s)
#define sfit(s, n)              (s = ck_sfit(s, n))
#define spush(s, ch)            do { if (!(s)) apush(s, ch); else (s)[slen(s)] = (ch); apush(s, 0); } while (0)
#define spop(s)                 (s = ck_spop(s))
#define spopn(s, n)             (s = ck_spopn(s, n))
#define sset(a, b)              (a = ck_sset(a, b))
#define sdup(s)                 ck_sset(NULL, s)
#define smake(s)                ck_sset(NULL, s)
#define sfmt(s, fmt, ...)       (s = ck_sfmt(s, fmt, __VA_ARGS__))
#define sfmt_append(s, fmt, ...) (s = ck_sfmt_append(s, fmt, __VA_ARGS__))
#define svfmt(s, fmt, args)     (s = ck_svfmt(s, fmt, args))
#define svfmt_append(s, fmt, args) (s = ck_svfmt_append(s, fmt, args))
#define sappend(a, b)           (a = ck_sappend(a, b))
#define scat(a, b)              sappend(a, b)
#define sappend_range(a, b, e)  (a = ck_sappend_range(a, b, e))
#define scat_range(a, b, e)     sappend_range(a, b, e)
#define scmp(a, b)              strcmp(a, b)
#define sicmp(a, b)             ck_stricmp(a, b)
#define sequ(a, b)              ((a) == NULL && (b) == NULL ? 1 : ((a) == NULL || (b) == NULL ? 0 : !strcmp((a), (b))))
#define siequ(a, b)             ((a) == NULL && (b) == NULL ? 1 : ((a) == NULL || (b) == NULL ? 0 : !ck_stricmp((a), (b))))
#define sprefix(s, p)           ck_sprefix(s, p)
#define ssuffix(s, p)           ck_ssuffix(s, p)
#define scontains(s, sub)       (slen(s) >= (int)strlen(sub) && !!strstr(s, sub))
#define sfirst_index_of(s, ch)  ck_sfirst_index_of(s, ch)
#define slast_index_of(s, ch)   ck_slast_index_of(s, ch)
#define sfind(s, sub)           strstr(s, sub)
#define stoupper(s)             ck_stoupper(s)
#define stolower(s)             ck_stolower(s)
#define shash(s)                ck_hash_fnv1a(s, slen(s))
#define strim(s)                (s = ck_strim(s))
#define sltrim(s)               (s = ck_sltrim(s))
#define srtrim(s)               (s = ck_srtrim(s))
#define slpad(s, ch, n)         (s = ck_slpad(s, ch, n))
#define srpad(s, ch, n)         (s = ck_srpad(s, ch, n))
#define sreplace(s, old, new)   (s = ck_sreplace(s, old, new))
#define sdedup(s, ch)           (s = ck_sdedup(s, ch))
#define serase(s, idx, n)       (s = ck_serase(s, idx, n))
#define ssplit_once(s, ch)      ck_ssplit_once(s, ch)
#define ssplit(s, ch)           ck_ssplit(s, ch)
#define stoint(s)               ck_stoint(s)
#define stouint(s)              ck_stouint(s)
#define stofloat(s)             ck_stofloat(s)
#define stodouble(s)            ck_stodouble(s)
#define stohex(s)               ck_stohex(s)
#define stobool(s)              (!strcmp(s, "true"))
#define sappend_UTF8(s, cp)     (s = ck_sappend_UTF8(s, cp))

// String formatting from primitives.
#define sint(s, i)              sfmt(s, "%d", (int)(i))
#define suint(s, u)             sfmt(s, "%" PRIu64, (uint64_t)(u))
#define sfloat(s, f)            sfmt(s, "%f", (double)(f))
#define sdouble(s, f)           sfmt(s, "%f", (double)(f))
#define shex_str(s, u)          sfmt(s, "0x%x", (unsigned)(u))
#define sbool(s, b)             sfmt(s, "%s", (b) ? "true" : "false")

//--------------------------------------------------------------------------------------------------
// String path utilities.
//
// Pure string manipulation. All return newly allocated strings (caller must sfree).
//
// Example:
//
//     char* dir = spdir_of("/usr/local/bin/app");   // -> "/local/bin"
//     char* ext = spext("file.txt");                // -> ".txt"
//     char* norm = spnorm("C:\\Users\\..\\foo");     // -> "C:/foo"
//     sfree(dir); sfree(ext); sfree(norm);

#define spfname(s)              ck_spfname(s)
#define spfname_no_ext(s)       ck_spfname_no_ext(s)
#define spext(s)                ck_spext(s)
#define spext_equ(s, ext)       ck_spext_equ(s, ext)
#define sppop(s)                ck_sppop(s)
#define sppopn(s, n)            ck_sppopn(s, n)
#define spcompact(s, n)         ck_spcompact(s, n)
#define spdir_of(s)             ck_spdir_of(s)
#define sptop_of(s)             ck_sptop_of(s)
#define spnorm(s)               ck_spnorm(s)

//--------------------------------------------------------------------------------------------------
// Map (typed hashtable).
//
// Supports arbitrary value types via val_size. Keys are always uint64_t (pointers, ints, enums, etc.)
// You can iterate over keys/vals as dense arrays.
//
// Example:
//
//     CK_MAP(int) m = {0};
//     map_init(m, int);
//     map_set(m, sintern("x"), int, 10);
//     map_set(m, sintern("y"), int, 20);
//     int x = map_get(m, sintern("x"), int);
//     map_free(m);

// Optional markup macro documenting the value type. Compiles to CK_Map.
#define CK_MAP(T) CK_Map

typedef struct CK_MapSlot
{
	uint64_t h;
	int item_index;
	int base_count;
} CK_MapSlot;

typedef struct CK_Map
{
	uint64_t*    keys;
	char*        vals;
	int          val_size;
	int          size;
	int          capacity;

	// Private.
	int*         islot;
	CK_MapSlot*  slots;
	int          slot_count;
	int          slot_capacity;
} CK_Map;

// Initialize map for a specific value type. Must call before first map_set.
#define map_init(m, T)          ck_map_init_impl(ck_map_ref(m), (int)sizeof(T))

// Set a key/value pair. Uses compound literal for rvalue support.
#define map_set(m, k, T, ...)   ck_map_set_impl(ck_map_ref(m), (uint64_t)(k), &(T){__VA_ARGS__}, (int)sizeof(T))

// Get item BY VALUE. Returns zero'd (T){ 0 } if key not found.
#define map_get(m, k, T)        (ck_map_get_ptr_impl(ck_map_ref(m), (uint64_t)(k)) \
                                 ? *(T*)ck_map_get_ptr_impl(ck_map_ref(m), (uint64_t)(k)) \
                                 : (T){ 0 })

// Get pointer to item. Returns NULL if key not found.
#define map_get_ptr(m, k, T)    ((T*)ck_map_get_ptr_impl(ck_map_ref(m), (uint64_t)(k)))

// Check if key exists.
#define map_has(m, k)           ck_map_has_impl(ck_map_ref(m), (uint64_t)(k))

// Delete key.
#define map_del(m, k)           ck_map_del_impl(ck_map_ref(m), (uint64_t)(k))

// Convenience for uint64_t values (backwards compat).
#define map_add(m, k, v)        ck_map_set_impl(ck_map_ref(m), (uint64_t)(k), &(uint64_t){(uint64_t)(v)}, (int)sizeof(uint64_t))
#define map_find(m, k)          map_get(m, k, uint64_t)

// Iteration helpers.
#define map_key(m, i)           (ck_map_ref(m)->keys[i])
#define map_val(m, i, T)        (*(T*)(ck_map_ref(m)->vals + (i) * ck_map_ref(m)->val_size))

// Sorts m->keys as an array of strings lexicographically.
#define map_ssort(m, ignore_case) ck_map_ssort_impl(ck_map_ref(m), ignore_case)

// Swap elements at indices i and j.
#define map_swap(m, i, j)       ck_map_swap_impl(ck_map_ref(m), i, j)

// Clear all items (no free).
#define map_clear(m)            ck_map_clear_impl(ck_map_ref(m))

// Free all resources.
#define map_free(m)             ck_map_free_impl(ck_map_ref(m))

//--------------------------------------------------------------------------------------------------
// String interning.
//
// What:
// - Create a unique, stable pointer for each unique string contents.
//
// Why:
// - For intern'd strings you may compare by pointer directly, instead of by string contents.
// - You can put these directly into hash tables as keys by cast to uint64_t.
// - Reduce RAM consumption by compressing redundant strings down to a single copy.
//
// Example:
//
//    const char* a = sintern("hello");
//    const char* b = sintern("he" "llo");
//    assert(a == b);

#define sintern_range(start, end) ck_sintern_range(start, end)

// Frees all memory used by string interning so far. All prior strings are now invalid.
void sintern_nuke();

//--------------------------------------------------------------------------------------------------
// Private implementation details.

// Hidden array header behind the user pointer.
typedef struct CK_ArrayHeader
{
	int size;
	int capacity;
	int is_static;
	char* data;
	uint32_t cookie;
} CK_ArrayHeader;

#define CK_AHDR(a)    ((CK_ArrayHeader*)(a) - 1)
#define CK_ACOOKIE    0xE6F7E359
#define CK_ACANARY(a) ((a) ? assert(CK_AHDR(a)->cookie == CK_ACOOKIE) : (void)0)

void* ck_agrow(const void* a, int new_size, size_t element_size);
void* ck_astatic(const void* a, int buffer_size, size_t element_size);
void* ck_aset(const void* a, const void* b, size_t element_size);
void* ck_arev(const void* a, size_t element_size);

#ifdef __cplusplus
inline CK_Map* ck_map_ref(CK_Map& m)  { return &m; }
inline CK_Map* ck_map_ref(CK_Map* mp) { return mp; }
#else
#define ck_map_ref(m) _Generic((m), CK_Map*: (m), default: &(m))
#endif

// Map implementation functions.
void  ck_map_init_impl(CK_Map* m, int val_size);
void  ck_map_set_impl(CK_Map* m, uint64_t key, const void* val, int val_size);
void* ck_map_get_ptr_impl(CK_Map* m, uint64_t key);
int   ck_map_has_impl(CK_Map* m, uint64_t key);
int   ck_map_del_impl(CK_Map* m, uint64_t key);
void  ck_map_clear_impl(CK_Map* m);
void  ck_map_free_impl(CK_Map* m);
void  ck_map_swap_impl(CK_Map* m, int i, int j);
void  ck_map_ssort_impl(CK_Map* m, int ignore_case);

// String implementation functions.
char* ck_sfit(char* a, int n);
char* ck_sset(char* a, const char* b);
char* ck_sfmt(char* s, const char* fmt, ...);
char* ck_sfmt_append(char* s, const char* fmt, ...);
char* ck_svfmt(char* s, const char* fmt, va_list args);
char* ck_svfmt_append(char* s, const char* fmt, va_list args);
int   ck_sprefix(const char* s, const char* prefix);
int   ck_ssuffix(const char* s, const char* suffix);
void  ck_stoupper(char* s);
void  ck_stolower(char* s);
char* ck_sappend(char* a, const char* b);
char* ck_sappend_range(char* a, const char* b, const char* b_end);
char* ck_strim(char* s);
char* ck_sltrim(char* s);
char* ck_srtrim(char* s);
char* ck_slpad(char* s, char pad, int count);
char* ck_srpad(char* s, char pad, int count);
char* ck_ssplit_once(char* s, char split_c);
char** ck_ssplit(const char* s, char split_c);
int   ck_sfirst_index_of(const char* s, char c);
int   ck_slast_index_of(const char* s, char c);
int   ck_stoint(const char* s);
uint64_t ck_stouint(const char* s);
float ck_stofloat(const char* s);
double ck_stodouble(const char* s);
uint64_t ck_stohex(const char* s);
char* ck_sreplace(char* s, const char* replace_me, const char* with_me);
char* ck_sdedup(char* s, int ch);
char* ck_serase(char* s, int index, int count);
char* ck_spop(char* s);
char* ck_spopn(char* s, int n);
char* ck_sappend_UTF8(char* s, int codepoint);
const char* cf_decode_UTF8(const char* s, int* codepoint);

// Path implementation functions.
char* ck_spfname(const char* path);
char* ck_spfname_no_ext(const char* path);
char* ck_spext(const char* path);
int   ck_spext_equ(const char* path, const char* ext);
char* ck_sppop(const char* path);
char* ck_sppopn(const char* path, int n);
char* ck_spcompact(const char* path, int n);
char* ck_spdir_of(const char* path);
char* ck_sptop_of(const char* path);
char* ck_spnorm(const char* path);

// Intern implementation.
const char* ck_sintern_range(const char* start, const char* end);
uint64_t ck_hash_fnv1a(const void* ptr, size_t sz);

static inline const char* ck_sintern(const char* s)
{
	return ck_sintern_range(s, s + strlen(s));
}
#define sintern(s) ck_sintern(s)

#endif // CKIT_H

//--------------------------------------------------------------------------------------------------

#ifdef CKIT_IMPLEMENTATION

#ifndef CKIT_IMPLEMENTATION_GUARD
#define CKIT_IMPLEMENTATION_GUARD

#include <inttypes.h>

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
	if (a) alen(a) = asize(b);
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
	alen(a) = bsize;
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
	alen(s) = n;
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
	alen(s) += n - nul;
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
	alen(s) = n;
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
	alen(s) += n - nul;
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
	alen(a) += blen;
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
	alen(a) += blen;
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
	alen(s) = new_len + 1;
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
	alen(s) = new_len + 1;
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
	alen(s) = new_len + 1;
	return s;
}

char* ck_slpad(char* s, char pad, int count)
{
	CK_ACANARY(s);
	sfit(s, scount(s) + count);
	memmove(s + count, s, (size_t)scount(s));
	memset(s, pad, (size_t)count);
	alen(s) += count;
	return s;
}

char* ck_srpad(char* s, char pad, int count)
{
	CK_ACANARY(s);
	sfit(s, scount(s) + count);
	memset(s + slen(s), pad, (size_t)count);
	alen(s) += count;
	s[slen(s)] = 0;
	return s;
}

char* ck_ssplit_once(char* s, char split_c)
{
	CK_ACANARY(s);
	char* start = s;
	char* end = s + slen(s) - 1;
	while (start < end) {
		if (*start == split_c) break;
		++start;
	}
	int len = (int)(start - s);
	if (len + 1 == slen(s)) return NULL;
	char* split = NULL;
	sfit(split, len + 1);
	alen(split) = len + 1;
	memcpy(split, s, (size_t)len);
	split[len] = 0;
	int new_len = slen(s) - len - 1;
	memmove(s, s + len + 1, (size_t)new_len);
	alen(s) = new_len + 1;
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
			alen(s) -= diff;
		} else {
			int remaining = scount(s) - find_offset - (int)replace_len;
			int diff = (int)(with_len - replace_len);
			sfit(s, scount(s) + diff);
			find = s + find_offset;
			memmove(find + with_len, find + replace_len, (size_t)remaining);
			memcpy(find, with_me, with_len);
			alen(s) += diff;
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
	alen(s) = i + 2;
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
		alen(s) = index + 1;
		s[index] = 0;
		return s;
	} else {
		int remaining = scount(s) - (count + index);
		memmove(s + index, s + count + index, (size_t)remaining);
		alen(s) -= count;
	}
	return s;
}

char* ck_spop(char* s)
{
	CK_ACANARY(s);
	if (s && slen(s)) s[--alen(s) - 1] = 0;
	return s;
}

char* ck_spopn(char* s, int n)
{
	CK_ACANARY(s);
	if (!s || n < 0) return s;
	while (scount(s) > 1 && n--) alen(s)--;
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

#ifndef CK_SKIP_CF_DECODE_UTF8
const char* cf_decode_UTF8(const char* s, int* codepoint)
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
#endif

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
// Map implementation (originally by Mattias Gustavsson).

uint64_t ck_map_hash(uint64_t x)
{
	x += 0x9e3779b97f4a7c15ull;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
	x ^= (x >> 31);
	return x ? x : 1ull;
}

void ck_map_zero_slots(CK_Map* m)
{
	for (int i = 0; i < m->slot_capacity; ++i) {
		m->slots[i].h = 0;
		m->slots[i].item_index = -1;
		m->slots[i].base_count = 0;
	}
	m->slot_count = 0;
}

void ck_map_alloc_slots(CK_Map* m, int min_want)
{
	int want = 16;
	while (want < min_want) want <<= 1;
	CK_MapSlot* s = (CK_MapSlot*)CK_REALLOC(m->slots, (size_t)want * sizeof(CK_MapSlot));
	assert(s || want == 0);
	m->slots = s;
	m->slot_capacity = want;
	if (want) ck_map_zero_slots(m);
	else m->slot_count = 0;
}

void ck_map_ensure_items(CK_Map* m, int want)
{
	if (want <= m->capacity) return;
	int nc = m->capacity ? m->capacity * 2 : 16;
	while (nc < want) nc <<= 1;

	uint64_t* nk = (uint64_t*)CK_REALLOC(m->keys, (size_t)nc * sizeof(uint64_t));
	char* nv = (char*)CK_REALLOC(m->vals, (size_t)nc * (size_t)m->val_size);
	int* ns = (int*)CK_REALLOC(m->islot, (size_t)nc * sizeof(int));
	for (int i = m->capacity; i < nc; ++i) ns[i] = -1;

	m->keys = nk;
	m->vals = nv;
	m->islot = ns;
	m->capacity = nc;
}

int ck_map_find_insertion_slot(CK_Map* m, uint64_t h)
{
	int mask = m->slot_capacity - 1;
	int base = (int)(h & (uint64_t)mask);
	int slot = base, first_free = base;
	int remaining = m->slots[base].base_count;

	while (remaining > 0) {
		if (m->slots[slot].item_index < 0 && m->slots[first_free].item_index >= 0)
			first_free = slot;
		uint64_t sh = m->slots[slot].h;
		if (sh) {
			if (((int)(sh & (uint64_t)mask)) == base) --remaining;
		}
		slot = (slot + 1) & mask;
	}

	slot = first_free;
	while (m->slots[slot].item_index >= 0) {
		slot = (slot + 1) & mask;
	}
	return slot;
}

int ck_map_find_slot(const CK_Map* m, uint64_t key, uint64_t h)
{
	if (m->slot_capacity == 0) return -1;
	int mask = m->slot_capacity - 1;
	int base = (int)(h & (uint64_t)mask);
	int slot = base;
	int remaining = m->slots[base].base_count;
	while (remaining > 0) {
		int item = m->slots[slot].item_index;
		if (item >= 0) {
			uint64_t sh = m->slots[slot].h;
			if (((int)(sh & (uint64_t)mask)) == base) {
				--remaining;
				if (sh == h && m->keys[item] == key) return slot;
			}
		}
		slot = (slot + 1) & mask;
	}
	return -1;
}

void ck_map_rebuild_slots(CK_Map* m, int new_cap)
{
	ck_map_alloc_slots(m, new_cap);
	if (m->slot_capacity == 0) return;
	for (int idx = 0; idx < m->size; ++idx) {
		uint64_t h = ck_map_hash(m->keys[idx]);
		int slot = ck_map_find_insertion_slot(m, h);
		int base = (int)(h & (uint64_t)(m->slot_capacity - 1));
		m->slots[slot].h = h;
		m->slots[slot].item_index = idx;
		++m->slots[base].base_count;
		m->islot[idx] = slot;
		++m->slot_count;
	}
}

void ck_map_grow(CK_Map* m)
{
	if (m->slot_capacity == 0) {
		ck_map_rebuild_slots(m, 16);
		return;
	}
	int thresh = m->slot_capacity - (m->slot_capacity >> 2);
	if (m->slot_count >= thresh) {
		ck_map_rebuild_slots(m, m->slot_capacity << 1);
	}
}

void ck_map_init_impl(CK_Map* m, int val_size)
{
	m->val_size = val_size;
}

void ck_map_set_impl(CK_Map* m, uint64_t key, const void* val, int val_size)
{
	// Auto-init val_size on first use.
	if (m->val_size == 0) m->val_size = val_size;
	assert(m->val_size == val_size);

	ck_map_ensure_items(m, m->size + 1);
	uint64_t h = ck_map_hash(key);

	// Update if entry already exists.
	if (m->slot_capacity) {
		int s = ck_map_find_slot(m, key, h);
		if (s >= 0) {
			int idx = m->slots[s].item_index;
			memcpy(m->vals + idx * m->val_size, val, (size_t)m->val_size);
			return;
		}
	}

	ck_map_grow(m);
	if (m->slot_capacity == 0) ck_map_rebuild_slots(m, 16);

	// Append to dense set.
	int idx = m->size++;
	m->keys[idx] = key;
	memcpy(m->vals + idx * m->val_size, val, (size_t)m->val_size);

	// Insert into slots.
	int slot = ck_map_find_insertion_slot(m, h);
	int base = (int)(h & (uint64_t)(m->slot_capacity - 1));
	m->slots[slot].h = h;
	m->slots[slot].item_index = idx;
	++m->slots[base].base_count;
	m->islot[idx] = slot;
	++m->slot_count;
}

void* ck_map_get_ptr_impl(CK_Map* m, uint64_t key)
{
	if (m->size == 0 || m->slot_capacity == 0) return NULL;
	uint64_t h = ck_map_hash(key);
	int s = ck_map_find_slot(m, key, h);
	if (s < 0) return NULL;
	return m->vals + m->slots[s].item_index * m->val_size;
}

int ck_map_has_impl(CK_Map* m, uint64_t key)
{
	if (m->size == 0 || m->slot_capacity == 0) return 0;
	uint64_t h = ck_map_hash(key);
	return ck_map_find_slot(m, key, h) >= 0;
}

int ck_map_del_impl(CK_Map* m, uint64_t key)
{
	if (m->size == 0 || m->slot_capacity == 0) return 0;
	uint64_t h = ck_map_hash(key);
	int s = ck_map_find_slot(m, key, h);
	if (s < 0) return 0;

	int mask = m->slot_capacity - 1;
	int base = (int)(h & (uint64_t)mask);
	int idx = m->slots[s].item_index;
	int last = m->size - 1;

	--m->slots[base].base_count;
	m->slots[s].item_index = -1;
	--m->slot_count;

	if (idx != last) {
		m->keys[idx] = m->keys[last];
		memcpy(m->vals + idx * m->val_size, m->vals + last * m->val_size, (size_t)m->val_size);
		int ms = m->islot[last];
		m->slots[ms].item_index = idx;
		m->islot[idx] = ms;
	}
	m->islot[last] = -1;
	--m->size;
	return 1;
}

void ck_map_swap_impl(CK_Map* m, int i, int j)
{
	if (i == j) return;
	assert(i >= 0 && i < m->size);
	assert(j >= 0 && j < m->size);

	// Swap keys.
	uint64_t tk = m->keys[i];
	m->keys[i] = m->keys[j];
	m->keys[j] = tk;

	// Swap values.
	char tmp[256];
	char* t = m->val_size <= (int)sizeof(tmp) ? tmp : (char*)CK_ALLOC((size_t)m->val_size);
	memcpy(t, m->vals + i * m->val_size, (size_t)m->val_size);
	memcpy(m->vals + i * m->val_size, m->vals + j * m->val_size, (size_t)m->val_size);
	memcpy(m->vals + j * m->val_size, t, (size_t)m->val_size);
	if (t != tmp) CK_FREE(t);

	// Swap islot backpointers.
	int si = m->islot[i];
	int sj = m->islot[j];
	if (si >= 0) m->slots[si].item_index = j;
	if (sj >= 0) m->slots[sj].item_index = i;
	m->islot[i] = sj;
	m->islot[j] = si;
}

void ck_map_ssort_range(CK_Map* m, int offset, int count, int ignore_case)
{
	if (count <= 1) return;

	#define CK_SSORT_KEY(i) (m->keys[(offset) + (i)])

	uint64_t pivot = CK_SSORT_KEY(count - 1);
	int lo = 0;
	for (int hi = 0; hi < count - 1; ++hi) {
		const char* hi_key = (const char*)CK_SSORT_KEY(hi);
		const char* pivot_key = (const char*)pivot;
		int cmp = ignore_case ? ck_stricmp(hi_key, pivot_key) : strcmp(hi_key, pivot_key);
		if (cmp < 0) {
			ck_map_swap_impl(m, offset + lo, offset + hi);
			++lo;
		}
	}
	ck_map_swap_impl(m, offset + (count - 1), offset + lo);
	ck_map_ssort_range(m, offset, lo, ignore_case);
	ck_map_ssort_range(m, offset + lo + 1, count - 1 - lo, ignore_case);

	#undef CK_SSORT_KEY
}

void ck_map_ssort_impl(CK_Map* m, int ignore_case)
{
	ck_map_ssort_range(m, 0, m->size, ignore_case);
}

void ck_map_free_impl(CK_Map* m)
{
	CK_FREE(m->keys);
	CK_FREE(m->vals);
	CK_FREE(m->slots);
	CK_FREE(m->islot);
	memset(m, 0, sizeof(*m));
}

void ck_map_clear_impl(CK_Map* m)
{
	m->size = 0;
	if (m->slot_capacity) ck_map_zero_slots(m);
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

typedef struct CK_UniqueString
{
	size_t len;
	struct CK_UniqueString* next;
	char* str;
} CK_UniqueString;

CK_Map g_interns;

const char* ck_sintern_range(const char* start, const char* end)
{
	size_t len = (size_t)(end - start);
	uint64_t key = ck_hash_fnv1a((void*)start, len);

	CK_UniqueString* head = (CK_UniqueString*)(uintptr_t)map_get(g_interns, key, uint64_t);
	for (CK_UniqueString* it = head; it; it = it->next) {
		if (it->len == len && memcmp(it->str, start, len) == 0) {
			return it->str;
		}
	}

	size_t bytes = sizeof(CK_UniqueString) + len + 1;
	CK_UniqueString* node = (CK_UniqueString*)CK_ALLOC(bytes);
	node->len = len;
	node->next = head;
	node->str = (char*)(node + 1);
	memcpy(node->str, start, len);
	node->str[len] = '\0';
	map_add(g_interns, key, (uintptr_t)node);

	return node->str;
}

void sintern_nuke()
{
	for (int i = 0; i < g_interns.size; ++i) {
		CK_UniqueString* it = (CK_UniqueString*)(uintptr_t)map_val(g_interns, i, uint64_t);
		while (it) {
			CK_UniqueString* next = it->next;
			CK_FREE(it);
			it = next;
		}
	}
	map_free(g_interns);
}

#endif // CKIT_IMPLEMENTATION_GUARD
#endif // CKIT_IMPLEMENTATION
