[](../header.md ':include')

# cf_https_response

Category: [web](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Returns a [CF_HttpsResponse](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_httpsresponse.md) from a request.

```cpp
CF_HttpsResponse cf_https_response(CF_Https* https);
```

## Remarks

A response can be retrieved from the `https` object after [cf_https_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_state.md) returns `CF_HTTPS_STATE_COMPLETED`.
Calling this function otherwise will get you a NULL pointer returned. This will get cleaned up automatically
when [cf_https_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_destroy.md) is called.

## Related Pages

[CF_Https](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https.md)  
[cf_https_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_get.md)  
[cf_https_post](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_post.md)  
[cf_https_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_destroy.md)  
[cf_https_process](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_process.md)  
[cf_https_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_state.md)  
