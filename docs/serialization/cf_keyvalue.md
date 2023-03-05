# CF_KeyValue | [serialization](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/README.md) | [cute_kv.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_kv.h)

An opaque pointer representing a Key-value instance, for serializing data to text.

## Remarks

"Key-value", or KV for short, is a serialization API designed to be very similar to JSON. It's a
text-based format to store any kind of data in key-value format. This key-value style has some
great advantages for serialziation, such as versioning your data, simplicity (human readable),
and a low-friction API.

You start by opening a KV in either read or write format. For 95% of use cases you can then get
away with using the same code for reading and writing data, with only the difference of opening
in read/write mode with [cf_kv_read](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_read.md) or [cf_kv_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_write.md).

Here's a quick example of some KV data.

```cpp
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

Here are the supported data types in KV:

- integers
- floats
- strings
- base64 blobs, as strings
- arrays
- objects

Reading data with KV is all about calling `cf_kv_key(kv, "key_name")` to lookup a serialized
field. If found, you can call the proper `kv_val_` function to fetch the data for that key.
For example, let's say we have this struct in our game containing some important info.

```cpp
struct ImportantStuff
{
    int state;
    float factor;
    int value_count;
    int values[MAX_ENTRIES];
};
```

We could save the `ImportantStuff` to a KV text file with some code like this:

```cpp
bool serialize(CF_KeyValue kv, ImportantStuff stuff)
{
    if (cf_kv_key("state"))       cf_kv_val_int32(kv, &stuff->state);
    if (cf_kv_key("factor"))      cf_kv_val_float(kv, &stuff->factor);
    if (cf_kv_key("value_count")) cf_kv_val_int32(kv, &stuff->value_count);

    if (cf_kv_object_begin(kv, &stuff->value_count, "values")) {
        for (int i = 0; i < stuff->value_count; ++i) {
            cf_kv_val_int32(kv, stuff->values + i);
        }
        cf_kv_object_end(kv);
    }
    
    return !cf_is_error(cf_kv_last_error(kv));
}

bool save(ImportantStuff stuff, const char path)
{
     CF_KeyValue kv = kv_write();
     if (!serialize(kv, stuff)) {
         cf_kv_destroy(kv);
         return NULL;
     }
     bool result = !cf_is_error(cf_fs_write_entire_buffer_to_file(cf_kv_buffer(kv), cf_kv_buffer_size(kv)));
     cf_kv_destroy(kv);
     return result;
}
```

The text file would then look something like this:

```cpp
state = 10
factor = 1.3
value_count = 3
values = [3] {
    7, -3, 20,
}
```

The bulk of the work happens in the `serialize` function from the example above. We can
reuse this entire function to also read back the KV data into an `ImportantStuff` struct
by making a similar function to `save`.

```cpp
bool load(ImportantStuff stuff, const char path)
{
     void data;
     size_t size;
     if (cf_is_error(cf_fs_read_entire_file_to_memory(path, &data, &size))) return false;
     CF_KeyValue kv = kv_read(data, size);
     if (!kv) return false;
     bool result = serialize(kv, stuff);
     cf_kv_destroy(kv);
     return result;
}
```

In the common case it's possible to reuse most seriaization code for both reading and
writing. However, sometimes it's necessary to have entirely different code for reading
and writing. Use [cf_kv_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_state.md) to see if the KV is currently in read vs write mode, and then
run different code accordingly.

## Related Pages

[cf_kv_write](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_write.md)  
[CF_KeyValueState](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_keyvaluestate.md)  
[cf_kv_read](https://github.com/RandyGaul/cute_framework/blob/master/docs/serialization/cf_kv_read.md)  
