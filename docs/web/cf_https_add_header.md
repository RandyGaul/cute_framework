[](../header.md ':include')

# cf_https_add_header

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Adds a header to the request.

```cpp
void cf_https_add_header(CF_HttpsRequest request, const char* name, const char* value);
```

Parameters | Description
--- | ---
request | The request.
name | The key value of the header.
value | String representation of the header's value.

## Remarks

You should call this before calling [cf_https_process](/web/cf_https_process.md). Calling this after [cf_https_process](/web/cf_https_process.md) will break things.

## Related Pages

[CF_HttpsRequest](/web/cf_httpsrequest.md)  
[cf_https_get](/web/cf_https_get.md)  
[cf_https_post](/web/cf_https_post.md)  
[cf_https_destroy](/web/cf_https_destroy.md)  
[cf_https_process](/web/cf_https_process.md)  
[cf_https_response](/web/cf_https_response.md)  
