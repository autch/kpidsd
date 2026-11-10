#pragma once

#include <vector>

#include "DsdDecoder.h"

// DSD を DoP (DSD Audio over PCM Frames, DoP open Standard v1.1) のフレームにして出す。
// 1 フレーム (PCM の 1 サンプル) にチャンネルあたり 16 ビットの DSD とマーカーを入れる
class KpiDopDecoder : public DsdDecoderBase
{
protected:
	bool setup(const KPI_MEDIAINFO* cpRequest) override;
	DWORD render(BYTE* pBuffer, DWORD dwSizeSample) override;
	void seek(uint64_t position) override;

private:
	uint64_t frame = 0;			// 次に出すフレーム
	uint8_t marker = 0x05;		// 次のフレームのマーカー。シークしても交互の並びを崩さない
	unsigned bytesPerWord = 4;	// 24bit なら 3、32bit なら 4
	std::vector<uint8_t> dsd;	// チャンネルごとに並べた DSD のバイト列
};
