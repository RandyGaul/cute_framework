[](../header.md ':include')

# CF_HttpsHeader

Category: [web](/api_reference?id=web)  
GitHub: [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)  
---

Represents an [HTTPS header](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers).

Struct Members | Description
--- | ---
`const char* name` | The name of the header, also known as the key of the header.
`const char* value` | The value of the header.

## Remarks

Headers may be fetched from a [CF_HttpsResponse](/web/cf_httpsresponse.md) by calling [cf_https_response_find_header](/web/cf_https_response_find_header.md), or [cf_https_response_headers](/web/cf_https_response_headers.md).

## Related Pages

[CF_HttpsRequest](/web/cf_httpsrequest.md)  
[CF_HttpsResponse](/web/cf_httpsresponse.md)  
[cf_https_response_headers](/web/cf_https_response_headers.md)  
[cf_https_response_find_header](/web/cf_https_response_find_header.md)  
