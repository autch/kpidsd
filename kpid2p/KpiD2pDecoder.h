#pragma once

#include <vector>

#include "kpi_impl.h"
#include "DsdSource.h"
#include "DsdDecimator.h"

// DSD を PCM (浮動小数点) にして出す
class KpiD2pDecoder : public KbKpiDecoderImpl
{
public:
	KpiD2pDecoder();
	~KpiD2pDecoder() override;

	// cpRequest は NULL のことがある (タグ取得用)
	bool Open(const KPI_MEDIAINFO* cpRequest, IKpiFile* pFile);

	DWORD  WINAPI Select(DWORD dwNumber, const KPI_MEDIAINFO** ppMediaInfo, IKpiTagInfo* pTagInfo, DWORD dwTagGetFlags) override;
	DWORD  WINAPI Render(BYTE* pBuffer, DWORD dwSizeSample) override;
	UINT64 WINAPI Seek(UINT64 qwPosSample, DWORD dwFlag) override;
	DWORD  WINAPI UpdateConfig(void* pvReserved) override { return 0; }

private:
	DWORD render(BYTE* pBuffer, DWORD dwSizeSample);
	// チャンネルあたりのバイト位置 [begin, end) を input に用意する。用意できなければ false
	bool fill(int64_t begin, int64_t end);

	DsdSource source;
	DsdDecimator decimator;
	KPI_MEDIAINFO info;
	uint64_t totalSamples = 0;	// 出力のサンプル数
	uint64_t sample = 0;		// 次に出すサンプル

	// input[ch] の先頭はチャンネルあたり inputStart バイト目。曲の前後は DSD の無音で埋める
	std::vector<std::vector<uint8_t>> input;
	int64_t inputStart = 0;
	uint64_t sourcePosition = UINT64_MAX;	// source が次に返すバイト位置
	std::vector<uint8_t> readBuffer;
};
