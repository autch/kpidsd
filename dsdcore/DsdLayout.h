#pragma once

#include <array>
#include <cstdint>
#include <string>

class Reader;

// 本体が受け付けるチャンネル数の上限 (7.1ch)。これより多いファイルは開かない
constexpr uint32_t MaxDsdChannels = 8;

// スピーカーの位置 (WAVEFORMATEXTENSIBLE::dwChannelMask と同じビット)
namespace Speaker
{
	constexpr uint32_t FL = 0x1, FR = 0x2, FC = 0x4, LFE = 0x8, BL = 0x10, BR = 0x20,
		FLC = 0x40, FRC = 0x80, BC = 0x100, SL = 0x200, SR = 0x400;
	constexpr uint32_t Defined = 0x3FFFF;	// SPEAKER_TOP_BACK_RIGHT (0x20000) まで
}

enum class DsdFormat { None, DSF, DFF, WSD };

// タグの文字列の文字コード (どの形式も仕様に規定がない)
enum class TextEncoding
{
	None,			// 文字列のタグを持たない (DSF は ID3v2 だけ)
	CP932,			// WSD: Shift_JIS 固定
	UTF8OrCP932,	// DSDIFF: UTF-8 として正しければ UTF-8、そうでなければ Shift_JIS
};

struct DsdText
{
	std::string title, artist, album, composer, genre, date, comment;
};

// パーサーが調べた「DSD のデータをどう読むか」と、タグ。形式によらず同じ形にする。
// 位置と長さはファイルの範囲に収まるように切り詰めてある (ValidateDsdLayout で確かめる)
struct DsdLayout
{
	DsdFormat format = DsdFormat::None;
	uint32_t sampleRate = 0;		// DSD のサンプリング周波数 (Hz)
	uint32_t channels = 0;

	// スピーカーの配置 (KPI_MEDIAINFO::dwSpeakerConfig)。0 はチャンネル数で決まる標準の配置。
	// 本体にはビットの小さい順にチャンネルを並べて渡す。channelOrder[i] は、i 番目に渡すチャンネルが
	// ファイルの何番目のチャンネルか。配置が分からなければ 0 で、ファイルの順のまま
	uint32_t speakerConfig = 0;
	std::array<uint8_t, MaxDsdChannels> channelOrder = { 0, 1, 2, 3, 4, 5, 6, 7 };

	// データは dataOffset から、チャンネルごとに blockSize バイトずつ交互に並ぶ。
	// DSF は 4096、DSDIFF と WSD は 1。DSF の最後のブロックは 0 で埋まっている
	uint64_t dataOffset = 0;
	uint32_t blockSize = 1;
	uint64_t bytesPerChannel = 0;	// 1 チャンネルあたりのバイト数 (8 サンプルで 1 バイト)
	bool lsbFirst = false;			// DSF の bits_per_sample == 1

	// 本体に解析させる ID3v2 タグの範囲。なければ id3Size == 0
	uint64_t id3Offset = 0;
	uint64_t id3Size = 0;

	TextEncoding encoding = TextEncoding::None;
	DsdText text;					// ファイルにあったままのバイト列
};

// ファイルの先頭 4 バイトから形式を判定する
DsdFormat DetectDsdFormat(Reader& reader);

// 形式を判定して読み、ValidateDsdLayout で確かめる。対応しないファイルなら false
bool ParseDsd(Reader& reader, DsdLayout& layout);

// 形式ごとのパーサー。ParseDsd から呼ぶ
bool ParseDSF(Reader& reader, DsdLayout& layout);
bool ParseDFF(Reader& reader, DsdLayout& layout);
bool ParseWSD(Reader& reader, DsdLayout& layout);

// ファイルの各チャンネルのスピーカーの位置 (Speaker の 1 ビット。分からなければ 0) から、
// speakerConfig と channelOrder を決める。1ch、位置の分からないチャンネル、重なりがあれば標準の配置のまま
void SetSpeakerLayout(DsdLayout& layout, const uint32_t* speakers, size_t count);

// 周波数、チャンネル数、ブロック長、配置と並び、データと ID3v2 の範囲がファイルに収まるかを確かめる
bool ValidateDsdLayout(const DsdLayout& layout, uint64_t fileSize);

// DSD のサンプリング周波数として受け付けるか (44.1kHz または 48kHz の 64 / 128 / 256 / 512 / 1024 倍)
bool IsSupportedDsdRate(uint32_t rate);
