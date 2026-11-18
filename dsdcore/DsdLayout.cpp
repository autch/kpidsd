#include "stdafx.h"

#include <algorithm>
#include <bit>

#include "DsdLayout.h"
#include "Reader.h"

namespace
{
	// DSF のブロック長の上限 (仕様では 4096 固定)
	constexpr uint32_t MaxBlockSize = 1 << 20;
}

DsdFormat DetectDsdFormat(Reader& reader)
{
	uint8_t signature[4];
	if (!reader.ReadAt(0, signature, sizeof signature))
		return DsdFormat::None;
	if (std::memcmp(signature, "DSD ", 4) == 0)
		return DsdFormat::DSF;
	if (std::memcmp(signature, "FRM8", 4) == 0)
		return DsdFormat::DFF;
	if (std::memcmp(signature, "1bit", 4) == 0)
		return DsdFormat::WSD;
	return DsdFormat::None;
}

bool ParseDsd(Reader& reader, DsdLayout& layout)
{
	layout = DsdLayout();
	bool ok = false;
	switch (DetectDsdFormat(reader))
	{
	case DsdFormat::DSF: ok = ParseDSF(reader, layout); break;
	case DsdFormat::DFF: ok = ParseDFF(reader, layout); break;
	case DsdFormat::WSD: ok = ParseWSD(reader, layout); break;
	default: break;
	}
	if (!ok || !ValidateDsdLayout(layout, reader.Size()))
	{
		layout = DsdLayout();
		return false;
	}
	return true;
}

bool IsSupportedDsdRate(uint32_t rate)
{
	for (const uint32_t base : { 44100u, 48000u })
	{
		for (uint32_t multiple = 64; multiple <= 1024; multiple *= 2)
		{
			if (rate == base * multiple)
				return true;
		}
	}
	return false;
}

void SetSpeakerLayout(DsdLayout& layout, const uint32_t* speakers, size_t count)
{
	layout.speakerConfig = 0;
	for (uint8_t i = 0; i < MaxDsdChannels; i++)
		layout.channelOrder[i] = i;
	if (count < 2 || count > MaxDsdChannels || count != layout.channels)
		return;

	uint32_t mask = 0;
	for (size_t i = 0; i < count; i++)
	{
		const uint32_t bit = speakers[i];
		// 1 ビットだけが立ち、ほかのチャンネルと重ならないこと
		if (bit == 0 || (bit & (bit - 1)) != 0 || (bit & ~Speaker::Defined) != 0 || (mask & bit) != 0)
			return;
		mask |= bit;
	}
	// ビットの小さい順に並べる
	std::array<uint8_t, MaxDsdChannels> order = {};
	for (size_t i = 0; i < count; i++)
		order[i] = static_cast<uint8_t>(i);
	std::sort(order.begin(), order.begin() + count, [&](uint8_t a, uint8_t b) { return speakers[a] < speakers[b]; });
	layout.speakerConfig = mask;
	layout.channelOrder = order;
}

bool ValidateDsdLayout(const DsdLayout& layout, uint64_t fileSize)
{
	if (layout.format == DsdFormat::None || !IsSupportedDsdRate(layout.sampleRate))
		return false;
	if (layout.channels == 0 || layout.channels > MaxDsdChannels)
		return false;

	// 配置は、立っているビットの数がチャンネル数と同じで、定義のあるビットだけ。並びは 0〜channels-1 の並べ替え
	if (layout.speakerConfig != 0 && (std::popcount(layout.speakerConfig) != static_cast<int>(layout.channels)
		|| (layout.speakerConfig & ~Speaker::Defined) != 0))
		return false;
	uint32_t seen = 0;
	for (uint32_t i = 0; i < layout.channels; i++)
	{
		if (layout.channelOrder[i] >= layout.channels)
			return false;
		seen |= 1u << layout.channelOrder[i];
	}
	if (seen != (1u << layout.channels) - 1)
		return false;
	if (layout.blockSize == 0 || layout.blockSize > MaxBlockSize)
		return false;

	// データはブロック単位でファイルに収まっていなければならない
	if (layout.dataOffset > fileSize)
		return false;
	const uint64_t blocks = (layout.bytesPerChannel + layout.blockSize - 1) / layout.blockSize;
	const uint64_t blockBytes = static_cast<uint64_t>(layout.blockSize) * layout.channels;
	if (blocks > (fileSize - layout.dataOffset) / blockBytes)
		return false;

	// ID3v2 はファイルに収まり、データと重ならない
	if (layout.id3Size != 0)
	{
		const uint64_t dataEnd = layout.dataOffset + blocks * blockBytes;
		if (layout.id3Offset > fileSize || layout.id3Size > fileSize - layout.id3Offset)
			return false;
		if (layout.id3Offset < dataEnd && layout.id3Offset + layout.id3Size > layout.dataOffset)
			return false;
	}
	return true;
}
