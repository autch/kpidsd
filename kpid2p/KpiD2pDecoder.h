#pragma once

#include <vector>

#include "DsdDecoder.h"
#include "DsdDecimator.h"

// DSD を PCM (浮動小数点) にして出す
class KpiD2pDecoder : public DsdDecoderBase
{
protected:
	bool setup(const KPI_MEDIAINFO* cpRequest) override;
	DWORD render(BYTE* pBuffer, DWORD dwSizeSample) override;
	void seek(uint64_t position) override;

private:
	// チャンネルあたりのバイト位置 [begin, end) を input に用意する。用意できなければ false
	bool fill(int64_t begin, int64_t end);

	DsdDecimator decimator;
	uint64_t sample = 0;		// 次に出すサンプル

	// input[ch] の先頭はチャンネルあたり inputStart バイト目。曲の前後は DSD の無音で埋める
	std::vector<std::vector<uint8_t>> input;
	int64_t inputStart = 0;
	uint64_t sourcePosition = UINT64_MAX;	// source が次に返すバイト位置
	std::vector<uint8_t> readBuffer;
};
