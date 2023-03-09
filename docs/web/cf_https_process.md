[](../header.md ':include')

# cf_https_process

Category: [web](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Processes an HTTPS request.

```cpp
size_t cf_https_process(CF_Https* https);
```

## Return Value

Returns the bytes recieved so far.

## Remarks

Since this API uses non-blocking sockets [cf_https_process](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_process.md) needs to be call periodically after [cf_https_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_get.md)
or [cf_https_post](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_post.md) is called for as long as [cf_https_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_state.md) returns `CF_HTTPS_STATE_PENDING`. You can call
this function from within its own loop, put it on another thread within a loop, or call it once per
game tick -- whichever you prefer.

## Related Pages

[CF_Https](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https.md)  
[cf_https_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_get.md)  
[cf_https_post](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_post.md)  
[cf_https_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_destroy.md)  
[cf_https_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_state.md)  
[cf_https_response](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_response.md)  
