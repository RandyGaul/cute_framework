#ifndef STB_COMPRESS_H
#define STB_COMPRESS_H

// Compresses with an algorithm very similar to DEFLATE.
// You must take a wild guess on how large to make the output buffer.
// Try input size * 2.
int stb_compress(void* dst, const void* input, int size);

// Returns the size of the uncompressed data.
int stb_decompress_length(const void* input);

// Decompresses the *compressed* input into the dst buffer.
// Returns the size of the uncompressed data upon success.
// dst should be `stb_decompress_length` in length.
int stb_decompress(void* dst, const void* input, int size);

#endif // STB_COMPRESS_H

#ifdef STB_COMPRESS_IMPLEMENTATION
#ifndef STB_COMPRESS_IMPLEMENTATION_ONCE
#define STB_COMPRESS_IMPLEMENTATION_ONCE

typedef unsigned int stb_uint;
typedef unsigned char stb_uchar;

static unsigned int stb_matchlen(stb_uchar *m1, stb_uchar *m2, stb_uint maxlen)
{
	stb_uint i;
	for (i=0; i < maxlen; ++i)
		if (m1[i] != m2[i]) return i;
	return i;
}

static stb_uchar *stb__out;
static FILE      *stb__outfile;
static stb_uint   stb__outbytes;

static void stb__write(unsigned char v)
{
	fputc(v, stb__outfile);
	++stb__outbytes;
}

#define stb_out(v)	do { if (stb__out) *stb__out++ = (stb_uchar) (v); else stb__write((stb_uchar) (v)); } while (0)
static void stb_out2(stb_uint v) { stb_out(v >> 8); stb_out(v); }
static void stb_out3(stb_uint v) { stb_out(v >> 16); stb_out(v >> 8); stb_out(v); }
static void stb_out4(stb_uint v) { stb_out(v >> 24); stb_out(v >> 16); stb_out(v >> 8 ); stb_out(v); }

static void outliterals(stb_uchar *in, int numlit)
{
	while (numlit > 65536) {
		outliterals(in,65536);
		in	 += 65536;
		numlit -= 65536;
	}

	if	  (numlit ==	 0)	;
	else if (numlit <=	32)	stb_out (0x000020 + numlit-1);
	else if (numlit <=  2048)	stb_out2(0x000800 + numlit-1);
	else /*  numlit <= 65536) */ stb_out3(0x070000 + numlit-1);

	if (stb__out) {
		memcpy(stb__out,in,numlit);
		stb__out += numlit;
	} else
		fwrite(in, 1, numlit, stb__outfile);
}

static int stb__window = 0x40000; // 256K

static int stb_not_crap(int best, int dist)
{
	return   ((best > 2  &&  dist <= 0x00100)
		|| (best > 5  &&  dist <= 0x04000)
		|| (best > 7  &&  dist <= 0x80000));
}

static  stb_uint stb__hashsize = 32768;

// note that you can play with the hashing functions all you
// want without needing to change the decompressor
#define stb__hc(q,h,c)	  (((h) << 7) + ((h) >> 25) + q[c])
#define stb__hc2(q,h,c,d)   (((h) << 14) + ((h) >> 18) + (q[c] << 7) + q[d])
#define stb__hc3(q,c,d,e)   ((q[c] << 14) + (q[d] << 7) + q[e])

static unsigned int stb__running_adler;

