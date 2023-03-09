# CF_BASE64_DECODED_SIZE

Category: [base64](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=base64)  
GitHub: [cute_base64.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_base64.h)  
---

Calculates the size of data after base64 decoding it.

```cpp
#define CF_BASE64_DECODED_SIZE(size) ((((size) + 3) / 4) * 3)
```

Parameters | Description
--- | ---
size | The size of base64 encoded data.

## Return Value

Returns the number of bytes the raw dencoded data will take up. This will deflate the size ~33%.

## Remarks

Use this for the `dst_size` in [cf_base64_decode](https://github.com/RandyGaul/cute_framework/blob/master/docs/base64/cf_base64_decode.md).
Base64 encoding is useful for storing data as text in a copy-paste-safe manner. For more information about
base64 encoding see this link: [RFC-4648](https://tools.ietf.org/html/rfc4648) or [Wikipedia Base64](https://en.wikipedia.org/wiki/Base64).

## Related Pages

[CF_BASE64_ENCODED_SIZE](https://github.com/RandyGaul/cute_framework/blob/master/docs/base64/cf_base64_encoded_size.md)  
[cf_base64_decode](https://github.com/RandyGaul/cute_framework/blob/master/docs/base64/cf_base64_decode.md)  
[cf_base64_encode](https://github.com/RandyGaul/cute_framework/blob/master/docs/base64/cf_base64_encode.md)  
