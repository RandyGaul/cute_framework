## String Class

Cute's string class is implemented by Cute's [string pool](https://github.com/RandyGaul/cute_framework/tree/master/doc/string/strpool) implementation. This means the strings are subject to two major limitations.

1. Raw c-strings retrieved from strings must not be stored, and only used temporarily.
2. The string pool behind the string class implementation is fundamentally single-threaded, so passing strings over multiple threads is a big no-no unless you know what you're doing.

For more details on these limitations read the documentation of Cute's [string pool here](https://github.com/RandyGaul/cute_framework/tree/master/doc/string/strpool).

[string_t::string_t](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_t.md)  
[string_t::~string_t](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/~string_t.md)  
[string_t::len](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/len.md)  
[string_t::c_str](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/c_str.md)  
[string_t::operator=](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/operator=.md)  
[string_t::operator==](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/operator==.md)  
[string_t::operator!=](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/operator!=.md)  
[string_t::operator[]](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/operator[].md)  
[string_t::incref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/incref.md)  
[string_t::decref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/decref.md)  
[string_t::is_valid](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/is_valid.md)  
[string_defrag_static_pool](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_defrag_static_pool.md)  
[string_nuke_static_pool](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_nuke_static_pool.md)  
[operator+](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/operator+.md)  
[to_int](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_int.md)  
[to_float](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_float.md)  
[to_int](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_int.md)  
[format](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/format.md)  
[to_string](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_string.md)  
[to_array](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/to_array.md)  
[string_utils_cleanup_static_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string/string_utils_cleanup_static_memory.md)  
