[](../header.md ':include')

# cf_https_post

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Creates an HTTPS POST request.

```cpp
CF_HttpsRequest cf_https_post(const char* host, int port, const char* uri, const void* content, int content_length, bool verify_cert);
```

Parameters | Description
--- | ---
host | The address of the host, e.g. "www.google.com".
port | The port number to connect over, typically 443 for common web traffic.
uri | The address of the resource the request references, e.g. "/index.html".
content | The content to send along with the POST request.
content_length | Length in bytes of the `content` string.
verify_cert | Recommended as true. Set to true to verify the server certificate (you want this on).

## Return Value

Returns a [CF_HttpsRequest](/web/cf_httpsrequest.md) for processing the get request and receiving a response.

## Remarks

You should continually call [cf_https_process](/web/cf_https_process.md) on the [CF_HttpsRequest](/web/cf_httpsrequest.md). See [CF_HttpsRequest](/web/cf_httpsrequest.md) for details.

## Related Pages

[CF_HttpsRequest](/web/cf_httpsrequest.md)  
[cf_https_get](/web/cf_https_get.md)  
[cf_https_response](/web/cf_https_response.md)  
[cf_https_destroy](/web/cf_https_destroy.md)  
[cf_https_process](/web/cf_https_process.md)  
