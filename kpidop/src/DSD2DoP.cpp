#include "stdafx.h"
#include "DSD2DoP.h"

namespace {
	// http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64BitsDiv
	inline BYTE reverse(BYTE b)
	{
		return ((b * 0x0202020202ULL & 0x010884422010ULL) % 1023) & 0xff;
	}
}

void DSD2DoP::Open(int ch, int desiredWordSize)
{
	channels = ch;
	word_size = desiredWordSize;
	marker = DOP_MARKER1;

	frame_offset = desiredWordSize == 24 ? 1 : 0;
	bytes_to_write = desiredWordSize >> 3;
}

void DSD2DoP::Reset()
{
	marker = DOP_MARKER1;
}

size_t DSD2DoP::Render(uint8_t* src, size_t src_size, size_t block_size, int lsbf, uint8_t* dst, size_t samplesToRender)
{
	uint8_t frame[4] = { 0, 0, 0, 0 };
	int src_stride = (block_size > 1) ? 1 : channels;
	size_t samplesWritten = 0;

	for (DWORD dwSamplePos = 0; dwSamplePos < src_size / channels && samplesToRender > 0; dwSamplePos += 2)
	{
		for (int ch = 0; ch < channels; ch++)
		{
			PBYTE p = src + ch * block_size + dwSamplePos * src_stride;
			PBYTE pp = src + ch * block_size + (dwSamplePos + 1) * src_stride;

			frame[3] = marker;
			frame[2] = lsbf ? reverse(*p) : *p;
			frame[1] = lsbf ? reverse(*pp) : *pp;
			memcpy(dst, frame + frame_offset, bytes_to_write);
			dst += bytes_to_write;
		}
		samplesToRender--;
		samplesWritten++;
		marker ^= 0xff;
	}
	return samplesWritten;
}
