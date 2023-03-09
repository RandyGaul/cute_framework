[](../header.md ':include')

# cf_fs_write_entire_buffer_to_file

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Writes an entire buffer of data to a file as binary data.

```cpp
CF_Result cf_fs_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size);
```

Parameters | Description
--- | ---
virtual_path | A path to the file.
data | A pointer to the data to write to the file.
size | The size in bytes of `data`.

## Remarks

Call `CUTE_FREE` on it when done. TODO_LINK_VFS_README.

## Related Pages

[cf_fs_read_entire_file_to_memory](/file/cf_fs_read_entire_file_to_memory.md)  
[cf_fs_read_entire_file_to_memory_and_nul_terminate](/file/cf_fs_read_entire_file_to_memory_and_nul_terminate.md)  
