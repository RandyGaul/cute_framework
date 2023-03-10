[](../header.md ':include')

# cf_https_destroy

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Frees up all memory and closes the underlying HTTPS connection if still open.

```cpp
CF_API void CF_CALL cf_https_destroy(CF_Https* https);
```

Parameters | Description
--- | ---
https | A [CF_Https](/web/cf_https.md) to destroy.

## Related Pages

[CF_Https](/web/cf_https.md)  
[cf_https_get](/web/cf_https_get.md)  
[cf_https_post](/web/cf_https_post.md)  
[cf_https_process](/web/cf_https_process.md)  
