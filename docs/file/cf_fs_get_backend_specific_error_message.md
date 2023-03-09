[](../header.md ':include')

# cf_fs_get_backend_specific_error_message

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Returns a backend specific error message in the case of any file system errors.

```cpp
const char* cf_fs_get_backend_specific_error_message();
```

## Remarks

Feel free to call this whenever an error occurs in any of the file system functions to try and get a detailed description
on what might have happened. Often times this string is already returned to you inside a [CF_Result](/utility/cf_result.md).

