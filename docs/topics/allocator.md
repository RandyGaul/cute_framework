# Allocators

The topic of custom allocators is only relevant for certain games trying to eek out performance, i.e. for more advanced users. For most games, simply calling `malloc` or `new` is quite sufficient. However, in the event where some kind of custom allocator is needed [`cf_allocator_override`](../allocator/cf_allocator_override.md) provides a way to supply a custom [`CF_Allocator`](../allocator/cf_allocator.md). This lets you hook in your own allocation model and do whatever you want with all allocations internal to Cute Framework. For example, some games might need to reduce memory fragmentation and can't afford simply call `malloc` alone, but instead want to wrap it up inside of their implementation.

## Overriding the Default Allocator

To override the default allocator a few functions must be defined and passed to CF as function pointers.

- alloc_fn
- free_fn
- calloc_fn
- realoc_fn

You can see the protoypes of each function in the definition of [`CF_Allocator`](../allocator/cf_allocator.md):

```cpp
typedef struct CF_Allocator
{
	void* udata;
	void* (*alloc_fn)(size_t size, void* udata);
	void (*free_fn)(void* ptr, void* udata);
	void* (*calloc_fn)(size_t size, size_t count, void* udata);
	void* (*realloc_fn)(void* ptr, size_t size, void* udata);
} CF_Allocator;
```

Each of these functions has a `udata` parameter, standing for "user data". This is an optional pointer that simply gets handed back to you whenever any of the function pointers are called. This lets you have easy access to external state, such as a pointer to your memory allocator.

## Implementing Your Own Allocator

Here's an example of how you might implement your own custom allocation functions. Let's assume you have `MyAllocator` implemented somewhere in your code.

```cpp
void* my_alloc(size_t size, void* udata)
{
	MyAllocator* allocator = (MyAllocator*)udata;
	return allocator->alloc(size, udata);
}

void my_free(void* ptr, void* udata)
{
	MyAllocator* allocator = (MyAllocator*)udata;
	return allocator->free(ptr);
}

void* my_calloc(size_t size, size_t count, void* udata)
{
	MyAllocator* allocator = (MyAllocator*)udata;
	return allocator->calloc(size, count, udata);
}

void* my_realloc(void* ptr, size_t size, void* udata)
{
	MyAllocator* allocator = (MyAllocator*)udata;
	return allocator->alloc(ptr, size, udata);
}
```

Then simply assign each function to a [`CF_Allocator`](../allocator/cf_allocator.md) struct, and pass it onto [`cf_allocator_override`](../allocator/cf_allocator_override.md).

```cpp
CF_Allocator allocator;
allocator.udata = &my_allocator;
allocator.alloc_fn = my_alloc;
allocator.free_fn = my_free;
allocator.calloc_fn = my_calloc;
allocator.realloc_fn = my_realloc;
```

## Restoring the Default Allocator

If for any reason you need to restore the default allocator, simply call [`cf_allocator_restore_default`](../allocator/cf_allocator_restore_default.md).
