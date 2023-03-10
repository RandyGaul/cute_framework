[](../header.md ':include')

# cf_fs_close

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Closes a file.

```cpp
CF_API CF_Result CF_CALL cf_fs_close(CF_File* file);
```

Parameters | Description
--- | ---
file | The file.

## Return Value

Returns any errors as a [CF_Result](/utility/cf_result.md).

## Related Pages

[CF_File](/file/cf_file.md)  
[CF_Stat](/file/cf_stat.md)  
[cf_fs_create_file](/file/cf_fs_create_file.md)  
[cf_fs_open_file_for_write](/file/cf_fs_open_file_for_write.md)  
[cf_fs_open_file_for_read](/file/cf_fs_open_file_for_read.md)  
