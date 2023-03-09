[](../header.md ':include')

# CF_Https

Category: [web](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Represents a single HTTPS request for clients to talk with web servers.

## Remarks

POST and GET requests are supported for when you just need a basic way to communicate over HTTPS. Insecure HTTP is not supported,
but cert verification can be skipped (not recommended).

Supports chunked encoding. Does not support trailing headers, 100-continue, keep-alive, or other
"advanced" HTTP features.

The design of this API comes mainly from Mattias Gustavsson's http.h single-file C header.
https://github.com/mattiasgustavsson/libs/blob/main/http.h

Here is a full working example.

```cpp
CF_Result err;
CF_Https https = cf_https_get("raw.githubusercontent.com", "443", "/RandyGaul/cute_framework/main/src/cute_https.h", &err);
if (https) {
    while (cf_https_state(https) == CF_HTTPS_STATE_PENDING) {
        size_t bytes_read = cf_https_process(https);
        printf("Received %zu bytes...\n", bytes_read);
    }
    if (cf_https_state(https) == CF_HTTPS_STATE_COMPLETED) {
        const CF_HttpsResponse response;
        cf_https_response(https, &response);
        printf("%s", response.content);
    }
    cf_https_destroy(https);
} else {
    printf("HTTPS request failed: %s\n", err.details);
}
```

## Related Pages

[cf_https_post](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_post.md)  
[cf_https_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_get.md)  
