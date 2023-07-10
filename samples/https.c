#include <cute.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	CF_HttpsRequest request = cf_https_get("www.google.com", 443, "/", true);

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
	FILE* fp = fopen("response.txt", "wb");
	if (!fp) {
		printf("Unable to open response.txt.\n");
		return -1;
	}
	fwrite(cf_https_response_content(response), cf_https_response_content_length(response), 1, fp);
	fclose(fp);

	cf_https_destroy(request);

	return 0;
}
