[](../header.md ':include')

# cf_fs_free_enumerated_directory

Category: [file](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Frees a file list from [cf_fs_create_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_create_directory.md).

```cpp
void cf_fs_free_enumerated_directory(const char** directory_list);
```

Parameters | Description
--- | ---
directory_list | The directory list returned from [cf_fs_create_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_create_directory.md).

## Related Pages

[cf_fs_remove_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_remove_directory.md)  
[cf_fs_create_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_create_directory.md)  
[cf_fs_enumerate_directory](https://github.com/RandyGaul/cute_framework/blob/master/docs/file/cf_fs_enumerate_directory.md)  
