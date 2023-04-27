[](../header.md ':include')

# cf_make_rw_lock

Category: [multithreading](/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

Returns an unlocked [CF_ReadWriteLock](/multithreading/cf_readwritelock.md) lock.

```cpp
CF_ReadWriteLock cf_make_rw_lock();
```

## Remarks

Call [cf_destroy_rw_lock](/multithreading/cf_destroy_rw_lock.md) when done.

## Related Pages

[CF_ReadWriteLock](/multithreading/cf_readwritelock.md)  
[cf_write_unlock](/multithreading/cf_write_unlock.md)  
[cf_destroy_rw_lock](/multithreading/cf_destroy_rw_lock.md)  
[cf_read_lock](/multithreading/cf_read_lock.md)  
[cf_read_unlock](/multithreading/cf_read_unlock.md)  
[cf_write_lock](/multithreading/cf_write_lock.md)  
