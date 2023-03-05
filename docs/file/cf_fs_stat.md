# cf_fs_stat | [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/README.md) | [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)

Returns file information at the given virtual path, such as file size or creation time.

```cpp
CF_Result cf_fs_stat(const char* virtual_path, CF_Stat* stat);
```

## Return Value

Returns any errors as a [CF_Result](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result.md).

## Remarks

This doesn't open the file itself, and is a fairly light-weight operation in comparison. TODO_LINK_VFS_README.

## Related Pages

[CF_File](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_file.md)  
[CF_Stat](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_stat.md)  
[cf_fs_create_file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_create_file.md)  
[cf_fs_open_file_for_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_open_file_for_write.md)  
[cf_fs_open_file_for_read](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_open_file_for_read.md)  
[cf_fs_close](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_close.md)  
