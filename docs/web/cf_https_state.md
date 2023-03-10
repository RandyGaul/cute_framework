[](../header.md ':include')

# cf_https_state

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Returns the current state of the `https` object.

```cpp
CF_API CF_HttpsState CF_CALL cf_https_state(CF_Https* https);
```

## Remarks

This is used mainly for calling `https_process`.

## Related Pages

[CF_Https](/web/cf_https.md)  
[cf_https_get](/web/cf_https_get.md)  
[cf_https_post](/web/cf_https_post.md)  
[cf_https_destroy](/web/cf_https_destroy.md)  
[cf_https_process](/web/cf_https_process.md)  
[cf_https_response](/web/cf_https_response.md)  
