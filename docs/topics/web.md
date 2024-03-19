[](../header.md ':include')

<br>

To access the web, as in websites, CF can make [HTTPS](https://en.wikipedia.org/wiki/HTTPS) requests. This can be used to download a webpage, or send a generic request to some webserver. Common usecases are to submit some high score information to a webserver, perform some authentication like a login system, or implement a [REST API](https://en.wikipedia.org/wiki/REST).

## Sending a Request

To send an [HTTPS get request](https://en.wikipedia.org/wiki/HTTP#Request_methods) call [`cf_https_get`](https://randygaul.github.io/cute_framework/#/web/cf_https_get), and for [post](https://en.wikipedia.org/wiki/POST_(HTTP)) call [`cf_https_post`](https://randygaul.github.io/cute_framework/#/web/cf_https_post). These will give you a [CF_HttpsRequest](https://randygaul.github.io/cute_framework/#/web/cf_httpsrequest) in return.

Here's a [full working example](https://github.com/RandyGaul/cute_framework/blob/master/samples/https.c) to download and print the front page of google.com:

```cpp
#include <cute.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	const char* hostname = "www.google.com";
	CF_HttpsRequest request = cf_https_get(hostname, 443, "/", true);

	while (1) {
		CF_HttpsResult state = cf_https_process(request);
		if (state < 0) {
			printf("%s\n", cf_https_result_to_string(state));
			cf_https_destroy(request);
			return -1;
		}
		if (state == CF_HTTPS_RESULT_OK) {
			printf("Connected!\n");
			break;
		}
	}

	CF_HttpsResponse response = cf_https_response(request);
	FILE* fp = fopen("response.txt", "wb");
	if (!fp) {
		printf("Unable to open response.txt.\n");
		return -1;
	}
	fwrite(cf_https_response_content(response), cf_https_response_content_length(response), 1, fp);
	fclose(fp);
	cf_https_destroy(request);
	printf("Saved response in response.txt\n");

	return 0;
}
```

## Headers

In the previous section's example we can see continual calling of [`cf_https_process`](https://randygaul.github.io/cute_framework/#/web/cf_https_process) until the request is completed. Once done we can grab a [CF_HttpsResponse](https://randygaul.github.io/cute_framework/#/web/cf_httpsresponse) out of the request, which is filled with [HTTP headers](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers). The headers contain lots of useful information about the response from the server.

Each header is a key-value pair of strings, as defined by the HTTP specs. It's up to the server to send you headers that make sense, and it's up to you to read them if you care about them. Typically a small number of headers are relevant, or maybe none, while lots of useful information is returned to you within the response's content. To look at a particular header call [`cf_https_response_find_header`](https://randygaul.github.io/cute_framework/#/web/cf_https_response_find_header) to fetch a particular [CF_HttpsHeader](https://randygaul.github.io/cute_framework/#/web/cf_httpsheader). To get the respone's content call [`cf_https_response_content`](https://randygaul.github.io/cute_framework/#/web/cf_https_response_content), as in the example from the previous section.
