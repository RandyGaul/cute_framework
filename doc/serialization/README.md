# kv - Key Value Serialization

"Key-value", or kv, is for text-based serialization to/from an in-memory buffer. The design of the kv api is supposed to allow for *mostly* (but not completely) the same code to be used for both to and from buffer.

## kv Format

The kv format is designed based on the [JSON file format](https://en.wikipedia.org/wiki/JSON). The main focus is on key-value based tables and arrays. Here is a quick example of some kv data.

```
size = 10,
name = "Clarence",
data = {
	x = 5,
	z = 15.0,
},
blob = "SGVsbG8gdGhlcmUgOik=",
codes = [3] {
	7, -3, 20,
}
```

kv supports a few types of data listed here.

- Integers
- Floats
- strings
- [base64](https://en.wikipedia.org/wiki/Base64) encoded blobs, as strings
- arrays
- objects

## kv Instances

An instance of a `kv_t` represents a serialization context capabale of reading or writing. Here is an example of how to make an uninitialized kv instance.

```cpp
kv_t* kv = kv_make();
```

The next step is to choose reading or writing mode. Reading mode is for deserializing data from a buffer into your own variables or objects. Writing is the reverse process, converting your runtime data into string format within a buffer.

## Read Mode

Read mode is setup by the `kv_parse` function, like so.

```cpp
error_t err = kv_parse(kv, buffer, size);
if (err.is_error()) {
	// The data in `buffer` is not well formed, handle this error here...
}

// kv is now ready to be read from.
```

The flow of reading from a kv is to search for a key, then query the value. Each step happens with the corresponding `kv_key` and `kv_val` functions. `kv_key` will search for a certain key, and if found the following `kv_val` function will fetch the associated value.

Say we are dealing with the following kv text in a buffer.

```cpp
const char* text =
	"a = 10,"
	"b = 12,"
;

void* buffer = (void*)text;
size_t len = strlen(text);


error_t err = kv_parse(kv, buffer, size);
if (err.is_error()) {
	// The data in `buffer` is not well formed, handle this error here...
}

// kv is now ready to be read from.
```

To read the value `10` from the key `a` followed by the value `12` from key `b` we can do the following.

```cpp
int value;

kv_key(kv, "a");
kv_val(kv, &value);
printf("a was %d\n", value);

kv_key(kv, "b");
kv_val(kv, &value);
printf("b was %d\n", value);
```

Which would print the following.

```
a was 10
b was 12
```

## Write Mode

To select write mode call `kv_set_write_buffer` like so.

```cpp
kv_set_write_buffer(kv, buffer, size);
```

As in the previous section values can be written to the buffer the same way they were read, by calling `kv_key` followed by `kv_val`.

```cpp
int a = 10;
int b = 12;

kv_key(kv, "a");
kv_val(kv, &a);

kv_key(kv, "b");
kv_val(kv, &b);
```

The resulting buffer will be filled with the following text (without the NUL terminator).

```
a = 10,
b = 12,
```

To add the NUL terminator to the buffer you can use `kv_size_written`.

```
char* ptr = (char*)buffer;
ptr[kv_size_written(kv)] = 0;
```

## Objects

In kv objects are values wrapped in curly braces. In the following example there is an object called `data`.

```
size = 10,
name = "Clarence",
data = {
	x = 5,
	z = 15.0,
},
blob = "SGVsbG8gdGhlcmUgOik=",
codes = [3] {
	7, -3, 20,
}
```

Objects act similar to a scope in the C programming language. Querying for the key `data` should be paired up with a matching `kv_object_begin` call, like so (after parsing the above example text with `kv_parse`).

```
kv_key(kv, "data");
kv_object_begin(kv);
```

Now any calls to `kv_key` will search within the `data` object. Once finished pulling values out of the `data` object such as `x` and `z` the function `kv_object_end` can be used to exit the `data` object and return to the previous scope.

```cpp
kv_object_end(kv);
```

Please note that objects can be arbitrarily nested.

## Arrays

Arrays in kv must be entered and exited explicitly by calling `kv_array_begin` and `kv_array_end`. Once `kv_array_begin` is called a consecutive series of `kv_val` calls can be made, one for each element of the array.

Here is an example of reading from an array.

```
data = [5] {
	5, 3, 2, -7, 12,
},
```

```cpp
kv_key(kv, "data");
int count = 0;
kv_array_begin(kv, &count);

for (int i = 0; i < count; ++i) {
	int val = 0;
	kv_val(kv, &val);
	printf("%d\n", val);
}
```

Which prints the following output.

```
5
3
2
-7
12
```

## Strings

Strings in kv are dealt with by the `kv_val_string` string function, just like the other `kv_val` functions.

```cpp
const char* string = "Hello.";
size_t size = strlen(string);

kv_key(kv, "data");
kv_val_string(kv, &string, &size);
```

## Base64 Blobs

Blobs in kv are for storing binary data. The data is encoded in [base64 format](https://en.wikipedia.org/wiki/Base64). The purpose of using base64 format is to ensure the binary data is safe to copy + paste manually and transmit via text without bugs.

Blobs are serialized as a string. If the contents of the string form a balid base64 blob then `kv_val_blob` can be used instead of `kv_val_string`. `kv_val_blob` performs base64 encoding and decoding internally, depending on if the kv is in read or write mode.

For example say we have this blob in a kv formatted buffer.

```
blob = "SGVsbG8gdGhlcmUgOik=",
```

It can be decoded and printed like so.

```cpp
size_t blob_len = 0;
kv_key(kv, "blob");
kv_val_blob(kv, buffer, size, &blob_len);
printf("%.*s\n", buffer, blob_len);
```

Which prints the following output.

```
Hello there :)
```

## Type Conversions

When parsing kv stores all integers in 64-bit format internally. Similarly all floats are stored internally as doubles. Whenever `kv_val` is called the requested type of the `val` parameter will be typecasted internally when dealing with integers and floats.

For example if we have this kv string.

```
val = 100.5,
```

The value can be pulled out of the string in a few ways, all completely valid with different internal typecasts.

```cpp
uint64_t u64;
float f;
double d;
char c;

kv_key(kv, "val"); kv_val(kv, &u64); // u64 is now 100
kv_key(kv, "val"); kv_val(kv, &f);   // f is now 100.5f
kv_key(kv, "val"); kv_val(kv, &d);   // d is now 100.5
kv_key(kv, "val"); kv_val(kv, &c);   // c is now 100
```

## Data Inheritence

## Delta Encoding

