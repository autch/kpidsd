#include "stdafx.h"

#include <algorithm>

#include "KpiD2pDecoder.h"

namespace
{
	constexpr uint8_t DsdSilence = 0x69;	// DSD の無音パターン
	constexpr size_t ChunkSamples = 4096;
}

KpiD2pDecoder::KpiD2pDecoder()
{
	kpi_InitMediaInfo(&info);
}

KpiD2pDecoder::~KpiD2pDecoder() = default;

bool KpiD2pDecoder::Open(const KPI_MEDIAINFO* cpRequest, IKpiFile* pFile)
{
	if (!source.Open(pFile))
		return false;

	const uint32_t decimation = DsdDecimator::ChooseDecimation(source.SampleRate(), cpRequest != nullptr ? cpRequest->dwSampleRate : 0);
	if (decimation == 0)
		return false;

	kpi_InitMediaInfo(&info);
	info.dwNumber = 1;
	info.dwCount = 1;
	info.dwFormatType = KPI_MEDIAINFO::FORMAT_PCM;
	info.dwSampleRate = source.SampleRate() / decimation;
	info.dwChannels = source.Channels();
	// 浮動小数点で出す。-32 を要求されたときだけ 32bit、それ以外は 64bit
	info.nBitsPerSample = (cpRequest != nullptr && cpRequest->nBitsPerSample == -32) ? -32 : -64;

	// 出力のサンプル k は、DSD の k * D サンプル目を中心にした窓から計算する
	const uint64_t step = decimation / 8;
	totalSamples = (source.BytesPerChannel() + step - 1) / step;
	info.qwLength = kpi_SampleTo100ns(totalSamples, info.dwSampleRate);
	info.qwLoop = 0;
	info.qwFadeOut = static_cast<UINT64>(-1);	// 本体の設定に従う
	info.dwSeekableFlags = KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE | KPI_MEDIAINFO::SEEK_FLAGS_ACCURATE | KPI_MEDIAINFO::SEEK_FLAGS_ROUGH;
	info.dwUnitSample = 0;

	// タグ取得用に開いたときはフィルタを作らない
	if (cpRequest != nullptr)
	{
		decimator.Design(source.SampleRate(), decimation);
		input.assign(info.dwChannels, {});
	}
	sample = 0;
	inputStart = 0;
	sourcePosition = UINT64_MAX;
	return true;
}

DWORD KpiD2pDecoder::Select(DWORD dwNumber, const KPI_MEDIAINFO** ppMediaInfo, IKpiTagInfo* pTagInfo, DWORD dwTagGetFlags)
{
	try
	{
		if (dwNumber != 1)
		{
			if (ppMediaInfo != nullptr)
				*ppMediaInfo = nullptr;
			return 0;
		}
		if (ppMediaInfo != nullptr)
			*ppMediaInfo = &info;
		if (pTagInfo != nullptr)
			source.SetTags(pTagInfo, dwTagGetFlags);
		return 1;
	}
	catch (...)
	{
		if (ppMediaInfo != nullptr)
			*ppMediaInfo = nullptr;
		return 0;
	}
}

UINT64 KpiD2pDecoder::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	// 出力は入力の決まった範囲だけから計算するので、サンプル単位で正確にシークできる
	sample = std::min<uint64_t>(qwPosSample, totalSamples);
	for (auto& buffer : input)
		buffer.clear();
	return sample;
}

DWORD KpiD2pDecoder::Render(BYTE* pBuffer, DWORD dwSizeSample)
{
	try
	{
		return render(pBuffer, dwSizeSample);
	}
	catch (...)
	{
		return 0;
	}
}

bool KpiD2pDecoder::fill(int64_t begin, int64_t end)
{
	const size_t channels = input.size();
	std::vector<uint8_t>& first = input[0];
	const int64_t have = inputStart + static_cast<int64_t>(first.size());

	// 前に用意した範囲と重ならなければ作り直す。重なれば、要らなくなった前の部分を捨てる
	if (first.empty() || begin < inputStart || begin > have)
	{
		for (auto& buffer : input)
			buffer.clear();
		inputStart = begin;
	}
	else if (begin > inputStart)
	{
		for (auto& buffer : input)
			buffer.erase(buffer.begin(), buffer.begin() + static_cast<ptrdiff_t>(begin - inputStart));
		inputStart = begin;
	}

	const int64_t total = static_cast<int64_t>(source.BytesPerChannel());
	int64_t pos = inputStart + static_cast<int64_t>(first.size());
	while (pos < end)
	{
		if (pos < 0 || pos >= total)
		{
			// 曲の前と後ろ
			const int64_t until = pos < 0 ? std::min<int64_t>(0, end) : end;
			for (auto& buffer : input)
				buffer.insert(buffer.end(), static_cast<size_t>(until - pos), DsdSilence);
			pos = until;
			continue;
		}

		const size_t bytes = static_cast<size_t>(std::min(end, total) - pos);
		if (sourcePosition != static_cast<uint64_t>(pos))
			source.Seek(static_cast<uint64_t>(pos));
		readBuffer.resize(bytes * channels);
		const size_t got = source.Read(readBuffer.data(), bytes, bytes);
		sourcePosition = static_cast<uint64_t>(pos) + got;
		for (size_t ch = 0; ch < channels; ch++)
		{
			const uint8_t* src = readBuffer.data() + ch * bytes;
			input[ch].insert(input[ch].end(), src, src + got);
		}
		if (got < bytes)
		{
			sourcePosition = UINT64_MAX;
			return false;	// 開いたときより短くなったファイル
		}
		pos += static_cast<int64_t>(got);
	}
	return true;
}

DWORD KpiD2pDecoder::render(BYTE* pBuffer, DWORD dwSizeSample)
{
	if (input.empty())
		return 0;

	const size_t channels = info.dwChannels;
	const int64_t step = static_cast<int64_t>(decimator.StepBytes());
	const int64_t window = static_cast<int64_t>(decimator.WindowBytes());
	float* out32 = reinterpret_cast<float*>(pBuffer);
	double* out64 = reinterpret_cast<double*>(pBuffer);
	DWORD done = 0;

	while (done < dwSizeSample && sample < totalSamples)
	{
		const size_t samples = static_cast<size_t>(std::min<uint64_t>({ dwSizeSample - done, totalSamples - sample, ChunkSamples }));
		// サンプル k の窓は [k * step - window / 2, k * step + window / 2)
		const int64_t begin = static_cast<int64_t>(sample) * step - window / 2;
		const int64_t end = begin + static_cast<int64_t>(samples - 1) * step + window;
		if (!fill(begin, end))
		{
			totalSamples = sample;
			break;
		}

		for (size_t i = 0; i < samples; i++)
		{
			for (size_t ch = 0; ch < channels; ch++)
			{
				const double value = decimator.Apply(input[ch].data() + i * step);
				if (info.nBitsPerSample == -32)
					*out32++ = static_cast<float>(value);
				else
					*out64++ = value;
			}
		}
		sample += samples;
		done += static_cast<DWORD>(samples);
	}
	return done;
}
