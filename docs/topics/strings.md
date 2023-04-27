[](../header.md ':include')

<br>

CF has a [dynamic string API](https://randygaul.github.io/cute_framework/#/api_reference?id=string) where strings are 100% compatible with normal C-strings. Dynamic strings can be modified and grow onto the heap as necessary. They come with a wide variety of manipulation functions such as removing certain characters, trimming whitespace, find + replace, and a whole lot more. In C++ we also have the `String` class which wraps the dynamic C string API.

## Dynamic Strings

In CF's C API we can create a new string with [`sset`](https://randygaul.github.io/cute_framework/#/string/sset).

```cpp
char* s = NULL;
sset(s, "Hello world!");
printf("%s", s);
sfree(s);
```

Or alternatively:

```cpp
char* s = sset(NULL, "Hello world!");
printf("%s", s);
sfree(s);
```

Which outputs:

```
Hello world!
```

All dynamic strings must be free'd up with [`sfree`](https://randygaul.github.io/cute_framework/#/string/sfree) when done.

To push some more characters onto the end of the string use [`spush`](https://randygaul.github.io/cute_framework/#/string/spush).

```cpp
char* s = NULL;
sset(s, "Hello world!");

spush(s, '!');
spush(s, '!');

printf("%s", s);
sfree(s);
```

Which outputs:

```
Hello world!!!
```

You can append a string onto the end of a dynamic string with [`sappend`](https://randygaul.github.io/cute_framework/#/string/sappend).

```cpp
char* s = NULL;
sset(s, "Hello world!");

sappend(s, " What a nice string we have.");
printf("%s", s);

sfree(s);
```

Which outputs:

```
Hello world! What a string string we have.
```

## String Conversions

To convert an integer to a string call [`sint`](https://randygaul.github.io/cute_framework/#/string/sint).

```cpp
char* s = sint(NULL, 10);
printf("%s", s); // s is now "10".
```

To convert from a string to an integer call [stoint](https://randygaul.github.io/cute_framework/#/string/stoint).

```cpp
int x = stoint("100");
```

There are similar functions available for float, double, boolean, and hex numbers.

## String Formatting

String formatting is done with a printf-style function called [`sfmt`](https://randygaul.github.io/cute_framework/#/string/sfmt).

```cpp
sfmt(s, "%s said hello to %s.\n", "Bob", "Sally");
printf("%s, s"); // Prints: "Bob said hello to Sally."
```

You can append a format (instead of overwriting the previous string contents) with [`sfmt_append`](https://randygaul.github.io/cute_framework/#/string/sfmt_append).

## String Manipulation

There are a variety of manipulation functions available for strings. Be sure to check out the [String API Reference](https://randygaul.github.io/cute_framework/#/api_reference?id=string) for a full list and for more examples. Some really popular ones include [`sreplace`](https://randygaul.github.io/cute_framework/#/string/sreplace), [`ssplit`](https://randygaul.github.io/cute_framework/#/string/ssplit), and [`strim`](https://randygaul.github.io/cute_framework/#/string/strim).

## String in C++

In C++ we have access to the `String` class. It wraps up the dynamic C string API into a convenience class. It will automatically call `sfree` in its destructor. Here's a quick demo of some its basic features:

```cpp
String s = "   Well hello there! Today is <color> day, meaning everything is the color <color>.    ";
s.trim().replace("<color>", "red");
printf("%s", s.c_str());
```

Which would output:

```
Well hello there! Today is red day, meaning everything is the color red.
```

## String Hashing

To get a hash of a string call [`shash`](https://randygaul.github.io/cute_framework/#/string/shash).

Be sure to check out this section on [String Interning](https://randygaul.github.io/cute_framework/#/topics/data_structures?id=strings-as-keys), which covers the [String Intern API](https://randygaul.github.io/cute_framework/#/string/sintern). You may use this to construct immutable strings that work super efficiently for comparisons and hash tables.

## UTF8

It's highly recommended to store strings for your game in text files and load them up from disk. This makes it easy for a localizer to make different versions of text in different langauges without editing anything other than simple text files. The format of your strings should be in the [UTF8 format](https://en.wikipedia.org/wiki/UTF-8), which is 100% backwards compatible with typical C-strings you're already used to.

The UTF8 format encodes a large number of characters by making certain characters take up more than a single byte. To encode or decode UTF8 characters you may call [`sappend_UTF8`](https://randygaul.github.io/cute_framework/#/string/sappend_utf8) or [`cf_decode_UTF8`](https://randygaul.github.io/cute_framework/#/string/cf_decode_utf8).

In C++ we have access to the `UTF8` helper class. Simply load it up with a string and call `.next()` to get each decoded character with `.codepoint`.

```cpp

UTF8 utf8 = UTF8(my_string_in_utf8_format);
while (utf8.next()) {
    DoSomethingWithCodepoint(utf8.codepoint);
}
```

## Paths

TODO - Document the string path API : https://randygaul.github.io/cute_framework/#/api_reference/?id=path
