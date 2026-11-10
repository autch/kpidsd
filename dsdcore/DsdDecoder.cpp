#include "stdafx.h"

#include <algorithm>

#include "DsdDecoder.h"

DsdDecoderBase::DsdDecoderBase()
{
	kpi_InitMediaInfo(&info);
}

bool DsdDecoderBase::Open(const KPI_MEDIAINFO* cpRequest, IKpiFile* pFile)
{
	if (!source.Open(pFile))
		return false;

	kpi_InitMediaInfo(&info);
	info.dwNumber = 1;
	info.dwCount = 1;
	info.dwChannels = source.Channels();	// 2ch を超えても、そのまま伝える (並べ替えない)
	info.qwLoop = 0;
	info.qwFadeOut = static_cast<UINT64>(-1);	// 本体の設定に従う
	// 出力は位置だけで決まるので、サンプル単位で正確にシークできる
	info.dwSeekableFlags = KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE | KPI_MEDIAINFO::SEEK_FLAGS_ACCURATE | KPI_MEDIAINFO::SEEK_FLAGS_ROUGH;
	info.dwUnitSample = 0;	// Render には任意の長さで応じる

	totalSamples = 0;
	if (!setup(cpRequest) || info.dwSampleRate == 0)
		return false;
	info.qwLength = kpi_SampleTo100ns(totalSamples, info.dwSampleRate);
	return true;
}

DWORD DsdDecoderBase::Select(DWORD dwNumber, const KPI_MEDIAINFO** ppMediaInfo, IKpiTagInfo* pTagInfo, DWORD dwTagGetFlags)
{
	try
	{
		if (dwNumber != 1)
		{
			if (ppMediaInfo != nullptr)
				*ppMediaInfo = nullptr;
			return 0;
		}
		if (ppMediaInfo != nullptr)
			*ppMediaInfo = &info;
		if (pTagInfo != nullptr)
			source.SetTags(pTagInfo, dwTagGetFlags);
		return 1;
	}
	catch (...)
	{
		if (ppMediaInfo != nullptr)
			*ppMediaInfo = nullptr;
		return 0;
	}
}

DWORD DsdDecoderBase::Render(BYTE* pBuffer, DWORD dwSizeSample)
{
	try
	{
		return render(pBuffer, dwSizeSample);
	}
	catch (...)
	{
		return 0;
	}
}

UINT64 DsdDecoderBase::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	try
	{
		// 曲の終わりより先は、曲の終わりにする
		const uint64_t position = std::min<uint64_t>(qwPosSample, totalSamples);
		seek(position);
		return position;
	}
	catch (...)
	{
		return 0;
	}
}
