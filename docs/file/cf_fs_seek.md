[](../header.md ':include')

# cf_fs_seek

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Sets the current position within a file.

```cpp
CF_API CF_Result CF_CALL cf_fs_seek(CF_File* file, size_t position);
```

Parameters | Description
--- | ---
CF_File | The file.
position | The read/write position.

## Return Value

Returns any errors as a [CF_Result](/utility/cf_result.md).

## Remarks

This is an offset from the beginning of the file. The next read or write will happen at this position. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Related Pages

[CF_File](/file/cf_file.md)  
[cf_fs_file_exists](/file/cf_fs_file_exists.md)  
[cf_fs_read](/file/cf_fs_read.md)  
[cf_fs_write](/file/cf_fs_write.md)  
[cf_fs_eof](/file/cf_fs_eof.md)  
[cf_fs_tell](/file/cf_fs_tell.md)  
[cf_fs_size](/file/cf_fs_size.md)  
