[](../header.md ':include')

# cf_base64_decode

Category: [base64](/api_reference?id=base64)  
GitHub: [cute_base64.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_base64.h)  
---

Decodes base64 encoded data into raw bytes.

```cpp
CF_Result cf_base64_decode(void* dst, size_t dst_size, const void* src, size_t src_size);
```

Parameters | Description
--- | ---
dst | The destination buffer, where raw decoded data is written to.
dst_size | The size of `dst` in bytes. You can use [CF_BASE64_DECODED_SIZE](/base64/cf_base64_decoded_size.md) on `src_size` to calculate this value.
src | Base64 encoded bytes.
src_size | The size of `src` in bytes.

## Return Value

Returns a [CF_Result](/utility/cf_result.md) containing information about any errors.

## Remarks

Base64 encoding is useful for storing data as text in a copy-paste-safe manner. For more information about
base64 encoding see this link: [RFC-4648](https://tools.ietf.org/html/rfc4648) or [Wikipedia Base64](https://en.wikipedia.org/wiki/Base64).

## Related Pages

[CF_BASE64_ENCODED_SIZE](/base64/cf_base64_encoded_size.md)  
[CF_BASE64_DECODED_SIZE](/base64/cf_base64_decoded_size.md)  
[cf_base64_encode](/base64/cf_base64_encode.md)  
