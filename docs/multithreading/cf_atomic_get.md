# cf_atomic_get | [multithreading](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading_readme.md) | [cute_multithreading.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_multithreading.h)

Atomically fetches the value at `atomic`.

```cpp
int cf_atomic_get(CF_AtomicInt* atomic);
```

Parameters | Description
--- | ---
atomic | The integer to fetch from.

## Remarks

Atomics are an advanced topic. You've been warned! Beej has a [good article on atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html).

## Related Pages

[cf_atomic_zero](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_zero.md)  
[cf_atomic_add](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_add.md)  
[cf_atomic_set](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_set.md)  
[cf_atomic_ptr_cas](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_ptr_cas.md)  
[cf_atomic_cas](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_cas.md)  
[cf_atomic_ptr_set](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_ptr_set.md)  
[cf_atomic_ptr_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/multithreading/cf_atomic_ptr_get.md)  
