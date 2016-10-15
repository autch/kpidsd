#pragma once

#include "Downsampler.h"
#include "Noiseshaper.h"

#include <vector>
#include <soxr.h>

class DSD2PCM
{
private:
	long dsd_samples_per_sec;
	long desired_sample_rate;
	int channels;
	int wordSize;
	bool flush_phase;

	Downsampler_Precalc	ds_pc;
	std::vector<Downsampler> ds;
	std::vector<Noiseshaper> ns;

	std::vector<float> buffer;
	size_t buffer_stored = 0;

	soxr_t soxr;

public:
	DSD2PCM() : ds_pc(), soxr(NULL), flush_phase(false)
	{
		Close();
	}

	~DSD2PCM()
	{
		Close();
	}

	void Reset();
	void Open(long DSDsamplesPerSec, int desiredSampleRate, int ch, int desiredWordSize);
	void Close();

	// return: # of samples written
	size_t writeFinal(std::vector<float> resample_data, size_t odone, uint8_t* out);

	// convert channels, src is interlaced as LRLRLR...
	// return: # of samples rendered
	size_t Render(uint8_t* src, size_t src_size, size_t block_size, int lsbf, uint8_t* dst, size_t samplesToRender);
		
	bool isInFlush() const {
		return flush_phase;
	}

	size_t RenderLast();
	size_t RenderFlush(uint8_t* dst, size_t samplesToRender);
};
