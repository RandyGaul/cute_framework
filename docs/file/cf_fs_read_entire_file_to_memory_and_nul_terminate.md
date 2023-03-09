[](../header.md ':include')

# cf_fs_read_entire_file_to_memory_and_nul_terminate

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Reads an entire file into a buffer of memory and returns it as a nul-terminated C string.

```cpp
char* cf_fs_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, size_t* size);
```

Parameters | Description
--- | ---
virtual_path | A path to the file.
size | If the file exists the size of the file is stored here.

## Remarks

Call `CUTE_FREE` on it when done. TODO_LINK_VFS_README.

## Related Pages

[cf_fs_read_entire_file_to_memory](/file/cf_fs_read_entire_file_to_memory.md)  
[cf_fs_write_entire_buffer_to_file](/file/cf_fs_write_entire_buffer_to_file.md)  
