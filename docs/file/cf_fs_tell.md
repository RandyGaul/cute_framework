# cf_fs_tell | [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file_readme.md) | [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)

Returns the current position within the file.

```cpp
size_t cf_fs_tell(CF_File* file);
```

Parameters | Description
--- | ---
CF_File | The file.

## Remarks

This is an offset from the beginning of the file. Returns -1 on failure. TODO_LINK_VFS_README.

## Related Pages

[CF_File](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_file.md)  
[cf_fs_file_exists](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_file_exists.md)  
[cf_fs_read](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_read.md)  
[cf_fs_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_write.md)  
[cf_fs_eof](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_eof.md)  
[cf_fs_size](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_size.md)  
[cf_fs_seek](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_seek.md)  
