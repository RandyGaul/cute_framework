# cf_fs_size

Category: [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Returns the size of a file in bytes.

```cpp
size_t cf_fs_size(CF_File* file);
```

Parameters | Description
--- | ---
CF_File | The file.

## Remarks

You might want to use [cf_fs_stat](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_stat.md) instead to avoid needing to fully open the file first. TODO_LINK_VFS_README.

## Related Pages

[CF_File](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_file.md)  
[cf_fs_file_exists](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_file_exists.md)  
[cf_fs_read](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_read.md)  
[cf_fs_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_write.md)  
[cf_fs_eof](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_eof.md)  
[cf_fs_tell](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_tell.md)  
[cf_fs_seek](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_seek.md)  
