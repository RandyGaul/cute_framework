# Strings

CF has a [dynamic string API](../api_reference.md#string) where strings are 100% compatible with normal C-strings. Dynamic strings can be modified and grow onto the heap as necessary. They come with a wide variety of manipulation functions such as removing certain characters, trimming whitespace, find + replace, and a whole lot more. In C++ we also have the `String` class which wraps the dynamic C string API.

> [!NOTE]
> CF provides shortform convenience macros for common string operations (e.g. `smake`, `sfree`, `sappend`, `sfmt`). These are simple wrappers around the longform `cf_string_*` functions. The shortform macros are not individually documented but work identically to their longform counterparts.

## Dynamic Strings

In CF's C API we can create a new string with [`cf_string_make`](../string/cf_string_make.md) or [`cf_string_dup`](../string/cf_string_dup.md).

```cpp
char* s = smake("Hello world!");
printf("%s", s);
sfree(s);
```

Which outputs:

```
Hello world!
```

To overwrite an existing string use [`cf_string_set`](../string/cf_string_set.md). The first argument must be an l-value (a variable), not a literal like NULL. Use `cf_string_make` to create a string from scratch.

```cpp
char* s = smake("Hello world!");
sset(s, "Goodbye!");
printf("%s", s);
sfree(s);
```

All dynamic strings must be free'd up with [`cf_string_free`](../string/cf_string_free.md) when done.

To push some more characters onto the end of the string use [`cf_string_push`](../string/cf_string_push.md).

```cpp
char* s = smake("Hello world!");

spush(s, '!');
spush(s, '!');

printf("%s", s);
sfree(s);
```

Which outputs:

```
Hello world!!!
```

You can append a string onto the end of a dynamic string with [`cf_string_append`](../string/cf_string_append.md).

```cpp
char* s = smake("Hello world!");

sappend(s, " What a nice string we have.");
printf("%s", s);

sfree(s);
```

Which outputs:

```
Hello world! What a nice string we have.
```

## String Conversions

To convert an integer to a string call [`cf_string_int`](../string/cf_string_int.md).

```cpp
char* s = sint(NULL, 10);
printf("%s", s); // s is now "10".
```

To convert from a string to an integer call [`cf_string_toint`](../string/cf_string_toint.md).

```cpp
int x = stoint("100");
```

There are similar functions available for float, double, boolean, and hex numbers.

## String Formatting

String formatting is done with a printf-style function called [`cf_string_fmt`](../string/cf_string_fmt.md). To create a new formatted string from scratch use [`cf_string_fmt_make`](../string/cf_string_fmt_make.md).

```cpp
char* s = sfmake("%s said hello to %s.\n", "Bob", "Sally");
printf("%s", s); // Prints: "Bob said hello to Sally."
sfree(s);
```

To overwrite an existing string with formatted text use `cf_string_fmt`. To append formatted text use [`cf_string_fmt_append`](../string/cf_string_fmt_append.md).

## String Manipulation

There are a variety of manipulation functions available for strings. Be sure to check out the [String API Reference](../api_reference.md#string) for a full list and for more examples. Some really popular ones include [`cf_string_replace`](../string/cf_string_replace.md), [`cf_string_split`](../string/cf_string_split.md), and [`cf_string_trim`](../string/cf_string_trim.md).

## String in C++

In C++ we have access to the [`String` class](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h) (near the bottom of cute_string.h). It wraps up the dynamic C string API into a convenience class. It will automatically call `sfree` in its destructor. Here's a quick demo of some its basic features:

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

To get a hash of a string call [`cf_string_hash`](../string/cf_string_hash.md).

Be sure to check out this section on [String Interning](../topics/data_structures.md#strings-as-keys), which covers the [String Intern API](../string/sintern.md). You may use this to construct immutable strings that work super efficiently for comparisons and maps.

## UTF8

It's highly recommended to store strings for your game in text files and load them up from disk. This makes it easy for a localizer to make different versions of text in different languages without editing anything other than simple text files. The format of your strings should be in the [UTF8 format](https://en.wikipedia.org/wiki/UTF-8), which is 100% backwards compatible with typical C-strings you're already used to.

The UTF8 format encodes a large number of characters by making certain characters take up more than a single byte. To encode or decode UTF8 characters you may call [`sappend_UTF8`](../string/sappend_utf8.md) or [`cf_decode_UTF8`](../string/cf_decode_utf8.md).

In C++ we have access to the `UTF8` helper class. Simply load it up with a string and call `.next()` to get each decoded character with `.codepoint`.

```cpp
UTF8 utf8 = UTF8(my_string_in_utf8_format);
while (utf8.next()) {
    DoSomethingWithCodepoint(utf8.codepoint);
}
```

## Paths

The [path API in CF](../api_reference.md#path) is a set of helper functions to deal with paths as a string. It auto-magically inserts "/" between folder names as appropriate, and can pop parts of the path off the end. It's great for easily opening up and traversing directories.

### Paths in C++

It's recommended to use the [C++ wrapper for paths in CF](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h) (located at the bottom of cute_file_system.h), along with the directory wrapper.

Here's an example of creating a `Cute::Path`.

```cpp
Path path = "/content/my_file.txt";
```

There are a variety of helpful member functions on the path helper, such as checking if a path is a folder or file:

```cpp
Path path = "/content/my_file.txt";
if (path.is_directory()) {
	// ...
} else if (path.is_file()) {
	// ...
}
```

It's easy to pop files or directories off the end of the path:

```cpp
Path path = "/content/my_file.txt";
path.pop(1);              // Path is now "/content"."
path += "other_file.txt"; // Path is now "/content/other_file.txt".
// Notice how "/" was auto-magically inserted.
```

You may also check if a path is a file or directory without creating a `Path` instance using some static functions:

```cpp
// Check if path is a directory.
if (Path::is_directory("/content/areas")) {
	// ...
}

// Check if path is a file.
if (Path::is_file("/content/areas/area1.txt")) {
	// ...
}
```

Similarly a `Directory` helper class exists to easily enumerate files in a directory.

```cpp
// Print all files in a directory.
Directory dir = Directory::open("/content/areas");
for (const char* file = dir.next(); file; file = dir.next()) {
	printf("File file %s\n", file);
}
```
