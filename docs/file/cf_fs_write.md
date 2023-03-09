[](../header.md ':include')

# cf_fs_write

Category: [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Writes bytes from a file opened in write mode.

```cpp
size_t cf_fs_write(CF_File* file, const void* buffer, size_t size);
```

Parameters | Description
--- | ---
CF_File | The file.
buffer | Pointer to a buffer of bytes.
size | The size in bytes of `buffer`.

## Remarks

The file must be opened in write mode with [cf_fs_open_file_for_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_open_file_for_write.md). Returns the number of bytes written. Returns -1 on
failure. TODO_LINK_VFS_README.

## Related Pages

[CF_File](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_file.md)  
[cf_fs_file_exists](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_file_exists.md)  
[cf_fs_read](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_read.md)  
[cf_fs_size](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_size.md)  
[cf_fs_eof](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_eof.md)  
[cf_fs_tell](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_tell.md)  
[cf_fs_seek](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_seek.md)  
