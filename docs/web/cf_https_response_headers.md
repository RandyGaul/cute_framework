[](../header.md ':include')

# cf_https_response_headers

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Returns an array of response headers.

```cpp
htbl const CF_HttpsHeader* cf_https_response_headers(CF_HttpsResponse response);
```

## Remarks

Intended to be used with [cf_https_response_headers_count](/web/cf_https_response_headers_count.md). Do not free this array, it will
get cleaned up when the originating [CF_HttpsRequest](/web/cf_httpsrequest.md) is destroyed via [cf_https_destroy](/web/cf_https_destroy.md).
If you're familiar with [htbl](/hash/htbl.md) you may treat this pointer as a hashtable key'd by strings.

## Related Pages

[CF_HttpsHeader](/web/cf_httpsheader.md)  
[CF_HttpsResponse](/web/cf_httpsresponse.md)  
[cf_https_response_find_header](/web/cf_https_response_find_header.md)  
