#include "stdafx.h"

#include "DsdLayout.h"
#include "Reader.h"

namespace
{
	// これより多いチャンネル数のファイルは壊れているとみなす
	constexpr uint32_t MaxChannels = 16;
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

bool ValidateDsdLayout(const DsdLayout& layout, uint64_t fileSize)
{
	if (layout.format == DsdFormat::None || !IsSupportedDsdRate(layout.sampleRate))
		return false;
	if (layout.channels == 0 || layout.channels > MaxChannels)
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
