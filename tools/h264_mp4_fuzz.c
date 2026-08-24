// Feeds ch_mp4_unwrap damaged containers. A file picked off disk is untrusted input like any
// other, and the box sizes inside it are exactly the sort of thing that gets corrupted.
#define CUTE_H264_IMPLEMENTATION
#include "cute/cute_h264.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned s = 1;
static unsigned rnd(void) { s = s * 1103515245u + 12345u; return s >> 8; }

int main(int argc, char** argv)
{
	const char* in = argc > 1 ? argv[1] : "out.mp4";
	int rounds = argc > 2 ? atoi(argv[2]) : 2000;
	FILE* fp = fopen(in, "rb");
	if (!fp) { printf("cannot open %s\n", in); return 1; }
	fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, 0, SEEK_SET);
	unsigned char* orig = (unsigned char*)malloc((size_t)n);
	if (fread(orig, 1, (size_t)n, fp) != (size_t)n) return 1;
	fclose(fp);

	unsigned char* buf = (unsigned char*)malloc((size_t)n);
	int ok = 0, refused = 0, frames = 0;
	for (int i = 0; i < rounds; ++i) {
		long len = n;
		memcpy(buf, orig, (size_t)n);
		if (i % 3 == 0) len = (long)(rnd() % (unsigned)n);         // truncated
		int hits = 1 + (int)(rnd() % 24);
		for (int k = 0; k < hits && len > 0; ++k) buf[rnd() % (unsigned)len] = (unsigned char)rnd();
		int size = 0;
		const void* annexb = ch_mp4_unwrap(buf, (int)len, &size);
		if (!annexb) { ++refused; continue; }
		++ok;
		ch_decoder_t* d = ch_decoder_make(annexb, size);
		if (d) {
			while (ch_decoder_next(d)) { ch_decoder_rgba(d); ++frames; }
			ch_decoder_destroy(d);
		}
		CUTE_H264_FREE((void*)annexb);
	}
	printf("%d damaged containers: %d refused cleanly, %d unwrapped, %d pictures produced\n",
		rounds, refused, ok, frames);
	printf("no crash, no hang\n");
	return 0;
}
