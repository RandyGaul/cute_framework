[](../header.md ':include')

# cf_fs_write

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Writes bytes from a file opened in write mode.

```cpp
CF_API size_t CF_CALL cf_fs_write(CF_File* file, const void* buffer, size_t size);
```

Parameters | Description
--- | ---
CF_File | The file.
buffer | Pointer to a buffer of bytes.
size | The size in bytes of `buffer`.

## Remarks

The file must be opened in write mode with [cf_fs_open_file_for_write](/file/cf_fs_open_file_for_write.md). Returns the number of bytes written. Returns -1 on
failure. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Related Pages

[CF_File](/file/cf_file.md)  
[cf_fs_file_exists](/file/cf_fs_file_exists.md)  
[cf_fs_read](/file/cf_fs_read.md)  
[cf_fs_size](/file/cf_fs_size.md)  
[cf_fs_eof](/file/cf_fs_eof.md)  
[cf_fs_tell](/file/cf_fs_tell.md)  
[cf_fs_seek](/file/cf_fs_seek.md)  
