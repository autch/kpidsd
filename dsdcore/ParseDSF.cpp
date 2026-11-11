// DSF (Sony "DSF File Format Specification" 1.01、リトルエンディアン)
//
//   0  "DSD "  chunk size (8) = 28  file size (8)  ID3v2 pointer (8)
//  28  "fmt "  chunk size (8) = 52 (仕様では "Usually 52bytes")  version (4) = 1  format id (4) = 0
//              channel type (4)  channels (4)  sampling frequency (4)  bits per sample (4) = 1 / 8
//              sample count (8)  block size per channel (4) = 4096  reserved (4)
//  28 + fmt の長さ (ふつうは 80)
//      "data"  chunk size (8)  データ (チャンネルごとのブロックが交互に並ぶ)
//      ID3v2 (ID3v2 pointer から、ファイルの終わりまで)

#include "stdafx.h"

#include "DsdLayout.h"
#include "Reader.h"

namespace
{
	constexpr size_t DsdChunkSize = 28;
	constexpr size_t FmtSize = 52;			// fmt チャンクのうち使う部分 (ヘッダを含む)
	constexpr uint64_t MaxFmtSize = 4096;	// これより長い fmt チャンクは壊れているとみなす
}

bool ParseDSF(Reader& reader, DsdLayout& layout)
{
	uint8_t h[DsdChunkSize + FmtSize];
	if (!reader.ReadAt(0, h, sizeof h))
		return false;
	// ヘッダの file size は見ない (途中で切れたファイルは読める分だけにする)
	if (std::memcmp(h, "DSD ", 4) != 0 || ReadLE64(h + 4) != DsdChunkSize)
		return false;
	// fmt の長さは 52 に限らない。後ろに足されたものは読まずに飛ばす
	const uint64_t fmtSize = ReadLE64(h + 32);
	if (std::memcmp(h + 28, "fmt ", 4) != 0 || fmtSize < FmtSize || fmtSize > MaxFmtSize)
		return false;
	if (ReadLE32(h + 40) != 1 || ReadLE32(h + 44) != 0)	// version 1、DSD raw
		return false;
	const uint32_t channelType = ReadLE32(h + 48);
	if (channelType < 1 || channelType > 7)
		return false;
	const uint32_t bitsPerSample = ReadLE32(h + 60);
	if (bitsPerSample != 1 && bitsPerSample != 8)
		return false;
	const uint64_t dataHeader = DsdChunkSize + fmtSize;
	uint8_t d[12];
	if (!reader.ReadAt(dataHeader, d, sizeof d) || std::memcmp(d, "data", 4) != 0)
		return false;
	const uint64_t headerSize = dataHeader + sizeof d;

	layout.format = DsdFormat::DSF;
	layout.channels = ReadLE32(h + 52);
	layout.sampleRate = ReadLE32(h + 56);
	layout.lsbFirst = bitsPerSample == 1;
	layout.blockSize = ReadLE32(h + 72);
	layout.dataOffset = headerSize;
	if (layout.channels == 0 || layout.blockSize == 0)
		return false;

	// data チャンクの長さは 12 バイトのヘッダを含む。使うのはファイルに収まる完全なブロックだけ
	const uint64_t fileSize = reader.Size();
	const uint64_t chunkSize = ReadLE64(d + 4);
	const uint64_t declared = chunkSize >= 12 ? chunkSize - 12 : 0;
	const uint64_t available = fileSize - headerSize;
	uint64_t dataBytes = std::min(declared, available);
	const uint64_t blockBytes = static_cast<uint64_t>(layout.blockSize) * layout.channels;

	// ID3v2 は、ポインタがヘッダより後ろを指し、その先が "ID3" で始まるときだけ使う。
	// その位置を音のデータの終わりとみなしてから、完全なブロックを数える。音が途中で切れ、
	// 直後に長い ID3v2 があると、宣言どおりの長さがファイルに収まってしまい、ID3v2 を音として読むため
	const uint64_t pointer = ReadLE64(h + 20);
	uint8_t magic[3];
	if (pointer >= headerSize && pointer < fileSize && reader.ReadAt(pointer, magic, sizeof magic)
		&& std::memcmp(magic, "ID3", 3) == 0)
	{
		dataBytes = std::min(dataBytes, pointer - headerSize);
		layout.id3Offset = pointer;
		layout.id3Size = fileSize - pointer;
	}

	const uint64_t blocks = dataBytes / blockBytes;
	const uint64_t sampleCount = ReadLE64(h + 64);
	layout.bytesPerChannel = std::min(sampleCount / 8 + (sampleCount % 8 != 0), blocks * layout.blockSize);
	layout.encoding = TextEncoding::None;
	return true;
}
