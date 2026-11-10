#pragma once

#include <cstdint>
#include <string>

class Reader;

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

// 周波数、チャンネル数、ブロック長、データと ID3v2 の範囲がファイルに収まるかを確かめる
bool ValidateDsdLayout(const DsdLayout& layout, uint64_t fileSize);

// DSD のサンプリング周波数として受け付けるか (44.1kHz または 48kHz の 64 / 128 / 256 / 512 / 1024 倍)
bool IsSupportedDsdRate(uint32_t rate);
