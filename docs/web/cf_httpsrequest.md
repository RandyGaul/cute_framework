[](../header.md ':include')

# CF_HttpsRequest

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Represents an [HTTPS request](https://www.ibm.com/docs/en/cics-ts/5.3?topic=protocol-http-requests).

## Code Example

> Creating a request and waiting for a response.

```cpp
int main(int argc, char argv[])
{
    const char hostname = "www.google.com";
    //const char hostname = "badssl.com";
    //const char hostname = "expired.badssl.com";
    //const char hostname = "wrong.host.badssl.com";
    //const char hostname = "self-signed.badssl.com";
    //const char hostname = "untrusted-root.badssl.com";
    CF_HttpsRequest request = cf_https_get(hostname, 443, "/", true);

    while (1) {
        CF_HttpsResult state = cf_https_process(request);
        if (state < 0) {
            printf("%s\n", cf_https_result_to_string(state));
            cf_https_destroy(request);
            return -1;
        }
        if (state == CF_HTTPS_RESULT_OK) {
            break;
        }
    }

    CF_HttpsResponse response = cf_https_response(request);
    const char content = cf_https_response_content(response);
    int length = cf_https_response_content_length(response);
    printf("%.s", length, content);
    cf_https_destroy(request);

    return 0;
}
```

## Remarks

You may create a request by calling either [cf_https_get](/web/cf_https_get.md) or [cf_https_post](/web/cf_https_post.md). It is intended to continually
call [cf_https_process](/web/cf_https_process.md) in a loop until the request generates a response, or fails.

## Related Pages

[cf_https_post](/web/cf_https_post.md)  
[CF_HttpsResponse](/web/cf_httpsresponse.md)  
[cf_https_get](/web/cf_https_get.md)  
