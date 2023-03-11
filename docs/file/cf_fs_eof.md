[](../header.md ':include')

# cf_fs_eof

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Check to see if the eof has been found after reading a file opened in read mode.

```cpp
CF_API CF_Result CF_CALL cf_fs_eof(CF_File* file);
```

Parameters | Description
--- | ---
CF_File | The file.

## Remarks

[Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Related Pages

[CF_File](/file/cf_file.md)  
[cf_fs_file_exists](/file/cf_fs_file_exists.md)  
[cf_fs_read](/file/cf_fs_read.md)  
[cf_fs_write](/file/cf_fs_write.md)  
[cf_fs_size](/file/cf_fs_size.md)  
[cf_fs_tell](/file/cf_fs_tell.md)  
[cf_fs_seek](/file/cf_fs_seek.md)  
