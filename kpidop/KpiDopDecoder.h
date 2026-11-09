#pragma once

#include <vector>

#include "kpi_impl.h"
#include "DsdSource.h"

// DSD を DoP (DSD Audio over PCM Frames, DoP open Standard v1.1) のフレームにして出す。
// 1 フレーム (PCM の 1 サンプル) にチャンネルあたり 16 ビットの DSD とマーカーを入れる
class KpiDopDecoder : public KbKpiDecoderImpl
{
public:
	KpiDopDecoder();
	~KpiDopDecoder() override;

	// cpRequest は NULL のことがある (タグ取得用)
	bool Open(const KPI_MEDIAINFO* cpRequest, IKpiFile* pFile);

	DWORD  WINAPI Select(DWORD dwNumber, const KPI_MEDIAINFO** ppMediaInfo, IKpiTagInfo* pTagInfo, DWORD dwTagGetFlags) override;
	DWORD  WINAPI Render(BYTE* pBuffer, DWORD dwSizeSample) override;
	UINT64 WINAPI Seek(UINT64 qwPosSample, DWORD dwFlag) override;
	DWORD  WINAPI UpdateConfig(void* pvReserved) override { return 0; }

private:
	DWORD render(BYTE* pBuffer, DWORD dwSizeSample);

	DsdSource source;
	KPI_MEDIAINFO info;
	uint64_t totalFrames = 0;	// DoP のフレーム数 (最後の半端な 1 バイトは無音で埋める)
	uint64_t frame = 0;			// 次に出すフレーム
	uint8_t marker;				// 次のフレームのマーカー。シークしても交互の並びを崩さない
	unsigned bytesPerWord = 4;	// 24bit なら 3、32bit なら 4
	std::vector<uint8_t> dsd;	// チャンネルごとに並べた DSD のバイト列
};