static unsigned int stb_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
	const unsigned long ADLER_MOD = 65521;
	unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
	unsigned long blocklen = buflen % 5552;

	unsigned long i;
	while (buflen) {
		for (i=0; i + 7 < blocklen; i += 8) {
			s1 += buffer[0], s2 += s1;
			s1 += buffer[1], s2 += s1;
			s1 += buffer[2], s2 += s1;
			s1 += buffer[3], s2 += s1;
			s1 += buffer[4], s2 += s1;
			s1 += buffer[5], s2 += s1;
			s1 += buffer[6], s2 += s1;
			s1 += buffer[7], s2 += s1;

			buffer += 8;
		}

		for (; i < blocklen; ++i)
			s1 += *buffer++, s2 += s1;

		s1 %= ADLER_MOD, s2 %= ADLER_MOD;
		buflen -= blocklen;
		blocklen = 5552;
	}
	return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static int stb_compress_chunk(stb_uchar *history,
	stb_uchar *start,
	stb_uchar *end,
	int length,
	int *pending_literals,
	stb_uchar **chash,
	stb_uint mask)
{
	(void)history;
	int window = stb__window;
	stb_uint match_max;
	stb_uchar *lit_start = start - *pending_literals;
	stb_uchar *q = start;

#define STB__SCRAMBLE(h)   (((h) + ((h) >> 16)) & mask)

	// stop short of the end so we don't scan off the end doing
	// the hashing; this means we won't compress the last few bytes
	// unless they were part of something longer
	while (q < start+length && q+12 < end) {
		int m;
		stb_uint h1,h2,h3,h4, h;
		stb_uchar *t;
		int best = 2, dist=0;

		if (q+65536 > end)
			match_max = (stb_uint)(end-q);
		else
			match_max = 65536;

#define stb__nc(b,d)  ((d) <= window && ((b) > 9 || stb_not_crap((int)(b),(int)(d))))

#define STB__TRY(t,p)  /* avoid retrying a match we already tried */ \
	if (p ? dist != (int)(q-t) : 1)							 \
	if ((m = stb_matchlen(t, q, match_max)) > best)	 \
	if (stb__nc(m,q-(t)))								\
	best = m, dist = (int)(q - (t))

		// rather than search for all matches, only try 4 candidate locations,
		// chosen based on 4 different hash functions of different lengths.
		// this strategy is inspired by LZO; hashing is unrolled here using the
		// 'hc' macro
		h = stb__hc3(q,0, 1, 2); h1 = STB__SCRAMBLE(h);
		t = chash[h1]; if (t) STB__TRY(t,0);
		h = stb__hc2(q,h, 3, 4); h2 = STB__SCRAMBLE(h);
		h = stb__hc2(q,h, 5, 6);		t = chash[h2]; if (t) STB__TRY(t,1);
		h = stb__hc2(q,h, 7, 8); h3 = STB__SCRAMBLE(h);
		h = stb__hc2(q,h, 9,10);		t = chash[h3]; if (t) STB__TRY(t,1);
		h = stb__hc2(q,h,11,12); h4 = STB__SCRAMBLE(h);
		t = chash[h4]; if (t) STB__TRY(t,1);

		// because we use a shared hash table, can only update it
		// _after_ we've probed all of them
		chash[h1] = chash[h2] = chash[h3] = chash[h4] = q;

		if (best > 2)
			assert(dist > 0);

		// see if our best match qualifies
		if (best < 3) { // fast path literals
			++q;
		} else if (best > 2  &&  best <= 0x80	&&  dist <= 0x100) {
			outliterals(lit_start, (int)(q-lit_start)); lit_start = (q += best);
			stb_out(0x80 + best-1);
			stb_out(dist-1);
		} else if (best > 5  &&  best <= 0x100   &&  dist <= 0x4000) {
			outliterals(lit_start, (int)(q-lit_start)); lit_start = (q += best);
			stb_out2(0x4000 + dist-1);
			stb_out(best-1);
		} else if (best > 7  &&  best <= 0x100   &&  dist <= 0x80000) {
			outliterals(lit_start, (int)(q-lit_start)); lit_start = (q += best);
			stb_out3(0x180000 + dist-1);
			stb_out(best-1);
		} else if (best > 8  &&  best <= 0x10000 &&  dist <= 0x80000) {
			outliterals(lit_start, (int)(q-lit_start)); lit_start = (q += best);
			stb_out3(0x100000 + dist-1);
			stb_out2(best-1);
		} else if (best > 9					  &&  dist <= 0x1000000) {
			if (best > 65536) best = 65536;
			outliterals(lit_start, (int)(q-lit_start)); lit_start = (q += best);
			if (best <= 0x100) {
				stb_out(0x06);
				stb_out3(dist-1);
				stb_out(best-1);
			} else {
				stb_out(0x04);
				stb_out3(dist-1);
				stb_out2(best-1);
			}
		} else {  // fallback literals if no match was a balanced tradeoff
			++q;
		}
	}

	// if we didn't get all the way, add the rest to literals
	if (q-start < length)
		q = start+length;

	// the literals are everything from lit_start to q
	*pending_literals = (int)(q - lit_start);

	stb__running_adler = stb_adler32(stb__running_adler, start, (stb_uint)(q - start));
	return (int)(q - start);
}

static int stb_compress_inner(stb_uchar *input, stb_uint length)
{
	int literals = 0;
	stb_uint len,i;

	stb_uchar **chash;
	chash = (stb_uchar**) malloc(stb__hashsize * sizeof(stb_uchar*));
	if (chash == NULL) return 0; // failure
	for (i=0; i < stb__hashsize; ++i)
		chash[i] = NULL;

	// stream signature
	stb_out(0x57); stb_out(0xbc);
	stb_out2(0);

	stb_out4(0);	   // 64-bit length requires 32-bit leading 0
	stb_out4(length);
	stb_out4(stb__window);

	stb__running_adler = 1;

	len = stb_compress_chunk(input, input, input+length, length, &literals, chash, stb__hashsize-1);
	assert(len == length);

	outliterals(input+length - literals, literals);

	free(chash);

	stb_out2(0x05fa); // end opcode

	stb_out4(stb__running_adler);

	return 1; // success
}

