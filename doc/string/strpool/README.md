## String Pool

Cute's string pool API is a wrapper around [Mattias Gustavsson's](https://github.com/mattiasgustavsson/libs) Public Domain string pool/intern single-file C header. String pooling is a very efficient way to deal with strings, while coming at the cost of extra difficulty when dealing with multi-threading.

The overall idea of the string pool is to store many strings in one large pool and avoid duplicate entries. Strings can be referred to by a 64-bit id, making comparisons and copies simply integer comparisons or copies. However, there is some extra overhead for translating from id to actual c-string, which is just a hash table lookup. There is also the downside that changing the pool contents is a single-threaded operation, making string pools difficult to use across multiple threads.

If you're looking for something more like a string class, check out Cute's single-threaded [string](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string) implementation. The string pool API is mostly included here as a low-level option in case anyone wants to use it, and is used to implement Cute's [string](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/string) class.

[make_strpool](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/make_strpool.md)  
[destroy_strpool](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/destroy_strpool.md)  
[strpool_inject](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_inject.md)  
[strpool_discard](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_discard.md)  
[strpool_defrag](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_defrag.md)  
[strpool_incref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_incref.md)  
[strpool_decref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_decref.md)  
[strpool_getref](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_getref.md)  
[strpool_isvalid](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_isvalid.md)  
[strpool_cstr](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_cstr.md)  
[strpool_length](https://github.com/RandyGaul/cute_framework/blob/master/doc/string/strpool/strpool_length.md)  
