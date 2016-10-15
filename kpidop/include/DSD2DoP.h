#pragma once

#include "dop.h"

class DSD2DoP
{
private:
	uint8_t marker;
	int channels;
	int word_size;
	int frame_offset, bytes_to_write;

public:
	DSD2DoP()
	{

	}

	~DSD2DoP()
	{
		Close();
	}

	void Close()
	{

	}

	void Open(int ch, int desiredWordSize);
	void Reset();
	size_t Render(uint8_t* src, size_t src_size, size_t block_size, int lsbf, uint8_t* dst, size_t samplesToRender);
};
