[](../header.md ':include')

# cf_https_response_find_header

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Searches for and returns a header by name.

```cpp
CF_HttpsHeader cf_https_response_find_header(CF_HttpsResponse response, const char* header_name);
```

Parameters | Description
--- | ---
response | The HTTP response.
header_name | The name of the header to search for.

## Related Pages

[CF_HttpsHeader](/web/cf_httpsheader.md)  
[CF_HttpsResponse](/web/cf_httpsresponse.md)  
[cf_https_response_headers](/web/cf_https_response_headers.md)  
