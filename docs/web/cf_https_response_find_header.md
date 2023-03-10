[](../header.md ':include')

# cf_https_response_find_header

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Helper function find a specific header in a [CF_HttpsResponse](/web/cf_httpsresponse.md).

```cpp
CF_INLINE bool cf_https_response_find_header(const CF_HttpsResponse* response, const char* header_name, CF_HttpsHeader* header_out)
```

## Return Value

Returns true the header was found. The header will be written to `header_out`. Case is ignored.

## Related Pages

[CF_Https](/web/cf_https.md)  
[CF_HttpsResponse](/web/cf_httpsresponse.md)  
[cf_https_response](/web/cf_https_response.md)  
[CF_HttpsString](/web/cf_httpsstring.md)  
[CF_HttpsHeader](/web/cf_httpsheader.md)  
