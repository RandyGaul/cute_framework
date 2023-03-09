[](../header.md ':include')

# cf_fs_read

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Reads bytes from a file opened in read mode.

```cpp
size_t cf_fs_read(CF_File* file, void* buffer, size_t size);
```

Parameters | Description
--- | ---
CF_File | The file.
buffer | Pointer to a buffer of bytes.
size | The size in bytes of `buffer`.

## Remarks

The file must be opened in read mode with [cf_fs_open_file_for_read](/file/cf_fs_open_file_for_read.md). Returns the number of bytes read. Returns -1 on
failure. TODO_LINK_VFS_README.

## Related Pages

[CF_File](/file/cf_file.md)  
[cf_fs_file_exists](/file/cf_fs_file_exists.md)  
[cf_fs_size](/file/cf_fs_size.md)  
[cf_fs_write](/file/cf_fs_write.md)  
[cf_fs_eof](/file/cf_fs_eof.md)  
[cf_fs_tell](/file/cf_fs_tell.md)  
[cf_fs_seek](/file/cf_fs_seek.md)  
