// Round-trips the arithmetic coder on its own, before any syntax is built on top of it. Encoder
// and decoder walk adaptive state in lockstep with no resynchronisation, so if the engine is even
// slightly wrong every higher-level bug will look like a syntax bug instead.
#define CUTE_H264_IMPLEMENTATION
#include "libraries/cute/cute_h264.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned rng_state = 12345;
static unsigned rnd(void) { rng_state = rng_state * 1103515245u + 12345u; return rng_state >> 16; }

static int trial(int n, int qp, int table, int seed, int bias)
{
	rng_state = (unsigned)seed;
	int* kind = (int*)malloc(sizeof(int) * n);
	int* ctxi = (int*)malloc(sizeof(int) * n);
	int* bins = (int*)malloc(sizeof(int) * n);
	for (int i = 0; i < n; ++i) {
		unsigned r = rnd();
		kind[i] = (r % 16) == 0 ? 1 : 0;                  // occasionally a bypass bin
		ctxi[i] = (int)(rnd() % (CH_CTX_COUNT - 1));
		// A skewed source is the interesting case: that is where the model should be winning.
		bins[i] = (int)(rnd() % 100) < bias ? 1 : 0;
	}

	ch_bits_t w;
	memset(&w, 0, sizeof(w));
	ch_cabac_t enc;
	ch_cabac_start(&enc, &w, qp, table);
	for (int i = 0; i < n; ++i) {
		ch_cabac_terminate(&enc, 0);                      // as a slice does before each macroblock
		if (kind[i]) ch_cabac_bypass(&enc, bins[i]);
		else ch_cabac_encode(&enc, ctxi[i], bins[i]);
	}
	ch_cabac_terminate(&enc, 1);
	int bits = w.bytes.len * 8 + w.nbits;
	while (w.nbits) ch_put_bit(&w, 1);

	ch_rbits_t r;
	r.data = w.bytes.data; r.len = w.bytes.len; r.pos = 0; r.error = 0;
	ch_cabac_dec_t dec;
	ch_cabac_dec_start(&dec, &r, qp, table);
	int bad = -1;
	for (int i = 0; i < n; ++i) {
		if (ch_cabac_dec_terminate(&dec)) { bad = i; break; }
		int v = kind[i] ? ch_cabac_dec_bypass(&dec) : ch_cabac_decode(&dec, ctxi[i]);
		if (v != bins[i]) { bad = i; break; }
	}
	if (bad < 0 && !ch_cabac_dec_terminate(&dec)) bad = n;

	double bpb = (double)bits / n;
	printf("  %6d bins, qp %2d, table %d, %3d%% ones: %-8s %.3f bits/bin\n",
		n, qp, table, bias, bad < 0 ? "match" : "MISMATCH", bpb);
	if (bad >= 0) printf("      first divergence at bin %d\n", bad);
	free(kind); free(ctxi); free(bins);
	return bad < 0;
}

int main(void)
{
	int ok = 1;
	printf("arithmetic coder round trip:\n");
	for (int qp = 0; qp <= 51; qp += 17)
		for (int tbl = 0; tbl < 4; ++tbl)
			ok &= trial(5000, qp, tbl, 1 + qp * 7 + tbl, 50);
	printf("skewed sources, where the model should beat one bit per bin:\n");
	for (int bias = 2; bias <= 98; bias += 24) ok &= trial(20000, 26, 0, 99 + bias, bias);
	printf(ok ? "\nengine OK\n" : "\nENGINE BROKEN\n");
	return ok ? 0 : 1;
}
