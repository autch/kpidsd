#include "stdafx.h"

#include <algorithm>

#include "KpiDopDecoder.h"

namespace
{
	// DoP open Standard v1.1: マーカーは 0x05 と 0xFA をフレームごとに交互に使う (全チャンネル同じ値)
	constexpr uint8_t DopMarker1 = 0x05;
	constexpr uint8_t DopMarker2 = 0xFA;
	constexpr uint8_t DsdSilence = 0x69;	// DSD の無音パターン
	constexpr size_t ChunkFrames = 8192;
}

bool KpiDopDecoder::setup(const KPI_MEDIAINFO* cpRequest)
{
	// 最終出力段までデータを加工させない
	info.dwFormatType = KPI_MEDIAINFO::FORMAT_DOP;
	// DSD64 (2.8224MHz) なら 176.4kHz。DSD128 以上は PCM の周波数を上げる方式 (0x06/0xF9 は使わない)
	info.dwSampleRate = source.SampleRate() / 16;
	// 24bit を要求されたときだけ 24bit。それ以外 (普通は -64) は 32bit INT で出す
	const int requested = cpRequest != nullptr ? cpRequest->nBitsPerSample : 0;
	info.nBitsPerSample = (requested == 0 || requested == 24) ? 24 : 32;
	bytesPerWord = static_cast<unsigned>(info.nBitsPerSample / 8);

	// 1 フレームはチャンネルあたり 2 バイト。最後の半端な 1 バイトは無音で埋める
	totalSamples = (source.BytesPerChannel() + 1) / 2;
	// マーカーは開いたときだけ初期化する
	marker = DopMarker1;
	frame = 0;
	return true;
}

void KpiDopDecoder::seek(uint64_t position)
{
	frame = position;
	source.Seek(frame * 2);
}

DWORD KpiDopDecoder::render(BYTE* pBuffer, DWORD dwSizeSample)
{
	const unsigned channels = info.dwChannels;
	const uint64_t bytesPerChannel = source.BytesPerChannel();
	BYTE* out = pBuffer;
	DWORD done = 0;

	while (done < dwSizeSample && frame < totalSamples)
	{
		const size_t frames = static_cast<size_t>(std::min<uint64_t>({ dwSizeSample - done, totalSamples - frame, ChunkFrames }));
		const size_t bytes = frames * 2;
		dsd.resize(bytes * channels);

		const size_t expected = static_cast<size_t>(std::min<uint64_t>(bytes, bytesPerChannel - frame * 2));
		const size_t got = source.Read(dsd.data(), bytes, bytes);
		size_t usable = frames;
		if (got < expected)
			usable = got / 2;	// 開いたときより短くなったファイル。読めた分で終える
		else
		{
			for (unsigned ch = 0; ch < channels; ch++)
				std::fill(dsd.begin() + ch * bytes + got, dsd.begin() + (ch + 1) * bytes, DsdSilence);
		}

		for (size_t i = 0; i < usable; i++)
		{
			for (unsigned ch = 0; ch < channels; ch++)
			{
				// 24bit のワード: [23:16] マーカー、[15:8] 古いほうの 8 サンプル、[7:0] 新しいほうの 8 サンプル。
				// 32bit ではその下に 0 を 8 ビット置く。リトルエンディアンで書く
				const uint8_t* p = dsd.data() + ch * bytes + i * 2;
				if (bytesPerWord == 4)
					*out++ = 0;
				*out++ = p[1];
				*out++ = p[0];
				*out++ = marker;
			}
			marker = marker == DopMarker1 ? DopMarker2 : DopMarker1;
		}
		frame += usable;
		done += static_cast<DWORD>(usable);
		if (usable < frames)
		{
			truncate(frame);
			break;
		}
	}
	return done;
}
