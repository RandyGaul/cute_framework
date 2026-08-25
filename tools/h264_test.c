// Conformance harness for cute_h264.h. Encodes a synthetic pattern, then the caller decodes it
// with ffmpeg and compares against the raw dump this also writes. I_PCM is lossless, so the
// comparison is EXACT -- any difference at all is a bug, which is exactly the property worth
// having while the bitstream layer is being built.
#define CUTE_H264_IMPLEMENTATION
#include "cute/cute_h264.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// The encoder's own reconstruction of each picture, collected as it is coded. Coded order is not
// display order once B pictures are on, so these are sorted before being written: a decoder is
// only obliged to reproduce them in the order they are meant to be SHOWN.
#define MAX_PICS 4096
static unsigned char* g_buf[MAX_PICS];
static int g_poc[MAX_PICS];
static int g_n;
static int g_w, g_h;
static FILE* g_yuv;

static void plane_copy(unsigned char* dst, const unsigned char* src, int stride, int w, int h)
{
	for (int y = 0; y < h; ++y) memcpy(dst + (size_t)y * w, src + (size_t)y * stride, (size_t)w);
}

static void on_recon(void* udata, const void* y, const void* cb, const void* cr,
                     int luma_stride, int chroma_stride, int poc)
{
	(void)udata;
	if (g_n >= MAX_PICS) return;
	size_t n = (size_t)g_w * g_h + (size_t)(g_w / 2) * (g_h / 2) * 2;
	unsigned char* p = (unsigned char*)malloc(n);
	plane_copy(p, (const unsigned char*)y, luma_stride, g_w, g_h);
	plane_copy(p + (size_t)g_w * g_h, (const unsigned char*)cb, chroma_stride, g_w / 2, g_h / 2);
	plane_copy(p + (size_t)g_w * g_h + (size_t)(g_w / 2) * (g_h / 2), (const unsigned char*)cr,
		chroma_stride, g_w / 2, g_h / 2);
	g_buf[g_n] = p;
	g_poc[g_n] = poc;
	++g_n;
}

// A keyframe restarts the order count, so a count of zero starts a new group rather than sorting
// before everything that came before it.
static void flush_group(int first, int last)
{
	size_t n = (size_t)g_w * g_h + (size_t)(g_w / 2) * (g_h / 2) * 2;
	for (int a = first; a < last; ++a) {
		int best = a;
		for (int b = a + 1; b < last; ++b) if (g_poc[b] < g_poc[best]) best = b;
		unsigned char* t = g_buf[a]; g_buf[a] = g_buf[best]; g_buf[best] = t;
		int s = g_poc[a]; g_poc[a] = g_poc[best]; g_poc[best] = s;
		fwrite(g_buf[a], 1, n, g_yuv);
	}
}

static void write_all(void)
{
	int first = 0;
	for (int i = 1; i <= g_n; ++i) {
		if (i == g_n || g_poc[i] == 0) { flush_group(first, i); first = i; }
	}
	for (int i = 0; i < g_n; ++i) free(g_buf[i]);
}

int main(int argc, char** argv)
{
	int w = argc > 1 ? atoi(argv[1]) : 320;
	int h = argc > 2 ? atoi(argv[2]) : 176;
	int frames = argc > 3 ? atoi(argv[3]) : 6;
	int qp = argc > 4 ? atoi(argv[4]) : 26;
	int flat = argc > 5 ? atoi(argv[5]) : 0;

	unsigned char* rgba = (unsigned char*)malloc((size_t)w * h * 4);
	ch_encoder_t* e = ch_encoder_make(w, h, 30);
	if (!e) { printf("make failed: %s\n", ch_error_reason); return 1; }
	ch_encoder_qp(e, qp);
	{ const char* s = getenv("CABAC"); if (s && atoi(s)) ch_encoder_cabac(e, 1); }
	{ const char* s = getenv("REFS"); if (s) ch_encoder_ref_frames(e, atoi(s)); }
	{ const char* s = getenv("BFRAMES"); if (s) ch_encoder_bframes(e, atoi(s)); }
	g_w = w; g_h = h;
	ch_encoder_recon_callback(e, on_recon, NULL);

	FILE* raw = fopen("ref.rgba", "wb");
	g_yuv = fopen("ref.yuv", "wb");
	for (int f = 0; f < frames; ++f) {
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				unsigned char* p = rgba + ((size_t)y * w + x) * 4;
				// Hard edges, saturated primaries and a moving element: the combination that
				// catches colour-matrix, stride and chroma-siting mistakes. A smooth gradient
				// would hide all three.
				int cx = (x * 8 / w), cy = (y * 8 / h);
				int checker = (cx + cy) & 1;
				int bar = (x + f * 7) % w < w / 8;
				if (flat == 1) { p[0] = 90; p[1] = 140; p[2] = 60; p[3] = 255; continue; }
				if (flat == 2) {
					// A smooth field translated by a NON-integer amount each frame. The pattern
					// above shifts by whole pixels, which a broken sub-sample interpolator would
					// survive untouched because the motion search would never ask it for one.
					double sx = x - f * 1.4, sy = y - f * 0.7;
					int v = (int)(127.5 + 100.0 * sin(sx * 0.11) * cos(sy * 0.09));
					p[0] = (unsigned char)v;
					p[1] = (unsigned char)(255 - v);
					p[2] = (unsigned char)((v * 3) & 255);
					p[3] = 255;
					continue;
				}
				p[0] = (unsigned char)(checker ? 255 : (x * 255 / (w - 1)));
				p[1] = (unsigned char)(bar ? 255 : (y * 255 / (h - 1)));
				p[2] = (unsigned char)(checker ? (f * 37) & 255 : 24);
				p[3] = 255;
			}
		}
		fwrite(rgba, 1, (size_t)w * h * 4, raw);
		if (!ch_encoder_frame(e, rgba)) { printf("frame failed: %s\n", ch_error_reason); return 1; }
	}
	fclose(raw);

	if (!ch_encoder_save(e, "out.264")) { printf("save failed: %s\n", ch_error_reason); return 1; }
	// The same stream wrapped as an MP4, which is what cute-h264-mp4-test compares against.
	if (!ch_encoder_save_mp4(e, "out.mp4")) { printf("mp4 save failed: %s\n", ch_error_reason); return 1; }
	// Dump the encoder's OWN yuv420p, cropped, in ffmpeg's plane order. I_PCM is lossless in YUV
	// but not in RGB -- 4:2:0 throws chroma away before the codec ever sees it -- so this is the
	// buffer a conformant decoder has to reproduce exactly. Encoder and decoder run the same
	// inverse transform and the same prediction, so any difference at all is a bug in one of
	// them, and that is what makes the round trip a conformance test rather than a PSNR eyeball.
	write_all();
	fclose(g_yuv);
	int size = 0;
	ch_encoder_data(e, &size);
	printf("wrote out.264: %d bytes, %d frames, %dx%d (%.2f bytes/pixel)\n",
		size, frames, w, h, (double)size / ((double)w * h * frames));
	ch_encoder_destroy(e);
	free(rgba);
	return 0;
}
