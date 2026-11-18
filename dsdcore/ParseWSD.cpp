// WSD (1bit オーディオコンソーシアム WSD 1.1、ビッグエンディアン)
//
//   0  General Information (32): "1bit"  reserved (4)  version (1)  reserved (3)  file size (8)
//                                text offset (4)  data offset (4)  reserved (4)
//  32  Data Spec (96): playback time (4)  sampling frequency (4)  reserved (4)  channels (1) ...
//      version は 09h、10h、11h のように上位 4 ビットが主バージョン。2.0 以上はポインタの意味や
//      文字コードが変わってよいことになっているので開かない
//      channels は下位 4 ビットだけがチャンネル数 (上位 4 ビットは予約)
//      channel assignment (48〜51) からスピーカーの配置を決め、WAV の順に並べ替える
//      Text (1920): text offset から。0 ならテキストはない
//      データ: data offset からファイルの終わりまで、1 バイトずつチャンネルが交互に並ぶ (MSB first)

#include "stdafx.h"

#include "DsdLayout.h"
#include "Reader.h"

namespace
{
	constexpr uint64_t HeaderSize = 32 + 96;
	constexpr size_t TextSize = 1920;

	// 固定長の欄から、NUL より前を取り出して末尾の空白を除く
	std::string field(const uint8_t* text, size_t offset, size_t size)
	{
		const uint8_t* p = text + offset;
		size_t length = 0;
		while (length < size && p[length] != 0)
			length++;
		while (length > 0 && p[length - 1] == ' ')
			length--;
		return std::string(reinterpret_cast<const char*>(p), length);
	}
}

bool ParseWSD(Reader& reader, DsdLayout& layout)
{
	uint8_t h[HeaderSize];
	if (!reader.ReadAt(0, h, sizeof h) || std::memcmp(h, "1bit", 4) != 0)
		return false;
	if ((h[8] >> 4) >= 2)
		return false;

	// ヘッダの file size は見ない。データはヘッダ (とテキスト) の後ろからファイルの終わりまで
	const uint64_t fileSize = reader.Size();
	const uint64_t textOffset = ReadBE32(h + 20);
	const uint64_t dataOffset = ReadBE32(h + 24);
	if (dataOffset < HeaderSize || dataOffset >= fileSize)
		return false;
	if (textOffset != 0 && (textOffset < HeaderSize || textOffset + TextSize > dataOffset))
		return false;

	layout.format = DsdFormat::WSD;
	layout.sampleRate = ReadBE32(h + 36);
	layout.channels = h[44] & 0x0F;
	layout.dataOffset = dataOffset;
	layout.blockSize = 1;
	if (layout.channels == 0)
		return false;
	layout.bytesPerChannel = (fileSize - dataOffset) / layout.channels;

	// ストリームのチャンネルは、Ch_Asn (48〜51) の立っているビットの順 (各バイトは上位ビットから、
	// バイトは 48 から順に) に並ぶ。5.1ch なら Lf Cf Rf LFE Lr Rr なので、WAV の順に並べ替える。
	// Lr-middle / Rr-middle、予約のビット、サイドの欄 (49、50) は WAV に当たる位置がないので 0 (分からない)
	{
		using namespace Speaker;
		static constexpr uint32_t front[8] = { 0, FL, FLC, FC, FRC, FR, 0, LFE };	// RBP 48、上位ビットから
		static constexpr uint32_t rear[8] = { 0, BL, 0, BC, 0, BR, 0, 0 };			// RBP 51
		uint32_t speakers[MaxDsdChannels];
		size_t count = 0;
		bool ok = true;
		for (int byte = 0; byte < 4 && ok; byte++)
		{
			for (int bit = 0; bit < 8; bit++)
			{
				if ((h[48 + byte] & (0x80 >> bit)) == 0)
					continue;
				const uint32_t position = byte == 0 ? front[bit] : byte == 3 ? rear[bit] : 0;
				if (position == 0 || count == MaxDsdChannels)
				{
					ok = false;
					break;
				}
				speakers[count++] = position;
			}
		}
		if (ok)
			SetSpeakerLayout(layout, speakers, count);
	}

	layout.encoding = TextEncoding::CP932;
	if (textOffset != 0)
	{
		uint8_t text[TextSize];
		if (!reader.ReadAt(textOffset, text, sizeof text))
			return false;
		layout.text.title = field(text, 0, 128);
		layout.text.composer = field(text, 128, 128);
		layout.text.artist = field(text, 384, 128);
		layout.text.album = field(text, 512, 128);
		layout.text.genre = field(text, 640, 32);
		layout.text.date = field(text, 672, 32);
		layout.text.comment = field(text, 736, 512);
	}
	return true;
}
