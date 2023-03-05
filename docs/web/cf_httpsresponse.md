# CF_HttpsResponse | [web](https://github.com/RandyGaul/cute_framework/blob/master/docs/web_readme.md) | [cute_https.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_https.h)

Represents the response from a server,

Struct Members | Description
--- | ---
`int code` | The HTTP response code.
`size_t content_len` | Length of the HTTP response content.
`const char* content` | The HTTP response content.
`const CF_HttpsHeader* headers` | Array of headers from the response. See [CF_HttpsHeader](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_httpsheader.md).
`int headers_count` | Number of headers in `headers`.
`int transfer_encoding_flags` | Flags from `TransferEncoding`. For example, if content is gzip'd, you can tell by using
	          something like so: `bool is_gzip = !!(response->transfer_encoding & CF_TRANSFER_ENCODING_GZIP);`.
	          
	          Please note that if the encoding is `CF_TRANSFER_ENCODING_CHUNKED` the `content` buffer will not
	          contain any chunked encoding -- all chunked data has been decoded already. For gzip/deflate
	          the `content` buffer will need to be decompressed by you.

## Remarks

After a successful loop via `cf_https_process (see `CF_Https`) a response can be fetched by calling `cf_https_response`.

## Related Pages

[CF_Https](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https.md)  
[CF_HttpsString](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_httpsstring.md)  
[CF_HttpsHeader](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_httpsheader.md)  
[CF_HttpsHeader](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_httpsheader.md)  
[cf_https_response](https://github.com/RandyGaul/cute_framework/blob/master/docs/web/cf_https_response.md)  
