# cf_fs_open_file_for_read

Category: [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Opens a file for reading.

```cpp
CF_File* cf_fs_open_file_for_read(const char* virtual_path);
```

Parameters | Description
--- | ---
virtual_path | The virtual path to the file.

## Return Value

Returns a [CF_File](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_file.md) pointer representing the file.

## Remarks

If you just want some basic information about the file (such as it's size or when it was created), you can use [cf_fs_stat](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_stat.md) instead. TODO_LINK_VFS_README.

## Related Pages

[CF_File](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_file.md)  
[CF_Stat](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_stat.md)  
[cf_fs_create_file](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_create_file.md)  
[cf_fs_open_file_for_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_open_file_for_write.md)  
[cf_fs_close](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_close.md)  