stb_uint stb__compress(stb_uchar *out, stb_uchar *input, stb_uint length)
{
	stb__out = out;
	stb__outfile = NULL;

	stb_compress_inner(input, length);

	return (stb_uint)(stb__out - out);
}

static unsigned int stb__decompress_length(const unsigned char *input)
{
	return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

static unsigned char *stb__barrier_out_e, *stb__barrier_out_b;
static const unsigned char *stb__barrier_in_b;
static unsigned char *stb__dout;
static void stb__match(const unsigned char *data, unsigned int length)
{
	// INVERSE of memmove... write each byte before copying the next...
	assert(stb__dout + length <= stb__barrier_out_e);
	if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
	if (data < stb__barrier_out_b) { stb__dout = stb__barrier_out_e+1; return; }
	while (length--) *stb__dout++ = *data++;
}

static void stb__lit(const unsigned char *data, unsigned int length)
{
	assert(stb__dout + length <= stb__barrier_out_e);
	if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
	if (data < stb__barrier_in_b) { stb__dout = stb__barrier_out_e+1; return; }
	memcpy(stb__dout, data, length);
	stb__dout += length;
}

#define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
#define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
#define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

static const unsigned char *stb_decompress_token(const unsigned char *i)
{
	if (*i >= 0x20) { // use fewer if's for cases that expand small
		if (*i >= 0x80)	   stb__match(stb__dout-i[1]-1, i[0] - 0x80 + 1), i += 2;
		else if (*i >= 0x40)  stb__match(stb__dout-(stb__in2(0) - 0x4000 + 1), i[2]+1), i += 3;
		else /* *i >= 0x20 */ stb__lit(i+1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
	} else { // more ifs for cases that expand large, since overhead is amortized
		if (*i >= 0x18)	   stb__match(stb__dout-(stb__in3(0) - 0x180000 + 1), i[3]+1), i += 4;
		else if (*i >= 0x10)  stb__match(stb__dout-(stb__in3(0) - 0x100000 + 1), stb__in2(3)+1), i += 5;
		else if (*i >= 0x08)  stb__lit(i+2, stb__in2(0) - 0x0800 + 1), i += 2 + (stb__in2(0) - 0x0800 + 1);
		else if (*i == 0x07)  stb__lit(i+3, stb__in2(1) + 1), i += 3 + (stb__in2(1) + 1);
		else if (*i == 0x06)  stb__match(stb__dout-(stb__in3(1)+1), i[4]+1), i += 5;
		else if (*i == 0x04)  stb__match(stb__dout-(stb__in3(1)+1), stb__in2(4)+1), i += 6;
	}
	return i;
}

static unsigned int stb__decompress(unsigned char *output, const unsigned char *i, unsigned int /*length*/)
{
	if (stb__in4(0) != 0x57bC0000) return 0;
	if (stb__in4(4) != 0)		  return 0; // error! stream is > 4GB
	const unsigned int olen = stb_decompress_length(i);
	stb__barrier_in_b = i;
	stb__barrier_out_e = output + olen;
	stb__barrier_out_b = output;
	i += 16;

	stb__dout = output;
	for (;;) {
		const unsigned char *old_i = i;
		i = stb_decompress_token(i);
		if (i == old_i) {
			if (*i == 0x05 && i[1] == 0xfa) {
				assert(stb__dout == output + olen);
				if (stb__dout != output + olen) return 0;
				if (stb_adler32(1, output, olen) != (unsigned int) stb__in4(2))
					return 0;
				return olen;
			} else {
				assert(0); /* NOTREACHED */
				return 0;
			}
		}
		assert(stb__dout <= output + olen);
		if (stb__dout > output + olen)
			return 0;
	}
}

int stb_compress(void* dst, const void* input, int size)
{
	return (int)stb__compress((stb_uchar*)dst, (stb_uchar*)input, (stb_uint)size);
}

int stb_decompress_length(const void* input)
{
	return (int)stb__decompress_length((const unsigned char*)input);
}

int stb_decompress(void* dst, const void* input, int size)
{
	return (int)stb__decompress((unsigned char*)dst, (const unsigned char*)input, size);
}

#endif // STB_COMPRESS_IMPLEMENTATION_ONCE
#endif // STB_COMPRESS_IMPLEMENTATION
