#include "dl_alloc.h"

#include <stdlib.h>

static void* dl_internal_alloc( size_t size, void* /*ctx*/ )
{
	return malloc( size );
}

static void* dl_internal_realloc( void* ptr, size_t size, size_t /*old_size*/, void* /*ctx*/ )
{
	return realloc( ptr, size );
}

static void dl_internal_free( void* ptr, void* /*ctx*/ )
{
	free( ptr );
}

int dl_allocator_initialize( dl_allocator* alloc, dl_alloc_func alloc_f, dl_realloc_func realloc_f, dl_free_func free_f, void* alloc_ctx )
{
	if( alloc_f == 0x0 && free_f == 0x0 && realloc_f == 0x0 )
	{
		// use bulitin!
		alloc->alloc   = dl_internal_alloc;
		alloc->realloc = dl_internal_realloc;
		alloc->free    = dl_internal_free;
		alloc->ctx     = 0x0;
		return 1;
	}

	if( alloc_f == 0x0 || free_f == 0x0 )
	{
		// invalid setup!
		return 0;
	}

	alloc->alloc   = alloc_f;
	alloc->free    = free_f;
	alloc->realloc = realloc_f;
	alloc->ctx     = alloc_ctx;

	return 1;
}
