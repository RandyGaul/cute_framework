
# string_t::c_str

Returns the raw c-string.

## Syntax

```cpp
const char* c_str() const;
```

## Return Value

The raw c-string stored within the underlying string pool.

## Remarks

The c-string returned is only valid while temporarily - it can become invalid whenever [string_t::~string_t](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/~string_t.md) is called. Therefor it is recommended to only store `string_t` instances, and not the raw c-string itself. Any time you need the c-string, simply look it up briefly with [string_t::c_str](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/c_str.md).

This function call does contain a small overhead of a hash table lookup - however, it is quite optimized and can be called many times within a single game-tick without any worry about performance cost.

> An example to show storing `string_t` instances, and only temporarily using the c-string on an as-needed basis.

```cpp
std::vector<string_t> names = get_names();

for (int i = 0; i < names.size(); ++i) {
	string_t string = names[i];
	
	// Lookup a c-string for a given id, but do not store the c-string anywhere.
	const char* c_string = string.c_str();
	printf("Name %s is present.\n", c_string);
}
```

## Related Functions

[string_t::len](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/len.md)  
