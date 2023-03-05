# cf_fs_read | [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file_readme.md) | [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)

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

The file must be opened in read mode with [cf_fs_open_file_for_read](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_open_file_for_read.md). Returns the number of bytes read. Returns -1 on
failure. TODO_LINK_VFS_README.

## Related Pages

[CF_File](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_file.md)  
[cf_fs_file_exists](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_file_exists.md)  
[cf_fs_size](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_size.md)  
[cf_fs_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_write.md)  
[cf_fs_eof](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_eof.md)  
[cf_fs_tell](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_tell.md)  
[cf_fs_seek](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_seek.md)  
