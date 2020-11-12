
# string_t::decref

Manually decrements the reference count of this string.

## Syntax

```cpp
void decref();
```

## Remarks

This function can cause the underlying string to destruct and be removed from the string pool. If you frequently refer to a particular string, make sure at least one reference count exists at all times to avoid constantly injecting/discarding the string within the underlying string pool. You can use [string_t::incref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/incref.md) and [string_t::decref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/decref.md) to manually manipulate reference counts to avoid this problem.

## Related Functions

[string_t::incref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/incref.md)  
