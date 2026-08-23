#define CUTE_H264_IMPLEMENTATION
#include "libraries/cute/cute_h264.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char** argv)
{
	const char* in = argc > 1 ? argv[1] : "crash.264";
	FILE* fp = fopen(in, "rb");
	fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, 0, SEEK_SET);
	unsigned char* b = (unsigned char*)malloc((size_t)n);
	if (fread(b, 1, (size_t)n, fp) != (size_t)n) return 1;
	fclose(fp);
	ch_decoder_t* d = ch_decoder_make(b, (int)n);
	int got = 0;
	while (ch_decoder_next(d)) { if (!ch_decoder_rgba(d)) break; if (++got > 64) break; }
	printf("frames %d err %s\n", got, ch_decoder_error ? ch_decoder_error : "(none)");
	ch_decoder_destroy(d);
	free(b);
	return 0;
}
