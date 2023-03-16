[](../header.md ':include')

# cf_https_response

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Returns a [CF_HttpsResponse](/web/cf_httpsresponse.md) from a request.

```cpp
CF_HttpsResponse cf_https_response(CF_Https* https);
```

## Remarks

A response can be retrieved from the `https` object after [cf_https_state](/web/cf_https_state.md) returns `CF_HTTPS_STATE_COMPLETED`.
Calling this function otherwise will get you a NULL pointer returned. This will get cleaned up automatically
when [cf_https_destroy](/web/cf_https_destroy.md) is called.

## Related Pages

[CF_Https](/web/cf_https.md)  
[cf_https_get](/web/cf_https_get.md)  
[cf_https_post](/web/cf_https_post.md)  
[cf_https_destroy](/web/cf_https_destroy.md)  
[cf_https_process](/web/cf_https_process.md)  
[cf_https_state](/web/cf_https_state.md)  
