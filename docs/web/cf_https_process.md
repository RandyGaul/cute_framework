[](../header.md ':include')

# cf_https_process

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Processes a request and generates a response.

```cpp
CF_HttpsResult cf_https_process(CF_HttpsRequest request);
```

## Return Value

Returns the status of the request [CF_HttpsResult](/web/cf_httpsresult.md).

## Remarks

You should call this function in a loop. See [CF_HttpsResult](/web/cf_httpsresult.md).

## Related Pages

[CF_HttpsResult](/web/cf_httpsresult.md)  
[cf_https_response](/web/cf_https_response.md)  
[cf_https_get](/web/cf_https_get.md)  
[cf_https_post](/web/cf_https_post.md)  
