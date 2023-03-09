[](../header.md ':include')

# cf_fs_seek

Category: [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Sets the current position within a file.

```cpp
CF_Result cf_fs_seek(CF_File* file, size_t position);
```

Parameters | Description
--- | ---
CF_File | The file.
position | The read/write position.

## Return Value

Returns any errors as a [CF_Result](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result.md).

## Remarks

This is an offset from the beginning of the file. The next read or write will happen at this position. TODO_LINK_VFS_README.

## Related Pages

[CF_File](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_file.md)  
[cf_fs_file_exists](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_file_exists.md)  
[cf_fs_read](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_read.md)  
[cf_fs_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_write.md)  
[cf_fs_eof](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_eof.md)  
[cf_fs_tell](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_tell.md)  
[cf_fs_size](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_size.md)  
