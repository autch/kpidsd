#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "kpi.h"
#include "DsdLayout.h"

class KpiFileReader;

// DSF / DSDIFF / WSD のファイルを開き、DSD のデータをチャンネルごとのバイト列として読む。
// 読んだバイトは、どの形式でも MSB first (上位ビットほど古いサンプル) にそろえる。
// 位置と長さはすべて「1 チャンネルあたりのバイト数」(8 サンプルで 1 バイト) で数える。
class DsdSource
{
public:
	DsdSource();
	~DsdSource();
	DsdSource(const DsdSource&) = delete;
	DsdSource& operator=(const DsdSource&) = delete;

	// ヘッダを調べて開く。対応しないファイルなら false。成功したら file を AddRef して持つ
	bool Open(IKpiFile* file);
	void Close();

	const DsdLayout& Layout() const { return layout; }
	uint32_t SampleRate() const { return layout.sampleRate; }
	uint32_t Channels() const { return layout.channels; }
	uint64_t BytesPerChannel() const { return layout.bytesPerChannel; }

	// 次の Read を、チャンネルあたり pos バイト目から始める
	void Seek(uint64_t pos);
	// チャンネルあたり最大 bytes バイト読み、ch チャンネル目を dst[ch * stride + i] に入れる。
	// 読めたバイト数を返す (曲の終わりやファイルの途中切れでは bytes より少ない)
	size_t Read(uint8_t* dst, size_t stride, size_t bytes);

	// タグを設定する。ID3v2 (DSF と、ID3 チャンクのある DSDIFF) は範囲を限定して本体に解析させる。
	// それ以外は本体のタグ解析を止めてから、ファイルの文字列を設定する
	void SetTags(IKpiTagInfo* pTagInfo, DWORD dwTagGetFlags);

private:
	IKpiFile* kpiFile = nullptr;
	std::unique_ptr<KpiFileReader> reader;
	DsdLayout layout;

	uint64_t position = 0;			// 次に読むチャンネルあたりのバイト位置
	std::vector<uint8_t> raw;		// ファイルから読んだまま (インターリーブされた) のデータ
	uint64_t rawBlock = UINT64_MAX;	// raw に入っているブロックの番号 (blockSize > 1 のとき)
};
