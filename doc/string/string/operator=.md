
# string_t::operator=

Assigns a string value by copying from another `string_t` instance.

## Syntax

```cpp
string_t& operator=(const string_t& rhs);
```

## Remarks

Decrements the old string's reference count, and increments the new one copied.

## Related Functions

[string_t::string_t](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_t.md)  
[string_t::incref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/incref.md)  
[string_t::decref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/decref.md)  
