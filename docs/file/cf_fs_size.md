[](../header.md ':include')

# cf_fs_size

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Returns the size of a file in bytes.

```cpp
CF_API size_t CF_CALL cf_fs_size(CF_File* file);
```

Parameters | Description
--- | ---
CF_File | The file.

## Remarks

You might want to use [cf_fs_stat](/file/cf_fs_stat.md) instead to avoid needing to fully open the file first. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Related Pages

[CF_File](/file/cf_file.md)  
[cf_fs_file_exists](/file/cf_fs_file_exists.md)  
[cf_fs_read](/file/cf_fs_read.md)  
[cf_fs_write](/file/cf_fs_write.md)  
[cf_fs_eof](/file/cf_fs_eof.md)  
[cf_fs_tell](/file/cf_fs_tell.md)  
[cf_fs_seek](/file/cf_fs_seek.md)  
