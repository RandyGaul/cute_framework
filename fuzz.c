// Feeds the decoder damaged streams. A library that opens video files will eventually be handed a
// truncated download or a deliberately malformed file, and the only acceptable outcomes are a
// picture or a refusal -- never a crash, and never a read outside its own buffers.
#define CUTE_H264_IMPLEMENTATION
#include "libraries/cute/cute_h264.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned s = 1;
static unsigned rnd(void) { s = s * 1103515245u + 12345u; return s >> 8; }

int main(int argc, char** argv)
{
	const char* in = argc > 1 ? argv[1] : "out.264";
	int rounds = argc > 2 ? atoi(argv[2]) : 2000;

	FILE* fp = fopen(in, "rb");
	if (!fp) { printf("cannot open %s\n", in); return 1; }
	fseek(fp, 0, SEEK_END);
	long n = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char* orig = (unsigned char*)malloc((size_t)n);
	if (fread(orig, 1, (size_t)n, fp) != (size_t)n) return 1;
	fclose(fp);

	unsigned char* buf = (unsigned char*)malloc((size_t)n);
	int decoded = 0, refused = 0, frames = 0;
	for (int i = 0; i < rounds; ++i) {
		long len = n;
		memcpy(buf, orig, (size_t)n);
		int kind = i % 3;
		if (kind == 0) {
			len = 1 + (long)(rnd() % (unsigned)n);              // truncated
		} else if (kind == 1) {
			int flips = 1 + (int)(rnd() % 32);                  // a few flipped bits
			for (int k = 0; k < flips; ++k) buf[rnd() % (unsigned)n] ^= (unsigned char)(1u << (rnd() % 8));
		} else {
			int bytes = 1 + (int)(rnd() % 64);                  // a corrupt run
			long at = (long)(rnd() % (unsigned)n);
			for (int k = 0; k < bytes && at + k < n; ++k) buf[at + k] = (unsigned char)rnd();
		}

		if (getenv("TRACE")) { printf("round %d kind %d len %ld\n", i, kind, len); fflush(stdout); }
		if (getenv("SAVE")) { FILE* sf = fopen("crash.264", "wb"); fwrite(buf, 1, (size_t)len, sf); fclose(sf); }
		ch_decoder_t* d = ch_decoder_make(buf, (int)len);
		if (!d) { ++refused; continue; }
		int got = 0;
		while (ch_decoder_next(d)) {
			int w, h;
			ch_decoder_size(d, &w, &h);
			if (!ch_decoder_rgba(d)) break;                     // exercise the output path too
			++got;
			if (got > 64) break;
		}
		if (ch_decoder_error) ++refused; else ++decoded;
		frames += got;
		ch_decoder_destroy(d);
	}
	printf("%d damaged streams: %d refused cleanly, %d ran to the end, %d pictures produced\n",
		rounds, refused, decoded, frames);
	printf("no crash, no hang\n");
	free(orig); free(buf);
	return 0;
}
