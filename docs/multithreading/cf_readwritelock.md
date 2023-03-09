[](../header.md ':include')

# CF_ReadWriteLock

Category: [multithreading](/api_reference?id=multithreading)  
GitHub: [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)  
---

An opaque handle representing a read-write lock.

## Remarks

A read/write lock can have a large number of simultaneous readers, but only one writer at a time. This can be
used as an opimization where a resources can be safely read from many threads. Then, when the resource must be
modified a writer can wait for all readers to leave, and then exclusively lock to perform a write update.

## Related Pages

[cf_write_unlock](/multithreading/cf_write_unlock.md)  
[cf_make_rw_lock](/multithreading/cf_make_rw_lock.md)  
[cf_destroy_rw_lock](/multithreading/cf_destroy_rw_lock.md)  
[cf_read_lock](/multithreading/cf_read_lock.md)  
[cf_read_unlock](/multithreading/cf_read_unlock.md)  
[cf_write_lock](/multithreading/cf_write_lock.md)  
