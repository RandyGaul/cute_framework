[](../header.md ':include')

# cf_fs_read_entire_file_to_memory

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Reads an entire file into a buffer of memory and returns it.

```cpp
CF_API void* CF_CALL cf_fs_read_entire_file_to_memory(const char* virtual_path, size_t* size);
```

Parameters | Description
--- | ---
virtual_path | A path to the file.
size | If the file exists the size of the file is stored here.

## Remarks

Call [CF_FREE](/allocator/cf_free.md) on it when done. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Related Pages

[cf_fs_write_entire_buffer_to_file](/file/cf_fs_write_entire_buffer_to_file.md)  
[cf_fs_read_entire_file_to_memory_and_nul_terminate](/file/cf_fs_read_entire_file_to_memory_and_nul_terminate.md)  
