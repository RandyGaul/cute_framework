[](../header.md ':include')

# cf_fs_open_file_for_write

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Opens a file for writing relative to the write directory.

```cpp
CF_API CF_File* CF_CALL cf_fs_open_file_for_write(const char* virtual_path);
```

## Return Value

Returns a [CF_File](/file/cf_file.md) pointer representing the file.

## Remarks

The write directory is specified by you when calling [cf_fs_set_write_directory](/file/cf_fs_set_write_directory.md). TODO_LINK_VFS_README.

## Related Pages

[CF_File](/file/cf_file.md)  
[CF_Stat](/file/cf_stat.md)  
[cf_fs_create_file](/file/cf_fs_create_file.md)  
[cf_fs_close](/file/cf_fs_close.md)  
[cf_fs_open_file_for_read](/file/cf_fs_open_file_for_read.md)  
