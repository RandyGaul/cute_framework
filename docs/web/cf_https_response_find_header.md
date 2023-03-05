# cf_https_response_find_header | [web](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/README.md) | [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)

Helper function find a specific header in a [CF_HttpsResponse](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_httpsresponse.md).

```cpp
bool cf_https_response_find_header(const CF_HttpsResponse* response, const char* header_name, CF_HttpsHeader* header_out)
```

## Return Value

Returns true the header was found. The header will be written to `header_out`. Case is ignored.

## Related Pages

[CF_Https](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https.md)  
[CF_HttpsResponse](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_httpsresponse.md)  
[cf_https_response](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_response.md)  
[CF_HttpsString](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_httpsstring.md)  
[CF_HttpsHeader](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_httpsheader.md)  
