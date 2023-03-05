
# string_t::string_t

Constructs a new `string_t` instance.

## Syntax

```cpp
string_t();
string_t(char* str);
string_t(const char* str);
string_t(const char* begin, const char* end);
string_t(const string_t& other);
```

## Function Parameters

Parameter Name | Description
--- | ---
str | A c-string to be converted to a `string_t` instance.
begin | The beginning of a c-string to be converted to a `string_t` instance.
end | One past the end of a c-string to be converted to a `string_t` instance.
other | Another `string_t` instance to make a copy from.

## Remarks

Be careful about threading concerns. No special care is taken here to handle multi-threading when constructing or dealing with `string_t` instances. Sharing strings across threads would be a no-no, unless you really know what you're doing.

There is no constructor with a `size_t` len value. Simply use the `begin` and `end` overload.

```cpp
string_t s = string_t(c_string, c_string + len);
```

The string's reference count is incremented by one.

## Related Functions

[string_t::~string_t](https://github.com/RandyGaul/cute_framework/blob/master/docs/string/strpool/~string_t.md)  
