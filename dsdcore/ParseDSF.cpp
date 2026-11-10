// DSF (Sony "DSF File Format Specification" 1.01、リトルエンディアン)
//
//   0  "DSD "  chunk size (8) = 28  file size (8)  ID3v2 pointer (8)
//  28  "fmt "  chunk size (8) = 52  version (4) = 1  format id (4) = 0  channel type (4)
//              channels (4)  sampling frequency (4)  bits per sample (4) = 1 / 8
//              sample count (8)  block size per channel (4) = 4096  reserved (4)
//  80  "data"  chunk size (8)  データ (チャンネルごとのブロックが交互に並ぶ)
//      ID3v2 (ID3v2 pointer から、ファイルの終わりまで)

#include "stdafx.h"

#include "DsdLayout.h"
#include "Reader.h"

namespace
{
	constexpr size_t HeaderSize = 28 + 52 + 12;
}

bool ParseDSF(Reader& reader, DsdLayout& layout)
{
	uint8_t h[HeaderSize];
	if (!reader.ReadAt(0, h, sizeof h))
		return false;
	// ヘッダの file size は見ない (途中で切れたファイルは読める分だけにする)
	if (std::memcmp(h, "DSD ", 4) != 0 || ReadLE64(h + 4) != 28)
		return false;
	if (std::memcmp(h + 28, "fmt ", 4) != 0 || ReadLE64(h + 32) != 52)
		return false;
	if (ReadLE32(h + 40) != 1 || ReadLE32(h + 44) != 0)	// version 1、DSD raw
		return false;
	const uint32_t channelType = ReadLE32(h + 48);
	if (channelType < 1 || channelType > 7)
		return false;
	const uint32_t bitsPerSample = ReadLE32(h + 60);
	if (bitsPerSample != 1 && bitsPerSample != 8)
		return false;
	if (std::memcmp(h + 80, "data", 4) != 0)
		return false;

	layout.format = DsdFormat::DSF;
	layout.channels = ReadLE32(h + 52);
	layout.sampleRate = ReadLE32(h + 56);
	layout.lsbFirst = bitsPerSample == 1;
	layout.blockSize = ReadLE32(h + 72);
	layout.dataOffset = HeaderSize;
	if (layout.channels == 0 || layout.blockSize == 0)
		return false;

	// data チャンクの長さは 12 バイトのヘッダを含む。使うのはファイルに収まる完全なブロックだけ
	const uint64_t fileSize = reader.Size();
	const uint64_t chunkSize = ReadLE64(h + 84);
	const uint64_t dataBytes = std::min(chunkSize >= 12 ? chunkSize - 12 : 0, fileSize - HeaderSize);
	const uint64_t blockBytes = static_cast<uint64_t>(layout.blockSize) * layout.channels;
	const uint64_t blocks = dataBytes / blockBytes;
	const uint64_t sampleCount = ReadLE64(h + 64);
	layout.bytesPerChannel = std::min(sampleCount / 8 + (sampleCount % 8 != 0), blocks * layout.blockSize);

	// ID3v2 は、ポインタがデータより後ろにあり、"ID3" で始まるときだけ使う
	const uint64_t pointer = ReadLE64(h + 20);
	const uint64_t dataEnd = HeaderSize + blocks * blockBytes;
	uint8_t magic[3];
	if (pointer >= dataEnd && pointer < fileSize && reader.ReadAt(pointer, magic, sizeof magic)
		&& std::memcmp(magic, "ID3", 3) == 0)
	{
		layout.id3Offset = pointer;
		layout.id3Size = fileSize - pointer;
	}
	layout.encoding = TextEncoding::None;
	return true;
}
