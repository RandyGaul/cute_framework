
# strpool_cstr

Returns the NUL terminated c-string for a given string id.

## Syntax

```cpp
const char* strpool_cstr(const strpool_t* pool, strpool_id id);
```

## Function Parameters

Parameter Name | Description
--- | ---
pool | The string pool.
id | The string to lookup.

## Return Value

Returns the NUL terminated c-string for the given string id. Returns NULL if the id is invalid.

## Remarks

The c-string returned is only valid while [destroy_strpool](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/destroy_strpool.md), [strpool_defrag](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_defrag.md), and [strpool_discard](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_discard.md) are not called. Therefor it is recommended to only store the string id associated with a string, and not the raw c-string itself. Any time you need the c-string, simply look it up briefly with [strpool_cstr](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_cstr.md).

This function call does contain a small overhead of a hash table lookup - however, it is quite optimized and can be called many times within a single game-tick without any worry about performance cost.

> An example to show storing a string id, and only temporarily using the c-string on an as-needed basis.

```cpp
std::vector<string_id> names = get_names();

for (int i = 0; i < names.size(); ++i) {
	string_id string = names[i];
	
	// Lookup a c-string for a given id, but do not store the c-string anywhere.
	const char* c_string = strpool_cstr(pool, string);
	printf("Name %s is present.\n", c_string);
}
```

## Related Functions
  
[strpool_inject](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_inject.md)  
[strpool_discard](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_discard.md)  
[strpool_length](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_length.md)  
