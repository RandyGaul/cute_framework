# kv - Key Value Serialization

"Key-value", or kv. Used for text-based serialization either to/from an in-memory buffer. The design of the kv api is supposed to allow for *mostly* (but not completely) the same code to be used for both to and from buffer.

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
- base64 encoded blobs, as strings
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
kv_val(kv, &val);
printf("a was %d\n", val);

kv_key(kv, "b");
kv_val(kv, &val);
printf("b was %d\n", val);
```

Which would print the following.

```
a was 10
b was 12
```

## Write Mode

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

## Strings

## Base64 Blobs

## Type Conversions

## Data Inheritence

## Delta Encoding

