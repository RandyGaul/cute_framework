[](../header.md ':include')

# cf_https_process

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Processes an HTTPS request.

```cpp
CF_API size_t CF_CALL cf_https_process(CF_Https* https);
```

## Return Value

Returns the bytes recieved so far.

## Remarks

Since this API uses non-blocking sockets [cf_https_process](/web/cf_https_process.md) needs to be call periodically after [cf_https_get](/web/cf_https_get.md)
or [cf_https_post](/web/cf_https_post.md) is called for as long as [cf_https_state](/web/cf_https_state.md) returns `CF_HTTPS_STATE_PENDING`. You can call
this function from within its own loop, put it on another thread within a loop, or call it once per
game tick -- whichever you prefer.

## Related Pages

[CF_Https](/web/cf_https.md)  
[cf_https_get](/web/cf_https_get.md)  
[cf_https_post](/web/cf_https_post.md)  
[cf_https_destroy](/web/cf_https_destroy.md)  
[cf_https_state](/web/cf_https_state.md)  
[cf_https_response](/web/cf_https_response.md)  
