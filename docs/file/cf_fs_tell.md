[](../header.md ':include')

# cf_fs_tell

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

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

[CF_File](/file/cf_file.md)  
[cf_fs_file_exists](/file/cf_fs_file_exists.md)  
[cf_fs_read](/file/cf_fs_read.md)  
[cf_fs_write](/file/cf_fs_write.md)  
[cf_fs_eof](/file/cf_fs_eof.md)  
[cf_fs_size](/file/cf_fs_size.md)  
[cf_fs_seek](/file/cf_fs_seek.md)  
