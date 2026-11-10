#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "kpi.h"

class CDSFFile;
class CDFFFile;
class CWSDFile;
class KpiFileAdapter;

// DSF / DSDIFF / WSD のファイルを開き、DSD のデータをチャンネルごとのバイト列として読む。
// 読んだバイトは、どの形式でも MSB first (上位ビットほど古いサンプル) にそろえる。
// 位置と長さはすべて「1 チャンネルあたりのバイト数」(8 サンプルで 1 バイト) で数える。
class DsdSource
{
public:
	enum class Format { None, DSF, DFF, WSD };

	DsdSource();
	~DsdSource();
	DsdSource(const DsdSource&) = delete;
	DsdSource& operator=(const DsdSource&) = delete;

	// ファイルの先頭 4 バイトから形式を判定する。対応しないなら Format::None
	static Format Detect(IKpiFile* file);
	// DSD のサンプリング周波数として受け付けるか (44.1kHz または 48kHz の 64 / 128 / 256 / 512 / 1024 倍)
	static bool IsSupportedRate(uint32_t rate);

	// ヘッダを調べて開く。対応しないファイルなら false。成功したら file を AddRef して持つ
	bool Open(IKpiFile* file);
	void Close();

	Format GetFormat() const { return format; }
	uint32_t SampleRate() const { return sampleRate; }
	uint32_t Channels() const { return channels; }
	uint64_t BytesPerChannel() const { return bytesPerChannel; }

	// 次の Read を、チャンネルあたり pos バイト目から始める
	void Seek(uint64_t pos);
	// チャンネルあたり最大 bytes バイト読み、ch チャンネル目を dst[ch * stride + i] に入れる。
	// 読めたバイト数を返す (曲の終わりやファイルの途中切れでは bytes より少ない)
	size_t Read(uint8_t* dst, size_t stride, size_t bytes);

	// タグを設定する。DSF と、ID3 チャンクのある DSDIFF は ID3v2 の範囲を本体に解析させる。
	// それ以外は本体のタグ解析を止めてから、ファイルの文字列を設定する
	void SetTags(IKpiTagInfo* pTagInfo, DWORD dwTagGetFlags);

private:
	bool openDSF();
	bool openDFF();
	bool openWSD();
	bool readAt(uint64_t offset, uint8_t* dst, size_t bytes);
	size_t readInterleaved(uint8_t* dst, size_t stride, size_t bytes);
	size_t readBlocks(uint8_t* dst, size_t stride, size_t bytes);
	void delegateID3v2(IKpiTagInfo* pTagInfo, uint64_t start, uint64_t end, DWORD dwTagGetFlags);
	void setDsdInfo(IKpiTagInfo* pTagInfo);

	IKpiFile* kpiFile = nullptr;
	std::unique_ptr<KpiFileAdapter> adapter;
	std::unique_ptr<CDSFFile> dsf;
	std::unique_ptr<CDFFFile> dff;
	std::unique_ptr<CWSDFile> wsd;

	Format format = Format::None;
	uint32_t sampleRate = 0;
	uint32_t channels = 0;
	uint64_t bytesPerChannel = 0;

	uint64_t dataOffset = 0;	// DSD データの先頭のファイル位置
	uint64_t dataEnd = 0;		// 使う DSD データの終わりのファイル位置 (DSF のブロックの終わり)
	uint32_t blockSize = 0;		// DSF のチャンネルあたりのブロック長 (DSDIFF と WSD は 1 バイトずつ交互)
	bool lsbFirst = false;		// DSF の bits_per_sample == 1

	uint64_t position = 0;		// 次に読むチャンネルあたりのバイト位置
	uint64_t filePosition = UINT64_MAX;	// IKpiFile の今の位置 (分からなければ UINT64_MAX)
	std::vector<uint8_t> raw;	// ファイルから読んだまま (インターリーブされた) のデータ
	uint64_t rawBlock = UINT64_MAX;	// DSF: raw に入っているブロックの番号
};
