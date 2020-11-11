
# strpool_inject

Adds a string to the pool and returns a handle representing the string. Each handle is unique for each unique string. Duplicate strings containing the same contents are represented by the same handle.

## Syntax

```cpp
strpool_id strpool_inject(strpool_t* pool, const char* string, int length);
strpool_id strpool_inject(strpool_t* pool, const char* string);
```

## Function Parameters

Parameter Name | Description
--- | ---
pool | The string pool to insert the string into.
string | The string to insert into the pool. The string does not need to be NUL terminated for the first overload (with the `length` parameter included), but whenever it is fetched by [strpool_cstr](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_cstr.md) the returned string will be NUL terminated. In the second function overload `string` must be NUL terminated, as `length` is not provided as a parameter at all.
length | The length of the `string` parameter.

## Return Value

Returns a `strpool_id`, which is simply a `uint64_t`.

> An excerpt of how strpool_id is defined within cute_strpool.h.

```cpp
using strpool_id = uint64_t;
```

## Remarks

Since each unique string in the pool is represented with a unique 64-bit integer id `string_id`, each id can be copied around and compared with 64-bit integer comparisons. This means storing and comparing strings for equality is very efficient when using the string pool.

> Example of inserting some strings into the pool and performing integer comparisons.

```cpp
strpool_t* pool = make_strpool();
string_id a = strpool_inject(pool, "Hello");
string_id b = strpool_inject(pool, "Goodbye");
string_id c = strpool_inject(pool, "Hello");

printf("a == b is %s\n", a == b ? "true", "false");
printf("b == c is %s\n", b == c ? "true", "false");
printf("a == c is %s\n", a == c ? "true", "false");
```

Which outputs the following.

```
a == b is false
b == c is false
a == c is true
```

## Related Functions

[strpool_discard](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_discard.md)  
[strpool_cstr](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_cstr.md)  
[strpool_length](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_length.md)  
