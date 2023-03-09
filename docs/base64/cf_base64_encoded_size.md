[](../header.md ':include')

# CF_BASE64_ENCODED_SIZE

Category: [base64](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=base64)  
GitHub: [cute_base64.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_base64.h)  
---

Calculates the size of data after base64 encoding it.

```cpp
#define CF_BASE64_ENCODED_SIZE(size) ((((size) + 2) / 3) * 4)
```

Parameters | Description
--- | ---
size | The size of un-encoded raw data.

## Return Value

Returns the number of bytes the base64 encoded data will take up. This will inflate the size ~33%.

## Remarks

Use this for the `dst_size` in [cf_base64_encode](https://github.com/RandyGaul/cute_framework/blob/master/docs/base64/cf_base64_encode.md).
Base64 encoding is useful for storing data as text in a copy-paste-safe manner. For more information about
base64 encoding see this link: [RFC-4648](https://tools.ietf.org/html/rfc4648) or [Wikipedia Base64](https://en.wikipedia.org/wiki/Base64).

## Related Pages

[cf_base64_decode](https://github.com/RandyGaul/cute_framework/blob/master/docs/base64/cf_base64_decode.md)  
[CF_BASE64_DECODED_SIZE](https://github.com/RandyGaul/cute_framework/blob/master/docs/base64/cf_base64_decoded_size.md)  
[cf_base64_encode](https://github.com/RandyGaul/cute_framework/blob/master/docs/base64/cf_base64_encode.md)  
