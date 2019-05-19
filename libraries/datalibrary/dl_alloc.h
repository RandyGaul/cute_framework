#ifndef DL_ALLOC_H_INCLUDED
#define DL_ALLOC_H_INCLUDED

#include <dl/dl.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct dl_allocator
{
	dl_alloc_func   alloc;
	dl_realloc_func realloc;
	dl_free_func    free;
	void* ctx;
} dl_allocator;

/**
 * Initialize a dl_allocator to be used internally in DL.
 *
 * If alloc_f and free_f is both NULL, malloc, realloc and free will be used.
 * If alloc_f and free_f is not NULL, but realloc_f if NULL a fallback using alloc_f and free_f together with memcpy will be used.
 * Returns 0 if initialization fails, 1 if it succeeds.
 */
int dl_allocator_initialize( dl_allocator* alloc, dl_alloc_func alloc_f, dl_realloc_func realloc_f, dl_free_func free_f, void* alloc_ctx );

/**
 * Allocator memory on an allocator.
 */
inline void* dl_alloc( dl_allocator* alloc, size_t size )
{
	return alloc->alloc( size, alloc->ctx );
}

/**
 * Free memory on an allocator.
 */
inline void dl_free( dl_allocator* alloc, void* ptr )
{
	alloc->free( ptr, alloc->ctx );
}

/**
 * Realloc data on an allocator.
 */
inline void* dl_realloc( dl_allocator* alloc, void* ptr, size_t size, size_t old_size )
{
	if( alloc->realloc != 0x0 )
		return alloc->realloc( ptr, size, old_size, alloc->ctx );

	void* new_ptr = dl_alloc( alloc, size );
	if( ptr != 0x0 )
	{
		memcpy( new_ptr, ptr, old_size );
		dl_free( alloc, ptr );
	}
	return new_ptr;
}

#ifdef __cplusplus
}
#endif  // __cplusplus


#endif // DL_ALLOC_H_INCLUDED

