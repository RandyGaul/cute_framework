# cf_fs_open_file_for_append | [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/README.md) | [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)

Opens a file for appending relative to the write directory.

```cpp
CF_File* cf_fs_open_file_for_append(const char* virtual_path);
```

## Return Value

Returns a [CF_File](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_file.md) pointer representing the file.

## Remarks

The write directory is specified by you when calling [cf_fs_set_write_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_set_write_directory.md). TODO_LINK_VFS_README.

## Related Pages

[CF_File](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_file.md)  
[CF_Stat](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_stat.md)  
[cf_fs_create_file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_create_file.md)  
[cf_fs_open_file_for_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_open_file_for_write.md)  
[cf_fs_open_file_for_read](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_open_file_for_read.md)  
[cf_fs_close](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_close.md)  
