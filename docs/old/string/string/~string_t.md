
# string_t::~string_t

Destructs a `string_t` instance.

## Syntax

```cpp
~string_t();
```

## Remarks

The strings reference count is decremented by one. This can cause the string to destruct, invalidating all `string_id`s for the associated raw c-string. Sometimes you must be careful not to destruct your last reference to a string in order to avoid performance problems. If you frequently refer to a particular string, make sure at least one reference count exists at all times to avoid constantly injecting/discarding the string within the underlying string pool. You can use [string_t::incref](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/incref.md) and [string_t::decref](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/decref.md) to manually manipulate reference counts to avoid this problem.

## Related Functions

[string_t::string_t](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/string_t.md)  
