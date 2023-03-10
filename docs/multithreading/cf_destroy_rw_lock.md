[](../header.md ':include')

# cf_destroy_rw_lock

Category: [multithreading](/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Destroys a [CF_ReadWriteLock](/multithreading/cf_readwritelock.md) made from [cf_make_rw_lock](/multithreading/cf_make_rw_lock.md).

```cpp
CF_API void CF_CALL cf_destroy_rw_lock(CF_ReadWriteLock* rw);
```

Parameters | Description
--- | ---
rw | The read/write lock.

## Related Pages

[CF_ReadWriteLock](/multithreading/cf_readwritelock.md)  
[cf_make_rw_lock](/multithreading/cf_make_rw_lock.md)  
[cf_write_unlock](/multithreading/cf_write_unlock.md)  
[cf_read_lock](/multithreading/cf_read_lock.md)  
[cf_read_unlock](/multithreading/cf_read_unlock.md)  
[cf_write_lock](/multithreading/cf_write_lock.md)  
