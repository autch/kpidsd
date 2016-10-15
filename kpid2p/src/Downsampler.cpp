#include "stdafx.h"
#include "Downsampler.h"

void Downsampler_Precalc::precalc(void)
{
	int t, e, m, k;
	double acc;

	for (t = 0, e = 0; t < 256; ++t) {
		bitreverse[t] = e;
		for (m = 128; m && !((e ^= m) & m); m >>= 1)
			;
	}
	for (t = 0; t < CTABLES; ++t) {
		k = HTAPS - t * 8;
		if (k > 8) k = 8;
		for (e = 0; e < 256; ++e) {
			acc = 0.0;
			for (m = 0; m < k; ++m) {
				acc += (((e >> (7 - m)) & 1) * 2 - 1) * htaps[t * 8 + m];
			}
			ctables[CTABLES - 1 - t][e] = (float)acc;
		}
	}
}

long Downsampler::translate(
	size_t samples,
	const uint8_t* src, ptrdiff_t src_stride,
	int lsbf,
	float *dst, ptrdiff_t dst_stride)
{
	unsigned ffp;
	unsigned i;
	unsigned bite1, bite2;
	uint8_t* p;
	double acc;
	long xlated = 0;

	ffp = fifopos;
	lsbf = lsbf ? 1 : 0;
	while (samples-- > 0) {
		bite1 = *src & 0xFFu;
		fifo[ffp] = lsbf ? pc.BitReverse(bite1) : bite1; src += src_stride;

		p = fifo + ((ffp - CTABLES) & FIFOMASK);
		*p = pc.BitReverse(*p & 0xFF);
		acc = 0;
		for (i = 0; i < CTABLES; ++i) {
			bite1 = fifo[(ffp - i) & FIFOMASK] & 0xFF;
			bite2 = fifo[(ffp - (CTABLES * 2 - 1) + i) & FIFOMASK] & 0xFF;
			acc += pc.CTables(i, bite1) + pc.CTables(i, bite2);
		}
		*dst = (float)acc; dst += dst_stride;
		xlated++;
		ffp = (ffp + 1) & FIFOMASK;
	}
	fifopos = ffp;

	return xlated;
}
