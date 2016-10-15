#include "stdafx.h"
#include "DSD2PCM.h"

namespace {
	long myround(float x)
	{
		return static_cast<long>(x + (x >= 0 ? 0.5f : -0.5f));
	}

	template<typename T>
	struct id { typedef T type; };

	template<typename T>
	T clip(
		T min,
		T v,
		T max)
	{
		if (v<min) return min;
		if (v>max) return max;
		return v;
	}

	void write_intel16(uint8_t* ptr, int16_t word)
	{
		ptr[0] = word & 0xFF;
		ptr[1] = (word >> 8) & 0xFF;
	}

	void write_intel24(uint8_t* ptr, int32_t word)
	{
		ptr[0] = word & 0xFF;
		ptr[1] = (word >> 8) & 0xFF;
		ptr[2] = (word >> 16) & 0xFF;
	}

	void write_intel32(uint8_t* ptr, int32_t word)
	{
		ptr[0] = word & 0xFF;
		ptr[1] = (word >> 8) & 0xFF;
		ptr[2] = (word >> 16) & 0xFF;
		ptr[3] = (word >> 24) & 0xFF;
	}
}

void DSD2PCM::Reset()
{
	buffer.clear();
	buffer_stored = 0;

	for (int ch = 0; ch < channels; ++ch) {
		ds[ch].reset();
		ns[ch].reset();
	}
	flush_phase = false;

	soxr_clear(soxr);
}

void DSD2PCM::Open(long DSDsamplesPerSec, int desiredSampleRate, int ch, int desiredWordSize)
{
	dsd_samples_per_sec = DSDsamplesPerSec;
	channels = ch;
	wordSize = desiredWordSize;
	desired_sample_rate = desiredSampleRate;

	ds.clear();
	ns.clear();
	buffer.clear();
	buffer_stored = 0;
	flush_phase = false;

	for (int i = 0; i < ch; ++i) {
		ds.push_back(Downsampler(ds_pc));
		ns.push_back(Noiseshaper());
	}
	if (soxr != NULL) {
		soxr_delete(soxr);
	}
	{
		soxr_error_t e = NULL;
		soxr_runtime_spec_t spec = soxr_runtime_spec(0);
		soxr = soxr_create(DSDsamplesPerSec / 8, desiredSampleRate, ch, &e, NULL, NULL, &spec);
		if (e != NULL) {
			OutputDebugStringA((char*)e);
			DebugBreak();
		}

	}
}

void DSD2PCM::Close()
{
	dsd_samples_per_sec = channels = 0;
	ds.clear();
	ns.clear();
	buffer.clear();
	buffer_stored = 0;

	if (soxr != NULL) {
		soxr_delete(soxr);
		soxr = NULL;
	}
}

// return: # of samples written
size_t DSD2PCM::writeFinal(std::vector<float> resample_data, size_t odone, uint8_t* out)
{
	uint8_t* op = out;

	switch (wordSize) {
	case 16:
		for (int s = 0; s < odone * channels; ) {
			for (int ch = 0; ch < channels; ++ch) {
				float r = resample_data[s] * 32768.0f + ns[ch].get();
				long smp = clip<long>(-32768, myround(r), 32767);
				ns[ch].update(clip<long>(-1, smp - r, 1));
				write_intel16(op, smp);
				op += 2;
				++s;
			}
		}
		break;
	case 24:
		for (int s = 0; s < odone * channels; ) {
			for (int ch = 0; ch < channels; ++ch) {
				float r = resample_data[s] * 8388608.0f;
				long smp = clip<long>(-8388608, myround(r), 8388607);
				write_intel24(op, smp);
				op += 3;
				++s;
			}
		}
		break;
	case 32:
		for (int s = 0; s < odone * channels; ) {
			for (int ch = 0; ch < channels; ++ch) {
				float r = resample_data[s] * (1 << 31);
				long smp = clip<long>(INT32_MIN, myround(r), INT32_MAX);
				write_intel32(op, smp);
				op += 4;
				++s;
			}
		}
		break;
	}
	return odone;
}

// convert channels, src is interlaced as LRLRLR...
// return: # of samples rendered
size_t DSD2PCM::Render(uint8_t* src, size_t src_size, size_t block_size, int lsbf, uint8_t* dst, size_t samplesToRender)
{
	std::vector<float> float_data(src_size, 0), resample_data(src_size, 0);
	size_t idone = 0, odone = 0;

	if (flush_phase) {
		return 0; // RenderFlush(dst, samplesToRender);
	}

	for (int ch = 0; ch < channels; ++ch) {
		ds[ch].translate(src_size / channels, src + ch * block_size, (block_size > 1) ? 1 : channels, lsbf, float_data.data() + ch, channels);
	}

	soxr_process(soxr, float_data.data(), src_size / channels, &idone, resample_data.data(), src_size / channels, &odone);
	if (odone > 0) {
		buffer.insert(buffer.end(), resample_data.begin(), resample_data.begin() + odone * channels);
		buffer_stored += odone;
	}
	if (buffer_stored >= samplesToRender) {
		size_t samplesWritten = writeFinal(buffer, samplesToRender, dst);
		if (samplesWritten > 0) {
			buffer.erase(buffer.begin(), buffer.begin() + samplesToRender * channels);
			buffer_stored -= samplesWritten;

			return samplesWritten;
		}
	}
	return 0;
}

size_t DSD2PCM::RenderLast()
{
	const int src_size = 4096;
	std::vector<float> resample_data(src_size, 0);

	if (flush_phase)
		return buffer_stored;

	size_t idone = 0, odone = 0;
	do {
		soxr_process(soxr, NULL, 0, &idone, resample_data.data(), src_size / channels, &odone);
		if (odone > 0) {
			buffer.insert(buffer.end(), resample_data.begin(), resample_data.begin() + odone * channels);
			buffer_stored += odone;
		}
	} while (odone > 0);

	flush_phase = true;

	return buffer_stored;
}

size_t DSD2PCM::RenderFlush(uint8_t* dst, size_t samplesToRender)
{
	if (buffer_stored == 0) {
		return 0;
	}
	if (buffer_stored < samplesToRender) {
		samplesToRender = buffer_stored;
	}

	size_t samplesWritten = writeFinal(buffer, samplesToRender, dst);
	if (samplesWritten > 0) {
		buffer.erase(buffer.begin(), buffer.begin() + samplesToRender * channels);
		buffer_stored -= samplesWritten;
	}
	return samplesWritten;
}
