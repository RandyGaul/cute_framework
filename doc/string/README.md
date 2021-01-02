# String

Cute has a utility string class available for single-threaded use cases. Cute [strings](https://github.com/RandyGaul/cute_framework/tree/master/doc/string/string) are implemented with Cute's [string pool](https://github.com/RandyGaul/cute_framework/tree/master/doc/string/strpool) API.

Strings in Cute (the `string_t` class) are an alternative to C++'s std::string. There are two big performance problems with std::string that string_t addresses.

1. Duplicate strings incur a large RAM overhead.
2. Copying strings to and fro is a fairly involved operation, especially in terms of cache coherency.

Cute's string class avoids both problems by using a [string interning](https://en.wikipedia.org/wiki/String_interning#:~:text=In%20computer%20science%2C%20string%20interning,string%20is%20created%20or%20interned.) algorithm. However, there is a major limitation for solving these two problems -- strings can not be rapidly mutated. Doing so would cause the string to be inserted and removed from the internal interning cache, which is way slower than simply using std::string. This means the string_t *does not have a write-able index operator []*. It also means there is no += operator for similar reasons. However, for convenience, there is a + operator, but please use it sparingly.

If you really need to mutate a string rapidly, for example, changing the case of a string to all upper/lower, or performing some string sort operation, it is then recommended to first convert to an array. Once in array form operations on the individual characters can be performed efficiently. When done, convert back to a string.

```cpp
string_t to_upper(string_t s)
{
    array<char> a = to_array(s);
    for (int i = 0; i < a.size(); ++i) {
        a[i] = toupper(a[i];
    }
    return to_string(a);
}
```

[string](https://github.com/RandyGaul/cute_framework/tree/master/doc/string/string)  
[strpool](https://github.com/RandyGaul/cute_framework/tree/master/doc/string/strpool)  
[utf8](https://github.com/RandyGaul/cute_framework/tree/master/doc/string/utf8)  
